cmake_minimum_required(VERSION 3.1)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
option(ENABLE_TESTS "Enable unit tests." OFF)

project(NetKet)

if(ENABLE_TESTS)
   include(CTest) # adds option BUILD_TESTING (default ON)
endif()

include(ExternalProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/)

#
# Dependencies
#

# json
################################################################################
#if (NOT EXISTS "${CMAKE_BINARY_DIR}/External/json/json.hpp")
#    file(DOWNLOAD
#        "https://github.com/nlohmann/json/releases/download/v3.2.0/json.hpp"
#        "External/json/json.hpp")
#endif()
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json
    INTERFACE "/usr/include/nlohmann")

# optional-lite
################################################################################
#if (NOT EXISTS "${CMAKE_BINARY_DIR}/External/optional-lite/nonstd/optional.hpp")
#    file(DOWNLOAD
#        "https://raw.githubusercontent.com/martinmoene/optional-lite/e240fdaaa4071a980926dea4ed5b610aa8fbc538/include/nonstd/optional.hpp"
#        "External/optional-lite/nonstd/optional.hpp")
#endif()
#add_library(optional_lite INTERFACE)
#target_include_directories(optional_lite
#    INTERFACE "${CMAKE_BINARY_DIR}/External/optional-lite")


# span-lite
################################################################################
if (NOT EXISTS "${CMAKE_BINARY_DIR}/External/span-lite/nonstd/span.hpp")
    file(DOWNLOAD
        "https://raw.githubusercontent.com/martinmoene/span-lite/308482109be5df5af750f8ad9d4fb76639a28efb/include/nonstd/span.hpp"
        "External/span-lite/nonstd/span.hpp")
endif()
add_library(span_lite INTERFACE)
target_include_directories(span_lite
    INTERFACE "${CMAKE_BINARY_DIR}/External/span-lite")


# Eigen3
################################################################################
add_library(Eigen3 INTERFACE)
target_include_directories(Eigen3 SYSTEM INTERFACE "/usr/include/eigen3")

# IETL
################################################################################
add_library(IETL INTERFACE)
target_include_directories(IETL SYSTEM INTERFACE "${CMAKE_SOURCE_DIR}/External/ietl")


# MPI
################################################################################
find_package(MPI REQUIRED)

# Catch2
################################################################################
if (BUILD_TESTING)
    add_library(Catch2 INTERFACE)
    target_include_directories(Catch2
        INTERFACE "/usr/include/catch2")
endif()

#
################################################################################

# option(NETKET_ENABLE_BENCHMARKS "Enable building benchmarks." ON)
# option(BENCHMARK_ENABLE_TESTING "" OFF)
# option(BENCHMARK_ENABLE_INSTALL "" OFF)
# option(BENCHMARK_DOWNLOAD_DEPENDENCIES "" ON)
# add_subdirectory(External/benchmark)


if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX  OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
  # Update if necessary
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic -Wextra -Wshadow")
endif()


if("Ninja" STREQUAL ${CMAKE_GENERATOR})
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-fdiagnostics-color" COMPILER_SUPPORTS_-fdiagnostics-color)
    if (COMPILER_SUPPORTS_-fdiagnostics-color)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color")
    endif ()
endif()

#
# NetKet
#

add_library(netket_lib INTERFACE)
target_include_directories(netket_lib INTERFACE NetKet)
target_include_directories(netket_lib SYSTEM INTERFACE ${MPI_CXX_INCLUDE_PATH})
target_link_libraries(netket_lib
    INTERFACE
        ${MPI_CXX_LIBRARIES}
        Eigen3
        nlohmann_json
        #optional_lite
        span_lite
        IETL
)

# Check if we should use native Lapack with Eigen
option(USE_LAPACK "Enable Lapack Support." ON)
if(USE_LAPACK)
    find_package(LAPACK REQUIRED)
    add_definitions(-DEIGEN_USE_BLAS)
    target_link_libraries(netket_lib INTERFACE ${LAPACK_LIBRARIES})
    find_package(LAPACKE)
    if(LAPACKE_FOUND)
    add_definitions(-DEIGEN_USE_LAPACKE)
    target_link_libraries(netket_lib INTERFACE ${LAPACKE_LIBRARIES})
    endif()

endif()


add_executable(netket NetKet/netket.cc)
target_link_libraries(netket PUBLIC netket_lib)
# Yes, this is ugly, but until CMake 3.5, we can't add an external project
# dependency to an interface library
#add_dependencies(netket eigen_project)

#
# Testing
#

if(BUILD_TESTING)
   enable_testing()
   add_subdirectory(Test)
endif()

#
# Installing
#

install (TARGETS netket DESTINATION bin)
