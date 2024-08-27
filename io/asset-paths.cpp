#include "asset-paths.h"
#include <filesystem>
#include <sstream>

std::string io::CalculatePathForShader(const std::string& filename)
{
    std::filesystem::path executablePath = std::filesystem::current_path();
    std::string strExecutablePath = executablePath.string();
    std::stringstream ss;
    ss << strExecutablePath << "/shaders/";
    ss << filename;
    std::string finalPath = ss.str();
    return finalPath;
}

std::string io::CalculatePathForAsset(const std::string& filename)
{
    std::filesystem::path executablePath = std::filesystem::current_path();
    std::string strExecutablePath = executablePath.string();
    std::stringstream ss;
    ss << strExecutablePath << "/assets/";
    ss << filename;
    std::string finalPath = ss.str();
    return finalPath;
}
