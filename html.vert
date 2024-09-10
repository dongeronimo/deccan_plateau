#version 450

layout(location=0) in vec3 inPosition;
layout(location=1) in vec2 inUV0;
layout(location=2) in vec3 inColor;

layout(location = 0) out vec2 uv0Coord;
void main() {
    gl_Position = vec4(inPosition,1);
    uv0Coord = inUV0;
}