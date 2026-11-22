#include "batchschedulingsolver/algorithms/local_search.hpp"

#include "batchschedulingsolver/algorithm_formatter.hpp"
#include "batchschedulingsolver/solution_builder.hpp"

#include "localsearchsolver/population.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <sstream>

using namespace batchschedulingsolver;

namespace
{

constexpr Time INFINITE_COST = std::numeric_limits<Time>::max();

/**
 * Values of a set of jobs used to compute the cost of a batch in constant
 * time (except for the total tardiness when the batch completes between the
 * smallest and the largest due date of its jobs).
 */
struct BatchAggregates
{
    /** Total weight. */
    Time weight = 0;

    /** Sum of the weighted release dates. */
    Time weighted_release_dates = 0;

    /** Sum of the weighted due dates. */
    Time weighted_due_dates = 0;

    /** Smallest due date. */
    Time smallest_due_date = std::numeric_limits<Time>::max();

    /** Largest due date. */
    Time largest_due_date = std::numeric_limits<Time>::min();
};

/** Batch of a local search solution. */
struct LocalSearchBatch
{
    /** Jobs of the batch. */
    std::vector<JobId> jobs;

    /** Total size of the jobs. */
    Size size = 0;

    /** Largest release date of the jobs. */
    Time release_date = 0;

    /** Family of the jobs. */
    FamilyId family_id = -1;

    /** Aggregates of the jobs. */
    BatchAggregates aggregates;
};

/** Local search solution: the sequence of batches of each machine. */
struct LocalSearchSolution
{
    /** Batches of each machine, in processing order. */
    std::vector<std::vector<LocalSearchBatch>> machines;

    /** Cost. */
    Time cost = 0;

    /**
     * For each job, a key of its batch (the xor of the keys of the jobs of the
     * batch), used to compute the distance between two solutions.
     */
    std::vector<uint64_t> job_batch_keys;
};

/**
 * Batch of a machine sequence being evaluated: the batch is not stored in
 * the solution, so it is described by its jobs, its release date and its
 * processing time on the machine.
 */
struct SegmentBatch
{
    const std::vector<JobId>* jobs;

    BatchAggregates aggregates;

    Time release_date;

    Time processing_time;
};

/** Values of the batches of a machine of the current solution. */
struct MachineCache
{
    /** Processing time of each batch on the machine. */
    std::vector<Time> processing_times;

    /** Completion time of each batch. */
    std::vector<Time> completion_times;

    /** Combined cost of the batches 0, ..., b. */
    std::vector<Time> prefix_costs;

    /** Combined cost of the batches b, ..., end (one more element, 0). */
    std::vector<Time> suffix_costs;

    /** Cost of the machine. */
    Time cost = 0;
};

enum class Neighborhood
{
    JobInsertion,
    JobSwap,
    BatchInsertion,
    BatchSwap,
    BatchMerge,
};

enum class MoveType
{
    /** Move the job (i1, b1, p1) into the batch (i2, b2). */
    JobInsertion,
    /** Move the job (i1, b1, p1) into a new batch at position b2 of i2. */
    JobInsertionNewBatch,
    /** Swap the jobs (i1, b1, p1) and (i2, b2, p2). */
    JobSwap,
    /** Move the batch (i1, b1) to position b2 of i2. */
    BatchInsertion,
    /** Swap the batches (i1, b1) and (i2, b2). */
    BatchSwap,
    /**
     * Merge the batches (i1, b1) and (i2, b2), at the position of (i1, b1),
     * or of (i2, b2) if 'second' is true.
     */
    BatchMerge,
};

/**
 * Cost of a solution, compared lexicographically.
 *
 * For the objectives which are a sum (total flow time, total tardiness), the
 * primary cost is the cost of the solution and the secondary cost is 0. For
 * the objectives which are a maximum over the machines (makespan, maximum
 * lateness), the secondary cost is the sum of the costs of the machines: only
 * the critical machine counts in the primary cost, so the secondary cost
 * guides the search on the plateaus of the primary cost.
 */
struct Cost
{
    Time primary = INFINITE_COST;

    Time secondary = INFINITE_COST;

    bool operator<(const Cost& cost) const
    {
        return primary < cost.primary
            || (primary == cost.primary && secondary < cost.secondary);
    }

    bool operator==(const Cost& cost) const
    {
        return primary == cost.primary && secondary == cost.secondary;
    }
};

struct Move
{
    MoveType type;
    MachineId machine_id_1 = -1;
    BatchId batch_id_1 = -1;
    JobId job_pos_1 = -1;
    MachineId machine_id_2 = -1;
    BatchId batch_id_2 = -1;
    JobId job_pos_2 = -1;
    bool second = false;
    Cost cost;
};

class LocalSearch
{

public:

    LocalSearch(
            const Instance& instance,
            const LocalSearchParameters& parameters,
            std::mt19937_64& generator,
            LocalSearchOutput& output,
            AlgorithmFormatter& algorithm_formatter):
        instance_(instance),
        parameters_(parameters),
        generator_(generator),
        output_(output),
        algorithm_formatter_(algorithm_formatter),
        sum_(instance.objective() == Objective::TotalFlowTime
                || instance.objective() == Objective::TotalTardiness)
    {
        std::uniform_int_distribution<uint64_t> distribution;
        job_keys_.resize(instance.number_of_jobs());
        for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id)
            job_keys_[job_id] = distribution(generator_);
    }

    void run(const Solution* initial_solution);

private:

    /*
     * Costs
     */

    /** Combine the costs of two sets of batches. */
    inline Time combine(Time cost_1, Time cost_2) const
    {
        return (sum_)? cost_1 + cost_2: (std::max)(cost_1, cost_2);
    }

    /** Compute the aggregates of a set of jobs. */
    BatchAggregates aggregates(const std::vector<JobId>& jobs) const
    {
        BatchAggregates aggregates;
        for (JobId job_id: jobs) {
            const Job& job = instance_.job(job_id);
            aggregates.weight += job.weight;
            aggregates.weighted_release_dates += job.weight * job.release_date;
            aggregates.weighted_due_dates += job.weight * job.due_date;
            aggregates.smallest_due_date = (std::min)(aggregates.smallest_due_date, job.due_date);
            aggregates.largest_due_date = (std::max)(aggregates.largest_due_date, job.due_date);
        }
        return aggregates;
    }

    /**
     * Cost of a batch completing at 'completion_time', as computed by
     * SolutionBuilder.
     */
    Time batch_cost(
            const std::vector<JobId>& jobs,
            const BatchAggregates& aggregates,
            Time completion_time) const
    {
        switch (instance_.objective()) {
        case Objective::Makespan:
            return completion_time;
        case Objective::TotalFlowTime:
            return aggregates.weight * completion_time - aggregates.weighted_release_dates;
        case Objective::TotalTardiness: {
            if (completion_time <= aggregates.smallest_due_date)
                return 0;
            if (completion_time >= aggregates.largest_due_date)
                return aggregates.weight * completion_time - aggregates.weighted_due_dates;
            Time cost = 0;
            for (JobId job_id: jobs) {
                const Job& job = instance_.job(job_id);
                if (completion_time > job.due_date)
                    cost += job.weight * (completion_time - job.due_date);
            }
            return cost;
        } case Objective::MaximumLateness:
            // Only the late jobs count.
            return (completion_time > aggregates.smallest_due_date)?
                completion_time - aggregates.smallest_due_date: 0;
        default:
            return 0;
        }
    }

