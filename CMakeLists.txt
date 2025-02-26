cmake_minimum_required(VERSION 3.15...3.31)
project(ParserGenerator DESCRIPTION "Table-Driven Parser Generator for a LR(1) grammar"
						LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
add_executable(parsegen parsegen.cpp)