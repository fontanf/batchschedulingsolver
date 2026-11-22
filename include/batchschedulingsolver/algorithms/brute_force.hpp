/**
 * Brute force
 *
 * Enumerate all the semi-active schedules: all the sequences of batches, each
 * batch being a subset of the remaining jobs assigned to a machine and
 * started as early as possible. Exact, but exponential: only for small
 * instances. It is kept as simple as possible, to serve as a reference for
 * the other algorithms.
 *
 * Problems:
 * - 1, P, R | p-batch, B_i, size_j, r_j, families | C_max
 * - 1, P, R | p-batch, B_i, size_j, r_j, families | sum w_j F_j
 * - 1, P, R | p-batch, B_i, size_j, r_j, families | sum w_j T_j
 * - 1, P, R | p-batch, B_i, size_j, r_j, families | L_max
 *
 * See doc/brute_force.md.
 */

#pragma once

#include "batchschedulingsolver/algorithm_formatter.hpp"

namespace batchschedulingsolver
{

Output brute_force(
        const Instance& instance,
        const Parameters& parameters = {});

}
