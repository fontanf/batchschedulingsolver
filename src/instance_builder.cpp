#include "batchschedulingsolver/instance_builder.hpp"

using namespace batchschedulingsolver;

void InstanceBuilder::set_number_of_machines(MachineId number_of_machines)
{
    if (number_of_machines <= 0) {
        throw std::invalid_argument(
                "batchschedulingsolver::InstanceBuilder::set_number_of_machines: "
                "'number_of_machines' must be > 0; "
                "number_of_machines: " + std::to_string(number_of_machines) + ".");
    }

    instance_.machines_ = std::vector<Machine>(number_of_machines);
}

void InstanceBuilder::set_machine_capacity(
        MachineId machine_id,
        Size capacity)
{
    if (machine_id < 0 || machine_id >= instance_.machines_.size()) {
        throw std::invalid_argument(
                "batchschedulingsolver::InstanceBuilder::set_machine_capacity: "
                "invalid 'machine_id'; "
                "machine_id: " + std::to_string(machine_id) + "; "
                "instance_.machines_.size(): " + std::to_string(instance_.machines_.size()) + ".");
    }
    if (capacity <= 0) {
        throw std::invalid_argument(
                "batchschedulingsolver::InstanceBuilder::set_machine_capacity: "
                "'capacity' must be > 0; "
                "capacity: " + std::to_string(capacity) + ".");
    }

    instance_.machines_[machine_id].capacity = capacity;
}

JobId InstanceBuilder::add_job()
{
    JobId job_id = instance_.jobs_.size();
    Job job;
    job.processing_times = std::vector<Time>(instance_.number_of_machines());
    instance_.jobs_.push_back(job);
    return job_id;
}

void InstanceBuilder::set_job_processing_time(
        JobId job_id,
        MachineId machine_id,
        Time processing_time)
{
    if (job_id < 0 || job_id >= instance_.jobs_.size()) {
        throw std::invalid_argument(
                "batchschedulingsolver::InstanceBuilder::set_job_processing_time: "
                "invalid 'job_id'; "
                "job_id: " + std::to_string(job_id) + "; "
                "instance_.jobs_.size(): " + std::to_string(instance_.jobs_.size()) + ".");
    }
    if (machine_id < 0 || machine_id >= instance_.machines_.size()) {
        throw std::invalid_argument(
                "batchschedulingsolver::InstanceBuilder::set_job_processing_time: "
                "invalid 'machine_id'; "
                "machine_id: " + std::to_string(machine_id) + "; "
                "instance_.machines_.size(): " + std::to_string(instance_.machines_.size()) + ".");
    }
    if (processing_time <= 0) {
        throw std::invalid_argument(
                "batchschedulingsolver::InstanceBuilder::set_job_processing_time: "
                "'processing_time' must be > 0; "
                "processing_time: " + std::to_string(processing_time) + ".");
    }

    instance_.jobs_[job_id].processing_times[machine_id] = processing_time;
}

void InstanceBuilder::set_job_size(
        JobId job_id,
        Size size)
{
    if (job_id < 0 || job_id >= instance_.jobs_.size()) {
        throw std::invalid_argument(
                "batchschedulingsolver::InstanceBuilder::set_job_size: "
                "invalid 'job_id'; "
                "job_id: " + std::to_string(job_id) + "; "
                "instance_.jobs_.size(): " + std::to_string(instance_.jobs_.size()) + ".");
    }
    if (size <= 0) {
        throw std::invalid_argument(
                "batchschedulingsolver::InstanceBuilder::set_job_size: "
                "'size' must be > 0; "
                "size: " + std::to_string(size) + ".");
    }

    instance_.jobs_[job_id].size = size;
}

void InstanceBuilder::set_job_family(
        JobId job_id,
        FamilyId family_id)
{
    if (job_id < 0 || job_id >= instance_.jobs_.size()) {
        throw std::invalid_argument(
                "batchschedulingsolver::InstanceBuilder::set_job_family: "
                "invalid 'job_id'; "
                "job_id: " + std::to_string(job_id) + "; "
                "instance_.jobs_.size(): " + std::to_string(instance_.jobs_.size()) + ".");
    }
    if (family_id < 0) {
        throw std::invalid_argument(
                "batchschedulingsolver::InstanceBuilder::set_job_family: "
                "'family_id' must be >= 0; "
                "family_id: " + std::to_string(family_id) + ".");
    }

    instance_.jobs_[job_id].family_id = family_id;
}

