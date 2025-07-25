#pragma once

#include "batchschedulingsolver/instance.hpp"

namespace batchschedulingsolver
{

using BatchId = int64_t;

/**
 * Solution class.
 */
class Solution
{

public:

    /**
     * Structure for a batch of a solution.
     */
    struct Batch
    {
        /** Jobs. */
        std::vector<JobId> jobs;

        /** Total size of the batch. */
        Size size = 0;

        /** Start time of the batch. */
        Time start = 0;

        /** Processing-time of the batch. */
        Time processing_time = 0;
    };

    /**
     * Structure for a machine schedule of a solution.
     */
    struct Machine
    {
        /** Batches scheduled on the machine. */
        std::vector<Batch> batches;

        /** Makespan. */
        Time makespan = 0;

        /** Total (weighted) flow time. */
        Time total_flow_time = 0;

        /** Total (weighted) tardiness. */
        Time total_tardiness = 0;

        /** Maximum lateness. */
        Time maximum_lateness = 0;
    };

    struct Job
    {
        MachineId machine_id = -1;

        BatchId batch_id = -1;

        JobId job_pos = -1;
    };

    /*
     * Getters
     */

    /** Get the instance of the solution. */
    const Instance& instance() const { return *this->instance_; }

    /** Get a machine schedule. */
    const Machine& machine(MachineId machine_id) const { return this->machines_[machine_id]; }

    /** Get the number of jobs in the solution. */
    JobId number_of_jobs() const { return this->number_of_jobs_; }

    /** Get the number of batches in the solution. */
    JobId number_of_batches() const { return this->number_of_batches_; }

    /** Get the info of a job in the solution. */
    const Job& job(JobId job_id) const { return this->jobs_[job_id]; }

    /** Check if the solution contains a given job. */
    bool contains(JobId job_id) const { return this->jobs_[job_id].machine_id != -1; }

    /** Get the number of overcapacitated batches. */
    BatchId number_of_overcapacitated_batches() const { return this->number_of_overlapping_batches_; }

    /** Get the number of overlapping batches. */
    BatchId number_of_overlapping_batches() const { return this->number_of_overlapping_batches_; }

    /** Get the number of violations of the family constraint. */
    JobId number_of_family_violations() const { return this->number_of_family_violations_; }

    /** Get the number of released dates violated. */
    JobId number_of_release_date_violations() const { return this->number_of_release_date_violations_; }

    /** Get the makespan of the solution. */
    Time makespan() const { return makespan_; }

    /** Get the total (weighted) flow time of the solution. */
    Time total_flow_time() const { return total_flow_time_; }

    /** Get the total_tardiness of the solution. */
    Time total_tardiness() const { return total_tardiness_; }

    /** Get the maximum lateness of the solution. */
    Time maximum_lateness() const { return maximum_lateness_; }

    /** Check if the solution is feasible. */
    bool feasible() const;

    /** Check if the solution is strictly better than another solution. */
    bool strictly_better(const Solution& solution) const;

    /*
     * Export
     */

    /** Write the solution to a file. */
    void write(
            const std::string& certificate_path,
            const std::string& format) const;

    /** Export solution characteristics to a JSON structure. */
    nlohmann::json to_json() const;

    /** Write a formatted output of the instance to a stream. */
    void format(
            std::ostream& os,
            int verbosity_level = 1) const;

private:

    /*
     * Private attributes
     */

    /** Instance. */
    const Instance* instance_;

    /** Machines. */
    std::vector<Machine> machines_;

    /** Jobs. */
    std::vector<Job> jobs_;

    /** Number of jobs. */
    JobId number_of_jobs_ = 0;

    /** Number of batches. */
    JobId number_of_batches_ = 0;

    /** Batch capacity constraint violation. */
    BatchId number_of_overcapacitated_batches_ = 0;

    /** Batch overlap on a machine. */
    BatchId number_of_overlapping_batches_ = 0;

    /** Number of violations of the family constraint. */
    JobId number_of_family_violations_ = 0;

    /** Number of released dates violated. */
    JobId number_of_release_date_violations_ = 0;

    /** Makespan. */
    Time makespan_ = 0;

    /** Total (weighted) flow time. */
    Time total_flow_time_ = 0;

    /** Total (weighted) tardiness. */
    Time total_tardiness_ = 0;

    /** Maximum lateness. */
    Time maximum_lateness_ = 0;

    friend class SolutionBuilder;

};

}
