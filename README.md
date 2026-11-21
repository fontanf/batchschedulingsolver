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

$1 \mid \text{p-batch}, \text{size}\_j \mid C_{\max}$
* [Greedy, longest processing-time](doc/greedy_longest_processing_time.md) `greedy-longest-processing-time`

$P_m \mid \text{p-batch}, B_i, \text{size}\_j \mid C_{\max}$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `milp-rank-based-three-index-no-starts`
* [MILP, two-index](doc/milp_rank_based_two_index.md) `milp-rank-based-two-index-no-starts`

$P_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j \mid C_{\max}$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `milp-rank-based-three-index`
* [MILP, two-index](doc/milp_rank_based_two_index.md) `milp-rank-based-two-index`

$R_m \mid \text{p-batch}, B_i, \text{size}\_j \mid C_{\max}$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `milp-rank-based-three-index-no-starts`

$R_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j \mid C_{\max}$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `milp-rank-based-three-index`

$R_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j \mid \sum w_j C_j$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `milp-rank-based-three-index`

$R_m \mid \text{p-batch}, B_i, \text{size}\_j, r_j \mid \sum w_j T_j$
* [MILP, three-index](doc/milp_rank_based_three_index.md) `milp-rank-based-three-index`

$1 \mid \text{p-batch}, B, \text{size}\_j \mid C_{\max}$
* [MILP, arc-flow](doc/milp_arc_flow.md) `milp-arc-flow`

$1$, $P_m \mid \text{p-batch}, B, \text{size}\_j, r_j \mid C_{\max}$, $\sum w_j F_j$, $\sum w_j T_j$ (with or without release dates)
* [Tree search](doc/tree_search.md) `tree-search` (exact, dynamic programming over the subsets of jobs; beam search on large instances)


The tests, and the generation of their reference values, are described in [`doc/tests.md`](doc/tests.md).

