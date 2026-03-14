#include "batchschedulingsolver/algorithms/milp_rank_based.hpp"

#include "mathoptsolverscmake/milp.hpp"

#include <algorithm>
#include <numeric>

using namespace batchschedulingsolver;

namespace
{

////////////////////////////////////////////////////////////////////////////////
//////////////////// milp_rank_based_three_index_no_starts /////////////////////
////////////////////////////////////////////////////////////////////////////////

struct MilpModelThreeIndexNoStarts
{
    mathoptsolverscmake::MilpModel model;

    /**
     * x_jkm[job_id][batch_id][machine_id] = 1 if job job_id is assigned to
     * batch batch_id processed on machine machine_id.
     */
    std::vector<std::vector<std::vector<int>>> x_jkm;

    /**
     * p_km[batch_id][machine_id] = processing time of batch batch_id on
     * machine machine_id.
     */
    std::vector<std::vector<int>> p_km;

    /** c_max = makespan. */
    int c_max = -1;
};

MilpModelThreeIndexNoStarts create_milp_model_three_index_no_starts(
        const Instance& instance)
{
    MilpModelThreeIndexNoStarts model;
    model.model.objective_direction = mathoptsolverscmake::ObjectiveDirection::Minimize;

    // Variables x_jkm[job_id][batch_id][machine_id].
    model.x_jkm.assign(
            instance.number_of_jobs(),
            std::vector<std::vector<int>>(
                instance.number_of_jobs(),
                std::vector<int>(instance.number_of_machines())));
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        for (JobId batch_id = 0; batch_id < instance.number_of_jobs(); ++batch_id) {
            for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
                model.x_jkm[job_id][batch_id][machine_id] = model.model.variables_lower_bounds.size();
                model.model.variables_names.push_back(
                        "x_{" + std::to_string(job_id)
                        + "," + std::to_string(batch_id)
                        + "," + std::to_string(machine_id) + "}");
                model.model.variables_lower_bounds.push_back(0);
                model.model.variables_upper_bounds.push_back(1);
                model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Binary);
                model.model.objective_coefficients.push_back(0);
            }
        }
    }

    // Variables p_km[batch_id][machine_id].
    model.p_km.assign(
            instance.number_of_jobs(),
            std::vector<int>(instance.number_of_machines()));
    for (JobId batch_id = 0; batch_id < instance.number_of_jobs(); ++batch_id) {
        for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
            model.p_km[batch_id][machine_id] = model.model.variables_lower_bounds.size();
            model.model.variables_names.push_back(
                    "p_{" + std::to_string(batch_id) + "," + std::to_string(machine_id) + "}");
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Continuous);
            model.model.objective_coefficients.push_back(0);
        }
    }

    // Variable c_max.
    model.c_max = model.model.variables_lower_bounds.size();
    model.model.variables_names.push_back("c_max");
    model.model.variables_lower_bounds.push_back(0);
    model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
    model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Continuous);
    model.model.objective_coefficients.push_back(1);

    // Constraints.

    // (15) Each job assigned to exactly one (batch, machine):
    //   sum_{batch_id} sum_{machine_id} x_jkm[j][k][m] = 1  for all job_id.
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        model.model.constraints_starts.push_back(model.model.elements_variables.size());
        model.model.constraints_names.push_back("assign_{" + std::to_string(job_id) + "}");
        for (JobId batch_id = 0; batch_id < instance.number_of_jobs(); ++batch_id) {
            for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
                model.model.elements_variables.push_back(model.x_jkm[job_id][batch_id][machine_id]);
                model.model.elements_coefficients.push_back(1.0);
            }
        }
        model.model.constraints_lower_bounds.push_back(1);
        model.model.constraints_upper_bounds.push_back(1);
    }

    // (16) Batch capacity:
    //   sum_{job_id} sum_{machine_id} s_{job_id} * x_jkm[j][k][m] <= B  for all batch_id.
    for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
        const Machine& machine = instance.machine(machine_id);
        for (JobId batch_id = 0; batch_id < instance.number_of_jobs(); ++batch_id) {
            model.model.constraints_starts.push_back(model.model.elements_variables.size());
            model.model.constraints_names.push_back("cap_{" + std::to_string(machine_id) + "_" + std::to_string(batch_id) + "}");
            for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
                const Job& job = instance.job(job_id);
                model.model.elements_variables.push_back(model.x_jkm[job_id][batch_id][machine_id]);
                model.model.elements_coefficients.push_back(job.size);
            }
            model.model.constraints_lower_bounds.push_back(-std::numeric_limits<double>::infinity());
            model.model.constraints_upper_bounds.push_back(machine.capacity);
        }
    }

    // (17) Batch processing time is the max processing time of its assigned jobs:
    //   p_km[k][m] >= p_{job_id} * x_jkm[j][k][m]  for all job_id, batch_id, machine_id.
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        const Job& job = instance.job(job_id);
        for (JobId batch_id = 0; batch_id < instance.number_of_jobs(); ++batch_id) {
            for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
                model.model.constraints_starts.push_back(model.model.elements_variables.size());
                model.model.constraints_names.push_back(
                        "proc_{" + std::to_string(job_id)
                        + "," + std::to_string(batch_id)
                        + "," + std::to_string(machine_id) + "}");
                model.model.elements_variables.push_back(model.p_km[batch_id][machine_id]);
                model.model.elements_coefficients.push_back(1.0);
                model.model.elements_variables.push_back(model.x_jkm[job_id][batch_id][machine_id]);
                model.model.elements_coefficients.push_back(-job.processing_times[machine_id]);
                model.model.constraints_lower_bounds.push_back(0);
                model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            }
        }
    }

    // (18) Makespan is the latest machine completion time:
    //   c_max >= sum_{batch_id} p_km[k][m]  for all machine_id.
    for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
        model.model.constraints_starts.push_back(model.model.elements_variables.size());
        model.model.constraints_names.push_back("makespan_{" + std::to_string(machine_id) + "}");
        model.model.elements_variables.push_back(model.c_max);
        model.model.elements_coefficients.push_back(1.0);
        for (JobId batch_id = 0; batch_id < instance.number_of_jobs(); ++batch_id) {
            model.model.elements_variables.push_back(model.p_km[batch_id][machine_id]);
            model.model.elements_coefficients.push_back(-1.0);
        }
        model.model.constraints_lower_bounds.push_back(0);
        model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
    }

    return model;
}

Solution retrieve_solution_three_index_no_starts(
        const Instance& instance,
        const MilpModelThreeIndexNoStarts& model,
        const std::vector<double>& milp_solution)
{
    SolutionBuilder solution_builder;
    solution_builder.set_instance(instance);

    if (milp_solution.empty())
        return solution_builder.build();

    // For each machine, iterate batches in index order (order does not affect
    // makespan when there are no release dates).
    for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
        Time current_time = 0;
        for (JobId batch_id = 0; batch_id < instance.number_of_jobs(); ++batch_id) {
            std::vector<JobId> jobs_in_batch;
            for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
                if (milp_solution[model.x_jkm[job_id][batch_id][machine_id]] > 0.5)
                    jobs_in_batch.push_back(job_id);
            }
            if (jobs_in_batch.empty())
                continue;

            solution_builder.append_batch(machine_id, current_time);
            for (JobId job_id: jobs_in_batch)
                solution_builder.add_job_to_last_batch(machine_id, job_id);

            current_time += static_cast<Time>(
                    std::round(milp_solution[model.p_km[batch_id][machine_id]]));
        }
    }

    return solution_builder.build();
}

#ifdef CBC_FOUND

class EventHandlerThreeIndexNoStarts: public CbcEventHandler
{

public:

    virtual CbcAction event(CbcEvent which_event);

    EventHandlerThreeIndexNoStarts(
            const Instance& instance,
            const MilpRankBasedParameters& parameters,
            const MilpModelThreeIndexNoStarts& milp_model,
            Output& output,
            AlgorithmFormatter& algorithm_formatter):
        CbcEventHandler(),
        instance_(instance),
        parameters_(parameters),
        milp_model_(milp_model),
        output_(output),
        algorithm_formatter_(algorithm_formatter) { }

    virtual ~EventHandlerThreeIndexNoStarts() { }

    EventHandlerThreeIndexNoStarts(const EventHandlerThreeIndexNoStarts& rhs):
        CbcEventHandler(rhs),
        instance_(rhs.instance_),
        parameters_(rhs.parameters_),
        milp_model_(rhs.milp_model_),
        output_(rhs.output_),
        algorithm_formatter_(rhs.algorithm_formatter_) { }

    virtual CbcEventHandler* clone() const { return new EventHandlerThreeIndexNoStarts(*this); }

private:

    const Instance& instance_;
    const MilpRankBasedParameters& parameters_;
    const MilpModelThreeIndexNoStarts& milp_model_;
    Output& output_;
    AlgorithmFormatter& algorithm_formatter_;

};

