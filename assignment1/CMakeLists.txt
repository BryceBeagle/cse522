cmake_minimum_required(VERSION 3.10)
project(assignment1 C)

set(CMAKE_C_STANDARD 11)

add_executable(assignment1 main.c)

target_link_libraries(assignment1 m)
target_link_libraries(assignment1 pthread)

target_compile_options(assignment1 PRIVATE "-Wall")