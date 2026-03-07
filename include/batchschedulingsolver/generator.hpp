#pragma once

#include "batchschedulingsolver/instance.hpp"

#include <random>

namespace batchschedulingsolver
{

struct GenerateInput
{
    Objective objective = Objective::Makespan;
    MachineId number_of_machines = 1;
    MachineId number_of_batches_per_machine = 3;
    MachineId number_of_jobs_per_batch = 3;
    bool identical_sizes = false;
    Time capacity = 100;
    Time processing_times_range = 100;
    Time weights_range = 1;
};

Instance generate(
        const GenerateInput& input,
        std::mt19937_64& generator);

}
