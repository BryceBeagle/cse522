cmake_minimum_required(VERSION 3.9.6)
set(CMAKE_C_STANDARD 11)

project(assignment2 C)

add_subdirectory(edf)
add_subdirectory(rm)

add_executable(main main.c task_types.h)
target_link_libraries(main m edf rm)

target_compile_options(main PRIVATE "-Wall")
