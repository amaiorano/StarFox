cmake_minimum_required (VERSION 3.2)
project (gsgamelib)

file(GLOB_RECURSE SRC src/*.cpp src/*.h unit_tests/*.cpp unit_tests/*.h)

add_library(gsgamelib ${SRC})

# @TODO: split headers from cpp files
target_include_directories(gsgamelib PUBLIC src)

target_link_libraries(gsgamelib INTERFACE)
