#include "ShaderModule.hpp"

// from Utils
#include "Utils/Assert.hpp"
#include "Utils/BuildType.hpp"
#include "Utils/CommandLineFlag.hpp"
#include "Utils/FileSystemUtils.hpp"

// from VulkanWrapper
#include "ResourceLimits.hpp"
#include "ShaderReflection.hpp"

// from std
#include <array>

// from glslang
#include "glslang/SPIRV/GlslangToSpv.h"

// from spdlog
#include "spdlog/spdlog.h"

#if 0
static RG::CommandLineOnOffFlag disableShaderCacheFlag ("--disableShaderCache");
#endif

namespace RG {

    
ShaderCompileException::ShaderCompileException (const std::string& errorMessage)
    : std::runtime_error (errorMessage)
{
}

class ShaderKindDescriptor final {
public:
    const char*           extension;
    VkShaderStageFlagBits vkflag;
    ShaderKind            shaderKind;
    EShLanguage           esh;
    const char*           displayName;

private:
    ShaderKindDescriptor (const char*           extension,
                       VkShaderStageFlagBits vkflag,
                       ShaderKind            shaderKind,
                       EShLanguage           esh,
                       const char*           displayName);

public:
    static std::optional<ShaderKindDescriptor> FromExtension (const std::string&);
    static std::optional<ShaderKindDescriptor> FromVk (VkShaderStageFlagBits);
    static std::optional<ShaderKindDescriptor> FromShaderKind (ShaderKind);
    static std::optional<ShaderKindDescriptor> FromEsh (EShLanguage);

private:
    static std::optional<ShaderKindDescriptor> Find (const std::function<bool (const ShaderKindDescriptor&)>&);
    static const ShaderKindDescriptor vert, tesc, tese, geom, frag, comp;

