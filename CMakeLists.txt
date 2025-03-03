cmake_minimum_required(VERSION 3.15...3.31)
project(ParserGenerator DESCRIPTION "Table-Driven Parser Generator for a LR(1) grammar"
						LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# parsegen
add_executable(parsegen parsegen.cpp)

# calculator interpreter
add_executable(calculator cli-calculator/cli-calculator.cpp
			  cli-calculator/parse-tables.h)

# parentheses interpreter
add_executable(parentheses parentheses/parentheses.cpp
			   parentheses/parse-tables.h)
