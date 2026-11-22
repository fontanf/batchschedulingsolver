/**
 * Column generation for 1 | p-batch, B, size_j, families | sum w_j C_j
 *
 * Arc-based model of Druetto and Grosso (2022), which generalizes the model
 * of Alfieri et al. (2021) for sum C_j to the weighted case.
 *
 * Let W be the sum of the weights of the jobs. The graph has the nodes
 * 0, 1, ..., W. Node t means that the jobs of total weight t are scheduled.
 * An arc (t, t + w(B), B) is a batch B, scheduled after the jobs of total
 * weight t. Since the jobs of the batch and the jobs of the following
 * batches, of total weight W - t, all complete after the end of B, the cost
 * of the arc is (W - t) p(B), and the cost of a path from 0 to W is the total
 * weighted completion time of the corresponding sequence of batches.
 *
 * Master problem:
 *
 * min sum_{(t, k, B)} (W - t) p(B) x_{t, k, B}
 *
 * sum_{(t, k, B)} x_{t, k, B} - sum_{(k, t, B)} x_{k, t, B} = 1 if t = 0, 0 if 0 < t < W
 *                                                    (flow, duals u_t)
 * sum_{(t, k, B), j in B} x_{t, k, B} = 1        for each job j
 *                                                    (partition, duals v_j)
 *
 * The flow conservation constraint of node W is redundant and omitted
 * (u_W = 0).
 *
 * Pricing: the reduced cost of an arc is
 *   (W - t) p(B) - u_t + u_{t + w(B)} - sum_{j in B} v_j.
 * For each family, the jobs are sorted by non-increasing processing time, and
 * g_r(τ, ℓ) is the largest sum of v_j over the subsets of the jobs r, r + 1,
 * ... of the family of total size at most τ and total weight exactly ℓ. For
 * each weight ℓ and each node t, the arc of minimum reduced cost is one of
 * the batches B_r(B, ℓ) of g_r(B, ℓ), r being the first job of each
 * processing time (Druetto and Grosso, 2022, Property 2). For a given ℓ, the
 * value (W - t) p_r - g_r(B, ℓ) is a line in (W - t), so the minimum over r
 * for all t is computed with the lower envelope of these lines.
 *
 * The solutions are built by the limited discrepancy search of
 * columngenerationsolver, which fixes columns (arcs) to 1.
 */

#include "batchschedulingsolver/algorithms/column_generation.hpp"

#include "batchschedulingsolver/solution_builder.hpp"

#include "columngenerationsolver/commons.hpp"
#include "columngenerationsolver/algorithms/limited_discrepancy_search.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

using namespace batchschedulingsolver;

namespace cgs = columngenerationsolver;

namespace
{

using Weight = Time;

/** Information stored in each column. */
struct ArcInfo
{
    /** Start node: total weight of the jobs scheduled before the batch. */
    Weight start;

    /** Jobs of the batch. */
    std::vector<JobId> jobs;

    /** Processing time of the batch. */
    Time processing_time;
};

const ArcInfo& arc_info(const cgs::Column& column)
{
    return *std::static_pointer_cast<const ArcInfo>(column.extra);
}

/** Line y = slope * x + intercept, for the lower envelope. */
struct Line
{
    double slope;

    double intercept;

    /** Family of the batch. */
    FamilyId family_pos;

    /** Position of the first job in the sorted jobs of the family. */
    JobId job_pos;

    double value(double x) const { return slope * x + intercept; }
};

class PricingSolver: public cgs::PricingSolver
{

public:

