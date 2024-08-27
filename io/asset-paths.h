#pragma once
#include <string>
namespace io {
    /// <summary>
    /// Assumes that the binaries (.spv) are in /shaders. In dev time this is build/shaders
    /// </summary>
    /// <param name="filename"></param>
    /// <returns></returns>
    std::string CalculatePathForShader(const std::string& filename);

    std::string CalculatePathForAsset(const std::string& filename);
}