    /** Processing time of a set of jobs on a machine. */
    Time processing_time(
            const std::vector<JobId>& jobs,
            MachineId machine_id) const
    {
        Time p = 0;
        for (JobId job_id: jobs)
            p = (std::max)(p, instance_.job(job_id).processing_times[machine_id]);
        return p;
    }

    /** Largest release date of a set of jobs. */
    Time release_date(const std::vector<JobId>& jobs) const
    {
        Time r = 0;
        for (JobId job_id: jobs)
            r = (std::max)(r, instance_.job(job_id).release_date);
        return r;
    }

    /** Update the size, the release date and the family of a batch. */
    void update_batch(LocalSearchBatch& batch) const
    {
        batch.size = 0;
        batch.release_date = 0;
        for (JobId job_id: batch.jobs) {
            const Job& job = instance_.job(job_id);
            batch.size += job.size;
            batch.release_date = (std::max)(batch.release_date, job.release_date);
        }
        batch.family_id = (batch.jobs.empty())? -1: instance_.job(batch.jobs[0]).family_id;
        batch.aggregates = aggregates(batch.jobs);
    }

    /** Recompute the cache of a machine of the current solution. */
    void compute_machine(MachineId machine_id)
    {
        const std::vector<LocalSearchBatch>& batches = solution_.machines[machine_id];
        MachineCache& cache = caches_[machine_id];
        BatchId number_of_batches = batches.size();
        cache.processing_times.resize(number_of_batches);
        cache.completion_times.resize(number_of_batches);
        cache.prefix_costs.resize(number_of_batches);
        cache.suffix_costs.resize(number_of_batches + 1);
        std::vector<Time> batch_costs(number_of_batches);
        Time completion_time = 0;
        Time cost = 0;
        for (BatchId batch_id = 0; batch_id < number_of_batches; ++batch_id) {
            const LocalSearchBatch& batch = batches[batch_id];
            Time p = processing_time(batch.jobs, machine_id);
            completion_time = (std::max)(completion_time, batch.release_date) + p;
            batch_costs[batch_id] = batch_cost(batch.jobs, batch.aggregates, completion_time);
            cost = combine(cost, batch_costs[batch_id]);
            cache.processing_times[batch_id] = p;
            cache.completion_times[batch_id] = completion_time;
            cache.prefix_costs[batch_id] = cost;
        }
        cache.suffix_costs[number_of_batches] = 0;
        for (BatchId batch_id = number_of_batches - 1; batch_id >= 0; --batch_id) {
            cache.suffix_costs[batch_id] = combine(
                    batch_costs[batch_id],
                    cache.suffix_costs[batch_id + 1]);
        }
        cache.cost = cost;
    }

    /** Add the cost of a machine to the cost of a set of machines. */
    inline Cost add_machine(
            const Cost& cost,
            Time machine_cost) const
    {
        if (sum_)
            return {cost.primary + machine_cost, 0};
        return {(std::max)(cost.primary, machine_cost), cost.secondary + machine_cost};
    }

    /** Cost of the current solution, from the costs of its machines. */
    Cost total_cost() const
    {
        Cost cost = {0, 0};
        for (MachineId machine_id = 0;
                machine_id < instance_.number_of_machines();
                ++machine_id) {
            cost = add_machine(cost, caches_[machine_id].cost);
        }
        return cost;
    }

    /** Recompute the caches and the cost of the current solution. */
    void compute_solution()
    {
        caches_.resize(instance_.number_of_machines());
        for (MachineId machine_id = 0;
                machine_id < instance_.number_of_machines();
                ++machine_id) {
            compute_machine(machine_id);
        }
        current_cost_ = total_cost();
        solution_.cost = current_cost_.primary;
    }

    /** Recompute two machines after a move, and check the cost of the move. */
    void update_after_move(
            MachineId machine_id_1,
            MachineId machine_id_2,
            const Cost& expected_cost)
    {
        compute_machine(machine_id_1);
        if (machine_id_2 != machine_id_1)
            compute_machine(machine_id_2);
        Cost cost = total_cost();
        if (!(cost == expected_cost)) {
            throw std::logic_error(
                    FUNC_SIGNATURE + ": wrong move evaluation; "
                    "expected cost: " + std::to_string(expected_cost.primary)
                    + " " + std::to_string(expected_cost.secondary) + "; "
                    "cost: " + std::to_string(cost.primary)
                    + " " + std::to_string(cost.secondary) + ".");
        }
        current_cost_ = cost;
        solution_.cost = cost.primary;
    }

    /** Cost of the machines other than 'machine_id_1' and 'machine_id_2'. */
    Cost others_cost(
            MachineId machine_id_1,
            MachineId machine_id_2) const
    {
        if (sum_) {
            Time cost = current_cost_.primary - caches_[machine_id_1].cost;
            if (machine_id_2 != machine_id_1)
                cost -= caches_[machine_id_2].cost;
            return {cost, 0};
        }
        Cost cost = {0, 0};
        for (MachineId machine_id = 0;
                machine_id < instance_.number_of_machines();
                ++machine_id) {
            if (machine_id != machine_id_1 && machine_id != machine_id_2)
                cost = add_machine(cost, caches_[machine_id].cost);
        }
        return cost;
    }

    /*
     * Move evaluation
     */

    /** Clear the segment. */
    void segment_clear() { segment_.clear(); }

    /** Append a batch which is not a batch of the machine to the segment. */
    void segment_push(
            const std::vector<JobId>& jobs,
            MachineId machine_id)
    {
        segment_.push_back({&jobs, aggregates(jobs), release_date(jobs), processing_time(jobs, machine_id)});
    }

    /** Append a batch of the solution to the segment. */
    void segment_push(
            const LocalSearchBatch& batch,
            MachineId machine_id)
    {
        segment_.push_back({&batch.jobs, batch.aggregates, batch.release_date, processing_time(batch.jobs, machine_id)});
    }

    /** Append the batches first, ..., last - 1 of a machine to the segment. */
    void segment_push_batches(
            MachineId machine_id,
            BatchId first,
            BatchId last)
    {
        for (BatchId batch_id = first; batch_id < last; ++batch_id) {
            const LocalSearchBatch& batch = solution_.machines[machine_id][batch_id];
            segment_.push_back({
                    &batch.jobs,
                    batch.aggregates,
                    batch.release_date,
                    caches_[machine_id].processing_times[batch_id]});
        }
    }

    /**
     * Return true iff a move whose cost is at least 'cost' can't be better
     * than 'cutoff' (or equal to it if 'allow_equal').
     */
    inline bool reached(
            const Cost& cost,
            const Cost& cutoff,
            bool allow_equal) const
    {
        // For the sums, the secondary costs are 0.
        if (sum_)
            return cost.primary > cutoff.primary || (!allow_equal && cost.primary == cutoff.primary);
        return cutoff < cost || (!allow_equal && cost == cutoff);
    }

