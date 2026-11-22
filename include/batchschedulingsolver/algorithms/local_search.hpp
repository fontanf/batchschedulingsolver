/**
 * Local search
 *
 * Population-based iterated local search, with the neighborhoods of Queiroga
 * et al. (2020) for the single machine, extended to parallel machines as in
 * Medeiros et al. (2024): job insertion (into another batch or as a new
 * batch), job swap, batch insertion, batch swap and batch merge.
 *
 * Problems:
 * - 1, P, R | p-batch, B_i, size_j, r_j, families | C_max
 * - 1, P, R | p-batch, B_i, size_j, r_j, families | sum w_j F_j
 * - 1, P, R | p-batch, B_i, size_j, r_j, families | sum w_j T_j
 * - 1, P, R | p-batch, B_i, size_j, r_j, families | L_max
 *
 * See doc/local_search.md.
 */

#pragma once

#include "batchschedulingsolver/algorithm_formatter.hpp"

#include <random>

namespace batchschedulingsolver
{

using Counter = int64_t;

struct LocalSearchParameters: Parameters
{
    /** Maximum number of iterations. */
    Counter maximum_number_of_iterations = -1;

    /** Maximum number of iterations without improvement. */
    Counter maximum_number_of_iterations_without_improvement = -1;


    virtual int format_width() const override { return 37; }

    virtual void format(std::ostream& os) const override
    {
        Parameters::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Max. # of iterations: " << maximum_number_of_iterations << std::endl
            << std::setw(width) << std::left << "Max. # of iterations without impr.: " << maximum_number_of_iterations_without_improvement << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Parameters::to_json();
        json.merge_patch({
                {"MaximumNumberOfIterations", maximum_number_of_iterations},
                {"MaximumNumberOfIterationsWithoutImprovement", maximum_number_of_iterations_without_improvement},
                });
        return json;
    }
};

struct LocalSearchOutput: Output
{
    LocalSearchOutput(
            const Instance& instance):
        Output(instance) { }


    /** Number of iterations. */
    Counter number_of_iterations = 0;


    virtual int format_width() const override { return 31; }

    virtual void format(std::ostream& os) const override
    {
        Output::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Number of iterations: " << number_of_iterations << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Output::to_json();
        json.merge_patch({
                {"NumberOfIterations", this->number_of_iterations},
                });
        return json;
    }
};

const LocalSearchOutput local_search(
        const Instance& instance,
        std::mt19937_64& generator,
        const Solution* initial_solution = nullptr,
        const LocalSearchParameters& parameters = {});

}