CbcEventHandler::CbcAction EventHandlerThreeIndexNoStarts::event(CbcEvent which_event)
{
    // Not in subtree.
    if ((model_->specialOptions() & 2048) != 0)
        return noAction;
    const CbcModel& cbc_model = *model_;

    int number_of_nodes = mathoptsolverscmake::get_number_of_nodes(cbc_model);

    // Retrieve solution.
    double milp_objective_value = mathoptsolverscmake::get_solution_value(cbc_model);
    if (!output_.solution.feasible()
            || output_.solution.makespan() > milp_objective_value) {
        std::vector<double> milp_solution = mathoptsolverscmake::get_solution(cbc_model);
        Solution solution = retrieve_solution_three_index_no_starts(instance_, milp_model_, milp_solution);
        algorithm_formatter_.update_solution(solution, "node " + std::to_string(number_of_nodes));
    }

    // Retrieve bound.
    Time bound = std::ceil(mathoptsolverscmake::get_bound(cbc_model) - 1e-5);
    algorithm_formatter_.update_makespan_bound(bound, "node " + std::to_string(number_of_nodes));

    // Check end.
    if (parameters_.timer.needs_to_end())
        return stop;

    return noAction;
}

#endif

#ifdef XPRESS_FOUND

struct XpressCallbackUserThreeIndexNoStarts
{
    const Instance& instance;
    const MilpRankBasedParameters& parameters;
    const MilpModelThreeIndexNoStarts& milp_model;
    Output& output;
    AlgorithmFormatter& algorithm_formatter;
};

void xpress_callback_three_index_no_starts(
        XPRSprob xpress_model,
        void* user,
        int*)
{
    const XpressCallbackUserThreeIndexNoStarts& d = *(const XpressCallbackUserThreeIndexNoStarts*)(user);

    // Retrieve solution.
    double milp_objective_value = mathoptsolverscmake::get_solution_value(xpress_model);
    if (!d.output.solution.feasible()
            || d.output.solution.makespan() > milp_objective_value) {
        std::vector<double> milp_solution = mathoptsolverscmake::get_solution(xpress_model);
        Solution solution = retrieve_solution_three_index_no_starts(d.instance, d.milp_model, milp_solution);
        d.algorithm_formatter.update_solution(solution, "");
    }

    // Retrieve bound.
    Time bound = std::ceil(mathoptsolverscmake::get_bound(xpress_model) - 1e-5);
    d.algorithm_formatter.update_makespan_bound(bound, "");

    // Check end.
    if (d.parameters.timer.needs_to_end())
        XPRSinterrupt(xpress_model, XPRS_STOP_USER);
};

#endif

}

Output batchschedulingsolver::milp_rank_based_three_index_no_starts(
        const Instance& instance,
        const MilpRankBasedParameters& parameters)
{
    Output output(instance);
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.start("Rank-based MILP (three-index, no starts)");

    algorithm_formatter.print_header();

    MilpModelThreeIndexNoStarts milp_model = create_milp_model_three_index_no_starts(instance);

    std::vector<double> milp_solution;
    double milp_bound = 0;

    if (parameters.solver == mathoptsolverscmake::SolverName::Cbc) {
#ifdef CBC_FOUND
        OsiCbcSolverInterface osi_solver;
        CbcModel cbc_model(osi_solver);
        mathoptsolverscmake::reduce_printout(cbc_model);
        mathoptsolverscmake::set_time_limit(cbc_model, parameters.timer.remaining_time());
        mathoptsolverscmake::load(cbc_model, milp_model.model);
        EventHandlerThreeIndexNoStarts cbc_event_handler(instance, parameters, milp_model, output, algorithm_formatter);
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
        mathoptsolverscmake::set_time_limit(highs, parameters.timer.remaining_time());
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
                        // Retrieve solution.
                        double milp_objective_value = highs_output->mip_primal_bound;
                        if (!output.solution.feasible()
                                || output.solution.makespan() > milp_objective_value) {
                            Solution solution = retrieve_solution_three_index_no_starts(instance, milp_model, highs_output->mip_solution);
                            algorithm_formatter.update_solution(solution, "node " + std::to_string(highs_output->mip_node_count));
                        }

                        // Retrieve bound.
                        Time bound = std::ceil(highs_output->mip_dual_bound - 1e-5);
                        if (bound != std::numeric_limits<double>::infinity())
                            algorithm_formatter.update_makespan_bound(bound, "node " + std::to_string(highs_output->mip_node_count));
                    }

                    // Check end.
                    if (parameters.timer.needs_to_end())
                        highs_input->user_interrupt = 1;
                },
                nullptr);
        HighsStatus highs_status;
        highs_status = highs.startCallback(HighsCallbackType::kCallbackMipImprovingSolution);
        highs_status = highs.startCallback(HighsCallbackType::kCallbackMipInterrupt);
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
        mathoptsolverscmake::set_time_limit(xpress_model, parameters.timer.remaining_time());
        mathoptsolverscmake::set_log_file(xpress_model, "xpress.log");
        mathoptsolverscmake::load(xpress_model, milp_model.model);
        XpressCallbackUserThreeIndexNoStarts xpress_callback_user{instance, parameters, milp_model, output, algorithm_formatter};
        XPRSaddcbprenode(xpress_model, xpress_callback_three_index_no_starts, (void*)&xpress_callback_user, 0);
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

    // Retrieve solution.
    Solution solution = retrieve_solution_three_index_no_starts(instance, milp_model, milp_solution);
    algorithm_formatter.update_solution(solution, "");

    // Retrieve bound.
    algorithm_formatter.update_makespan_bound(
            static_cast<Time>(std::ceil(milp_bound - 1e-5)), "");

    algorithm_formatter.end();
    return output;
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////// milp_rand_based_three_index //////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace
{

struct MilpModelThreeIndex
{
    mathoptsolverscmake::MilpModel model;

    /**
     * x_jkm[job_id][batch_id][machine_id] = 1 if job job_id is assigned to
     * batch batch_id processed on machine machine_id.
     */
    std::vector<std::vector<std::vector<int>>> x_jkm;

    /**
     * p_km[batch_id][machine_id] = processing time of batch batch_id on
     * machine machine_id.
     */
    std::vector<std::vector<int>> p_km;

    /**
     * s_km[batch_id][machine_id] = start time of batch batch_id on machine
     * machine_id.
     */
    std::vector<std::vector<int>> s_km;

    /** c_max = makespan. */
    int c_max = -1;
};

MilpModelThreeIndex create_milp_model_three_index(
        const Instance& instance)
{
    MilpModelThreeIndex model;
    model.model.objective_direction = mathoptsolverscmake::ObjectiveDirection::Minimize;

    // Variables x_jkm[job_id][batch_id][machine_id].
    model.x_jkm.assign(
            instance.number_of_jobs(),
            std::vector<std::vector<int>>(
                instance.number_of_jobs(),
                std::vector<int>(instance.number_of_machines())));
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        for (JobId batch_id = 0; batch_id < instance.number_of_jobs(); ++batch_id) {
            for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
                model.x_jkm[job_id][batch_id][machine_id] = model.model.variables_lower_bounds.size();
                model.model.variables_names.push_back(
                        "x_{" + std::to_string(job_id)
                        + "," + std::to_string(batch_id)
                        + "," + std::to_string(machine_id) + "}");
                model.model.variables_lower_bounds.push_back(0);
                model.model.variables_upper_bounds.push_back(1);
                model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Binary);
                model.model.objective_coefficients.push_back(0);
            }
        }
    }

    // Variables p_km[batch_id][machine_id].
    model.p_km.assign(
            instance.number_of_jobs(),
            std::vector<int>(instance.number_of_machines()));
    for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
        for (JobId batch_id = 0; batch_id < instance.number_of_jobs(); ++batch_id) {
            model.p_km[batch_id][machine_id] = model.model.variables_lower_bounds.size();
            model.model.variables_names.push_back(
                    "p_{" + std::to_string(machine_id) + "," + std::to_string(batch_id) + "}");
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Continuous);
            model.model.objective_coefficients.push_back(0);
        }
    }

    // Variables s_km[batch_id][machine_id].
    model.s_km.assign(
            instance.number_of_jobs(),
            std::vector<int>(instance.number_of_machines()));
    for (JobId batch_id = 0; batch_id < instance.number_of_jobs(); ++batch_id) {
        for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
            model.s_km[batch_id][machine_id] = model.model.variables_lower_bounds.size();
            model.model.variables_names.push_back(
                    "s_{" + std::to_string(batch_id) + "," + std::to_string(machine_id) + "}");
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Continuous);
            model.model.objective_coefficients.push_back(0);
        }
    }

    // Variable c_max.
    model.c_max = model.model.variables_lower_bounds.size();
    model.model.variables_names.push_back("c_max");
    model.model.variables_lower_bounds.push_back(0);
    model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
    model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Continuous);
    model.model.objective_coefficients.push_back(1);

    // Constraints.

    // (15) Each job assigned to exactly one (batch, machine):
    //   sum_{batch_id, machine_id} x_jkm[j][k][m] = 1  for all job_id.
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        model.model.constraints_starts.push_back(model.model.elements_variables.size());
        model.model.constraints_names.push_back("assign_{" + std::to_string(job_id) + "}");
        for (JobId batch_id = 0; batch_id < instance.number_of_jobs(); ++batch_id) {
            for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
                model.model.elements_variables.push_back(model.x_jkm[job_id][batch_id][machine_id]);
                model.model.elements_coefficients.push_back(1.0);
            }
        }
        model.model.constraints_lower_bounds.push_back(1);
        model.model.constraints_upper_bounds.push_back(1);
    }

    // (31) Batch capacity:
    //   sum_{job_id} s_{job_id} * x_jkm[j][k][m] <= B  for all batch_id, machine_id.
    for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
        const Machine& machine = instance.machine(machine_id);
        for (JobId batch_id = 0; batch_id < instance.number_of_jobs(); ++batch_id) {
            model.model.constraints_starts.push_back(model.model.elements_variables.size());
            model.model.constraints_names.push_back("cap_{" + std::to_string(machine_id) + "_" + std::to_string(batch_id) + "}");
            for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
                const Job& job = instance.job(job_id);
                model.model.elements_variables.push_back(model.x_jkm[job_id][batch_id][machine_id]);
                model.model.elements_coefficients.push_back(job.size);
            }
            model.model.constraints_lower_bounds.push_back(-std::numeric_limits<double>::infinity());
            model.model.constraints_upper_bounds.push_back(machine.capacity);
        }
    }

    // (32) Batch start time must be at or after the release date of every assigned job:
    //   s_km[k][m] >= r_{job_id} * x_jkm[j][k][m]  for all job_id, batch_id, machine_id.
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        const Job& job = instance.job(job_id);
        for (JobId batch_id = 0; batch_id < instance.number_of_jobs(); ++batch_id) {
            for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
                model.model.constraints_starts.push_back(model.model.elements_variables.size());
                model.model.constraints_names.push_back(
                        "release_{" + std::to_string(job_id)
                        + "," + std::to_string(batch_id)
                        + "," + std::to_string(machine_id) + "}");
                model.model.elements_variables.push_back(model.s_km[batch_id][machine_id]);
                model.model.elements_coefficients.push_back(1.0);
                model.model.elements_variables.push_back(model.x_jkm[job_id][batch_id][machine_id]);
                model.model.elements_coefficients.push_back(-job.release_date);
                model.model.constraints_lower_bounds.push_back(0);
                model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            }
        }
    }

    // Processing time definition:
    //   p_km[k][m] >= p_jm * x_jkm[j][k][m]  for all job_id, batch_id, machine_id.
    //   Rearranged: p_km - p_jm * x_jkm >= 0.
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        const Job& job = instance.job(job_id);
        for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
            for (JobId batch_id = 0; batch_id < instance.number_of_jobs(); ++batch_id) {
                model.model.constraints_starts.push_back(model.model.elements_variables.size());
                model.model.constraints_names.push_back(
                        "proc_{" + std::to_string(job_id)
                        + "," + std::to_string(batch_id)
                        + "," + std::to_string(machine_id) + "}");
                model.model.elements_variables.push_back(model.p_km[batch_id][machine_id]);
                model.model.elements_coefficients.push_back(1.0);
                model.model.elements_variables.push_back(model.x_jkm[job_id][batch_id][machine_id]);
                model.model.elements_coefficients.push_back(
                        -static_cast<double>(job.processing_times[machine_id]));
                model.model.constraints_lower_bounds.push_back(0);
                model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            }
        }
    }

    // (33) Sequential batch ordering:
    //   s_km[k][m] >= s_km[k-1][m] + p_km[k-1][m]  for all batch_id > 0, machine_id.
    for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
        for (JobId batch_id = 1; batch_id < instance.number_of_jobs(); ++batch_id) {
            model.model.constraints_starts.push_back(model.model.elements_variables.size());
            model.model.constraints_names.push_back(
                    "seq_{" + std::to_string(batch_id)
                    + "," + std::to_string(machine_id) + "}");
            model.model.elements_variables.push_back(model.s_km[batch_id][machine_id]);
            model.model.elements_coefficients.push_back(1.0);
            model.model.elements_variables.push_back(model.s_km[batch_id - 1][machine_id]);
            model.model.elements_coefficients.push_back(-1.0);
            model.model.elements_variables.push_back(model.p_km[batch_id - 1][machine_id]);
            model.model.elements_coefficients.push_back(-1.0);
            model.model.constraints_lower_bounds.push_back(0);
            model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
        }
    }

    // (35) Makespan >= completion time of last batch on each machine:
    //   c_max >= s_km[n-1][m] + p_km[n-1][m]  for all machine_id.
    for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
        model.model.constraints_starts.push_back(model.model.elements_variables.size());
        model.model.constraints_names.push_back("makespan_{" + std::to_string(machine_id) + "}");
        model.model.elements_variables.push_back(model.c_max);
        model.model.elements_coefficients.push_back(1.0);
        model.model.elements_variables.push_back(
                model.s_km[instance.number_of_jobs() - 1][machine_id]);
        model.model.elements_coefficients.push_back(-1.0);
        model.model.elements_variables.push_back(
                model.p_km[instance.number_of_jobs() - 1][machine_id]);
        model.model.elements_coefficients.push_back(-1.0);
        model.model.constraints_lower_bounds.push_back(0);
        model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
    }

    return model;
}