    PricingSolver(
            const Instance& instance,
            Weight total_weight,
            int64_t maximum_number_of_columns,
            bool source_first_branching):
        instance_(instance),
        total_weight_(total_weight),
        maximum_number_of_columns_(maximum_number_of_columns),
        source_first_branching_(source_first_branching)
    {
        // Group the jobs by family, sorted by non-increasing processing time.
        std::vector<FamilyId> families;
        for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id)
            families.push_back(instance.job(job_id).family_id);
        std::sort(families.begin(), families.end());
        families.erase(std::unique(families.begin(), families.end()), families.end());
        family_jobs_.resize(families.size());
        for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
            FamilyId family_pos = std::lower_bound(
                    families.begin(),
                    families.end(),
                    instance.job(job_id).family_id) - families.begin();
            family_jobs_[family_pos].push_back(job_id);
        }
        for (std::vector<JobId>& jobs: family_jobs_) {
            std::sort(
                    jobs.begin(),
                    jobs.end(),
                    [&instance](JobId job_id_1, JobId job_id_2)
                    {
                        Time p1 = instance.job(job_id_1).processing_times[0];
                        Time p2 = instance.job(job_id_2).processing_times[0];
                        if (p1 != p2)
                            return p1 > p2;
                        return job_id_1 < job_id_2;
                    });
        }
        takes_.resize(family_jobs_.size());
        weight_limits_.resize(family_jobs_.size());
    }

    /** Row of the flow conservation constraint of node t (t < W). */
    cgs::RowIdx flow_row(Weight t) const { return t; }

    /** Row of the partition constraint of job j. */
    cgs::RowIdx job_row(JobId job_id) const { return total_weight_ + job_id; }

    /** Build the column of an arc. */
    std::shared_ptr<const cgs::Column> build_column(
            Weight start,
            const std::vector<JobId>& jobs) const;

    virtual std::vector<std::shared_ptr<const cgs::Column>> infeasible_columns(
            const std::vector<std::shared_ptr<const cgs::Column>>& columns,
            const std::vector<std::pair<std::shared_ptr<const cgs::Column>, cgs::Value>>& fixed_columns,
            const std::vector<std::shared_ptr<const cgs::BranchingDecision>>& branching_decisions) const override;

    virtual PricingOutput solve_pricing(
            bool solve_feasibility,
            const std::vector<std::pair<std::shared_ptr<const cgs::Column>, cgs::Value>>& fixed_columns,
            const std::vector<std::shared_ptr<const cgs::BranchingDecision>>& branching_decisions,
            const std::unordered_set<std::shared_ptr<const cgs::Column>>& tabu,
            const std::vector<cgs::Value>& duals,
            const std::vector<std::pair<std::shared_ptr<const cgs::Cut>, cgs::Value>>& cut_duals,
            PricingType pricing_type,
            cgs::Value overcost_goal) override;

private:

    /** State of the search given the fixed columns. */
    struct FixedState
    {
        /** For each job, true iff it is in a fixed column. */
        std::vector<uint8_t> fixed_jobs;

        /** For each node t, true iff t is in [start, end) of a fixed arc. */
        std::vector<uint8_t> blocked_nodes;

        /**
         * For each node t, the smallest start of a fixed arc not smaller
         * than t, or W.
         */
        std::vector<Weight> next_fixed_starts;

        /** Number of jobs which are not in a fixed column. */
        JobId number_of_free_jobs = 0;
    };

    FixedState compute_fixed_state(
            const std::vector<std::pair<std::shared_ptr<const cgs::Column>, cgs::Value>>& fixed_columns) const;

    /** Return true iff an arc (start, end) is compatible with the fixed arcs. */
    static bool arc_allowed(
            const FixedState& fixed_state,
            Weight start,
            Weight end)
    {
        return !fixed_state.blocked_nodes[start]
            && end <= fixed_state.next_fixed_starts[start];
    }

    /** Index in 'takes_[family_pos]'. */
    size_t take_index(
            FamilyId family_pos,
            JobId job_pos,
            Size size,
            Weight weight) const
    {
        Size capacity = instance_.machine(0).capacity;
        return ((size_t)job_pos * (capacity + 1) + size)
            * (weight_limits_[family_pos] + 1) + weight;
    }

    /** Retrieve the batch of g_r(B, ℓ) from the decisions of the DP. */
    std::vector<JobId> retrieve_batch(
            FamilyId family_pos,
            JobId job_pos,
            Weight weight) const;

    /** Instance. */
    const Instance& instance_;

    /** Sum of the weights of the jobs. */
    Weight total_weight_;

    /** Maximum number of columns returned by a call to the pricing. */
    int64_t maximum_number_of_columns_;

    /** Branching priority of an arc: minus its start node. */
    bool source_first_branching_;

    /** For each family, its jobs sorted by non-increasing processing time. */
    std::vector<std::vector<JobId>> family_jobs_;

    /** For each family, its free jobs (positions in 'family_jobs_'). */
    std::vector<std::vector<JobId>> free_job_positions_;

    /**
     * For each family, the decisions of the dynamic programming:
     * takes_[f][index(r, τ, ℓ)] = 1 iff the job r is in the batch of
     * g_r(τ, ℓ).
     */
    std::vector<std::vector<uint8_t>> takes_;

    /** For each family, the largest weight of the dynamic programming. */
    std::vector<Weight> weight_limits_;

};

