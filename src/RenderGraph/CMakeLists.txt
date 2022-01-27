add_library (RenderGraph SHARED)

set (Utils_Headers
    Include/RenderGraph/Utils/Assert.hpp
    Include/RenderGraph/Utils/BuildType.hpp
    Include/RenderGraph/Utils/CommandLineFlag.hpp
    Include/RenderGraph/Utils/CompilerDefinitions.hpp
    Include/RenderGraph/Utils/Event.hpp
    Include/RenderGraph/Utils/Lazy.hpp
    Include/RenderGraph/Utils/MessageBox.hpp
    Include/RenderGraph/Utils/MovablePtr.hpp
    Include/RenderGraph/Utils/MultithreadedFunction.hpp
    Include/RenderGraph/Utils/NoInline.hpp
    Include/RenderGraph/Utils/Noncopyable.hpp
    Include/RenderGraph/Utils/Platform.hpp
    Include/RenderGraph/Utils/SourceLocation.hpp
    Include/RenderGraph/Utils/StaticInit.hpp
    Include/RenderGraph/Utils/TerminalColors.hpp
    Include/RenderGraph/Utils/Time.hpp
    Include/RenderGraph/Utils/Timer.hpp
    Include/RenderGraph/Utils/Utils.hpp
    Include/RenderGraph/Utils/FileSystemUtils.hpp
    Include/RenderGraph/Utils/UUID.hpp
    Include/RenderGraph/Utils/SetupLogger.hpp
)

set (VulkanWrapper_Headers
    Include/RenderGraph/VulkanWrapper/Utils/BufferTransferable.hpp
    Include/RenderGraph/VulkanWrapper/Utils/ImageData.hpp
    Include/RenderGraph/VulkanWrapper/Utils/MemoryMapping.hpp
    Include/RenderGraph/VulkanWrapper/Utils/SingleTimeCommand.hpp
    Include/RenderGraph/VulkanWrapper/Utils/VulkanUtils.hpp

    Include/RenderGraph/VulkanWrapper/Allocator.hpp
    Include/RenderGraph/VulkanWrapper/Buffer.hpp
    Include/RenderGraph/VulkanWrapper/Command.hpp
    Include/RenderGraph/VulkanWrapper/CommandBuffer.hpp
    Include/RenderGraph/VulkanWrapper/CommandPool.hpp
    Include/RenderGraph/VulkanWrapper/Commands.hpp
    Include/RenderGraph/VulkanWrapper/ComputePipeline.hpp
    Include/RenderGraph/VulkanWrapper/PipelineBase.hpp
    Include/RenderGraph/VulkanWrapper/DebugReportCallback.hpp
    Include/RenderGraph/VulkanWrapper/DebugUtilsMessenger.hpp
    Include/RenderGraph/VulkanWrapper/DescriptorPool.hpp
    Include/RenderGraph/VulkanWrapper/DescriptorSet.hpp
    Include/RenderGraph/VulkanWrapper/DescriptorSetLayout.hpp
    Include/RenderGraph/VulkanWrapper/Device.hpp
    Include/RenderGraph/VulkanWrapper/DeviceExtra.hpp
    Include/RenderGraph/VulkanWrapper/DeviceMemory.hpp
    Include/RenderGraph/VulkanWrapper/Event.hpp
    Include/RenderGraph/VulkanWrapper/Fence.hpp
    Include/RenderGraph/VulkanWrapper/Framebuffer.hpp
    Include/RenderGraph/VulkanWrapper/GraphicsPipeline.hpp
    Include/RenderGraph/VulkanWrapper/Image.hpp
    Include/RenderGraph/VulkanWrapper/ImageView.hpp
    Include/RenderGraph/VulkanWrapper/Instance.hpp
    Include/RenderGraph/VulkanWrapper/PhysicalDevice.hpp
    Include/RenderGraph/VulkanWrapper/PipelineLayout.hpp
    Include/RenderGraph/VulkanWrapper/Queue.hpp
    Include/RenderGraph/VulkanWrapper/RenderPass.hpp
    Include/RenderGraph/VulkanWrapper/Sampler.hpp
    Include/RenderGraph/VulkanWrapper/Semaphore.hpp
    Include/RenderGraph/VulkanWrapper/ShaderModule.hpp
    Include/RenderGraph/VulkanWrapper/ShaderReflection.hpp
    Include/RenderGraph/VulkanWrapper/Surface.hpp
    Include/RenderGraph/VulkanWrapper/Swapchain.hpp
    Include/RenderGraph/VulkanWrapper/VulkanObject.hpp
    Include/RenderGraph/VulkanWrapper/VulkanWrapper.hpp
)

