#version 450
#pragma shader_stage(vertex)

layout(location = 0) in vec2 vertex_source;

layout(location = 0) out vec4 fragment_color;

layout(binding = 0) uniform sampler2D source_texture;

void main() {
    fragment_color = texture(source_texture, vertex_source);
}