void InstanceBuilder::set_job_release_date(
        JobId job_id,
        Time release_date)
{
    if (job_id < 0 || job_id >= instance_.jobs_.size()) {
        throw std::invalid_argument(
                "batchschedulingsolver::InstanceBuilder::set_job_release_date: "
                "invalid 'job_id'; "
                "job_id: " + std::to_string(job_id) + "; "
                "instance_.jobs_.size(): " + std::to_string(instance_.jobs_.size()) + ".");
    }
    if (release_date < 0) {
        throw std::invalid_argument(
                "batchschedulingsolver::InstanceBuilder::set_job_release_date: "
                "'release_date' must be >= 0; "
                "release_date: " + std::to_string(release_date) + ".");
    }

    instance_.jobs_[job_id].release_date = release_date;
}

void InstanceBuilder::set_job_due_date(
        JobId job_id,
        Time due_date)
{
    if (job_id < 0 || job_id >= instance_.jobs_.size()) {
        throw std::invalid_argument(
                "batchschedulingsolver::InstanceBuilder::set_job_due_date: "
                "invalid 'job_id'; "
                "job_id: " + std::to_string(job_id) + "; "
                "instance_.jobs_.size(): " + std::to_string(instance_.jobs_.size()) + ".");
    }
    if (due_date < 0) {
        throw std::invalid_argument(
                "batchschedulingsolver::InstanceBuilder::set_job_due_date: "
                "'due_date' must be >= 0; "
                "due_date: " + std::to_string(due_date) + ".");
    }

    instance_.jobs_[job_id].due_date = due_date;
}

void InstanceBuilder::set_job_weight(
        JobId job_id,
        Time weight)
{
    if (job_id < 0 || job_id >= instance_.jobs_.size()) {
        throw std::invalid_argument(
                "batchschedulingsolver::InstanceBuilder::set_job_weight: "
                "invalid 'job_id'; "
                "job_id: " + std::to_string(job_id) + "; "
                "instance_.jobs_.size(): " + std::to_string(instance_.jobs_.size()) + ".");
    }
    if (weight < 0) {
        throw std::invalid_argument(
                "batchschedulingsolver::InstanceBuilder::set_job_weight: "
                "'weight' must be >= 0; "
                "weight: " + std::to_string(weight) + ".");
    }

    instance_.jobs_[job_id].weight = weight;
}

void InstanceBuilder::read(
        const std::string& instance_path,
        const std::string& format)
{
    std::ifstream file(instance_path);
    if (!file.good()) {
        throw std::runtime_error(
                "Unable to open file \"" + instance_path + "\".");
    }
    if (format == "" || format == "alfieri2021") {
        read_alfieri2021(file);
    } else if (format == "queiroga2020") {
        read_queiroga2020(file);
    } else {
        throw std::invalid_argument(
                "Unknown instance format \"" + format + "\".");
    }
    file.close();
}

void InstanceBuilder::read_alfieri2021(std::ifstream& file)
{
    JobId number_of_jobs = -1;
    Size capacity = -1;
    file >> number_of_jobs >> capacity;
    set_number_of_machines(1);
    MachineId machine_id = 0;
    set_machine_capacity(machine_id, capacity);

    Time processing_time;
    Size size;
    for (JobId job_id = 0; job_id < number_of_jobs; ++job_id) {
        file >> processing_time >> size;
        add_job();
        set_job_processing_time(job_id, machine_id, processing_time);
        set_job_size(job_id, size);
    }
    set_objective(Objective::TotalFlowTime);
}

void InstanceBuilder::read_queiroga2020(std::ifstream& file)
{
    JobId number_of_jobs = -1;
    Size capacity = -1;
    file >> number_of_jobs >> capacity;
    set_number_of_machines(1);
    MachineId machine_id = 0;
    set_machine_capacity(machine_id, capacity);

    Time processing_time;
    Time release_date;
    Time due_date;
    Size size;
    Time weight;
    for (JobId job_id = 0; job_id < number_of_jobs; ++job_id) {
        file
            >> processing_time
            >> due_date
            >> size
            >> weight
            >> release_date;
        add_job();
        set_job_processing_time(job_id, machine_id, processing_time);
        set_job_size(job_id, size);
        set_job_release_date(job_id, release_date);
        set_job_weight(job_id, weight);
    }
    set_objective(Objective::TotalTardiness);
}

Instance InstanceBuilder::build()
{
    return std::move(instance_);
}
