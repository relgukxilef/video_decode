cmake_minimum_required(VERSION 3.5)

project(video_decode LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    video_decode
    main.cpp
    io/io.h io/io.cpp
    utility/av_resource.h
    utility/out_ptr.h
    data/frame.h data/frame.cpp
)

target_link_libraries(video_decode avcodec avformat avutil avfilter)
