#include "Fence.hpp"

#include "Utils/Assert.hpp"
#include "Utils/Time.hpp"

#include "spdlog/spdlog.h"

constexpr bool LOG_WAITS = false;


namespace GVK {

Fence::Fence (VkDevice device, bool signaled)
    : device (device)
    , handle (VK_NULL_HANDLE)
{
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags             = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    if (GVK_ERROR (vkCreateFence (device, &fenceInfo, nullptr, &handle) != VK_SUCCESS)) {
        spdlog::critical ("VkFence creation failed.");
        throw std::runtime_error ("failed to create fence");
    }

    spdlog::trace ("VkFence created: {}, uuid: {}.", handle, GetUUID ().GetValue ());
}


Fence::~Fence ()
{
    vkDestroyFence (device, handle, nullptr);
    handle = nullptr;
}


void Fence::WaitImpl () const
{
    vkWaitForFences (device, 1, &handle, VK_TRUE, UINT64_MAX);
}


void Fence::Wait () const
{
    if constexpr (LOG_WAITS) {
        
        const GVK::TimePoint start = GVK::TimePoint::SinceEpoch ();

        WaitImpl ();
        
        const GVK::TimePoint end = GVK::TimePoint::SinceEpoch ();

        const std::string fenceId = !GetName ().empty () ? GetName () : GetUUID ().GetValue ();
        spdlog::info  ("fence \"{}\" waited {} ms", fenceId, (end - start).AsMilliseconds ());

    } else {
        
        WaitImpl ();

    }
}


void Fence::Reset () const
{
    vkResetFences (device, 1, &handle);
}

} // namespace GVK