Solution retrieve_solution_three_index(
        const Instance& instance,
        const MilpModelThreeIndex& model,
        const std::vector<double>& milp_solution)
{
    SolutionBuilder solution_builder;
    solution_builder.set_instance(instance);

    if (milp_solution.empty())
        return solution_builder.build();

    for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
        for (JobId batch_id = 0; batch_id < instance.number_of_jobs(); ++batch_id) {
            std::vector<JobId> jobs_in_batch;
            for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
                if (milp_solution[model.x_jkm[job_id][batch_id][machine_id]] > 0.5)
                    jobs_in_batch.push_back(job_id);
            }
            if (jobs_in_batch.empty())
                continue;

            Time start = static_cast<Time>(
                    std::round(milp_solution[model.s_km[batch_id][machine_id]]));
            solution_builder.append_batch(machine_id, start);
            for (JobId job_id: jobs_in_batch)
                solution_builder.add_job_to_last_batch(machine_id, job_id);
        }
    }

    return solution_builder.build();
}

#ifdef CBC_FOUND

class EventHandlerThreeIndex: public CbcEventHandler
{

public:

    virtual CbcAction event(CbcEvent which_event);

    EventHandlerThreeIndex(
            const Instance& instance,
            const MilpRankBasedParameters& parameters,
            const MilpModelThreeIndex& milp_model,
            Output& output,
            AlgorithmFormatter& algorithm_formatter):
        CbcEventHandler(),
        instance_(instance),
        parameters_(parameters),
        milp_model_(milp_model),
        output_(output),
        algorithm_formatter_(algorithm_formatter) { }

    virtual ~EventHandlerThreeIndex() { }

    EventHandlerThreeIndex(const EventHandlerThreeIndex& rhs):
        CbcEventHandler(rhs),
        instance_(rhs.instance_),
        parameters_(rhs.parameters_),
        milp_model_(rhs.milp_model_),
        output_(rhs.output_),
        algorithm_formatter_(rhs.algorithm_formatter_) { }

    virtual CbcEventHandler* clone() const { return new EventHandlerThreeIndex(*this); }

private:

    const Instance& instance_;
    const MilpRankBasedParameters& parameters_;
    const MilpModelThreeIndex& milp_model_;
    Output& output_;
    AlgorithmFormatter& algorithm_formatter_;

};

CbcEventHandler::CbcAction EventHandlerThreeIndex::event(CbcEvent which_event)
{
    if ((model_->specialOptions() & 2048) != 0)
        return noAction;
    const CbcModel& cbc_model = *model_;

    int number_of_nodes = mathoptsolverscmake::get_number_of_nodes(cbc_model);

    double milp_objective_value = mathoptsolverscmake::get_solution_value(cbc_model);
    if (!output_.solution.feasible()
            || output_.solution.makespan() > milp_objective_value) {
        std::vector<double> milp_solution = mathoptsolverscmake::get_solution(cbc_model);
        Solution solution = retrieve_solution_three_index(instance_, milp_model_, milp_solution);
        algorithm_formatter_.update_solution(solution, "node " + std::to_string(number_of_nodes));
    }

    Time bound = std::ceil(mathoptsolverscmake::get_bound(cbc_model) - 1e-5);
    algorithm_formatter_.update_makespan_bound(bound, "node " + std::to_string(number_of_nodes));

    if (parameters_.timer.needs_to_end())
        return stop;

    return noAction;
}

#endif

#ifdef XPRESS_FOUND

struct XpressCallbackUserThreeIndex
{
    const Instance& instance;
    const MilpRankBasedParameters& parameters;
    const MilpModelThreeIndex& milp_model;
    Output& output;
    AlgorithmFormatter& algorithm_formatter;
};

