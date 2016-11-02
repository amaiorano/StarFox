cmake_minimum_required (VERSION 3.2)
project (starfoxgame)

file(GLOB SRC "src/*.cpp" "src/*.h")
add_executable(starfoxgame ${SRC})

# gsgamelib
add_subdirectory(../gsgamelib gsgamelib)
list(APPEND SF_LINK_LIBS "gsgamelib")

# fbx sdk
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(FBX REQUIRED)
list(APPEND SF_INCLUDE_DIRS ${FBX_INCLUDE_DIRS})
list(APPEND SF_LINK_LIBS debug ${FBX_LIBRARIES_DEBUG})
list(APPEND SF_LINK_LIBS optimized ${FBX_LIBRARIES})

target_include_directories(starfoxgame PRIVATE ${SF_INCLUDE_DIRS})
target_link_libraries(starfoxgame PRIVATE ${SF_LINK_LIBS})
