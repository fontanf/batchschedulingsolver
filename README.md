# BatchSchedulingSolver

Research code for batch scheduling problems.

## Compilation

```shell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel && cmake --install build --config Release --prefix install
```

Setup Python environment to use the Python scripts:
```shell
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

Download data files:
```shell
python scripts/download_data.py
```

The data archives are published as GitHub releases and pinned in `data/data.json` (URL and SHA-256 checksum), so each version of the repository downloads the data it has been tested with. Use `--archives tests` to only download the instances used by the unit tests. To publish a new version of an archive, see `scripts/create_data_archives.py`.

The benchmark instances of the literature (where they come from, the problems they were designed for, how they were generated, their format and reference values) are described in [`doc/benchmarks.md`](doc/benchmarks.md).

Run
```shell
./install/bin/batchschedulingsolver  --verbosity-level 1  --input ./data/alfieri2021/inst_10_1_1.dat --format alfieri2021  --algorithm greedy-longest-processing-time  --certificate solution.json
```

Visualize solution:
```shell
python scripts/visualize.py solution.json
```

## Implemented algorithms

Each algorithm has a page in [`doc/`](doc/) describing it, the problems it supports, its model and its limits.

The algorithms planned next, in particular state-of-the-art ones, are listed in [`doc/roadmap.md`](doc/roadmap.md).

### All problems

* [Brute force](doc/brute_force.md) `--algorithm brute-force` (exact, small instances only; gives the optimal solutions of the tests)

### Single machine

#### Objective makespan

$1 \mid \text{p-batch}, B, \text{size}\_j \mid C_{\max}$
* [Greedy, longest processing-time](doc/greedy_longest_processing_time.md) `--algorithm greedy-longest-processing-time` (heuristic, optimal with identical sizes)
* [MILP, arc-flow](doc/milp_arc_flow.md) `--algorithm milp-arc-flow` (exact)
* [MILP, two-index](doc/milp_rank_based_two_index.md) `--algorithm milp-rank-based-two-index-no-starts` (exact)
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index-no-starts` (exact)
* [Tree search](doc/tree_search.md) `--algorithm tree-search` (exact; beam search on large instances)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$1 \mid \text{p-batch}, B, \text{size}\_j, r_j \mid C_{\max}$
* [MILP, two-index](doc/milp_rank_based_two_index.md) `--algorithm milp-rank-based-two-index` (exact)
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index` (exact)
* [Tree search](doc/tree_search.md) `--algorithm tree-search` (exact; beam search on large instances)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$1 \mid \text{p-batch}, B, \text{size}\_j, r_j, \text{incompatible} \mid C_{\max}$
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

#### Objective total flow time

$1 \mid \text{p-batch}, B, \text{size}\_j \mid \sum C_j$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index` (exact)
* [Tree search](doc/tree_search.md) `--algorithm tree-search` (exact; beam search on large instances)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$1 \mid \text{p-batch}, B, \text{size}\_j \mid \sum w_j C_j$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index` (exact)
* [Tree search](doc/tree_search.md) `--algorithm tree-search` (exact; beam search on large instances)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$1 \mid \text{p-batch}, B, \text{size}\_j, r_j \mid \sum w_j C_j$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index` (exact)
* [Tree search](doc/tree_search.md) `--algorithm tree-search` (exact; beam search on large instances)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$1 \mid \text{p-batch}, B, \text{size}\_j, \text{incompatible} \mid \sum w_j C_j$
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$1 \mid \text{p-batch}, B, \text{size}\_j, r_j, \text{incompatible} \mid \sum w_j C_j$
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

#### Objective total tardiness

$1 \mid \text{p-batch}, B, \text{size}\_j, r_j \mid \sum w_j T_j$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index` (exact)
* [Tree search](doc/tree_search.md) `--algorithm tree-search` (exact; beam search on large instances)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$1 \mid \text{p-batch}, B, \text{size}\_j, r_j, \text{incompatible} \mid \sum w_j T_j$
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

#### Objective maximum lateness

$1 \mid \text{p-batch}, B, \text{size}\_j, r_j, \text{incompatible} \mid L_{\max}$
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

### Parallel machines

#### Objective makespan

$P_m \mid \text{p-batch}, B, \text{size}\_j \mid C_{\max}$
* [MILP, two-index](doc/milp_rank_based_two_index.md) `--algorithm milp-rank-based-two-index-no-starts` (exact)
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index-no-starts` (exact)
* [Tree search](doc/tree_search.md) `--algorithm tree-search` (exact; beam search on large instances)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$P_m \mid \text{p-batch}, B, \text{size}\_j, r_j \mid C_{\max}$
* [MILP, two-index](doc/milp_rank_based_two_index.md) `--algorithm milp-rank-based-two-index` (exact)
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index` (exact)
* [Tree search](doc/tree_search.md) `--algorithm tree-search` (exact; beam search on large instances)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$P_m \mid \text{p-batch}, B_i, \text{size}\_j \mid C_{\max}$
* [MILP, two-index](doc/milp_rank_based_two_index.md) `--algorithm milp-rank-based-two-index-no-starts` (exact)
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index-no-starts` (exact)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$P_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j \mid C_{\max}$
* [MILP, two-index](doc/milp_rank_based_two_index.md) `--algorithm milp-rank-based-two-index` (exact)
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index` (exact)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$R_m \mid \text{p-batch}, B_i, \text{size}\_j \mid C_{\max}$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index-no-starts` (exact)
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index` (exact)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$R_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j \mid C_{\max}$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index` (exact)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$R_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j, \text{incompatible} \mid C_{\max}$
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

#### Objective total flow time

$P_m \mid \text{p-batch}, B, \text{size}\_j \mid \sum C_j$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index` (exact)
* [Tree search](doc/tree_search.md) `--algorithm tree-search` (exact; beam search on large instances)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$P_m \mid \text{p-batch}, B, \text{size}\_j, r_j \mid \sum C_j$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index` (exact)
* [Tree search](doc/tree_search.md) `--algorithm tree-search` (exact; beam search on large instances)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$P_m \mid \text{p-batch}, B, \text{size}\_j, r_j \mid \sum w_j C_j$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index` (exact)
* [Tree search](doc/tree_search.md) `--algorithm tree-search` (exact; beam search on large instances)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$P_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j \mid \sum C_j$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index` (exact)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$P_m \mid \text{p-batch}, B_i, \text{size}\_j \mid \sum w_j C_j$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index` (exact)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$R_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j \mid \sum w_j C_j$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index` (exact)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$R_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j, \text{incompatible} \mid \sum w_j C_j$
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

#### Objective total tardiness

$P_m \mid \text{p-batch}, B, \text{size}\_j, r_j \mid \sum w_j T_j$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index` (exact)
* [Tree search](doc/tree_search.md) `--algorithm tree-search` (exact; beam search on large instances)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$P_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j \mid \sum w_j T_j$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index` (exact)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$R_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j \mid \sum w_j T_j$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `--algorithm milp-rank-based-three-index` (exact)
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$P_m \mid \text{p-batch}, \text{incompatible} \mid \sum w_j T_j$
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$P_m \mid \text{p-batch}, \text{incompatible}, r_j \mid \sum w_j T_j$
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

$R_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j, \text{incompatible} \mid \sum w_j T_j$
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

#### Objective maximum lateness

$R_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j, \text{incompatible} \mid L_{\max}$
* [Local search](doc/local_search.md) `--algorithm local-search` (heuristic)

The tests, and the generation of the optimal solutions of the test instances, are described in [`doc/tests.md`](doc/tests.md).

