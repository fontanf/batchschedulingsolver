# Rank-based MILP, two-index

`--algorithm milp-rank-based-two-index-no-starts`
`--algorithm milp-rank-based-two-index`

Source: [`include/batchschedulingsolver/algorithms/milp_rank_based.hpp`](../include/batchschedulingsolver/algorithms/milp_rank_based.hpp), [`src/algorithms/milp_rank_based.cpp`](../src/algorithms/milp_rank_based.cpp)

Requires an MILP solver: build with `-DBATCHSCHEDULINGSOLVER_USE_HIGHS=ON` (default solver, `--solver highs`), `-DBATCHSCHEDULINGSOLVER_USE_CBC=ON` (`--solver cbc`) or `-DBATCHSCHEDULINGSOLVER_USE_XPRESS=ON` (`--solver xpress`).

## Supported problems

| Variant | Problems |
| --- | --- |
| `milp-rank-based-two-index-no-starts` | $1$, $P_m \mid \text{p-batch}, B_i, \text{size}\_j \mid C_{\max}$ |
| `milp-rank-based-two-index` | $1$, $P_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j \mid C_{\max}$ |

Machines may have different capacities $B_i$, but the processing times must not depend on the machine. Families, due dates and weights are ignored.

The algorithms throw `std::invalid_argument` if the objective is not the makespan or if the processing times depend on the machine, and `milp-rank-based-two-index-no-starts` also if a job has a non-zero release date.

## Principle

The models are the symmetry-broken formulations of Trindade et al. (2018): (MILP2+) for $P_m \mid s_j, B \mid C_{\max}$ and (MILP4+) for $P_m \mid r_j, s_j, B \mid C_{\max}$, generalized to machine-dependent capacities.

The three-index variables $x_{jik}$ of the [three-index models](milp_rank_based_three_index.md) are replaced by $x_{jk}$ (job $j$ is in batch $k$) and $y_{ki}$ (batch $k$ is processed on machine $i$). The symmetries are broken by sorting the jobs and by naming each batch after one of its jobs:

1. The jobs are renumbered in non-decreasing order of processing time (no starts) or of release date (with starts), ties keeping the order of the job ids.
2. There are $n$ batches, and batch $k$ is used iff job $k$ is in it.
3. Job $j$ can only be in batch $k$ if $j \le k$: the variables $x_{jk}$ are only defined for $j \le k$, which gives $n(n+1)/2$ variables instead of $n^2$.

Hence the job of largest index of a batch gives its name to the batch, and there is a single way to index the batches of a solution. Without starts, job $k$ is the longest job of batch $k$, so the processing time of batch $k$ is $p_k$ and no processing time variable is needed. With starts, job $k$ is the job released last in batch $k$, so the release date of batch $k$ is $r_k$, and the batches of a machine are processed in increasing order of $k$, i.e. in non-decreasing order of their release dates, which is an optimal order for the makespan.

## Model without batch starts

`milp-rank-based-two-index-no-starts`

**Input**: machines $i = 1, \ldots, m$ with capacity $B_i$; jobs $j = 1, \ldots, n$ with size $s_j$ and processing time $p_j$, numbered so that $p_1 \le \cdots \le p_n$.

**Variables**:

* $x_{jk} \in \{0, 1\}$ for $j \le k$: $x_{jk} = 1$ iff job $j$ is in batch $k$
* $y_{ki} \in \{0, 1\}$: $y_{ki} = 1$ iff batch $k$ is processed on machine $i$
* $b_k \in [0, \max_i B_i]$: capacity of batch $k$ (only when the capacities are not identical)
* $C_{\max} \ge 0$: makespan

**Objective**:

```math
\min C_{\max}
```

**Constraints** (numbers of Trindade et al., 2018, in the code comments):

* (39) Each job is in one batch

```math
\forall j \qquad \sum_{k \ge j} x_{jk} = 1
```

* (40) Capacity: with identical capacities $B$, the constraint also forces batch $k$ to contain job $k$ to be used; otherwise, the capacity of a batch is the capacity of its machine

