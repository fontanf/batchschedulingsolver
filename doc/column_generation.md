# Column generation

`--algorithm column-generation`

Source: [`include/batchschedulingsolver/algorithms/column_generation.hpp`](../include/batchschedulingsolver/algorithms/column_generation.hpp), [`src/algorithms/column_generation.cpp`](../src/algorithms/column_generation.cpp)

Column generation on the arc-based graph formulation of Alfieri et al. (2021) for $1 \mid \text{p-batch}, B, \text{size}\_j \mid \sum C_j$, in its weighted version by Druetto and Grosso (2022), implemented with [columngenerationsolver](https://github.com/fontanf/columngenerationsolver). The solutions are built by the limited discrepancy search of columngenerationsolver, instead of the heuristics of the papers (price-and-branch for Alfieri et al., 2021; rounding of the arcs from the source, VR-UB and ER-UB, for Druetto and Grosso, 2022).

## Supported problems

$1 \mid \text{p-batch}, B, \text{size}\_j, \text{incompatible families} \mid \sum w_j C_j$, and so $\sum C_j$ when the weights are 1.

The algorithm throws `std::invalid_argument` if the objective is not the total flow time, if the instance has several machines, if a job has a non-zero release date, if a weight is not positive, or if a job doesn't fit in the machine. Families are optional: all the jobs have the family $-1$ by default, and a batch only contains jobs with the same `family_id`, as in the brute force.

It needs an LP solver: HiGHS (`--solver highs`, the default, with `-DBATCHSCHEDULINGSOLVER_USE_HIGHS=ON`), CLP (`--solver cbc`, with `-DBATCHSCHEDULINGSOLVER_USE_CLP=ON` or `-DBATCHSCHEDULINGSOLVER_USE_CBC=ON`) or Xpress (`--solver xpress`).

## Optimality

Heuristic with a bound: the bound is the value of the linear relaxation at the root node, rounded up, valid once the column generation has converged (the pricing is exact). The output reports `"Optimal": true` when the value of the solution reaches the bound, which happens often since the bound is very tight (for example, on 20-job instances of `alfieri2021`, the root relaxation is often integral). Otherwise, the solution may not be optimal, even when the search stops before the time limit: the limited discrepancy search is not complete, see below.

The algorithm stops at the time limit (`--time-limit`) or when the limited discrepancy search has explored its tree.

## Model

Let $W = \sum_j w_j$. The graph has the nodes $0, 1, \dots, W$: node $t$ means that jobs of total weight $t$ are scheduled. An arc $(t, t + w(B), B)$, for a batch $B$ of total size at most $B$ whose jobs have the same family, means that the batch $B$ is scheduled after jobs of total weight $t$. The jobs of $B$ and of the following batches, of total weight $W - t$, all complete after the end of $B$, so the total weighted completion time of a sequence of batches is the sum of the costs $(W - t)\, p(B)$ of its arcs. A solution is a path from $0$ to $W$ whose batches partition the jobs (Druetto and Grosso, 2022, Property 1; with unit weights, the nodes are the numbers of scheduled jobs, as in Alfieri et al., 2021).

Master problem, with a variable $x_{t,k,B}$ per arc:

$$\min \sum_{(t, k, B)} (W - t)\, p(B)\, x_{t,k,B}$$

$$\sum_{(t, k, B)} x_{t,k,B} - \sum_{(k, t, B)} x_{k,t,B} = [t = 0] \quad 0 \le t < W \qquad (u_t)$$

$$\sum_{(t, k, B) : j \in B} x_{t,k,B} = 1 \quad \text{for each job } j \qquad (v_j)$$

The flow conservation constraint of node $W$ is redundant and omitted ($u_W = 0$). This is the arc-based model of Druetto and Grosso (2022); their path-based model (one variable per path) has the same bound but was much slower in their experiments, and fixing one of its columns fixes a whole solution, which doesn't suit the limited discrepancy search.

## Pricing

The reduced cost of an arc is $(W - t)\, p(B) - u_t + u_{t + w(B)} - \sum_{j \in B} v_j$. For each family, the free jobs (not in a fixed column) are sorted by non-increasing processing time, and the dynamic programming algorithm of Druetto and Grosso (2022) computes $g_r(\tau, \ell)$, the largest sum of the $v_j$ over the subsets of the jobs $r, r + 1, \dots$ of total size at most $\tau$ and total weight exactly $\ell$, for all $r$, $\tau \le B$ and $\ell$. For any $t$ and $\ell$, an arc of minimum reduced cost of weight $\ell$ starting at $t$ is one of the batches of $g_r(B, \ell)$, $r$ being the first job of each processing time (their Property 2). The pricing is exact.

Differences with the paper:
* For a weight $\ell$, $(W - t)\, p_r - g_r(B, \ell)$ is a line in $W - t$, so the minimum over $r$ for all the start nodes $t$ is computed with the lower envelope of these lines: $O(W (n + W))$ instead of $O(n W^2)$ for the enumeration of the paper. The dynamic programming takes $O(n B W)$ time, and stores one bit per state for the retrieval of the batches.
* The paper adds all the arcs of negative reduced cost (for each $\ell$, $r$ and $t$); here, only the best arc for each $\ell$ and $t$ is a candidate, and the $n + W$ candidates of most negative reduced cost are added (`maximum_number_of_columns_per_pricing` in `ColumnGenerationParameters`).
* With fixed columns (in the limited discrepancy search), the jobs of the fixed arcs are removed from the dynamic programming, and an arc is only generated if it doesn't overlap a fixed arc: its interval $[t, t + w(B))$ must not intersect the interval of a fixed arc. The columns of the master problem which violate these conditions are removed (`infeasible_columns`).
* Families: one dynamic programming per family.

Bound: in an integer solution compatible with the fixed columns, each arc which isn't fixed contains at least one free job, so the bound is the value of the restricted master problem plus the number of free jobs times the minimum reduced cost (when it is negative).

**Initial columns.** The arcs of an initial solution: the jobs of each family packed into batches with the first fit rule, by non-increasing processing time, and the batches sequenced by non-decreasing $p(B) / w(B)$ (the optimal sequence of fixed batches). This solution is also the first solution reported. The column generation of columngenerationsolver handles the feasibility of the restricted master problems with dummy columns, so no other initial columns are needed (the paper adds all the batches of consecutive jobs in SPT order at all the positions).

## Limited discrepancy search

`columngenerationsolver::limited_discrepancy_search`, with the default parameters (no discrepancy limit, `--time-limit` as time limit): at each node, the column generation solves the linear relaxation with the fixed columns; the main child fixes the column of the relaxation closest to 1 (a batch at a position of the sequence), the other child forbids it and has one more discrepancy; nodes are explored by increasing discrepancy. The first dive is similar to the rounding of Druetto and Grosso (2022), except that the fixed arc isn't necessarily the one closest to the source. Integral relaxation solutions, and the rounding of the relaxation solutions, give new solutions.

columngenerationsolver identifies the columns by their content, so a forbidden arc generated again by the pricing is discarded; since the pricing only returns the best arc for each weight and start node, it then returns no arc for this weight and start node at this node.

The search only branches on the columns of the relaxations, so it isn't complete: on small instances, it may explore its whole tree without finding an optimal solution (1 instance among 500 in the verification below).

## Verification

* The single machine total flow time and total weighted flow time test instances without release dates solved by the brute force (68 instances): the algorithm finds the optimal value and proves it (bound equal to the value).
* 500 random instances with up to 9 jobs, capacities 1 to 10, processing times up to 50, weights 1 to 10, and families on 40% of them, with a time limit of 5 s: all the solutions are feasible (independent check), every bound is at most the optimal value given by the brute force, and the algorithm finds the optimal value on 499 instances (it proves optimality on 482). On the remaining one, the limited discrepancy search explores its whole tree (17 nodes) and returns 611 for an optimal value of 609.
* Each solution of the master problem is checked: its arcs must form a path from 0 to $W$, and the value recomputed by `SolutionBuilder` must be the value of the master problem; otherwise, `std::logic_error` is thrown.

## Results

`alfieri2021` instances with $b = 10$ (single machine, $\sum C_j$), 60 s per instance (the time limit of the price-and-branch heuristic in Alfieri et al., 2021), 6 instances in parallel on 6 cores, HiGHS. The gap is $100 (UB - LB) / UB$ with the bound and the value of the solution of the algorithm, as in the paper.

| $n$ | $\sigma$ | Gap (average) | Gap (worst) | Gap (best) | Optimal | Paper: gap (average) | Paper: optimal |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 20 | $\sigma_1$ | 0.76% | 1.71% | 0.00% | 2 / 10 | 1.30% | 2 / 10 |
| 20 | $\sigma_2$ | 1.06% | 2.86% | 0.00% | 2 / 10 | 1.55% | 2 / 10 |
| 20 | $\sigma_3$ | 0.62% | 3.15% | 0.00% | 7 / 10 | 0.63% | 7 / 10 |
| 20 | $\sigma_4$ | 1.53% | 2.83% | 0.61% | 0 / 10 | 2.15% | 0 / 10 |
| 40 | $\sigma_1$ | 2.19% | 7.75% | 0.09% | 0 / 10 | 1.30% | 0 / 10 |
| 40 | $\sigma_2$ | 1.84% | 3.66% | 0.11% | 0 / 10 | 1.17% | 0 / 10 |
| 40 | $\sigma_3$ | 1.17% | 2.04% | 0.00% | 1 / 10 | 0.89% | 1 / 10 |
| 40 | $\sigma_4$ | 2.66% | 5.46% | 0.45% | 0 / 10 | 2.61% | 0 / 10 |
| 60 | $\sigma_1$ | 1.81% | 2.94% | 0.30% | 0 / 10 | 0.91% | 0 / 10 |
| 60 | $\sigma_2$ | 2.15% | 3.93% | 0.60% | 0 / 10 | 0.98% | 0 / 10 |
| 60 | $\sigma_3$ | 0.88% | 2.28% | 0.00% | 1 / 10 | 0.49% | 1 / 10 |
| 60 | $\sigma_4$ | 4.61% | 9.74% | 0.64% | 0 / 10 | 2.57% | 0 / 10 |
| 80 | $\sigma_1$ | 1.54% | 3.03% | 0.34% | 0 / 10 | 0.74% | 0 / 10 |
| 80 | $\sigma_2$ | 1.93% | 4.67% | 0.08% | 0 / 10 | 0.82% | 0 / 10 |
| 80 | $\sigma_3$ | 1.36% | 2.75% | 0.50% | 0 / 10 | 0.47% | 0 / 10 |
| 80 | $\sigma_4$ | 3.64% | 5.05% | 1.87% | 0 / 10 | 5.78% | 0 / 10 |
| 100 | $\sigma_1$ | 0.92% | 1.75% | 0.06% | 0 / 10 | 0.46% | 0 / 10 |
| 100 | $\sigma_2$ | 1.26% | 2.35% | 0.49% | 0 / 10 | 0.41% | 0 / 10 |
| 100 | $\sigma_3$ | 0.38% | 0.83% | 0.00% | 2 / 10 | 0.17% | 2 / 10 |
| 100 | $\sigma_4$ | 3.62% | 5.11% | 1.14% | 0 / 10 | 4.44% | 0 / 10 |

The columns "Paper" are the results of the price-and-branch heuristic (CG-UB) of Alfieri et al. (2021), Table 1, with the same lower bound and time limit, CPLEX 12.8 and an Intel Core i7-6500U, on their instances generated the same way (the files of `data/alfieri2021` may not be the instances of their tables, see [`benchmarks.md`](benchmarks.md)). The optimal counts are the instances where the solution reaches the bound.

The root bound is fast (about 3 s on `inst_100_4_1`), and the LP solves take almost all the time: on `inst_100_4_1`, 41 s of LP solves against 0.17 s of pricing in 60 s. The limited discrepancy search is better than the price-and-branch heuristic on the 20-job instances and on the $\sigma_4$ instances with 80 and 100 jobs (where the price-and-branch heuristic reaches its time limit), similar on the $\sigma_4$ instances with 40 jobs, and worse on the other instances with 40 to 100 jobs, by about a factor 2 on the average gap. It reaches the bound on the same numbers of instances.

## Tests

* `test/algorithms/column_generation_test.cpp` with the instances of `test/algorithms/column_generation_test.txt` (the single machine total flow time and total weighted flow time instances without release dates of `test/algorithms/brute_force_test.txt`, 68 instances): the algorithm finds the optimal value of the certificate, and its bound is at most this value.

## References

* A. Alfieri, A. Druetto, A. Grosso and F. Salassa (2021). Column generation for minimizing total completion time in a parallel-batching environment. *Journal of Scheduling*. [doi:10.1007/s10951-021-00703-9](https://doi.org/10.1007/s10951-021-00703-9)
* A. Druetto and A. Grosso (2022). Column generation and rounding heuristics for minimizing the total weighted completion time on a single batching machine. *Computers & Operations Research*, 139, 105639. [doi:10.1016/j.cor.2021.105639](https://doi.org/10.1016/j.cor.2021.105639)
* R. Sadykov, F. Vanderbeck, A. Pessoa, I. Tahiri and E. Uchoa (2019). Primal heuristics for branch and price: the assets of diving methods. *INFORMS Journal on Computing*, 31(2), 251–267. [doi:10.1287/ijoc.2018.0822](https://doi.org/10.1287/ijoc.2018.0822) (the limited discrepancy search of columngenerationsolver)