void xpress_callback_three_index(
        XPRSprob xpress_model,
        void* user,
        int*)
{
    const XpressCallbackUserThreeIndex& d = *(const XpressCallbackUserThreeIndex*)(user);

    double milp_objective_value = mathoptsolverscmake::get_solution_value(xpress_model);
    if (!d.output.solution.feasible()
            || d.output.solution.makespan() > milp_objective_value) {
        std::vector<double> milp_solution = mathoptsolverscmake::get_solution(xpress_model);
        Solution solution = retrieve_solution_three_index(d.instance, d.milp_model, milp_solution);
        d.algorithm_formatter.update_solution(solution, "");
    }

    Time bound = std::ceil(mathoptsolverscmake::get_bound(xpress_model) - 1e-5);
    d.algorithm_formatter.update_makespan_bound(bound, "");

    if (d.parameters.timer.needs_to_end())
        XPRSinterrupt(xpress_model, XPRS_STOP_USER);
};

#endif

}

Output batchschedulingsolver::milp_rank_based_three_index(
        const Instance& instance,
        const MilpRankBasedParameters& parameters)
{
    Output output(instance);
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.start("Rank-based MILP (three-index)");

    algorithm_formatter.print_header();

    MilpModelThreeIndex milp_model = create_milp_model_three_index(instance);

    std::vector<double> milp_solution;
    double milp_bound = 0;

    if (parameters.solver == mathoptsolverscmake::SolverName::Cbc) {
#ifdef CBC_FOUND
        OsiCbcSolverInterface osi_solver;
        CbcModel cbc_model(osi_solver);
        mathoptsolverscmake::reduce_printout(cbc_model);
        mathoptsolverscmake::set_time_limit(cbc_model, parameters.timer.remaining_time());
        mathoptsolverscmake::load(cbc_model, milp_model.model);
        EventHandlerThreeIndex cbc_event_handler(instance, parameters, milp_model, output, algorithm_formatter);
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
        mathoptsolverscmake::set_time_limit(highs, parameters.timer.remaining_time());
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
                        double milp_objective_value = highs_output->mip_primal_bound;
                        if (!output.solution.feasible()
                                || output.solution.makespan() > milp_objective_value) {
                            Solution solution = retrieve_solution_three_index(instance, milp_model, highs_output->mip_solution);
                            algorithm_formatter.update_solution(solution, "node " + std::to_string(highs_output->mip_node_count));
                        }

                        Time bound = std::ceil(highs_output->mip_dual_bound - 1e-5);
                        if (bound != std::numeric_limits<double>::infinity())
                            algorithm_formatter.update_makespan_bound(bound, "node " + std::to_string(highs_output->mip_node_count));
                    }

                    if (parameters.timer.needs_to_end())
                        highs_input->user_interrupt = 1;
                },
                nullptr);
        HighsStatus highs_status;
        highs_status = highs.startCallback(HighsCallbackType::kCallbackMipImprovingSolution);
        highs_status = highs.startCallback(HighsCallbackType::kCallbackMipInterrupt);
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
        mathoptsolverscmake::set_time_limit(xpress_model, parameters.timer.remaining_time());
        mathoptsolverscmake::set_log_file(xpress_model, "xpress.log");
        mathoptsolverscmake::load(xpress_model, milp_model.model);
        XpressCallbackUserThreeIndex xpress_callback_user{instance, parameters, milp_model, output, algorithm_formatter};
        XPRSaddcbprenode(xpress_model, xpress_callback_three_index, (void*)&xpress_callback_user, 0);
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

    Solution solution = retrieve_solution_three_index(instance, milp_model, milp_solution);
    algorithm_formatter.update_solution(solution, "");

    algorithm_formatter.update_makespan_bound(
            static_cast<Time>(std::ceil(milp_bound - 1e-5)), "");

    algorithm_formatter.end();
    return output;
}

////////////////////////////////////////////////////////////////////////////////
///////////////////// milp_rank_based_two_index_no_starts //////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace
{

struct ModelTwoIndexNoStarts
{
    mathoptsolverscmake::MilpModel model;

    /**
     * Sorted job indices: sorted_jobs[k] is the original job_id at sorted
     * position k (sorted by non-decreasing processing time).
     */
    std::vector<JobId> sorted_jobs;

    /**
     * x_jk[j][k] = 1 if job at sorted position j is assigned to batch k.
     * Only defined for j <= k; entry is -1 for j > k.
     */
    std::vector<std::vector<int>> x_jk;

    /**
     * y_km[k][m] = 1 if batch k is assigned to machine m.
     */
    std::vector<std::vector<int>> y_km;

    /**
     * b_k[k] is the capacity of batch k.
     *
     * These variables are not used if all machines have the same capacity.
     */
    std::vector<int> b_k;

    /** c_max = makespan. */
    int c_max = -1;
};

ModelTwoIndexNoStarts create_milp_model_two_index_no_starts(
        const Instance& instance)
{
    ModelTwoIndexNoStarts model;
    model.model.objective_direction = mathoptsolverscmake::ObjectiveDirection::Minimize;

    // Sort jobs by non-decreasing processing time (identical machines: use machine 0).
    model.sorted_jobs.resize(instance.number_of_jobs());
    std::iota(model.sorted_jobs.begin(), model.sorted_jobs.end(), 0);
    std::stable_sort(model.sorted_jobs.begin(), model.sorted_jobs.end(),
            [&](JobId a, JobId b) {
                return instance.job(a).processing_times[0]
                     < instance.job(b).processing_times[0];
            });

    // Variables x_jk[job_pos][batch_pos] for job_pos <= batch_pos (lower triangular, -1 elsewhere).
    model.x_jk.assign(
            instance.number_of_jobs(),
            std::vector<int>(instance.number_of_jobs(), -1));
    for (JobId job_pos = 0; job_pos < instance.number_of_jobs(); ++job_pos) {
        for (JobId batch_pos = job_pos; batch_pos < instance.number_of_jobs(); ++batch_pos) {
            model.x_jk[job_pos][batch_pos] = model.model.variables_lower_bounds.size();
            model.model.variables_names.push_back(
                    "x_{" + std::to_string(job_pos) + "," + std::to_string(batch_pos) + "}");
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(1);
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Binary);
            model.model.objective_coefficients.push_back(0);
        }
    }

    // Variables y_km[batch_pos][machine_id].
    model.y_km.assign(
            instance.number_of_jobs(),
            std::vector<int>(instance.number_of_machines()));
    for (JobId batch_pos = 0; batch_pos < instance.number_of_jobs(); ++batch_pos) {
        for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
            model.y_km[batch_pos][machine_id] = model.model.variables_lower_bounds.size();
            model.model.variables_names.push_back(
                    "y_{" + std::to_string(batch_pos) + "," + std::to_string(machine_id) + "}");
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(1);
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Binary);
            model.model.objective_coefficients.push_back(0);
        }
    }

    // Variables b_k.
    if (!instance.identical_machine_capacities()) {
        model.b_k = std::vector<int>(instance.number_of_jobs());
        for (JobId batch_pos = 0; batch_pos < instance.number_of_jobs(); ++batch_pos) {
            model.b_k[batch_pos] = model.model.variables_lower_bounds.size();
            model.model.variables_names.push_back("b_{" + std::to_string(batch_pos) + "}");
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(instance.largest_machine_capacity());
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Integer);
            model.model.objective_coefficients.push_back(0);
        }
    }

    // Variable c_max.
    model.c_max = model.model.variables_lower_bounds.size();
    model.model.variables_names.push_back("c_max");
    model.model.variables_lower_bounds.push_back(0);
    model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
    model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Continuous);
    model.model.objective_coefficients.push_back(1);

    // Constraints.

    // (39) Each job at position job_pos assigned to exactly one batch at position batch_pos >= job_pos:
    //   sum_{batch_pos >= job_pos} x_jk[job_pos][batch_pos] = 1  for all job_pos.
    for (JobId job_pos = 0; job_pos < instance.number_of_jobs(); ++job_pos) {
        model.model.constraints_starts.push_back(model.model.elements_variables.size());
        model.model.constraints_names.push_back("assign_{" + std::to_string(job_pos) + "}");
        for (JobId batch_pos = job_pos; batch_pos < instance.number_of_jobs(); ++batch_pos) {
            model.model.elements_variables.push_back(model.x_jk[job_pos][batch_pos]);
            model.model.elements_coefficients.push_back(1.0);
        }
        model.model.constraints_lower_bounds.push_back(1);
        model.model.constraints_upper_bounds.push_back(1);
    }

    // Batch capacity definition: b_k = sum_m B_m * y_{k,m}  for all batch_pos.
    if (!instance.identical_machine_capacities()) {
        for (JobId batch_pos = 0; batch_pos < instance.number_of_jobs(); ++batch_pos) {
            model.model.constraints_starts.push_back(model.model.elements_variables.size());
            model.model.constraints_names.push_back("b_def_{" + std::to_string(batch_pos) + "}");
            model.model.elements_variables.push_back(model.b_k[batch_pos]);
            model.model.elements_coefficients.push_back(1.0);
            for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
                model.model.elements_variables.push_back(model.y_km[batch_pos][machine_id]);
                model.model.elements_coefficients.push_back(
                        -static_cast<double>(instance.machine(machine_id).capacity));
            }
            model.model.constraints_lower_bounds.push_back(0);
            model.model.constraints_upper_bounds.push_back(0);
        }
    }

    // (40) Batch capacity:
    //   - Identical capacities: sum_{j<=k} size_j * x_{j,k} <= B * x_{k,k}
    //   - Non-identical capacities: sum_{j<=k} size_j * x_{j,k} <= b_k
    //   for all batch_pos.
    for (JobId batch_pos = 0; batch_pos < instance.number_of_jobs(); ++batch_pos) {
        model.model.constraints_starts.push_back(model.model.elements_variables.size());
        model.model.constraints_names.push_back("cap_{" + std::to_string(batch_pos) + "}");
        for (JobId job_pos = 0; job_pos < batch_pos; ++job_pos) {
            model.model.elements_variables.push_back(model.x_jk[job_pos][batch_pos]);
            model.model.elements_coefficients.push_back(
                    static_cast<double>(instance.job(model.sorted_jobs[job_pos]).size));
        }
        if (instance.identical_machine_capacities()) {
            // Diagonal element: combines the size term and -capacity term for x_kk.
            double diag_coef = static_cast<double>(
                    instance.job(model.sorted_jobs[batch_pos]).size
                    - instance.machine(0).capacity);
            if (diag_coef != 0.0) {
                model.model.elements_variables.push_back(model.x_jk[batch_pos][batch_pos]);
                model.model.elements_coefficients.push_back(diag_coef);
            }
        } else {
            double size_k = static_cast<double>(
                    instance.job(model.sorted_jobs[batch_pos]).size);
            if (size_k != 0.0) {
                model.model.elements_variables.push_back(model.x_jk[batch_pos][batch_pos]);
                model.model.elements_coefficients.push_back(size_k);
            }
            model.model.elements_variables.push_back(model.b_k[batch_pos]);
            model.model.elements_coefficients.push_back(-1.0);
        }
        model.model.constraints_lower_bounds.push_back(-std::numeric_limits<double>::infinity());
        model.model.constraints_upper_bounds.push_back(0);
    }

    // (41) Strengthening: x_jk[job_pos][batch_pos] <= x_kk[batch_pos][batch_pos]
    //   for all job_pos < batch_pos (skipped when equal: constraint would be 0 <= 0).
    for (JobId job_pos = 0; job_pos < instance.number_of_jobs(); ++job_pos) {
        for (JobId batch_pos = job_pos + 1; batch_pos < instance.number_of_jobs(); ++batch_pos) {
            model.model.constraints_starts.push_back(model.model.elements_variables.size());
            model.model.constraints_names.push_back(
                    "strength_{" + std::to_string(job_pos) + "," + std::to_string(batch_pos) + "}");
            model.model.elements_variables.push_back(model.x_jk[job_pos][batch_pos]);
            model.model.elements_coefficients.push_back(1.0);
            model.model.elements_variables.push_back(model.x_jk[batch_pos][batch_pos]);
            model.model.elements_coefficients.push_back(-1.0);
            model.model.constraints_lower_bounds.push_back(-std::numeric_limits<double>::infinity());
            model.model.constraints_upper_bounds.push_back(0);
        }
    }

    // (45) Each used batch is assigned to at least one machine:
    //   x_kk[batch_pos][batch_pos] <= sum_{machine_id} y_km[batch_pos][machine_id]
    //   for all batch_pos.
    for (JobId batch_pos = 0; batch_pos < instance.number_of_jobs(); ++batch_pos) {
        model.model.constraints_starts.push_back(model.model.elements_variables.size());
        model.model.constraints_names.push_back("machine_{" + std::to_string(batch_pos) + "}");
        model.model.elements_variables.push_back(model.x_jk[batch_pos][batch_pos]);
        model.model.elements_coefficients.push_back(1.0);
        for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
            model.model.elements_variables.push_back(model.y_km[batch_pos][machine_id]);
            model.model.elements_coefficients.push_back(-1.0);
        }
        model.model.constraints_lower_bounds.push_back(0);
        model.model.constraints_upper_bounds.push_back(0);
    }

    // (46) Makespan >= total processing time of batches assigned to each machine:
    //   c_max >= sum_{batch_pos} p_{sorted_batch_pos} * y_km[batch_pos][machine_id]
    //   for all machine_id.
    for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
        model.model.constraints_starts.push_back(model.model.elements_variables.size());
        model.model.constraints_names.push_back("makespan_{" + std::to_string(machine_id) + "}");
        model.model.elements_variables.push_back(model.c_max);
        model.model.elements_coefficients.push_back(1.0);
        for (JobId batch_pos = 0; batch_pos < instance.number_of_jobs(); ++batch_pos) {
            model.model.elements_variables.push_back(model.y_km[batch_pos][machine_id]);
            model.model.elements_coefficients.push_back(
                    -static_cast<double>(
                        instance.job(model.sorted_jobs[batch_pos]).processing_times[0]));
        }
        model.model.constraints_lower_bounds.push_back(0);
        model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
    }

    //model.model.format(std::cout, 4);

    return model;
}

