#include "batchschedulingsolver/algorithms/brute_force.hpp"

#include "batchschedulingsolver/algorithm_formatter.hpp"

using namespace batchschedulingsolver;

namespace
{

using JobSet = uint64_t;

struct ScheduledBatch
{
    MachineId machine_id;
    Time start;
    JobSet jobs;
};

class BruteForce
{

public:

    BruteForce(
            const Instance& instance,
            const Parameters& parameters,
            AlgorithmFormatter& algorithm_formatter):
        instance_(instance),
        parameters_(parameters),
        algorithm_formatter_(algorithm_formatter),
        machine_times_(instance.number_of_machines(), 0) { }

    /** Return 'true' if the enumeration completed. */
    bool run()
    {
        JobSet all_jobs = (instance_.number_of_jobs() == 64)?
            ~(JobSet)0:
            ((JobSet)1 << instance_.number_of_jobs()) - 1;
        enumerate(all_jobs, -1, 0, 0);
        return !stopped_;
    }

private:

    const Instance& instance_;

    const Parameters& parameters_;

    AlgorithmFormatter& algorithm_formatter_;

    /** Time at which each machine becomes available. */
    std::vector<Time> machine_times_;

    /** Batches of the current partial schedule, in order of start. */
    std::vector<ScheduledBatch> batches_;

    /** Cost of the best schedule found so far. */
    Time best_cost_ = -1;

    bool stopped_ = false;

    JobId lowest_job_id(JobSet jobs) const
    {
        JobId job_id = 0;
        while (!(jobs >> job_id & 1))
            ++job_id;
        return job_id;
    }

    /**
     * Cost of the current partial schedule after adding a batch of jobs
     * completing at 'completion_time'.
     *
     * All the supported objectives are non-decreasing when a batch is added
     * (the weights are non-negative), which justifies the pruning in
     * 'enumerate'.
     */
    Time add_cost(
            Time cost,
            JobSet jobs,
            Time completion_time) const
    {
        switch (instance_.objective()) {
        case Objective::Makespan:
            return (std::max)(cost, completion_time);
        case Objective::MaximumLateness:
            // As computed by SolutionBuilder: only the late jobs count.
            for (JobId job_id = 0; job_id < instance_.number_of_jobs(); ++job_id) {
                if (!(jobs >> job_id & 1))
                    continue;
                const Job& job = instance_.job(job_id);
                if (completion_time > job.due_date)
                    cost = (std::max)(cost, completion_time - job.due_date);
            }
            return cost;
        case Objective::TotalFlowTime:
            for (JobId job_id = 0; job_id < instance_.number_of_jobs(); ++job_id) {
                if (!(jobs >> job_id & 1))
                    continue;
                const Job& job = instance_.job(job_id);
                cost += job.weight * (completion_time - job.release_date);
            }
            return cost;
        case Objective::TotalTardiness:
            for (JobId job_id = 0; job_id < instance_.number_of_jobs(); ++job_id) {
                if (!(jobs >> job_id & 1))
                    continue;
                const Job& job = instance_.job(job_id);
                if (completion_time > job.due_date)
                    cost += job.weight * (completion_time - job.due_date);
            }
            return cost;
        default:
            return cost;
        }
    }

