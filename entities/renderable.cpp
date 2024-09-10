#include "renderable.h"
namespace entities {
    Renderable::Renderable(VkContext* ctx, const std::string& name, const Mesh* mesh)
        :GameObject(ctx, name), mMesh(mesh)
    {
        assert(mesh != nullptr);
    }
}
