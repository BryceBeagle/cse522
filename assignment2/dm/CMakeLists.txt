add_library(dm STATIC dm.c rm.h)
target_include_directories(dm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})