add_library(edf STATIC edf.c edf.h)
target_include_directories(edf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})