    /**
     * Enumerate the schedules of the remaining jobs.
     *
     * The batches are added in non-decreasing order of start, and of machine
     * for equal starts: each semi-active schedule is enumerated (at least)
     * once, in the order of its batches sorted by (start, machine).
     */
    void enumerate(
            JobSet remaining_jobs,
            MachineId last_machine_id,
            Time last_start,
            Time cost)
    {
        if (stopped_)
            return;
        if (parameters_.timer.needs_to_end()) {
            stopped_ = true;
            return;
        }

        if (remaining_jobs == 0) {
            if (best_cost_ == -1 || cost < best_cost_) {
                best_cost_ = cost;
                algorithm_formatter_.update_solution(to_solution(), "");
            }
            return;
        }

        for (MachineId machine_id = 0;
                machine_id < instance_.number_of_machines();
                ++machine_id) {
            const Machine& machine = instance_.machine(machine_id);
            // Enumerate the non-empty subsets of the remaining jobs.
            for (JobSet jobs = remaining_jobs;
                    jobs != 0;
                    jobs = (jobs - 1) & remaining_jobs) {
                Size size = 0;
                Time release_date = 0;
                Time processing_time = 0;
                // All the jobs of a batch must have the same family (-1 is
                // a family like the others).
                FamilyId family_id = instance_.job(lowest_job_id(jobs)).family_id;
                bool same_family = true;
                for (JobId job_id = 0; job_id < instance_.number_of_jobs(); ++job_id) {
                    if (!(jobs >> job_id & 1))
                        continue;
                    const Job& job = instance_.job(job_id);
                    size += job.size;
                    release_date = (std::max)(release_date, job.release_date);
                    processing_time = (std::max)(processing_time, job.processing_times[machine_id]);
                    if (job.family_id != family_id)
                        same_family = false;
                }
                if (size > machine.capacity || !same_family)
                    continue;
                Time start = (std::max)(machine_times_[machine_id], release_date);
                if (start < last_start
                        || (start == last_start && machine_id < last_machine_id)) {
                    continue;
                }
                Time completion_time = start + processing_time;
                Time new_cost = add_cost(cost, jobs, completion_time);
                // The cost can only increase: this branch can't lead to a
                // strictly better schedule.
                if (best_cost_ != -1 && new_cost >= best_cost_)
                    continue;

                Time machine_time = machine_times_[machine_id];
                machine_times_[machine_id] = completion_time;
                batches_.push_back({machine_id, start, jobs});
                enumerate(remaining_jobs & ~jobs, machine_id, start, new_cost);
                batches_.pop_back();
                machine_times_[machine_id] = machine_time;
            }
        }
    }

    Solution to_solution() const
    {
        SolutionBuilder solution_builder;
        solution_builder.set_instance(instance_);
        for (const ScheduledBatch& batch: batches_) {
            solution_builder.append_batch(batch.machine_id, batch.start);
            for (JobId job_id = 0; job_id < instance_.number_of_jobs(); ++job_id)
                if (batch.jobs >> job_id & 1)
                    solution_builder.add_job_to_last_batch(batch.machine_id, job_id);
        }
        return solution_builder.build();
    }

};

}

Output batchschedulingsolver::brute_force(
        const Instance& instance,
        const Parameters& parameters)
{
    Output output(instance);
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.start("Brute force");

    // Check the instance.
    if (instance.objective() != Objective::Makespan
            && instance.objective() != Objective::TotalFlowTime
            && instance.objective() != Objective::TotalTardiness
            && instance.objective() != Objective::MaximumLateness) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "only the makespan, total flow time, total tardiness and "
                "maximum lateness objectives are supported.");
    }
    if (instance.number_of_jobs() > 64) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "at most 64 jobs are supported.");
    }
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        if (instance.job(job_id).weight < 0) {
            throw std::invalid_argument(
                    FUNC_SIGNATURE + ": "
                    "negative weights are not supported.");
        }
    }

    algorithm_formatter.print_header();

    BruteForce brute_force(instance, parameters, algorithm_formatter);
    bool completed = brute_force.run();

    // If the enumeration completed, the best schedule is optimal. If no
    // schedule was found (a job doesn't fit on any machine), the solution of
    // the output stays infeasible.
    if (completed && output.solution.feasible()) {
        Time value = output.solution.objective_value();
        switch (instance.objective()) {
        case Objective::Makespan:
            algorithm_formatter.update_makespan_bound(value, "enumeration completed");
            break;
        case Objective::TotalFlowTime:
            algorithm_formatter.update_total_flow_time_bound(value, "enumeration completed");
            break;
        case Objective::TotalTardiness:
            algorithm_formatter.update_total_tardiness_bound(value, "enumeration completed");
            break;
        case Objective::MaximumLateness:
            algorithm_formatter.update_maximum_lateness_bound(value, "enumeration completed");
            break;
        default:
            break;
        }
    }

    algorithm_formatter.end();
    return output;
}
