#include "tests.hpp"
#include "batchschedulingsolver/algorithms/milp_rank_based.hpp"

using namespace batchschedulingsolver;

TEST_P(ExactAlgorithmTest, ExactAlgorithm)
{
    TestParams test_params = GetParam();
    const Instance instance = get_instance(test_params.files);
    const Solution solution = get_solution(instance, test_params.files);
    auto output = test_params.algorithm(instance);
    std::cout << std::endl;
    std::cout << "Reference solution" << std::endl;
    std::cout << "------------------" << std::endl;
    solution.format(std::cout, 1);
    EXPECT_EQ(output.solution.objective_value(), solution.objective_value());
}

INSTANTIATE_TEST_SUITE_P(
        MilpRankBasedThreeIndexNoStarts,
        ExactAlgorithmTest,
        testing::ValuesIn(get_test_params(
                {
                    [](const Instance& instance)
                    {
                        return milp_rank_based_three_index_no_starts(instance);
                    },
                },
                {
                    get_test_instance_paths(get_path({"test", "algorithms", "milp_rank_based_three_index_no_starts_test.txt"})),
                })));

INSTANTIATE_TEST_SUITE_P(
        MilpRankBasedThreeIndex,
        ExactAlgorithmTest,
        testing::ValuesIn(get_test_params(
                {
                    [](const Instance& instance)
                    {
                        return milp_rank_based_three_index(instance);
                    },
                },
                {
                    get_test_instance_paths(get_path({"test", "algorithms", "milp_rank_based_three_index_test.txt"})),
                })));

INSTANTIATE_TEST_SUITE_P(
        MilpRankBasedTwoIndexNoStarts,
        ExactAlgorithmTest,
        testing::ValuesIn(get_test_params(
                {
                    [](const Instance& instance)
                    {
                        return milp_rank_based_two_index_no_starts(instance);
                    },
                },
                {
                    get_test_instance_paths(get_path({"test", "algorithms", "milp_rank_based_two_index_no_starts_test.txt"})),
                })));

INSTANTIATE_TEST_SUITE_P(
        MilpRankBasedTwoIndex,
        ExactAlgorithmTest,
        testing::ValuesIn(get_test_params(
                {
                    [](const Instance& instance)
                    {
                        return milp_rank_based_two_index(instance);
                    },
                },
                {
                    get_test_instance_paths(get_path({"test", "algorithms", "milp_rank_based_two_index_test.txt"})),
                })));
