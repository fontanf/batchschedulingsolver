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
        MilpRankBased,
        ExactAlgorithmTest,
        testing::ValuesIn(get_test_params(
                {
                    [](const Instance& instance)
                    {
                        return milp_rank_based(instance);
                    },
                },
                {
                    get_test_instance_paths(get_path({"test", "algorithms", "milp_rank_based_test.txt"})),
                })));
