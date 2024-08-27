#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
namespace io {
    struct MeshData {
        std::string name;
        std::vector<glm::vec3> vertices;
        std::vector<uint16_t> indices;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> uv0s;
    };

}