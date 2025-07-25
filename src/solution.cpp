#include "batchschedulingsolver/solution.hpp"

using namespace batchschedulingsolver;

bool Solution::feasible() const
{
    return (this->number_of_overlapping_batches_ == 0
        && this->number_of_family_violations_ == 0
        && this->number_of_release_date_violations_ == 0
        && this->number_of_jobs_ == instance().number_of_jobs());
}

bool Solution::strictly_better(const Solution& solution) const
{
    if (!this->feasible())
        return false;
    if (!solution.feasible())
        return true;
    switch (solution.instance().objective()) {
    case Objective::Makespan: {
        return this->makespan() < solution.makespan();
    } case Objective::TotalFlowTime: {
        return this->total_flow_time() < solution.total_flow_time();
    } case Objective::TotalTardiness: {
        return this->total_tardiness() < solution.total_tardiness();
    } case Objective::MaximumLateness: {
        return this->maximum_lateness() < solution.maximum_lateness();
    }
    }
    return false;
}

void Solution::write(
        const std::string& certificate_path,
        const std::string& format) const
{
    // TODO
}

nlohmann::json Solution::to_json() const
{
    return nlohmann::json {
        {"NumberOfJobs", this->number_of_jobs()},
        {"NumberOfBatches", this->number_of_batches()},
        {"NumberOfOverlappingBatches", this->number_of_overlapping_batches()},
        {"NumberOfOvercapacitatedBatches", this->number_of_overcapacitated_batches()},
        {"NumberOfFamilyViolations", this->number_of_family_violations()},
        {"NumberOfReleaseDateViolations", this->number_of_release_date_violations()},
        {"Makespan", this->makespan()},
        {"TotalFlowTime", this->total_flow_time()},
        {"TotalTardiness", this->total_tardiness()},
        {"MaximumLateness", this->maximum_lateness()},
    };
}

void Solution::format(
        std::ostream& os,
        int verbosity_level) const
{
    // TODO
}
