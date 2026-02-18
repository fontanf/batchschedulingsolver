#include "batchschedulingsolver/solution_builder.hpp"

using namespace batchschedulingsolver;

SolutionBuilder& SolutionBuilder::set_instance(
        const Instance& instance)
{
    solution_.instance_ = &instance;
    solution_.machines_ = std::vector<Solution::Machine>(instance.number_of_machines());
    solution_.jobs_ = std::vector<Solution::Job>(instance.number_of_jobs());
    return *this;
}

void SolutionBuilder::append_batch(
        MachineId machine_id,
        Time start)
{
    Solution::Machine& solution_machine = this->solution_.machines_[machine_id];
    Solution::Batch& previous_batch = solution_machine.batches.back();

    BatchId batch_id = solution_machine.batches.size();
    Solution::Batch batch;
    batch.start = start;
    this->solution_.machines_[machine_id].batches.push_back(batch);
}

void SolutionBuilder::add_job_to_last_batch(
        MachineId machine_id,
        JobId job_id)
{
    const Job& job = this->solution_.instance().job(job_id);
    const Machine& machine = this->solution_.instance().machine(machine_id);
    Solution::Machine& solution_machine = this->solution_.machines_[machine_id];
    Solution::Batch& solution_batch = solution_machine.batches.back();
    solution_batch.jobs.push_back(job_id);
}

Solution SolutionBuilder::build()
{
    const Instance& instance = this->solution_.instance();
    this->solution_.jobs_ = std::vector<Solution::Job>(instance.number_of_jobs());
    // Check constraints.
    for (MachineId machine_id = 0;
            machine_id < instance.number_of_machines();
            ++machine_id) {
        const Machine& machine = instance.machine(machine_id);
        Solution::Machine& solution_machine = this->solution_.machines_[machine_id];
        for (BatchId batch_id = 0;
                batch_id < (BatchId)solution_machine.batches.size();
                ++batch_id) {
            Solution::Batch& batch = solution_machine.batches[batch_id];
            // Update number of overlapping batches.
            if (batch_id > 0 && batch.start
                    < solution_machine.batches[batch_id - 1].start
                    + solution_machine.batches[batch_id - 1].processing_time) {
                this->solution_.number_of_overlapping_batches_++;
            }
            FamilyId family_id = -1;
            for (JobId job_pos = 0;
                    job_pos < (JobId)batch.jobs.size();
                    ++job_pos) {
                JobId job_id = batch.jobs[job_pos];
                const Job& job = instance.job(job_id);
                Solution::Job& solution_job = this->solution_.jobs_[job_id];
                // Check for duplicate jobs in the solution.
                if (this->solution_.contains(job_id)) {
                    throw std::invalid_argument(
                            FUNC_SIGNATURE + ": "
                            "duplicate job in solution; "
                            "job_id: " + std::to_string(job_id) + ".");
                }
                // Update number of family violations.
                if (family_id == -1)
                    family_id = job.family_id;
                if (job.family_id != family_id)
                    solution_.number_of_family_violations_++;
                // Update number of release date violations.
                if (job.release_date > batch.start)
                    this->solution_.number_of_release_date_violations_++;
                // Update solution job.
                solution_job.machine_id = machine_id;
                solution_job.batch_id = batch_id;
                solution_job.job_pos = job_pos;
                // Update batch.
                batch.size += job.size;
                batch.processing_time = (std::max)(
                        batch.processing_time,
                        job.processing_times[machine_id]);
                // Update solution.
                this->solution_.number_of_jobs_++;
            }
            // Update number of overcapacitated batches.
            if (batch.size > machine.capacity)
                this->solution_.number_of_overcapacitated_batches_++;
            // Update makespan.
            Time completion_time = batch.start + batch.processing_time;
            solution_machine.makespan = (std::max)(
                    solution_machine.makespan,
                    completion_time);
            for (JobId job_pos = 0;
                    job_pos < (JobId)batch.jobs.size();
                    ++job_pos) {
                JobId job_id = batch.jobs[job_pos];
                const Job& job = instance.job(job_id);
                // Update total flow time.
                solution_machine.total_flow_time
                    += job.weight * (completion_time - job.release_date);
                if (completion_time > job.due_date) {
                    // Update total tardiness.
                    solution_machine.total_tardiness
                        += job.weight * (job.due_date - completion_time);
                    // Update maximum lateness.
                    solution_machine.maximum_lateness = (std::max)(
                            solution_machine.maximum_lateness,
                            job.due_date - completion_time);
                }
            }
        }
        // Update solution.
        this->solution_.number_of_batches_ += solution_machine.batches.size();
        this->solution_.makespan_ = (std::max)(
                solution_.makespan_,
                solution_machine.makespan);
        this->solution_.total_flow_time_ += solution_machine.total_flow_time;
        this->solution_.total_tardiness_ += solution_machine.total_tardiness;
        this->solution_.maximum_lateness_ = (std::max)(
                solution_.maximum_lateness_,
                solution_machine.maximum_lateness);
    }
    return std::move(solution_);
}

void SolutionBuilder::read(
        const std::string& certificate_path)
{
    std::ifstream file(certificate_path);
    if (!file.good()) {
        throw std::runtime_error(
                FUNC_SIGNATURE + ": "
                "unable to open file \"" + certificate_path + "\".");
    }

    nlohmann::json j;
    file >> j;
    const Instance& instance = this->solution_.instance();

    for (MachineId machine_id = 0;
            machine_id < instance.number_of_machines();
            ++machine_id) {
        for (const auto& json_batch: j["machines"][machine_id]["batches"]) {
            this->append_batch(
                    machine_id,
                    json_batch["start"]);
            for (const auto& json_job: json_batch["jobs"]) {
                this->add_job_to_last_batch(
                        machine_id,
                        json_job["job_id"]);
            }
        }
    }
}
