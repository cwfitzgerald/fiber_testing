cmake_minimum_required(VERSION 3.12)
project(fiber_tasking CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set( FTL_BUILD_TESTS OFF CACHE BOOL  "Disable tests" )
set( FTL_BUILD_BENCHMARKS OFF CACHE BOOL  "Disable benchmarks" )
add_subdirectory(fiber_tasking_lib)

file(GLOB SOURCES LIST_DIRECTORIES false CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(fiber_tasking ${SOURCES})

target_link_libraries(fiber_tasking ftl)