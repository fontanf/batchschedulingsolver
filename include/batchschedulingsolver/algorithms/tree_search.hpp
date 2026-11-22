/**
 * Tree search
 *
 * Forward branching scheme, batch by batch and job by job, with dominances
 * on the set of scheduled jobs: with an unbounded queue, it is a dynamic
 * programming algorithm over the subsets of jobs; with a bounded queue, it is
 * a beam search.
 *
 * Problems:
 * - 1, P | p-batch, B, size_j, r_j | C_max
 * - 1, P | p-batch, B, size_j, r_j | sum w_j F_j
 * - 1, P | p-batch, B, size_j, r_j | sum w_j T_j
 *
 * See doc/tree_search.md.
 */

#pragma once

#include "batchschedulingsolver/algorithm_formatter.hpp"

namespace batchschedulingsolver
{

struct TreeSearchParameters: Parameters
{
    /** Minimum size of the queue of the iterative beam search. */
    int64_t minimum_size_of_the_queue = 1;

    /** Maximum size of the queue of the iterative beam search. */
    int64_t maximum_size_of_the_queue = 100000000;


    virtual int format_width() const override { return 37; }

    virtual void format(std::ostream& os) const override
    {
        Parameters::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Minimum size of the queue: " << minimum_size_of_the_queue << std::endl
            << std::setw(width) << std::left << "Maximum size of the queue: " << maximum_size_of_the_queue << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Parameters::to_json();
        json.merge_patch({
                {"MinimumSizeOfTheQueue", minimum_size_of_the_queue},
                {"MaximumSizeOfTheQueue", maximum_size_of_the_queue},
                });
        return json;
    }
};

Output tree_search(
        const Instance& instance,
        const TreeSearchParameters& parameters = {});

}