    /**
     * Evaluate a machine whose sequence of batches is: its batches before
     * position 'position', the batches of the segment, then its batches from
     * position 'tail_position'.
     *
     * Return the cost of the machine, or INFINITE_COST as soon as the cost of
     * the move, 'base' plus the cost of the machine, reaches 'cutoff'. Since
     * all the costs are non-negative and the primary and secondary costs are
     * non-decreasing when batches are added, a partial cost reaching the
     * cutoff can't lead to a better move.
     */
    Time evaluate_machine(
            MachineId machine_id,
            BatchId position,
            BatchId tail_position,
            const Cost& base,
            const Cost& cutoff,
            bool allow_equal) const
    {
        const std::vector<LocalSearchBatch>& batches = solution_.machines[machine_id];
        const MachineCache& cache = caches_[machine_id];
        Time completion_time = (position > 0)? cache.completion_times[position - 1]: 0;
        Time cost = (position > 0)? cache.prefix_costs[position - 1]: 0;
        if (reached(add_machine(base, cost), cutoff, allow_equal))
            return INFINITE_COST;
        for (const SegmentBatch& segment_batch: segment_) {
            completion_time = (std::max)(completion_time, segment_batch.release_date)
                + segment_batch.processing_time;
            cost = combine(cost, batch_cost(*segment_batch.jobs, segment_batch.aggregates, completion_time));
            if (reached(add_machine(base, cost), cutoff, allow_equal))
                return INFINITE_COST;
        }
        for (BatchId batch_id = tail_position;
                batch_id < (BatchId)batches.size();
                ++batch_id) {
            const LocalSearchBatch& batch = batches[batch_id];
            Time start = (std::max)(completion_time, batch.release_date);
            Time old_start = (std::max)(
                    (batch_id > 0)? cache.completion_times[batch_id - 1]: 0,
                    batch.release_date);
            if (start == old_start) {
                // The rest of the machine is unchanged.
                cost = combine(cost, cache.suffix_costs[batch_id]);
                return (reached(add_machine(base, cost), cutoff, allow_equal))? INFINITE_COST: cost;
            }
            completion_time = start + cache.processing_times[batch_id];
            cost = combine(cost, batch_cost(batch.jobs, batch.aggregates, completion_time));
            if (reached(add_machine(base, cost), cutoff, allow_equal))
                return INFINITE_COST;
        }
        return cost;
    }

    /**
     * Evaluate a machine as 'evaluate_machine', and return the cost of the
     * move: 'base' plus the cost of the machine (infinite if it reaches the
     * cutoff).
     */
    Cost evaluate_one(
            MachineId machine_id,
            BatchId position,
            BatchId tail_position,
            const Cost& base,
            const Cost& cutoff,
            bool allow_equal) const
    {
        Time cost = evaluate_machine(machine_id, position, tail_position, base, cutoff, allow_equal);
        if (cost == INFINITE_COST)
            return Cost();
        return add_machine(base, cost);
    }

    /** Fill 'jobs' with the jobs of 'batch' except the one at position 'job_pos'. */
    static void copy_without(
            const LocalSearchBatch& batch,
            JobId job_pos,
            std::vector<JobId>& jobs)
    {
        jobs.clear();
        for (JobId pos = 0; pos < (JobId)batch.jobs.size(); ++pos)
            if (pos != job_pos)
                jobs.push_back(batch.jobs[pos]);
    }

    /**
     * Evaluate a move modifying the batches b1 of machine i1 and b2 of
     * machine i2 (b1 < b2 if i1 == i2) into 'jobs_1' and 'jobs_2'. An empty
     * batch is removed.
     */
    Cost evaluate_two_batches(
            MachineId machine_id_1,
            BatchId batch_id_1,
            const std::vector<JobId>& jobs_1,
            MachineId machine_id_2,
            BatchId batch_id_2,
            const std::vector<JobId>& jobs_2,
            const Cost& cutoff,
            bool allow_equal)
    {
        Cost base = others_cost(machine_id_1, machine_id_2);
        if (machine_id_1 != machine_id_2) {
            segment_clear();
            if (!jobs_1.empty())
                segment_push(jobs_1, machine_id_1);
            Cost cost = evaluate_one(machine_id_1, batch_id_1, batch_id_1 + 1, base, cutoff, allow_equal);
            if (cost.primary == INFINITE_COST)
                return Cost();
            segment_clear();
            if (!jobs_2.empty())
                segment_push(jobs_2, machine_id_2);
            return evaluate_one(machine_id_2, batch_id_2, batch_id_2 + 1, cost, cutoff, allow_equal);
        }
        segment_clear();
        if (!jobs_1.empty())
            segment_push(jobs_1, machine_id_1);
        segment_push_batches(machine_id_1, batch_id_1 + 1, batch_id_2);
        if (!jobs_2.empty())
            segment_push(jobs_2, machine_id_1);
        return evaluate_one(machine_id_1, batch_id_1, batch_id_2 + 1, base, cutoff, allow_equal);
    }

    /** Return true iff 'move' is better than 'best_move' (ties broken at random). */
    bool is_better(const Move& move, Move& best_move, Counter& number_of_ties)
    {
        if (move.cost < best_move.cost) {
            number_of_ties = 1;
            return true;
        }
        if (move.cost == best_move.cost && move.cost < current_cost_) {
            number_of_ties++;
            return std::uniform_int_distribution<Counter>(1, number_of_ties)(generator_) == 1;
        }
        return false;
    }

    /*
     * Neighborhoods
     */

    bool explore_job_insertion();

    bool explore_job_swap();

    bool explore_batch_insertion();

    bool explore_batch_swap();

    bool explore_batch_merge();

    void apply_move(const Move& move);

    /** Apply the local search to the current solution. */
    void local_search();

    /*
     * Construction and perturbations
     */

    void construct();

    void load(const Solution& solution);

    void perturbation_batch_split();

    bool perturbation_job_swap();

    /*
     * Solutions
     */

    Solution to_solution() const;

    void compute_keys(LocalSearchSolution& solution) const;

    /** Update the output with the current solution if it is better. */
    void update_output(const std::string& s);

    /*
     * Attributes
     */

    const Instance& instance_;

    const LocalSearchParameters& parameters_;

    std::mt19937_64& generator_;

    LocalSearchOutput& output_;

    AlgorithmFormatter& algorithm_formatter_;

    /** True iff the cost is a sum of the costs of the batches, a maximum otherwise. */
    bool sum_;

    /** Random key of each job. */
    std::vector<uint64_t> job_keys_;

    /** Current solution. */
    LocalSearchSolution solution_;

    /** Cost of the current solution. */
    Cost current_cost_;

    /** Cache of each machine of the current solution. */
    std::vector<MachineCache> caches_;

    /** Batches of the machine sequence being evaluated. */
    std::vector<SegmentBatch> segment_;

    /** Temporary job lists of the batches modified by a move. */
    std::vector<JobId> jobs_1_;
    std::vector<JobId> jobs_2_;
    std::vector<JobId> jobs_3_;

};

