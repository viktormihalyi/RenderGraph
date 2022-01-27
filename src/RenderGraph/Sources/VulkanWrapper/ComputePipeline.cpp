#include "ComputePipeline.hpp"
#include "ShaderModule.hpp"

#include "spdlog/spdlog.h"

namespace RG {

ComputePipeline::ComputePipeline (VkDevice            device,
                                  VkPipelineLayout    pipelineLayout,
                                  const ShaderModule& shaderModule)
    : device (device)
{
    VkComputePipelineCreateInfo createInfo = {};
    createInfo.sType                       = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    createInfo.pNext                       = nullptr;
    createInfo.flags                       = 0;
    createInfo.stage                       = shaderModule.GetShaderStageCreateInfo ();
    createInfo.layout                      = pipelineLayout;
    createInfo.basePipelineHandle          = VK_NULL_HANDLE;
    createInfo.basePipelineIndex           = -1;

    if (RG_ERROR (vkCreateComputePipelines (device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle) != VK_SUCCESS)) {
        spdlog::critical ("VkPipeline creation failed.");
        throw std::runtime_error ("failed to create pipeline");
    }

    spdlog::trace ("VkPipeline created: {}, uuid: {}.", handle, GetUUID ().GetValue ());
}

}
