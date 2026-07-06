cmake_minimum_required(VERSION 3.15)
project(LoadBalancerSimulation CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

find_package(Threads REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(loadbalancer_core
    src/Request.cpp
    src/LoadBalancer.cpp
)
target_link_libraries(loadbalancer_core PUBLIC Threads::Threads)

add_executable(loadbalancer src/main.cpp)
target_link_libraries(loadbalancer PRIVATE loadbalancer_core)

# --- Tests -------------------------------------------------------------
# Lightweight, dependency-free test runner (no GoogleTest install required
# to build/run). If GoogleTest is available on the system it will be used
# instead for a richer report -- see tests/CMakeLists.txt.
enable_testing()
add_subdirectory(tests)