bool LocalSearch::explore_job_insertion()
{
    Move best_move;
    best_move.cost = current_cost_;
    Counter number_of_ties = 0;
    for (MachineId machine_id_1 = 0;
            machine_id_1 < instance_.number_of_machines();
            ++machine_id_1) {
        const std::vector<LocalSearchBatch>& batches_1 = solution_.machines[machine_id_1];
        for (BatchId batch_id_1 = 0;
                batch_id_1 < (BatchId)batches_1.size();
                ++batch_id_1) {
            const LocalSearchBatch& batch_1 = batches_1[batch_id_1];
            for (JobId job_pos_1 = 0;
                    job_pos_1 < (JobId)batch_1.jobs.size();
                    ++job_pos_1) {
                JobId job_id = batch_1.jobs[job_pos_1];
                const Job& job = instance_.job(job_id);
                copy_without(batch_1, job_pos_1, jobs_1_);

                for (MachineId machine_id_2 = 0;
                        machine_id_2 < instance_.number_of_machines();
                        ++machine_id_2) {
                    const std::vector<LocalSearchBatch>& batches_2 = solution_.machines[machine_id_2];
                    Size capacity = instance_.machine(machine_id_2).capacity;

                    // Insertion into another batch.
                    for (BatchId batch_id_2 = 0;
                            batch_id_2 < (BatchId)batches_2.size();
                            ++batch_id_2) {
                        if (machine_id_2 == machine_id_1 && batch_id_2 == batch_id_1)
                            continue;
                        const LocalSearchBatch& batch_2 = batches_2[batch_id_2];
                        if (batch_2.family_id != job.family_id)
                            continue;
                        if (batch_2.size + job.size > capacity)
                            continue;
                        jobs_2_ = batch_2.jobs;
                        jobs_2_.push_back(job_id);
                        Move move;
                        move.type = MoveType::JobInsertion;
                        move.machine_id_1 = machine_id_1;
                        move.batch_id_1 = batch_id_1;
                        move.job_pos_1 = job_pos_1;
                        move.machine_id_2 = machine_id_2;
                        move.batch_id_2 = batch_id_2;
                        if (machine_id_1 == machine_id_2 && batch_id_2 < batch_id_1) {
                            move.cost = evaluate_two_batches(
                                    machine_id_2, batch_id_2, jobs_2_,
                                    machine_id_1, batch_id_1, jobs_1_,
                                    best_move.cost, number_of_ties > 0);
                        } else {
                            move.cost = evaluate_two_batches(
                                    machine_id_1, batch_id_1, jobs_1_,
                                    machine_id_2, batch_id_2, jobs_2_,
                                    best_move.cost, number_of_ties > 0);
                        }
                        if (is_better(move, best_move, number_of_ties))
                            best_move = move;
                    }

                    // Insertion as a new batch. If the job is alone in its
                    // batch, this is a batch insertion.
                    if (batch_1.jobs.size() == 1)
                        continue;
                    if (job.size > capacity)
                        continue;
                    jobs_2_.assign(1, job_id);
                    Cost base = others_cost(machine_id_1, machine_id_2);
                    for (BatchId position = 0;
                            position <= (BatchId)batches_2.size();
                            ++position) {
                        const Cost cutoff = best_move.cost;
                        bool allow_equal = (number_of_ties > 0);
                        Move move;
                        move.type = MoveType::JobInsertionNewBatch;
                        move.machine_id_1 = machine_id_1;
                        move.batch_id_1 = batch_id_1;
                        move.job_pos_1 = job_pos_1;
                        move.machine_id_2 = machine_id_2;
                        move.batch_id_2 = position;
                        if (machine_id_1 != machine_id_2) {
                            segment_clear();
                            segment_push(jobs_1_, machine_id_1);
                            Cost cost = evaluate_one(machine_id_1, batch_id_1, batch_id_1 + 1, base, cutoff, allow_equal);
                            if (cost.primary != INFINITE_COST) {
                                segment_clear();
                                segment_push(jobs_2_, machine_id_2);
                                cost = evaluate_one(machine_id_2, position, position, cost, cutoff, allow_equal);
                            }
                            move.cost = cost;
                        } else if (position <= batch_id_1) {
                            segment_clear();
                            segment_push(jobs_2_, machine_id_1);
                            segment_push_batches(machine_id_1, position, batch_id_1);
                            segment_push(jobs_1_, machine_id_1);
                            move.cost = evaluate_one(machine_id_1, position, batch_id_1 + 1, base, cutoff, allow_equal);
                        } else {
                            segment_clear();
                            segment_push(jobs_1_, machine_id_1);
                            segment_push_batches(machine_id_1, batch_id_1 + 1, position);
                            segment_push(jobs_2_, machine_id_1);
                            move.cost = evaluate_one(machine_id_1, batch_id_1, position, base, cutoff, allow_equal);
                        }
                        if (is_better(move, best_move, number_of_ties))
                            best_move = move;
                    }
                }
            }
        }
    }
    if (!(best_move.cost < current_cost_))
        return false;
    apply_move(best_move);
    return true;
}

bool LocalSearch::explore_job_swap()
{
    Move best_move;
    best_move.cost = current_cost_;
    Counter number_of_ties = 0;
    for (MachineId machine_id_1 = 0;
            machine_id_1 < instance_.number_of_machines();
            ++machine_id_1) {
        const std::vector<LocalSearchBatch>& batches_1 = solution_.machines[machine_id_1];
        Size capacity_1 = instance_.machine(machine_id_1).capacity;
        for (BatchId batch_id_1 = 0;
                batch_id_1 < (BatchId)batches_1.size();
                ++batch_id_1) {
            const LocalSearchBatch& batch_1 = batches_1[batch_id_1];
            for (MachineId machine_id_2 = machine_id_1;
                    machine_id_2 < instance_.number_of_machines();
                    ++machine_id_2) {
                const std::vector<LocalSearchBatch>& batches_2 = solution_.machines[machine_id_2];
                Size capacity_2 = instance_.machine(machine_id_2).capacity;
                for (BatchId batch_id_2 = (machine_id_2 == machine_id_1)? batch_id_1 + 1: 0;
                        batch_id_2 < (BatchId)batches_2.size();
                        ++batch_id_2) {
                    const LocalSearchBatch& batch_2 = batches_2[batch_id_2];
                    // Jobs of different families can't be swapped, except
                    // two jobs alone in their batches (a batch swap).
                    if (batch_1.family_id != batch_2.family_id)
                        continue;
                    if (batch_1.jobs.size() == 1 && batch_2.jobs.size() == 1)
                        continue;
                    for (JobId job_pos_1 = 0;
                            job_pos_1 < (JobId)batch_1.jobs.size();
                            ++job_pos_1) {
                        JobId job_id_1 = batch_1.jobs[job_pos_1];
                        const Job& job_1 = instance_.job(job_id_1);
                        for (JobId job_pos_2 = 0;
                                job_pos_2 < (JobId)batch_2.jobs.size();
                                ++job_pos_2) {
                            JobId job_id_2 = batch_2.jobs[job_pos_2];
                            const Job& job_2 = instance_.job(job_id_2);
                            if (batch_1.size - job_1.size + job_2.size > capacity_1)
                                continue;
                            if (batch_2.size - job_2.size + job_1.size > capacity_2)
                                continue;
                            jobs_1_ = batch_1.jobs;
                            jobs_1_[job_pos_1] = job_id_2;
                            jobs_2_ = batch_2.jobs;
                            jobs_2_[job_pos_2] = job_id_1;
                            Move move;
                            move.type = MoveType::JobSwap;
                            move.machine_id_1 = machine_id_1;
                            move.batch_id_1 = batch_id_1;
                            move.job_pos_1 = job_pos_1;
                            move.machine_id_2 = machine_id_2;
                            move.batch_id_2 = batch_id_2;
                            move.job_pos_2 = job_pos_2;
                            move.cost = evaluate_two_batches(
                                    machine_id_1, batch_id_1, jobs_1_,
                                    machine_id_2, batch_id_2, jobs_2_,
                                    best_move.cost, number_of_ties > 0);
                            if (is_better(move, best_move, number_of_ties))
                                best_move = move;
                        }
                    }
                }
            }
        }
    }
    if (!(best_move.cost < current_cost_))
        return false;
    apply_move(best_move);
    return true;
}

