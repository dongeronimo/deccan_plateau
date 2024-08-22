#include "object_namer.h"
#include <cassert>
VkDevice vk::ObjectNamer::gDevice;
namespace vk {
    void vk::ObjectNamer::Init(VkDevice device)
    {
        gDevice = device;
    }

    ObjectNamer& ObjectNamer::Instance()
    {
        static ObjectNamer instance; // Guaranteed to be destroyed, instantiated on first use
        return instance;

    }

    void ObjectNamer::SetName(uint64_t object, VkObjectType objectType, const char* name)
    {
        assert(gDevice != nullptr);
        VkDebugUtilsObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.pNext = nullptr;
        nameInfo.objectType = objectType;
        nameInfo.objectHandle = object;
        nameInfo.pObjectName = name;

        // Get the function pointer for vkSetDebugUtilsObjectNameEXT
        static PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT =
            (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(gDevice, 
                "vkSetDebugUtilsObjectNameEXT");

        if (vkSetDebugUtilsObjectNameEXT != nullptr) {
            vkSetDebugUtilsObjectNameEXT(gDevice, &nameInfo);
        }

    }

    vk::ObjectNamer::ObjectNamer()
    {
        gDevice = nullptr;
    }

}