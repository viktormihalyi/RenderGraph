#include "CommandBuffer.hpp"
#include "Image.hpp"
#include "Utils/Assert.hpp"

#include <iostream>

#include "spdlog/spdlog.h"

namespace GVK {

    
CommandBuffer::CommandBuffer (VkDevice device, VkCommandPool commandPool)
    : device (device)
    , commandPool (commandPool)
    , handle (VK_NULL_HANDLE)
    , canRecordCommands (false)
{
    VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
    commandBufferAllocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocInfo.commandPool                 = commandPool;
    commandBufferAllocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocInfo.commandBufferCount          = 1;

    if (GVK_ERROR (vkAllocateCommandBuffers (device, &commandBufferAllocInfo, &handle) != VK_SUCCESS)) {
        spdlog::critical ("VkCommandBuffer creation failed.");
        throw std::runtime_error ("failed to allocate command buffer");
    }

    spdlog::trace ("VkCommandBuffer created: {}, uuid: {}.", handle, GetUUID ().GetValue ());
}


CommandBuffer::CommandBuffer (const DeviceExtra& device)
    : CommandBuffer (device, device.GetCommandPool ())
{
}


CommandBuffer::~CommandBuffer ()
{
    if (handle != VK_NULL_HANDLE) {
        vkFreeCommandBuffers (device, commandPool, 1, &handle);
        handle = VK_NULL_HANDLE;
    }
}


void CommandBuffer::Begin (VkCommandBufferUsageFlags flags)
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags                    = flags;
    beginInfo.pInheritanceInfo         = nullptr;

    if (GVK_ERROR (vkBeginCommandBuffer (handle, &beginInfo) != VK_SUCCESS)) {
        throw std::runtime_error ("commandbuffer begin failed");
    }

    canRecordCommands = true;
}


void CommandBuffer::End ()
{
    if (GVK_ERROR (vkEndCommandBuffer (handle) != VK_SUCCESS)) {
        throw std::runtime_error ("commandbuffer end failed");
    }

    canRecordCommands = false;
}


void CommandBuffer::Reset (bool releaseResources)
{
    if (GVK_ERROR (vkResetCommandBuffer (handle, (releaseResources) ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT : 0) != VK_SUCCESS)) {
        throw std::runtime_error ("commandbuffer reset failed");
    }

    canRecordCommands = false;
}


Command& CommandBuffer::RecordCommand (std::unique_ptr<Command>&& command)
{
    command->Record (*this);
    recordedAbstractCommands.push_back (std::move (command));
    return *recordedAbstractCommands.back ();
}

}