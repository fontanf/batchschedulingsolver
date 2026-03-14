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
    bool machine_independent_processing_times = false;
    Time processing_times_range = 100;
    double release_dates_dispersion_factor = 0;
    double due_dates_tightness_factor = 1;
    Time weights_range = 1;
};

Instance generate(
        const GenerateInput& input,
        std::mt19937_64& generator);

}
