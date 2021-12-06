cmake_minimum_required(VERSION 3.5)

project(video_decode LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    video_decode
    main.cpp
    io/io.h io/io.cpp
    utility/resource.h
    utility/av_resource.h
    utility/vulkan_resource.h utility/vulkan_resource.cpp
    utility/out_ptr.h
    data/frame.h data/frame.cpp
    ui/ui.h ui/ui.cpp
)

target_link_libraries(
    video_decode
    avcodec avformat avutil avfilter
    gdi32 user32 kernel32 glfw3 vulkan-1
)
