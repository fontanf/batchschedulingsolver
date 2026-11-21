/**
 * Arc-flow MILP for 1|s_j, B|C_max
 *
 * Reference:
 *   Trindade et al. (2021), "Arc-flow approach for single
 *   batch-processing machine scheduling", Computers & Operations Research.
 *
 * The graph G has nodes V = {0, 1, ..., B}.
 *   A^J = job arcs  (i,j): exists job k with s_k = j-i, i < j
 *   A^L = loss arcs (i,B): for i in {1,...,B-1}
 *   A^F = feedback arc (B,0)
 *
 * One copy of G (an "arc-flow structure") is created per distinct
 * processing time P_t.  Each structure t may only carry jobs with
 * p_j <= P_t; the z_{l,t} variables transfer unassigned jobs of each
 * size to the next structure.
 *
 * Decision variables:
 *   f_{a,t}  : flow on job arc a in structure t
 *   y_{i,t}  : flow on loss arc (i,B) in structure t
 *   v_t      : number of batches (feedback flow) in structure t
 *   z_{l,t}  : jobs of size S_l not yet assigned after structure t  (t < delta)
 *
 * Objective: min sum_t P_t * v_t
 */

#include "batchschedulingsolver/algorithms/milp_arc_flow.hpp"

#include "mathoptsolverscmake/milp.hpp"

#include <algorithm>
#include <set>

using namespace batchschedulingsolver;

namespace
{

struct ArcFlowModel
{
    mathoptsolverscmake::MilpModel model;

    /** Machine capacity B. */
    Size capacity;

    /** Distinct processing times P_1 < P_2 < ... < P_delta. */
    std::vector<Time> processing_times;

    /** Distinct sizes S_1 < S_2 < ... < S_theta. */
    std::vector<Size> sizes;

    /**
     * Job arcs after arc reduction (Rule 1).
     * Each arc is stored as (from_node, to_node).
     */
    std::vector<std::pair<Size, Size>> job_arcs;

    /**
     * Adjacency list for job arcs indexed by from_node.
     * job_arcs_from[i] = list of (to_node, arc_index).
     */
    std::vector<std::vector<std::pair<Size, int>>> job_arcs_from;

    /**
     * Adjacency list for job arcs indexed by to_node.
     * job_arcs_to[j] = list of (from_node, arc_index).
     */
    std::vector<std::vector<std::pair<Size, int>>> job_arcs_to;

    /**
     * Loss arc nodes after arc reduction (Rule 2).
     * loss_arc_nodes[l] = node i for loss arc (i, B).
     */
    std::vector<Size> loss_arc_nodes;

    /**
     * loss_arc_idx[i] = index in loss_arc_nodes for node i (-1 if none).
     */
    std::vector<int> loss_arc_idx;

    /**
     * f_var[arc_idx][structure_id]: variable index for flow on job arc arc_idx
     * in arc-flow structure structure_id.
     */
    std::vector<std::vector<int>> f_var;

    /**
     * y_var[loss_idx][structure_id]: variable index for flow on loss arc loss_idx
     * in arc-flow structure structure_id.
     */
    std::vector<std::vector<int>> y_var;

    /**
     * v_var[structure_id]: variable index for feedback flow (number of batches)
     * in arc-flow structure structure_id.
     */
    std::vector<int> v_var;

    /**
     * z_var[size_idx][structure_id]: variable index for z_{l,t}.
     * Only defined for structure_id in {0, ..., delta-2}.
     */
    std::vector<std::vector<int>> z_var;
};

ArcFlowModel create_milp_model_arc_flow(const Instance& instance)
{
    ArcFlowModel model;
    model.model.objective_direction = mathoptsolverscmake::ObjectiveDirection::Minimize;

    model.capacity = instance.machine(0).capacity;

    // Compute distinct processing times and sizes.
    {
        std::set<Time> pt_set;
        std::set<Size> size_set;
        for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
            const Job& job = instance.job(job_id);
            pt_set.insert(job.processing_times[0]);
            size_set.insert(job.size);
        }
        model.processing_times.assign(pt_set.begin(), pt_set.end());
        model.sizes.assign(size_set.begin(), size_set.end());
    }