std::shared_ptr<const cgs::Column> PricingSolver::build_column(
        Weight start,
        const std::vector<JobId>& jobs) const
{
    auto info = std::make_shared<ArcInfo>();
    info->start = start;
    info->jobs = jobs;
    std::sort(info->jobs.begin(), info->jobs.end());
    info->processing_time = 0;
    Weight weight = 0;
    for (JobId job_id: jobs) {
        const Job& job = instance_.job(job_id);
        info->processing_time = (std::max)(
                info->processing_time,
                job.processing_times[0]);
        weight += job.weight;
    }
    Weight end = start + weight;

    auto column = std::make_shared<cgs::Column>();
    column->objective_coefficient = (double)(total_weight_ - start) * info->processing_time;
    if (start < total_weight_)
        column->elements.push_back({flow_row(start), 1.0});
    if (end < total_weight_)
        column->elements.push_back({flow_row(end), -1.0});
    for (JobId job_id: info->jobs)
        column->elements.push_back({job_row(job_id), 1.0});
    if (source_first_branching_)
        column->branching_priority = -(double)start;
    column->extra = info;
    return column;
}

PricingSolver::FixedState PricingSolver::compute_fixed_state(
        const std::vector<std::pair<std::shared_ptr<const cgs::Column>, cgs::Value>>& fixed_columns) const
{
    FixedState fixed_state;
    fixed_state.fixed_jobs.resize(instance_.number_of_jobs(), 0);
    fixed_state.blocked_nodes.resize(total_weight_ + 1, 0);
    fixed_state.next_fixed_starts.resize(total_weight_ + 1, total_weight_);
    std::vector<uint8_t> is_fixed_start(total_weight_ + 1, 0);
    for (const auto& p: fixed_columns) {
        if (p.second < 0.5)
            continue;
        const ArcInfo& info = arc_info(*p.first);
        Weight end = info.start;
        for (JobId job_id: info.jobs) {
            fixed_state.fixed_jobs[job_id] = 1;
            end += instance_.job(job_id).weight;
        }
        for (Weight t = info.start; t < end; ++t)
            fixed_state.blocked_nodes[t] = 1;
        is_fixed_start[info.start] = 1;
    }
    for (Weight t = total_weight_ - 1; t >= 0; --t) {
        fixed_state.next_fixed_starts[t] = (is_fixed_start[t])?
            t: fixed_state.next_fixed_starts[t + 1];
    }
    for (JobId job_id = 0; job_id < instance_.number_of_jobs(); ++job_id)
        if (!fixed_state.fixed_jobs[job_id])
            fixed_state.number_of_free_jobs++;
    return fixed_state;
}

std::vector<std::shared_ptr<const cgs::Column>> PricingSolver::infeasible_columns(
        const std::vector<std::shared_ptr<const cgs::Column>>& columns,
        const std::vector<std::pair<std::shared_ptr<const cgs::Column>, cgs::Value>>& fixed_columns,
        const std::vector<std::shared_ptr<const cgs::BranchingDecision>>&) const
{
    std::vector<std::shared_ptr<const cgs::Column>> infeasible;
    if (fixed_columns.empty())
        return infeasible;
    FixedState fixed_state = compute_fixed_state(fixed_columns);
    std::unordered_set<const cgs::Column*> fixed;
    for (const auto& p: fixed_columns)
        fixed.insert(p.first.get());
    for (const auto& column: columns) {
        if (fixed.find(column.get()) != fixed.end())
            continue;
        if (column->extra == nullptr)
            continue;
        const ArcInfo& info = arc_info(*column);
        bool ok = true;
        Weight end = info.start;
        for (JobId job_id: info.jobs) {
            if (fixed_state.fixed_jobs[job_id])
                ok = false;
            end += instance_.job(job_id).weight;
        }
        if (ok && !arc_allowed(fixed_state, info.start, end))
            ok = false;
        if (!ok)
            infeasible.push_back(column);
    }
    return infeasible;
}

