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
    data/frame_cache.h data/frame_cache.cpp
    ui/ui.h ui/ui.cpp
)

target_link_libraries(
    video_decode
    avcodec avformat avutil avfilter
    gdi32 user32 kernel32 glfw3 vulkan-1
)

function(add_shader TARGET SHADER)
    find_program(GLSLC glslc)

    set(current-shader-path ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER})
    set(current-output-path ${CMAKE_BINARY_DIR}/${SHADER})

    # Add a custom command to compile GLSL to SPIR-V.
    get_filename_component(current-output-dir ${current-output-path} DIRECTORY)
    file(MAKE_DIRECTORY ${current-output-dir})

    add_custom_command(
        OUTPUT ${current-output-path}.o
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND ${GLSLC} -o ${SHADER}.spv ${current-shader-path}
        COMMAND ld -r -b binary -o ${SHADER}.o ${SHADER}.spv
        DEPENDS ${current-shader-path}
        IMPLICIT_DEPENDS CXX ${current-shader-path}
        VERBATIM
    )

    # Make sure our build depends on this output.
    set_source_files_properties(
        ${current-output-path}.o PROPERTIES GENERATED TRUE
    )
    # shader is only listed here to make it show up in Qt Creator
    target_sources(
        ${TARGET} PRIVATE ${current-output-path}.o ${current-shader-path}
    )
endfunction(add_shader)

add_shader(video_decode ui/video_vertex.glsl)
add_shader(video_decode ui/video_fragment.glsl)