bool LocalSearch::explore_batch_insertion()
{
    Move best_move;
    best_move.cost = current_cost_;
    Counter number_of_ties = 0;
    for (MachineId machine_id_1 = 0;
            machine_id_1 < instance_.number_of_machines();
            ++machine_id_1) {
        const std::vector<LocalSearchBatch>& batches_1 = solution_.machines[machine_id_1];
        for (BatchId batch_id_1 = 0;
                batch_id_1 < (BatchId)batches_1.size();
                ++batch_id_1) {
            const LocalSearchBatch& batch_1 = batches_1[batch_id_1];
            for (MachineId machine_id_2 = 0;
                    machine_id_2 < instance_.number_of_machines();
                    ++machine_id_2) {
                const std::vector<LocalSearchBatch>& batches_2 = solution_.machines[machine_id_2];
                if (batch_1.size > instance_.machine(machine_id_2).capacity)
                    continue;
                Cost base = others_cost(machine_id_1, machine_id_2);
                // Position of the batch in the new sequence of machine 2.
                BatchId number_of_positions = (machine_id_1 == machine_id_2)?
                    batches_2.size(): batches_2.size() + 1;
                Cost cost_1;
                for (BatchId position = 0;
                        position < number_of_positions;
                        ++position) {
                    const Cost cutoff = best_move.cost;
                    bool allow_equal = (number_of_ties > 0);
                    Move move;
                    move.type = MoveType::BatchInsertion;
                    move.machine_id_1 = machine_id_1;
                    move.batch_id_1 = batch_id_1;
                    move.machine_id_2 = machine_id_2;
                    move.batch_id_2 = position;
                    if (machine_id_1 != machine_id_2) {
                        if (position == 0) {
                            segment_clear();
                            cost_1 = evaluate_one(machine_id_1, batch_id_1, batch_id_1 + 1, base, Cost(), false);
                        }
                        if (reached(cost_1, cutoff, allow_equal))
                            break;
                        segment_clear();
                        segment_push(batch_1, machine_id_2);
                        move.cost = evaluate_one(machine_id_2, position, position, cost_1, cutoff, allow_equal);
                    } else if (position == batch_id_1) {
                        continue;
                    } else if (position < batch_id_1) {
                        segment_clear();
                        segment_push_batches(machine_id_1, batch_id_1, batch_id_1 + 1);
                        segment_push_batches(machine_id_1, position, batch_id_1);
                        move.cost = evaluate_one(machine_id_1, position, batch_id_1 + 1, base, cutoff, allow_equal);
                    } else {
                        segment_clear();
                        segment_push_batches(machine_id_1, batch_id_1 + 1, position + 1);
                        segment_push_batches(machine_id_1, batch_id_1, batch_id_1 + 1);
                        move.cost = evaluate_one(machine_id_1, batch_id_1, position + 1, base, cutoff, allow_equal);
                    }
                    if (is_better(move, best_move, number_of_ties))
                        best_move = move;
                }
            }
        }
    }
    if (!(best_move.cost < current_cost_))
        return false;
    apply_move(best_move);
    return true;
}

bool LocalSearch::explore_batch_swap()
{
    Move best_move;
    best_move.cost = current_cost_;
    Counter number_of_ties = 0;
    for (MachineId machine_id_1 = 0;
            machine_id_1 < instance_.number_of_machines();
            ++machine_id_1) {
        const std::vector<LocalSearchBatch>& batches_1 = solution_.machines[machine_id_1];
        for (BatchId batch_id_1 = 0;
                batch_id_1 < (BatchId)batches_1.size();
                ++batch_id_1) {
            const LocalSearchBatch& batch_1 = batches_1[batch_id_1];
            for (MachineId machine_id_2 = machine_id_1;
                    machine_id_2 < instance_.number_of_machines();
                    ++machine_id_2) {
                const std::vector<LocalSearchBatch>& batches_2 = solution_.machines[machine_id_2];
                if (batch_1.size > instance_.machine(machine_id_2).capacity)
                    continue;
                Cost base = others_cost(machine_id_1, machine_id_2);
                for (BatchId batch_id_2 = (machine_id_2 == machine_id_1)? batch_id_1 + 1: 0;
                        batch_id_2 < (BatchId)batches_2.size();
                        ++batch_id_2) {
                    const LocalSearchBatch& batch_2 = batches_2[batch_id_2];
                    if (batch_2.size > instance_.machine(machine_id_1).capacity)
                        continue;
                    const Cost cutoff = best_move.cost;
                    bool allow_equal = (number_of_ties > 0);
                    Move move;
                    move.type = MoveType::BatchSwap;
                    move.machine_id_1 = machine_id_1;
                    move.batch_id_1 = batch_id_1;
                    move.machine_id_2 = machine_id_2;
                    move.batch_id_2 = batch_id_2;
                    if (machine_id_1 != machine_id_2) {
                        segment_clear();
                        segment_push(batch_2, machine_id_1);
                        Cost cost = evaluate_one(machine_id_1, batch_id_1, batch_id_1 + 1, base, cutoff, allow_equal);
                        if (cost.primary != INFINITE_COST) {
                            segment_clear();
                            segment_push(batch_1, machine_id_2);
                            cost = evaluate_one(machine_id_2, batch_id_2, batch_id_2 + 1, cost, cutoff, allow_equal);
                        }
                        move.cost = cost;
                    } else {
                        segment_clear();
                        segment_push_batches(machine_id_1, batch_id_2, batch_id_2 + 1);
                        segment_push_batches(machine_id_1, batch_id_1 + 1, batch_id_2);
                        segment_push_batches(machine_id_1, batch_id_1, batch_id_1 + 1);
                        move.cost = evaluate_one(machine_id_1, batch_id_1, batch_id_2 + 1, base, cutoff, allow_equal);
                    }
                    if (is_better(move, best_move, number_of_ties))
                        best_move = move;
                }
            }
        }
    }
    if (!(best_move.cost < current_cost_))
        return false;
    apply_move(best_move);
    return true;
}

bool LocalSearch::explore_batch_merge()
{
    Move best_move;
    best_move.cost = current_cost_;
    Counter number_of_ties = 0;
    for (MachineId machine_id_1 = 0;
            machine_id_1 < instance_.number_of_machines();
            ++machine_id_1) {
        const std::vector<LocalSearchBatch>& batches_1 = solution_.machines[machine_id_1];
        for (BatchId batch_id_1 = 0;
                batch_id_1 < (BatchId)batches_1.size();
                ++batch_id_1) {
            const LocalSearchBatch& batch_1 = batches_1[batch_id_1];
            for (MachineId machine_id_2 = machine_id_1;
                    machine_id_2 < instance_.number_of_machines();
                    ++machine_id_2) {
                const std::vector<LocalSearchBatch>& batches_2 = solution_.machines[machine_id_2];
                for (BatchId batch_id_2 = (machine_id_2 == machine_id_1)? batch_id_1 + 1: 0;
                        batch_id_2 < (BatchId)batches_2.size();
                        ++batch_id_2) {
                    const LocalSearchBatch& batch_2 = batches_2[batch_id_2];
                    if (batch_1.family_id != batch_2.family_id)
                        continue;
                    Size size = batch_1.size + batch_2.size;
                    jobs_3_ = batch_1.jobs;
                    jobs_3_.insert(jobs_3_.end(), batch_2.jobs.begin(), batch_2.jobs.end());
                    jobs_1_.clear();
                    for (bool second: {false, true}) {
                        MachineId machine_id = (second)? machine_id_2: machine_id_1;
                        if (size > instance_.machine(machine_id).capacity)
                            continue;
                        Move move;
                        move.type = MoveType::BatchMerge;
                        move.machine_id_1 = machine_id_1;
                        move.batch_id_1 = batch_id_1;
                        move.machine_id_2 = machine_id_2;
                        move.batch_id_2 = batch_id_2;
                        move.second = second;
                        move.cost = (second)?
                            evaluate_two_batches(
                                    machine_id_1, batch_id_1, jobs_1_,
                                    machine_id_2, batch_id_2, jobs_3_,
                                    best_move.cost, number_of_ties > 0):
                            evaluate_two_batches(
                                    machine_id_1, batch_id_1, jobs_3_,
                                    machine_id_2, batch_id_2, jobs_1_,
                                    best_move.cost, number_of_ties > 0);
                        if (is_better(move, best_move, number_of_ties))
                            best_move = move;
                    }
                }
            }
        }
    }
    if (!(best_move.cost < current_cost_))
        return false;
    apply_move(best_move);
    return true;
}

