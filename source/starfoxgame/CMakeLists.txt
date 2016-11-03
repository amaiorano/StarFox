cmake_minimum_required (VERSION 3.2)
project (starfoxgame)

file(GLOB SRC "src/*.cpp" "src/*.h")
add_executable(starfoxgame ${SRC})

# gsgamelib
add_subdirectory(../gsgamelib gsgamelib)
target_link_libraries(starfoxgame PRIVATE gsgamelib)

# fbx sdk
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(FBX REQUIRED)
target_include_directories(starfoxgame PRIVATE ${FBX_INCLUDE_DIRS})
target_link_libraries(starfoxgame PRIVATE debug ${FBX_LIBRARIES_DEBUG})
target_link_libraries(starfoxgame PRIVATE optimized ${FBX_LIBRARIES})
