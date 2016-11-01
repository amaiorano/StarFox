cmake_minimum_required (VERSION 3.2)
project (starfoxgame)

file(GLOB SRC "src/*.cpp" "src/*.h")
add_executable(starfoxgame ${SRC})

add_subdirectory(../gsgamelib gsgamelib)
include(../fbxsdk/fbxsdk-targets.cmake)

target_link_libraries(starfoxgame gsgamelib fbxsdk)
