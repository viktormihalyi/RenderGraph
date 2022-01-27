#ifndef SHADERMODULE_HPP
#define SHADERMODULE_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include "RenderGraph/Utils/MovablePtr.hpp"

#include "ShaderReflection.hpp"
#include "VulkanObject.hpp"

#include <filesystem>

#include <vulkan/vulkan.h>

namespace GVK {

class RENDERGRAPH_DLL_EXPORT ShaderCompileException : public std::runtime_error {
public:
    ShaderCompileException (const std::string& errorMessage);
};

enum class ShaderKind : uint8_t {
    Vertex,
    Fragment,
    TessellationControl,
    TessellationEvaluation,
    Geometry,
    Compute,
};


RENDERGRAPH_DLL_EXPORT
std::string ShaderKindToString (ShaderKind);


struct RENDERGRAPH_DLL_EXPORT ShaderModuleReflection {
    std::vector<std::shared_ptr<SR::BufferObject>> ubos;
    std::vector<SR::Sampler>                       samplers;
    std::vector<std::shared_ptr<SR::BufferObject>> storageBuffers;
    std::vector<SR::Input>                         inputs;
    std::vector<SR::Output>                        outputs;
    std::vector<SR::SubpassInput>                  subpassInputs;

    ShaderModuleReflection (const std::vector<uint32_t>& binary);
};

class RENDERGRAPH_DLL_EXPORT ShaderModule : public VulkanObject {
public:
    static constexpr uint32_t ShaderKindCount = 6;

    enum class ReadMode {
        GLSLFilePath,
        SPVFilePath,
        GLSLString,
    };

private:
    VkDevice                        device;
    GVK::MovablePtr<VkShaderModule> handle;
    ReadMode                        readMode;
    ShaderKind                      shaderKind;

    std::string           sourceCode;
    std::filesystem::path fileLocation;
    std::vector<uint32_t> binary;

    std::vector<std::string> defines;
    std::vector<std::string> undefines;

    ShaderModuleReflection reflection;

private:
    // dont use this ctor, use factories instead
    ShaderModule (ShaderKind                   shaderKind,
                  ReadMode                     mode,
                  VkDevice                     device,
                  VkShaderModule               handle,
                  const std::filesystem::path& fileLocation,
                  const std::vector<uint32_t>& binary,
                  const std::string&              sourceCode,
                  const std::vector<std::string>& defines,
                  const std::vector<std::string>& undefines);

    ShaderModule (ShaderModule&&) = default;
    ShaderModule& operator= (ShaderModule&&) = default;

public:
    static std::unique_ptr<ShaderModule> CreateFromGLSLString (VkDevice                        device,
                                                               ShaderKind                      shaderKind,
                                                               const std::string&              shaderSource,
                                                               const std::vector<std::string>& defines   = {},
                                                               const std::vector<std::string>& undefines = {});

    static std::unique_ptr<ShaderModule> CreateFromGLSLFile (VkDevice                        device,
                                                             const std::filesystem::path&    fileLocation,
                                                             const std::vector<std::string>& defines   = {},
                                                             const std::vector<std::string>& undefines = {});

    static std::unique_ptr<ShaderModule> CreateFromSPVFile (VkDevice                        device,
                                                            ShaderKind                      shaderKind,
                                                            const std::filesystem::path&    fileLocation,
                                                            const std::vector<std::string>& defines   = {},
                                                            const std::vector<std::string>& undefines = {});

    virtual ~ShaderModule () override;

    virtual void* GetHandleForName () const override { return handle; }

    virtual VkObjectType GetObjectTypeForName () const override { return VK_OBJECT_TYPE_SHADER_MODULE; }

    void Reload ();

    const std::vector<uint32_t>& GetBinary () const { return binary; }

    const std::filesystem::path& GetLocation () const { return fileLocation; }

    ShaderKind GetShaderKind () const { return shaderKind; }

    ReadMode GetReadMode () const { return readMode; }

    VkPipelineShaderStageCreateInfo GetShaderStageCreateInfo () const;

    const ShaderModuleReflection& GetReflection () const { return reflection; }

    const std::string& GetSourceCode () const { return sourceCode; }
};

} // namespace GVK

#endif