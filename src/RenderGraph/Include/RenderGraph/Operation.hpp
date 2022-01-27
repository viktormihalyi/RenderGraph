#ifndef OPERATION_HPP
#define OPERATION_HPP

#include "RenderGraph/Utils/Assert.hpp"

#include "RenderGraph/RenderGraphExport.hpp"

#include "RenderGraph/ComputeShaderPipeline.hpp"
#include "RenderGraph/Node.hpp"
#include "RenderGraph/ShaderPipeline.hpp"
#include "RenderGraph/ShaderReflectionToAttachment.hpp"
#include "RenderGraph/ShaderReflectionToDescriptor.hpp"

#include "RenderGraph/VulkanWrapper/ShaderModule.hpp"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>


namespace GVK {
class DeviceExtra;
class DescriptorPool;
class DescriptorSet;
class DescriptorSetLayout;
class Framebuffer;
class ImageView2D;
class CommandBuffer;
} // namespace GVK

namespace RG {
class Resource;
class GraphSettings;
class ConnectionSet;
class Drawable;
class DrawableInfo;
} // namespace RG


namespace RG {

class RENDERGRAPH_DLL_EXPORT Operation : public Node {
public:
    struct RENDERGRAPH_DLL_EXPORT Descriptors {
        std::unique_ptr<GVK::DescriptorPool>             descriptorPool;
        std::unique_ptr<GVK::DescriptorSetLayout>        descriptorSetLayout;
        std::vector<std::unique_ptr<GVK::DescriptorSet>> descriptorSets;
    };

    virtual ~Operation () override = default;

    virtual void Compile (const GraphSettings&)                                                          = 0;
    virtual void CompileWithExtent (const GraphSettings& graphSettings, uint32_t width, uint32_t height) = 0;

    virtual void Record (const ConnectionSet& connectionSet, uint32_t resourceIndex, GVK::CommandBuffer& commandBuffer) = 0;

    // when record called, input images will be in GetImageLayoutAtStartForInputs ()
    // output images will be in GetImageLayoutAtStartForOutputs () layouts.
    // recorded commands will put the input images in GetImageLayoutAtEndForInputs ()
    // output images in GetImageLayoutAtEndForOutputs () layouts.
    virtual VkImageLayout GetImageLayoutAtStartForInputs (Resource&)  = 0;
    virtual VkImageLayout GetImageLayoutAtEndForInputs (Resource&)    = 0;
    virtual VkImageLayout GetImageLayoutAtStartForOutputs (Resource&) = 0;
    virtual VkImageLayout GetImageLayoutAtEndForOutputs (Resource&)   = 0;
};


class RENDERGRAPH_DLL_EXPORT ComputeOperation : public Operation {
private:
    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;

public:
    struct RENDERGRAPH_DLL_EXPORT CompileSettings {
        std::unique_ptr<ComputeShaderPipeline> computeShaderPipeline;

        std::unique_ptr<RG::FromShaderReflection::DescriptorWriteInfoTable> descriptorWriteProvider;
        std::unique_ptr<RG::FromShaderReflection::AttachmentDataTable>      attachmentProvider;
    };

    struct RENDERGRAPH_DLL_EXPORT CompileResult {
        Descriptors descriptors;
    };

