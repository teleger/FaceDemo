cmake_minimum_required(VERSION 3.4.1)
if(CMAKE_TOOLCHAIN_FILE)
set(LIBRARY_OUTPUT_PATH_ROOT ${CMAKE_BINARY_DIR} CACHE PATH "root for library output, set this to change where aarch64 libs are compiled to")
# get absolute path, but get_filename_component ABSOLUTE only refer with source dir, so find_file here :(
get_filename_component(CMAKE_TOOLCHAIN_FILE_NAME ${CMAKE_TOOLCHAIN_FILE} NAME)
find_file(CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE_NAME} PATHS ${CMAKE_SOURCE_DIR} NO_DEFAULT_PATH)
message(STATUS "CMAKE_TOOLCHAIN_FILE = ${CMAKE_TOOLCHAIN_FILE}")
endif()

add_compile_options(-std=c++11)
add_definitions(-DLINUX)
#add_definitions("-Wall -g")
add_definitions("-fPIC")

set(JLXFACEDEMO_SRC_MAIN ${CMAKE_SOURCE_DIR})

set(JLXSDK_MTCNN_DIRS ${JLXFACEDEMO_SRC_MAIN}/src/main/cpp/include)
message(STATUS "THis is mtcnn include : ${JLXSDK_MTCNN_DIRS}")

set(JLXSDK_SAMPLE_DIR  ${JLXFACEDEMO_SRC_MAIN}/../Sample)
message(STATUS "THis is SAMPLE_DIR : ${JLXSDK_SAMPLE_DIR}")

set(JLXSDK_SSL_DIR  ${JLXFACEDEMO_SRC_MAIN}/../filessl/include)
message(STATUS "THis is SSL_DIR : ${JLXSDK_SSL_DIR}")

set(JLXSDK_JSON_DIR  ${JLXFACEDEMO_SRC_MAIN}/../Json/json_include)
message(STATUS "THis is JSON_DIR : ${JLXSDK_JSON_DIR}")


#add mtcnn headers 
include_directories(${JLXSDK_MTCNN_DIRS})


add_library(libncnn STATIC IMPORTED)
set_target_properties(libncnn
			PROPERTIES IMPORTED_LOCATION
			${CMAKE_SOURCE_DIR}/libs/libncnn.a)

#add opencv
set(OpenCV_DIR /usr/local/lib/lib/cmake/opencv4)
find_package(OpenCV REQUIRED)
if(OpenCV_FOUND)
	message(STATUS "Found OpenCV")
	message(STATUS "OpenCV_INCLUDE_DIRS = ${OpenCV_INCLUDE_DIRS}")
endif(OpenCV_FOUND)

#OpenCV_INCLUDE_DIRS
include_directories(${OpenCV_INCLUDE_DIRS})
message(STATUS "OpenCV_LIBS = ${OpenCV_LIBS}")
message(STATUS " OpenCV version: ${OpenCV_VERSION}")
#end

#target
add_library(mtcnn SHARED 
	${JLXFACEDEMO_SRC_MAIN}/src/main/cpp/mtcnn.h
	${JLXFACEDEMO_SRC_MAIN}/src/main/cpp/mtcnn.cpp)

# 
target_link_libraries(mtcnn   libncnn gomp ${OpenCV_LIBS}) 
