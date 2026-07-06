# Each test file builds into its own standalone executable using the
# lightweight header-only MiniTest framework (no external dependency
# required). Swap in find_package(GTest) here if GoogleTest is installed
# on your system and you prefer its richer reporting/fixtures.

add_executable(test_strategies test_strategies.cpp)
target_link_libraries(test_strategies PRIVATE loadbalancer_core)
add_test(NAME test_strategies COMMAND test_strategies)

add_executable(test_reliability test_reliability.cpp)
target_link_libraries(test_reliability PRIVATE loadbalancer_core)
add_test(NAME test_reliability COMMAND test_reliability)

add_executable(test_integration test_integration.cpp)
target_link_libraries(test_integration PRIVATE loadbalancer_core)
add_test(NAME test_integration COMMAND test_integration)