    int delta = model.processing_times.size();
    int theta = model.sizes.size();

    // Index maps for fast lookup.
    std::vector<int> size_to_idx(model.capacity + 1, -1);
    for (int size_id = 0; size_id < theta; ++size_id)
        size_to_idx[model.sizes[size_id]] = size_id;

    std::vector<int> pt_to_idx(model.processing_times.back() + 1, -1);
    for (int structure_id = 0; structure_id < delta; ++structure_id)
        pt_to_idx[model.processing_times[structure_id]] = structure_id;

    // nt[size_id][structure_id]      = number of jobs with size S_ell and p_j == P_t.
    // nt_plus[size_id][structure_id] = number of jobs with size S_ell and p_j <= P_t.
    // nj[structure_id]           = number of jobs with p_j == P_t.
    std::vector<std::vector<int>> nt(theta, std::vector<int>(delta, 0));
    std::vector<std::vector<int>> nt_plus(theta, std::vector<int>(delta, 0));
    std::vector<int> nj(delta, 0);

    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        const Job& job = instance.job(job_id);
        int size_id = size_to_idx[job.size];
        int structure_id = pt_to_idx[job.processing_times[0]];
        nt[size_id][structure_id]++;
        nj[structure_id]++;
    }
    for (int size_id = 0; size_id < theta; ++size_id) {
        nt_plus[size_id][0] = nt[size_id][0];
        for (int structure_id = 1; structure_id < delta; ++structure_id)
            nt_plus[size_id][structure_id] =
                nt_plus[size_id][structure_id - 1] + nt[size_id][structure_id];
    }

    // Build job arcs A^J and apply Rule 1 (BFS from node 0).
    model.job_arcs_from.resize(model.capacity + 1);
    model.job_arcs_to.resize(model.capacity + 1);

    std::vector<bool> reachable(model.capacity + 1, false);
    reachable[0] = true;
    std::vector<Size> bfs_queue = {0};

    for (int bfs_idx = 0; bfs_idx < (int)bfs_queue.size(); ++bfs_idx) {
        Size from_node = bfs_queue[bfs_idx];
        for (const Size& job_size : model.sizes) {
            Size to_node = from_node + job_size;
            if (to_node > model.capacity)
                continue;
            int arc_idx = model.job_arcs.size();
            model.job_arcs.push_back({from_node, to_node});
            model.job_arcs_from[from_node].push_back({to_node, arc_idx});
            model.job_arcs_to[to_node].push_back({from_node, arc_idx});
            if (!reachable[to_node]) {
                reachable[to_node] = true;
                bfs_queue.push_back(to_node);
            }
        }
    }

    int num_job_arcs = model.job_arcs.size();

    // Apply Rule 2: keep loss arc (node, B) only if node is incident to
    // at least one job arc.
    model.loss_arc_idx.assign(model.capacity + 1, -1);
    for (Size node = 1; node < model.capacity; ++node) {
        if (!model.job_arcs_from[node].empty() || !model.job_arcs_to[node].empty()) {
            model.loss_arc_idx[node] = model.loss_arc_nodes.size();
            model.loss_arc_nodes.push_back(node);
        }
    }
    int num_loss_arcs = model.loss_arc_nodes.size();

    // For each size index, list of arc indices with that size.
    std::vector<std::vector<int>> size_arcs(theta);
    for (int arc_id = 0; arc_id < num_job_arcs; ++arc_id) {
        Size arc_size = model.job_arcs[arc_id].second - model.job_arcs[arc_id].first;
        int size_id = size_to_idx[arc_size];
        size_arcs[size_id].push_back(arc_id);
    }

    // =========================================================================
    // Variables
    // =========================================================================

    // f_var[arc_id][structure_id]: flow on job arc arc_id in structure structure_id.
    // Upper bound (constraints 22): min(nj[structure_id], nt_plus[size_id][structure_id]).
    model.f_var.assign(num_job_arcs, std::vector<int>(delta));
    for (int arc_id = 0; arc_id < num_job_arcs; ++arc_id) {
        Size from = model.job_arcs[arc_id].first;
        Size to = model.job_arcs[arc_id].second;
        int size_id = size_to_idx[to - from];
        for (int structure_id = 0; structure_id < delta; ++structure_id) {
            model.f_var[arc_id][structure_id] =
                model.model.variables_lower_bounds.size();
            int ub = std::min(nj[structure_id], nt_plus[size_id][structure_id]);
            model.model.variables_names.push_back(
                    "f_{" + std::to_string(from) + ","
                    + std::to_string(to) + "," + std::to_string(structure_id) + "}");
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(ub);
            model.model.variables_types.push_back(
                    mathoptsolverscmake::VariableType::Integer);
            model.model.objective_coefficients.push_back(0);
        }
    }

    // y_var[loss_id][structure_id]: flow on loss arc loss_id in structure structure_id.
    // Upper bound (constraint 24): nj[structure_id].
    model.y_var.assign(num_loss_arcs, std::vector<int>(delta));
    for (int loss_id = 0; loss_id < num_loss_arcs; ++loss_id) {
        Size loss_node = model.loss_arc_nodes[loss_id];
        for (int structure_id = 0; structure_id < delta; ++structure_id) {
            model.y_var[loss_id][structure_id] =
                model.model.variables_lower_bounds.size();
            model.model.variables_names.push_back(
                    "y_{" + std::to_string(loss_node)
                    + "," + std::to_string(structure_id) + "}");
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(nj[structure_id]);
            model.model.variables_types.push_back(
                    mathoptsolverscmake::VariableType::Integer);
            model.model.objective_coefficients.push_back(0);
        }
    }

    // v_var[structure_id]: number of batches in structure structure_id.
    // Upper bound (constraint 23): nj[structure_id].
    // Objective coefficient: P_t.
    model.v_var.resize(delta);
    for (int structure_id = 0; structure_id < delta; ++structure_id) {
        model.v_var[structure_id] = model.model.variables_lower_bounds.size();
        model.model.variables_names.push_back(
                "v_{" + std::to_string(structure_id) + "}");
        model.model.variables_lower_bounds.push_back(0);
        model.model.variables_upper_bounds.push_back(nj[structure_id]);
        model.model.variables_types.push_back(
                mathoptsolverscmake::VariableType::Integer);
        model.model.objective_coefficients.push_back(
                static_cast<double>(model.processing_times[structure_id]));
    }

    // z_var[size_idx][structure_id]: jobs of size S_l not yet assigned after
    // structure structure_id. Only for structure_id in {0, ..., delta-2}.
    // Upper bound (constraint 25): nt_plus[size_id][structure_id].
    model.z_var.assign(theta, std::vector<int>(std::max(0, delta - 1)));
    for (int size_id = 0; size_id < theta; ++size_id) {
        for (int structure_id = 0; structure_id < delta - 1; ++structure_id) {
            model.z_var[size_id][structure_id] =
                model.model.variables_lower_bounds.size();
            model.model.variables_names.push_back(
                    "z_{" + std::to_string(size_id)
                    + "," + std::to_string(structure_id) + "}");
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(
                    nt_plus[size_id][structure_id]);
            model.model.variables_types.push_back(
                    mathoptsolverscmake::VariableType::Integer);
            model.model.objective_coefficients.push_back(0);
        }
    }

    // =========================================================================
    // Constraints
    // =========================================================================

    for (int structure_id = 0; structure_id < delta; ++structure_id) {

        // (18) Flow conservation at node 0:
        //   sum_{(0,j) in A^J} f_{0,j,t} - v_t = 0.
        {
            model.model.constraints_starts.push_back(
                    model.model.elements_variables.size());
            model.model.constraints_names.push_back(
                    "flow0_{" + std::to_string(structure_id) + "}");
            for (int ai = 0; ai < (int)model.job_arcs_from[0].size(); ++ai) {
                int arc_id = model.job_arcs_from[0][ai].second;
                model.model.elements_variables.push_back(
                        model.f_var[arc_id][structure_id]);
                model.model.elements_coefficients.push_back(1.0);
            }
            model.model.elements_variables.push_back(model.v_var[structure_id]);
            model.model.elements_coefficients.push_back(-1.0);
            model.model.constraints_lower_bounds.push_back(0);
            model.model.constraints_upper_bounds.push_back(0);
        }

        // (19) Flow conservation at intermediate nodes in {1, ..., B-1}:
        //   sum_{(i,node)} f_{i,node,t} - sum_{(node,k)} f_{node,k,t}
        //                               - y_{node,t} = 0.
        for (Size node = 1; node < model.capacity; ++node) {
            if (model.job_arcs_to[node].empty() && model.job_arcs_from[node].empty())
                continue;
            model.model.constraints_starts.push_back(
                    model.model.elements_variables.size());
            model.model.constraints_names.push_back(
                    "flow_{" + std::to_string(node)
                    + "," + std::to_string(structure_id) + "}");
            for (int ai = 0; ai < (int)model.job_arcs_to[node].size(); ++ai) {
                int arc_id = model.job_arcs_to[node][ai].second;
                model.model.elements_variables.push_back(
                        model.f_var[arc_id][structure_id]);
                model.model.elements_coefficients.push_back(1.0);
            }
            for (int ai = 0; ai < (int)model.job_arcs_from[node].size(); ++ai) {
                int arc_id = model.job_arcs_from[node][ai].second;
                model.model.elements_variables.push_back(
                        model.f_var[arc_id][structure_id]);
                model.model.elements_coefficients.push_back(-1.0);
            }
            int li = model.loss_arc_idx[node];
            if (li != -1) {
                model.model.elements_variables.push_back(
                        model.y_var[li][structure_id]);
                model.model.elements_coefficients.push_back(-1.0);
            }
            model.model.constraints_lower_bounds.push_back(0);
            model.model.constraints_upper_bounds.push_back(0);
        }

        // (20) Flow conservation at node B:
        //   sum_{(i,B) in A^J} f_{i,B,t} + sum_{(i,B) in A^L} y_{i,t} - v_t = 0.
        {
            model.model.constraints_starts.push_back(
                    model.model.elements_variables.size());
            model.model.constraints_names.push_back(
                    "flowB_{" + std::to_string(structure_id) + "}");
            for (int ai = 0;
                    ai < (int)model.job_arcs_to[model.capacity].size(); ++ai) {
                int arc_id = model.job_arcs_to[model.capacity][ai].second;
                model.model.elements_variables.push_back(
                        model.f_var[arc_id][structure_id]);
                model.model.elements_coefficients.push_back(1.0);
            }
            for (int loss_id = 0; loss_id < num_loss_arcs; ++loss_id) {
                model.model.elements_variables.push_back(
                        model.y_var[loss_id][structure_id]);
                model.model.elements_coefficients.push_back(1.0);
            }
            model.model.elements_variables.push_back(model.v_var[structure_id]);
            model.model.elements_coefficients.push_back(-1.0);
            model.model.constraints_lower_bounds.push_back(0);
            model.model.constraints_upper_bounds.push_back(0);
        }

        // (21) Job assignment for each size size_id:
        //   sum_{(i,j): j-i=S_{size_id}} f_{i,j,t} + [t<delta-1]*z_{size_id,t}
        //                                           - [t>0]*z_{size_id,t-1} = nt[size_id][t].
        for (int size_id = 0; size_id < theta; ++size_id) {
            model.model.constraints_starts.push_back(
                    model.model.elements_variables.size());
            model.model.constraints_names.push_back(
                    "assign_{" + std::to_string(size_id)
                    + "," + std::to_string(structure_id) + "}");
            for (int arc_id : size_arcs[size_id]) {
                model.model.elements_variables.push_back(
                        model.f_var[arc_id][structure_id]);
                model.model.elements_coefficients.push_back(1.0);
            }
            if (structure_id < delta - 1) {
                model.model.elements_variables.push_back(
                        model.z_var[size_id][structure_id]);
                model.model.elements_coefficients.push_back(1.0);
            }
            if (structure_id > 0) {
                model.model.elements_variables.push_back(
                        model.z_var[size_id][structure_id - 1]);
                model.model.elements_coefficients.push_back(-1.0);
            }
            model.model.constraints_lower_bounds.push_back(nt[size_id][structure_id]);
            model.model.constraints_upper_bounds.push_back(nt[size_id][structure_id]);
        }
    }

    return model;
}

