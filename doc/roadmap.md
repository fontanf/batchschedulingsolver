# Roadmap

Algorithms planned for this repository, in particular state-of-the-art ones, grouped by problem. Each entry gives the reference and what to implement from it.

Status: **planned**, **suggested**, **in progress** or **done** (with the `--algorithm` option).

All the problems are parallel batching (p-batch) problems with non-identical job sizes: the processing time of a batch is the largest processing time of its jobs, and the total size of its jobs doesn't exceed the capacity of the machine. Survey: J. W. Fowler and L. Mönch (2021). A survey of scheduling with parallel batch (p-batch) processing. *European Journal of Operational Research*. [doi:10.1016/j.ejor.2021.06.012](https://doi.org/10.1016/j.ejor.2021.06.012)

## Several problems

* **Done**: rank-based MILPs of R. S. Trindade, O. C. B. de Araújo, M. H. C. Fampa and F. M. Müller (2018). Modelling and symmetry breaking in scheduling problems on batch processing machines. *International Journal of Production Research*, 56(22), 7031–7048. [doi:10.1080/00207543.2018.1424371](https://doi.org/10.1080/00207543.2018.1424371)
  * Done: three-index models, `--algorithm milp-rank-based-three-index-no-starts` and `milp-rank-based-three-index`, generalized to unrelated machines and to $\sum w_j C_j$ and $\sum w_j T_j$. See [`milp_rank_based_three_index.md`](milp_rank_based_three_index.md).
  * Done: symmetry-broken two-index models, `--algorithm milp-rank-based-two-index-no-starts` and `milp-rank-based-two-index`. See [`milp_rank_based_two_index.md`](milp_rank_based_two_index.md).
* **Done**: brute force, `--algorithm brute-force`, for all the variants of the repository except the throughput: enumeration of the semi-active schedules, for small instances. It computes the optimal solutions of the test instances. See [`brute_force.md`](brute_force.md).
* **Done**: tree search, `--algorithm tree-search`, for $C_{\max}$, $\sum w_j F_j$ and $\sum w_j T_j$ on one machine and identical parallel machines, with release dates: forward branching with dominances, a dynamic programming algorithm over the subsets of jobs with an unbounded queue, and a beam search otherwise. Generalized from the branching scheme of the treesearchsolver examples. See [`tree_search.md`](tree_search.md).
* **Suggested**: optimal batching of a fixed job sequence by dynamic programming, the building block of most of the sequence-based metaheuristics below (Chou and Wang, 2008; Wang, 2011; Rafiee Parsa et al., 2016; Queiroga et al., 2020). For $\sum w_j T_j$ with non-identical sizes, this problem is NP-hard, and Queiroga et al. (2020) give a pseudo-polynomial algorithm.

## Single machine, makespan

$1 \mid \text{p-batch}, B, \text{size}\_j \mid C_{\max}$
* **Done**: greedy LPT, `--algorithm greedy-longest-processing-time`, optimal with identical sizes. See [`greedy_longest_processing_time.md`](greedy_longest_processing_time.md).
* **Done**: R. S. Trindade, O. C. B. de Araújo and M. Fampa (2021). Arc-flow approach for single batch-processing machine scheduling. *Computers & Operations Research*, 134, 105394. [doi:10.1016/j.cor.2021.105394](https://doi.org/10.1016/j.cor.2021.105394)
  * Done: arc-flow MILP, `--algorithm milp-arc-flow`. See [`milp_arc_flow.md`](milp_arc_flow.md).
  * Missing: the benchmark of the paper (instances of Chen et al., 2011, and random instances with many identical jobs).
* **Planned**: S. Zheng, N. Xie, Q. Wu and C. Liu (2025). Novel mathematical formulations for parallel-batching processing machine scheduling problems. *Computers & Operations Research*, 173, 106859. [doi:10.1016/j.cor.2024.106859](https://doi.org/10.1016/j.cor.2024.106859)
  * New formulations with valid inequalities, and a framework using dual information and bounding techniques, for single and parallel machines. Reported as the state of the art, proving the optimality of instances of the literature that were open.
* **Suggested**: İ. Muter (2020). Exact algorithms to minimize makespan on single and parallel batch processing machines. *European Journal of Operational Research*, 285(2), 470–483. [doi:10.1016/j.ejor.2020.01.065](https://doi.org/10.1016/j.ejor.2020.01.065)
  * Column-and-cut generation for the single machine problem; its bound is reused for the parallel machine problem (see below).
* **Suggested**: N. Rafiee Parsa, B. Karimi and A. Husseinzadeh Kashan (2010). A branch and price algorithm to minimize makespan on a single batch processing machine with non-identical job sizes. *Computers & Operations Research*, 37(10), 1720–1730. [doi:10.1016/j.cor.2009.12.007](https://doi.org/10.1016/j.cor.2009.12.007)
  * Set-partitioning formulation (a column is a batch) solved by branch-and-price, with a column generation lower bound and a heuristic for the initial columns.
* **Suggested**: A. Husseinzadeh Kashan and O. Ozturk (2022). Improved MILP formulation equipped with valid inequalities for scheduling a batch processing machine with non-identical job sizes. *Omega*, 102673. [doi:10.1016/j.omega.2022.102673](https://doi.org/10.1016/j.omega.2022.102673)
  * Valid inequalities from properties of the optimal solutions, which could be added to the rank-based MILPs.
* **Suggested**: A. Husseinzadeh Kashan and O. Ozturk (2025). A new class of lower bounds for scheduling a batch processing machine to minimize makespan. *European Journal of Operational Research*, 327(3), 754–775. [doi:10.1016/j.ejor.2025.05.047](https://doi.org/10.1016/j.ejor.2025.05.047)
  * Combinatorial lower bounds $LB_m$ and $ILB_m$ (worst-case ratio at least 4/7, against 1/2 for the previous bound), cheap to compute for $m \le 3$. Useful to prove the optimality of the heuristic solutions.
* **Suggested**: L. Dupont and C. Dhaenens-Flipo (2002). Minimizing the makespan on a batch machine with non-identical job sizes: an exact procedure. *Computers & Operations Research*, 29(7), 807–819. [doi:10.1016/S0305-0548(00)00078-2](https://doi.org/10.1016/S0305-0548(00)00078-2)
  * Branch-and-bound with dominance properties, also used as a heuristic: a classical reference to compare with.
* **Suggested**: R. Uzsoy (1994). Scheduling a single batch processing machine with non-identical job sizes. *International Journal of Production Research*, 32(7), 1615–1635. [doi:10.1080/00207549408957026](https://doi.org/10.1080/00207549408957026)
  * FFLPT (first fit instead of the next fit of `greedy-longest-processing-time`), the baseline heuristic of most papers.
* Benchmark: the instances of H. Chen, B. Du and G. Q. Huang (2011). Scheduling a batch processing machine with non-identical job sizes: a clustering perspective. *International Journal of Production Research*, 49(19), 5755–5778. [doi:10.1080/00207543.2010.512620](https://doi.org/10.1080/00207543.2010.512620). They are in `data/chen2011` (XML files), but there is no reader for their format yet.

## Single machine, makespan, release dates

$1 \mid \text{p-batch}, B, \text{size}\_j, r_j \mid C_{\max}$
* **Done**: two-index rank-based MILP with batch starts, `--algorithm milp-rank-based-two-index`.
* **Suggested**: S. Zhou, H. Chen, R. Xu and X. Li (2014). Minimising makespan on a single batch processing machine with dynamic job arrivals and non-identical job sizes. *International Journal of Production Research*, 52(8), 2258–2274. [doi:10.1080/00207543.2013.854937](https://doi.org/10.1080/00207543.2013.854937)
  * Construction heuristics, compared with FFLPT-ERT, a hybrid genetic algorithm and an ant colony optimization; two lower bounds.
* **Suggested**: S. Li, G. Li, X. Wang and Q. Liu (2005). Minimizing makespan on a single batching machine with release times and non-identical job sizes. *Operations Research Letters*, 33(2), 157–164. [doi:10.1016/j.orl.2004.04.009](https://doi.org/10.1016/j.orl.2004.04.009)
  * $(2 + \varepsilon)$-approximation algorithm.

## Parallel machines, makespan

$P_m \mid \text{p-batch}, B, \text{size}\_j \mid C_{\max}$
* **Done**: rank-based MILPs, see above.
* **Suggested**: İ. Muter (2020), see above.
  * Reformulation based on a decomposition in two levels: the single machine problem, solved by column-and-cut generation, gives a lower bound for the parallel machine problem, which is then solved exactly. Reported as the first exact algorithm for this problem other than formulations solved by off-the-shelf solvers.
* **Suggested**: R. S. Trindade, O. C. B. de Araújo, M. Fampa, et al. (2020). Arc-flow approach for parallel batch processing machine scheduling with non-identical job sizes. In *Combinatorial Optimization (ISCO 2020)*, 179–190. [doi:10.1007/978-3-030-53262-8_15](https://doi.org/10.1007/978-3-030-53262-8_15)
  * Extension of `milp-arc-flow` to parallel machines.
* **Planned**: S. Zheng et al. (2025), see above.

$P_m$, $R_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j \mid C_{\max}$
* **Done**: three-index rank-based MILP, `--algorithm milp-rank-based-three-index`.
* **Suggested**: J. E. C. Arroyo and J. Y.-T. Leung (2017). An effective iterated greedy algorithm for scheduling unrelated parallel batch machines with non-identical capacities and unequal ready times. *Computers & Industrial Engineering*, 105, 84–100. [doi:10.1016/j.cie.2016.12.038](https://doi.org/10.1016/j.cie.2016.12.038)
  * Iterated greedy, lower bound and MILP, exactly for the problem of `milp-rank-based-three-index` (unrelated machines, machine-dependent capacities, release dates).
* **Suggested**: J. E. C. Arroyo and J. Y.-T. Leung (2017). Scheduling unrelated parallel batch processing machines with non-identical job sizes and unequal ready times. *Computers & Operations Research*, 78, 117–128. [doi:10.1016/j.cor.2016.08.015](https://doi.org/10.1016/j.cor.2016.08.015)
  * First-fit and best-fit earliest ready time heuristics and a lower bound: baselines for the previous entry.
* **Suggested**: S. Zhou, H. Chen and X. Li (2017). Distance matrix based heuristics to minimize makespan of parallel batch processing machines with arbitrary job sizes and release times. *Applied Soft Computing*, 52, 630–641. [doi:10.1016/j.asoc.2016.10.008](https://doi.org/10.1016/j.asoc.2016.10.008)

## Total completion time

$1 \mid \text{p-batch}, B, \text{size}\_j \mid \sum C_j$, $P_m \mid \text{p-batch}, B, \text{size}\_j \mid \sum C_j$
* **Planned**: A. Alfieri, A. Druetto, A. Grosso and F. Salassa (2021). Column generation for minimizing total completion time in a parallel-batching environment. *Journal of Scheduling*. [doi:10.1007/s10951-021-00703-9](https://doi.org/10.1007/s10951-021-00703-9)
  * Graph-based formulation with an exponential number of variables, solved by column generation: a lower bound that strongly dominates the previous ones, embedded in a price-and-branch heuristic and an exact branch-and-price. Handles parallel machines.
  * Benchmark: the instances of the paper, in `data/alfieri2021` (`--format alfieri2021`).
* **Suggested**: N. Rafiee Parsa, B. Karimi and S. M. Moattar Husseini (2016). Minimizing total flow time on a batch processing machine using a hybrid max–min ant system. *Computers & Industrial Engineering*, 99, 372–381. [doi:10.1016/j.cie.2016.06.008](https://doi.org/10.1016/j.cie.2016.06.008)
  * Max–min ant system on the job sequence, with dynamic programming for the optimal batching of the sequence and a local search.
* **Suggested**: V. Chandru, C.-Y. Lee and R. Uzsoy (1993). Minimizing total completion time on batch processing machines. *International Journal of Production Research*, 31(9), 2097–2121. [doi:10.1080/00207549308956847](https://doi.org/10.1080/00207549308956847)
  * Exact procedure and heuristics for identical sizes: a classical baseline.

$P_m \mid \text{p-batch}, B, \text{size}\_j, r_j \mid \sum C_j$
* **Suggested**: O. Ozturk (2020). A truncated column generation algorithm for the parallel batch scheduling problem to minimize total flow time. *European Journal of Operational Research*, 286(2), 432–443. [doi:10.1016/j.ejor.2020.03.044](https://doi.org/10.1016/j.ejor.2020.03.044)
  * Time-indexed column generation (a column is a batch and its start time), with rounding and a second model to assign the batches to the machines.
* **Suggested**: H. Zhang, K. Li, Z. Jia and C. Chu (2023). Minimizing total completion time on non-identical parallel batch machines with arbitrary release times using ant colony optimization. *European Journal of Operational Research*. [doi:10.1016/j.ejor.2023.02.015](https://doi.org/10.1016/j.ejor.2023.02.015)
  * Elite ant system with local search (MEASL) for machines with different capacities.

## Total weighted completion time

$1 \mid \text{p-batch}, B, \text{size}\_j \mid \sum w_j C_j$
* **Suggested**: A. Druetto and A. Grosso (2021). Column generation and rounding heuristics for minimizing the total weighted completion time on a single batching machine. *Computers & Operations Research*, 105639. [doi:10.1016/j.cor.2021.105639](https://doi.org/10.1016/j.cor.2021.105639)
  * Weighted extension of the graph-based formulation of Alfieri et al. (2021): very tight column generation bound and rounding heuristics within a few percent of the optimum.

$P_m \mid \text{p-batch}, B_i, \text{size}\_j \mid \sum w_j C_j$
* **Suggested**: H. Zhang, Z. Jia and K. Li (2020). Ant colony optimization algorithm for total weighted completion time minimization on non-identical batch machines. *Computers & Operations Research*, 117, 104889. [doi:10.1016/j.cor.2020.104889](https://doi.org/10.1016/j.cor.2020.104889)
  * Ant system and max–min ant system, with a lower bound, for machines with different capacities.

## Total weighted tardiness

$1 \mid \text{p-batch}, B, \text{size}\_j, r_j \mid \sum w_j T_j$
* **Done**: E. Queiroga, R. G. S. Pinheiro, Q. Christ, A. Subramanian and A. A. Pessoa (2020). Iterated local search for single machine total weighted tardiness batch scheduling. *Journal of Heuristics*. [doi:10.1007/s10732-020-09461-x](https://doi.org/10.1007/s10732-020-09461-x)
  * Iterated local search with five neighborhoods and their efficient evaluation; pseudo-polynomial dynamic programming for the optimal batching of a fixed sequence.
  * Done: the neighborhoods, the perturbations and the constructive heuristic, within the population-based framework of Medeiros et al. (2024), `--algorithm local-search`, generalized to unrelated machines, machine-dependent capacities, families and the other objectives. See [`local_search.md`](local_search.md).
  * Missing: the dynamic programming for the optimal batching of a fixed sequence.
  * Benchmark: the instances of the paper, with optimal solutions for some of them, in `data/queiroga2020` (`--format queiroga2020`).
* **Suggested**: A. A. Pessoa, T. Bulhões, V. Nesello and A. Subramanian (2022). Exact approaches for single machine total weighted tardiness batch scheduling. *INFORMS Journal on Computing*. [doi:10.1287/ijoc.2021.1133](https://doi.org/10.1287/ijoc.2021.1133)
  * Time-indexed formulation (TIF), which already outperforms the rank-based formulation, and time-size-indexed formulation (TSIF) solved by column generation, with a bucket graph labeling algorithm and ng-sets for the pricing: the state-of-the-art exact method.
* **Suggested**: H.-M. Wang (2011). Solving single batch-processing machine problems using an iterated heuristic. *International Journal of Production Research*, 49(14), 4245–4261. [doi:10.1080/00207543.2010.518995](https://doi.org/10.1080/00207543.2010.518995)

$P_m$, $R_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j \mid \sum w_j T_j$
* **Done**: three-index rank-based MILP, `--algorithm milp-rank-based-three-index`.
* **Suggested**: F. D. Chou (2013). Minimising the total weighted tardiness for non-identical parallel batch processing machines with job release times and non-identical job sizes. *European Journal of Industrial Engineering*, 7(5), 529. [doi:10.1504/ejie.2013.057380](https://doi.org/10.1504/ejie.2013.057380)

## Incompatible job families

The instances have families (`family_id`), but only the brute force and the local search support incompatible families (a batch can only contain jobs of the same family).

* **Suggested**: F. Yang, M. Davari, W. Wei, B. Hermans and R. Leus (2022). Scheduling a single parallel-batching machine with non-identical job sizes and incompatible job families. *European Journal of Operational Research*. [doi:10.1016/j.ejor.2022.03.027](https://doi.org/10.1016/j.ejor.2022.03.027)
  * $1 \mid \text{p-batch}, B, \text{size}\_j, \text{incompatible} \mid \sum w_j C_j$: assignment-based, time-indexed and set-partitioning formulations, branch-and-price and a column-generation-based heuristic.
* **Done**: J. M. F. Medeiros, A. Subramanian and E. Queiroga (2024). Population-based iterated local search for batch scheduling on parallel machines with incompatible job families, release dates, and tardiness penalties. *Optimization Letters*. [doi:10.1007/s11590-024-02116-x](https://doi.org/10.1007/s11590-024-02116-x)
  * $P_m \mid \text{p-batch}, \text{incompatible}, r_j \mid \sum w_j T_j$: population-based ILS, close to the one of shopschedulingsolver.
  * Done: `--algorithm local-search`, see above.
  * Missing: the time window decomposition heuristic used for the initial solutions (ATC and BATC-II rules), and the benchmark of the paper (Chiang et al., 2010; not in `data/`).
* **Suggested**: C. Almeder and L. Mönch (2011). Metaheuristics for scheduling jobs with incompatible families on parallel batching machines. *Journal of the Operational Research Society*, 62(12), 2083–2096. [doi:10.1057/jors.2010.186](https://doi.org/10.1057/jors.2010.186)
  * Variable neighborhood search, better than ACO and genetic algorithms, for $P_m \mid \text{p-batch}, \text{incompatible} \mid \sum w_j T_j$.
