"""Brute force for batch scheduling instances in JSON format.

Enumerate all the ordered partitions of the jobs into batches that respect
the capacity, and schedule each sequence of batches by list scheduling: each
batch starts on the machine available first, as soon as this machine is
available and all the jobs of the batch are released.

On a single machine and on identical parallel machines (machine-independent
processing times, identical capacities), an optimal schedule is obtained this
way: sort the batches of any schedule by start time, and list scheduling
starts each of them no later than in the original schedule.

The objective values are computed as in SolutionBuilder::build:
- makespan: largest completion time;
- total flow time: sum of w_j (C_j - r_j);
- total tardiness: sum of w_j (C_j - d_j) over the jobs with C_j > d_j.

The search is a depth first search with a simple bound: the objective of a
partial schedule never decreases when batches are appended.
"""

import json
import sys


OBJECTIVES = {
        "makespan": "makespan",
        "Makespan": "makespan",
        "total-flow-time": "total-flow-time",
        "tft": "total-flow-time",
        "TFT": "total-flow-time",
        "total flow time": "total-flow-time",
        "Total flow time": "total-flow-time",
        "total-tardiness": "total-tardiness",
        "tt": "total-tardiness",
        "TT": "total-tardiness",
        "total tardiness": "total-tardiness",
        "Total tardiness": "total-tardiness",
}


def read_instance(instance_path):
    with open(instance_path, "r") as file:
        data = json.load(file)
    objective = OBJECTIVES.get(data.get("objective", "Makespan"))
    if objective is None:
        raise ValueError(f"unsupported objective: {data.get('objective')}")
    capacities = [machine["capacity"] for machine in data["machines"]]
    jobs = []
    for job in data["jobs"]:
        processing_times = job["processing_times"]
        jobs.append({
            "p": processing_times,
            "s": job.get("size", 1),
            "r": job.get("release_date", 0),
            "d": job.get("due_date", -1),
            "w": job.get("weight", 1),
        })
    return objective, capacities, jobs


def supported(capacities, jobs):
    """Return True iff the brute force is exact for this instance."""
    if len(set(capacities)) != 1:
        return False
    for job in jobs:
        if len(set(job["p"])) != 1:
            return False
    return True


def solve(objective, capacities, jobs):
    """Return the optimal value, or None if there is no feasible schedule."""
    n = len(jobs)
    m = len(capacities)
    capacity = capacities[0]
    if any(job["s"] > capacity for job in jobs):
        return None
    if n == 0:
        return 0

    best = [None]

    def job_cost(j, completion_time):
        job = jobs[j]
        if objective == "total-flow-time":
            return job["w"] * (completion_time - job["r"])
        if objective == "total-tardiness":
            if completion_time > job["d"]:
                return job["w"] * (completion_time - job["d"])
            return 0
        return 0

    def batches(remaining):
        """Enumerate the subsets of 'remaining' (a sorted list) that fit."""
        result = []

        def rec(pos, batch, size):
            if batch:
                result.append(list(batch))
            for i in range(pos, len(remaining)):
                j = remaining[i]
                if size + jobs[j]["s"] <= capacity:
                    batch.append(j)
                    rec(i + 1, batch, size + jobs[j]["s"])
                    batch.pop()

        rec(0, [], 0)
        return result

    def dfs(remaining, machine_times, cost):
        if best[0] is not None and cost >= best[0]:
            return
        if not remaining:
            best[0] = cost
            return
        machine = min(range(m), key=lambda i: machine_times[i])
        for batch in batches(remaining):
            start = max(
                    [machine_times[machine]]
                    + [jobs[j]["r"] for j in batch])
            end = start + max(jobs[j]["p"][0] for j in batch)
            new_machine_times = list(machine_times)
            new_machine_times[machine] = end
            if objective == "makespan":
                new_cost = max(cost, end)
            else:
                new_cost = cost + sum(job_cost(j, end) for j in batch)
            new_remaining = [j for j in remaining if j not in batch]
            dfs(new_remaining, new_machine_times, new_cost)

    dfs(list(range(n)), [0] * m, 0)
    return best[0]


if __name__ == "__main__":
    objective, capacities, jobs = read_instance(sys.argv[1])
    if not supported(capacities, jobs):
        print("unsupported", file=sys.stderr)
        sys.exit(1)
    print(solve(objective, capacities, jobs))