Solution retrieve_solution_arc_flow(
        const Instance& instance,
        const ArcFlowModel& model,
        const std::vector<double>& milp_solution)
{
    SolutionBuilder solution_builder;
    solution_builder.set_instance(instance);

    if (milp_solution.empty())
        return solution_builder.build();

    int delta = model.processing_times.size();
    int num_job_arcs = model.job_arcs.size();
    int num_loss_arcs = model.loss_arc_nodes.size();
    int theta = model.sizes.size();

    // For each size, collect unassigned jobs sorted by processing time ascending.
    std::vector<int> size_idx_map(model.capacity + 1, -1);
    for (int size_id = 0; size_id < theta; ++size_id)
        size_idx_map[model.sizes[size_id]] = size_id;

    std::vector<std::vector<std::pair<Time, JobId>>> available(theta);
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        const Job& job = instance.job(job_id);
        int size_id = size_idx_map[job.size];
        available[size_id].push_back({job.processing_times[0], job_id});
    }
    for (auto& jobs : available)
        std::sort(jobs.begin(), jobs.end());

    // next_job[size_id] = index of first unassigned job in available[size_id].
    std::vector<int> next_job(theta, 0);

    Time current_time = 0;

    for (int structure_id = 0; structure_id < delta; ++structure_id) {
        Time processing_time = model.processing_times[structure_id];
        int num_batches = static_cast<int>(
                std::round(milp_solution[model.v_var[structure_id]]));
        if (num_batches == 0)
            continue;

        // Copy mutable flow values for this structure.
        std::vector<int> job_arc_flows(num_job_arcs);
        for (int arc_id = 0; arc_id < num_job_arcs; ++arc_id)
            job_arc_flows[arc_id] = static_cast<int>(
                    std::round(milp_solution[model.f_var[arc_id][structure_id]]));

        std::vector<int> loss_arc_flows(num_loss_arcs);
        for (int loss_id = 0; loss_id < num_loss_arcs; ++loss_id)
            loss_arc_flows[loss_id] = static_cast<int>(
                    std::round(milp_solution[model.y_var[loss_id][structure_id]]));

        // Decompose the multi-flow into num_batches unit flows (batches).
        for (int batch = 0; batch < num_batches; ++batch) {
            std::vector<Size> sizes_in_batch;
            Size current_node = 0;

            while (current_node != model.capacity) {
                bool found = false;
                for (int ai = 0;
                        ai < (int)model.job_arcs_from[current_node].size();
                        ++ai) {
                    Size to_node = model.job_arcs_from[current_node][ai].first;
                    int arc_id = model.job_arcs_from[current_node][ai].second;
                    if (job_arc_flows[arc_id] > 0) {
                        job_arc_flows[arc_id]--;
                        sizes_in_batch.push_back(to_node - current_node);
                        current_node = to_node;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    int li = model.loss_arc_idx[current_node];
                    loss_arc_flows[li]--;
                    current_node = model.capacity;
                }
            }

            solution_builder.append_batch(0, current_time);
            for (Size job_size : sizes_in_batch) {
                int size_id = size_idx_map[job_size];
                // By feasibility, next unassigned job has p_j <= P_t.
                solution_builder.add_job_to_last_batch(
                        0, available[size_id][next_job[size_id]].second);
                next_job[size_id]++;
            }
            current_time += processing_time;
        }
    }

    return solution_builder.build();
}

