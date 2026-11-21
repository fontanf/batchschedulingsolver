/**
 * Tree search
 *
 * Forward branching scheme. A node is a partial schedule: a sequence of
 * batches, processed in non-decreasing order of start time, each one on the
 * machine that becomes available first. The last batch of the sequence, the
 * "current batch", is still open. A child either adds a job to the current
 * batch or opens a new batch with a job.
 *
 * See doc/tree_search.md for the description of the branching scheme, of the
 * pruning rules and of the dominance rule, and for the proofs.
 */

#include "batchschedulingsolver/algorithms/tree_search.hpp"

#include "batchschedulingsolver/solution_builder.hpp"

#include "treesearchsolver/iterative_beam_search_2.hpp"

#include <limits>
#include <memory>
#include <sstream>

using namespace batchschedulingsolver;

namespace
{

using NodeId = int64_t;

class BranchingScheme
{

public:

    struct Node
    {
        /** Parent node. */
        std::shared_ptr<Node> parent = nullptr;

        /** For each job, true iff it is scheduled. */
        std::vector<bool> jobs;

        /** Last job added. */
        JobId job_id = -1;

        /** True iff the last job added opened a new batch. */
        bool new_batch = false;

        /** Number of scheduled jobs. */
        JobId number_of_jobs = 0;

        /**
         * Availability times of the machines other than the machine of the
         * current batch, sorted in non-decreasing order.
         */
        std::vector<Time> other_machines_times;

        /** Ids of the machines of 'other_machines_times'. */
        std::vector<MachineId> other_machines_ids;

        /** Machine of the current batch. */
        MachineId machine_id = 0;

        /** Start of the current batch. */
        Time batch_start = 0;

        /** End of the current batch. */
        Time batch_end = 0;

        /** Total size of the jobs of the current batch. */
        Size batch_size = 0;

        /**
         * True iff the end of the current batch may still increase: the
         * current batch contains a single job, and it starts after the
         * availability of its machine and after the start of the previous
         * batch, because of the release date of its job.
         */
        bool batch_can_grow = false;

        /** Objective value of the batches before the current batch. */
        Time cost_before_batch = 0;

        /**
         * Objective value of the partial schedule, with the jobs of the
         * current batch completing at 'batch_end'.
         */
        Time cost = 0;

        /** Lower bound on the objective value of the leaves of the node. */
        Time bound = 0;

        /** Guide. */
        double guide = 0;

        /** Unique id of the node. */
        NodeId id = -1;
    };

    BranchingScheme(const Instance& instance):
        instance_(instance)
    {
    }

    inline const std::shared_ptr<Node> root() const
    {
        auto root = std::shared_ptr<Node>(new Node());
        root->id = node_id_++;
        root->jobs.resize(instance_.number_of_jobs(), false);
        for (MachineId machine_id = 1;
                machine_id < instance_.number_of_machines();
                ++machine_id) {
            root->other_machines_times.push_back(0);
            root->other_machines_ids.push_back(machine_id);
        }
        compute_bound(root);
        return root;
    }