void LocalSearch::apply_move(const Move& move)
{
    std::vector<LocalSearchBatch>& batches_1 = solution_.machines[move.machine_id_1];
    std::vector<LocalSearchBatch>& batches_2 = solution_.machines[move.machine_id_2];
    switch (move.type) {
    case MoveType::JobInsertion: {
        JobId job_id = batches_1[move.batch_id_1].jobs[move.job_pos_1];
        batches_2[move.batch_id_2].jobs.push_back(job_id);
        update_batch(batches_2[move.batch_id_2]);
        LocalSearchBatch& batch_1 = batches_1[move.batch_id_1];
        batch_1.jobs.erase(batch_1.jobs.begin() + move.job_pos_1);
        update_batch(batch_1);
        if (batch_1.jobs.empty())
            batches_1.erase(batches_1.begin() + move.batch_id_1);
        break;
    } case MoveType::JobInsertionNewBatch: {
        LocalSearchBatch& batch_1 = batches_1[move.batch_id_1];
        LocalSearchBatch batch;
        batch.jobs = {batch_1.jobs[move.job_pos_1]};
        update_batch(batch);
        batch_1.jobs.erase(batch_1.jobs.begin() + move.job_pos_1);
        update_batch(batch_1);
        // The batch 1 is not removed, so the positions of the batches of
        // machine 2 are unchanged.
        batches_2.insert(batches_2.begin() + move.batch_id_2, batch);
        break;
    } case MoveType::JobSwap: {
        std::swap(
                batches_1[move.batch_id_1].jobs[move.job_pos_1],
                batches_2[move.batch_id_2].jobs[move.job_pos_2]);
        update_batch(batches_1[move.batch_id_1]);
        update_batch(batches_2[move.batch_id_2]);
        break;
    } case MoveType::BatchInsertion: {
        LocalSearchBatch batch = batches_1[move.batch_id_1];
        batches_1.erase(batches_1.begin() + move.batch_id_1);
        batches_2.insert(batches_2.begin() + move.batch_id_2, batch);
        break;
    } case MoveType::BatchSwap: {
        std::swap(batches_1[move.batch_id_1], batches_2[move.batch_id_2]);
        break;
    } case MoveType::BatchMerge: {
        LocalSearchBatch& batch_1 = batches_1[move.batch_id_1];
        LocalSearchBatch& batch_2 = batches_2[move.batch_id_2];
        if (!move.second) {
            batch_1.jobs.insert(batch_1.jobs.end(), batch_2.jobs.begin(), batch_2.jobs.end());
            update_batch(batch_1);
            batches_2.erase(batches_2.begin() + move.batch_id_2);
        } else {
            batch_2.jobs.insert(batch_2.jobs.end(), batch_1.jobs.begin(), batch_1.jobs.end());
            update_batch(batch_2);
            batches_1.erase(batches_1.begin() + move.batch_id_1);
        }
        break;
    }
    }
    update_after_move(move.machine_id_1, move.machine_id_2, move.cost);
}

void LocalSearch::local_search()
{
    std::vector<Neighborhood> neighborhoods = {
        Neighborhood::JobInsertion,
        Neighborhood::JobSwap,
        Neighborhood::BatchInsertion,
        Neighborhood::BatchSwap,
        Neighborhood::BatchMerge,
    };
    std::shuffle(neighborhoods.begin(), neighborhoods.end(), generator_);
    for (std::size_t neighborhood_pos = 0;
            neighborhood_pos < neighborhoods.size();) {
        if (parameters_.timer.needs_to_end())
            break;
        // A solution of cost 0 is optimal.
        if (solution_.cost == 0)
            break;
        bool improved = false;
        switch (neighborhoods[neighborhood_pos]) {
        case Neighborhood::JobInsertion:
            improved = explore_job_insertion();
            break;
        case Neighborhood::JobSwap:
            improved = explore_job_swap();
            break;
        case Neighborhood::BatchInsertion:
            improved = explore_batch_insertion();
            break;
        case Neighborhood::BatchSwap:
            improved = explore_batch_swap();
            break;
        case Neighborhood::BatchMerge:
            improved = explore_batch_merge();
            break;
        }
        if (improved) {
            std::shuffle(neighborhoods.begin(), neighborhoods.end(), generator_);
            neighborhood_pos = 0;
        } else {
            neighborhood_pos++;
        }
    }
}

void LocalSearch::construct()
{
    // Randomized constructive heuristic of Queiroga et al. (2020), extended to
    // parallel machines: the jobs are sorted by due date (tardiness and
    // lateness) or by release date (makespan and flow time); a job is chosen
    // at random among the first 'alpha' jobs of the list and added to the
    // current batch if it fits, otherwise the smallest of these jobs which
    // fits is added; if none fits, the batch is closed. A batch is opened on
    // the machine available first among the machines where its first job
    // fits.
    const JobId alpha = 4;
    std::vector<JobId> jobs(instance_.number_of_jobs());
    std::iota(jobs.begin(), jobs.end(), 0);
    bool due_dates = (instance_.objective() == Objective::TotalTardiness
            || instance_.objective() == Objective::MaximumLateness);
    std::sort(
            jobs.begin(), jobs.end(),
            [this, due_dates](JobId job_id_1, JobId job_id_2)
            {
                const Job& job_1 = instance_.job(job_id_1);
                const Job& job_2 = instance_.job(job_id_2);
                return (due_dates)?
                    job_1.due_date < job_2.due_date:
                    job_1.release_date < job_2.release_date;
            });

    solution_.machines.assign(instance_.number_of_machines(), {});
    std::vector<Time> machine_times(instance_.number_of_machines(), 0);
    LocalSearchBatch batch;
    MachineId machine_id = -1;
    auto close_batch = [this, &batch, &machine_id, &machine_times]()
    {
        update_batch(batch);
        machine_times[machine_id] = (std::max)(machine_times[machine_id], batch.release_date)
            + processing_time(batch.jobs, machine_id);
        solution_.machines[machine_id].push_back(batch);
        batch.jobs.clear();
        batch.size = 0;
        machine_id = -1;
    };
    auto fits = [this, &batch, &machine_id](JobId job_id)
    {
        const Job& job = instance_.job(job_id);
        return batch.size + job.size <= instance_.machine(machine_id).capacity
            && instance_.job(batch.jobs[0]).family_id == job.family_id;
    };
    while (!jobs.empty()) {
        JobId number_of_candidates = (std::min)(alpha, (JobId)jobs.size());
        JobId pos = std::uniform_int_distribution<JobId>(0, number_of_candidates - 1)(generator_);
        JobId job_id = jobs[pos];
        if (!batch.jobs.empty() && !fits(job_id)) {
            JobId pos_best = -1;
            for (JobId pos_2 = 0; pos_2 < number_of_candidates; ++pos_2) {
                if (pos_2 == pos || !fits(jobs[pos_2]))
                    continue;
                if (pos_best == -1
                        || instance_.job(jobs[pos_2]).size < instance_.job(jobs[pos_best]).size) {
                    pos_best = pos_2;
                }
            }
            if (pos_best != -1) {
                pos = pos_best;
                job_id = jobs[pos];
            } else {
                close_batch();
            }
        }
        if (batch.jobs.empty()) {
            for (MachineId machine_id_2 = 0;
                    machine_id_2 < instance_.number_of_machines();
                    ++machine_id_2) {
                if (instance_.job(job_id).size > instance_.machine(machine_id_2).capacity)
                    continue;
                if (machine_id == -1 || machine_times[machine_id_2] < machine_times[machine_id])
                    machine_id = machine_id_2;
            }
        }
        batch.jobs.push_back(job_id);
        batch.size += instance_.job(job_id).size;
        jobs.erase(jobs.begin() + pos);
    }
    if (!batch.jobs.empty())
        close_batch();
    compute_solution();
}

