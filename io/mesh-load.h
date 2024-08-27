#pragma once
#include <memory>
#include "mesh-data.h"
namespace io {
    std::vector<std::shared_ptr<MeshData>> LoadMeshes(
        const std::string& file
    );
}