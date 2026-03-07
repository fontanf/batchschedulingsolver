#pragma once

#include "batchschedulingsolver/solution.hpp"

namespace batchschedulingsolver
{

class SolutionBuilder
{

public:

    /** Constructor. */
    SolutionBuilder() { }

    /** Set the instance of the solution. */
    SolutionBuilder& set_instance(const Instance& instance);

    /** Read a solution from a file. */
    void read(
            const std::string& certificate_path,
            const std::string& format = "default");

    /** Add a new batch to a machine. */
    void append_batch(
            MachineId machine_id,
            Time start);

    /** Add a job to a batch. */
    void add_job_to_last_batch(
            MachineId machine_id,
            JobId job_id);

    /*
     * Build
     */

    /** Build. */
    Solution build();

private:

    /*
     * Private methods
     */

    /*
     * Private attributes
     */

    /** Solution. */
    Solution solution_;

};

}