    inline std::vector<std::shared_ptr<Node>> children(
            const std::shared_ptr<Node>& parent) const
    {
        std::vector<std::shared_ptr<Node>> output;
        bool has_batch = (parent->number_of_jobs > 0);
        Size capacity = instance_.machine(0).capacity;

        // Children adding a job to the current batch.
        if (has_batch) {
            for (JobId job_id = 0;
                    job_id < instance_.number_of_jobs();
                    ++job_id) {
                if (!can_join_current_batch(parent, job_id))
                    continue;
                const Job& job = instance_.job(job_id);
                auto child = std::shared_ptr<Node>(new Node());
                child->id = node_id_++;
                child->parent = parent;
                child->jobs = parent->jobs;
                child->jobs[job_id] = true;
                child->job_id = job_id;
                child->new_batch = false;
                child->number_of_jobs = parent->number_of_jobs + 1;
                child->other_machines_times = parent->other_machines_times;
                child->other_machines_ids = parent->other_machines_ids;
                child->machine_id = parent->machine_id;
                child->batch_start = parent->batch_start;
                child->batch_end = (std::max)(
                        parent->batch_end,
                        parent->batch_start + job.processing_times[0]);
                child->batch_size = parent->batch_size + job.size;
                child->batch_can_grow = false;
                child->cost_before_batch = parent->cost_before_batch;
                compute_cost(child);
                compute_bound(child);
                if (child->bound >= cutoff_)
                    continue;
                output.push_back(child);
            }

            // Pruning rule 1: if a job can be added to the current batch
            // without increasing its end, and without completing earlier in
            // another batch, then closing the current batch is dominated.
            for (JobId job_id = 0;
                    job_id < instance_.number_of_jobs();
                    ++job_id) {
                if (parent->jobs[job_id])
                    continue;
                const Job& job = instance_.job(job_id);
                Time p = job.processing_times[0];
                if (job.release_date > parent->batch_start)
                    continue;
                if (parent->batch_size + job.size > capacity)
                    continue;
                if (parent->batch_start + p > parent->batch_end)
                    continue;
                bool ok = true;
                for (Time t: parent->other_machines_times) {
                    Time completion = (std::max)(
                            (std::max)(t, parent->batch_start),
                            job.release_date) + p;
                    if (completion < parent->batch_end) {
                        ok = false;
                        break;
                    }
                }
                if (ok)
                    return output;
            }
        }

        // Children opening a new batch on the machine available first.
        Time machine_time = parent->batch_end;
        MachineId machine_id = parent->machine_id;
        std::vector<Time> other_machines_times;
        std::vector<MachineId> other_machines_ids;
        if (!parent->other_machines_times.empty()
                && parent->other_machines_times[0] < parent->batch_end) {
            machine_time = parent->other_machines_times[0];
            machine_id = parent->other_machines_ids[0];
            // Remove the first machine and insert the machine of the current
            // batch.
            bool inserted = false;
            for (MachineId pos = 1;
                    pos < (MachineId)parent->other_machines_times.size();
                    ++pos) {
                if (!inserted && parent->batch_end <= parent->other_machines_times[pos]) {
                    other_machines_times.push_back(parent->batch_end);
                    other_machines_ids.push_back(parent->machine_id);
                    inserted = true;
                }
                other_machines_times.push_back(parent->other_machines_times[pos]);
                other_machines_ids.push_back(parent->other_machines_ids[pos]);
            }
            if (!inserted) {
                other_machines_times.push_back(parent->batch_end);
                other_machines_ids.push_back(parent->machine_id);
            }
        } else {
            other_machines_times = parent->other_machines_times;
            other_machines_ids = parent->other_machines_ids;
        }
        Time earliest_start = (std::max)(machine_time, parent->batch_start);

        // Pruning rule 2: a new batch with job j is dominated if another job
        // k can be processed alone in a new batch on the same machine before
        // the start of the batch of j. Compute the two smallest ends of such
        // batches.
        Time best_end_1 = -1;
        JobId best_job_1 = -1;
        Time best_end_2 = -1;
        for (JobId job_id = 0;
                job_id < instance_.number_of_jobs();
                ++job_id) {
            if (parent->jobs[job_id])
                continue;
            const Job& job = instance_.job(job_id);
            Time p = job.processing_times[0];
            if (p <= 0)
                continue;
            Time end = (std::max)(earliest_start, job.release_date) + p;
            if (best_job_1 == -1 || end < best_end_1) {
                best_end_2 = best_end_1;
                best_end_1 = end;
                best_job_1 = job_id;
            } else if (best_end_2 == -1 || end < best_end_2) {
                best_end_2 = end;
            }
        }

        for (JobId job_id = 0;
                job_id < instance_.number_of_jobs();
                ++job_id) {
            if (parent->jobs[job_id])
                continue;
            const Job& job = instance_.job(job_id);
            Time start = (std::max)(earliest_start, job.release_date);
            Time best_end = (job_id == best_job_1)? best_end_2: best_end_1;
            if (best_end != -1 && best_end <= start)
                continue;
            auto child = std::shared_ptr<Node>(new Node());
            child->id = node_id_++;
            child->parent = parent;
            child->jobs = parent->jobs;
            child->jobs[job_id] = true;
            child->job_id = job_id;
            child->new_batch = true;
            child->number_of_jobs = parent->number_of_jobs + 1;
            child->other_machines_times = other_machines_times;
            child->other_machines_ids = other_machines_ids;
            child->machine_id = machine_id;
            child->batch_start = start;
            child->batch_end = start + job.processing_times[0];
            child->batch_size = job.size;
            child->batch_can_grow = (start > earliest_start);
            child->cost_before_batch = parent->cost;
            compute_cost(child);
            compute_bound(child);
            if (child->bound >= cutoff_)
                continue;
            output.push_back(child);
        }
        return output;
    }

