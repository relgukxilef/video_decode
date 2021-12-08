#version 450
#pragma shader_stage(vertex)

layout(location = 0) in vec4 source;
layout(location = 1) in vec4 destination;

layout(location = 0) out vec2 vertex_source;

vec2 positions[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

void main() {
    gl_Position = vec4(
        mix(destination.xy, destination.zw, positions[gl_VertexIndex]),
        0.0, 1.0
    );
    vertex_source = mix(source.xy, source.zw, positions[gl_VertexIndex]);
}
