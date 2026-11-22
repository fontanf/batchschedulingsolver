/**
 * Column generation
 *
 * Arc-based graph formulation of Alfieri et al. (2021) and Druetto and Grosso
 * (2022), solved by column generation with columngenerationsolver; the
 * solutions are built by the limited discrepancy search of
 * columngenerationsolver instead of the rounding heuristics of the papers.
 *
 * Problems:
 * - 1 | p-batch, B, size_j | sum C_j
 * - 1 | p-batch, B, size_j | sum w_j C_j
 * - 1 | p-batch, B, size_j, incompatible families | sum w_j C_j
 *
 * See doc/column_generation.md.
 */

#pragma once

#include "batchschedulingsolver/algorithm_formatter.hpp"

#include "mathoptsolverscmake/common.hpp"

namespace batchschedulingsolver
{

struct ColumnGenerationParameters: Parameters
{
    /** Linear programming solver. */
    mathoptsolverscmake::SolverName solver = mathoptsolverscmake::SolverName::Highs;

    /**
     * Maximum number of columns returned by a call to the pricing algorithm.
     *
     * If negative, n + W, where W is the sum of the weights of the jobs.
     */
    int64_t maximum_number_of_columns_per_pricing = -1;

    /** Maximum discrepancy of the limited discrepancy search. */
    double discrepancy_limit = std::numeric_limits<double>::infinity();


    virtual int format_width() const override { return 37; }

    virtual void format(std::ostream& os) const override
    {
        Parameters::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Solver: " << solver << std::endl
            << std::setw(width) << std::left << "Max. # of columns per pricing: " << maximum_number_of_columns_per_pricing << std::endl
            << std::setw(width) << std::left << "Discrepancy limit: " << discrepancy_limit << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Parameters::to_json();
        json.merge_patch({
                {"MaximumNumberOfColumnsPerPricing", maximum_number_of_columns_per_pricing},
                {"DiscrepancyLimit", discrepancy_limit},
                });
        return json;
    }
};

struct ColumnGenerationOutput: Output
{
    ColumnGenerationOutput(
            const Instance& instance):
        Output(instance) { }


    /** Value of the linear relaxation at the root node. */
    double root_relaxation_value = 0.0;

    /** Number of nodes of the limited discrepancy search. */
    int64_t number_of_nodes = 0;

    /** Number of column generation iterations. */
    int64_t number_of_column_generation_iterations = 0;

    /** Number of generated columns. */
    int64_t number_of_columns = 0;

    /** Time spent solving the linear programs. */
    double time_lpsolve = 0.0;

    /** Time spent solving the pricing problems. */
    double time_pricing = 0.0;


    virtual int format_width() const override { return 31; }

    virtual void format(std::ostream& os) const override
    {
        Output::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Root relaxation value: " << root_relaxation_value << std::endl
            << std::setw(width) << std::left << "Number of nodes: " << number_of_nodes << std::endl
            << std::setw(width) << std::left << "Number of CG iterations: " << number_of_column_generation_iterations << std::endl
            << std::setw(width) << std::left << "Number of columns: " << number_of_columns << std::endl
            << std::setw(width) << std::left << "Time LP solve: " << time_lpsolve << std::endl
            << std::setw(width) << std::left << "Time pricing: " << time_pricing << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Output::to_json();
        json.merge_patch({
                {"RootRelaxationValue", root_relaxation_value},
                {"NumberOfNodes", number_of_nodes},
                {"NumberOfColumnGenerationIterations", number_of_column_generation_iterations},
                {"NumberOfColumns", number_of_columns},
                {"TimeLpSolve", time_lpsolve},
                {"TimePricing", time_pricing},
                });
        return json;
    }
};

const ColumnGenerationOutput column_generation(
        const Instance& instance,
        const ColumnGenerationParameters& parameters = {});

}
