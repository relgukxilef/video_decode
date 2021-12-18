#version 450
#pragma shader_stage(fragment)

layout(location = 0) in vec2 vertex_source;

layout(location = 0) out vec4 fragment_color;

layout(binding = 0) uniform sampler2D source_texture_y;
layout(binding = 1) uniform sampler2D source_texture_cb;
layout(binding = 2) uniform sampler2D source_texture_cr;

void main() {
    vec3 color = vec3(
        texture(source_texture_y, vertex_source).r,
        texture(source_texture_cb, vertex_source).r - 0.5,
        texture(source_texture_cr, vertex_source).r - 0.5
    ) * mat3(
        1.0, 0.0, 1.5748,
        1.0, -0.1873, -0.4681,
        1.0, 1.8556, 0.0
    );
    fragment_color = vec4(color, 1.0);
}
