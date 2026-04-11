cmake_minimum_required(VERSION 3.14)
project(FlakeHound++ VERSION 1.0.0 LANGUAGES CXX)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# google test setup
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
# for windows platforms
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# flakehound++ library
add_library(flakehound_core
    src/xml_parser.cpp
    src/test_runner.cpp
    src/flakiness_calculator.cpp
    src/lexer.cpp
    src/ast_builder.cpp
    src/cfg_generator.cpp
    src/static_analyzer.cpp
    src/report_generator.cpp
)

target_include_directories(flakehound_core PUBLIC 
    ${PROJECT_SOURCE_DIR}/include
)

add_executable(flakehound
    src/main.cpp
)

target_link_libraries(flakehound PRIVATE flakehound_core)

# Abseil analysis executable
add_executable(analyze_abseil
    src/analyze_abseil.cpp
)

target_link_libraries(analyze_abseil PRIVATE flakehound_core)

# XML parsing test executable
add_executable(test_xml_parsing
    src/test_xml_parsing.cpp
)

target_link_libraries(test_xml_parsing PRIVATE flakehound_core)

add_executable(test_ast_builder
    src/test_ast_builder.cpp
)

target_link_libraries(test_ast_builder PRIVATE flakehound_core)

# demo tests
add_executable(demo_tests
    demo/demo_tests.cpp
)

target_link_libraries(demo_tests 
    PRIVATE 
    GTest::gtest_main
)