void LocalSearch::load(const Solution& solution)
{
    solution_.machines.assign(instance_.number_of_machines(), {});
    for (MachineId machine_id = 0;
            machine_id < instance_.number_of_machines();
            ++machine_id) {
        for (const Solution::Batch& solution_batch: solution.machine(machine_id).batches) {
            if (solution_batch.jobs.empty())
                continue;
            LocalSearchBatch batch;
            batch.jobs = solution_batch.jobs;
            update_batch(batch);
            for (JobId job_id: batch.jobs) {
                if (instance_.job(job_id).family_id != batch.family_id) {
                    throw std::invalid_argument(
                            FUNC_SIGNATURE + ": "
                            "the batches of the initial solution must contain "
                            "jobs with the same family.");
                }
            }
            solution_.machines[machine_id].push_back(batch);
        }
    }
    compute_solution();
}

void LocalSearch::perturbation_batch_split()
{
    // Batch split of Medeiros et al. (2024): split a random batch of at least
    // two jobs at a random point, after shuffling its jobs, and insert the two
    // new batches at random positions.
    std::vector<std::pair<MachineId, BatchId>> candidates;
    for (MachineId machine_id = 0;
            machine_id < instance_.number_of_machines();
            ++machine_id) {
        for (BatchId batch_id = 0;
                batch_id < (BatchId)solution_.machines[machine_id].size();
                ++batch_id) {
            if (solution_.machines[machine_id][batch_id].jobs.size() >= 2)
                candidates.push_back({machine_id, batch_id});
        }
    }
    if (candidates.empty())
        return;
    auto candidate = candidates[std::uniform_int_distribution<std::size_t>(0, candidates.size() - 1)(generator_)];
    std::vector<LocalSearchBatch>& batches = solution_.machines[candidate.first];
    std::vector<JobId> jobs = batches[candidate.second].jobs;
    batches.erase(batches.begin() + candidate.second);
    std::shuffle(jobs.begin(), jobs.end(), generator_);
    JobId cut = std::uniform_int_distribution<JobId>(1, jobs.size() - 1)(generator_);
    for (int part = 0; part < 2; ++part) {
        LocalSearchBatch batch;
        batch.jobs = (part == 0)?
            std::vector<JobId>(jobs.begin(), jobs.begin() + cut):
            std::vector<JobId>(jobs.begin() + cut, jobs.end());
        update_batch(batch);
        // Random position among the positions of the machines where the
        // batch fits.
        std::vector<std::pair<MachineId, BatchId>> positions;
        for (MachineId machine_id = 0;
                machine_id < instance_.number_of_machines();
                ++machine_id) {
            if (batch.size > instance_.machine(machine_id).capacity)
                continue;
            for (BatchId position = 0;
                    position <= (BatchId)solution_.machines[machine_id].size();
                    ++position) {
                positions.push_back({machine_id, position});
            }
        }
        auto position = positions[std::uniform_int_distribution<std::size_t>(0, positions.size() - 1)(generator_)];
        std::vector<LocalSearchBatch>& batches_2 = solution_.machines[position.first];
        batches_2.insert(batches_2.begin() + position.second, batch);
    }
    compute_solution();
}

bool LocalSearch::perturbation_job_swap()
{
    // Swap two random jobs of different batches of the same family
    // (Queiroga et al., 2020). Return false if no valid swap was found.
    std::vector<std::pair<MachineId, BatchId>> job_batches(instance_.number_of_jobs());
    std::vector<JobId> job_positions(instance_.number_of_jobs());
    for (MachineId machine_id = 0;
            machine_id < instance_.number_of_machines();
            ++machine_id) {
        for (BatchId batch_id = 0;
                batch_id < (BatchId)solution_.machines[machine_id].size();
                ++batch_id) {
            const LocalSearchBatch& batch = solution_.machines[machine_id][batch_id];
            for (JobId job_pos = 0; job_pos < (JobId)batch.jobs.size(); ++job_pos) {
                job_batches[batch.jobs[job_pos]] = {machine_id, batch_id};
                job_positions[batch.jobs[job_pos]] = job_pos;
            }
        }
    }
    std::uniform_int_distribution<JobId> distribution(0, instance_.number_of_jobs() - 1);
    for (Counter attempt = 0; attempt < 10 * instance_.number_of_jobs(); ++attempt) {
        JobId job_id_1 = distribution(generator_);
        JobId job_id_2 = distribution(generator_);
        auto b1 = job_batches[job_id_1];
        auto b2 = job_batches[job_id_2];
        if (b1 == b2)
            continue;
        const Job& job_1 = instance_.job(job_id_1);
        const Job& job_2 = instance_.job(job_id_2);
        if (job_1.family_id != job_2.family_id)
            continue;
        LocalSearchBatch& batch_1 = solution_.machines[b1.first][b1.second];
        LocalSearchBatch& batch_2 = solution_.machines[b2.first][b2.second];
        if (batch_1.size - job_1.size + job_2.size > instance_.machine(b1.first).capacity)
            continue;
        if (batch_2.size - job_2.size + job_1.size > instance_.machine(b2.first).capacity)
            continue;
        std::swap(batch_1.jobs[job_positions[job_id_1]], batch_2.jobs[job_positions[job_id_2]]);
        update_batch(batch_1);
        update_batch(batch_2);
        compute_solution();
        return true;
    }
    return false;
}

Solution LocalSearch::to_solution() const
{
    SolutionBuilder solution_builder;
    solution_builder.set_instance(instance_);
    for (MachineId machine_id = 0;
            machine_id < instance_.number_of_machines();
            ++machine_id) {
        const std::vector<LocalSearchBatch>& batches = solution_.machines[machine_id];
        const MachineCache& cache = caches_[machine_id];
        for (BatchId batch_id = 0; batch_id < (BatchId)batches.size(); ++batch_id) {
            solution_builder.append_batch(
                    machine_id,
                    cache.completion_times[batch_id] - cache.processing_times[batch_id]);
            for (JobId job_id: batches[batch_id].jobs)
                solution_builder.add_job_to_last_batch(machine_id, job_id);
        }
    }
    Solution solution = solution_builder.build();
    if (!solution.feasible() || solution.objective_value() != solution_.cost) {
        throw std::logic_error(
                FUNC_SIGNATURE + ": wrong solution; "
                "feasible: " + std::to_string(solution.feasible()) + "; "
                "solution: " + std::to_string(solution.objective_value()) + "; "
                "local search solution: " + std::to_string(solution_.cost) + ".");
    }
    return solution;
}

void LocalSearch::compute_keys(LocalSearchSolution& solution) const
{
    solution.job_batch_keys.resize(instance_.number_of_jobs());
    for (const auto& batches: solution.machines) {
        for (const LocalSearchBatch& batch: batches) {
            uint64_t key = 0;
            for (JobId job_id: batch.jobs)
                key ^= job_keys_[job_id];
            for (JobId job_id: batch.jobs)
                solution.job_batch_keys[job_id] = key;
        }
    }
}