#ifdef CBC_FOUND

class EventHandlerArcFlow: public CbcEventHandler
{

public:

    virtual CbcAction event(CbcEvent which_event);

    EventHandlerArcFlow(
            const Instance& instance,
            const MilpArcFlowParameters& parameters,
            const ArcFlowModel& milp_model,
            Output& output,
            AlgorithmFormatter& algorithm_formatter):
        CbcEventHandler(),
        instance_(instance),
        parameters_(parameters),
        milp_model_(milp_model),
        output_(output),
        algorithm_formatter_(algorithm_formatter) { }

    virtual ~EventHandlerArcFlow() { }

    EventHandlerArcFlow(const EventHandlerArcFlow& rhs):
        CbcEventHandler(rhs),
        instance_(rhs.instance_),
        parameters_(rhs.parameters_),
        milp_model_(rhs.milp_model_),
        output_(rhs.output_),
        algorithm_formatter_(rhs.algorithm_formatter_) { }

    virtual CbcEventHandler* clone() const
    {
        return new EventHandlerArcFlow(*this);
    }

private:

    const Instance& instance_;
    const MilpArcFlowParameters& parameters_;
    const ArcFlowModel& milp_model_;
    Output& output_;
    AlgorithmFormatter& algorithm_formatter_;

};

CbcEventHandler::CbcAction EventHandlerArcFlow::event(CbcEvent which_event)
{
    if ((model_->specialOptions() & 2048) != 0)
        return noAction;
    const CbcModel& cbc_model = *model_;

    int number_of_nodes = mathoptsolverscmake::get_number_of_nodes(cbc_model);

    double milp_objective_value = mathoptsolverscmake::get_solution_value(cbc_model);
    if (!output_.solution.feasible()
            || output_.solution.makespan() > milp_objective_value) {
        std::vector<double> milp_solution =
            mathoptsolverscmake::get_solution(cbc_model);
        Solution solution = retrieve_solution_arc_flow(
                instance_, milp_model_, milp_solution);
        algorithm_formatter_.update_solution(
                solution, "node " + std::to_string(number_of_nodes));
    }

    Time bound = std::ceil(
            mathoptsolverscmake::get_bound(cbc_model) - 1e-5);
    algorithm_formatter_.update_makespan_bound(
            bound, "node " + std::to_string(number_of_nodes));

    if (parameters_.timer.needs_to_end())
        return stop;

    return noAction;
}

