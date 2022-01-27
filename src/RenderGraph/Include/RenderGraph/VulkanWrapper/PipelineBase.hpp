#ifndef VULKANWRAPPER_PIPELINEBASE_HPP
#define VULKANWRAPPER_PIPELINEBASE_HPP

#include "RenderGraph/RenderGraphExport.hpp"
#include <vulkan/vulkan.h>

#include "RenderGraph/Utils/Assert.hpp"
#include "RenderGraph/Utils/MovablePtr.hpp"
#include "RenderGraph/VulkanWrapper/VulkanObject.hpp"

namespace GVK {

class RENDERGRAPH_DLL_EXPORT PipelineBase : public VulkanObject {
public:
    PipelineBase () = default;

    virtual ~PipelineBase () override;

    PipelineBase (PipelineBase&&) = default;
    PipelineBase& operator= (PipelineBase&&) = default;

    virtual VkObjectType GetObjectTypeForName () const override { return VK_OBJECT_TYPE_PIPELINE; }

    virtual operator VkPipeline () const = 0;
};

} // namespace GVK

#endif