    static const std::array<ShaderKindDescriptor, 6> allShaderKinds;
};


ShaderKindDescriptor::ShaderKindDescriptor (const char*           extension,
                                      VkShaderStageFlagBits vkflag,
                                      ShaderKind            shaderKind,
                                      EShLanguage           esh,
                                      const char*           displayName)
    : extension { extension }
    , vkflag { vkflag }
    , shaderKind { shaderKind }
    , esh { esh }
    , displayName { displayName }
{
}


const ShaderKindDescriptor ShaderKindDescriptor::vert {
    ".vert",
    VK_SHADER_STAGE_VERTEX_BIT,
    ShaderKind::Vertex,
    EShLangVertex,
    "Vertex Shader"
};


const ShaderKindDescriptor ShaderKindDescriptor::tesc {
    ".tesc",
    VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
    ShaderKind::TessellationControl,
    EShLangTessControl,
    "Tessellation Control Shader"
};


const ShaderKindDescriptor ShaderKindDescriptor::tese {
    ".tese",
    VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
    ShaderKind::TessellationEvaluation,
    EShLangTessEvaluation,
    "Tessellation Evaluation Shader"
};


const ShaderKindDescriptor ShaderKindDescriptor::geom {
    ".geom",
    VK_SHADER_STAGE_GEOMETRY_BIT,
    ShaderKind::Geometry,
    EShLangGeometry,
    "Geometry Shader"
};


const ShaderKindDescriptor ShaderKindDescriptor::frag {
    ".frag",
    VK_SHADER_STAGE_FRAGMENT_BIT,
    ShaderKind::Fragment,
    EShLangFragment,
    "Fragment Shader"
};


const ShaderKindDescriptor ShaderKindDescriptor::comp {
    ".comp",
    VK_SHADER_STAGE_COMPUTE_BIT,
    ShaderKind::Compute,
    EShLangCompute,
    "Compute Shader"
};


const std::array<ShaderKindDescriptor, 6> ShaderKindDescriptor::allShaderKinds ({
    ShaderKindDescriptor::vert,
    ShaderKindDescriptor::tese,
    ShaderKindDescriptor::tesc,
    ShaderKindDescriptor::geom,
    ShaderKindDescriptor::frag,
    ShaderKindDescriptor::comp,
});


std::optional<ShaderKindDescriptor> ShaderKindDescriptor::Find (const std::function<bool (const ShaderKindDescriptor&)>& callback)
{
    for (auto& s : allShaderKinds) {
        if (callback (s)) {
            return s;
        }
    }

    return std::nullopt;
}

std::optional<ShaderKindDescriptor> ShaderKindDescriptor::FromExtension (const std::string& ext)
{
    return Find ([&] (auto& s) {
        return s.extension == ext;
    });
}

std::optional<ShaderKindDescriptor> ShaderKindDescriptor::FromVk (VkShaderStageFlagBits flag)
{
    return Find ([&] (auto& s) {
        return s.vkflag == flag;
    });
}

std::optional<ShaderKindDescriptor> ShaderKindDescriptor::FromShaderKind (ShaderKind sk)
{
    return Find ([&] (auto& s) {
        return s.shaderKind == sk;
    });
}

std::optional<ShaderKindDescriptor> ShaderKindDescriptor::FromEsh (EShLanguage e)
{
    return Find ([&] (auto& s) {
        return s.esh == e;
    });
}


#if 0

class ShaderCache {
private:
    const std::filesystem::path tempFolder;

public:
    ShaderCache (const std::string& typeName)
        : tempFolder (std::filesystem::temp_directory_path () / "GearsVk" / "ShaderCache" / typeName)
    {
    }

private:
    std::string GetShaderSourceHash (const std::string& sourceCode) const
    {
        std::hash<std::string> sourceCodeHasher;

        const size_t sourceCodeHash = sourceCodeHasher (sourceCode);

        return std::to_string (sourceCodeHash);
    };

public:
    std::optional<std::vector<uint32_t>> Load (const std::string& sourceCode) const
    {
        if (disableShaderCacheFlag.IsFlagOn ()) {
            return std::nullopt;
        }

        const std::string hashString = GetShaderSourceHash (sourceCode);

        const std::string cachedCodeFileName   = hashString + "_code.txt";
        const std::string cachedBinaryFileName = hashString + "_binary.txt";

        if (!std::filesystem::exists (tempFolder / cachedCodeFileName)) {
            return std::nullopt;
        }

        const std::optional<std::string> actualCode = RG::ReadTextFile (tempFolder / cachedCodeFileName);
        if (RG_ERROR (!actualCode)) {
            return std::nullopt;
        }

        if (sourceCode != *actualCode) {
            return std::nullopt;
        }

        if (!std::filesystem::exists (tempFolder / cachedBinaryFileName)) {
            return std::nullopt;
        }

        std::optional<std::vector<char>> binaryC = RG::ReadBinaryFile (tempFolder / cachedBinaryFileName);
        if (RG_ERROR (!binaryC.has_value ())) {
            return std::nullopt;
        }

        std::vector<uint32_t> code;
        code.resize (binaryC->size () / sizeof (uint32_t));
        memcpy (code.data (), binaryC->data (), binaryC->size ());

        return code;
    }

    bool Save (const std::string& sourceCode, const std::vector<uint32_t>& binary) const
    {
        if (disableShaderCacheFlag.IsFlagOn ()) {
            return false;
        }

        const std::string hashString = GetShaderSourceHash (sourceCode);

        const std::string cachedCodeFileName   = hashString + "_code.txt";
        const std::string cachedBinaryFileName = hashString + "_binary.txt";

        if (std::filesystem::exists (tempFolder / cachedCodeFileName) || std::filesystem::exists (tempFolder / cachedBinaryFileName)) {
            return false;
        }

        RG_VERIFY (RG::WriteTextFile (tempFolder / cachedCodeFileName, sourceCode));
        RG_VERIFY (RG::WriteBinaryFile (tempFolder / cachedBinaryFileName, binary.data (), binary.size () * sizeof (uint32_t)));

        return true;
    }
};

ShaderCache debugShaderCache ("Debug");
ShaderCache releaseShaderCache ("Release");

#endif


namespace {

// from https://github.com/KhronosGroup/glslang/blob/master/StandAlone/StandAlone.cpp
class TPreamble {
private:
    std::string              text;
    std::vector<std::string> processes; 

public:
    TPreamble () = default;

