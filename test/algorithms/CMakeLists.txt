add_library(BatchSchedulingSolver_tests)
target_sources(BatchSchedulingSolver_tests PRIVATE
    tests.cpp)
target_link_libraries(BatchSchedulingSolver_tests PUBLIC
    BatchSchedulingSolver_batchschedulingsolver
    Boost::filesystem
    GTest::gtest_main)
add_library(BatchSchedulingSolver::tests ALIAS BatchSchedulingSolver_tests)

add_executable(BatchSchedulingSolver_milp_rank_based_test)
target_sources(BatchSchedulingSolver_milp_rank_based_test PRIVATE
    milp_rank_based_test.txt
    milp_rank_based_test.cpp)
target_link_libraries(BatchSchedulingSolver_milp_rank_based_test
    BatchSchedulingSolver_tests
    BatchSchedulingSolver_milp_rank_based
    GTest::gtest_main)
gtest_discover_tests(BatchSchedulingSolver_milp_rank_based_test
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    PROPERTIES TIMEOUT 3600)
