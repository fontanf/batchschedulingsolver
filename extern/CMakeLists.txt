# Enable FetchContent.
include(FetchContent)

# Fetch boost.
if(BATCHSCHEDULINGSOLVER_BUILD_MAIN OR BATCHSCHEDULINGSOLVER_BUILD_TEST)
    set(BOOST_INCLUDE_LIBRARIES thread filesystem system program_options dynamic_bitset)
    set(BOOST_ENABLE_CMAKE ON)
    FetchContent_Declare(
            Boost
            URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.xz
            EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(Boost)
endif()

# Fetch googletest.
if(BATCHSCHEDULINGSOLVER_BUILD_TEST)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip)
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF)
    FetchContent_MakeAvailable(googletest)
endif()

# Fetch fontanf/optimizationtools.
set(OPTIMIZATIONTOOLS_BUILD_TEST OFF)
FetchContent_Declare(
    optimizationtools
    GIT_REPOSITORY https://github.com/fontanf/optimizationtools.git
    GIT_TAG 6398c6bf14a4a95010c65ec4cb7966aaedc8179d
    #SOURCE_DIR "${PROJECT_SOURCE_DIR}/../optimizationtools/"
    EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(optimizationtools)

# Fetch fontanf/mathoptsolverscmake.
set(MATHOPTSOLVERSCMAKE_USE_CLP ON)
set(MATHOPTSOLVERSCMAKE_USE_CBC ON)
if(BATСHSCHEDULINGSOLVER_USE_HIGHS)
    set(MATHOPTSOLVERSCMAKE_USE_HIGHS ON)
endif()
if(BATСHSCHEDULINGSOLVER_USE_XPRESS)
    set(MATHOPTSOLVERSCMAKE_USE_XPRESS ON)
endif()
FetchContent_Declare(
    mathoptsolverscmake
    GIT_REPOSITORY https://github.com/fontanf/mathoptsolverscmake.git
    GIT_TAG 59fd908038906df649e121f42652a0e8f99cbc67
    #SOURCE_DIR "${PROJECT_SOURCE_DIR}/../mathoptsolverscmake/"
    EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(mathoptsolverscmake)