std::vector<JobId> PricingSolver::retrieve_batch(
        FamilyId family_pos,
        JobId job_pos,
        Weight weight) const
{
    const std::vector<JobId>& positions = free_job_positions_[family_pos];
    std::vector<JobId> jobs;
    Size size = instance_.machine(0).capacity;
    for (JobId pos = job_pos;
            pos < (JobId)positions.size() && weight > 0;
            ++pos) {
        if (takes_[family_pos][take_index(family_pos, pos, size, weight)]) {
            JobId job_id = family_jobs_[family_pos][positions[pos]];
            const Job& job = instance_.job(job_id);
            jobs.push_back(job_id);
            size -= job.size;
            weight -= job.weight;
        }
    }
    if (weight != 0) {
        throw std::logic_error(
                "batchschedulingsolver::column_generation: "
                "wrong batch retrieval.");
    }
    return jobs;
}

PricingSolver::PricingOutput PricingSolver::solve_pricing(
        bool solve_feasibility,
        const std::vector<std::pair<std::shared_ptr<const cgs::Column>, cgs::Value>>& fixed_columns,
        const std::vector<std::shared_ptr<const cgs::BranchingDecision>>&,
        const std::unordered_set<std::shared_ptr<const cgs::Column>>&,
        const std::vector<cgs::Value>& duals,
        const std::vector<std::pair<std::shared_ptr<const cgs::Cut>, cgs::Value>>&,
        PricingType,
        cgs::Value)
{
    const double inf = std::numeric_limits<double>::infinity();
    const Size capacity = instance_.machine(0).capacity;
    const Weight total_weight = total_weight_;
    FixedState fixed_state = compute_fixed_state(fixed_columns);

    auto flow_dual = [this, &duals](Weight t)
    {
        return (t < total_weight_)? duals[flow_row(t)]: 0.0;
    };

    // Lines of the lower envelope for each weight.
    std::vector<std::vector<Line>> lines(total_weight + 1);

    free_job_positions_.assign(family_jobs_.size(), {});
    for (FamilyId family_pos = 0;
            family_pos < (FamilyId)family_jobs_.size();
            ++family_pos) {
        const std::vector<JobId>& jobs = family_jobs_[family_pos];
        std::vector<JobId>& positions = free_job_positions_[family_pos];
        Weight weight_limit = 0;
        for (JobId pos = 0; pos < (JobId)jobs.size(); ++pos) {
            JobId job_id = jobs[pos];
            if (fixed_state.fixed_jobs[job_id])
                continue;
            positions.push_back(pos);
            weight_limit += instance_.job(job_id).weight;
        }
        if (positions.empty())
            continue;
        weight_limits_[family_pos] = weight_limit;
        JobId m = positions.size();
        size_t layer_size = (size_t)(capacity + 1) * (weight_limit + 1);
        takes_[family_pos].assign((size_t)m * layer_size, 0);

        // g_{r+1} and g_r.
        std::vector<double> next(layer_size, -inf);
        for (Size size = 0; size <= capacity; ++size)
            next[size * (weight_limit + 1)] = 0;
        std::vector<double> current(layer_size);

        for (JobId pos = m - 1; pos >= 0; --pos) {
            JobId job_id = jobs[positions[pos]];
            const Job& job = instance_.job(job_id);
            double profit = duals[job_row(job_id)];
            current = next;
            uint8_t* takes = takes_[family_pos].data() + (size_t)pos * layer_size;
            for (Size size = job.size; size <= capacity; ++size) {
                const double* from = next.data() + (size - job.size) * (weight_limit + 1);
                double* to = current.data() + size * (weight_limit + 1);
                uint8_t* t = takes + size * (weight_limit + 1);
                for (Weight weight = job.weight; weight <= weight_limit; ++weight) {
                    double value = from[weight - job.weight];
                    if (value == -inf)
                        continue;
                    value += profit;
                    if (value > to[weight]) {
                        to[weight] = value;
                        t[weight] = 1;
                    }
                }
            }
            // First job of its processing time among the free jobs of the
            // family.
            bool first = (pos == 0)
                || instance_.job(jobs[positions[pos - 1]]).processing_times[0]
                > job.processing_times[0];
            if (first) {
                double slope = (solve_feasibility)? 0.0: (double)job.processing_times[0];
                const double* g = current.data() + capacity * (weight_limit + 1);
                for (Weight weight = 1; weight <= weight_limit; ++weight) {
                    if (g[weight] == -inf)
                        continue;
                    lines[weight].push_back({slope, -g[weight], family_pos, pos});
                }
            }
            std::swap(current, next);
        }
    }

    // For each weight, compute the arcs of minimum reduced cost.
    struct Candidate
    {
        double reduced_cost;
        Weight start;
        Weight weight;
        FamilyId family_pos;
        JobId job_pos;
    };
    std::vector<Candidate> candidates;
    double reduced_cost_min = 0.0;
    const double tolerance = 1e-6;
    std::vector<Line> hull;
    for (Weight weight = 1; weight <= total_weight; ++weight) {
        std::vector<Line>& weight_lines = lines[weight];
        if (weight_lines.empty())
            continue;
        // Lower envelope, the slopes in decreasing order.
        std::sort(
                weight_lines.begin(),
                weight_lines.end(),
                [](const Line& line_1, const Line& line_2)
                {
                    if (line_1.slope != line_2.slope)
                        return line_1.slope > line_2.slope;
                    return line_1.intercept < line_2.intercept;
                });
        hull.clear();
        for (const Line& line: weight_lines) {
            if (!hull.empty() && hull.back().slope == line.slope)
                continue;
            while (hull.size() >= 2) {
                const Line& l1 = hull[hull.size() - 2];
                const Line& l2 = hull[hull.size() - 1];
                // l2 is useless iff the intersection of l1 and line is left
                // of the intersection of l1 and l2.
                if ((line.intercept - l1.intercept) * (l1.slope - l2.slope)
                        <= (l2.intercept - l1.intercept) * (l1.slope - line.slope)) {
                    hull.pop_back();
                } else {
                    break;
                }
            }
            hull.push_back(line);
        }
        // Query with x = W - t increasing, i.e. t decreasing.
        size_t hull_pos = 0;
        for (Weight start = total_weight - weight; start >= 0; --start) {
            double x = (double)(total_weight - start);
            while (hull_pos + 1 < hull.size()
                    && hull[hull_pos + 1].value(x) <= hull[hull_pos].value(x)) {
                hull_pos++;
            }
            Weight end = start + weight;
            if (!arc_allowed(fixed_state, start, end))
                continue;
            const Line& line = hull[hull_pos];
            double reduced_cost = line.value(x) - flow_dual(start) + flow_dual(end);
            if (reduced_cost < reduced_cost_min)
                reduced_cost_min = reduced_cost;
            if (reduced_cost < -tolerance) {
                candidates.push_back({
                        reduced_cost,
                        start,
                        weight,
                        line.family_pos,
                        line.job_pos});
            }
        }
    }

    // Keep the best candidates.
    int64_t maximum_number_of_columns = (maximum_number_of_columns_ > 0)?
        maximum_number_of_columns_:
        instance_.number_of_jobs() + total_weight;
    auto comp = [](const Candidate& c1, const Candidate& c2)
    {
        return c1.reduced_cost < c2.reduced_cost;
    };
    if ((int64_t)candidates.size() > maximum_number_of_columns) {
        std::nth_element(
                candidates.begin(),
                candidates.begin() + maximum_number_of_columns,
                candidates.end(),
                comp);
        candidates.resize(maximum_number_of_columns);
    }

    PricingOutput output;
    for (const Candidate& candidate: candidates) {
        std::vector<JobId> batch = retrieve_batch(
                candidate.family_pos,
                candidate.job_pos,
                candidate.weight);
        output.columns.push_back(build_column(candidate.start, batch));
    }

    // In an integer solution compatible with the fixed columns, each arc
    // which is not fixed contains at least one free job, so there are at
    // most 'number_of_free_jobs' such arcs.
    output.overcost = fixed_state.number_of_free_jobs * reduced_cost_min;
    return output;
}

