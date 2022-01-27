#ifndef RENDERGRAPH_SHADERREFLECTIONTOVERTEXATTRIBUE_HPP
#define RENDERGRAPH_SHADERREFLECTIONTOVERTEXATTRIBUE_HPP

#include "RenderGraphExport.hpp"

#include "RenderGraph/VulkanWrapper/ShaderModule.hpp"

#include <vulkan/vulkan.h>

#include <vector>
#include <functional>
#include <string>


namespace RG {
namespace FromShaderReflection {

RENDERGRAPH_DLL_EXPORT
std::vector<VkVertexInputAttributeDescription> GetVertexAttributes (const RG::ShaderModuleReflection& reflection, const std::function<bool (const std::string&)>& instanceNameProvider);

RENDERGRAPH_DLL_EXPORT
std::vector<VkVertexInputBindingDescription> GetVertexBindings (const RG::ShaderModuleReflection& reflection, const std::function<bool (const std::string&)>& instanceNameProvider);

} // namespace FromShaderReflection
} // namespace RG

#endif
