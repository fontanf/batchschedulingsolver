#include "tests.hpp"
#include "batchschedulingsolver/algorithms/local_search.hpp"

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
}

// The local search is a heuristic: with a fixed seed and number of
// iterations, it is tested on the small instances solved by the brute force,
// on which it finds the optimal solutions.
INSTANTIATE_TEST_SUITE_P(
        LocalSearch,
        ExactAlgorithmTest,
        testing::ValuesIn(get_test_params(
                {
                    [](const Instance& instance)
                    {
                        std::mt19937_64 generator(0);
                        LocalSearchParameters parameters;
                        parameters.maximum_number_of_iterations = 100;
                        return local_search(instance, generator, nullptr, parameters);
                    },
                },
                {
                    get_test_instance_paths(get_path({"test", "algorithms", "brute_force_test.txt"})),
                })));
