#pragma once
#include "game-object.h"
namespace entities {
    /// <summary>
    /// Things that can be rendered, like meshes.
    /// </summary>
    class Renderable :public GameObject {
    public:
        Renderable(VkContext* ctx,
            const std::string& name,
            const Mesh* mesh);
        const Mesh* mMesh;
    };
}