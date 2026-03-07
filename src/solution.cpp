#include "batchschedulingsolver/solution.hpp"

using namespace batchschedulingsolver;

double Solution::objective_value() const
{
    switch (this->instance().objective()) {
    case Objective::Makespan:
        return this->makespan();
    case Objective::TotalFlowTime:
        return this->total_flow_time();
    case Objective::TotalTardiness:
        return this->total_tardiness();
    case Objective::MaximumLateness:
        return this->maximum_lateness();
    case Objective::Throughput:
        return this->throughput();
    }
    return -1;
}

bool Solution::feasible() const
{
    return (this->number_of_overlapping_batches_ == 0
        && this->number_of_family_violations_ == 0
        && this->number_of_release_date_violations_ == 0
        && this->number_of_jobs_ == instance().number_of_jobs());
}

bool Solution::strictly_better(const Solution& solution) const
{
    if (!this->feasible())
        return false;
    if (!solution.feasible())
        return true;
    switch (solution.instance().objective()) {
    case Objective::Makespan: {
        return this->makespan() < solution.makespan();
    } case Objective::TotalFlowTime: {
        return this->total_flow_time() < solution.total_flow_time();
    } case Objective::Throughput: {
        return this->throughput() > solution.throughput();
    } case Objective::TotalTardiness: {
        return this->total_tardiness() < solution.total_tardiness();
    } case Objective::MaximumLateness: {
        return this->maximum_lateness() < solution.maximum_lateness();
    }
    }
    return false;
}

void Solution::write(
        const std::string& certificate_path,
        const std::string& format) const
{
    if (certificate_path.empty())
        return;
    std::ofstream file{ certificate_path };
    if (!file.good()) {
        throw std::runtime_error(
                FUNC_SIGNATURE + ": "
                "unable to open file \"" + certificate_path + "\".");
    }

    if (format == "json") {
        nlohmann::json json;
        for (MachineId machine_id = 0;
                machine_id < this->instance().number_of_machines();
                ++machine_id) {
            const Machine& solution_machine = this->machine(machine_id);
            const batchschedulingsolver::Machine& machine = this->instance().machine(machine_id);

            for (BatchId batch_id = 0;
                    batch_id < (BatchId)solution_machine.batches.size();
                    ++batch_id) {
                const Batch& machine_batch = solution_machine.batches[batch_id];

                for (JobId job_pos = 0;
                        job_pos < (JobId)machine_batch.jobs.size();
                        ++job_pos) {
                    JobId job_id = machine_batch.jobs[job_pos];
                    const batchschedulingsolver::Job& job = this->instance().job(job_id);

                    json["machines"][machine_id]["batches"][batch_id]["jobs"][job_pos]["job_id"] = job_id;
                    json["machines"][machine_id]["batches"][batch_id]["jobs"][job_pos]["size"] = job.size;
                    json["machines"][machine_id]["batches"][batch_id]["jobs"][job_pos]["family_id"] = job.family_id;
                    json["machines"][machine_id]["batches"][batch_id]["jobs"][job_pos]["due_date"] = job.due_date;
                    json["machines"][machine_id]["batches"][batch_id]["jobs"][job_pos]["release_date"] = job.release_date;
                    json["machines"][machine_id]["batches"][batch_id]["jobs"][job_pos]["weight"] = job.weight;
                    json["machines"][machine_id]["batches"][batch_id]["jobs"][job_pos]["processing_time"] = job.processing_times[machine_id];
                }
                json["machines"][machine_id]["batches"][batch_id]["start"] = machine_batch.start;
                json["machines"][machine_id]["batches"][batch_id]["size"] = machine_batch.size;
                json["machines"][machine_id]["batches"][batch_id]["processing_time"] = machine_batch.processing_time;
            }

            json["machines"][machine_id]["capacity"] = machine.capacity;
            json["machines"][machine_id]["makespan"] = solution_machine.makespan;
            json["machines"][machine_id]["total_flow_time"] = solution_machine.total_flow_time;
            json["machines"][machine_id]["total_tardiness"] = solution_machine.total_tardiness;
            json["machines"][machine_id]["maximum_lateness"] = solution_machine.maximum_lateness;
        }
        file << json.dump(4);
    }
}

nlohmann::json Solution::to_json() const
{
    return nlohmann::json {
        {"NumberOfJobs", this->number_of_jobs()},
        {"NumberOfBatches", this->number_of_batches()},
        {"NumberOfOverlappingBatches", this->number_of_overlapping_batches()},
        {"NumberOfOvercapacitatedBatches", this->number_of_overcapacitated_batches()},
        {"NumberOfFamilyViolations", this->number_of_family_violations()},
        {"NumberOfReleaseDateViolations", this->number_of_release_date_violations()},
        {"Makespan", this->makespan()},
        {"TotalFlowTime", this->total_flow_time()},
        {"TotalTardiness", this->total_tardiness()},
        {"MaximumLateness", this->maximum_lateness()},
    };
}

void Solution::format(
        std::ostream& os,
        int verbosity_level) const
{
    if (verbosity_level >= 1) {
        os
            << "Number of jobs:                " << this->number_of_jobs() << std::endl
            << "Number of batches:             " << this->number_of_batches() << std::endl
            << "# of overcapacitated batches:  " << this->number_of_overcapacitated_batches() << std::endl
            << "# family violations:           " << this->number_of_family_violations() << std::endl
            << "# release dates violations:    " << this->number_of_release_date_violations() << std::endl
            << "Feasible:                      " << this->feasible() << std::endl
            << "Makespan:                      " << this->makespan() << std::endl
            << "Total flow time:               " << this->total_flow_time() << std::endl
            << "Throughput:                    " << this->throughput() << std::endl
            << "Total tardiness:               " << this->total_tardiness() << std::endl
            << "Maximum lateness:              " << this->maximum_lateness() << std::endl
            ;
    }

    const Instance& instance = this->instance();
    if (verbosity_level >= 2) {
        os << std::right << std::endl
            << std::setw(12) << "Machine"
            << std::setw(12) << "Batch"
            << std::setw(12) << "Job"
            << std::setw(12) << "Size"
            << std::setw(12) << "Start"
            << std::setw(12) << "Proc. time"
            << std::endl
            << std::setw(12) << "-------"
            << std::setw(12) << "-----"
            << std::setw(12) << "---"
            << std::setw(12) << "----"
            << std::setw(12) << "-----"
            << std::setw(12) << "----------"
            << std::endl;
        for (MachineId machine_id = 0;
                machine_id < instance.number_of_machines();
                ++machine_id) {
            const Solution::Machine& solution_machine = this->machine(machine_id);
            for (BatchId batch_id = 0;
                    batch_id < (BatchId)solution_machine.batches.size();
                    ++batch_id) {
                const Solution::Batch& batch = solution_machine.batches[batch_id];
                for (JobId job_id: batch.jobs) {
                    os
                        << std::setw(12) << machine_id
                        << std::setw(12) << batch_id
                        << std::setw(12) << job_id
                        << std::setw(12) << batch.size
                        << std::setw(12) << batch.start
                        << std::setw(12) << batch.processing_time
                        << std::endl;
                }
            }
        }
    }
}
