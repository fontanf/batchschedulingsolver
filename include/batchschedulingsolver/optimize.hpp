#pragma once

#include "batchschedulingsolver/instance.hpp"
#include "batchschedulingsolver/algorithm_formatter.hpp"

namespace batchschedulingsolver
{

struct OptimizeOutput: batchschedulingsolver::Output
{
};

struct OptimizeParameters: batchschedulingsolver::Parameters
{
};

Output optimize(
        const Instance& instance,
        const OptimizeParameters& parameters = {});

}
