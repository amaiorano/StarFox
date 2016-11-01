#cmake_minimum_required (VERSION 3.2)
#project (fbxsdk)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}")
find_package(FBX REQUIRED)

add_library(fbxsdk UNKNOWN IMPORTED)
set_property(TARGET fbxsdk PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${FBX_INCLUDE_DIRS}")
set_property(TARGET fbxsdk PROPERTY IMPORTED_LOCATION "${FBX_LIBRARIES}")
set_property(TARGET fbxsdk PROPERTY IMPORTED_LOCATION_DEBUG "${FBX_LIBRARIES_DEBUG}")

#target_include_directories(fbxsdk INTERFACE ${FBX_INCLUDE_DIRS})
#target_link_libraries(fbxsdk INTERFACE ${FBX_LIBRARIES}) # todo: FBX_LIBRARIES_DEBUG
