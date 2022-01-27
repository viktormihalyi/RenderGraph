#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include <vulkan/vulkan.h>

#include "RenderGraph/VulkanWrapper/PipelineBase.hpp"

#include "RenderGraph/Utils/Assert.hpp"
#include "RenderGraph/Utils/MovablePtr.hpp"
#include "VulkanObject.hpp"

#include <vector>

namespace GVK {

class RENDERGRAPH_DLL_EXPORT GraphicsPipeline : public PipelineBase {
private:
    VkDevice                    device;
    GVK::MovablePtr<VkPipeline> handle;

public:
    GraphicsPipeline (VkDevice                                              device,
                      uint32_t                                              width,
                      uint32_t                                              height,
                      uint32_t                                              attachmentCount,
                      VkPipelineLayout                                      pipelineLayout,
                      VkRenderPass                                          renderPass,
                      const std::vector<VkPipelineShaderStageCreateInfo>&   shaderStages,
                      const std::vector<VkVertexInputBindingDescription>&   vertexBindingDescriptions,
                      const std::vector<VkVertexInputAttributeDescription>& vertexAttributeDescriptions,
                      VkPrimitiveTopology                                   topology,
                      bool                                                  blendEnabled = true);

    GraphicsPipeline (GraphicsPipeline&&) = default;
    GraphicsPipeline& operator= (GraphicsPipeline&&) = default;

    virtual ~GraphicsPipeline () override
    {
        vkDestroyPipeline (device, handle, nullptr);
        handle = nullptr;
    }
    
    virtual void* GetHandleForName () const override { return handle; }

    virtual operator VkPipeline () const override { return handle; }
};

} // namespace GVK

#endif