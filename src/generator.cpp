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
    std::uniform_int_distribution<Time> distribution_s(
            (std::max)(average_size / 2, (Size)1),
            (std::min)(3 * average_size / 2, input.capacity));
    std::uniform_int_distribution<Time> distribution_w(1, input.weights_range);
    for (JobId job_id = 0; job_id < number_of_jobs; ++job_id) {
        instance_builder.add_job();
        Time processing_time = distribution_p(generator);
        for (MachineId machine_id = 0;
                machine_id < input.number_of_machines;
                ++machine_id) {
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
        instance_builder.set_job_weight(job_id, distribution_w(generator));
    }
    return instance_builder.build();
}