void LocalSearch::update_output(const std::string& s)
{
    if (output_.solution.feasible()
            && output_.solution.objective_value() <= solution_.cost) {
        return;
    }
    algorithm_formatter_.update_solution(to_solution(), s);
    if (solution_.cost == 0) {
        // All the costs are non-negative.
        switch (instance_.objective()) {
        case Objective::Makespan:
            algorithm_formatter_.update_makespan_bound(0, "");
            break;
        case Objective::TotalFlowTime:
            algorithm_formatter_.update_total_flow_time_bound(0, "");
            break;
        case Objective::TotalTardiness:
            algorithm_formatter_.update_total_tardiness_bound(0, "");
            break;
        case Objective::MaximumLateness:
            algorithm_formatter_.update_maximum_lateness_bound(0, "");
            break;
        default:
            break;
        }
    }
}

void LocalSearch::run(const Solution* initial_solution)
{
    // Parameters of Medeiros et al. (2024).
    const Counter population_size = 5;
    const Counter generation_size = 10;
    const Counter number_of_elite_solutions = 2;
    const Counter number_of_closest_neighbors = 1;
    const Counter maximum_number_of_iterations_before_diversification = 250;
    const Counter maximum_number_of_perturbation_moves = 3;

    localsearchsolver::PenalizedCostCallback<LocalSearchSolution, Time> penalized_cost_callback =
        [](const LocalSearchSolution& solution) { return solution.cost; };
    localsearchsolver::DistanceCallback<LocalSearchSolution> distance_callback =
        [this](const LocalSearchSolution& solution_1, const LocalSearchSolution& solution_2)
        {
            // Number of jobs whose batch differs.
            localsearchsolver::Distance distance = 0;
            for (JobId job_id = 0; job_id < instance_.number_of_jobs(); ++job_id)
                if (solution_1.job_batch_keys[job_id] != solution_2.job_batch_keys[job_id])
                    distance++;
            return distance;
        };
    localsearchsolver::Population<LocalSearchSolution, Time>::Parameters population_parameters;
    population_parameters.minimum_size = population_size;
    population_parameters.maximum_size = population_size + generation_size;
    population_parameters.number_of_elite_solutions = number_of_elite_solutions;
    population_parameters.number_of_closest_neighbors = number_of_closest_neighbors;
    using Population = localsearchsolver::Population<LocalSearchSolution, Time>;
    Population population(penalized_cost_callback, distance_callback, population_parameters);

    auto add_to_population = [this](Population& population)
    {
        compute_keys(solution_);
        population.add(solution_, generator_);
    };

    // Initial population.
    if (initial_solution != nullptr) {
        load(*initial_solution);
        update_output("initial solution");
        local_search();
        update_output("initial solution, local search");
        add_to_population(population);
    }
    while (population.size() < population_size
            && !parameters_.timer.needs_to_end()) {
        construct();
        update_output("construction");
        local_search();
        update_output("construction, local search");
        add_to_population(population);
    }
    if (population.size() == 0)
        return;

    std::vector<Counter> perturbation_successes(2, 0);
    Counter number_of_iterations_without_improvement = 0;
    Counter number_of_iterations_since_diversification = 0;
    Time best_cost = output_.solution.objective_value();
    for (output_.number_of_iterations = 1;
            !parameters_.timer.needs_to_end();
            ++output_.number_of_iterations) {
        if (output_.solution.feasible()
                && output_.solution.objective_value() <= output_.bound()) {
            break;
        }
        if (parameters_.maximum_number_of_iterations >= 0
                && output_.number_of_iterations > parameters_.maximum_number_of_iterations) {
            break;
        }
        if (parameters_.maximum_number_of_iterations_without_improvement >= 0
                && number_of_iterations_without_improvement
                >= parameters_.maximum_number_of_iterations_without_improvement) {
            break;
        }
        // Perturbations require at least two jobs.
        if (instance_.number_of_jobs() < 2)
            break;

        // Diversification: keep the elite solutions and generate new ones.
        if (number_of_iterations_since_diversification
                >= maximum_number_of_iterations_before_diversification) {
            std::vector<LocalSearchSolution> elite;
            for (Counter solution_id = 0; solution_id < population.size(); ++solution_id)
                elite.push_back(population.solution(solution_id).solution);
            std::sort(
                    elite.begin(), elite.end(),
                    [](const LocalSearchSolution& solution_1, const LocalSearchSolution& solution_2)
                    {
                        return solution_1.cost < solution_2.cost;
                    });
            elite.resize((std::min)((Counter)elite.size(), number_of_elite_solutions));
            population = Population(penalized_cost_callback, distance_callback, population_parameters);
            for (const LocalSearchSolution& solution: elite)
                population.add(solution, generator_);
            while (population.size() < population_size
                    && !parameters_.timer.needs_to_end()) {
                construct();
                local_search();
                update_output("diversification");
                add_to_population(population);
            }
            number_of_iterations_since_diversification = 0;
        }

        solution_ = population.binary_tournament_single(generator_);
        compute_solution();
        Time cost_before = solution_.cost;

        // Perturbation, chosen with weights 2 + number of successes.
        std::discrete_distribution<int> distribution_perturbation({
                2.0 + perturbation_successes[0],
                2.0 + perturbation_successes[1]});
        int perturbation = distribution_perturbation(generator_);
        Counter number_of_moves = std::uniform_int_distribution<Counter>(
                1, maximum_number_of_perturbation_moves)(generator_);
        for (Counter move = 0; move < number_of_moves; ++move) {
            // If no job swap is possible, split a batch.
            if (perturbation == 1 && perturbation_job_swap())
                continue;
            perturbation_batch_split();
        }

        local_search();
        if (solution_.cost < cost_before)
            perturbation_successes[perturbation]++;
        std::stringstream ss;
        ss << "iteration " << output_.number_of_iterations;
        update_output(ss.str());
        add_to_population(population);

        if (output_.solution.objective_value() < best_cost) {
            best_cost = output_.solution.objective_value();
            number_of_iterations_without_improvement = 0;
            number_of_iterations_since_diversification = 0;
        } else {
            number_of_iterations_without_improvement++;
            number_of_iterations_since_diversification++;
        }
    }
}

}

const LocalSearchOutput batchschedulingsolver::local_search(
        const Instance& instance,
        std::mt19937_64& generator,
        const Solution* initial_solution,
        const LocalSearchParameters& parameters)
{
    LocalSearchOutput output(instance);
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.start("Local search");

    // Check the instance.
    if (instance.objective() != Objective::Makespan
            && instance.objective() != Objective::TotalFlowTime
            && instance.objective() != Objective::TotalTardiness
            && instance.objective() != Objective::MaximumLateness) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "only the makespan, total flow time, total tardiness and "
                "maximum lateness objectives are supported.");
    }
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        const Job& job = instance.job(job_id);
        if (job.weight < 0) {
            throw std::invalid_argument(
                    FUNC_SIGNATURE + ": "
                    "negative weights are not supported.");
        }
        bool fits = false;
        for (MachineId machine_id = 0;
                machine_id < instance.number_of_machines();
                ++machine_id) {
            if (job.size <= instance.machine(machine_id).capacity)
                fits = true;
        }
        if (!fits) {
            throw std::invalid_argument(
                    FUNC_SIGNATURE + ": "
                    "job " + std::to_string(job_id) + " doesn't fit in any machine.");
        }
    }
    if (initial_solution != nullptr && !initial_solution->feasible()) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "the initial solution is infeasible.");
    }

    algorithm_formatter.print_header();

    LocalSearch local_search(instance, parameters, generator, output, algorithm_formatter);
    local_search.run(initial_solution);

    algorithm_formatter.end();
    return output;
}
