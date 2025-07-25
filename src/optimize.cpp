#include "batchschedulingsolver/optimize.hpp"

using namespace batchschedulingsolver;

Output batchschedulingsolver::optimize(
        const Instance& instance,
        const OptimizeParameters& parameters)
{
    Output output;
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.print_header();

    // TODO

    algorithm_formatter.end();
    return output;
}
