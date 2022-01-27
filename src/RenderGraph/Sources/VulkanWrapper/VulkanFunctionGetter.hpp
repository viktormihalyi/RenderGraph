#ifndef VULKANWRAPPER_VULKANFUNCTIONGETTER_HPP
#define VULKANWRAPPER_VULKANFUNCTIONGETTER_HPP

#include <vulkan/vulkan.h>

#include <stdexcept>

// from Utils
#include "RenderGraph/Utils/Assert.hpp"

#include "spdlog/spdlog.h"


template<typename FunctionType>
FunctionType GetVulkanFunction (VkInstance instance, const char* functionName)
{
    FunctionType func = reinterpret_cast<FunctionType> (vkGetInstanceProcAddr (instance, functionName));

    if (GVK_ERROR (func == nullptr)) {
        spdlog::error ("Failed to load Vulkan function \"{}\".", functionName);
        throw std::runtime_error ("Function not loaded.");
    }

    return func;
}

#endif