    bool IsSet () const { return !text.empty (); }

    const char* GetText () const { return text.c_str (); }

    const std::vector<std::string>& GetProcesses () const { return processes; }

    void AddDef (const std::string& def_)
    {
        std::string def = def_;

        text.append ("#define ");
        FixLine (def);

        processes.push_back ("define-macro ");
        processes.back ().append (def);

        // The first "=" needs to turn into a space
        const size_t equal = def.find_first_of ("=");
        if (equal != def.npos)
            def[equal] = ' ';

        text.append (def);
        text.append ("\n");
    }

    void AddUndef (const std::string& undef_)
    {
        std::string undef = undef_;

        text.append ("#undef ");
        FixLine (undef);

        processes.push_back ("undef-macro ");
        processes.back ().append (undef);

        text.append (undef);
        text.append ("\n");
    }

private:
    void FixLine (std::string& line)
    {
        // Can't go past a newline in the line
        const size_t end = line.find_first_of ("\n");
        if (end != line.npos)
            line = line.substr (0, end);
    }
};

} // namespace


struct CompileParameters {
    std::string                         sourceCode;
    std::optional<ShaderKindDescriptor> shaderKindDescriptor;
    std::vector<std::string>            defines;
    std::vector<std::string>            undefines;
};


// TODO ennek nem kene itt lennie
extern RG::CommandLineOnOffFlag enableShaderPrintfFlag;


static std::vector<uint32_t> CompileWithGlslangCppInterface (CompileParameters params)
{
    static bool init = false;
    if (!init) {
        init = true;
        glslang::InitializeProcess ();
    }

    if (enableShaderPrintfFlag.IsFlagOn ())
        params.defines.push_back ("SHADERPRINTF");

    if (RG_ERROR (params.sourceCode.empty ()))
        throw ShaderCompileException ("No shader source provided.");

    if (RG_ERROR (!params.shaderKindDescriptor.has_value ()))
        throw ShaderCompileException ("No shader kind provided.");

    const char* const                       sourceCstr                  = params.sourceCode.c_str ();
    const int                               ClientInputSemanticsVersion = 100;
    const glslang::EShTargetClientVersion   VulkanClientVersion         = glslang::EShTargetVulkan_1_2;
    const glslang::EShTargetLanguageVersion TargetVersion               = glslang::EShTargetSpv_1_5;
    const EShMessages                       messages                    = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules | EShMsgDebugInfo); // debug info is required for release builds as well for variable names
    const TBuiltInResource                  resources                   = GetDefaultResourceLimits (); // TODO use DefaultTBuiltInResource ?

    TPreamble preamble;
    for (const std::string& def : params.defines)
        preamble.AddDef (def);
    for (const std::string& undef : params.undefines)
        preamble.AddUndef (undef);


    glslang::TShader shader (params.shaderKindDescriptor->esh);
    shader.setStrings (&sourceCstr, 1);
    shader.setEnvInput (glslang::EShSourceGlsl, params.shaderKindDescriptor->esh, glslang::EShClientVulkan, ClientInputSemanticsVersion);
    shader.setEnvClient (glslang::EShClientVulkan, VulkanClientVersion);
    shader.setEnvTarget (glslang::EShTargetSpv, TargetVersion);

    if (preamble.IsSet ())
        shader.setPreamble (preamble.GetText ());
    shader.addProcesses (preamble.GetProcesses ());