    inline bool operator()(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (node_1->number_of_jobs != node_2->number_of_jobs)
            return node_1->number_of_jobs < node_2->number_of_jobs;
        if (node_1->guide != node_2->guide)
            return node_1->guide < node_2->guide;
        return node_1->id < node_2->id;
    }

    inline bool leaf(
            const std::shared_ptr<Node>& node) const
    {
        return node->number_of_jobs == instance_.number_of_jobs();
    }

    bool bound(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (!leaf(node_2))
            return false;
        return node_1->bound >= node_2->cost;
    }

    /*
     * Solution pool.
     */

    bool better(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (!leaf(node_1))
            return false;
        if (!leaf(node_2))
            return true;
        return node_1->cost < node_2->cost;
    }

    bool equals(
            const std::shared_ptr<Node>&,
            const std::shared_ptr<Node>&) const
    {
        return false;
    }

    /*
     * Dominances.
     */

    inline bool comparable(
            const std::shared_ptr<Node>& node) const
    {
        return node->number_of_jobs > 0 && !node->batch_can_grow;
    }

    struct NodeHasher
    {
        std::hash<std::vector<bool>> hasher;

        inline bool operator()(
                const std::shared_ptr<Node>& node_1,
                const std::shared_ptr<Node>& node_2) const
        {
            return node_1->jobs == node_2->jobs;
        }

        inline std::size_t operator()(
                const std::shared_ptr<Node>& node) const
        {
            return hasher(node->jobs);
        }
    };

    inline NodeHasher node_hasher() const { return NodeHasher(); }

    inline bool dominates(
            const std::shared_ptr<Node>& node_1,
            const std::shared_ptr<Node>& node_2) const
    {
        if (node_1->cost > node_2->cost)
            return false;

        // Case 1: the current batch of node 1 can receive all the jobs that
        // the current batch of node 2 can receive.
        if (node_1->batch_start == node_2->batch_start
                && node_1->batch_end == node_2->batch_end
                && node_1->batch_size <= node_2->batch_size
                && (node_1->job_id == node_2->job_id
                    || accepts_after(node_1, node_2->job_id))
                && times_dominate(
                    node_1->other_machines_times,
                    node_2->other_machines_times)) {
            return true;
        }

        // Case 2: node 1 can process the jobs that node 2 can add to its
        // current batch in a new batch, which completes before the end of
        // the current batch of node 2.
        Time machine_time = node_1->batch_end;
        const std::vector<Time>* others = &node_1->other_machines_times;
        std::vector<Time> tmp;
        if (!node_1->other_machines_times.empty()
                && node_1->other_machines_times[0] < node_1->batch_end) {
            machine_time = node_1->other_machines_times[0];
            tmp.assign(
                    node_1->other_machines_times.begin() + 1,
                    node_1->other_machines_times.end());
            tmp.insert(
                    std::upper_bound(tmp.begin(), tmp.end(), node_1->batch_end),
                    node_1->batch_end);
            others = &tmp;
        }
        if ((std::max)(machine_time, node_1->batch_start) > node_2->batch_start)
            return false;
        return times_dominate(*others, node_2->other_machines_times);
    }

    /*
     * Outputs.
     */

    std::string display(const std::shared_ptr<Node>& node) const
    {
        if (!leaf(node))
            return "";
        return std::to_string(node->cost);
    }

