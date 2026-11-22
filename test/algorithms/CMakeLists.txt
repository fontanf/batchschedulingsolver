add_library(BatchSchedulingSolver_tests)
target_sources(BatchSchedulingSolver_tests PRIVATE
    tests.cpp)
target_link_libraries(BatchSchedulingSolver_tests PUBLIC
    BatchSchedulingSolver_batchschedulingsolver
    Boost::filesystem
    GTest::gtest_main)
add_library(BatchSchedulingSolver::tests ALIAS BatchSchedulingSolver_tests)

add_executable(BatchSchedulingSolver_greedy_longest_processing_time_test)
target_sources(BatchSchedulingSolver_greedy_longest_processing_time_test PRIVATE
    greedy_longest_processing_time_test.txt
    greedy_longest_processing_time_test.cpp)
target_link_libraries(BatchSchedulingSolver_greedy_longest_processing_time_test
    BatchSchedulingSolver_tests
    BatchSchedulingSolver_greedy_longest_processing_time
    GTest::gtest_main)
gtest_discover_tests(BatchSchedulingSolver_greedy_longest_processing_time_test
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    PROPERTIES TIMEOUT 3600)

add_executable(BatchSchedulingSolver_milp_rank_based_test)
target_sources(BatchSchedulingSolver_milp_rank_based_test PRIVATE
    milp_rank_based_three_index_no_starts_test.txt
    milp_rank_based_two_index_no_starts_test.txt
    milp_rank_based_two_index_test.txt
    milp_rank_based_test.cpp)
target_link_libraries(BatchSchedulingSolver_milp_rank_based_test
    BatchSchedulingSolver_tests
    BatchSchedulingSolver_milp_rank_based
    GTest::gtest_main)
gtest_discover_tests(BatchSchedulingSolver_milp_rank_based_test
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    PROPERTIES TIMEOUT 3600)

add_executable(BatchSchedulingSolver_brute_force_test)
target_sources(BatchSchedulingSolver_brute_force_test PRIVATE
    brute_force_test.txt
    brute_force_test.cpp)
target_link_libraries(BatchSchedulingSolver_brute_force_test
    BatchSchedulingSolver_tests
    BatchSchedulingSolver_brute_force
    GTest::gtest_main)
gtest_discover_tests(BatchSchedulingSolver_brute_force_test
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    PROPERTIES TIMEOUT 3600)

add_executable(BatchSchedulingSolver_tree_search_test)
target_sources(BatchSchedulingSolver_tree_search_test PRIVATE
    tree_search_test.txt
    tree_search_test.cpp)
target_link_libraries(BatchSchedulingSolver_tree_search_test
    BatchSchedulingSolver_tests
    BatchSchedulingSolver_tree_search
    GTest::gtest_main)
gtest_discover_tests(BatchSchedulingSolver_tree_search_test
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    PROPERTIES TIMEOUT 3600)

add_executable(BatchSchedulingSolver_local_search_test)
target_sources(BatchSchedulingSolver_local_search_test PRIVATE
    local_search_test.cpp)
target_link_libraries(BatchSchedulingSolver_local_search_test
    BatchSchedulingSolver_tests
    BatchSchedulingSolver_local_search
    GTest::gtest_main)
gtest_discover_tests(BatchSchedulingSolver_local_search_test
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    PROPERTIES TIMEOUT 3600)
