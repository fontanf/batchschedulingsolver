#include "batchschedulingsolver/instance_builder.hpp"

using namespace batchschedulingsolver;

void InstanceBuilder::set_number_of_machines(MachineId number_of_machines)
{
    if (number_of_machines <= 0) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
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
                FUNC_SIGNATURE + ": "
                "invalid 'machine_id'; "
                "machine_id: " + std::to_string(machine_id) + "; "
                "instance_.machines_.size(): " + std::to_string(instance_.machines_.size()) + ".");
    }
    if (capacity <= 0) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
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
                FUNC_SIGNATURE + ": "
                "invalid 'job_id'; "
                "job_id: " + std::to_string(job_id) + "; "
                "instance_.jobs_.size(): " + std::to_string(instance_.jobs_.size()) + ".");
    }
    if (machine_id < 0 || machine_id >= instance_.machines_.size()) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "invalid 'machine_id'; "
                "machine_id: " + std::to_string(machine_id) + "; "
                "instance_.machines_.size(): " + std::to_string(instance_.machines_.size()) + ".");
    }
    if (processing_time <= 0) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
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
                FUNC_SIGNATURE + ": "
                "invalid 'job_id'; "
                "job_id: " + std::to_string(job_id) + "; "
                "instance_.jobs_.size(): " + std::to_string(instance_.jobs_.size()) + ".");
    }
    if (size <= 0) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
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
                FUNC_SIGNATURE + ": "
                "invalid 'job_id'; "
                "job_id: " + std::to_string(job_id) + "; "
                "instance_.jobs_.size(): " + std::to_string(instance_.jobs_.size()) + ".");
    }
    if (family_id < -1) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "'family_id' must be >= -1; "
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
                FUNC_SIGNATURE + ": "
                "invalid 'job_id'; "
                "job_id: " + std::to_string(job_id) + "; "
                "instance_.jobs_.size(): " + std::to_string(instance_.jobs_.size()) + ".");
    }
    if (release_date < 0) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
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
                FUNC_SIGNATURE + ": "
                "invalid 'job_id'; "
                "job_id: " + std::to_string(job_id) + "; "
                "instance_.jobs_.size(): " + std::to_string(instance_.jobs_.size()) + ".");
    }
    if (due_date < -1) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "'due_date' must be >= -1; "
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
                FUNC_SIGNATURE + ": "
                "invalid 'job_id'; "
                "job_id: " + std::to_string(job_id) + "; "
                "instance_.jobs_.size(): " + std::to_string(instance_.jobs_.size()) + ".");
    }
    if (weight < 0) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
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
                FUNC_SIGNATURE + ": "
                "unable to open file \"" + instance_path + "\".");
    }
    if (format == "" || format == "json") {
        read_json(file);
    } else if (format == "" || format == "alfieri2021") {
        read_alfieri2021(file);
    } else if (format == "queiroga2020") {
        read_queiroga2020(file);
    } else {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "unknown instance format \"" + format + "\".");
    }
    file.close();
}

void InstanceBuilder::read_json(std::ifstream& file)
{
    nlohmann ::json j;
    file >> j;

    if (j.contains("objective")) {
        std::stringstream objective_ss;
        objective_ss << std::string(j["objective"]);
        Objective objective;
        objective_ss >> objective;
        set_objective(objective);
    }

    // Read machines.
    MachineId number_of_machines = j["machines"].size();
    set_number_of_machines(number_of_machines);
    for (MachineId machine_id = 0;
            machine_id < number_of_machines;
            ++machine_id) {
        set_machine_capacity(machine_id, j["machines"][machine_id]["capacity"]);
    }

    // Read jobs.
    JobId number_of_jobs = j["jobs"].size();
    for (JobId job_id = 0; job_id < number_of_jobs; ++job_id) {
        add_job();
        for (MachineId machine_id = 0;
                machine_id < number_of_machines;
                ++machine_id) {
            set_job_processing_time(
                    job_id,
                    machine_id,
                    j["jobs"][job_id]["processing_times"][machine_id]);
        }
        if (j["jobs"][job_id].contains("size"))
            set_job_size(job_id, j["jobs"][job_id]["size"]);
        if (j["jobs"][job_id].contains("family_id"))
            set_job_family(job_id, j["jobs"][job_id]["family_id"]);
        if (j["jobs"][job_id].contains("release_date"))
            set_job_release_date(job_id, j["jobs"][job_id]["release_date"]);
        if (j["jobs"][job_id].contains("due_date"))
            set_job_due_date(job_id, j["jobs"][job_id]["due_date"]);
        if (j["jobs"][job_id].contains("weight"))
            set_job_weight(job_id, j["jobs"][job_id]["weight"]);
    }
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
