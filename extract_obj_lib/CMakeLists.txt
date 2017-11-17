cmake_minimum_required(VERSION 2.6)

set(TARGET_NAME extract_obj_lib)

project(${TARGET_NAME})

# --- thirdparty lib
find_package(OpenCV REQUIRED)

# Set exe output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# Set lib output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file (GLOB MAIN_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        )

file (GLOB MAIN_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/header/public/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/header/private/*.h
        )

#message(STATE ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/header/public/
	${CMAKE_CURRENT_SOURCE_DIR}/header/private/
	${OpenCV_INCLUDE_DIRS}
	)

# ---[ cpp/h file
aux_source_directory(./header/public/ HEADERPUBLIC)
aux_source_directory(./header/private/ HEADERPRIVATE)
aux_source_directory(./src/ SRC)

# Create library file from sources.
#add_library(${TARGET_NAME} STATIC ${MAIN_SRC} ${MAIN_HEADERS})
add_library(${TARGET_NAME} SHARED ${MAIN_SRC} ${MAIN_HEADERS})

target_link_libraries(${TARGET_NAME} ${OpenCV_LIBS})