#pragma once

#include "batchschedulingsolver/algorithm_formatter.hpp"

namespace batchschedulingsolver
{


  

    Output greedy_longest_processing_time(
        const Instance& instance,
        const Parameters& parameters = {}
    );

    
}