    if (!shader.parse (&resources, 100, false, messages)) {
        throw ShaderCompileException (std::string { "Failed to parse " } + params.shaderKindDescriptor->displayName + ":\n"
                                     + "================================== GLSL CODE BEGIN ==================================\n"
                                     + params.sourceCode + "\n"
                                     + "================================== GLSL CODE END ====================================\n"
                                     + shader.getInfoLog ());
    }

    glslang::TProgram program;
    program.addShader (&shader);

    if (!program.link (EShMsgDefault)) {
        throw ShaderCompileException (std::string { "Failed to link " } + params.shaderKindDescriptor->displayName + ":\n"
                                     + "================================== GLSL CODE BEGIN ==================================\n"
                                     + params.sourceCode + "\n"
                                     + "================================== GLSL CODE END ====================================\n"
                                     + shader.getInfoLog ());
    }

    spv::SpvBuildLogger logger;
    glslang::SpvOptions spvOptions;
    spvOptions.generateDebugInfo = IsDebugBuild; // why is this not needef for release builds?
    spvOptions.stripDebugInfo    = false; // this is needed for shader resource names
    spvOptions.disableOptimizer  = IsDebugBuild;
    spvOptions.optimizeSize      = false;
    spvOptions.disassemble       = false;
    spvOptions.validate          = IsDebugBuild;

    std::vector<uint32_t> spirvBinary;
    glslang::GlslangToSpv (*program.getIntermediate (params.shaderKindDescriptor->esh), spirvBinary, &logger, &spvOptions);

    const std::string loggerMessages = logger.getAllMessages ();
    if (RG_ERROR (!loggerMessages.empty ()))
        spdlog::error ("{}", loggerMessages);

    return spirvBinary;
}


static std::vector<uint32_t> CompileFromSourceCode (const CompileParameters& params)
{
#if 0
    ShaderCache& shaderCache = IsDebugBuild ? debugShaderCache : releaseShaderCache;

    std::optional<std::vector<uint32_t>> cachedBinary = shaderCache.Load (shaderSource);
    if (cachedBinary.has_value ()) {
        return *cachedBinary;
    }
#endif

    try {
        const std::vector<uint32_t> result = CompileWithGlslangCppInterface (params);
#if 0
        shaderCache.Save (shaderSource, result);
#endif
        return result;

    } catch (ShaderCompileException& /* ex */) {
        throw;
    }
}


static VkShaderModule CreateShaderModuleImpl (VkDevice device, const std::vector<uint32_t>& binary)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize                 = static_cast<uint32_t> (binary.size () * sizeof (uint32_t)); // size must be in bytes
    createInfo.pCode                    = reinterpret_cast<const uint32_t*> (binary.data ());

    VkShaderModule result = VK_NULL_HANDLE;
    if (RG_ERROR (vkCreateShaderModule (device, &createInfo, nullptr, &result) != VK_SUCCESS)) {
        spdlog::critical ("VkShaderModule creation failed.");
        throw std::runtime_error ("failed to create shader module");
    }
    
    return result;
}


static VkShaderModule CreateShaderModuleImpl (VkDevice device, const std::vector<char>& binary)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize                 = binary.size ();
    createInfo.pCode                    = reinterpret_cast<const uint32_t*> (binary.data ());

    VkShaderModule result = VK_NULL_HANDLE;
    if (RG_ERROR (vkCreateShaderModule (device, &createInfo, nullptr, &result) != VK_SUCCESS)) {
        spdlog::critical ("VkShaderModule creation failed.");
        throw std::runtime_error ("failed to create shader module");
    }

    return result;
}


ShaderModule::ShaderModule (ShaderKind                      shaderKind,
                            ReadMode                        readMode,
                            VkDevice                        device,
                            VkShaderModule                  handle,
                            const std::filesystem::path&    fileLocation,
                            const std::vector<uint32_t>&    binary,
                            const std::string&              sourceCode,
                            const std::vector<std::string>& defines,
                            const std::vector<std::string>& undefines)
    : readMode (readMode)
    , shaderKind (shaderKind)
    , device (device)
    , handle (handle)
    , fileLocation (fileLocation)
    , binary (binary)
    , reflection (binary)
    , sourceCode (sourceCode)
    , defines (defines)
    , undefines (undefines)
{
    spdlog::trace ("VkShaderModule created: {}, uuid: {}.", this->handle, GetUUID ().GetValue ());
}