set (RenderGraph_Headers
    Include/RenderGraph/Drawable/Drawable.hpp
    Include/RenderGraph/Drawable/DrawableInfo.hpp
    Include/RenderGraph/Drawable/FullscreenQuad.hpp

    Include/RenderGraph/GraphRenderer.hpp
    Include/RenderGraph/GraphSettings.hpp
    Include/RenderGraph/DescriptorBindable.hpp
    Include/RenderGraph/Node.hpp
    Include/RenderGraph/Operation.hpp
    Include/RenderGraph/RenderGraph.hpp
    Include/RenderGraph/RenderGraphPass.hpp
    Include/RenderGraph/Resource.hpp
    Include/RenderGraph/ShaderPipeline.hpp
    Include/RenderGraph/ComputeShaderPipeline.hpp
    Include/RenderGraph/UniformReflection.hpp
    
    Include/RenderGraph/Window/GLFWWindow.hpp
    Include/RenderGraph/Window/SDLWindow.hpp
    Include/RenderGraph/Window/Window.hpp

    Include/RenderGraph/Font.hpp
    Include/RenderGraph/RenderGraphExport.hpp
    Include/RenderGraph/BufferView.hpp
    Include/RenderGraph/VulkanEnvironment.hpp

    Include/RenderGraph/ShaderReflectionToVertexAttribute.hpp
    Include/RenderGraph/ShaderReflectionToDescriptor.hpp
    Include/RenderGraph/ShaderReflectionToAttachment.hpp
)


set (VulkanWrapper_SourcesGroup_Internal
    Sources/VulkanWrapper/Utils/BufferTransferable.cpp
    Sources/VulkanWrapper/Utils/ImageData.cpp
    Sources/VulkanWrapper/Utils/MemoryMapping.cpp
    Sources/VulkanWrapper/Utils/VulkanUtils.cpp

    Sources/VulkanWrapper/Allocator.cpp
    Sources/VulkanWrapper/Buffer.cpp
    Sources/VulkanWrapper/CommandBuffer.cpp
    Sources/VulkanWrapper/Commands.cpp
    Sources/VulkanWrapper/ComputePipeline.cpp
    Sources/VulkanWrapper/PipelineBase.cpp
    Sources/VulkanWrapper/DebugReportCallback.cpp
    Sources/VulkanWrapper/DebugUtilsMessenger.cpp
    Sources/VulkanWrapper/Device.cpp
    Sources/VulkanWrapper/DeviceMemory.cpp
    Sources/VulkanWrapper/Event.cpp
    Sources/VulkanWrapper/Fence.cpp
    Sources/VulkanWrapper/Framebuffer.cpp
    Sources/VulkanWrapper/GraphicsPipeline.cpp
    Sources/VulkanWrapper/Image.cpp
    Sources/VulkanWrapper/ImageView.cpp
    Sources/VulkanWrapper/Instance.cpp
    Sources/VulkanWrapper/PhysicalDevice.cpp
    Sources/VulkanWrapper/Queue.cpp
    Sources/VulkanWrapper/ResourceLimits.cpp
    Sources/VulkanWrapper/Sampler.cpp
    Sources/VulkanWrapper/ShaderModule.cpp
    Sources/VulkanWrapper/ShaderReflection.cpp
    Sources/VulkanWrapper/Surface.cpp
    Sources/VulkanWrapper/Swapchain.cpp
    Sources/VulkanWrapper/VulkanObject.cpp
)

set (VulkanWrapper_SourcesGroup_STB Sources/VulkanWrapper/Utils/STBImpl.cpp)
set (VulkanWrapper_SourcesGroup_VMA Sources/VulkanWrapper/VulkanMemoryAllocatorImpl.cpp)

set (RenderGraph_Sources
    Sources/Drawable/Drawable.cpp
    Sources/Drawable/DrawableInfo.cpp

    Sources/GraphRenderer.cpp
    Sources/GraphSettings.cpp
    Sources/Operation.cpp
    Sources/RenderGraph.cpp
    Sources/RenderGraphPass.cpp
    Sources/Resource.cpp
    Sources/ShaderPipeline.cpp
    Sources/ComputeShaderPipeline.cpp
    Sources/UniformReflection.cpp

    Sources/Window/GLFWWindow.cpp
    Sources/Window/SDLWindow.cpp

    Sources/Font.cpp
    Sources/LogInitializer.cpp
    Sources/BufferView.cpp
    Sources/VulkanEnvironment.cpp
    
    Sources/ShaderReflectionToVertexAttribute.cpp
    Sources/ShaderReflectionToDescriptor.cpp
    Sources/ShaderReflectionToAttachment.cpp
)

set (Utils_Sources
    Sources/Utils/Assert.cpp
    Sources/Utils/CommandLineFlag.cpp
    Sources/Utils/MessageBox.cpp
    Sources/Utils/SourceLocation.cpp
    Sources/Utils/Time.cpp
    Sources/Utils/Utils.cpp
    Sources/Utils/FileSystemUtils.cpp
    Sources/Utils/UUID.cpp
    Sources/Utils/SetupLogger.cpp
)