Solution retrieve_solution_two_index_no_starts(
        const Instance& instance,
        const ModelTwoIndexNoStarts& model,
        const std::vector<double>& milp_solution)
{
    SolutionBuilder solution_builder;
    solution_builder.set_instance(instance);

    if (milp_solution.empty())
        return solution_builder.build();

    for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
        Time current_time = 0;
        for (JobId batch_pos = 0; batch_pos < instance.number_of_jobs(); ++batch_pos) {
            if (milp_solution[model.y_km[batch_pos][machine_id]] < 0.5)
                continue;

            std::vector<JobId> jobs_in_batch;
            for (JobId job_pos = 0; job_pos <= batch_pos; ++job_pos) {
                JobId job_id = model.sorted_jobs[job_pos];
                if (milp_solution[model.x_jk[job_pos][batch_pos]] > 0.5)
                    jobs_in_batch.push_back(job_id);
            }
            if (jobs_in_batch.empty())
                continue;

            solution_builder.append_batch(machine_id, current_time);
            for (JobId job_id: jobs_in_batch)
                solution_builder.add_job_to_last_batch(machine_id, job_id);

            // Batch batch_pos's processing time = p_{sorted_jobs[batch_pos]} (longest in the batch).
            current_time += instance.job(model.sorted_jobs[batch_pos]).processing_times[0];
        }
    }

    return solution_builder.build();
}

#ifdef CBC_FOUND

class EventHandlerTwoIndexNoStarts: public CbcEventHandler
{

public:

    virtual CbcAction event(CbcEvent which_event);

    EventHandlerTwoIndexNoStarts(
            const Instance& instance,
            const MilpRankBasedParameters& parameters,
            const ModelTwoIndexNoStarts& milp_model,
            Output& output,
            AlgorithmFormatter& algorithm_formatter):
        CbcEventHandler(),
        instance_(instance),
        parameters_(parameters),
        milp_model_(milp_model),
        output_(output),
        algorithm_formatter_(algorithm_formatter) { }

    virtual ~EventHandlerTwoIndexNoStarts() { }

    EventHandlerTwoIndexNoStarts(const EventHandlerTwoIndexNoStarts& rhs):
        CbcEventHandler(rhs),
        instance_(rhs.instance_),
        parameters_(rhs.parameters_),
        milp_model_(rhs.milp_model_),
        output_(rhs.output_),
        algorithm_formatter_(rhs.algorithm_formatter_) { }

    virtual CbcEventHandler* clone() const { return new EventHandlerTwoIndexNoStarts(*this); }

private:

    const Instance& instance_;
    const MilpRankBasedParameters& parameters_;
    const ModelTwoIndexNoStarts& milp_model_;
    Output& output_;
    AlgorithmFormatter& algorithm_formatter_;

};

CbcEventHandler::CbcAction EventHandlerTwoIndexNoStarts::event(CbcEvent which_event)
{
    if ((model_->specialOptions() & 2048) != 0)
        return noAction;
    const CbcModel& cbc_model = *model_;

    int number_of_nodes = mathoptsolverscmake::get_number_of_nodes(cbc_model);

    double milp_objective_value = mathoptsolverscmake::get_solution_value(cbc_model);
    if (!output_.solution.feasible()
            || output_.solution.makespan() > milp_objective_value) {
        std::vector<double> milp_solution = mathoptsolverscmake::get_solution(cbc_model);
        Solution solution = retrieve_solution_two_index_no_starts(instance_, milp_model_, milp_solution);
        algorithm_formatter_.update_solution(solution, "node " + std::to_string(number_of_nodes));
    }

    Time bound = std::ceil(mathoptsolverscmake::get_bound(cbc_model) - 1e-5);
    algorithm_formatter_.update_makespan_bound(bound, "node " + std::to_string(number_of_nodes));

    if (parameters_.timer.needs_to_end())
        return stop;

    return noAction;
}

#endif

#ifdef XPRESS_FOUND

struct XpressCallbackUserTwoIndexNoStarts
{
    const Instance& instance;
    const MilpRankBasedParameters& parameters;
    const ModelTwoIndexNoStarts& milp_model;
    Output& output;
    AlgorithmFormatter& algorithm_formatter;
};