```math
\forall k \qquad \sum_{j \le k} s_j \, x_{jk} \le B \, x_{kk}
\qquad \text{or} \qquad
\sum_{j \le k} s_j \, x_{jk} \le b_k, \quad b_k = \sum_i B_i \, y_{ki}
```

* (41) Strengthening: a job can only be in batch $k$ if job $k$ is

```math
\forall j < k \qquad x_{jk} \le x_{kk}
```

* (45) Each used batch is on one machine (equality in the code)

```math
\forall k \qquad x_{kk} = \sum_i y_{ki}
```

* (46) Makespan

```math
\forall i \qquad C_{\max} \ge \sum_k p_k \, y_{ki}
```

The solution is built by processing the batches of each machine in increasing order of $k$, without idle time.

## Model with batch starts

`milp-rank-based-two-index`

**Input**: as above, with release dates $r_j$, and the jobs numbered so that $r_1 \le \cdots \le r_n$.

**Additional variables**:

* $P_{ik} \ge 0$: processing time of batch $k$ on machine $i$ (0 if batch $k$ isn't on machine $i$)
* $S_{ik} \ge 0$: start time of batch $k$ on machine $i$

**Objective**:

```math
\min C_{\max}
```

**Constraints**: (39), (40), (41), (58) = (45), and

* (59) Processing time of batch $k$ on machine $i$

```math
\forall j \le k, \; \forall i \qquad P_{ik} \ge p_j \, (x_{jk} + y_{ki} - 1)
```

* (61) A batch starts after its release date

```math
\forall i, k \qquad S_{ik} \ge r_k \, y_{ki}
```

* (62) The batches of a machine are processed in increasing order of $k$

```math
\forall i, \; \forall k > 1 \qquad S_{ik} \ge S_{i,k-1} + P_{i,k-1}
```

* (64) Makespan

```math
\forall i \qquad C_{\max} \ge S_{in} + P_{in}
```

The solution is built by taking, for each batch $k$ with $y_{ki} = 1$, its start time $S_{ik}$ from the MILP solution.

## Solver

The model is solved with the default options of the solver and the time limit of the parameters. The solutions found by the solver are reported as they are found (when they improve the current one), and the bound of the solver, rounded up, is reported as the makespan bound. HiGHS writes its log to `highs.log` in the working directory.

## Optimality

Exact for $1, P_m \mid \text{p-batch}, B_i, \text{size}\_j (, r_j) \mid C_{\max}$: when the solver terminates, the bound equals the value of the solution and the output reports `"Optimal": true`. Otherwise (time limit), the output contains the best solution and the best bound found. The solver stops with its default relative gap (`1e-4` for HiGHS), so on instances with large objective values, the bound may be slightly below the value of the solution.

## Comparison with the three-index models

* $n(n+1)/2 + n m$ binary variables instead of $n^2 m$.
* No equivalent solutions from batch indexing or batch ordering on a machine. On identical machines, the permutations of the machines remain.
* Trindade et al. (2018) report that the symmetry-broken formulations solve larger instances to optimality, and give much better results at the time limit, than the formulations from the literature.
* Restricted to machine-independent processing times and to the makespan; use the three-index models for unrelated machines and the other objectives.

## Tests

`test/algorithms/milp_rank_based_test.cpp` (built when HiGHS is enabled) compares the value of the solution to the reference value on:

* `milp-rank-based-two-index-no-starts`: [`test/algorithms/milp_rank_based_two_index_no_starts_test.txt`](../test/algorithms/milp_rank_based_two_index_no_starts_test.txt), makespan on single and parallel machines without release dates (157 instances).
* `milp-rank-based-two-index`: [`test/algorithms/milp_rank_based_two_index_test.txt`](../test/algorithms/milp_rank_based_two_index_test.txt), makespan on single and parallel machines, with and without release dates (205 instances).

## References

* R. S. Trindade, O. C. B. de Araújo, M. H. C. Fampa and F. M. Müller (2018). Modelling and symmetry breaking in scheduling problems on batch processing machines. *International Journal of Production Research*, 56(22), 7031–7048. [doi:10.1080/00207543.2018.1424371](https://doi.org/10.1080/00207543.2018.1424371)
