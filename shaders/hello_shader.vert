#version 450
layout(set = 0, binding = 0) uniform CameraUniformBuffer {
    mat4 view;
    mat4 proj;
} cameraUniform;

layout(set = 1, binding = 0) uniform ObjectUniformBuffer {
    mat4 model;
} objectUniform;

layout(location=0) in vec3 inPosition;
layout(location=1) in vec2 inUV0;
layout(location=2) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 uv0Coord;

void main() {
    gl_Position = cameraUniform.proj * cameraUniform.view * objectUniform.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    uv0Coord = inUV0;
}