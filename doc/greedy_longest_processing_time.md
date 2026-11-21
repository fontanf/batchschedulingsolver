# Greedy, longest processing time first (LPT)

`--algorithm greedy-longest-processing-time`

Source: [`include/batchschedulingsolver/algorithms/greedy_longest_processing_time.hpp`](../include/batchschedulingsolver/algorithms/greedy_longest_processing_time.hpp), [`src/algorithms/greedy_longest_processing_time.cpp`](../src/algorithms/greedy_longest_processing_time.cpp)

## Algorithm

1. Sort the jobs in non-increasing order of processing time. The sort is not stable: the order of jobs with equal processing times is unspecified.
2. Next fit: take the jobs in this order and add each one to the current batch if it fits, i.e. if the size of the batch plus the size of the job doesn't exceed the capacity $B$ of the machine. Otherwise, close the current batch and open a new one containing the job.
3. The batches are processed in the order in which they were created, without idle time: each batch starts when the previous one completes, and its processing time is the processing time of its first job (the longest one).

Only the current batch is considered in step 2 (next fit). The classical FFLPT heuristic of Uzsoy (1994) uses first fit instead: each job goes into the first batch in which it fits.

Complexity: $O(n \log n)$.

## Supported problems

$1 \mid \text{p-batch}, B, \text{size}\_j \mid C_{\max}$

Families are ignored. The algorithm throws `std::invalid_argument` if the objective is not the makespan, if the instance has several machines or if a job has a non-zero release date.

## Optimality

| Problem | Result |
| --- | --- |
| $1 \mid \text{p-batch}, B, \text{size}\_j = s \mid C_{\max}$ | Optimal. With identical sizes, each batch holds $\lfloor B / s \rfloor$ jobs, and the algorithm is the full-batch LPT rule, which is optimal for $1 \mid \text{p-batch}, b < n \mid C_{\max}$ (Brucker et al., 1998). |
| $1 \mid \text{p-batch}, B, \text{size}\_j \mid C_{\max}$ | Heuristic. The problem is strongly NP-hard, since it contains bin packing (all processing times equal) (Uzsoy, 1994). |

With non-identical sizes, next fit can open more batches than first fit. For example, with $B = 10$, equal processing times and sizes $(6, 5, 6, 5, \ldots)$ in the sorted order ($2k$ jobs), next fit opens one batch per job ($2k$ batches), while first fit pairs the jobs of size 5, which is optimal ($k + \lceil k/2 \rceil$ batches): the makespan ratio tends to $4/3$.

## Proved optimality

When all the jobs have the same size, the algorithm sets the makespan bound to the value of its solution, and the output reports `"Optimal": true`.

## Tests

* `test/algorithms/greedy_longest_processing_time_test.cpp`: the value of the solution is compared to the reference value on the instances of [`test/algorithms/greedy_longest_processing_time_test.txt`](../test/algorithms/greedy_longest_processing_time_test.txt), generated from `data/test_makespan_identical_sizes.txt` (single machine, identical sizes, 50 instances).

## References

* R. Uzsoy (1994). Scheduling a single batch processing machine with non-identical job sizes. *International Journal of Production Research*, 32(7), 1615–1635. [doi:10.1080/00207549408957026](https://doi.org/10.1080/00207549408957026)
* P. Brucker, A. Gladky, H. Hoogeveen, M. Y. Kovalyov, C. N. Potts, T. Tautenhahn and S. L. van de Velde (1998). Scheduling a batching machine. *Journal of Scheduling*, 1(1), 31–54.