/**
 * Initial solution: the jobs of each family are packed into batches with the
 * first fit decreasing processing time rule, and the batches are sequenced
 * by non-decreasing p(B) / w(B) (optimal sequence of the batches).
 */
std::vector<std::vector<JobId>> initial_batches(
        const Instance& instance)
{
    std::vector<JobId> jobs(instance.number_of_jobs());
    std::iota(jobs.begin(), jobs.end(), 0);
    std::sort(
            jobs.begin(),
            jobs.end(),
            [&instance](JobId job_id_1, JobId job_id_2)
            {
                Time p1 = instance.job(job_id_1).processing_times[0];
                Time p2 = instance.job(job_id_2).processing_times[0];
                if (p1 != p2)
                    return p1 > p2;
                return job_id_1 < job_id_2;
            });
    Size capacity = instance.machine(0).capacity;
    std::vector<std::vector<JobId>> batches;
    std::vector<Size> sizes;
    std::vector<FamilyId> families;
    for (JobId job_id: jobs) {
        const Job& job = instance.job(job_id);
        bool added = false;
        for (size_t batch_pos = 0; batch_pos < batches.size(); ++batch_pos) {
            if (families[batch_pos] == job.family_id
                    && sizes[batch_pos] + job.size <= capacity) {
                batches[batch_pos].push_back(job_id);
                sizes[batch_pos] += job.size;
                added = true;
                break;
            }
        }
        if (!added) {
            batches.push_back({job_id});
            sizes.push_back(job.size);
            families.push_back(job.family_id);
        }
    }
    auto ratio = [&instance](const std::vector<JobId>& batch)
    {
        Time processing_time = 0;
        Time weight = 0;
        for (JobId job_id: batch) {
            processing_time = (std::max)(processing_time, instance.job(job_id).processing_times[0]);
            weight += instance.job(job_id).weight;
        }
        return (double)processing_time / weight;
    };
    std::stable_sort(
            batches.begin(),
            batches.end(),
            [&ratio](const std::vector<JobId>& batch_1, const std::vector<JobId>& batch_2)
            {
                return ratio(batch_1) < ratio(batch_2);
            });
    return batches;
}

