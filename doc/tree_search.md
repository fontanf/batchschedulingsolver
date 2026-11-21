# Tree search

`--algorithm tree-search`

Source: [`include/batchschedulingsolver/algorithms/tree_search.hpp`](../include/batchschedulingsolver/algorithms/tree_search.hpp), [`src/algorithms/tree_search.cpp`](../src/algorithms/tree_search.cpp)

Uses the iterative beam search of [fontanf/treesearchsolver](https://github.com/fontanf/treesearchsolver).

## Supported problems

$\alpha \mid \text{p-batch}, B, \text{size}\_j, r_j \mid \gamma$ with $\alpha \in \{1, P_m\}$ and $\gamma \in \{C_{\max}, \sum w_j F_j, \sum w_j T_j\}$, with or without release dates.

On parallel machines, the machines must be identical: same capacity, and machine-independent processing times. Families are ignored. The algorithm throws `std::invalid_argument` on the other instances (other objectives, machine-dependent processing times or capacities, a job larger than the capacity).

## Principle

The algorithm comes from the single machine total weighted tardiness branching scheme of the treesearchsolver examples (`examples/batchschedulingtotalweightedtardiness.hpp`, removed from treesearchsolver in December 2023), generalized to identical parallel machines and to the makespan and total weighted flow time objectives, with two corrections (see [Differences with the treesearchsolver example](#differences-with-the-treesearchsolver-example)).

A node is a partial schedule. A child adds one job, so all the nodes of depth $k$ have $k$ scheduled jobs. Two nodes with the same set of scheduled jobs are compared by a dominance rule, and the dominated one is discarded. With an unbounded queue, the search is a dynamic programming algorithm: its states are the non-dominated partial schedules of each subset of jobs, and it is exact, but its number of states grows exponentially with the number of jobs. With a bounded queue, it is a beam search, a heuristic that keeps the best nodes of each depth according to their bound.

This is not the same as the dynamic programming algorithms of the roadmap (Chou and Wang, 2008; Wang, 2011; Rafiee Parsa et al., 2016; Queiroga et al., 2020). They take a fixed sequence of jobs and only decide where to cut it into consecutive batches; they are used inside metaheuristics that search over the sequences. The tree search decides the batches and their sequence together.

## Branching scheme

A schedule is a sequence of batches $\beta_1, \beta_2, \ldots$, in non-decreasing order of start time. Each batch is processed on the machine that becomes available first, and starts at
$$S(\beta_k) = \max\left(a, \; S(\beta_{k-1}), \; \max_{j \in \beta_k} r_j\right)$$
where $a$ is the availability time of this machine. A node stores the availability times of the machines (sorted), and the last batch, the "current batch": its start $S$, its end $E$ and its size.

The children of a node are:
* **Add a job $k$ to the current batch**, if $r_k \le S$, if it fits in the capacity, and if $k$ comes after the last job added in the order of the jobs in a batch: non-increasing processing times, ties broken by increasing job ids. Then the first job of a batch is its longest job, and $E = S + p_{\text{first}}$ doesn't change when jobs are added.
  * Exception: when the batch has a single job $f$ and starts after the availability of its machine and after the start of the previous batch, its start is the release date of $f$, and $f$ must be its first job (the other jobs of the batch are released earlier). Then any job can be added second, possibly longer than $f$, and the end of the batch increases. Such nodes are not compared with the dominance rule, since the completion time of $f$ may still increase.
* **Open a new batch with a job $j$**, on the machine available first, at $\max(a, S, r_j)$.

**Lemma 1 (list scheduling).** On identical machines, any schedule can be turned into a schedule of this form without increasing any completion time. *Proof.* Sort the batches by start time, and schedule them in this order, each one on the machine available first, as early as possible. By induction, the sorted vector of the machine availability times stays componentwise smaller than or equal to the one of the original schedule, so each batch starts no later than in the original schedule.

**Lemma 2.** Every schedule of this form is a leaf of the tree (before pruning). *Proof.* If a batch starts at $\max(a, S)$, all its jobs are released, and it is built by adding its jobs in the order above. Otherwise, it starts at the release date of one of its jobs $f$, which is opened first; then its longest other job is added second (the exception), and the other jobs in the order above.

Since the objectives are regular (non-decreasing in the completion times), the tree contains an optimal schedule.

## Pruning rules

**Rule 1.** If an unscheduled job $k$ can be added to the current batch without increasing its end ($r_k \le S$, it fits, $S + p_k \le E$), and if $k$ can't complete earlier than $E$ on another machine (for all the other machines with availability $a'$, $\max(a', S, r_k) + p_k \ge E$), then the node doesn't open new batches: its only children add jobs to the current batch. *Proof.* In a schedule where the current batch is closed without $k$, $k$ is processed in a later batch, which starts at $S$ or later, and completes at $E$ or later. Moving $k$ to the current batch doesn't change $E$, completes $k$ at $E$, and only removes $k$ from its later batch, so all the other start times can be kept. On a single machine, the second condition always holds. If $k$ can't be added because of the order of the jobs, the batch with $k$ is built in another branch.

**Rule 2.** A new batch with job $j$, starting at $S_j$, is not opened if another job $k$ (with $p_k > 0$) can be processed alone in a new batch on the same machine that ends at $\max(a, S, r_k) + p_k \le S_j$. *Proof.* Insert the batch $\{k\}$ before the batch of $j$ on this machine: the batch of $j$ still starts at $S_j$, $k$ completes before $S_j$, which is earlier than in any later batch (later batches start at $S_j$ or later), and removing $k$ from its later batch doesn't delay anything.

Both rules replace a schedule by one that is at least as good, and decrease the sum of the start times of the batches of the jobs (rule 2), or keep it and move a job to an earlier batch of the sequence (rule 1): the replacements can't cycle, so an optimal schedule that satisfies both rules exists.

## Dominance rule

Let $F$ be the sorted availability times of all the machines of a node, $F^-$ those of the machines other than the machine of the current batch, and $\text{cost}$ the objective value of the scheduled jobs (with the jobs of the current batch completing at $E$). For two nodes 1 and 2 with the same set of scheduled jobs, whose current batches can't grow, node 1 dominates node 2 if $\text{cost}_1 \le \text{cost}_2$ and either:
* **Case 1:** the current batches have the same start and end, $\text{size}_1 \le \text{size}_2$, the last job of node 1 comes before (or is) the last job of node 2 in the order of the jobs in a batch, and $F^-_1 \le F^-_2$ componentwise. Then node 1 can add to its current batch any set of jobs that node 2 can add to its current batch.
* **Case 2:** let $a_1$ be the earliest availability of node 1: $\max(a_1, S_1) \le S_2$, and $F_1$ without $a_1$ is componentwise smaller than or equal to $F^-_2$. Then node 1 can process the jobs that node 2 adds to its current batch in a new batch, which starts no later than $S_2$ and ends no later than $E_2$, and the availability times of node 1 remain componentwise smaller than or equal to the ones of node 2.

In both cases, by Lemma 1, any completion of node 2 can be reproduced from node 1 with completion times no larger. On a single machine, case 2 is $E_1 \le S_2$.

## Bound and guide

The bound of a node is the cost of its scheduled jobs plus, for each unscheduled job $k$, the cost of its earliest possible completion: in a new batch on the machine available first, $\max(a, S, r_k) + p_k$, or in the current batch, $\max(E, S + p_k)$, if it can be added to it. For the makespan, the bound is the largest of these completion times and of the availability times of the machines. The guide of the beam search is the bound.

A node is pruned when its bound is greater than or equal to the value of the best solution found; the children whose bound is greater than or equal to it are not generated.

## Search

Iterative beam search (`treesearchsolver::iterative_beam_search_2`): beam searches of width 1, 2, 4, ..., up to $10^8$, with the dominance rule applied within each depth. The minimum and maximum widths are parameters of the C++ function (`TreeSearchParameters`), not of the command line. If a beam search ends without discarding any node because of the beam width, the search is complete.

## Optimality

Exact. If a beam search completes, the best solution is optimal: the algorithm sets the bound to its value, and the output reports `"Optimal": true`. Otherwise (time limit), the output contains the best solution found, and the bound of the root, which is weak.

## Verification

* Brute force (`scripts/brute_force.py`): 8,300 random instances with up to 8 jobs and 4 machines, generated to hit the special cases (ties between processing times, idle time due to release dates, capacities from 1 to 10, the three objectives), and the 580 test instances of `test/algorithms/tree_search_test.txt`: the tree search returns the optimal value, proves it, and its solution is feasible.
* The instances of Queiroga et al. (2020) ($1 \mid \text{p-batch}, B, \text{size}\_j, r_j \mid \sum w_j T_j$, `data/queiroga2020`, best known values and bounds in `data/data_single_twt.csv`), with a time limit of 60 s for 10 to 20 jobs and 30 s for 25 to 100 jobs (6 instances in parallel):

| Jobs | Instances | Proved optimal | Equal to the best known value | Gap to the best known value (average, maximum) | Best known value proved optimal in `data_single_twt.csv` |
| --- | --- | --- | --- | --- | --- |
| 10 | 90 | 90 (all in less than 0.1 s) | 90 | 0%, 0% | 90 |
| 15 | 90 | 90 (at most 1 s) | 90 | 0%, 0% | 90 |
| 20 | 90 | 84 (at most 59 s) | 90 | 0%, 0% | 74 |
| 25 | 90 | 48 | 90 | 0%, 0% | 41 |
| 50 | 90 | 0 | 30 | 1.9%, 11.4% | 3 |
| 100 | 90 | 0 | 2 | 11.7%, 21.9% | 0 |

  No value is below the best known value, no proved value differs from a best known value proved optimal, and all the solutions are feasible. The proofs of 33 instances (12 with 20 jobs, 21 with 25 jobs) whose best known value wasn't proved optimal in `data_single_twt.csv` confirm that this value is optimal.

  These results need the reader of the `queiroga2020` format to set the due dates (fixed in #14).

## Differences with the treesearchsolver example

* **Pruning of the new batches.** The example didn't open a new batch starting at or after the end of a sibling, including a sibling that adds a job to the current batch. When the current batch can grow (the exception above), such a sibling increases the end of the current batch and delays its first job, so it doesn't dominate. Counterexample (one machine, capacity 10): jobs $f$ ($p = 1$, $r = 5$, $d = 6$, $w = 100$), $k$ ($p = 10$, $r = 0$, $d = 1000$, $w = 1$) and $j$ ($p = 1$, $r = 15$, $d = 16$, $w = 100$). The only schedule with no tardiness is $\{f\}, \{j\}, \{k\}$; after $\{f\}$, adding $k$ to the batch of $f$ ends at 15, and the example prunes the new batch $\{j\}$, which starts at 15. Rule 2 only uses new batches, and rule 1 only jobs that don't increase the end of the current batch.
* **Dominance.** The example compared two nodes whose current batches have the same start and end with their sizes only. A node can then dominate a node whose current batch can still receive more jobs (its last job is longer). Case 1 adds the condition on the last jobs. On a single machine, rule 1 makes this condition redundant (a job that fits in the current batch of the dominated node fits in the current batch of the dominating node without increasing its end, so rule 1 applies there), and no counterexample was found in the random tests, but the proof needs it on parallel machines.

## Limits

* The number of states grows exponentially with the number of jobs: the proofs of optimality stop at 20 to 25 jobs within a minute on the instances of Queiroga et al. (2020).
* As a heuristic, the beam search is far from the best known values on large instances (11.7% on average with 100 jobs in 30 s), where local search is better (Queiroga et al., 2020).
* The bound is weak: it considers each unscheduled job independently.
* Without a proof of optimality, the reported bound is the bound of the root.

## Tests

* `test/algorithms/tree_search_test.cpp` with the instances of `test/algorithms/tree_search_test.txt`: all the single and parallel machine test instances with up to 8 jobs, for the makespan, the total (weighted) flow time and the total (weighted) tardiness, with and without release dates, with their optimal values computed by brute force. The test checks the value, the proof of optimality and the feasibility of the solution.

## References

* E. Queiroga, R. G. S. Pinheiro, Q. Christ, A. Subramanian and A. A. Pessoa (2020). Iterated local search for single machine total weighted tardiness batch scheduling. *Journal of Heuristics*. [doi:10.1007/s10732-020-09461-x](https://doi.org/10.1007/s10732-020-09461-x)
* L. Libralesso, P. A. Focke, A. Secardin and V. Jost (2022). Iterative beam search algorithms for the permutation flowshop. *European Journal of Operational Research*, 301(1), 217–234. [doi:10.1016/j.ejor.2021.10.015](https://doi.org/10.1016/j.ejor.2021.10.015) (the iterative beam search)
