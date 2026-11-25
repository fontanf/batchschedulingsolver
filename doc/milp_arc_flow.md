# Arc-flow MILP

`--algorithm milp-arc-flow`

Source: [`include/batchschedulingsolver/algorithms/milp_arc_flow.hpp`](../include/batchschedulingsolver/algorithms/milp_arc_flow.hpp), [`src/algorithms/milp_arc_flow.cpp`](../src/algorithms/milp_arc_flow.cpp)

Requires an MILP solver: build with `-DBATCHSCHEDULINGSOLVER_USE_HIGHS=ON` (default solver, `--solver highs`), `-DBATCHSCHEDULINGSOLVER_USE_CBC=ON` (`--solver cbc`) or `-DBATCHSCHEDULINGSOLVER_USE_XPRESS=ON` (`--solver xpress`).

## Supported problems

$1 \mid \text{p-batch}, B, \text{size}\_j \mid C_{\max}$

Families are ignored. The algorithm throws `std::invalid_argument` if the objective is not the makespan, if the instance has several machines or if a job has a non-zero release date. A job larger than the capacity makes the model infeasible.

## Principle

This is the (FLOW) formulation of Trindade, de Araújo and Fampa (2021).

A batch is a path from node 0 to node $B$ in a graph $G$ whose nodes $0, 1, \ldots, B$ are the levels of filling of the batch: a job of size $s$ is an arc $(u, u + s)$, and the empty space at the end of the batch is a loss arc $(u, B)$. There is no empty space at the beginning of a batch or between jobs, which removes symmetric representations of a batch.

A batch has the processing time of its longest job. Hence $G$ is replicated once per distinct processing time $P_1 < P_2 < \cdots < P_\delta$: the copy $t$ (an "arc-flow structure") holds the batches of processing time $P_t$, and only jobs with $p_j \le P_t$ can be assigned to it. The flow $v_t$ that goes from $0$ to $B$ in structure $t$ is the number of batches of processing time $P_t$, and the makespan is $\sum_t P_t v_t$. The jobs are assigned to the structures in aggregate, by counting them per size: the variables $z_{\ell t}$ carry the jobs of size $S_\ell$ that haven't been assigned to the structures $1, \ldots, t$ to the next structures.

The number of variables depends on the capacity $B$, the number $\theta$ of distinct sizes and the number $\delta$ of distinct processing times, but not on the number of jobs: there are at most $\delta (\theta (B + 1) + B)$ arc variables. The model is pseudo-polynomial, and well suited to instances with many jobs and few distinct sizes and processing times.

## Graph and arc reduction

* Job arcs $A^J$: from node 0, a breadth-first search adds an arc $(u, u + S_\ell)$ for each reached node $u$ and each distinct size $S_\ell$ with $u + S_\ell \le B$. Only the job arcs that belong to a path starting at node 0 are created (Rule 1 of the paper).
* Loss arcs $A^L$: an arc $(u, B)$ for each node $u \in \{1, \ldots, B - 1\}$ incident to a job arc (Rule 2).
* The feedback arc $(B, 0)$ is represented by the variable $v_t$.

## Model

**Input**:

* capacity $B$; distinct processing times $P_1 < \cdots < P_\delta$; distinct sizes $S_1 < \cdots < S_\theta$
* $n_{\ell t}$: number of jobs of size $S_\ell$ and processing time $P_t$; $n^+_{\ell t} = \sum_{t' \le t} n_{\ell t'}$: number of jobs of size $S_\ell$ and processing time at most $P_t$; $n_t = \sum_\ell n_{\ell t}$: number of jobs of processing time $P_t$
* for a job arc $a = (u, w)$, $\ell(a)$ is the index of its size $w - u$

**Variables** (all integer):

* $f_{at} \in [0, \min(n_t, n^+_{\ell(a) t})]$: flow on job arc $a$ in structure $t$ (Rules 3 and 4 of the paper)
* $y_{ut} \in [0, n_t]$: flow on loss arc $(u, B)$ in structure $t$
* $v_t \in [0, n_t]$: number of batches of processing time $P_t$
* $z_{\ell t} \in [0, n^+_{\ell t}]$, $t < \delta$: number of jobs of size $S_\ell$ and processing time at most $P_t$ that are not assigned to the structures $1, \ldots, t$

**Objective**:

```math
\min \sum_t P_t \, v_t
```

**Constraints** (numbers of Trindade et al., 2021, in the code comments):

* (18)–(20) Flow conservation in each structure $t$, at node 0, at the intermediate nodes $u$ incident to a job arc, and at node $B$

