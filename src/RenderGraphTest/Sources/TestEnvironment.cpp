#include "TestEnvironment.hpp"

#include "RenderGraph/Window/GLFWWindow.hpp"
#include "RenderGraph/VulkanWrapper/Utils/ImageData.hpp"
#include "RenderGraph/VulkanWrapper/Surface.hpp"
#include "RenderGraph/VulkanEnvironment.hpp"
#include "RenderGraph/VulkanWrapper/VulkanWrapper.hpp"

#include <optional>


const std::filesystem::path ReferenceImagesFolder = std::filesystem::current_path () / "TestData" / "ReferenceImages";
const std::filesystem::path TempFolder            = std::filesystem::current_path () / "temp";

const std::string passThroughVertexShader = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec2 textureCoords;

vec2 uvs[6] = vec2[] (
    vec2 (0.f, 0.f),
    vec2 (0.f, 1.f),
    vec2 (1.f, 1.f),
    vec2 (1.f, 1.f),
    vec2 (0.f, 0.f),
    vec2 (1.f, 0.f)
);

vec2 positions[6] = vec2[] (
    vec2 (-1.f, -1.f),
    vec2 (-1.f, +1.f),
    vec2 (+1.f, +1.f),
    vec2 (+1.f, +1.f),
    vec2 (-1.f, -1.f),
    vec2 (+1.f, -1.f)
);


void main() {
    gl_Position = vec4 (positions[gl_VertexIndex], 0.0, 1.0);
    textureCoords = uvs[gl_VertexIndex];
}
)";


void testDebugCallback (VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                         VkDebugUtilsMessageTypeFlagsEXT             messageType,
                         const VkDebugUtilsMessengerCallbackDataEXT* callbackData)
{
    RG::defaultDebugCallback (messageSeverity, messageType, callbackData);

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        EXPECT_TRUE (false);
    }
}


RG::PhysicalDevice& TestEnvironmentBase::GetPhysicalDevice ()
{
    return *env->physicalDevice;
}


RG::Device& TestEnvironmentBase::GetDevice ()
{
    return *env->device;
}


RG::CommandPool& TestEnvironmentBase::GetCommandPool ()
{
    return *env->commandPool;
}


RG::Queue& TestEnvironmentBase::GetGraphicsQueue ()
{
    return *env->graphicsQueue;
}


RG::DeviceExtra& TestEnvironmentBase::GetDeviceExtra ()
{
    return *env->deviceExtra;
}


RG::Window& TestEnvironmentBase::GetWindow ()
{
    return *window;
}


RG::Swapchain& TestEnvironmentBase::GetSwapchain ()
{
    return presentable->GetSwapchain ();
}


void TestEnvironmentBase::CompareImages (const std::string& imageName, const RG::Image& image, std::optional<VkImageLayout> transitionFrom)
{
    if (transitionFrom.has_value ()) {
        TransitionImageLayout (GetDeviceExtra (), image, *transitionFrom, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    }

    const bool imagesMatch = AreImagesEqual (GetDeviceExtra (), image, ReferenceImagesFolder / (imageName + "_reference.png"));

    EXPECT_TRUE (imagesMatch);

    if (!imagesMatch) {
        const std::filesystem::path outPath = TempFolder / (imageName + "_actual.png");
        std::cout << "Saving " << outPath.string () << "..." << std::endl;
        SaveImageToFileAsync (GetDeviceExtra (), image, outPath).join ();
    }
}


void TestEnvironmentBase::CompareImages (const std::string& name, const RG::ImageData& actualImage)
{
    std::optional<RG::ImageData> referenceImage;
    if (std::filesystem::exists (ReferenceImagesFolder / (name + ".png")))
        referenceImage.emplace (ReferenceImagesFolder / (name + ".png"));

    std::optional<RG::ImageData::ComparisonResult> comparison;
    if (referenceImage.has_value ())
        comparison = referenceImage->CompareTo (actualImage);

    EXPECT_TRUE (comparison.has_value ());
    if (comparison.has_value ())
        EXPECT_TRUE (comparison->equal);

    if (!comparison.has_value () || !comparison->equal) {

        if (referenceImage.has_value ()) {
            const std::filesystem::path outPathRef = TempFolder / (name + "_Reference.png");
            std::cout << "Saving " << outPathRef.string () << "..." << std::endl;
            referenceImage->SaveTo (outPathRef);
        }

        const std::filesystem::path outPath = TempFolder / (name + "_Actual.png");
        std::cout << "Saving " << outPath.string () << "..." << std::endl;
        actualImage.SaveTo (outPath);

        if (comparison.has_value ()) {
            if (RG_VERIFY (comparison->diffImage != nullptr)) {
                const std::filesystem::path outPathDiff = TempFolder / (name + "_Diff.png");
                std::cout << "Saving " << outPathDiff.string () << "..." << std::endl;
                comparison->diffImage->SaveTo (outPathDiff);
            }
        }
    }
}


void HeadlessTestEnvironment::SetUp ()
{
    env = std::make_unique<RG::VulkanEnvironment> (testDebugCallback);
}


void HeadlessTestEnvironment::TearDown ()
{
    env.reset ();
}


void ShownWindowTestEnvironment::SetUp ()
{
    window      = std::make_unique<RG::GLFWWindow> ();
    env         = std::make_unique<RG::VulkanEnvironment> (testDebugCallback, RG::GetGLFWInstanceExtensions (), std::vector<const char*> { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME });
    presentable = std::make_unique<RG::Presentable> (*env, *window, std::make_unique<RG::DefaultSwapchainSettings> ());
}


void ShownWindowTestEnvironment::TearDown ()
{
    presentable.reset ();
    env.reset ();
    window.reset ();
}


void HiddenWindowTestEnvironment::SetUp ()
{
    window      = std::make_unique<RG::HiddenGLFWWindow> ();
    env         = std::make_unique<RG::VulkanEnvironment> (testDebugCallback, RG::GetGLFWInstanceExtensions (), std::vector<const char*> { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME });
    presentable = std::make_unique<RG::Presentable> (*env, *window, std::make_unique<RG::DefaultSwapchainSettings> ());
}


void HiddenWindowTestEnvironment::TearDown ()
{
    presentable.reset ();
    env.reset ();
    window.reset ();
}
