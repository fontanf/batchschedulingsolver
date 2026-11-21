#include "tests.hpp"
#include "batchschedulingsolver/algorithms/tree_search.hpp"

using namespace batchschedulingsolver;

TEST_P(ExactAlgorithmTest, ExactAlgorithm)
{
    TestParams test_params = GetParam();
    const Instance instance = get_instance(test_params.files);
    auto output = test_params.algorithm(instance);
    // The reference values of 'tree_search_test.txt' are computed by brute
    // force (scripts/solve_test_data.py --criterion objective).
    EXPECT_EQ(output.solution.feasible(), true);
    EXPECT_EQ(output.solution.objective_value(), test_params.files.value);
    EXPECT_EQ(output.bound(), test_params.files.value);
}

INSTANTIATE_TEST_SUITE_P(
        TreeSearch,
        ExactAlgorithmTest,
        testing::ValuesIn(get_test_params(
                {
                    [](const Instance& instance)
                    {
                        return tree_search(instance);
                    },
                },
                {
                    get_test_instance_paths(get_path({"test", "algorithms", "tree_search_test.txt"})),
                })));
