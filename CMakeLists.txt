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

# flakehound++ library
add_library(flakehound_core
    src/xml_parser.cpp
    src/test_runner.cpp
    src/flakiness_calculator.cpp
    src/logistic_regression.cpp
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

add_executable(analyze_static
    src/analyze_static.cpp
)

target_link_libraries(analyze_static PRIVATE flakehound_core)

# demo tests
add_executable(demo_tests
    demo/demo_tests.cpp
)
