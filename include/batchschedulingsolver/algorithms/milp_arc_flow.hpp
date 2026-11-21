/**
 * Mixed-integer linear programming, arc-flow model
 *
 * See:
 * - "Arc-flow approach for single batch-processing machine scheduling"
 *   (Trindade et al., 2021)
 *
 * This model is only suited for the single-machine makespan objective
 * (1|s_j, B|C_max).
 */

#pragma once

#include "batchschedulingsolver/algorithm_formatter.hpp"

#include "mathoptsolverscmake/common.hpp"

namespace batchschedulingsolver
{

struct MilpArcFlowParameters: Parameters
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

Output milp_arc_flow(
        const Instance& instance,
        const MilpArcFlowParameters& parameters = {});

}
