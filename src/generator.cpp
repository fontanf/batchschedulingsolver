#include "batchschedulingsolver/generator.hpp"

#include "batchschedulingsolver/instance_builder.hpp"

using namespace batchschedulingsolver;

Instance batchschedulingsolver::generate(
        const GenerateInput& input,
        std::mt19937_64& generator)
{
    InstanceBuilder instance_builder;
    instance_builder.set_objective(input.objective);
    instance_builder.set_number_of_machines(input.number_of_machines);
    for (MachineId machine_id = 0;
            machine_id < input.number_of_machines;
            ++machine_id) {
        instance_builder.set_machine_capacity(machine_id, input.capacity);
    }
    JobId number_of_jobs = input.number_of_machines * input.number_of_batches_per_machine * input.number_of_jobs_per_batch;
    std::uniform_int_distribution<Time> distribution_p(1, input.processing_times_range);
    Size average_size = input.capacity / input.number_of_jobs_per_batch;
    std::uniform_int_distribution<Time> distribution_s;
    if (!input.identical_sizes) {
        distribution_s = std::uniform_int_distribution<Time>(
                (std::max)(average_size / 2, (Size)1),
                (std::min)(3 * average_size / 2, input.capacity));
    }
    std::uniform_int_distribution<Time> distribution_w(1, input.weights_range);
    Time machine_expected_makespan = input.number_of_batches_per_machine * input.processing_times_range / 2;
    std::uniform_int_distribution<Time> distribution_r(0, input.release_dates_dispersion_factor * machine_expected_makespan);
    std::uniform_int_distribution<Time> distribution_d(0, input.due_dates_tightness_factor * machine_expected_makespan);
    Time processing_time = -1;
    for (JobId job_id = 0; job_id < number_of_jobs; ++job_id) {
        instance_builder.add_job();
        // Processing-time.
        if (input.machine_independent_processing_times)
            processing_time = distribution_p(generator);
        for (MachineId machine_id = 0;
                machine_id < input.number_of_machines;
                ++machine_id) {
            if (!input.machine_independent_processing_times)
                processing_time = distribution_p(generator);
            instance_builder.set_job_processing_time(
                    job_id,
                    machine_id,
                    processing_time);
        }
        if (input.identical_sizes) {
            instance_builder.set_job_size(job_id, average_size);
        } else {
            instance_builder.set_job_size(job_id, distribution_s(generator));
        }
        // Release date.
        Time release_date = distribution_r(generator);
        instance_builder.set_job_release_date(job_id, release_date);
        // Due date.
        Time due_date = release_date + distribution_d(generator);
        instance_builder.set_job_due_date(job_id, due_date);
        // Weight.
        instance_builder.set_job_weight(job_id, distribution_w(generator));
    }
    return instance_builder.build();
}
