#include "batchschedulingsolver/algorithms/greedy_longest_processing_time.hpp"

#include "batchschedulingsolver/algorithm_formatter.hpp"


using namespace batchschedulingsolver;

Output batchschedulingsolver::greedy_longest_processing_time(
    const Instance& instance,
    const Parameters& parameters)
{
    Output output(instance);
    AlgorithmFormatter algorithm_formatter(instance,parameters,output);
    algorithm_formatter.start("Greedy longest processing time");
    algorithm_formatter.print_header();

    SolutionBuilder solution_builder;
    solution_builder.set_instance(instance);

    MachineId machine_id = 0; // assuming single machine for LPT

    // create table containing job_id in descending order
    std::vector<JobId> indices(instance.number_of_jobs());
    std::iota(indices.begin(), indices.end(), 0);

    // sort id depending on job processing time
    std::sort(indices.begin(), indices.end(),
            [&instance,machine_id](JobId job_1_id, JobId job_2_id)
            {
                return instance.job(job_1_id).processing_times[machine_id]
                    > instance.job(job_2_id).processing_times[machine_id];
            });

    Time max_time_current_batch = 0;
    Size current_batch_size = 0;
    Time start_time = 0;
    const Machine& machine = instance.machine(machine_id);
    // create first batch
    solution_builder.append_batch(machine_id, start_time);
    for (JobId job_id: indices) {
        const Job& job = instance.job(job_id);
        if (current_batch_size + job.size <= machine.capacity) {
            // if job can be added to current batch
            solution_builder.add_job_to_last_batch(machine_id, job_id);
            max_time_current_batch = std::max(
                    max_time_current_batch,
                    job.processing_times[machine_id]);
            current_batch_size += job.size;
        } else {
            // create new batch
            start_time += max_time_current_batch;
            solution_builder.append_batch(machine_id, start_time);
            solution_builder.add_job_to_last_batch(machine_id, job_id);
            current_batch_size = job.size;
        }

    }

    Solution solution = solution_builder.build();
    algorithm_formatter.update_solution(solution, "greedy");

    algorithm_formatter.end();
    return output;
}
