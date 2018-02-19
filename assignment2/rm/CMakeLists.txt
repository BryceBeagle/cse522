add_library(rm STATIC rm.c rm.h)
target_include_directories(rm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})