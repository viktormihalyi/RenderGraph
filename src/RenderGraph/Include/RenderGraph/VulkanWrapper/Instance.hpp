#ifndef INSTANCE_HPP
#define INSTANCE_HPP

#include "RenderGraph/RenderGraphExport.hpp"
#include "RenderGraph/Utils/MovablePtr.hpp"
#include "VulkanObject.hpp"

#include <vector>
#include <vulkan/vulkan.h>

namespace RG {

struct RENDERGRAPH_DLL_EXPORT InstanceSettings {
    std::vector<const char*> extensions;
    std::vector<const char*> layers;
};


extern RENDERGRAPH_DLL_EXPORT const InstanceSettings instanceDebugMode;
extern RENDERGRAPH_DLL_EXPORT const InstanceSettings instanceReleaseMode;


class RENDERGRAPH_DLL_EXPORT Instance : public VulkanObject, public Nonmovable {
private:
    RG::MovablePtr<VkInstance> handle;

public:
    Instance (const std::vector<const char*>& instanceExtensions, const std::vector<const char*>& instanceLayers);
    Instance (const InstanceSettings& settings);

    virtual ~Instance () override;

    virtual void* GetHandleForName () const override { return handle; }

    virtual VkObjectType GetObjectTypeForName () const override { return VK_OBJECT_TYPE_INSTANCE; }

    operator VkInstance () const { return handle; }
};

} // namespace RG

#endif