target_sources (RenderGraph
    PRIVATE
        ${Utils_Headers}
        ${VulkanWrapper_Headers}
        ${RenderGraph_Headers}
        
        ${Utils_Sources}
        ${VulkanWrapper_SourcesGroup_Internal}
        ${VulkanWrapper_SourcesGroup_STB}
        ${VulkanWrapper_SourcesGroup_VMA}
        ${RenderGraph_Sources}
)

set_target_properties (RenderGraph PROPERTIES UNITY_BUILD_MODE GROUP)
set_source_files_properties (${Utils_Sources} ${RenderGraph_Sources} ${VulkanWrapper_SourcesGroup_Internal} PROPERTIES UNITY_GROUP "bucket_Internal")
set_source_files_properties (${VulkanWrapper_SourcesGroup_STB} PROPERTIES UNITY_GROUP "bucket_STB")
set_source_files_properties (${VulkanWrapper_SourcesGroup_VMA} PROPERTIES UNITY_GROUP "bucket_VMA")

find_package (Vulkan REQUIRED)

target_compile_definitions (RenderGraph
    PUBLIC SPDLOG_FMT_EXTERNAL
    PUBLIC GLM_FORCE_RADIANS
    PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE
)

target_precompile_headers (RenderGraph PRIVATE PrecompiledHeader.hpp)

FetchContent_Declare (
    glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
    GIT_TAG        925503088e2bcd76921b1e102c37fc320bace254
)

FetchContent_Declare (
    spirv-cross
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
    GIT_TAG        9acb9ec31f5a8ef80ea6b994bb77be787b08d3d1
)

FetchContent_Declare (
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        bf71a834948186f4097caa076cd2663c69a10e1e # 0.9.9.8
)

FetchContent_Declare (
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        7d5a16ce714f0b5f4efa3262de22e4d948851525 # 3.3.6
)

FetchContent_Declare (
    VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG        e9bfb539d853607172f9a1a272dacab60a86e529 # v2.3.0
)

FetchContent_Declare (
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        af1a5bc352164740c1cc1354942b1c6b72eacb8a
)

FetchContent_Declare (
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        eb3220622e73a4889eee355ffa37972b3cac3df5 # v1.9.2
)

FetchContent_Declare (
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        d141cdbeb0fb422a3fb7173b285fd38e0d1772dc # 8.0.1
)

set (GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set (SPIRV_CROSS_SHARED OFF CACHE BOOL "" FORCE)
set (SPIRV_CROSS_STATIC ON CACHE BOOL "" FORCE)
set (SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
set (SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)

set (SPIRV_CROSS_ENABLE_HLSL OFF CACHE BOOL "" FORCE)
set (SPIRV_CROSS_ENABLE_MSL OFF CACHE BOOL "" FORCE)
set (SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "" FORCE)

set (ENABLE_SPVREMAPPER OFF CACHE BOOL "" FORCE)
set (ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set (ENABLE_HLSL OFF CACHE BOOL "" FORCE)

set (SPDLOG_BUILD_SHARED ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable (glslang)
FetchContent_MakeAvailable (spirv-cross)
FetchContent_MakeAvailable (glm glfw)
FetchContent_MakeAvailable (VulkanMemoryAllocator stb)
FetchContent_MakeAvailable (fmt spdlog)

target_include_directories (RenderGraph
    PRIVATE
        Include/RenderGraph
        Include/RenderGraph/Drawable
        Include/RenderGraph/Window
        Include/RenderGraph/VulkanWrapper
        Include/RenderGraph/VulkanWrapper/Utils
        Include/RenderGraph/Utils
        
        $<TARGET_PROPERTY:glfw,INTERFACE_INCLUDE_DIRECTORIES>
        ${glslang_SOURCE_DIR}
        ${spirv-cross_SOURCE_DIR}
        ${stb_SOURCE_DIR}
    
    PUBLIC
        Include
        
        ${Vulkan_INCLUDE_DIRS}
        ${vulkanmemoryallocator_SOURCE_DIR}/src
        $<TARGET_PROPERTY:glm,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:fmt::fmt,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:spdlog::spdlog,INTERFACE_INCLUDE_DIRECTORIES>
)

if (WIN32)
    set (UUID_LIB rpcrt4)
elseif (UNIX)
    set (UUID_LIB uuid)
endif ()

target_link_libraries (RenderGraph
    PRIVATE
        ${UUID_LIB}

        glfw
        glslang
        SPIRV
        spirv-cross-core
        spdlog::spdlog

    PUBLIC
        ${Vulkan_LIBRARIES}

        fmt::fmt
)

if (MSVC)
target_compile_options (RenderGraph PRIVATE /W4 /WX)
target_compile_options (RenderGraph
    PRIVATE
        /wd4099     # PDB 'filename' was not found with 'object/library' or at 'path'; linking object as if no debug info
        /wd4251     # 'type' : class 'type1' needs to have dll-interface to be used by clients of class 'type2'
        /wd4275     # non - DLL-interface class 'class_1' used as base for DLL-interface class 'class_2'
        /wd26812    # The enum type type-name is unscoped. Prefer 'enum class' over 'enum' (Enum.3)
)
endif ()