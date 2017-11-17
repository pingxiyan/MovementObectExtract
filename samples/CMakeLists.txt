cmake_minimum_required(VERSION 2.6)

set(TARGET_NAME samples)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(ExtracObjLib extract_obj_lib)

project(${TARGET_NAME})

# --- thirdparty lib
find_package(OpenCV REQUIRED)


file (GLOB MAIN_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        )

file (GLOB MAIN_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/../extract_obj_lib/header/public/*.h
        )

include_directories(./
	${CMAKE_CURRENT_SOURCE_DIR}/../extract_obj_lib/header/public
	${OpenCV_INCLUDE_DIRS}
	)

# Set 3rd library position
link_directories(${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Create executable programe
add_executable(${TARGET_NAME} ${MAIN_SRC} ${MAIN_HEADERS})

# Link 3rd library
target_link_libraries(${TARGET_NAME} ${OpenCV_LIBS} ${ExtracObjLib})