Solution build_solution(
        const Instance& instance,
        const std::vector<std::vector<JobId>>& batches)
{
    SolutionBuilder solution_builder;
    solution_builder.set_instance(instance);
    Time current_time = 0;
    for (const std::vector<JobId>& batch: batches) {
        solution_builder.append_batch(0, current_time);
        Time processing_time = 0;
        for (JobId job_id: batch) {
            solution_builder.add_job_to_last_batch(0, job_id);
            processing_time = (std::max)(processing_time, instance.job(job_id).processing_times[0]);
        }
        current_time += processing_time;
    }
    return solution_builder.build();
}

/** Convert a solution of the master problem into a solution. */
Solution convert_solution(
        const Instance& instance,
        Weight total_weight,
        const cgs::Solution& cgs_solution)
{
    std::vector<const ArcInfo*> arcs;
    for (const auto& p: cgs_solution.columns()) {
        if (p.second < 0.5)
            continue;
        if (std::abs(p.second - 1.0) > 1e-6) {
            throw std::logic_error(
                    "batchschedulingsolver::column_generation: "
                    "non-integer column value.");
        }
        arcs.push_back(&arc_info(*p.first));
    }
    std::sort(
            arcs.begin(),
            arcs.end(),
            [](const ArcInfo* arc_1, const ArcInfo* arc_2)
            {
                return arc_1->start < arc_2->start;
            });
    std::vector<std::vector<JobId>> batches;
    Weight node = 0;
    for (const ArcInfo* arc: arcs) {
        if (arc->start != node) {
            throw std::logic_error(
                    "batchschedulingsolver::column_generation: "
                    "the arcs of the solution don't form a path.");
        }
        for (JobId job_id: arc->jobs)
            node += instance.job(job_id).weight;
        batches.push_back(arc->jobs);
    }
    if (node != total_weight) {
        throw std::logic_error(
                "batchschedulingsolver::column_generation: "
                "the path of the solution doesn't end at node W.");
    }
    Solution solution = build_solution(instance, batches);
    if (!solution.feasible()
            || std::abs((double)solution.total_flow_time() - cgs_solution.objective_value()) > 0.5) {
        throw std::logic_error(
                "batchschedulingsolver::column_generation: "
                "wrong solution; "
                "master value: " + std::to_string(cgs_solution.objective_value()) + "; "
                "solution value: " + std::to_string(solution.total_flow_time()) + ".");
    }
    return solution;
}

}

