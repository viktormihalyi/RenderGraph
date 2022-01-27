#ifndef VULKANOBJECT_HPP
#define VULKANOBJECT_HPP

#include "RenderGraph/RenderGraphExport.hpp"
#include "RenderGraph/Utils/UUID.hpp"
#include "RenderGraph/Utils/Noncopyable.hpp"

#include <vulkan/vulkan.h>


namespace RG {

class DeviceExtra;

class RENDERGRAPH_DLL_EXPORT VulkanObject : public Noncopyable {
private:
    RG::UUID   uuid;
    std::string name;

protected:
    VulkanObject ();

    VulkanObject (const VulkanObject&) = delete;
    VulkanObject& operator= (const VulkanObject&) = delete;

    VulkanObject (VulkanObject&&) = default;
    VulkanObject& operator= (VulkanObject&&) = default;

public:
    virtual ~VulkanObject () override;

    void SetName (const DeviceExtra& device, const std::string& value);

    const std::string& GetName () const { return name; }

    const RG::UUID& GetUUID () const { return uuid; }

private:
    virtual void*        GetHandleForName () const     = 0;
    virtual VkObjectType GetObjectTypeForName () const = 0;
};

} // namespace RG

#endif