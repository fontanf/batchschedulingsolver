# Local search

`--algorithm local-search`

Source: [`include/batchschedulingsolver/algorithms/local_search.hpp`](../include/batchschedulingsolver/algorithms/local_search.hpp), [`src/algorithms/local_search.cpp`](../src/algorithms/local_search.cpp)

Population-based iterated local search: the neighborhoods, perturbations and constructive heuristic of the iterated local search of Queiroga et al. (2020) for $1 \mid \text{p-batch}, B, \text{size}\_j, r_j \mid \sum w_j T_j$, within the population-based framework of Medeiros et al. (2024) for identical parallel machines with incompatible families. The structure is the same as the local searches of shopschedulingsolver (population of `localsearchsolver`, adaptive choice of the perturbation).

## Supported problems

$1$, $P_m$, $R_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j, \text{incompatible families} \mid \gamma$ with $\gamma \in \{C_{\max}, \sum w_j F_j, \sum w_j T_j, L_{\max}\}$.

The algorithm throws `std::invalid_argument` for the throughput objective, with negative weights, or if a job doesn't fit in any machine. For $L_{\max}$, as in `SolutionBuilder`, only the late jobs count.

All the objectives have the same form: the cost of a batch depends only on its jobs and on its completion time, the cost of a machine is the sum (for $\sum w_j F_j$ and $\sum w_j T_j$) or the maximum (for $C_{\max}$ and $L_{\max}$) of the costs of its batches, and the cost of the solution is the sum or the maximum of the costs of the machines. All the costs are non-negative. This is what the evaluation of the moves relies on, so the neighborhoods are the same for all the objectives.

For $C_{\max}$ and $L_{\max}$, the moves are compared lexicographically on the objective, then on the sum of the costs of the machines. Only the critical machine counts in the objective, so most moves don't change it; the secondary cost makes the moves that unload the other machines improving, instead of leaving the search on a plateau. With the objective alone, the local search missed the optimum of `tests/makespan_unrelated/makespan_n5x2x1_s0.json` (5 machines) with 11 of 30 seeds in 100 iterations.

## Optimality

Heuristic: no bound is computed, except when a solution of cost 0 is found, which is optimal since all the costs are non-negative. Otherwise, the output never reports `"Optimal": true`.

The algorithm stops at the time limit, or after a maximum number of iterations (`--maximum-number-of-iterations`). It also has a maximum number of iterations without improvement, a parameter of the C++ function (`LocalSearchParameters`) only. Without a time limit or a maximum number of iterations, it doesn't stop, so **`--time-limit` or `--maximum-number-of-iterations` must be set**. The random generator is seeded with `--seed` (0 by default).

## Algorithm

**Solution.** A sequence of batches on each machine; each batch starts as soon as the previous batch of its machine is completed and all its jobs are released (semi-active schedule), so a solution is determined by the batches and their sequences.

**Population.** Parameters of Medeiros et al. (2024): between 5 and 15 solutions (when the size goes above 15, solutions are removed until there are 5 left), with a biased fitness combining the rank of the cost and the rank of the contribution to the diversity (distance to the closest solution, 2 elite solutions). The distance between two solutions is the number of jobs whose batch doesn't contain the same jobs in both solutions, whatever the machines and the positions of the batches (Medeiros et al., 2024); it is computed with a random key per job, the key of a batch being the xor of the keys of its jobs. After 250 iterations without improvement of the best solution, the population is diversified: only the 2 best solutions are kept, and new solutions are generated.

**Initial solutions.** Constructive heuristic of Queiroga et al. (2020), extended to parallel machines: the jobs are sorted by due date (by release date for $C_{\max}$ and $\sum w_j F_j$); a job is chosen at random among the first 4 jobs of the list and added to the current batch if it fits (capacity and family); otherwise, the smallest of these 4 jobs which fits is added; if none fits, the batch is closed. A batch is opened on the machine available first among the machines where its first job fits. Each solution is then improved by the local search. The time window decomposition heuristic used by Medeiros et al. (2024) is not implemented.

**Iterations.** At each iteration:
1. Select a solution of the population by binary tournament.
2. Apply 1 to 3 moves (chosen at random) of a perturbation, chosen at random with weights $2 + s$, $s$ being the number of times the perturbation has improved the solution it was applied to:
   * Batch split (Medeiros et al., 2024): split a random batch with at least 2 jobs, after shuffling its jobs, at a random point, and insert the two new batches at random positions, on random machines where they fit.
   * Job swap (Queiroga et al., 2020): swap two random jobs of different batches, of the same family, if the capacities allow it; otherwise, split a batch.
3. Apply the local search, and add the solution to the population.

**Local search.** Randomized variable neighborhood descent (Queiroga et al., 2020): the neighborhoods are explored in a random order, each one completely, and the best move is applied if it improves the solution (ties broken at random; lexicographic comparison for $C_{\max}$ and $L_{\max}$, see above); after an improvement, the order is shuffled again and the descent restarts from the first neighborhood; it stops when no neighborhood improves the solution. The neighborhoods, on all the machines:
* Job insertion: move a job into another batch, of any machine (if it fits; its batch is removed if it becomes empty), or into a new batch at any position of any machine (Medeiros et al., 2024; not for a job alone in its batch, which is a batch insertion).
* Job swap: swap two jobs of the same family of different batches.
* Batch insertion: move a batch to any position of any machine.
* Batch swap: swap two batches.
* Batch merge: merge two batches of the same family, at the position of the first or of the second one.

