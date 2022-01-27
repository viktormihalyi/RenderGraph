#ifndef RAWIMAGEDATA_HPP
#define RAWIMAGEDATA_HPP

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <vector>
#include <memory>

#include "RenderGraph/VulkanWrapper/DeviceExtra.hpp"
#include "RenderGraph/VulkanWrapper/Image.hpp"

namespace RG {

class RENDERGRAPH_DLL_EXPORT ImageData {
public:
    static const ImageData Empty;

private:
    ImageData ();
    ImageData (size_t                      components,
               size_t                      width,
               size_t                      height,
               const std::vector<uint8_t>& data);

public:
    size_t               componentByteSize;
    size_t               components;
    size_t               width;
    size_t               height;
    std::vector<uint8_t> data;

    ImageData (const DeviceExtra& device, const Image& image, uint32_t layerIndex, std::optional<VkImageLayout> currentLayout = std::nullopt);
    ImageData (const std::filesystem::path& path, const uint32_t components = 4);
    
    static void FillBuffer (const DeviceExtra& device, const Image& image, uint32_t layerIndex, std::optional<VkImageLayout> currentLayout, uint8_t* buffer, size_t bufferSize);

    static ImageData FromDataUint (const std::vector<uint8_t>& data, uint32_t width, uint32_t height, uint32_t components);
    static ImageData FromDataFloat (const std::vector<float>& data, uint32_t width, uint32_t height, uint32_t components);

    bool operator== (const ImageData& other) const;

    struct ComparisonResult {
        bool                       equal = false;
        std::unique_ptr<ImageData> diffImage;
    };
    ComparisonResult CompareTo (const ImageData& diff) const;

    uint32_t GetByteCount () const;

    void ConvertBGRToRGB ();

    void SaveTo (const std::filesystem::path& path) const;
    void UploadTo (const DeviceExtra& device, const Image& image, std::optional<VkImageLayout> currentLayout = std::nullopt) const;
};

}

#endif