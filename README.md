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

Run
```shell
./install/bin/batchschedulingsolver  --verbosity-level 1  --input ./data/alfieri2021/inst_10_1_1.dat --format alfieri2021  --algorithm greedy-longest-processing-time  --certificate solution.json
```

Visualize solution:
```shell
python scripts/visualize.py solution.json
```

## Implemented algorithms

$1 \mid \text{p-batch}, \text{size}_j \mid C_{\max}$
* Greedy, longest processing-time `greedy-longest-processing-time`
* MILP, rank-based `milp-rank-based`


Generate test instances list for each algorithm:
```shell
python scripts/solve_test_data.py  --algorithm greedy-longest-processing-time  --output test/algorithms/greedy_longest_processing_time_test.txt  --instances \
        data/test_makespan_identical_sizes.txt
python scripts/solve_test_data.py  --algorithm milp-rank-based  --output test/algorithms/milp_rank_based_test.txt  --instances \
        data/test_makespan_identical_sizes.txt \
        data/test_makespan_single.txt
cmake --build build --config Release --target clean
cmake --build build --config Release --parallel  &&  cmake --install build --config Release --prefix install
ctest --parallel --output-on-failure  --test-dir build/test
```
