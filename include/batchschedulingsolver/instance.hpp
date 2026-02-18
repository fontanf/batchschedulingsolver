#pragma once

#include "optimizationtools/utils/output.hpp"
#include "optimizationtools/utils/common.hpp"

#include <cstdint>
#include <vector>
#include <iostream>

namespace batchschedulingsolver
{

using MachineId = int64_t;
using JobId = int64_t;
using Time = int64_t;
using Size = int64_t;
using FamilyId = int64_t;
using Seed = int64_t;

enum class Objective
{
    Makespan,
    TotalFlowTime,
    Throughput,
    TotalTardiness,
    MaximumLateness,
};

std::istream& operator>>(
        std::istream& in,
        Objective& objective);

std::ostream& operator<<(
        std::ostream& os,
        Objective objective);

/**
 * Structure for a machine.
 */
struct Machine
{
    Size capacity = 1;
};

/**
 * Structure for a job.
 */
struct Job
{
    /** Processing-time on each machine. */
    std::vector<Time> processing_times;

    /** Size. */
    Size size = 1;

    /** Family. */
    FamilyId family_id = -1;

    /** Release date. */
    Time release_date = 0;

    /** Due date. */
    Time due_date = -1;

    /** Weight. */
    Time weight = 1;
};

/**
 * Instance class.
 */
class Instance
{

public:

    /*
     * Getters
     */

    /** Get the objective. */
    Objective objective() const { return objective_; }

    /** Get the number of machines. */
    MachineId number_of_machines() const { return machines_.size(); }

    /** Get a machine. */
    const Machine& machine(MachineId machine_id) const { return machines_[machine_id]; }

    /** Get the number of jobs. */
    JobId number_of_jobs() const { return jobs_.size(); }

    /** Get a job. */
    const Job& job(JobId job_id) const { return jobs_[job_id]; }

    /*
     * Export
     */

    /** Print the instance into a stream. */
    std::ostream& format(
            std::ostream& os,
            int verbosity_level = 1) const;

    /** Write the instance to a file. */
    void write(const std::string& instance_path) const;

private:

    /*
     * Private methods
     */

    /** Create an instance manually. */
    Instance() { }

    /*
     * Private attributes
     */

    /** Objective. */
    Objective objective_;

    /** Machines. */
    std::vector<Machine> machines_;

    /** Jobs. */
    std::vector<Job> jobs_;

    friend class InstanceBuilder;

};

}
