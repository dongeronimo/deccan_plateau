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


void main() {
    gl_Position = cameraUniform.proj * cameraUniform.view * objectUniform.model * vec4(inPosition, 1.0);
}