    Solution to_solution(const std::shared_ptr<Node>& node) const
    {
        // Retrieve the batches, from the last one to the first one.
        std::vector<std::pair<MachineId, Time>> batches;
        std::vector<std::vector<JobId>> batches_jobs;
        std::vector<JobId> jobs;
        for (auto node_tmp = node;
                node_tmp->parent != nullptr;
                node_tmp = node_tmp->parent) {
            jobs.push_back(node_tmp->job_id);
            if (node_tmp->new_batch) {
                batches.push_back({node_tmp->machine_id, node_tmp->batch_start});
                batches_jobs.push_back(jobs);
                jobs.clear();
            }
        }

        SolutionBuilder solution_builder;
        solution_builder.set_instance(instance_);
        for (BatchId batch_pos = (BatchId)batches.size() - 1;
                batch_pos >= 0;
                --batch_pos) {
            MachineId machine_id = batches[batch_pos].first;
            solution_builder.append_batch(machine_id, batches[batch_pos].second);
            for (JobId job_id: batches_jobs[batch_pos])
                solution_builder.add_job_to_last_batch(machine_id, job_id);
        }
        return solution_builder.build();
    }

    /**
     * Set the value of the best known solution: the children whose bound is
     * greater than or equal to it are not generated.
     */
    void set_cutoff(Time cutoff) const { cutoff_ = cutoff; }

private:

    /** Instance. */
    const Instance& instance_;

    mutable NodeId node_id_ = 0;

    /** Value of the best known solution. */
    mutable Time cutoff_ = std::numeric_limits<Time>::max();

    /**
     * Return true iff job 'job_id' can be added after the last job of the
     * current batch of 'node' with respect to the order of the jobs in a
     * batch: non-increasing processing times, ties broken by increasing job
     * ids.
     */
    inline bool accepts_after(
            const std::shared_ptr<Node>& node,
            JobId job_id) const
    {
        Time p = instance_.job(job_id).processing_times[0];
        Time p_last = instance_.job(node->job_id).processing_times[0];
        return p < p_last || (p == p_last && job_id > node->job_id);
    }

    inline bool can_join_current_batch(
            const std::shared_ptr<Node>& node,
            JobId job_id) const
    {
        if (node->number_of_jobs == 0)
            return false;
        if (node->jobs[job_id])
            return false;
        const Job& job = instance_.job(job_id);
        if (job.release_date > node->batch_start)
            return false;
        if (node->batch_size + job.size > instance_.machine(0).capacity)
            return false;
        if (!node->batch_can_grow && !accepts_after(node, job_id))
            return false;
        return true;
    }

    /**
     * Return true iff each value of 'times_1' is smaller than or equal to the
     * value at the same position in 'times_2' (both are sorted).
     */
    static inline bool times_dominate(
            const std::vector<Time>& times_1,
            const std::vector<Time>& times_2)
    {
        for (MachineId pos = 0; pos < (MachineId)times_1.size(); ++pos)
            if (times_1[pos] > times_2[pos])
                return false;
        return true;
    }

    /** Objective value of a job completing at a given time. */
    inline Time job_cost(
            JobId job_id,
            Time completion_time) const
    {
        const Job& job = instance_.job(job_id);
        switch (instance_.objective()) {
        case Objective::TotalFlowTime:
            return job.weight * (completion_time - job.release_date);
        case Objective::TotalTardiness:
            return (completion_time > job.due_date)?
                job.weight * (completion_time - job.due_date): 0;
        default:
            return 0;
        }
    }

    inline void compute_cost(
            const std::shared_ptr<Node>& node) const
    {
        if (instance_.objective() == Objective::Makespan) {
            Time makespan = node->batch_end;
            if (!node->other_machines_times.empty())
                makespan = (std::max)(makespan, node->other_machines_times.back());
            node->cost = makespan;
            return;
        }
        // The end of the current batch may have changed: recompute the cost
        // of all its jobs.
        node->cost = node->cost_before_batch;
        for (auto node_tmp = node;; node_tmp = node_tmp->parent) {
            node->cost += job_cost(node_tmp->job_id, node->batch_end);
            if (node_tmp->new_batch)
                break;
        }
    }

