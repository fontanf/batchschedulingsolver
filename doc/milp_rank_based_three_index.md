# Rank-based MILP, three-index

`--algorithm milp-rank-based-three-index-no-starts`
`--algorithm milp-rank-based-three-index`

Source: [`include/batchschedulingsolver/algorithms/milp_rank_based.hpp`](../include/batchschedulingsolver/algorithms/milp_rank_based.hpp), [`src/algorithms/milp_rank_based.cpp`](../src/algorithms/milp_rank_based.cpp)

Requires an MILP solver: build with `-DBATCHSCHEDULINGSOLVER_USE_HIGHS=ON` (default solver, `--solver highs`), `-DBATCHSCHEDULINGSOLVER_USE_CBC=ON` (`--solver cbc`) or `-DBATCHSCHEDULINGSOLVER_USE_XPRESS=ON` (`--solver xpress`).

## Supported problems

| Variant | Problems |
| --- | --- |
| `milp-rank-based-three-index-no-starts` | $1$, $P_m$, $R_m \mid \text{p-batch}, B_i, \text{size}\_j \mid C_{\max}$ |
| `milp-rank-based-three-index` | $1$, $P_m$, $R_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j \mid C_{\max}$ |
| | $1$, $P_m$, $R_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j \mid \sum w_j F_j$ |
| | $1$, $P_m$, $R_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j \mid \sum w_j T_j$ |

Machines may have different capacities $B_i$ and machine-dependent processing times $p_{ij}$. Families are ignored. The throughput and maximum lateness objectives are not supported.

The algorithms throw `std::invalid_argument` on the instances they don't support: `milp-rank-based-three-index-no-starts` if the objective is not the makespan or if a job has a non-zero release date, `milp-rank-based-three-index` if the objective is not the makespan, the total flow time or the total tardiness.

## Principle

The models are the three-index formulations of Trindade et al. (2018): (MILP2) of Chang, Damodaran and Melouk (2004) for $P_m \mid s_j, B \mid C_{\max}$ and (MILP4) of Velez-Gallego (2009) for $P_m \mid r_j, s_j, B \mid C_{\max}$, generalized to unrelated machines, machine-dependent capacities and to the total weighted flow time and total weighted tardiness objectives.

Each machine has $n$ batch slots $k = 1, \ldots, n$ ("ranks"), and a binary variable $x_{jik}$ assigns job $j$ to slot $k$ of machine $i$. The processing time of a slot is the largest processing time of its jobs. In the model with starts, the slots of a machine are processed in increasing order of $k$, and each slot starts after the previous one completes and after the release dates of its jobs.

The model has $n^2 m$ binary variables. It doesn't break symmetries: any permutation of the slots of a machine (without release dates, or among slots with the same jobs), any relabelling of the empty slots and, on identical machines, any permutation of the machines, give the same schedule. See the [two-index models](milp_rank_based_two_index.md) for the symmetry-broken versions.

## Model without batch starts

`milp-rank-based-three-index-no-starts`

**Input**:

* machines $i = 1, \ldots, m$ with capacity $B_i$; jobs $j = 1, \ldots, n$ with size $s_j$ and processing time $p_{ij}$ on machine $i$
* batch slots $k = 1, \ldots, n$ on each machine

**Variables**:

* $x_{jik} \in \{0, 1\}$: $x_{jik} = 1$ iff job $j$ is in slot $k$ of machine $i$
* $P_{ik} \ge 0$: processing time of slot $k$ of machine $i$
* $C_{\max} \ge 0$: makespan

**Objective**:

```math
\min C_{\max}
```

**Constraints** (numbers of Trindade et al., 2018, in the code comments):

* (15) Each job is in one slot

```math
\forall j \qquad \sum_{i, k} x_{jik} = 1
```

* (16) Capacity of each slot

```math
\forall i, k \qquad \sum_j s_j \, x_{jik} \le B_i
```

* (17) Processing time of each slot

```math
\forall j, i, k \qquad P_{ik} \ge p_{ij} \, x_{jik}
```

* (18) Makespan: without release dates, the machines are never idle and their completion time is the sum of the processing times of their slots

```math
\forall i \qquad C_{\max} \ge \sum_k P_{ik}
```

The solution is built by processing the non-empty slots of each machine in increasing order of $k$, without idle time.

## Model with batch starts

`milp-rank-based-three-index`

