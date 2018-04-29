include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(cse522-assignment4)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