    inline void compute_bound(
            const std::shared_ptr<Node>& node) const
    {
        bool has_batch = (node->number_of_jobs > 0);
        Time machine_time = node->batch_end;
        if (!node->other_machines_times.empty())
            machine_time = (std::min)(machine_time, node->other_machines_times[0]);
        Time earliest_start = (std::max)(machine_time, node->batch_start);

        Time bound = node->cost;
        for (JobId job_id = 0;
                job_id < instance_.number_of_jobs();
                ++job_id) {
            if (node->jobs[job_id])
                continue;
            const Job& job = instance_.job(job_id);
            Time p = job.processing_times[0];
            Time completion_time = (std::max)(earliest_start, job.release_date) + p;
            if (has_batch && can_join_current_batch(node, job_id)) {
                completion_time = (std::min)(
                        completion_time,
                        (std::max)(node->batch_end, node->batch_start + p));
            }
            if (instance_.objective() == Objective::Makespan) {
                bound = (std::max)(bound, completion_time);
            } else {
                bound += job_cost(job_id, completion_time);
            }
        }
        node->bound = bound;
        node->guide = (double)bound;
    }

};

}

Output batchschedulingsolver::tree_search(
        const Instance& instance,
        const TreeSearchParameters& parameters)
{
    Output output(instance);
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.start("Tree search");

    // Check the instance.
    if (instance.objective() != Objective::Makespan
            && instance.objective() != Objective::TotalFlowTime
            && instance.objective() != Objective::TotalTardiness) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "only the makespan, total flow time and total tardiness objectives are supported.");
    }
    if (!instance.machine_independent_processing_times()) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "machine-dependent processing times are not supported.");
    }
    if (!instance.identical_machine_capacities()) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "machines with different capacities are not supported.");
    }
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        if (instance.job(job_id).size > instance.machine(0).capacity) {
            throw std::invalid_argument(
                    FUNC_SIGNATURE + ": "
                    "job " + std::to_string(job_id) + " is larger than the capacity.");
        }
    }

    algorithm_formatter.print_header();

    BranchingScheme branching_scheme(instance);

    // The bound of the root is a bound of the instance.
    Time root_bound = branching_scheme.root()->bound;
    auto update_bound = [&instance, &algorithm_formatter](Time bound, const std::string& s)
    {
        switch (instance.objective()) {
        case Objective::Makespan:
            algorithm_formatter.update_makespan_bound(bound, s);
            break;
        case Objective::TotalFlowTime:
            algorithm_formatter.update_total_flow_time_bound(bound, s);
            break;
        case Objective::TotalTardiness:
            algorithm_formatter.update_total_tardiness_bound(bound, s);
            break;
        default:
            break;
        }
    };
    update_bound(root_bound, "root");

    if (instance.number_of_jobs() == 0) {
        SolutionBuilder solution_builder;
        solution_builder.set_instance(instance);
        algorithm_formatter.update_solution(solution_builder.build(), "");
        update_bound(0, "");
        algorithm_formatter.end();
        return output;
    }

    treesearchsolver::IterativeBeamSearch2Parameters<BranchingScheme> beam_search_parameters;
    beam_search_parameters.verbosity_level = 0;
    beam_search_parameters.timer = parameters.timer;
    beam_search_parameters.minimum_size_of_the_queue = parameters.minimum_size_of_the_queue;
    beam_search_parameters.maximum_size_of_the_queue = parameters.maximum_size_of_the_queue;
    beam_search_parameters.new_solution_callback
        = [&branching_scheme, &algorithm_formatter](
                const treesearchsolver::Output<BranchingScheme>& tree_search_output)
        {
            const auto& beam_search_output = static_cast<const treesearchsolver::IterativeBeamSearch2Output<BranchingScheme>&>(tree_search_output);
            auto node = tree_search_output.solution_pool.best();
            if (!branching_scheme.leaf(node))
                return;
            branching_scheme.set_cutoff(node->cost);
            std::stringstream ss;
            ss << "queue " << beam_search_output.maximum_size_of_the_queue;
            algorithm_formatter.update_solution(branching_scheme.to_solution(node), ss.str());
        };
    auto tree_search_output = treesearchsolver::iterative_beam_search_2(
            branching_scheme,
            beam_search_parameters);

    if (tree_search_output.optimal && output.solution.feasible())
        update_bound(output.solution.objective_value(), "tree search completed");

    algorithm_formatter.end();
    return output;
}
