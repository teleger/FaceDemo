if(CMAKE_TOOLCHAIN_FILE)
get_filename_component(CMAKE_TOOLCHAIN_FILE_NAME ${CMAKE_TOOLCHAIN_FILE} NAME)
find_file(CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE_NAME} PATHS ${CMAKE_SOURCE_DIR} NO_DEFAULT_PATH)
message(STATUS "CMAKE_TOOLCHAIN_FILE = ${CMAKE_TOOLCHAIN_FILE}")
endif()

cmake_minimum_required(VERSION 3.4.1)

add_compile_options(-std=c++11)
add_definitions(-DLINUX)
add_definitions(-DLINUX_C_DEBUG)
add_definitions("-fPIC")
set(JLXSDK_SRC_MAIN ${CMAKE_SOURCE_DIR})

set(JLXSDK_SRC ${JLXSDK_SRC_MAIN}/Samples)
include_directories(${JLXSDK_SRC}/Sample/include)
set(JLXSDK_SSL_INCLUDE ${JLXSDK_SRC_MAIN}/Samples/filessl/include)
include_directories(${JLXSDK_SSL_INCLUDE})
message(STATUS "JLXSDK_SSL_INCLUDE = ${JLXSDK_SSL_INCLUDE}")

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


#add    json
set(JLX_JSON_SRC_DIR   ${JLXSDK_SRC}/Json/json_src)
set(JLX_JSON_INCLUDE_DIR ${JLXSDK_SRC}/Json/json_include)
include_directories(${JLX_JSON_INCLUDE_DIR})
message(STATUS "JSON_INCLUDE=${JLX_JSON_INCLUDE_DIR}")

set(COMMON_JSON_SOURCES
    ${JLX_JSON_SRC_DIR}/json_reader.cpp
    ${JLX_JSON_SRC_DIR}/json_value.cpp
    ${JLX_JSON_SRC_DIR}/json_writer.cpp)
#end


#add   libcrypto.a  and crypto header file
set(JLX_CRYPTO_INDIR  ${JLXSDK_SRC}/filessl/include)
include_directories(${JLX_CRYPTO_INDIR})

add_library(jlxcrypto STATIC IMPORTED)
set_target_properties(jlxcrypto
			PROPERTIES IMPORTED_LOCATION
			${CMAKE_SOURCE_DIR}/Samples/filessl/libs/libcrypto.a)
message(STATUS " crypto include : ${JLX_CRYPTO_INDIR}")
#end


#add     ncnn.a  and header file
set(MTCNN_INCLUDE_PATH ${JLXSDK_SRC}/facedetect/src/main/cpp/include)
include_directories(${MTCNN_INCLUDE_PATH})

add_library(jlxncnn STATIC IMPORTED)
set_target_properties(jlxncnn
			PROPERTIES IMPORTED_LOCATION
			${CMAKE_SOURCE_DIR}/Samples/facedetect/libs/libncnn.a)
message(STATUS " mtcnn dir :${MTCNN_INCLUDE_PATH}" )
#end


#add engine so
add_library(jlxengine STATIC IMPORTED)
set_target_properties(jlxengine
		PROPERTIES IMPORTED_LOCATION
			${CMAKE_SOURCE_DIR}/Samples/GTILibs/libGTILibrary.a)

add_library(jlxftd STATIC IMPORTED)
set_target_properties(jlxftd
		PROPERTIES IMPORTED_LOCATION
		${CMAKE_SOURCE_DIR}/Samples/GTILibs/libftd3xx.a)
#end

#SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS}")

#add sqlite
set(sqlite_src ${CMAKE_SOURCE_DIR}/Samples/sqlite3)

add_library(sqlite3 SHARED
			  ${sqlite_src}/shell.c
		          ${sqlite_src}/sqlite3.c
			 ${sqlite_src}/sqlite3.h)

message(STATUS " sqlite3  path : ${sqlite_src}")
#end

#source directory源文件目录
file(GLOB SAMPLE_SRC
		${JLXSDK_SRC}/Sample/*.hpp
		${JLXSDK_SRC}/Sample/*.cpp
		${LXSDK_SRC}/facedetect/src/main/cpp/*.h
		${JLXSDK_SRC}/facedetect/src/main/cpp/*.cpp
		)

add_library(jlx_classify SHARED
	  ${SAMPLE_SRC}
	  ${sqlite_src}
    ${COMMON_JSON_SOURCES})

target_link_libraries(jlx_classify
		${OpenCV_LIBS}
		jlxcrypto
		jlxncnn
		jlxengine
		jlxftd
		gomp
		sqlite3
		atomic
		c
		z
		m
		dl
		stdc++)
