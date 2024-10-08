cmake_minimum_required(VERSION 3.5)

project(video_decode LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

add_subdirectory(submodules/glfw)
find_package(Vulkan 1.1)

add_executable(
    video_decode
    main.cpp
    io/io.h io/io.cpp
    utility/resource.h
    utility/av_resource.h
    utility/vulkan_resource.h utility/vulkan_resource.cpp
    utility/out_ptr.h
    data/frame.h data/frame.cpp
    data/frame_cache.h data/frame_cache.cpp
    ui/ui.h ui/ui.cpp
)

# Unfortunately MSVC doesn't actually read the INCLUDE environment variable, so I put the path here explicitly
target_include_directories(
    video_decode PUBLIC
    D:/Felix/Documents/C++/ffmpeg-4.3.2-2021-02-27-full_build-shared/include
)
target_link_directories(
    video_decode PUBLIC
    D:/Felix/Documents/C++/ffmpeg-4.3.2-2021-02-27-full_build-shared/lib
)

target_link_libraries(
    video_decode
    avcodec avformat avutil avfilter
    gdi32 user32 kernel32 glfw Vulkan::Vulkan
)

function(add_shader TARGET SHADER)
    find_program(GLSLC glslc)

    set(current-shader-path ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER})
    set(current-output-path ${CMAKE_BINARY_DIR}/${SHADER})

    # Add a custom command to compile GLSL to SPIR-V.
    get_filename_component(current-output-dir ${current-output-path} DIRECTORY)
    file(MAKE_DIRECTORY ${current-output-dir})

    add_custom_command(
        OUTPUT ${current-output-path}.spv
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${GLSLC} -O -o ${SHADER}.spv ${current-shader-path}
        DEPENDS ${current-shader-path}
        VERBATIM
    )
    # shader is only listed here to make it show up in Qt Creator
    target_sources(
        ${TARGET} PRIVATE ${current-output-path}.spv ${current-shader-path}
    )
endfunction(add_shader)

add_shader(video_decode ui/video_vertex.glsl)
add_shader(video_decode ui/video_fragment.glsl)

target_compile_options(
    video_decode PUBLIC
    -Wall
    /showIncludes
    $<$<CONFIG:Debug>:-g -O0 -D_FORTIFY_SOURCE=2>
)
target_link_options(
    video_decode PUBLIC
    $<$<CONFIG:Debug>:-g -O0 -D_FORTIFY_SOURCE=2>
)
