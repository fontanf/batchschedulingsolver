#include "batchschedulingsolver/algorithms/greedy_longest_processing_time.hpp"

#include "batchschedulingsolver/algorithm_formatter.hpp"


using namespace batchschedulingsolver;

Output batchschedulingsolver::greedy_longest_processing_time(
    const Instance& instance,
    const Parameters& parameters)
{
    Output output(instance);
    AlgorithmFormatter algorithm_formatter(instance,parameters,output);
    algorithm_formatter.start("greedy_longest_processing_time");

    algorithm_formatter.print_header();

    algorithm_formatter.end();
    return output;
}