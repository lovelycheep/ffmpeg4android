
cmake_minimum_required(VERSION 3.4.1)


add_library(native-stream
           SHARED
           src/main/cpp/native_stream.cpp
           src/main/cpp/mp4_stream.c)

#获取上级目录
get_filename_component(PARENT_DIR ${CMAKE_SOURCE_DIR} PATH)
set(LIBRARY_DIR ${PARENT_DIR}/ffmpeg-single)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
set(CMAKE_VERBOSE_MAKEFILE on)

add_library(ffmpeg-single
           SHARED
           IMPORTED)

set_target_properties(ffmpeg-single
                    PROPERTIES IMPORTED_LOCATION
                    ${LIBRARY_DIR}/libs/${ANDROID_ABI}/libffmpeg.so
                    )

#头文件
include_directories(${LIBRARY_DIR}/libs/${ANDROID_ABI}/include)

target_link_libraries(native-stream ffmpeg-single log)