void xpress_callback_two_index_no_starts(
        XPRSprob xpress_model,
        void* user,
        int*)
{
    const XpressCallbackUserTwoIndexNoStarts& d =
        *(const XpressCallbackUserTwoIndexNoStarts*)(user);

    double milp_objective_value = mathoptsolverscmake::get_solution_value(xpress_model);
    if (!d.output.solution.feasible()
            || d.output.solution.makespan() > milp_objective_value) {
        std::vector<double> milp_solution = mathoptsolverscmake::get_solution(xpress_model);
        Solution solution = retrieve_solution_two_index_no_starts(d.instance, d.milp_model, milp_solution);
        d.algorithm_formatter.update_solution(solution, "");
    }

    Time bound = std::ceil(mathoptsolverscmake::get_bound(xpress_model) - 1e-5);
    d.algorithm_formatter.update_makespan_bound(bound, "");

    if (d.parameters.timer.needs_to_end())
        XPRSinterrupt(xpress_model, XPRS_STOP_USER);
};

#endif

}

Output batchschedulingsolver::milp_rank_based_two_index_no_starts(
        const Instance& instance,
        const MilpRankBasedParameters& parameters)
{
    Output output(instance);
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.start("Rank-based MILP (two index, no starts)");

    algorithm_formatter.print_header();

    ModelTwoIndexNoStarts milp_model = create_milp_model_two_index_no_starts(instance);

    std::vector<double> milp_solution;
    double milp_bound = 0;

    if (parameters.solver == mathoptsolverscmake::SolverName::Cbc) {
#ifdef CBC_FOUND
        OsiCbcSolverInterface osi_solver;
        CbcModel cbc_model(osi_solver);
        mathoptsolverscmake::reduce_printout(cbc_model);
        mathoptsolverscmake::set_time_limit(cbc_model, parameters.timer.remaining_time());
        mathoptsolverscmake::load(cbc_model, milp_model.model);
        EventHandlerTwoIndexNoStarts cbc_event_handler(instance, parameters, milp_model, output, algorithm_formatter);
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
        mathoptsolverscmake::set_time_limit(highs, parameters.timer.remaining_time());
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
                        double milp_objective_value = highs_output->mip_primal_bound;
                        if (!output.solution.feasible()
                                || output.solution.makespan() > milp_objective_value) {
                            Solution solution = retrieve_solution_two_index_no_starts(instance, milp_model, highs_output->mip_solution);
                            algorithm_formatter.update_solution(solution, "node " + std::to_string(highs_output->mip_node_count));
                        }

                        Time bound = std::ceil(highs_output->mip_dual_bound - 1e-5);
                        if (bound != std::numeric_limits<double>::infinity())
                            algorithm_formatter.update_makespan_bound(bound, "node " + std::to_string(highs_output->mip_node_count));
                    }

                    if (parameters.timer.needs_to_end())
                        highs_input->user_interrupt = 1;
                },
                nullptr);
        HighsStatus highs_status;
        highs_status = highs.startCallback(HighsCallbackType::kCallbackMipImprovingSolution);
        highs_status = highs.startCallback(HighsCallbackType::kCallbackMipInterrupt);
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
        mathoptsolverscmake::set_time_limit(xpress_model, parameters.timer.remaining_time());
        mathoptsolverscmake::set_log_file(xpress_model, "xpress.log");
        mathoptsolverscmake::load(xpress_model, milp_model.model);
        XpressCallbackUserTwoIndexNoStarts xpress_callback_user{instance, parameters, milp_model, output, algorithm_formatter};
        XPRSaddcbprenode(xpress_model, xpress_callback_two_index_no_starts, (void*)&xpress_callback_user, 0);
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

    Solution solution = retrieve_solution_two_index_no_starts(instance, milp_model, milp_solution);
    algorithm_formatter.update_solution(solution, "");

    algorithm_formatter.update_makespan_bound(
            static_cast<Time>(std::ceil(milp_bound - 1e-5)), "");

    algorithm_formatter.end();
    return output;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////// milp_rank_based_two_index ///////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace
{

struct ModelTwoIndex
{
    mathoptsolverscmake::MilpModel model;

    /**
     * Sorted job indices: sorted_jobs[k] is the original job_id at sorted
     * position k (sorted by non-decreasing release time).
     */
    std::vector<JobId> sorted_jobs;

    /**
     * x_jk[j][k] = 1 if job at sorted position j is assigned to batch k.
     * Only defined for j <= k; entry is -1 for j > k.
     */
    std::vector<std::vector<int>> x_jk;

    /**
     * y_km[k][m] = 1 if batch k is assigned to machine m.
     */
    std::vector<std::vector<int>> y_km;

    /**
     * b_k[k] is the capacity of batch k.
     *
     * These variables are not used if all machines have the same capacity.
     */
    std::vector<int> b_k;

    /**
     * p_km[k][m] = processing time of batch k on machine m.
     */
    std::vector<std::vector<int>> p_km;

    /**
     * s_km[k][m] = start time of batch k on machine m.
     */
    std::vector<std::vector<int>> s_km;

    /** c_max = makespan. */
    int c_max = -1;
};

ModelTwoIndex create_milp_model_two_index(
        const Instance& instance)
{
    ModelTwoIndex model;
    model.model.objective_direction = mathoptsolverscmake::ObjectiveDirection::Minimize;

    // Sort jobs by non-decreasing release time.
    model.sorted_jobs.resize(instance.number_of_jobs());
    std::iota(model.sorted_jobs.begin(), model.sorted_jobs.end(), 0);
    std::stable_sort(model.sorted_jobs.begin(), model.sorted_jobs.end(),
            [&](JobId a, JobId b) {
                return instance.job(a).release_date < instance.job(b).release_date;
            });

    // Variables x_jk[job_pos][batch_pos] for job_pos <= batch_pos (lower triangular, -1 elsewhere).
    model.x_jk.assign(
            instance.number_of_jobs(),
            std::vector<int>(instance.number_of_jobs(), -1));
    for (JobId job_pos = 0; job_pos < instance.number_of_jobs(); ++job_pos) {
        for (JobId batch_pos = job_pos; batch_pos < instance.number_of_jobs(); ++batch_pos) {
            model.x_jk[job_pos][batch_pos] = model.model.variables_lower_bounds.size();
            model.model.variables_names.push_back(
                    "x_{" + std::to_string(job_pos) + "," + std::to_string(batch_pos) + "}");
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(1);
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Binary);
            model.model.objective_coefficients.push_back(0);
        }
    }

    // Variables y_km[batch_pos][machine_id].
    model.y_km.assign(
            instance.number_of_jobs(),
            std::vector<int>(instance.number_of_machines()));
    for (JobId batch_pos = 0; batch_pos < instance.number_of_jobs(); ++batch_pos) {
        for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
            model.y_km[batch_pos][machine_id] = model.model.variables_lower_bounds.size();
            model.model.variables_names.push_back(
                    "y_{" + std::to_string(batch_pos) + "," + std::to_string(machine_id) + "}");
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(1);
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Binary);
            model.model.objective_coefficients.push_back(0);
        }
    }

    // Variables b_k[batch_pos].
    if (!instance.identical_machine_capacities()) {
        model.b_k = std::vector<int>(instance.number_of_jobs());
        for (JobId batch_pos = 0; batch_pos < instance.number_of_jobs(); ++batch_pos) {
            model.b_k[batch_pos] = model.model.variables_lower_bounds.size();
            model.model.variables_names.push_back("b_{" + std::to_string(batch_pos) + "}");
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(instance.largest_machine_capacity());
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Integer);
            model.model.objective_coefficients.push_back(0);
        }
    }

    // Variables p_km[batch_pos][machine_id].
    model.p_km.assign(
            instance.number_of_jobs(),
            std::vector<int>(instance.number_of_machines()));
    for (JobId batch_pos = 0; batch_pos < instance.number_of_jobs(); ++batch_pos) {
        for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
            model.p_km[batch_pos][machine_id] = model.model.variables_lower_bounds.size();
            model.model.variables_names.push_back(
                    "p_{" + std::to_string(batch_pos) + "," + std::to_string(machine_id) + "}");
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Continuous);
            model.model.objective_coefficients.push_back(0);
        }
    }

    // Variables s_km[batch_pos][machine_id].
    model.s_km.assign(
            instance.number_of_jobs(),
            std::vector<int>(instance.number_of_machines()));
    for (JobId batch_pos = 0; batch_pos < instance.number_of_jobs(); ++batch_pos) {
        for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
            model.s_km[batch_pos][machine_id] = model.model.variables_lower_bounds.size();
            model.model.variables_names.push_back(
                    "s_{" + std::to_string(batch_pos) + "," + std::to_string(machine_id) + "}");
            model.model.variables_lower_bounds.push_back(0);
            model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
            model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Continuous);
            model.model.objective_coefficients.push_back(0);
        }
    }

    // Variable c_max.
    model.c_max = model.model.variables_lower_bounds.size();
    model.model.variables_names.push_back("c_max");
    model.model.variables_lower_bounds.push_back(0);
    model.model.variables_upper_bounds.push_back(std::numeric_limits<double>::infinity());
    model.model.variables_types.push_back(mathoptsolverscmake::VariableType::Continuous);
    model.model.objective_coefficients.push_back(1);

    // Constraints.

    // (39) Each job at position job_pos assigned to exactly one batch at position batch_pos >= job_pos:
    //   sum_{batch_pos >= job_pos} x_jk[job_pos][batch_pos] = 1  for all job_pos.
    for (JobId job_pos = 0; job_pos < instance.number_of_jobs(); ++job_pos) {
        model.model.constraints_starts.push_back(model.model.elements_variables.size());
        model.model.constraints_names.push_back("assign_{" + std::to_string(job_pos) + "}");
        for (JobId batch_pos = job_pos; batch_pos < instance.number_of_jobs(); ++batch_pos) {
            model.model.elements_variables.push_back(model.x_jk[job_pos][batch_pos]);
            model.model.elements_coefficients.push_back(1.0);
        }
        model.model.constraints_lower_bounds.push_back(1);
        model.model.constraints_upper_bounds.push_back(1);
    }

    // Batch capacity definition: b_k = sum_m B_m * y_{k,m}  for all batch_pos.
    if (!instance.identical_machine_capacities()) {
        for (JobId batch_pos = 0; batch_pos < instance.number_of_jobs(); ++batch_pos) {
            model.model.constraints_starts.push_back(model.model.elements_variables.size());
            model.model.constraints_names.push_back("b_def_{" + std::to_string(batch_pos) + "}");
            model.model.elements_variables.push_back(model.b_k[batch_pos]);
            model.model.elements_coefficients.push_back(1.0);
            for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
                model.model.elements_variables.push_back(model.y_km[batch_pos][machine_id]);
                model.model.elements_coefficients.push_back(
                        -static_cast<double>(instance.machine(machine_id).capacity));
            }
            model.model.constraints_lower_bounds.push_back(0);
            model.model.constraints_upper_bounds.push_back(0);
        }
    }

    // (40) Batch capacity:
    //   - Identical capacities: sum_{j<=k} size_j * x_{j,k} <= B * x_{k,k}
    //   - Non-identical capacities: sum_{j<=k} size_j * x_{j,k} <= b_k
    //   for all batch_pos.
    for (JobId batch_pos = 0; batch_pos < instance.number_of_jobs(); ++batch_pos) {
        model.model.constraints_starts.push_back(model.model.elements_variables.size());
        model.model.constraints_names.push_back("cap_{" + std::to_string(batch_pos) + "}");
        for (JobId job_pos = 0; job_pos < batch_pos; ++job_pos) {
            model.model.elements_variables.push_back(model.x_jk[job_pos][batch_pos]);
            model.model.elements_coefficients.push_back(
                    static_cast<double>(instance.job(model.sorted_jobs[job_pos]).size));
        }
        if (instance.identical_machine_capacities()) {
            // Diagonal element: combines the size term and -capacity term for x_kk.
            double diag_coef = static_cast<double>(
                    instance.job(model.sorted_jobs[batch_pos]).size
                    - instance.machine(0).capacity);
            if (diag_coef != 0.0) {
                model.model.elements_variables.push_back(model.x_jk[batch_pos][batch_pos]);
                model.model.elements_coefficients.push_back(diag_coef);
            }
        } else {
            double size_k = static_cast<double>(
                    instance.job(model.sorted_jobs[batch_pos]).size);
            if (size_k != 0.0) {
                model.model.elements_variables.push_back(model.x_jk[batch_pos][batch_pos]);
                model.model.elements_coefficients.push_back(size_k);
            }
            model.model.elements_variables.push_back(model.b_k[batch_pos]);
            model.model.elements_coefficients.push_back(-1.0);
        }
        model.model.constraints_lower_bounds.push_back(-std::numeric_limits<double>::infinity());
        model.model.constraints_upper_bounds.push_back(0);
    }

    // (41) Strengthening: x_jk[job_pos][batch_pos] <= x_kk[batch_pos][batch_pos]
    //   for all job_pos < batch_pos (skipped when equal: constraint would be 0 <= 0).
    for (JobId job_pos = 0; job_pos < instance.number_of_jobs(); ++job_pos) {
        for (JobId batch_pos = job_pos + 1; batch_pos < instance.number_of_jobs(); ++batch_pos) {
            model.model.constraints_starts.push_back(model.model.elements_variables.size());
            model.model.constraints_names.push_back(
                    "strength_{" + std::to_string(job_pos) + "," + std::to_string(batch_pos) + "}");
            model.model.elements_variables.push_back(model.x_jk[job_pos][batch_pos]);
            model.model.elements_coefficients.push_back(1.0);
            model.model.elements_variables.push_back(model.x_jk[batch_pos][batch_pos]);
            model.model.elements_coefficients.push_back(-1.0);
            model.model.constraints_lower_bounds.push_back(-std::numeric_limits<double>::infinity());
            model.model.constraints_upper_bounds.push_back(0);
        }
    }

    // (58) Each used batch is assigned to a machine:
    //   x_kk[batch_pos][batch_pos] <= sum_{machine_id} y_km[batch_pos][machine_id]
    //   for all batch_pos.
    for (JobId batch_pos = 0; batch_pos < instance.number_of_jobs(); ++batch_pos) {
        model.model.constraints_starts.push_back(model.model.elements_variables.size());
        model.model.constraints_names.push_back("machine_{" + std::to_string(batch_pos) + "}");
        model.model.elements_variables.push_back(model.x_jk[batch_pos][batch_pos]);
        model.model.elements_coefficients.push_back(1.0);
        for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
            model.model.elements_variables.push_back(model.y_km[batch_pos][machine_id]);
            model.model.elements_coefficients.push_back(-1.0);
        }
        model.model.constraints_lower_bounds.push_back(0);
        model.model.constraints_upper_bounds.push_back(0);
    }

    // (59) Processing time of batch batch_pos on machine machine_id:
    //   p_km[batch_pos][machine_id] >= p_{sorted_job_pos}
    //   * (x_jk[job_pos][batch_pos] + y_km[batch_pos][machine_id] - 1)
    //   for all job_pos <= batch_pos, batch_pos, machine_id.
    //   Rearranged: p_km - processing_time * x_jk - processing_time * y_km >= -processing_time.
    for (JobId job_pos = 0; job_pos < instance.number_of_jobs(); ++job_pos) {
        double processing_time = static_cast<double>(
                instance.job(model.sorted_jobs[job_pos]).processing_times[0]);
        for (JobId batch_pos = job_pos; batch_pos < instance.number_of_jobs(); ++batch_pos) {
            for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
                model.model.constraints_starts.push_back(model.model.elements_variables.size());
                model.model.constraints_names.push_back(
                        "proc_{" + std::to_string(job_pos) + "," + std::to_string(batch_pos)
                        + "," + std::to_string(machine_id) + "}");
                model.model.elements_variables.push_back(model.p_km[batch_pos][machine_id]);
                model.model.elements_coefficients.push_back(1.0);
                model.model.elements_variables.push_back(model.x_jk[job_pos][batch_pos]);
                model.model.elements_coefficients.push_back(-processing_time);
                model.model.elements_variables.push_back(model.y_km[batch_pos][machine_id]);
                model.model.elements_coefficients.push_back(-processing_time);
                model.model.constraints_lower_bounds.push_back(-processing_time);
                model.model.constraints_upper_bounds.push_back(
                        std::numeric_limits<double>::infinity());
            }
        }
    }

    // (61) Batch start time must respect the batch's release time:
    //   s_km[batch_pos][machine_id] >= r_{sorted_batch_pos} * y_km[batch_pos][machine_id]
    //   for all batch_pos, machine_id.
    //   Rearranged: s_km - release_date * y_km >= 0.
    for (JobId batch_pos = 0; batch_pos < instance.number_of_jobs(); ++batch_pos) {
        double release_date = static_cast<double>(
                instance.job(model.sorted_jobs[batch_pos]).release_date);
        for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
            model.model.constraints_starts.push_back(model.model.elements_variables.size());
            model.model.constraints_names.push_back(
                    "release_{" + std::to_string(batch_pos) + "," + std::to_string(machine_id) + "}");
            model.model.elements_variables.push_back(model.s_km[batch_pos][machine_id]);
            model.model.elements_coefficients.push_back(1.0);
            model.model.elements_variables.push_back(model.y_km[batch_pos][machine_id]);
            model.model.elements_coefficients.push_back(-release_date);
            model.model.constraints_lower_bounds.push_back(0);
            model.model.constraints_upper_bounds.push_back(
                    std::numeric_limits<double>::infinity());
        }
    }

    // (62) Sequential batch ordering on each machine:
    //   s_km[batch_pos][machine_id] >= s_km[batch_pos-1][machine_id] + p_km[batch_pos-1][machine_id]
    //   for all batch_pos > 0, machine_id.
    for (JobId batch_pos = 1; batch_pos < instance.number_of_jobs(); ++batch_pos) {
        for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
            model.model.constraints_starts.push_back(model.model.elements_variables.size());
            model.model.constraints_names.push_back(
                    "seq_{" + std::to_string(batch_pos) + "," + std::to_string(machine_id) + "}");
            model.model.elements_variables.push_back(model.s_km[batch_pos][machine_id]);
            model.model.elements_coefficients.push_back(1.0);
            model.model.elements_variables.push_back(model.s_km[batch_pos - 1][machine_id]);
            model.model.elements_coefficients.push_back(-1.0);
            model.model.elements_variables.push_back(model.p_km[batch_pos - 1][machine_id]);
            model.model.elements_coefficients.push_back(-1.0);
            model.model.constraints_lower_bounds.push_back(0);
            model.model.constraints_upper_bounds.push_back(
                    std::numeric_limits<double>::infinity());
        }
    }

    // (64) Makespan >= completion time of the last batch on each machine:
    //   c_max >= s_km[n-1][machine_id] + p_km[n-1][machine_id]  for all machine_id.
    for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
        model.model.constraints_starts.push_back(model.model.elements_variables.size());
        model.model.constraints_names.push_back("makespan_{" + std::to_string(machine_id) + "}");
        model.model.elements_variables.push_back(model.c_max);
        model.model.elements_coefficients.push_back(1.0);
        model.model.elements_variables.push_back(
                model.s_km[instance.number_of_jobs() - 1][machine_id]);
        model.model.elements_coefficients.push_back(-1.0);
        model.model.elements_variables.push_back(
                model.p_km[instance.number_of_jobs() - 1][machine_id]);
        model.model.elements_coefficients.push_back(-1.0);
        model.model.constraints_lower_bounds.push_back(0);
        model.model.constraints_upper_bounds.push_back(std::numeric_limits<double>::infinity());
    }

    return model;
}