#endif

#ifdef XPRESS_FOUND

struct XpressCallbackUserArcFlow
{
    const Instance& instance;
    const MilpArcFlowParameters& parameters;
    const ArcFlowModel& milp_model;
    Output& output;
    AlgorithmFormatter& algorithm_formatter;
};

void xpress_callback_arc_flow(
        XPRSprob xpress_model,
        void* user,
        int*)
{
    const XpressCallbackUserArcFlow& user_data =
        *(const XpressCallbackUserArcFlow*)(user);

    double milp_objective_value =
        mathoptsolverscmake::get_solution_value(xpress_model);
    if (!user_data.output.solution.feasible()
            || user_data.output.solution.makespan() > milp_objective_value) {
        std::vector<double> milp_solution =
            mathoptsolverscmake::get_solution(xpress_model);
        Solution solution = retrieve_solution_arc_flow(
                user_data.instance, user_data.milp_model, milp_solution);
        user_data.algorithm_formatter.update_solution(solution, "");
    }

    Time bound = std::ceil(
            mathoptsolverscmake::get_bound(xpress_model) - 1e-5);
    user_data.algorithm_formatter.update_makespan_bound(bound, "");

    if (user_data.parameters.timer.needs_to_end())
        XPRSinterrupt(xpress_model, XPRS_STOP_USER);
};

