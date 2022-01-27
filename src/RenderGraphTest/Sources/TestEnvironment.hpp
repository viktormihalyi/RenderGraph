#ifndef TestENVIRONMENT_HPP
#define TestENVIRONMENT_HPP

#include <memory>

#pragma warning(push, 0)
// from gtest
#include "gtest/gtest.h"
#pragma warning(pop)

// from std
#include <filesystem>
#include <optional>
#include <string>

// from vulkan
#include <vulkan/vulkan.h>

namespace RG {
class ImageData;
class PhysicalDevice;
class Swapchain;
class Device;
class CommandPool;
class Queue;
class DeviceExtra;
class Image;
} // namespace RG

namespace RG {
class Presentable;
class VulkanEnvironment;
class Window;
} // namespace RG


extern const std::filesystem::path ReferenceImagesFolder;
extern const std::filesystem::path TempFolder;

extern const std::string passThroughVertexShader;

void testDebugCallback (VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                         VkDebugUtilsMessageTypeFlagsEXT             messageType,
                         const VkDebugUtilsMessengerCallbackDataEXT* callbackData);


using EmptyTestEnvironment = ::testing::Test;


class TestEnvironmentBase : public ::testing::Test {
protected:
    std::unique_ptr<RG::VulkanEnvironment> env;
    std::unique_ptr<RG::Window>            window;
    std::shared_ptr<RG::Presentable>       presentable;

    RG::PhysicalDevice& GetPhysicalDevice ();
    RG::Device&         GetDevice ();
    RG::CommandPool&    GetCommandPool ();
    RG::Queue&          GetGraphicsQueue ();
    RG::DeviceExtra&    GetDeviceExtra ();
    RG::Window&          GetWindow ();
    RG::Swapchain&      GetSwapchain ();

    virtual ~TestEnvironmentBase () override = default;

    void CompareImages (const std::string& imageName, const RG::Image& image, std::optional<VkImageLayout> transitionFrom = std::nullopt);

    void CompareImages (const std::string& name, const RG::ImageData& actualImage);
};


// no window, swapchain, surface
class HeadlessTestEnvironment : public TestEnvironmentBase {
protected:
    virtual void SetUp () override;
    virtual void TearDown () override;
};


// window shown by default
class ShownWindowTestEnvironment : public TestEnvironmentBase {
protected:
    virtual void SetUp () override;
    virtual void TearDown () override;
};


// window hidden by default
class HiddenWindowTestEnvironment : public TestEnvironmentBase {
protected:
    virtual void SetUp () override;
    virtual void TearDown () override;
};

#endif