```math
\sum_{(0, w) \in A^J} f_{(0, w) t} = v_t
\qquad
\sum_{(u', u) \in A^J} f_{(u', u) t} = \sum_{(u, w) \in A^J} f_{(u, w) t} + y_{ut}
\qquad
\sum_{(u, B) \in A^J} f_{(u, B) t} + \sum_{u} y_{ut} = v_t
```

* (21) Assignment of the jobs of each size $S_\ell$ to the structures, with $z_{\ell 0} = z_{\ell \delta} = 0$

```math
\forall \ell, t \qquad \sum_{a : \ell(a) = \ell} f_{at} + z_{\ell t} - z_{\ell, t-1} = n_{\ell t}
```

## Solution

For each structure $t$ in increasing order of $P_t$, the flow is decomposed into $v_t$ paths from 0 to $B$ (following, from each node, the first job arc with remaining flow, or the loss arc). Each path is a batch of processing time $P_t$, and each job arc of size $S_\ell$ of the path receives the shortest job of size $S_\ell$ not assigned yet. The batches are processed without idle time, in non-decreasing order of processing time.

Since the jobs of size $S_\ell$ can only move to structures of larger processing time (through $z_{\ell t}$), the jobs of size $S_\ell$ assigned to the structures $1, \ldots, t$ can always be chosen among the shortest ones, so each job assigned to structure $t$ has $p_j \le P_t$ and the batch has processing time at most $P_t$.

## Solver

The model is solved with the default options of the solver and the time limit of the parameters. The solutions found by the solver are reported as they are found (when they improve the current one), and the bound of the solver, rounded up, is reported as the makespan bound. HiGHS writes its log to `highs.log` in the working directory.

## Optimality

Exact: when the solver terminates, the bound equals the value of the solution and the output reports `"Optimal": true`. Otherwise (time limit), the output contains the best solution and the best bound found. The solver stops with its default relative gap (`1e-4` for HiGHS), so on instances with large objective values, the bound may be slightly below the value of the solution.

## Comparison with the rank-based MILPs

* The size of the model doesn't depend on the number of jobs, but grows with the capacity and with the numbers of distinct sizes and processing times. The [rank-based models](milp_rank_based_two_index.md) have $O(n^2)$ binary variables.
* Trindade et al. (2021) report much better results with (FLOW) on benchmark instances from the literature than with two other formulations, including the rank-based (MILP1+), and optimal solutions for instance sizes not solved before by exact methods.
* Single machine and makespan only.

## Results

The instances of Chen et al. (2011) (`data/chen2011`, `--format chen2011`, see [`benchmarks.md`](benchmarks.md#chen2011)), $B = 10$, with HiGHS, a time limit of 60 s, 6 instances in parallel on 6 cores (the times are in whole seconds):

| Jobs | Instances | Optimal within 60 s | Time (average) | Time (maximum) |
| --- | --- | --- | --- | --- |
| 10 | 600 | 600 | 0.0 s | 0 s |
| 20 | 600 | 600 | 0.1 s | 7 s |
| 50 | 600 | 600 | 1.5 s | 58 s |
| 100 | 600 | 600 | 2.5 s | 44 s |
| 200 | 600 | 596 | 5.7 s | 60 s |
| 300 | 600 | 599 | 5.1 s | 60 s |
| 500 | 600 | 600 | 4.2 s | 41 s |

The 5 instances not solved within 60 s (4 with 200 jobs, 1 with 300 jobs, all with $p_j \in [1, 20]$ and sizes in $[1, 10]$ or $[2, 4]$) are solved to optimality in 40 to 65 s with a time limit of 600 s. All the certificates are feasible (independent check). The instances with sizes in $[4, 8]$ (`s3`) are solved in less than 2 s, and the instances with processing times in $[1, 20]$ and sizes in $[2, 4]$ (`p2s2`) are the hardest. The optimal values are stored in `data/data_single_makespan.csv`.

## Tests

There is no test for this algorithm in `test/algorithms/` yet.

## References

* R. S. Trindade, O. C. B. de Araújo and M. Fampa (2021). Arc-flow approach for single batch-processing machine scheduling. *Computers & Operations Research*, 134, 105394. [doi:10.1016/j.cor.2021.105394](https://doi.org/10.1016/j.cor.2021.105394)
* R. S. Trindade, O. C. B. de Araújo, M. H. C. Fampa and F. M. Müller (2018). Modelling and symmetry breaking in scheduling problems on batch processing machines. *International Journal of Production Research*, 56(22), 7031–7048. [doi:10.1080/00207543.2018.1424371](https://doi.org/10.1080/00207543.2018.1424371)
