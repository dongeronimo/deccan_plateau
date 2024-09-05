#version 450
layout(push_constant) uniform PushConstants {
    int id;  // Add your integer here
} pushConstants;
layout(location = 0) out vec4 outColor;

vec3 idToColor(int id) {
    float r = float((id >> 16) & 0xFF) / 255.0;
    float g = float((id >> 8) & 0xFF) / 255.0;
    float b = float(id & 0xFF) / 255.0;
    return vec3(r, g, b);
}

void main() {
    outColor = vec4(idToColor(pushConstants.id),1);
}