cmake_minimum_required(VERSION 3.15...3.31)
project(ParserGenerator DESCRIPTION "Table-Driven Parser Generator for a LR(1) grammar"
						LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# parsegen
add_executable(parsegen parsegen.cpp)

# calculator interpreter
add_executable(expressions math-expressions/parse-tables.h math-expressions/expressions.cpp)

# parentheses interpreter
add_executable(parentheses parentheses/parse-tables.h parentheses/parentheses.cpp)
