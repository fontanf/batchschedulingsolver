#include "batchschedulingsolver/solution.hpp"

using namespace batchschedulingsolver;

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
            "batchchedulingsolver::Solution::write: "
            "Unable to open file \"" + certificate_path + "\".");
    }
    nlohmann::json json;
    nlohmann::json machines_json = nlohmann::json::array();

    if (format == "json") {
        std::cout << "Format JSON reconnu";
        const Instance& instance = this->instance();
        for (MachineId machine_id = 0;
            machine_id < instance.number_of_machines();
            ++machine_id) {

            const Machine& solution_machine = this->machine(machine_id);
            nlohmann::json batches_json = nlohmann::json::array();

            for (BatchId batch_id = 0;
                batch_id < solution_machine.batches.size();
                ++batch_id) {
                const Batch& machine_batch = solution_machine.batches[batch_id];
                nlohmann::json jobs_array = nlohmann::json::array();

                for (JobId job_pos = 0;
                    job_pos < (JobId)machine_batch.jobs.size();
                    ++job_pos) {
                    JobId job_id = machine_batch.jobs[job_pos];
                    const batchschedulingsolver::Job& job = instance.job(job_id);

                    jobs_array.push_back({
                        {"Job_ID", job_id},
                        {"Job_Size", job.size},
                        {"Job_FamilyID", job.family_id},
                        {"Job_Due_Date", job.due_date},
                        {"Job_Release_Date", job.release_date},
                        {"Job_Weight", job.weight},
                        {"Job_Processing_Time", job.processing_times[machine_id]}
                        });
                }
                batches_json.push_back({
                    {"Jobs", jobs_array},
                    {"Batch_Start", machine_batch.start},
                    {"Batch_Size", machine_batch.size},
                    {"Batch_Processing_Time", machine_batch.processing_time}
                    });
            }

            machines_json.push_back({
                {"Machine_ID", +machine_id },
                {"Makespan", solution_machine.makespan},
                {"Machine_Capacity", this->instance().machine(machine_id).capacity},
                {"Total_Flow_Time", solution_machine.total_flow_time},
                {"Total_Tardiness", solution_machine.total_tardiness},
                {"Maximum_Lateness", solution_machine.maximum_lateness},
                {"Batches",batches_json}
                });

            json["Machine_" + std::to_string(machine_id)] = machines_json;
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
