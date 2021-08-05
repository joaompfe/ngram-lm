cmake_minimum_required(VERSION 3.21)
project(ngram_lm C CXX)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(src)
add_subdirectory(test)