Solution retrieve_solution_two_index(
        const Instance& instance,
        const ModelTwoIndex& model,
        const std::vector<double>& milp_solution)
{
    SolutionBuilder solution_builder;
    solution_builder.set_instance(instance);

    if (milp_solution.empty())
        return solution_builder.build();

    for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
        for (JobId batch_pos = 0; batch_pos < instance.number_of_jobs(); ++batch_pos) {
            if (milp_solution[model.y_km[batch_pos][machine_id]] < 0.5)
                continue;

            std::vector<JobId> jobs_in_batch;
            for (JobId job_pos = 0; job_pos <= batch_pos; ++job_pos) {
                if (milp_solution[model.x_jk[job_pos][batch_pos]] > 0.5)
                    jobs_in_batch.push_back(model.sorted_jobs[job_pos]);
            }
            if (jobs_in_batch.empty())
                continue;

            Time start = static_cast<Time>(
                    std::round(milp_solution[model.s_km[batch_pos][machine_id]]));
            solution_builder.append_batch(machine_id, start);
            for (JobId job_id: jobs_in_batch)
                solution_builder.add_job_to_last_batch(machine_id, job_id);
        }
    }

    return solution_builder.build();
}

#ifdef CBC_FOUND

class EventHandlerTwoIndex: public CbcEventHandler
{

public:

    virtual CbcAction event(CbcEvent which_event);

    EventHandlerTwoIndex(
            const Instance& instance,
            const MilpRankBasedParameters& parameters,
            const ModelTwoIndex& milp_model,
            Output& output,
            AlgorithmFormatter& algorithm_formatter):
        CbcEventHandler(),
        instance_(instance),
        parameters_(parameters),
        milp_model_(milp_model),
        output_(output),
        algorithm_formatter_(algorithm_formatter) { }

    virtual ~EventHandlerTwoIndex() { }

    EventHandlerTwoIndex(const EventHandlerTwoIndex& rhs):
        CbcEventHandler(rhs),
        instance_(rhs.instance_),
        parameters_(rhs.parameters_),
        milp_model_(rhs.milp_model_),
        output_(rhs.output_),
        algorithm_formatter_(rhs.algorithm_formatter_) { }

    virtual CbcEventHandler* clone() const { return new EventHandlerTwoIndex(*this); }

private:

    const Instance& instance_;
    const MilpRankBasedParameters& parameters_;
    const ModelTwoIndex& milp_model_;
    Output& output_;
    AlgorithmFormatter& algorithm_formatter_;

};

CbcEventHandler::CbcAction EventHandlerTwoIndex::event(CbcEvent which_event)
{
    if ((model_->specialOptions() & 2048) != 0)
        return noAction;
    const CbcModel& cbc_model = *model_;

    int number_of_nodes = mathoptsolverscmake::get_number_of_nodes(cbc_model);

    double milp_objective_value = mathoptsolverscmake::get_solution_value(cbc_model);
    if (!output_.solution.feasible()
            || output_.solution.makespan() > milp_objective_value) {
        std::vector<double> milp_solution = mathoptsolverscmake::get_solution(cbc_model);
        Solution solution = retrieve_solution_two_index(instance_, milp_model_, milp_solution);
        algorithm_formatter_.update_solution(solution, "node " + std::to_string(number_of_nodes));
    }

    Time bound = std::ceil(mathoptsolverscmake::get_bound(cbc_model) - 1e-5);
    algorithm_formatter_.update_makespan_bound(bound, "node " + std::to_string(number_of_nodes));

    if (parameters_.timer.needs_to_end())
        return stop;

    return noAction;
}

#endif

#ifdef XPRESS_FOUND

struct XpressCallbackUserTwoIndex
{
    const Instance& instance;
    const MilpRankBasedParameters& parameters;
    const ModelTwoIndex& milp_model;
    Output& output;
    AlgorithmFormatter& algorithm_formatter;
};

void xpress_callback_two_index(
        XPRSprob xpress_model,
        void* user,
        int*)
{
    const XpressCallbackUserTwoIndex& d =
        *(const XpressCallbackUserTwoIndex*)(user);

    double milp_objective_value = mathoptsolverscmake::get_solution_value(xpress_model);
    if (!d.output.solution.feasible()
            || d.output.solution.makespan() > milp_objective_value) {
        std::vector<double> milp_solution = mathoptsolverscmake::get_solution(xpress_model);
        Solution solution = retrieve_solution_two_index(d.instance, d.milp_model, milp_solution);
        d.algorithm_formatter.update_solution(solution, "");
    }

    Time bound = std::ceil(mathoptsolverscmake::get_bound(xpress_model) - 1e-5);
    d.algorithm_formatter.update_makespan_bound(bound, "");

    if (d.parameters.timer.needs_to_end())
        XPRSinterrupt(xpress_model, XPRS_STOP_USER);
};

#endif

}

Output batchschedulingsolver::milp_rank_based_two_index(
        const Instance& instance,
        const MilpRankBasedParameters& parameters)
{
    Output output(instance);
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.start("Rank-based MILP (two-index)");

    algorithm_formatter.print_header();

    ModelTwoIndex milp_model = create_milp_model_two_index(instance);

    std::vector<double> milp_solution;
    double milp_bound = 0;

    if (parameters.solver == mathoptsolverscmake::SolverName::Cbc) {
#ifdef CBC_FOUND
        OsiCbcSolverInterface osi_solver;
        CbcModel cbc_model(osi_solver);
        mathoptsolverscmake::reduce_printout(cbc_model);
        mathoptsolverscmake::set_time_limit(cbc_model, parameters.timer.remaining_time());
        mathoptsolverscmake::load(cbc_model, milp_model.model);
        EventHandlerTwoIndex cbc_event_handler(instance, parameters, milp_model, output, algorithm_formatter);
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
        mathoptsolverscmake::set_time_limit(highs, parameters.timer.remaining_time());
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
                        double milp_objective_value = highs_output->mip_primal_bound;
                        if (!output.solution.feasible()
                                || output.solution.makespan() > milp_objective_value) {
                            Solution solution = retrieve_solution_two_index(instance, milp_model, highs_output->mip_solution);
                            algorithm_formatter.update_solution(solution, "node " + std::to_string(highs_output->mip_node_count));
                        }

                        Time bound = std::ceil(highs_output->mip_dual_bound - 1e-5);
                        if (bound != std::numeric_limits<double>::infinity())
                            algorithm_formatter.update_makespan_bound(bound, "node " + std::to_string(highs_output->mip_node_count));
                    }

                    if (parameters.timer.needs_to_end())
                        highs_input->user_interrupt = 1;
                },
                nullptr);
        HighsStatus highs_status;
        highs_status = highs.startCallback(HighsCallbackType::kCallbackMipImprovingSolution);
        highs_status = highs.startCallback(HighsCallbackType::kCallbackMipInterrupt);
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
        mathoptsolverscmake::set_time_limit(xpress_model, parameters.timer.remaining_time());
        mathoptsolverscmake::set_log_file(xpress_model, "xpress.log");
        mathoptsolverscmake::load(xpress_model, milp_model.model);
        XpressCallbackUserTwoIndex xpress_callback_user{instance, parameters, milp_model, output, algorithm_formatter};
        XPRSaddcbprenode(xpress_model, xpress_callback_two_index, (void*)&xpress_callback_user, 0);
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

    Solution solution = retrieve_solution_two_index(instance, milp_model, milp_solution);
    algorithm_formatter.update_solution(solution, "");

    algorithm_formatter.update_makespan_bound(
            static_cast<Time>(std::ceil(milp_bound - 1e-5)), "");

    algorithm_formatter.end();
    return output;
}
