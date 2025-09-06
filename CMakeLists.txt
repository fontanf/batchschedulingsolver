cmake_minimum_required(VERSION 3.28)

project(BatchSchedulingSolver LANGUAGES CXX)

# Build options.
option(BATCHSCHEDULINGSOLVER_BUILD_MAIN "Build main" ON)
option(BATCHSCHEDULINGSOLVER_BUILD_TEST "Build the unit tests" ON)

option(BATСHSCHEDULINGSOLVER_USE_CBC "Use Cbc" OFF)
option(BATСHSCHEDULINGSOLVER_USE_HIGHS "Use HiGHS" ON)
option(BATСHSCHEDULINGSOLVER_USE_XPRESS "Use FICO Xpress" OFF)

# Avoid FetchContent warning.
cmake_policy(SET CMP0135 NEW)

# Require C++14.
set(CMAKE_CXX_STANDARD 14)

# Enable output of compile commands during generation.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add sub-directories.
add_subdirectory(extern)
add_subdirectory(src)
if(BATCHSCHEDULINGSOLVER_BUILD_TEST)
    add_subdirectory(test)
endif()
