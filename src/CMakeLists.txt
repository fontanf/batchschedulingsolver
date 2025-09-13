add_library(BatchSchedulingSolver_batchschedulingsolver)
target_sources(BatchSchedulingSolver_batchschedulingsolver PRIVATE
    instance.cpp
    instance_builder.cpp
    solution.cpp
    solution_builder.cpp
    algorithm_formatter.cpp)
target_include_directories(BatchSchedulingSolver_batchschedulingsolver PUBLIC
    ${PROJECT_SOURCE_DIR}/include)
target_include_directories(BatchSchedulingSolver_batchschedulingsolver PRIVATE
    ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(BatchSchedulingSolver_batchschedulingsolver PUBLIC
    OptimizationTools::utils)
add_library(BatchSchedulingSolver::batchschedulingsolver ALIAS BatchSchedulingSolver_batchschedulingsolver)

add_subdirectory(algorithms)

if(BATCHSCHEDULINGSOLVER_BUILD_MAIN)
    add_executable(BatchSchedulingSolver_main)
    target_sources(BatchSchedulingSolver_main PRIVATE
        main.cpp)
    target_link_libraries(BatchSchedulingSolver_main PUBLIC
        BatchSchedulingSolver_batchschedulingsolver
        BatchSchedulingSolver_greedy_longest_processing_time
        Boost::program_options
        Boost::filesystem)
    set_target_properties(BatchSchedulingSolver_main PROPERTIES OUTPUT_NAME "batchschedulingsolver")
    install(TARGETS BatchSchedulingSolver_main)
endif()
