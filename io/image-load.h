#pragma once
#include <vector>
#include <string>
namespace io {
    struct ImageData {
        int w = 0;
        int h = 0;
        std::vector<uint8_t> pixels = {};
        uint64_t size = 0;
        std::string name;
    };

    /// <summary>
    /// Load an image from the disk into the memory,
    /// returning it's width, height, size and bytes.
    /// 
    /// It assumes the image has 4 color channels and will
    /// break if the image does not.
    /// </summary>
    /// <param name="w"></param>
    /// <param name="h"></param>
    /// <param name="pixels"></param>
    /// <param name="size"></param>
    /// <param name="file"></param>
    void LoadImage(int& w,
        int& h,
        std::vector<uint8_t>& pixels,
        uint64_t& size,
        const std::string& file
    );

    ImageData* LoadImage(const std::string& file);

    void WriteImage(const std::string& filename, int w, int h, const std::vector<uint8_t>& data);

}