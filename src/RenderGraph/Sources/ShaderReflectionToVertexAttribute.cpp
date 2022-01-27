#include "ShaderReflectionToVertexAttribute.hpp"

#include "Utils/Assert.hpp"


namespace RG {
namespace FromShaderReflection {

std::vector<VkVertexInputAttributeDescription> GetVertexAttributes (const RG::ShaderModuleReflection& reflection, const std::function<bool (const std::string&)>& IsInputInstanced)
{
    std::vector<VkVertexInputAttributeDescription> result;

    uint32_t currentOffsetVertex   = 0;
    uint32_t currentOffsetInstance = 0;

    for (const RG::Refl::Input& input : reflection.inputs) {
        VkVertexInputAttributeDescription attrib = {};
        attrib.binding                           = 0;
        attrib.location                          = input.location;
        attrib.format                            = FieldTypeToVkFormat (input.type);

        if (IsInputInstanced (input.name)) {
            attrib.offset = currentOffsetInstance;
            currentOffsetInstance += input.sizeInBytes;
        } else {
            attrib.offset = currentOffsetVertex;
            currentOffsetVertex += input.sizeInBytes;
        }

        result.push_back (attrib);
    }

    return result;
}


std::vector<VkVertexInputBindingDescription> GetVertexBindings (const RG::ShaderModuleReflection& reflection, const std::function<bool (const std::string&)>& IsInputInstanced)
{
    if (reflection.inputs.empty ()) {
        return {};
    }

    std::vector<VkVertexInputBindingDescription> result;

    uint32_t fullSizeVertex   = 0;
    uint32_t fullSizeInstance = 0;

    for (const RG::Refl::Input& input : reflection.inputs) {
        if (IsInputInstanced (input.name)) {
            fullSizeInstance += input.sizeInBytes;
        } else {
            fullSizeVertex += input.sizeInBytes;
        }
    }

    {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription                                 = {};
        bindingDescription.binding                         = 0;
        bindingDescription.stride                          = fullSizeVertex;
        bindingDescription.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;
        result.push_back (bindingDescription);
    }

    if (fullSizeInstance > 0) {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription                                 = {};
        bindingDescription.binding                         = 0;
        bindingDescription.stride                          = fullSizeInstance;
        bindingDescription.inputRate                       = VK_VERTEX_INPUT_RATE_INSTANCE;
        result.push_back (bindingDescription);
    }

    return result;
}

} // namespace FromShaderReflection
} // namespace RG