    CompileSettings compileSettings;
    CompileResult   compileResult;

public:
    ComputeOperation (uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

    virtual ~ComputeOperation () override = default;

    uint32_t GetWorkGroupSizeX () const { return groupCountX; }
    uint32_t GetWorkGroupSizeY () const { return groupCountY; }
    uint32_t GetWorkGroupSizeZ () const { return groupCountZ; }

    virtual void Compile (const GraphSettings&) override;
    virtual void CompileWithExtent (const GraphSettings&, uint32_t width, uint32_t height) override;

    virtual void Record (const ConnectionSet& connectionSet, uint32_t resourceIndex, GVK::CommandBuffer& commandBuffer) override;

    virtual VkImageLayout GetImageLayoutAtStartForInputs (Resource&) override
    {
        GVK_BREAK ();
        throw std::runtime_error ("Compute shaders do not operate on images.");
    }
    virtual VkImageLayout GetImageLayoutAtEndForInputs (Resource&) override
    {
        GVK_BREAK ();
        throw std::runtime_error ("Compute shaders do not operate on images.");
    }
    virtual VkImageLayout GetImageLayoutAtStartForOutputs (Resource&) override
    {
        GVK_BREAK ();
        throw std::runtime_error ("Compute shaders do not operate on images.");
    }
    virtual VkImageLayout GetImageLayoutAtEndForOutputs (Resource&) override
    {
        GVK_BREAK ();
        throw std::runtime_error ("Compute shaders do not operate on images.");
    }
};


class RENDERGRAPH_DLL_EXPORT RenderOperation : public Operation {
public:
    class RENDERGRAPH_DLL_EXPORT Builder {
    private:
        VkDevice                           device;
        std::unique_ptr<Drawable>          drawable;
        std::unique_ptr<ShaderPipeline>    shaderPipiline;
        std::optional<VkPrimitiveTopology> topology;
        std::optional<glm::vec4>           clearColor;
        std::optional<bool>                blendEnabled;
        std::optional<std::string>         name;

    public:
        Builder (VkDevice device);

        Builder& SetPrimitiveTopology (VkPrimitiveTopology value);
        Builder& SetVertexShader (const std::string& value);
        Builder& SetFragmentShader (const std::string& value);
        Builder& SetVertexShader (const std::filesystem::path& value);
        Builder& SetFragmentShader (const std::filesystem::path& value);
        Builder& SetVertices (std::unique_ptr<Drawable>&& value);
        Builder& SetBlendEnabled (bool value = true);
        Builder& SetClearColor (const glm::vec4& value);
        Builder& SetName (const std::string& value);

        std::shared_ptr<RenderOperation> Build ();

    private:
        void EnsurePipelineCreated ();
    };

    struct RENDERGRAPH_DLL_EXPORT CompileSettings {
        std::unique_ptr<Drawable>       drawable;
        std::unique_ptr<ShaderPipeline> pipeline;
        VkPrimitiveTopology             topology;

        std::optional<glm::vec4> clearColor;   // (0, 0, 0, 1) by default
        std::optional<bool>      blendEnabled; // true by default

        std::unique_ptr<RG::FromShaderReflection::DescriptorWriteInfoTable> descriptorWriteProvider;
        std::unique_ptr<RG::FromShaderReflection::AttachmentDataTable>      attachmentProvider;
    };

    struct RENDERGRAPH_DLL_EXPORT CompileResult {
        uint32_t                                       width;
        uint32_t                                       height;
        Descriptors                                    descriptors;
        std::vector<std::unique_ptr<GVK::Framebuffer>> framebuffers;
    };

    CompileSettings compileSettings;
    CompileResult   compileResult;

    RenderOperation (std::unique_ptr<Drawable>&& drawable, std::unique_ptr<ShaderPipeline>&& shaderPipiline, VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    virtual ~RenderOperation () override = default;

    virtual void Compile (const GraphSettings&) override;
    virtual void CompileWithExtent (const GraphSettings&, uint32_t width, uint32_t height) override;
    virtual void Record (const ConnectionSet& connectionSet, uint32_t imageIndex, GVK::CommandBuffer& commandBuffer) override;

    const std::unique_ptr<ShaderPipeline>& GetShaderPipeline () const { return compileSettings.pipeline; }

private:
    virtual VkImageLayout GetImageLayoutAtStartForInputs (Resource&) override;
    virtual VkImageLayout GetImageLayoutAtEndForInputs (Resource&) override;
    virtual VkImageLayout GetImageLayoutAtStartForOutputs (Resource&) override;
    virtual VkImageLayout GetImageLayoutAtEndForOutputs (Resource&) override;
};


} // namespace RG

#endif