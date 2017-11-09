cmake_minimum_required(VERSION 2.6)

set(TARGET_NAME object_extract)

project(${TARGET_NAME})

####################################
## to use C++11
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
####################################

add_subdirectory(extract_obj_lib)
add_subdirectory(samples)
