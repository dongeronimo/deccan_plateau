#pragma once
#include <vulkan/vulkan.h>
//The gpu objects namimg api wants the object as uint64. All vk objects are opaque handlers so this macro here is ok
//as long as you are in 64 bits.
#define TO_HANDLE(o) reinterpret_cast<uint64_t>(o)
namespace vk
{
    /// <summary>
    /// Utility singleton to name vk entities like buffers. To use it you need to 
    /// first call ObjectNamer::GetInstance().Init and pass a valid VkDevice.
    /// </summary>
    class ObjectNamer {
    public:
        ObjectNamer(const ObjectNamer&) = delete;
        ObjectNamer& operator=(const ObjectNamer&) = delete;
        void Init(VkDevice device);
        static ObjectNamer& Instance();
        void SetName(uint64_t object, VkObjectType objectType, const char* name);
    private:
        static VkDevice gDevice;
        // Private constructor to prevent instantiation
        ObjectNamer();
        ~ObjectNamer() = default;
    };
}
//A more concise way of setting the name of vulkan objects, i want to write less
#define SET_NAME(obj, type, name) vk::ObjectNamer::Instance().SetName(TO_HANDLE(obj), type, name);