std::unique_ptr<ShaderModule> ShaderModule::CreateFromSPVFile (VkDevice device, ShaderKind shaderKind, const std::filesystem::path& fileLocation, const std::vector<std::string>& defines, const std::vector<std::string>& undefines)
{
    std::optional<std::vector<char>> binaryC = RG::ReadBinaryFile (fileLocation);
    if (RG_ERROR (!binaryC.has_value ())) {
        throw std::runtime_error ("failed to read shader");
    }

    std::vector<uint32_t> code;
    code.resize (binaryC->size () / sizeof (uint32_t));
    memcpy (code.data (), binaryC->data (), binaryC->size ());

    VkShaderModule handle = CreateShaderModuleImpl (device, *binaryC);

    return std::unique_ptr<ShaderModule> (new ShaderModule (shaderKind, ReadMode::SPVFilePath, device, handle, fileLocation, code, "", defines, undefines));
}


std::unique_ptr<ShaderModule> ShaderModule::CreateFromGLSLFile (VkDevice device, const std::filesystem::path& fileLocation, const std::vector<std::string>& defines, const std::vector<std::string>& undefines)
{
    std::optional<std::string> fileContents = RG::ReadTextFile (fileLocation);
    if (RG_ERROR (!fileContents.has_value ())) {
        throw std::runtime_error ("Failed to read file.");
    }

    std::optional<ShaderKindDescriptor> shaderKindDescriptor = ShaderKindDescriptor::FromExtension (fileLocation.extension ().string ());
    if (RG_ERROR (!shaderKindDescriptor.has_value ())) {
        throw std::runtime_error ("Unknown shader file extension.");
    }

    CompileParameters parameters;
    parameters.sourceCode           = *fileContents;
    parameters.shaderKindDescriptor = *shaderKindDescriptor;
    parameters.defines              = {};
    parameters.undefines            = {};

    std::optional<std::vector<uint32_t>> binary = CompileFromSourceCode (parameters);
    if (RG_ERROR (!binary.has_value ())) {
        throw std::runtime_error ("failed to compile shader");
    }

    VkShaderModule handle = CreateShaderModuleImpl (device, *binary);
    
    return std::unique_ptr<ShaderModule> (new ShaderModule (
        shaderKindDescriptor->shaderKind,
        ReadMode::GLSLFilePath,
        device,
        handle,
        fileLocation,
        *binary,
        *fileContents,
        defines,
        undefines));
}


std::unique_ptr<ShaderModule> ShaderModule::CreateFromGLSLString (VkDevice device, ShaderKind shaderKind, const std::string& shaderSource, const std::vector<std::string>& defines, const std::vector<std::string>& undefines)
{
    std::optional<ShaderKindDescriptor> shaderKindDescriptor = ShaderKindDescriptor::FromShaderKind (shaderKind);
    if (RG_ERROR (!shaderKindDescriptor.has_value ())) {
        throw std::runtime_error ("Unknown shaderkind.");
    }

    CompileParameters parameters;
    parameters.sourceCode           = shaderSource;
    parameters.shaderKindDescriptor = *shaderKindDescriptor;
    parameters.defines              = defines;
    parameters.undefines            = undefines;

    std::vector<uint32_t> binary = CompileFromSourceCode (parameters);

    VkShaderModule handle = CreateShaderModuleImpl (device, binary);
    
    return std::unique_ptr<ShaderModule> (new ShaderModule (
        shaderKind,
        ReadMode::GLSLString,
        device,
        handle,
        "",
        binary,
        shaderSource,
        defines,
        undefines));
}


