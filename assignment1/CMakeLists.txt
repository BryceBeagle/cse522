cmake_minimum_required(VERSION 3.9.6)
project(assignment1 C)

set(CMAKE_C_STANDARD 11)

#add_custom_target(local)
#add_custom_target(galileo)

add_executable(main main.c)

target_link_libraries(main m pthread)

target_compile_options(main PRIVATE "-Wall")

#target_compile_options(local PRIVATE "-Wall")
#target_compile_options(galileo PRIVATE "-Wall")
