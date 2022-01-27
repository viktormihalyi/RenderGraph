#include "VulkanWrapper/Event.hpp"
#include "Utils/Assert.hpp"

#include <stdexcept>

namespace VW {

Event::Event (VkDevice device)
    : device (device)
{
    VkEventCreateInfo createInfo = {};
    createInfo.sType             = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    createInfo.pNext             = nullptr;
    createInfo.flags             = 0;

    if (RG_ERROR (vkCreateEvent (device, &createInfo, nullptr, &handle) != VK_SUCCESS)) {
        throw std::runtime_error ("failed to create vkevent");
    }
}


Event::~Event ()
{
    vkDestroyEvent (device, handle, nullptr);
    handle = nullptr;
}

} // namespace VW
