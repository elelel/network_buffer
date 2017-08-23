cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(network_buffer_test VERSION 2.2.4 LANGUAGES C CXX)
include(CTest)

enable_testing()

if (BUILD_TESTING)
  set(TEST_SOURCES
    network_buffer_test.cpp
    )

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -Wall -Wextra -Werror -Wunused")

  add_executable(network_buffer_test ${TEST_SOURCES})
  target_compile_definitions(network_buffer_test PRIVATE "CATCH_CONFIG_MAIN" "CATCH_CONFIG_COLOUR_NONE")

  add_test(NAME RunBufferTests WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" COMMAND network_buffer_test)
endif()
