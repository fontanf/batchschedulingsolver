#pragma once

#include "batchschedulingsolver/instance.hpp"

namespace batchschedulingsolver
{

class InstanceBuilder
{

public:

    /** Constructor. */
    InstanceBuilder() { }

    /** Read instance from a file. */
    void read(
            const std::string& instance_path,
            const std::string& format);

    /** Set the objective. */
    void set_objective(Objective objective) { instance_.objective_ = objective; }

    /**
     * Set the number of machines.
     *
     * This resets the jobs of the instance.
     */
    void set_number_of_machines(MachineId number_of_machines);

    /** Set the capacity of a machine. */
    void set_machine_capacity(
            MachineId machine_id,
            Size capacity);

    /** Add a job. */
    JobId add_job();

    /** Set the processing-time of a job for a given machine. */
    void set_job_processing_time(
            JobId job_id,
            MachineId machine_id,
            Time processing_time);

    /** Set the size of a job. */
    void set_job_size(
            JobId job_id,
            Size size);

    /** Set the family of a job. */
    void set_job_family(
            JobId job_id,
            FamilyId family_id);

    /** Set the release date of a job. */
    void set_job_release_date(
            JobId job_id,
            Time release_date);

    /** Set the due date of a job. */
    void set_job_due_date(
            JobId job_id,
            Time due_date);

    /** Set the weight of a job. */
    void set_job_weight(
            JobId job_id,
            Time weight);

    /*
     * Build
     */

    /** Build. */
    Instance build();

private:

    /*
     * Private methods
     */

    /*
     * Private attributes
     */

    /** Instance. */
    Instance instance_;

};

}
