cmake_minimum_required(VERSION 3.14)
project(squid_poker_solver CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(SOURCES
    src/card.cpp
    src/hand_eval.cpp
    src/marker.cpp
    src/bridge.cpp
)

# Shared library for CGO
add_library(squid_solver SHARED ${SOURCES})
target_include_directories(squid_solver PUBLIC src)

# Static library option
add_library(squid_solver_static STATIC ${SOURCES})
target_include_directories(squid_solver_static PUBLIC src)


