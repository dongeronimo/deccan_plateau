#include "image-load.h"
#include "asset-paths.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
void io::LoadImage(int& w, 
    int& h, 
    std::vector<uint8_t>& pixels, 
    uint64_t& size,
    const std::string& file)
{
    int texChannels;
    std::string path = CalculatePathForAsset(file);
    stbi_uc* bytes = stbi_load(path.c_str(),
        &w, &h, &texChannels, STBI_rgb_alpha);
    size = w * h * 4;
    pixels.resize(size);
    memcpy(pixels.data(), bytes, size);
    stbi_image_free(bytes);
}

io::ImageData* io::LoadImage(const std::string& file)
{
    ImageData* data = new ImageData;
    LoadImage(data->w, data->h, data->pixels, data->size, file);
    return data;
}