#endif

}  // namespace

Output batchschedulingsolver::milp_arc_flow(
        const Instance& instance,
        const MilpArcFlowParameters& parameters)
{
    Output output(instance);
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.start("Arc-flow MILP");

    algorithm_formatter.print_header();

    ArcFlowModel milp_model = create_milp_model_arc_flow(instance);

    std::vector<double> milp_solution;
    double milp_bound = 0;

    if (parameters.solver == mathoptsolverscmake::SolverName::Cbc) {
#ifdef CBC_FOUND
        OsiCbcSolverInterface osi_solver;
        CbcModel cbc_model(osi_solver);
        mathoptsolverscmake::reduce_printout(cbc_model);
        mathoptsolverscmake::set_time_limit(
                cbc_model, parameters.timer.remaining_time());
        mathoptsolverscmake::load(cbc_model, milp_model.model);
        EventHandlerArcFlow cbc_event_handler(
                instance, parameters, milp_model, output, algorithm_formatter);
        cbc_model.passInEventHandler(&cbc_event_handler);
        mathoptsolverscmake::solve(cbc_model);
        milp_solution = mathoptsolverscmake::get_solution(cbc_model);
        milp_bound = mathoptsolverscmake::get_bound(cbc_model);
#else
        throw std::invalid_argument(FUNC_SIGNATURE);
#endif

    } else if (parameters.solver == mathoptsolverscmake::SolverName::Highs) {
#ifdef HIGHS_FOUND
        Highs highs;
        mathoptsolverscmake::reduce_printout(highs);
        mathoptsolverscmake::set_time_limit(
                highs, parameters.timer.remaining_time());
        mathoptsolverscmake::set_log_file(highs, "highs.log");
        mathoptsolverscmake::load(highs, milp_model.model);
        highs.setCallback([
                &instance,
                &parameters,
                &milp_model,
                &output,
                &algorithm_formatter](
                    const int,
                    const std::string& message,
                    const HighsCallbackOutput* highs_output,
                    HighsCallbackInput* highs_input,
                    void*)
                {
                    if (!highs_output->mip_solution.empty()) {
                        double milp_objective_value =
                            highs_output->mip_primal_bound;
                        if (!output.solution.feasible()
                                || output.solution.makespan()
                                        > milp_objective_value) {
                            Solution solution = retrieve_solution_arc_flow(
                                    instance, milp_model,
                                    highs_output->mip_solution);
                            algorithm_formatter.update_solution(
                                    solution,
                                    "node " + std::to_string(
                                        highs_output->mip_node_count));
                        }

                        Time bound = std::ceil(
                                highs_output->mip_dual_bound - 1e-5);
                        if (bound != std::numeric_limits<double>::infinity())
                            algorithm_formatter.update_makespan_bound(
                                    bound,
                                    "node " + std::to_string(
                                        highs_output->mip_node_count));
                    }

                    if (parameters.timer.needs_to_end())
                        highs_input->user_interrupt = 1;
                },
                nullptr);
        HighsStatus highs_status;
        highs_status = highs.startCallback(
                HighsCallbackType::kCallbackMipImprovingSolution);
        highs_status = highs.startCallback(
                HighsCallbackType::kCallbackMipInterrupt);
        mathoptsolverscmake::solve(highs);
        milp_solution = mathoptsolverscmake::get_solution(highs);
        milp_bound = mathoptsolverscmake::get_bound(highs);
#else
        throw std::invalid_argument(FUNC_SIGNATURE);
#endif

    } else if (parameters.solver == mathoptsolverscmake::SolverName::Xpress) {
#ifdef XPRESS_FOUND
        XPRSprob xpress_model;
        XPRScreateprob(&xpress_model);
        mathoptsolverscmake::set_time_limit(
                xpress_model, parameters.timer.remaining_time());
        mathoptsolverscmake::set_log_file(xpress_model, "xpress.log");
        mathoptsolverscmake::load(xpress_model, milp_model.model);
        XpressCallbackUserArcFlow xpress_callback_user{
            instance, parameters, milp_model, output, algorithm_formatter};
        XPRSaddcbprenode(
                xpress_model, xpress_callback_arc_flow,
                (void*)&xpress_callback_user, 0);
        mathoptsolverscmake::solve(xpress_model);
        milp_solution = mathoptsolverscmake::get_solution(xpress_model);
        milp_bound = mathoptsolverscmake::get_bound(xpress_model);
        XPRSdestroyprob(xpress_model);
#else
        throw std::invalid_argument(FUNC_SIGNATURE);
#endif

    } else {
        throw std::invalid_argument(FUNC_SIGNATURE);
    }

    Solution solution = retrieve_solution_arc_flow(
            instance, milp_model, milp_solution);
    algorithm_formatter.update_solution(solution, "");

    algorithm_formatter.update_makespan_bound(
            static_cast<Time>(std::ceil(milp_bound - 1e-5)), "");

    algorithm_formatter.end();
    return output;
}
