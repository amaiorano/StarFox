cmake_minimum_required (VERSION 3.2)
project (starfoxgame)

file(GLOB SRC "src/*.cpp" "src/*.h")
add_executable(starfoxgame ${SRC})

# Enable to build DLLs instead of static libs
set(BUILD_SHARED_LIBS On)

# gsgamelib
add_subdirectory(../gsgamelib gsgamelib)
target_link_libraries(starfoxgame PRIVATE gsgamelib)
if (BUILD_SHARED_LIBS)
	add_custom_command(TARGET starfoxgame POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:gsgamelib>" $<TARGET_FILE_DIR:starfoxgame>)
endif()

# fbx sdk
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(FBX REQUIRED)
target_include_directories(starfoxgame PRIVATE ${FBX_INCLUDE_DIRS})
target_link_libraries(starfoxgame PRIVATE debug ${FBX_LIBRARIES_DEBUG})
target_link_libraries(starfoxgame PRIVATE optimized ${FBX_LIBRARIES})