**Evaluation of the moves.** For each machine, the completion time of each batch, the cost of each prefix and the cost of each suffix of its sequence are stored. A move changes the sequences of one or two machines from some position: the new sequence is evaluated from this position only, and the evaluation stops (i) as soon as the cost reaches the cost of the best move found, since costs (primary and secondary) only increase, or (ii) when an unchanged batch starts at the same time as in the current solution, since the rest of the machine is unchanged: its cost is the stored suffix cost. The cost of a batch is computed in constant time from aggregates of its jobs (total weight, sum of the weighted release dates and due dates, smallest and largest due date), except for $\sum w_j T_j$ when the batch completes between the smallest and the largest due date of its jobs. A move is evaluated in $O(n)$ time, as in Queiroga et al. (2020), and each neighborhood has $O(n^2)$ moves.

After each move, the cost of the solution is recomputed and compared with the cost given by the evaluation of the move, and the solutions reported are checked with `SolutionBuilder`: a difference throws `std::logic_error`.

## Generalization

Compared with the papers:
* Unrelated machines: the processing time of a batch is computed for the machine it is evaluated on.
* Machine-dependent capacities: each move checks the capacity of the machines of its batches.
* Families are optional (all the jobs have the family $-1$ by default), as in Medeiros et al. (2024); without them, the neighborhoods are those of Queiroga et al. (2020) extended to several machines.
* The objectives $C_{\max}$, $\sum w_j F_j$ and $L_{\max}$, with the form described above.

## Verification

* Random instances with up to 8 jobs and 3 machines, generated to hit the special cases (identical and unrelated machines, identical and different capacities, families, release dates, the four objectives), with a time limit of 0.2 s: on 1,000 instances, the solutions are feasible (independent check: capacities, release dates, overlaps, families, each job exactly once), no evaluation of a move differs from the recomputed cost, and the local search finds the optimal value given by the brute force on all of them.
* The same on 500 instances with 10 to 60 jobs and up to 4 machines: feasible solutions, no difference between the evaluation of the moves and the recomputed costs.
* The instances of `test/algorithms/brute_force_test.txt` (1,085 instances) with 40 seeds and 100 iterations: the local search finds the optimal value in all the 43,400 runs (with 30 iterations, it misses it in 1 of 10,850 runs).
* The instances of Queiroga et al. (2020) ($1 \mid \text{p-batch}, B, \text{size}\_j, r_j \mid \sum w_j T_j$, `data/queiroga2020`, best known values (BKS) in `data/data_single_twt.csv`), one run per instance with seed 0, 6 runs in parallel, with the version before the lexicographic comparison (for $\sum w_j T_j$, it explores the same moves, and it is 2.5% faster). The gap of a value $v$ is $100 (v - \text{BKS}) / v$, as in the paper:

| Jobs | Time limit | Equal to the BKS | Below the BKS | Gap (average, maximum) | Time limit | Equal to the BKS | Below the BKS | Gap (average, maximum) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 10 | 1 s | 90 / 90 | 0 | 0%, 0% | | | | |
| 15 | 1.5 s | 90 / 90 | 0 | 0%, 0% | | | | |
| 20 | 2 s | 90 / 90 | 0 | 0%, 0% | | | | |
| 25 | 2.5 s | 90 / 90 | 0 | 0%, 0% | | | | |
| 30 | 3 s | 90 / 90 | 0 | 0%, 0% | | | | |
| 35 | 3.5 s | 89 / 90 | 0 | 0.01%, 0.75% | | | | |
| 50 | 5 s | 76 / 90 | 0 | 0.13%, 5.11% | 25 s | 80 / 90 | 0 | 0.06%, 1.76% |
| 75 | 7.5 s | 23 / 90 | 0 | 1.09%, 7.28% | 37.5 s | 42 / 90 | 0 | 0.54%, 5.85% |
| 100 | 10 s | 1 / 90 | 0 | 2.29%, 12.07% | 50 s | 5 / 90 | 1 | 1.17%, 6.60% |

  For comparison, the average gaps of the iterated local search of Queiroga et al. (2020) (average of 10 runs; the BKS come from its long runs) are about 0.1% with 50 jobs in 3 to 8 s, 0.5% with 75 jobs in 15 to 42 s and 0.6% with 100 jobs in 42 to 140 s, on an Intel Xeon E5-2650 v4. On a 100-job instance, the local search runs about 150 to 170 iterations per second, the same order as the about 80 local searches per second of the paper (20 restarts of $4n$ iterations). On `queiroga2020/Capacity20/100/100J_s10_r10/1.dat`, the local search found a solution of value 35898, below the BKS (35930).

## Tests

* `test/algorithms/local_search_test.cpp` with the instances of `test/algorithms/brute_force_test.txt` (1,085 instances, all the environments and objectives except the throughput): with the seed 0 and 100 iterations, the local search finds the optimal value of the certificate on all of them. The random numbers depend on the standard library (the C++ distributions and `std::shuffle` are not specified exactly), so the test is run with a margin: see the verification above.

## References

* E. Queiroga, R. G. S. Pinheiro, Q. Christ, A. Subramanian and A. A. Pessoa (2020). Iterated local search for single machine total weighted tardiness batch scheduling. *Journal of Heuristics*. [doi:10.1007/s10732-020-09461-x](https://doi.org/10.1007/s10732-020-09461-x)
* J. M. F. Medeiros, A. Subramanian and E. Queiroga (2024). Population-based iterated local search for batch scheduling on parallel machines with incompatible job families, release dates, and tardiness penalties. *Optimization Letters*, 19, 193–210. [doi:10.1007/s11590-024-02116-x](https://doi.org/10.1007/s11590-024-02116-x)
* T. Vidal, T. G. Crainic, M. Gendreau, N. Lahrichi and W. Rei (2012). A hybrid genetic algorithm for multidepot and periodic vehicle routing problems. *Operations Research*, 60(3), 611–624. [doi:10.1287/opre.1120.1048](https://doi.org/10.1287/opre.1120.1048) (the biased fitness of the population)
