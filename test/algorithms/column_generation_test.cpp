#include "tests.hpp"
#include "batchschedulingsolver/algorithms/column_generation.hpp"

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
    EXPECT_EQ(output.solution.feasible(), true);
    EXPECT_EQ(output.solution.objective_value(), solution.objective_value());
    // The bound of the column generation is valid.
    EXPECT_LE(output.bound(), solution.objective_value());
}

// The single machine total (weighted) flow time instances without release
// dates solved by the brute force.
INSTANTIATE_TEST_SUITE_P(
        ColumnGeneration,
        ExactAlgorithmTest,
        testing::ValuesIn(get_test_params(
                {
                    [](const Instance& instance)
                    {
                        ColumnGenerationParameters parameters;
                        return column_generation(instance, parameters);
                    },
                },
                {
                    get_test_instance_paths(get_path({"test", "algorithms", "column_generation_test.txt"})),
                })));
