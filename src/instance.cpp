#include "batchschedulingsolver/instance.hpp"

using namespace batchschedulingsolver;

std::istream& batchschedulingsolver::operator>>(
        std::istream& in,
        Objective& objective)
{
    std::string token;
    in >> token;
    if (token == "makespan" || token == "Makespan") {
        objective = Objective::Makespan;
    } else if (token == "total-flow-time"
            || token == "tft"
            || token == "TFT"
            || token == "total flow time"
            || token == "Total flow time") {
        objective = Objective::TotalFlowTime;
    } else if (token == "throughput"
            || token == "Throughput") {
        objective = Objective::Throughput;
    } else if (token == "total-tardiness"
            || token == "tt"
            || token == "TT"
            || token == "total tardiness"
            || token == "Total tardiness") {
        objective = Objective::TotalTardiness;
    } else if (token == "maximum-lateness"
            || token == "ml"
            || token == "ML"
            || token == "maximum lateness"
            || token == "Maximum lateness") {
        objective = Objective::MaximumLateness;
    } else  {
        in.setstate(std::ios_base::failbit);
    }
    return in;
}

std::ostream& batchschedulingsolver::operator<<(
        std::ostream& os,
        Objective objective)
{
    switch (objective) {
    case Objective::Makespan: {
        os << "Makespan";
        break;
    } case Objective::TotalFlowTime: {
        os << "Total flow time";
        break;
    } case Objective::Throughput: {
        os << "Throughput";
        break;
    } case Objective::TotalTardiness: {
        os << "Total tardiness";
        break;
    } case Objective::MaximumLateness: {
        os << "Maximum lateness";
        break;
    }
    }
    return os;
}

void Instance::write(
        const std::string& instance_path,
        const std::string& format) const
{
    if (instance_path.empty())
        return;
    std::ofstream file{instance_path};
    if (!file.good()) {
        throw std::runtime_error(
                FUNC_SIGNATURE + ": "
                "unable to open file \"" + instance_path + "\".");
    }

    nlohmann::json json;

    std::stringstream objective_ss;
    objective_ss << this->objective();
    json["objective"] = objective_ss.str();
    for (MachineId machine_id = 0;
            machine_id < this->number_of_machines();
            ++machine_id) {
        const Machine& machine = this->machine(machine_id);
        json["machines"][machine_id]["capacity"] = machine.capacity;
    }
    for (JobId job_id = 0; job_id < this->number_of_jobs(); ++job_id) {
        const Job& job = this->job(job_id);
        for (MachineId machine_id = 0;
                machine_id < this->number_of_machines();
                ++machine_id) {
            json["jobs"][job_id]["processing_times"][machine_id] = job.processing_times[machine_id];
        }
        json["jobs"][job_id]["size"] = job.size;
        json["jobs"][job_id]["family_id"] = job.family_id;
        json["jobs"][job_id]["release_date"] = job.release_date;
        json["jobs"][job_id]["due_date"] = job.due_date;
        json["jobs"][job_id]["weight"] = job.weight;
    }

    file << std::setw(4) << json << std::endl;
}

std::ostream& Instance::format(
        std::ostream& os,
        int verbosity_level) const
{
    if (verbosity_level >= 1) {
        os
            << "Number of jobs:              " << this->number_of_jobs() << std::endl
            << "Number of machines:          " << this->number_of_machines() << std::endl
            << "Objective:                   " << this->objective() << std::endl
            ;
    }

    if (verbosity_level >= 2) {
        os << std::right << std::endl
            << std::setw(12) << "Job"
            << std::setw(12) << "Size"
            << std::setw(12) << "Family"
            << std::setw(12) << "Rel. date"
            << std::setw(12) << "Due date"
            << std::setw(12) << "Weight"
            << std::endl
            << std::setw(12) << "---"
            << std::setw(12) << "----"
            << std::setw(12) << "------"
            << std::setw(12) << "---------"
            << std::setw(12) << "--------"
            << std::setw(12) << "------"
            << std::endl;
        for (JobId job_id = 0; job_id < this->number_of_jobs(); ++job_id) {
            const Job& job = this->job(job_id);
            os
                << std::setw(12) << job_id
                << std::setw(12) << job.size
                << std::setw(12) << job.family_id
                << std::setw(12) << job.release_date
                << std::setw(12) << job.due_date
                << std::setw(12) << job.weight
                << std::endl;
        }
    }

    if (verbosity_level >= 2) {
        os << std::right << std::endl
            << std::setw(12) << "Machine"
            << std::setw(12) << "Capacity"
            << std::endl
            << std::setw(12) << "-------"
            << std::setw(12) << "--------"
            << std::endl;
        for (MachineId machine_id = 0;
                machine_id < this->number_of_machines();
                ++machine_id) {
            const Machine& machine = this->machine(machine_id);
            os
                << std::setw(12) << machine_id
                << std::setw(12) << machine.capacity
                << std::endl;
        }
    }

    if (verbosity_level >= 3) {
        os << std::right << std::endl
            << std::setw(12) << "Job"
            << std::setw(12) << "Machine"
            << std::setw(12) << "Proc. time"
            << std::endl
            << std::setw(12) << "---"
            << std::setw(12) << "-------"
            << std::setw(12) << "----------"
            << std::endl;
        for (JobId job_id = 0; job_id < this->number_of_jobs(); ++job_id) {
            const Job& job = this->job(job_id);
            for (MachineId machine_id = 0;
                    machine_id < this->number_of_machines();
                    ++machine_id) {
                os
                    << std::setw(12) << job_id
                    << std::setw(12) << machine_id
                    << std::setw(12) << job.processing_times[machine_id]
                    << std::endl;
            }
        }
    }

    return os;
}
