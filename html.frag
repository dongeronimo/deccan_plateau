#version 450

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 uv0;

void main() {
    outColor = texture(texSampler, uv0);
}