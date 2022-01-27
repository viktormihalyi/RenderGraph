#ifndef VULKANWRAPPER_COMPUTEPIPELINE_HPP
#define VULKANWRAPPER_COMPUTEPIPELINE_HPP

#include "RenderGraph/RenderGraphExport.hpp"
#include "RenderGraph/VulkanWrapper/PipelineBase.hpp"

#include <vulkan/vulkan.h>

#include "RenderGraph/Utils/Assert.hpp"
#include "RenderGraph/Utils/MovablePtr.hpp"
#include "VulkanObject.hpp"

namespace RG {

class ShaderModule;

class RENDERGRAPH_DLL_EXPORT ComputePipeline : public PipelineBase {
private:
    VkDevice                    device;
    RG::MovablePtr<VkPipeline> handle;

public:
    ComputePipeline (VkDevice            device,
                     VkPipelineLayout    pipelineLayout,
                     const ShaderModule& shaderModule);

    ComputePipeline (ComputePipeline&&) = default;
    ComputePipeline& operator= (ComputePipeline&&) = default;

    virtual ~ComputePipeline () override
    {
        vkDestroyPipeline (device, handle, nullptr);
        handle = nullptr;
    }

    virtual void* GetHandleForName () const override { return handle; }

    virtual operator VkPipeline () const override { return handle; }
};

} // namespace RG

#endif