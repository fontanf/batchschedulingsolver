# Brute force

`--algorithm brute-force`

Source: [`include/batchschedulingsolver/algorithms/brute_force.hpp`](../include/batchschedulingsolver/algorithms/brute_force.hpp), [`src/algorithms/brute_force.cpp`](../src/algorithms/brute_force.cpp)

The brute force enumerates all the schedules. It is exponential, so it only solves small instances, but it is kept as simple as possible, so that its correctness can be checked by reading it: it computes the reference solutions of the test instances (see [`tests.md`](tests.md)) and serves as a reference when developing the other algorithms.

## Algorithm

All the objectives are regular (non-decreasing in the completion times of the jobs), so there is an optimal schedule that is semi-active: each batch starts at the maximum of the end of the previous batch on its machine and of the release dates of its jobs.

The algorithm enumerates the semi-active schedules recursively: at each step, it chooses a machine $i$ and a non-empty subset $S$ of the remaining jobs that fits in the machine ($\sum_{j \in S} \text{size}\_j \le B_i$, and all the jobs of $S$ have the same `family_id`, $-1$ being a family like the others), and appends the batch $S$ to machine $i$, starting at
$$\max\left(a_i, \max_{j \in S} r_j\right),$$
where $a_i$ is the end of the last batch of machine $i$, with processing time $\max_{j \in S} p_{ij}$.

Each schedule is enumerated in the order of its batches sorted by start, then by machine: a batch is only appended if its start is greater than the start of the previous batch, or equal to it with a machine index greater than or equal to the machine of the previous batch. Every semi-active schedule is enumerated: appending its batches in this order gives each batch its start in the schedule, since the previous batch of its machine has already been appended.

A branch is cut when the cost of the partial schedule is greater than or equal to the cost of the best schedule found: the cost of a partial schedule (for $\sum w_j F_j$ and $\sum w_j T_j$, the sum over its jobs; for $C_{\max}$ and $L_{\max}$, the maximum) can only increase when batches are appended, since the weights are non-negative.

Complexity: exponential. The number of sequences of batches of $n$ jobs is the ordered Bell number of $n$ (545,835 for 8 jobs, $1.0 \cdot 10^8$ for 10 jobs), multiplied by the choices of machines.

## Supported problems

$1$, $P_m$, $R_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j, \text{incompatible families} \mid C_{\max}$, $\sum w_j F_j$, $\sum w_j T_j$, $L_{\max}$

`SolutionBuilder` accepts more batches than the brute force when some jobs have the family $-1$ and others don't: it takes the family of the first job with a family, so that, for example, a batch with the families $(-1, 0)$ is accepted, and a batch with the families $(0, -1)$ isn't. The brute force only builds batches of jobs with the same `family_id`, which are accepted by `SolutionBuilder`. The test instances have no families.

The objective values are those computed by `SolutionBuilder`: for $L_{\max}$, only the late jobs count, so the value is the maximum tardiness.

The algorithm throws `std::invalid_argument` for the throughput objective, with more than 64 jobs, or with negative weights.

## Optimality

Exact. If the enumeration completes, the best schedule is optimal: the algorithm sets the bound to its value, and the output reports `"Optimal": true`. Otherwise (time limit), the output contains the best schedule found, and no bound. If a job doesn't fit in any machine, there is no feasible schedule and the solution of the output is infeasible.

## Verification

* Against the brute force of an earlier version of this repository, written in Python, on 3,000 random instances with up to 7 jobs and 3 identical machines, generated to hit the special cases (ties between processing times, idle time due to release dates, capacities from 1 to 10, the three objectives).
* With families: against the same Python brute force, extended with the family constraint, on 4,000 random instances where each job has the family $-1$, $0$ or $1$.
* Against the three-index MILP (`milp-rank-based-three-index`), on 400 random instances with up to 6 jobs and 3 unrelated machines with different capacities.
* Against the certificates of the test instances computed by the MILPs: the 228 certificates of instances solved by the brute force within 1 s have the same values.

## Tests

* `test/algorithms/brute_force_test.cpp` with the instances of `test/algorithms/brute_force_test.txt`: the test instances solved to optimality by the brute force in 1 s (1,085 instances: all the instances with up to 8 jobs, 176 of the 306 instances with 9 or 10 jobs and 1 with 12 jobs, in all the environments and for all the objectives except the throughput). The test checks the value against the certificate, the proof of optimality and the feasibility of the solution.
