#version 450
#pragma shader_stage(vertex)

layout(location = 0) out vec2 vertex_source;

vec2 positions[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

void main() {
    gl_Position = vec4(
        positions[gl_VertexIndex] * 2.0 - 1.0,
        0.0, 1.0
    );
    vertex_source = positions[gl_VertexIndex] * vec2(720, 480) / vec2(1024);
}