const ColumnGenerationOutput batchschedulingsolver::column_generation(
        const Instance& instance,
        const ColumnGenerationParameters& parameters)
{
    ColumnGenerationOutput output(instance);
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.start("Column generation");

    // Check the instance.
    if (instance.objective() != Objective::TotalFlowTime) {
        throw std::invalid_argument(
                "batchschedulingsolver::column_generation: "
                "the objective must be the total flow time.");
    }
    if (instance.number_of_machines() != 1) {
        throw std::invalid_argument(
                "batchschedulingsolver::column_generation: "
                "the instance must have a single machine.");
    }
    if (instance.has_release_dates()) {
        throw std::invalid_argument(
                "batchschedulingsolver::column_generation: "
                "the jobs must not have release dates.");
    }
    Weight total_weight = 0;
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        const Job& job = instance.job(job_id);
        if (job.weight < 1) {
            throw std::invalid_argument(
                    "batchschedulingsolver::column_generation: "
                    "the weights must be positive.");
        }
        if (job.size > instance.machine(0).capacity) {
            throw std::invalid_argument(
                    "batchschedulingsolver::column_generation: "
                    "a job is larger than the capacity of the machine.");
        }
        total_weight += job.weight;
    }

    algorithm_formatter.print_header();

    if (instance.number_of_jobs() == 0) {
        algorithm_formatter.update_solution(build_solution(instance, {}), "");
        algorithm_formatter.update_total_flow_time_bound(0, "");
        algorithm_formatter.end();
        return output;
    }

    // Initial solution.
    std::vector<std::vector<JobId>> batches = initial_batches(instance);
    algorithm_formatter.update_solution(build_solution(instance, batches), "initial solution");

    // Model.
    cgs::Model model;
    model.objective_sense = optimizationtools::ObjectiveDirection::Minimize;
    for (Weight t = 0; t < total_weight; ++t) {
        cgs::Row row;
        row.lower_bound = (t == 0)? 1: 0;
        row.upper_bound = (t == 0)? 1: 0;
        row.coefficient_lower_bound = -1;
        row.coefficient_upper_bound = 1;
        model.rows.push_back(row);
    }
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        cgs::Row row;
        row.lower_bound = 1;
        row.upper_bound = 1;
        row.coefficient_lower_bound = 0;
        row.coefficient_upper_bound = 1;
        model.rows.push_back(row);
    }
    PricingSolver* pricing_solver = new PricingSolver(
            instance,
            total_weight,
            parameters.maximum_number_of_columns_per_pricing,
            parameters.source_first_branching);
    model.pricing_solver = std::unique_ptr<cgs::PricingSolver>(pricing_solver);

    // Initial columns: the arcs of the initial solution.
    std::vector<std::shared_ptr<const cgs::Column>> initial_columns;
    Weight start = 0;
    for (const std::vector<JobId>& batch: batches) {
        initial_columns.push_back(pricing_solver->build_column(start, batch));
        for (JobId job_id: batch)
            start += instance.job(job_id).weight;
    }

    // Limited discrepancy search.
    cgs::LimitedDiscrepancySearchParameters lds_parameters;
    lds_parameters.verbosity_level = 0;
    lds_parameters.timer = parameters.timer;
    lds_parameters.discrepancy_limit = parameters.discrepancy_limit;
    lds_parameters.initial_columns = initial_columns;
    switch (parameters.solver) {
    case mathoptsolverscmake::SolverName::Highs:
        lds_parameters.column_generation_parameters.solver_name = cgs::SolverName::Highs;
        break;
    case mathoptsolverscmake::SolverName::Xpress:
        lds_parameters.column_generation_parameters.solver_name = cgs::SolverName::Xpress;
        break;
    default:
        lds_parameters.column_generation_parameters.solver_name = cgs::SolverName::CLP;
        break;
    }
    lds_parameters.new_solution_callback = [&instance, &algorithm_formatter, total_weight](
            const cgs::Output& cgs_output)
    {
        if (!cgs_output.solution.feasible())
            return;
        Solution solution = convert_solution(instance, total_weight, cgs_output.solution);
        algorithm_formatter.update_solution(solution, "");
    };
    lds_parameters.new_bound_callback = [&algorithm_formatter](
            const cgs::Output& cgs_output)
    {
        if (std::isinf(cgs_output.bound))
            return;
        algorithm_formatter.update_total_flow_time_bound(
                (Time)std::ceil(cgs_output.bound - 1e-6),
                "column generation");
    };
    auto lds_output = cgs::limited_discrepancy_search(model, lds_parameters);

    output.root_relaxation_value = lds_output.root_relaxation_solution.objective_value();
    output.number_of_nodes = lds_output.number_of_nodes;
    output.number_of_column_generation_iterations = lds_output.number_of_column_generation_iterations;
    output.number_of_columns = lds_output.columns.size();
    output.time_lpsolve = lds_output.time_lpsolve;
    output.time_pricing = lds_output.time_pricing;

    algorithm_formatter.end();
    return output;
}
