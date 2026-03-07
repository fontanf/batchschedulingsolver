#pragma once

#include "batchschedulingsolver/algorithm_formatter.hpp"

#include "mathoptsolverscmake/milp.hpp"

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

Output milp_rank_based(
        const Instance& instance,
        const MilpRankBasedParameters& parameters = {});

Output milp_rank_based_release_dates(
        const Instance& instance,
        const MilpRankBasedParameters& parameters = {});

Output milp_rank_based_parallel(
        const Instance& instance,
        const MilpRankBasedParameters& parameters = {});

Output milp_rank_based_parallel_release_dates(
        const Instance& instance,
        const MilpRankBasedParameters& parameters = {});

}
