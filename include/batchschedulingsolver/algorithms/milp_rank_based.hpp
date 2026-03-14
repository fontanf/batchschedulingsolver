/**
 * Mixed-integer linear programming, rank-based models
 *
 * See:
 * - "Modelling and symmetry breaking in scheduling problems on batch processing
 *   machines" (Trindad et al., 2018)
 *
 * The models are generalized to handle the more variants.
 *
 * The models without batch starts are only suited for the makespan objective.
 *
 * The two-index models don't support unrelated parallel machines.
 */

#pragma once

#include "batchschedulingsolver/algorithm_formatter.hpp"

#include "mathoptsolverscmake/common.hpp"

namespace batchschedulingsolver
{

struct MilpRankBasedParameters: Parameters
{
    mathoptsolverscmake::SolverName solver = mathoptsolverscmake::SolverName::Highs;


    virtual int format_width() const override { return 37; }

    virtual void format(std::ostream& os) const override
    {
        Parameters::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Solver: " << solver << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Parameters::to_json();
        json.merge_patch({
                //{"Solver", solver},
                });
        return json;
    }
};

Output milp_rank_based_three_index_no_starts(
        const Instance& instance,
        const MilpRankBasedParameters& parameters = {});

Output milp_rank_based_three_index(
        const Instance& instance,
        const MilpRankBasedParameters& parameters = {});

Output milp_rank_based_two_index_no_starts(
        const Instance& instance,
        const MilpRankBasedParameters& parameters = {});

Output milp_rank_based_two_index(
        const Instance& instance,
        const MilpRankBasedParameters& parameters = {});

}
