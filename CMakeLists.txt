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

enable_testing()

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

# XML parsing test executable
add_executable(test_xml_parsing
    src/test_xml_parsing.cpp
)

target_link_libraries(test_xml_parsing PRIVATE flakehound_core)

add_executable(test_ast_builder
    src/test_ast_builder.cpp
)

target_link_libraries(test_ast_builder PRIVATE flakehound_core)

add_executable(test_static_analyzer
    src/test_static_analyzer.cpp
)

target_link_libraries(test_static_analyzer PRIVATE flakehound_core)

# demo tests
add_executable(demo_tests
    demo/demo_tests.cpp
)

add_test(NAME ast_builder_smoke COMMAND test_ast_builder)
set_tests_properties(ast_builder_smoke PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(NAME xml_parser_smoke
    COMMAND test_xml_parsing ${PROJECT_SOURCE_DIR}/demo/sample_gtest_results.xml
)
set_tests_properties(xml_parser_smoke PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(NAME flakehound_end_to_end
    COMMAND flakehound $<TARGET_FILE:demo_tests> 3
)
set_tests_properties(flakehound_end_to_end PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_test(NAME static_analyzer_smoke COMMAND test_static_analyzer)
set_tests_properties(static_analyzer_smoke PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
