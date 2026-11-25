#include "batchschedulingsolver/instance_builder.hpp"

#include <map>
#include <sstream>

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
    } else if (format == "chen2011") {
        read_chen2011(file);
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
        if (!(objective_ss >> objective)) {
            throw std::invalid_argument(
                    FUNC_SIGNATURE + ": "
                    "unknown objective \"" + std::string(j["objective"]) + "\".");
        }
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
        set_job_due_date(job_id, due_date);
        set_job_weight(job_id, weight);
    }
    set_objective(Objective::TotalTardiness);
}

Instance InstanceBuilder::build()
{
    for (MachineId machine_id = 0;
            machine_id < instance_.number_of_machines();
            ++machine_id) {
        const Machine& machine = instance_.machine(machine_id);
        if (machine.capacity > instance_.largest_machine_capacity_)
            instance_.largest_machine_capacity_ = machine.capacity;
        if (machine.capacity != instance_.machine(0).capacity)
            instance_.identical_machine_capacities_ = false;
    }

    instance_.machine_independent_processing_times_ = true;
    for (JobId job_id = 0;
            job_id < instance_.number_of_jobs();
            ++job_id) {
        Job& job = instance_.jobs_[job_id];
        if (job.release_date > instance_.largest_release_date_)
            instance_.largest_release_date_ = job.release_date;
        for (MachineId machine_id = 0;
                machine_id < (MachineId)job.processing_times.size();
                ++machine_id) {
            if (job.processing_times[machine_id] > job.largest_processing_time)
                job.largest_processing_time = job.processing_times[machine_id];
            if (machine_id > 0
                    && job.processing_times[machine_id] != job.processing_times[0]) {
                instance_.machine_independent_processing_times_ = false;
            }
        }
    }

    return std::move(instance_);
}

namespace
{

/**
 * Return the elements named 'name' of an XML document, each as a map from
 * the names of its child elements to their texts.
 *
 * Only supports what the 'chen2011' files use: elements without attributes
 * whose children are simple elements with a text.
 */
std::vector<std::map<std::string, std::string>> xml_elements(
        const std::string& content,
        const std::string& name)
{
    std::vector<std::map<std::string, std::string>> elements;
    const std::string open = "<" + name + ">";
    const std::string close = "</" + name + ">";
    std::string::size_type pos = 0;
    for (;;) {
        std::string::size_type start = content.find(open, pos);
        if (start == std::string::npos)
            break;
        start += open.size();
        std::string::size_type end = content.find(close, start);
        if (end == std::string::npos) {
            throw std::invalid_argument(
                    FUNC_SIGNATURE + ": "
                    "missing \"" + close + "\".");
        }
        std::map<std::string, std::string> element;
        std::string::size_type child_pos = start;
        for (;;) {
            std::string::size_type tag_start = content.find('<', child_pos);
            if (tag_start == std::string::npos || tag_start >= end)
                break;
            std::string::size_type tag_end = content.find('>', tag_start);
            std::string tag = content.substr(tag_start + 1, tag_end - tag_start - 1);
            std::string child_close = "</" + tag + ">";
            std::string::size_type text_end = content.find(child_close, tag_end);
            if (text_end == std::string::npos || text_end > end) {
                throw std::invalid_argument(
                        FUNC_SIGNATURE + ": "
                        "missing \"" + child_close + "\" in \"" + name + "\".");
            }
            element[tag] = content.substr(tag_end + 1, text_end - tag_end - 1);
            child_pos = text_end + child_close.size();
        }
        elements.push_back(element);
        pos = end + close.size();
    }
    return elements;
}

int64_t xml_integer(
        const std::map<std::string, std::string>& element,
        const std::string& name,
        const std::string& tag)
{
    auto it = element.find(tag);
    if (it == element.end()) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "missing \"" + tag + "\" in \"" + name + "\".");
    }
    std::size_t length = 0;
    int64_t value = 0;
    try {
        value = std::stoll(it->second, &length);
    } catch (const std::exception&) {
        length = 0;
    }
    if (length == 0 || length != it->second.size()) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "invalid \"" + tag + "\" in \"" + name + "\": "
                "\"" + it->second + "\".");
    }
    return value;
}

}

void InstanceBuilder::read_chen2011(std::ifstream& file)
{
    std::stringstream ss;
    ss << file.rdbuf();
    const std::string content = ss.str();

    // Machines.
    std::vector<std::map<std::string, std::string>> machines = xml_elements(content, "Machine");
    if (machines.empty()) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "no \"Machine\" element.");
    }
    set_number_of_machines(machines.size());
    for (MachineId machine_id = 0;
            machine_id < (MachineId)machines.size();
            ++machine_id) {
        set_machine_capacity(
                machine_id,
                xml_integer(machines[machine_id], "Machine", "Capacity"));
    }

    // Jobs, in the order of their IDs.
    std::vector<std::map<std::string, std::string>> jobs = xml_elements(content, "Job");
    for (JobId job_pos = 0; job_pos < (JobId)jobs.size(); ++job_pos) {
        if (xml_integer(jobs[job_pos], "Job", "ID") != job_pos) {
            throw std::invalid_argument(
                    FUNC_SIGNATURE + ": "
                    "the jobs must have the IDs 0, 1, 2, ... in this order.");
        }
        if (xml_integer(jobs[job_pos], "Job", "Deadline") != 0) {
            throw std::invalid_argument(
                    FUNC_SIGNATURE + ": "
                    "deadlines are not supported.");
        }
        JobId job_id = add_job();
        for (MachineId machine_id = 0;
                machine_id < (MachineId)machines.size();
                ++machine_id) {
            set_job_processing_time(
                    job_id,
                    machine_id,
                    xml_integer(jobs[job_pos], "Job", "ProcessingTime"));
        }
        set_job_size(job_id, xml_integer(jobs[job_pos], "Job", "Size"));
        set_job_release_date(job_id, xml_integer(jobs[job_pos], "Job", "ReleaseTime"));
        // A due date of 0 means no due date.
        Time due_date = xml_integer(jobs[job_pos], "Job", "DueDate");
        if (due_date > 0)
            set_job_due_date(job_id, due_date);
        set_job_weight(job_id, xml_integer(jobs[job_pos], "Job", "Weight"));
    }

    set_objective(Objective::Makespan);
}