**Additional input**: release dates $r_j$, weights $w_j$, due dates $d_j$; $M = \max_j r_j + \sum_j \max_i p_{ij}$, an upper bound on the completion time of a slot in a semi-active schedule.

**Additional variables**:

* $S_{ik} \ge 0$: start time of slot $k$ of machine $i$
* $C_j \ge 0$: completion time of job $j$ (total weighted flow time and total weighted tardiness only)
* $T_j \ge 0$: tardiness of job $j$ (total weighted tardiness only)
* $C_{\max} \ge 0$ (makespan only)

**Objective**: depending on the objective of the instance

```math
\min C_{\max}
\qquad \text{(makespan)}
```

```math
\min \sum_j w_j \, C_j
\qquad \text{(total weighted flow time } \textstyle\sum_j w_j (C_j - r_j) \text{, up to the constant } \textstyle\sum_j w_j r_j \text{)}
```

```math
\min \sum_j w_j \, T_j
\qquad \text{(total weighted tardiness)}
```

**Constraints**: (15), (31) = (16), (17), and

* (32) A slot starts after the release dates of its jobs

```math
\forall j, i, k \qquad S_{ik} \ge r_j \, x_{jik}
```

* (33) The slots of a machine are processed in order

```math
\forall i, \; \forall k > 1 \qquad S_{ik} \ge S_{i,k-1} + P_{i,k-1}
```

* (35) Makespan, for the makespan objective

```math
\forall i \qquad C_{\max} \ge S_{in} + P_{in}
```

* Completion times, for the total weighted flow time and total weighted tardiness objectives

```math
\forall j, i, k \qquad C_j \ge S_{ik} + P_{ik} - M \, (1 - x_{jik})
```

* Tardiness, for the total weighted tardiness objective

```math
\forall j \qquad T_j \ge C_j - d_j
```

The solution is built by taking, for each non-empty slot, its start time $S_{ik}$ from the MILP solution.

## Solver

The model is solved with the default options of the solver and the time limit of the parameters. The solutions found by the solver are reported as they are found (when they improve the current one), and the bound of the solver, rounded up, is reported as a bound for the objective of the instance. For the total weighted flow time, the constant $\sum_j w_j r_j$ is subtracted from the objective value and the bound of the MILP. HiGHS writes its log to `highs.log` in the working directory.

## Optimality

Exact for the makespan, the total weighted flow time and the total weighted tardiness: when the solver terminates, the bound equals the value of the solution and the output reports `"Optimal": true`. Otherwise (time limit), the output contains the best solution and the best bound found. The solver stops with its default relative gap (`1e-4` for HiGHS), so on instances with large objective values, the bound may be slightly below the value of the solution.

## Limits

* The model has $n^2 m$ binary variables and $O(n^2 m)$ constraints, and its linear relaxation is weak (the $P_{ik} \ge p_{ij} x_{jik}$ constraints and, for the sum objectives, the big-M constraints). It is only suited to small instances.
* No symmetry breaking (see [Principle](#principle)).

## Tests

`test/algorithms/milp_rank_based_test.cpp` (built when HiGHS is enabled) compares the value of the solution to the reference value on:

* `milp-rank-based-three-index-no-starts`: [`test/algorithms/milp_rank_based_three_index_no_starts_test.txt`](../test/algorithms/milp_rank_based_three_index_no_starts_test.txt), makespan on single, parallel and unrelated machines without release dates (106 instances).
* `milp-rank-based-three-index`: [`test/algorithms/milp_rank_based_three_index_test.txt`](../test/algorithms/milp_rank_based_three_index_test.txt), makespan on single, parallel and unrelated machines, with and without release dates (160 instances).

The README command for `milp-rank-based-three-index` lists the total (weighted) flow time and total (weighted) tardiness instance lists, but the current reference file only contains makespan instances.

## References

* R. S. Trindade, O. C. B. de Araújo, M. H. C. Fampa and F. M. Müller (2018). Modelling and symmetry breaking in scheduling problems on batch processing machines. *International Journal of Production Research*, 56(22), 7031–7048. [doi:10.1080/00207543.2018.1424371](https://doi.org/10.1080/00207543.2018.1424371)
* P.-Y. Chang, P. Damodaran and S. Melouk (2004). Minimizing makespan on parallel batch processing machines. *International Journal of Production Research*, 42(19), 4211–4220. (MILP2)
* M. C. Velez-Gallego (2009). Algorithms for scheduling parallel batch processing machines with non-identical job ready times. PhD thesis, Florida International University. (MILP4)