ShaderModule::~ShaderModule ()
{
    vkDestroyShaderModule (device, handle, nullptr);
    handle = nullptr;
}


VkPipelineShaderStageCreateInfo ShaderModule::GetShaderStageCreateInfo () const
{
    VkPipelineShaderStageCreateInfo result = {};
    result.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    result.pNext                           = nullptr;
    result.flags                           = 0;
    result.stage                           = ShaderKindDescriptor::FromShaderKind (shaderKind)->vkflag;
    result.module                          = handle;
    result.pName                           = "main";
    result.pSpecializationInfo             = nullptr;
    return result;
}


ShaderModuleReflection::ShaderModuleReflection (const std::vector<uint32_t>& binary)
{
    RG::Refl::SpirvParser c (binary);

    ubos           = RG::Refl::GetUBOsFromBinary (c);
    samplers       = RG::Refl::GetSamplersFromBinary (c);
    storageBuffers = RG::Refl::GetStorageBuffersFromBinary (c);
    inputs         = RG::Refl::GetInputsFromBinary (c);
    outputs        = RG::Refl::GetOutputsFromBinary (c);
    subpassInputs  = RG::Refl::GetSubpassInputsFromBinary (c);
}


void ShaderModule::Reload ()
{
    if (readMode == ReadMode::GLSLFilePath) {
        vkDestroyShaderModule (device, handle, nullptr);

        std::optional<std::string> fileContents = RG::ReadTextFile (fileLocation);
        if (RG_ERROR (!fileContents.has_value ())) {
            throw std::runtime_error ("failed to read shader");
        }

        std::optional<ShaderKindDescriptor> shaderKindDescriptor = ShaderKindDescriptor::FromExtension (fileLocation.extension ().string ());
        if (RG_ERROR (!shaderKindDescriptor.has_value ())) {
            return;
        }

        CompileParameters parameters;
        parameters.sourceCode           = *fileContents;
        parameters.shaderKindDescriptor = *shaderKindDescriptor;
        parameters.defines              = defines;
        parameters.undefines            = undefines;

        std::optional<std::vector<uint32_t>> newBinary = CompileFromSourceCode (parameters);
        if (RG_ERROR (!newBinary.has_value ())) {
            throw std::runtime_error ("failed to compile shader");
        }

        handle = CreateShaderModuleImpl (device, *newBinary);

        binary = *newBinary;

        reflection = ShaderModuleReflection (binary);

        sourceCode = *fileContents;

    } else if (readMode == ReadMode::SPVFilePath) {
        vkDestroyShaderModule (device, handle, nullptr);

        std::optional<std::vector<char>> binaryC = RG::ReadBinaryFile (fileLocation);
        if (RG_ERROR (!binaryC.has_value ())) {
            throw std::runtime_error ("failed to read shader");
        }

        std::vector<uint32_t> code;
        code.resize (binaryC->size () / sizeof (uint32_t));
        memcpy (code.data (), binaryC->data (), binaryC->size ());

        handle = CreateShaderModuleImpl (device, *binaryC);

        reflection = ShaderModuleReflection (binary);

        binary = code;

    } else if (readMode == ReadMode::GLSLString) {
        RG_BREAK_STR ("cannot reload shaders from hard coded strings");

    } else {
        RG_BREAK_STR ("unknown read mode");
    }
}


std::string ShaderKindToString (ShaderKind shaderKind)
{
    switch (shaderKind) {
        case ShaderKind::Fragment: return "Fragment";
        case ShaderKind::Vertex: return "Vertex";
        case ShaderKind::TessellationControl: return "TessellationControl";
        case ShaderKind::TessellationEvaluation: return "TessellationEvaluation";
        case ShaderKind::Geometry: return "Geometry";
        case ShaderKind::Compute: return "Compute";
        default:
            RG_BREAK_STR ("unexpected shaderkind type");
            return "";
    }
}

} // namespace RG