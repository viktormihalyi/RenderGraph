#include "TestEnvironment.hpp"

#include "RenderGraph/BufferView.hpp"
#include "RenderGraph/Drawable/FullscreenQuad.hpp"
#include "RenderGraph/GraphRenderer.hpp"
#include "RenderGraph/GraphSettings.hpp"
#include "RenderGraph/Operation.hpp"
#include "RenderGraph/RenderGraph.hpp"
#include "RenderGraph/Resource.hpp"
#include "RenderGraph/ShaderPipeline.hpp"
#include "RenderGraph/UniformReflection.hpp"
#include "RenderGraph/VulkanEnvironment.hpp"
#include "RenderGraph/Window/GLFWWindow.hpp"


#include "RenderGraph/Utils/SourceLocation.hpp"
#include "RenderGraph/Utils/Timer.hpp"
#include "RenderGraph/Utils/Utils.hpp"

#include "RenderGraph/VulkanWrapper/Allocator.hpp"
#include "RenderGraph/VulkanWrapper/Commands.hpp"
#include "RenderGraph/VulkanWrapper/DeviceExtra.hpp"
#include "RenderGraph/VulkanWrapper/Utils/ImageData.hpp"
#include "RenderGraph/VulkanWrapper/Utils/VulkanUtils.hpp"
#include "RenderGraph/VulkanWrapper/VulkanWrapper.hpp"


#include <glm/glm.hpp>

#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>


const std::filesystem::path ShadersFolder = std::filesystem::current_path () / "TestData" / "shaders";


TEST_F (HeadlessTestEnvironment, DISABLED_LCGShader)
{
    const std::string fragSrc = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_shader_explicit_arithmetic_types : enable

layout (location = 0) in vec2 textureCoords;
layout (location = 0) out vec4 outColor;

uint64_t Forrest_G (const uint64_t k, const uint64_t seed, const uint64_t g, const uint64_t m)
{
    uint64_t G = seed % m;
    uint64_t h = g;

    uint64_t i = (k + m) % m;

    while (i > 0) {
        if (i % 2 == 1) {
            G = (G * h) % m;
        }
        h = (h * h) % m;
        i = i / 2;
    }

    return G;
}

uint64_t Forrest_C (const uint64_t k, const uint64_t seed, const uint64_t g, const uint64_t c, const uint64_t m)
{
    uint64_t C = seed % m;
    uint64_t f = c;
    uint64_t h = g;
    uint64_t i = (k + m) % m;

    while (i > 0) {
        if (i % 2 == 1) {
            C = (C * h + f) % m;
        }
        f = (f * (h + 1)) % m;
        h = (h * h) % m;
        i = i / 2;
    }

    return C;
}

void main ()
{
    float rng = float (Forrest_C (int (textureCoords.y * 800 * 600 + textureCoords.x * 600), 456, 48271, 0, 2147483647)) / float(2147483647);
 
   outColor = vec4 (vec3 (rng), 1);
}
    )";

    std::shared_ptr<RG::RenderOperation> redFillOperation = RG::RenderOperation::Builder (GetDevice ())
                                                                .SetVertices (std::make_unique<RG::DrawableInfo> (1, 6))
                                                                .SetPrimitiveTopology (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                                .SetVertexShader (passThroughVertexShader)
                                                                .SetFragmentShader (fragSrc)
                                                                .Build ();

    std::shared_ptr<RG::WritableImageResource> red = std::make_unique<RG::WritableImageResource> (512, 512);

    RG::GraphSettings s (GetDeviceExtra (), 3);

    auto& aTable2 = redFillOperation->compileSettings.attachmentProvider;
    aTable2->table.push_back ({ "outColor", RG::ShaderKind::Fragment, { red->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_CLEAR, red->GetImageViewForFrameProvider (), red->GetInitialLayout (), red->GetFinalLayout () } });

    s.connectionSet.Add (redFillOperation, red);

    RG::RenderGraph graph;
    graph.Compile (std::move (s));
    graph.Submit (0);

    env->Wait ();

    CompareImages ("lcg", *red->GetImages ()[0], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}


TEST_F (HeadlessTestEnvironment, UniformReflection_NestedTypes)
{
    std::unique_ptr<RG::ShaderModule> sm = RG::ShaderModule::CreateFromGLSLString (GetDevice (), RG::ShaderKind::Fragment, R"(#version 450

struct A {
    vec3 abc;
    double hello;
};

struct B {
    A bs[3];
};

struct C {
    B cs[3];
    mat3x4 WTF;
    float hehe[3];
};

layout (std140, binding = 2) uniform Quadrics {
    float dddddddddddd;
    vec3 dddddddddddd33;
    C quadrics[2];
    C theotherone[9];
};

layout (location = 0) out vec4 presented;
layout (binding = 3) uniform sampler3D agySampler;
layout (binding = 4) uniform sampler2D matcapSampler;

void main ()
{
    presented = vec4 (vec3 (1), dddddddddddd);
}

)");

    RG::Refl::SpirvParser      spirvParser (sm->GetBinary ());
    auto                       ubos = RG::Refl::GetUBOsFromBinary (spirvParser);
    RG::Refl::ShaderBufferData refl (ubos);

    refl["Quadrics"]["quadrics"][0]["WTF"] = glm::mat3x4 ();


    EXPECT_EQ (4256, refl.GetUbo ("Quadrics")->GetFullSize ());
}


TEST_F (HeadlessTestEnvironment, UniformReflection_MultiDimensionalArray)
{
    std::unique_ptr<RG::ShaderModule> sm = RG::ShaderModule::CreateFromGLSLString (GetDevice (), RG::ShaderKind::Fragment, R"(#version 450

layout (std140, binding = 2) uniform Values {
    float multiDim[5][6][7];
};

layout (location = 0) out vec4 presented;

void main ()
{
    presented = vec4 (vec3 (1), multiDim[1][2][3]);
}

)");

    RG::Refl::SpirvParser      spirvParser (sm->GetBinary ());
    auto                       ubos = RG::Refl::GetUBOsFromBinary (spirvParser);
    RG::Refl::ShaderBufferData refl (ubos);

    RG::Refl::BufferView v = refl["Values"]["multiDim"][1][2][3];
    v                      = static_cast<float> (3.4f);

    EXPECT_EQ (944, v.GetOffset ());
    EXPECT_EQ (3360, refl["Values"].GetSize ());
}


TEST_F (HeadlessTestEnvironment, UniformReflection_MultiDimensionalArray_RenderGraph)
{
    std::unique_ptr<RG::ShaderModule> sm = RG::ShaderModule::CreateFromGLSLString (GetDevice (), RG::ShaderKind::Fragment, R"(#version 450

layout (std140, binding = 2) uniform Values {
    float dddddddddddd[5][6][7];
};

layout (location = 0) out vec4 presented;

void main ()
{
    presented = vec4 (vec3 (1), dddddddddddd[1][2][3]);
}

)");

    RG::Refl::SpirvParser      spirvParser (sm->GetBinary ());
    auto                       ubos = RG::Refl::GetUBOsFromBinary (spirvParser);
    RG::Refl::ShaderBufferData refl (ubos);

    RG::Refl::BufferView v = refl["Values"]["dddddddddddd"][1];
    v[2][3]                = static_cast<float> (3.4f);

    EXPECT_EQ (3360, refl["Values"].GetSize ());
}


TEST_F (HeadlessTestEnvironment, ShaderPipeline_CompileTest)
{
    RG::DeviceExtra& device = GetDeviceExtra ();

    RG::RenderOperation op (std::make_unique<RG::DrawableInfo> (1, 3),
                            std::make_unique<RG::ShaderPipeline> (device, std::vector<std::filesystem::path> {
                                                                              ShadersFolder / "test.vert",
                                                                              ShadersFolder / "test.frag",
                                                                          }));
}


TEST_F (HeadlessTestEnvironment, ShaderModule_CompileError)
{
    try {
        RG::ShaderModule::CreateFromGLSLString (GetDevice (), RG::ShaderKind::Vertex, R"(
        #version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec2 textureCoords;

    vec2 uvs[6] = vec2[] (
        vec2 (0.f, 0.f),
        vec2 (0.f, 1.f),
        vec2 (1.f,d 1.f),
        vec2 (1.f, 1.f),
        vec2 (0.f, 0.f),
        vec2 (1.f, 0.f)
    );

    vec2 positions[6] = vec2[] (
        vec2 (-1.f, -1.f),
        vec2 (-1.f, +1.f),
        vec2 (+1.f, +1.f),
        vec2 (+1.f, +1.f),
        vec2 (-1.f, -1.f),
        vec2 (+1.f, -1.f)
    );


    void main ()
    {
        gl_Position = vec4 (positions[gl_VertexIndex], 0.0, 1.0);
        textureCoords = uvs[gl_VertexIndex];
    }
)");

        FAIL () << "expected ShaderCompileException";

    } catch (RG::ShaderCompileException&) {
        // good
    } catch (...) {
        FAIL () << "expected ShaderCompileException";
    }
}


TEST_F (HeadlessTestEnvironment, UniformReflection_ImageMap_TextureArray)
{
    const std::string fragSrc = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 7) uniform sampler2D textureArray_RGBA_32x32[2];
layout (binding = 8) uniform sampler2D textureArray_R_32x32[2];

layout (location = 0) out vec4 outColor;

void main () {
    outColor = vec4 (
        texture (textureArray_RGBA_32x32[0], vec2 ( 0.0/32.0,  0.0/32.0)).r,
        texture (textureArray_RGBA_32x32[0], vec2 (15.0/32.0, 28.0/32.0)).g,
        texture (textureArray_R_32x32[1],    vec2 ( 7.0/32.0, 31.0/32.0)).r,
        1
    );
}
    )";

    std::shared_ptr<RG::RenderOperation> redFillOperation = RG::RenderOperation::Builder (GetDevice ())
                                                                .SetVertices (std::make_unique<RG::DrawableInfo> (1, 6))
                                                                .SetPrimitiveTopology (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                                .SetVertexShader (passThroughVertexShader)
                                                                .SetFragmentShader (fragSrc)
                                                                .Build ();

    RG::GraphSettings s (GetDeviceExtra (), 3);

    s.connectionSet.Add (redFillOperation);

    RG::ImageMap imgMap = RG::CreateEmptyImageResources (s.connectionSet, [&] (const RG::Refl::Sampler& sampler) -> std::optional<RG::CreateParams> {
        if (sampler.name == "textureArray_RGBA_32x32") {
            return std::make_tuple (glm::uvec3 { 32, 32, 0 }, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FILTER_NEAREST);
        }
        if (sampler.name == "textureArray_R_32x32") {
            return std::make_tuple (glm::uvec3 { 32, 32, 0 }, VK_FORMAT_R32_SFLOAT, VK_FILTER_NEAREST);
        }
        return std::nullopt;
    });

    std::shared_ptr<RG::WritableImageResource> presented = std::make_unique<RG::WritableImageResource> (512, 512);

    auto& aTable2 = redFillOperation->compileSettings.attachmentProvider;
    aTable2->table.push_back ({ "outColor", RG::ShaderKind::Fragment, { presented->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_CLEAR, presented->GetImageViewForFrameProvider (), presented->GetInitialLayout (), presented->GetFinalLayout () } });

    s.connectionSet.Add (redFillOperation, presented);

    RG::RenderGraph graph;
    graph.Compile (std::move (s));
    {
        std::shared_ptr<RG::ReadOnlyImageResource> textureArray = imgMap.FindByName ("textureArray_R_32x32");
        RG_ASSERT (textureArray != nullptr);
        std::vector<float> pixelData (32 * 32);
        std::generate_n (pixelData.begin (), 32 * 32, [] () {
            return 0.3f;
        });
        textureArray->CopyTransitionTransfer (pixelData);
    }
    {
        std::shared_ptr<RG::ReadOnlyImageResource> textureArray = imgMap.FindByName ("textureArray_RGBA_32x32");
        RG_ASSERT (textureArray != nullptr);
        std::vector<glm::vec4> pixelData (32 * 32);
        std::generate_n (pixelData.begin (), 32 * 32, [] () {
            return glm::vec4 (0.7, 0.6, 0.8, 1.0);
        });
        textureArray->CopyTransitionTransfer (pixelData);
    }

    graph.Submit (0);

    env->Wait ();

    CompareImages ("textureArray", *presented->GetImages ()[0], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}


TEST_F (HeadlessTestEnvironment, RenderGraph_SingleOperation_SingleOutput_RenderRedImage)
{
    const std::string fragSrc = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec4 outColor;

void main () {
    outColor = vec4 (1, 0, 0, 1);
}
    )";

    std::shared_ptr<RG::RenderOperation> redFillOperation = RG::RenderOperation::Builder (GetDevice ())
                                                                .SetVertices (std::make_unique<RG::DrawableInfo> (1, 6))
                                                                .SetPrimitiveTopology (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                                .SetVertexShader (passThroughVertexShader)
                                                                .SetFragmentShader (fragSrc)
                                                                .Build ();

    std::shared_ptr<RG::WritableImageResource> red = std::make_unique<RG::WritableImageResource> (512, 512);

    RG::GraphSettings s (GetDeviceExtra (), 3);

    auto& aTable2 = redFillOperation->compileSettings.attachmentProvider;
    aTable2->table.push_back ({ "outColor", RG::ShaderKind::Fragment, { red->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_CLEAR, red->GetImageViewForFrameProvider (), red->GetInitialLayout (), red->GetFinalLayout () } });

    s.connectionSet.Add (redFillOperation, red);

    RG::RenderGraph graph;
    graph.Compile (std::move (s));
    graph.Submit (0);
    graph.Submit (1);
    graph.Submit (2);

    env->Wait ();

    CompareImages ("red", *red->GetImages ()[0], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    CompareImages ("red", *red->GetImages ()[1], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    CompareImages ("red", *red->GetImages ()[2], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}


TEST_F (HeadlessTestEnvironment, RenderGraph_MultipleOperations_MultipleOutputs)
{
    std::shared_ptr<RG::WritableImageResource> presented = std::make_unique<RG::WritableImageResource> (512, 512);
    std::shared_ptr<RG::WritableImageResource> green     = std::make_unique<RG::WritableImageResource> (512, 512);
    std::shared_ptr<RG::WritableImageResource> red       = std::make_unique<RG::WritableImageResource> (512, 512);
    std::shared_ptr<RG::WritableImageResource> finalImg  = std::make_unique<RG::WritableImageResource> (512, 512);


    /*
        green -> dummyPass --> presented -> secondPass -> finalImg
                          \
                           \-> red
    */

    std::shared_ptr<RG::RenderOperation> dummyPass = RG::RenderOperation::Builder (GetDevice ())
                                                         .SetPrimitiveTopology (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                         .SetVertices (std::make_unique<RG::DrawableInfo> (1, 3))
                                                         .SetVertexShader (ShadersFolder / "test.vert")
                                                         .SetVertexShader (ShadersFolder / "test.frag")
                                                         .Build ();

    std::shared_ptr<RG::RenderOperation> secondPass = RG::RenderOperation::Builder (GetDevice ())
                                                          .SetPrimitiveTopology (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                          .SetVertices (std::make_unique<RG::DrawableInfo> (1, 3))
                                                          .SetVertexShader (ShadersFolder / "fullscreenquad.vert")
                                                          .SetVertexShader (ShadersFolder / "fullscreenquad.frag")
                                                          .Build ();

    auto& table = dummyPass->compileSettings.descriptorWriteProvider;
    table->imageInfos.push_back ({ "sampl", RG::ShaderKind::Fragment, green->GetSamplerProvider (), green->GetImageViewForFrameProvider (), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

    auto& table2 = secondPass->compileSettings.descriptorWriteProvider;
    table2->imageInfos.push_back ({ "sampl", RG::ShaderKind::Fragment, red->GetSamplerProvider (), red->GetImageViewForFrameProvider (), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

    auto& aTable = dummyPass->compileSettings.attachmentProvider;
    aTable->table.push_back ({ "outColor", RG::ShaderKind::Fragment, { presented->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_CLEAR, presented->GetImageViewForFrameProvider (), presented->GetInitialLayout (), presented->GetFinalLayout () } });
    aTable->table.push_back ({ "red", RG::ShaderKind::Fragment, { red->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_CLEAR, red->GetImageViewForFrameProvider (), red->GetInitialLayout (), red->GetFinalLayout () } });

    auto& aTable2 = secondPass->compileSettings.attachmentProvider;
    aTable2->table.push_back ({ "outColor", RG::ShaderKind::Fragment, { finalImg->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_CLEAR, finalImg->GetImageViewForFrameProvider (), finalImg->GetInitialLayout (), finalImg->GetFinalLayout () } });

    RG::ConnectionSet connectionSet;
    connectionSet.Add (green, dummyPass);
    connectionSet.Add (red, secondPass);
    connectionSet.Add (dummyPass, presented);
    connectionSet.Add (dummyPass, red);
    connectionSet.Add (secondPass, finalImg);

    RG::RenderGraph graph;
    graph.Compile (RG::GraphSettings (GetDeviceExtra (), std::move (connectionSet), 4));

    for (uint32_t i = 0; i < 1; ++i) {
        graph.Submit (i);
    }

    env->Wait ();

    ASSERT_TRUE (RG::ImageData (GetDeviceExtra (), *presented->GetImages ()[0], 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) == RG::ImageData (ReferenceImagesFolder / "black.png"));
}


TEST_F (HeadlessTestEnvironment, DebugPrintf_ShaderModule_Compile)
{
    std::unique_ptr<RG::ShaderModule> shaderModule = RG::ShaderModule::CreateFromGLSLString (GetDevice (), RG::ShaderKind::Compute, R"(
#version 450
#extension GL_EXT_debug_printf : enable
void main() {
    float myfloat = 3.1415f;
    debugPrintfEXT("My float is %f", myfloat);
}
)");
}


TEST_F (HeadlessTestEnvironment, InputAttachment_ShaderModule_Compile)
{
    std::unique_ptr<RG::ShaderModule> shaderModule = RG::ShaderModule::CreateFromGLSLString (GetDevice (), RG::ShaderKind::Fragment, R"(
#version 450

layout (input_attachment_index = 0, binding = 0) uniform subpassInput inputColor;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth;

layout (binding = 2) uniform BufferObject {
        vec2 brightnessContrast;
        vec2 range;
        int attachmentIndex;
} ubo;

layout (location = 0) out vec4 outColor;

vec3 brightnessContrast(vec3 color, float brightness, float contrast) {
        return (color - 0.5) * contrast + 0.5 + brightness;
}

void main() 
{
    // Apply brightness and contrast filer to color input
    if (ubo.attachmentIndex == 0) {
        // Read color from previous color input attachment
        vec3 color = subpassLoad(inputColor).rgb;
        outColor.rgb = brightnessContrast(color, ubo.brightnessContrast[0], ubo.brightnessContrast[1]);
    }

    // Visualize depth input range
    if (ubo.attachmentIndex == 1) {
        // Read depth from previous depth input attachment
        float depth = subpassLoad(inputDepth).r;
        outColor.rgb = vec3((depth - ubo.range[0]) * 1.0 / (ubo.range[1] - ubo.range[0]));
    }
}
)");
}


TEST_F (HeadlessTestEnvironment, InputAttachment_RenderGraph)
{
    const std::string fragSrc = R"(
#version 450

layout (input_attachment_index = 0, binding = 0) uniform usubpassInput inputColor;

layout (location = 0) out uvec4 outColor;

void main() 
{
    uvec3 si = subpassLoad (inputColor).bgr;
    outColor = uvec4 (si, 255);
}
)";

    std::shared_ptr<RG::RenderOperation> firstPass = RG::RenderOperation::Builder (GetDevice ())
                                                         .SetPrimitiveTopology (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                         .SetVertices (std::make_unique<RG::DrawableInfo> (1, 6))
                                                         .SetVertexShader (passThroughVertexShader)
                                                         .SetFragmentShader (fragSrc)
                                                         .SetBlendEnabled (false)
                                                         .Build ();

    std::shared_ptr<RG::WritableImageResource> inputColor = std::make_unique<RG::WritableImageResource> (VK_FILTER_LINEAR, 32, 32, 1, VK_FORMAT_R8G8B8A8_UINT);

    std::shared_ptr<RG::WritableImageResource> outColor = std::make_unique<RG::WritableImageResource> (VK_FILTER_LINEAR, 32, 32, 1, VK_FORMAT_R8G8B8A8_UINT);

    inputColor->initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    inputColor->finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    auto& aTable = firstPass->compileSettings.attachmentProvider->table;
    aTable.push_back ({ "inputColor", RG::ShaderKind::Fragment, { inputColor->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_LOAD, inputColor->GetImageViewForFrameProvider (), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } });
    aTable.push_back ({ "outColor", RG::ShaderKind::Fragment, { outColor->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_CLEAR, outColor->GetImageViewForFrameProvider (), outColor->GetInitialLayout (), outColor->GetFinalLayout () } });

    auto& dTable = firstPass->compileSettings.descriptorWriteProvider;
    dTable->imageInfos.push_back ({ "inputColor", RG::ShaderKind::Fragment, inputColor->GetSamplerProvider (), inputColor->GetImageViewForFrameProvider (), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

    RG::ConnectionSet connectionSet;

    connectionSet.Add (inputColor, firstPass);
    connectionSet.Add (firstPass, outColor);

    RG::GraphSettings s;

    s.framesInFlight = 1;
    s.device         = &GetDeviceExtra ();
    s.connectionSet  = std::move (connectionSet);

    RG::RenderGraph graph;
    graph.Compile (std::move (s));

    {
        RG::Buffer buff (*env->allocator, 32 * 32 * 4 * 3, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, RG::Buffer::MemoryLocation::CPU);

        RG::MemoryMapping mapping (*env->allocator, buff);

        std::vector<uint8_t> pixelData (32 * 32 * 4 * 3);
        for (uint32_t i = 0; i < pixelData.size (); i += 4) {
            pixelData[i + 0] = 255;
            pixelData[i + 1] = 0;
            pixelData[i + 2] = 0;
            pixelData[i + 3] = 128;
        }
        mapping.Copy (pixelData);

        {
            RG::SingleTimeCommand single (GetDevice (), GetCommandPool (), GetGraphicsQueue ());
            single.Record<RG::CommandTranstionImage> (*inputColor->GetImages ()[0], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            single.Record<RG::CommandCopyBufferToImage> (buff, *inputColor->GetImages ()[0], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, std::vector<VkBufferImageCopy> { inputColor->GetImages ()[0]->GetFullBufferImageCopy () });
            single.Record<RG::CommandTranstionImage> (*inputColor->GetImages ()[0], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }


    env->Wait ();
    graph.Submit (0);
    env->Wait ();

    RG::ImageData img (GetDeviceExtra (), *outColor->GetImages ()[0], 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    RG::ImageData referenceImage (ReferenceImagesFolder / "blue_32x32.png");

    EXPECT_TRUE (referenceImage == img);
}


TEST_F (HeadlessTestEnvironment, DISABLED_InputAttachment_RenderGraph_WriteAsOutput)
{
    const std::string fragSrc = R"(
#version 450

layout (input_attachment_index = 0, binding = 0) uniform usubpassInput inputColor1;
layout (input_attachment_index = 1, binding = 1) uniform usubpassInput inputColor2;
layout (input_attachment_index = 2, binding = 2) uniform usubpassInput inputColor3;

layout (location = 0) out uvec4 outColor1;
layout (location = 1) out uvec4 outColor2;
layout (location = 2) out uvec4 outColor3;

void main() 
{
    uvec3 si1 = subpassLoad (inputColor1).rgb;
    uvec3 si2 = subpassLoad (inputColor2).rgb;
    outColor3 = uvec4 (si1 + si2, 255);
}
)";

    std::shared_ptr<RG::RenderOperation> firstPass = RG::RenderOperation::Builder (GetDevice ())
                                                         .SetPrimitiveTopology (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                         .SetVertices (std::make_unique<RG::DrawableInfo> (1, 6))
                                                         .SetVertexShader (passThroughVertexShader)
                                                         .SetFragmentShader (fragSrc)
                                                         .SetBlendEnabled (false)
                                                         .Build ();

    std::shared_ptr<RG::WritableImageResource> inputColor1 = std::make_unique<RG::WritableImageResource> (VK_FILTER_LINEAR, 32, 32, 1, VK_FORMAT_R8G8B8A8_UINT);
    std::shared_ptr<RG::WritableImageResource> inputColor2 = std::make_unique<RG::WritableImageResource> (VK_FILTER_LINEAR, 32, 32, 1, VK_FORMAT_R8G8B8A8_UINT);
    std::shared_ptr<RG::WritableImageResource> inputColor3 = std::make_unique<RG::WritableImageResource> (VK_FILTER_LINEAR, 32, 32, 1, VK_FORMAT_R8G8B8A8_UINT);

    inputColor1->initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    inputColor1->finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    inputColor2->initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    inputColor2->finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    auto& aTable = firstPass->compileSettings.attachmentProvider->table;
    aTable.push_back ({ "inputColor1", RG::ShaderKind::Fragment, { inputColor1->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_LOAD, inputColor1->GetImageViewForFrameProvider (), inputColor1->GetInitialLayout (), inputColor1->GetFinalLayout () } });
    aTable.push_back ({ "outColor1", RG::ShaderKind::Fragment, { inputColor1->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_LOAD, inputColor1->GetImageViewForFrameProvider (), inputColor1->GetInitialLayout (), inputColor1->GetFinalLayout () } });

    aTable.push_back ({ "inputColor2", RG::ShaderKind::Fragment, { inputColor2->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_LOAD, inputColor2->GetImageViewForFrameProvider (), inputColor2->GetInitialLayout (), inputColor2->GetFinalLayout () } });
    aTable.push_back ({ "outColor2", RG::ShaderKind::Fragment, { inputColor2->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_LOAD, inputColor2->GetImageViewForFrameProvider (), inputColor2->GetInitialLayout (), inputColor2->GetFinalLayout () } });

    aTable.push_back ({ "inputColor3", RG::ShaderKind::Fragment, { inputColor3->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_LOAD, inputColor3->GetImageViewForFrameProvider (), inputColor3->GetInitialLayout (), inputColor3->GetFinalLayout () } });
    aTable.push_back ({ "outColor3", RG::ShaderKind::Fragment, { inputColor3->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_LOAD, inputColor3->GetImageViewForFrameProvider (), inputColor3->GetInitialLayout (), inputColor3->GetFinalLayout () } });

    auto& dTable = firstPass->compileSettings.descriptorWriteProvider;
    dTable->imageInfos.push_back ({ "inputColor1", RG::ShaderKind::Fragment, inputColor1->GetSamplerProvider (), inputColor1->GetImageViewForFrameProvider (), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
    dTable->imageInfos.push_back ({ "inputColor2", RG::ShaderKind::Fragment, inputColor2->GetSamplerProvider (), inputColor2->GetImageViewForFrameProvider (), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
    dTable->imageInfos.push_back ({ "inputColor3", RG::ShaderKind::Fragment, inputColor3->GetSamplerProvider (), inputColor3->GetImageViewForFrameProvider (), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

    RG::ConnectionSet connectionSet;

    connectionSet.Add (inputColor1, firstPass);
    connectionSet.Add (inputColor2, firstPass);
    connectionSet.Add (inputColor3, firstPass);
    connectionSet.Add (firstPass, inputColor1);
    connectionSet.Add (firstPass, inputColor2);
    connectionSet.Add (firstPass, inputColor3);

    RG::GraphSettings s;

    s.framesInFlight = 1;
    s.device         = &GetDeviceExtra ();
    s.connectionSet  = std::move (connectionSet);

    RG::RenderGraph graph;
    graph.Compile (std::move (s));

    {
        RG::Buffer buff (*env->allocator, 32 * 32 * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, RG::Buffer::MemoryLocation::CPU);

        RG::MemoryMapping mapping (*env->allocator, buff);

        std::vector<uint8_t> pixelData (32 * 32 * 4);
        for (uint32_t i = 0; i < pixelData.size (); i += 4) {
            pixelData[i + 0] = 0;
            pixelData[i + 1] = 0;
            pixelData[i + 2] = 100;
            pixelData[i + 3] = 128;
        }
        mapping.Copy (pixelData);

        {
            RG::SingleTimeCommand single (GetDevice (), GetCommandPool (), GetGraphicsQueue ());
            single.Record<RG::CommandTranstionImage> (*inputColor1->GetImages ()[0], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            single.Record<RG::CommandCopyBufferToImage> (buff, *inputColor1->GetImages ()[0], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, std::vector<VkBufferImageCopy> { inputColor1->GetImages ()[0]->GetFullBufferImageCopy () });
            single.Record<RG::CommandTranstionImage> (*inputColor1->GetImages ()[0], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }

    {
        RG::Buffer buff (*env->allocator, 32 * 32 * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, RG::Buffer::MemoryLocation::CPU);

        RG::MemoryMapping mapping (*env->allocator, buff);

        std::vector<uint8_t> pixelData (32 * 32 * 4);
        for (uint32_t i = 0; i < pixelData.size (); i += 4) {
            pixelData[i + 0] = 100;
            pixelData[i + 1] = 0;
            pixelData[i + 2] = 0;
            pixelData[i + 3] = 128;
        }
        mapping.Copy (pixelData);

        {
            RG::SingleTimeCommand single (GetDevice (), GetCommandPool (), GetGraphicsQueue ());
            single.Record<RG::CommandTranstionImage> (*inputColor2->GetImages ()[0], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            single.Record<RG::CommandCopyBufferToImage> (buff, *inputColor2->GetImages ()[0], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, std::vector<VkBufferImageCopy> { inputColor2->GetImages ()[0]->GetFullBufferImageCopy () });
            single.Record<RG::CommandTranstionImage> (*inputColor2->GetImages ()[0], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }


    env->Wait ();
    graph.Submit (0);
    env->Wait ();

    RG::ImageData img (GetDeviceExtra (), *inputColor3->GetImages ()[0], 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    RG::ImageData referenceImage (ReferenceImagesFolder / "blue_32x32.png");

    EXPECT_TRUE (referenceImage == img);
}


TEST_F (HeadlessTestEnvironment, DISABLED_RenderGraph_SameImageAsInputAndOutput)
{
    /*
        presented ---> firstPass  ---> presented
    */

    const std::string frag1 = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) flat in uvec4 inColor[3];

layout (location = 0) out uvec4 outColor[3];

void main () {
    outColor[0] = uvec4 (inColor[0].bgr, 255);
}
    )";

    std::shared_ptr<RG::RenderOperation> firstPass = RG::RenderOperation::Builder (GetDevice ())
                                                         .SetPrimitiveTopology (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                         .SetVertices (std::make_unique<RG::DrawableInfo> (1, 6))
                                                         .SetVertexShader (passThroughVertexShader)
                                                         .SetFragmentShader (frag1)
                                                         .SetName ("FIRST")
                                                         .Build ();

    std::shared_ptr<RG::WritableImageResource> presented = std::make_unique<RG::WritableImageResource> (VK_FILTER_NEAREST, 32, 32, 3, VK_FORMAT_R8G8B8A8_UINT);

    RG::GraphSettings s (GetDeviceExtra (), 1);

    auto& aTable2 = firstPass->compileSettings.attachmentProvider;
    aTable2->table.push_back ({ "outColor", RG::ShaderKind::Fragment, { presented->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_LOAD, presented->GetImageViewForFrameProvider (), presented->GetInitialLayout (), presented->GetFinalLayout () } });

    auto& table = firstPass->compileSettings.descriptorWriteProvider;
    table->imageInfos.push_back ({ "inColor", RG::ShaderKind::Fragment, presented->GetSamplerProvider (), presented->GetImageViewForFrameProvider (), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

    s.connectionSet.Add (presented, firstPass);
    s.connectionSet.Add (firstPass, presented);

    RG::RenderGraph graph;
    graph.Compile (std::move (s));

    RG::ImageData before2_resultImage (GetDeviceExtra (), *presented->GetImages ()[0], 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    before2_resultImage.SaveTo (TempFolder / "before_tempDoubl2e.png");

    {
        RG::Buffer buff (*env->allocator, 32 * 32 * 4 * 3, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, RG::Buffer::MemoryLocation::CPU);

        RG::MemoryMapping mapping (*env->allocator, buff);

        std::vector<uint8_t> pixelData (32 * 32 * 4 * 3);
        for (uint32_t i = 0; i < pixelData.size (); i += 4) {
            pixelData[i + 0] = 255;
            pixelData[i + 1] = 0;
            pixelData[i + 2] = 0;
            pixelData[i + 3] = 128;
        }
        mapping.Copy (pixelData);

        {
            RG::SingleTimeCommand single (GetDevice (), GetCommandPool (), GetGraphicsQueue ());
            single.Record<RG::CommandTranstionImage> (*presented->GetImages ()[0], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            single.Record<RG::CommandCopyBufferToImage> (buff, *presented->GetImages ()[0], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, std::vector<VkBufferImageCopy> { presented->GetImages ()[0]->GetFullBufferImageCopy () });
            single.Record<RG::CommandTranstionImage> (*presented->GetImages ()[0], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        }
    }

    env->Wait ();
    RG::ImageData before_resultImage (GetDeviceExtra (), *presented->GetImages ()[0], 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    before_resultImage.SaveTo (TempFolder / "before_tempDouble.png");

    graph.Submit (0);
    env->Wait ();

    RG::ImageData resultImage (GetDeviceExtra (), *presented->GetImages ()[0], 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    resultImage.SaveTo (TempFolder / "tempDouble.png");
}


TEST_F (HeadlessTestEnvironment, ComputeShader_ShaderModule)
{
    std::unique_ptr<RG::ShaderModule> shaderModule = RG::ShaderModule::CreateFromGLSLString (GetDevice (), RG::ShaderKind::Compute, R"(
#version 450
layout (local_size_x = 256) in;

layout(set = 0, binding = 0) uniform Config{
    mat4 transform;
    int matrixCount;
} opData;

layout(set = 0, binding = 1) readonly buffer  InputBuffer{
    mat4 matrices[];
} sourceData;

layout(set = 0, binding = 2) buffer  OutputBuffer{
    mat4 matrices[];
} outputData;


void main()
{
    //grab global ID
	uint gID = gl_GlobalInvocationID.x;
    //make sure we don't access past the buffer size
    if(gID < opData.matrixCount)
    {
        // do math
        outputData.matrices[gID] = sourceData.matrices[gID] * opData.transform;
    }
}
    )");
}


TEST_F (HeadlessTestEnvironment, ComputeShader_RenderGraph)
{
    const std::string compSrc = R"(
#version 450
#extension GL_EXT_shader_explicit_arithmetic_types : enable

layout (local_size_x = 32, local_size_y = 32) in;

layout (set = 0, binding = 0) buffer OutputBuffer {
    uint64_t randomsBuffer[32][32];
};

void main()
{
    uint gIDx = gl_GlobalInvocationID.x;
    uint gIDy = gl_GlobalInvocationID.y;

    if (gIDx < 32 && gIDy < 32) {
        randomsBuffer[gIDy][gIDx] = gIDy * 32 + gIDx;
    }
}
    )";

    std::shared_ptr<RG::ComputeOperation> randomGenerator = std::make_unique<RG::ComputeOperation> (32, 32, 1);

    std::shared_ptr<RG::CPUBufferResource> randomsBuffer = std::make_unique<RG::CPUBufferResource> (32 * 32 * 8);

    randomGenerator->compileSettings.descriptorWriteProvider->bufferInfos.push_back ({ "OutputBuffer", RG::ShaderKind::Compute, randomsBuffer->GetBufferForFrameProvider (), 0, randomsBuffer->GetBufferSize () });

    randomGenerator->compileSettings.computeShaderPipeline = std::make_unique<RG::ComputeShaderPipeline> (GetDevice (), compSrc);

    RG::ConnectionSet connectionSet;
    connectionSet.Add (randomGenerator, randomsBuffer);

    RG::GraphSettings s;
    s.connectionSet  = std::move (connectionSet);
    s.device         = &GetDeviceExtra ();
    s.framesInFlight = 1;

    RG::RenderGraph graph;
    graph.Compile (std::move (s));

    env->Wait ();
    graph.Submit (0);
    env->Wait ();

    std::vector<uint64_t> randomData;
    randomData.resize (32 * 32, 0);

    memcpy (randomData.data (), randomsBuffer->GetMapping (0).Get (), 32 * 32 * 8);

    for (uint64_t i = 0; i < 32 * 32; ++i) {
        EXPECT_EQ (i, randomData[i]);
    }
}


TEST_F (HeadlessTestEnvironment, ComputeShader_RenderGraph_RandomGenerator_XorShift)
{
    const std::string compSrc = R"(
#version 450

layout (set = 0, binding = 0) uniform RandomGeneratorConfig {
    uint seed;
    uint framesInFlight;
    uint startFrameIndex;
    uint nextElementIndex;
};

layout (set = 0, binding = 1) buffer OutputBuffer {
    uvec4 randomsBuffer[5][4][4];
};

void main()
{
    uint gIDx = gl_GlobalInvocationID.x;
    uint gIDy = gl_GlobalInvocationID.y;

    uint framesToGenerate = min (startFrameIndex, framesInFlight);

    for (uint frame = startFrameIndex; frame < startFrameIndex + framesToGenerate; ++frame) {
        uvec4 nextElement = uvec4 (0);
        if(frame == 1) {
            nextElement.r = gIDx * 1341593453u ^ gIDy *  971157919u ^ seed * 2883500843u;
            nextElement.g = gIDx * 1790208463u ^ gIDy * 1508561443u ^ seed * 2321036227u;
            nextElement.b = gIDx * 2659567811u ^ gIDy * 2918034323u ^ seed * 2244239747u;
            nextElement.a = gIDx * 3756158669u ^ gIDy * 1967864287u ^ seed * 1275070309u;
        } else if (frame == 2) {
            nextElement.r = gIDx * 2771446331u ^ gIDy * 3030392353u ^ seed *  395945089u;
            nextElement.g = gIDx * 3459812197u ^ gIDy * 2853318569u ^ seed * 1233582347u;
            nextElement.b = gIDx * 2926663697u ^ gIDy * 2265556091u ^ seed * 3073622047u;
            nextElement.a = gIDx * 3459811891u ^ gIDy * 1756462801u ^ seed * 2805899363u;
        } else if (frame == 3) {
            nextElement.r = gIDx * 1470939049u ^ gIDy * 2244239737u ^ seed * 2056949767u;
            nextElement.g = gIDx * 1584004207u ^ gIDy * 1630196153u ^ seed * 2965533797u;
            nextElement.b = gIDx * 2248501561u ^ gIDy * 2728389799u ^ seed * 2099451241u;
            nextElement.a = gIDx *  715964407u ^ gIDy * 1735392947u ^ seed * 1496011453u;
        } else if (frame == 4) {
            nextElement.r = gIDx * 1579813297u ^ gIDy *  890180033u ^ seed * 1760681059u;
            nextElement.g = gIDx * 4132540697u ^ gIDy * 1362405383u ^ seed * 3052005647u;
            nextElement.b = gIDx * 3155894689u ^ gIDy * 1883169037u ^ seed * 2870559073u;
            nextElement.a = gIDx * 1883169037u ^ gIDy * 2278336279u ^ seed * 2278336133u;
        } else {
	        uvec4 x = randomsBuffer[(nextElementIndex + 1) % 5][gIDy][gIDx];
	        uvec4 y = randomsBuffer[(nextElementIndex + 2) % 5][gIDy][gIDx];
	        uvec4 z = randomsBuffer[(nextElementIndex + 3) % 5][gIDy][gIDx];
	        uvec4 w = randomsBuffer[(nextElementIndex + 4) % 5][gIDy][gIDx];
            // 128-bit xorshift algorithm
            uvec4 t = x ^ (x << 11u);
            nextElement = w ^ (w >> 19u) ^ t ^ (t >> 8u);
        }
        randomsBuffer[(nextElementIndex + frame - 1) % framesInFlight][gIDy][gIDx] = nextElement;
    }
}
    )";

    std::shared_ptr<RG::ComputeOperation> randomGenerator  = std::make_unique<RG::ComputeOperation> (4, 4, 1);
    randomGenerator->compileSettings.computeShaderPipeline = std::make_unique<RG::ComputeShaderPipeline> (GetDevice (), compSrc);

    RG::ConnectionSet connectionSet;
    connectionSet.Add (randomGenerator);

    auto creator = [&] (const std::shared_ptr<RG::Operation>&, const RG::ShaderModule&, const std::shared_ptr<RG::Refl::BufferObject>& bufferObject, bool& /* treatAsOutput */) -> std::shared_ptr<RG::DescriptorBindableBufferResource> {
        if (bufferObject->name == "OutputBuffer")
            return std::make_unique<RG::GPUBufferResource> (bufferObject->GetFullSize ());

        return std::make_unique<RG::CPUBufferResource> (bufferObject->GetFullSize ());
    };

    RG::UniformReflection refl (connectionSet, creator);

    constexpr uint32_t framesInFlight = 3;

    RG::GraphSettings s;
    s.connectionSet  = std::move (connectionSet);
    s.device         = &GetDeviceExtra ();
    s.framesInFlight = framesInFlight;

    RG::RenderGraph graph;
    graph.Compile (std::move (s));

    std::shared_ptr<RG::GPUBufferResource> randomsBuffer = graph.GetConnectionSet ().GetByName<RG::GPUBufferResource> ("OutputBuffer");
    std::shared_ptr<RG::CPUBufferResource> randomsConfig = graph.GetConnectionSet ().GetByName<RG::CPUBufferResource> ("RandomGeneratorConfig");

    for (uint32_t i = 0; i < 3; ++i) {
        const uint32_t frameIndex    = i;
        const uint32_t resourceIndex = i % framesInFlight;

        refl[randomGenerator][RG::ShaderKind::Compute]["RandomGeneratorConfig"]["seed"]             = static_cast<uint32_t> (7);
        refl[randomGenerator][RG::ShaderKind::Compute]["RandomGeneratorConfig"]["framesInFlight"]   = static_cast<uint32_t> (framesInFlight);
        refl[randomGenerator][RG::ShaderKind::Compute]["RandomGeneratorConfig"]["nextElementIndex"] = static_cast<uint32_t> (frameIndex % 5);
        refl[randomGenerator][RG::ShaderKind::Compute]["RandomGeneratorConfig"]["startFrameIndex"]  = static_cast<uint32_t> (frameIndex + 1);

        refl.Flush (resourceIndex);

        env->Wait ();
        graph.Submit (resourceIndex);
        env->Wait ();
    }

    constexpr uint32_t checkedResourceIndex = 2;

    randomsBuffer->TransferFromGPUToCPU (checkedResourceIndex);

    std::vector<glm::uvec4> randomsBufferOut;
    randomsBufferOut.resize (randomsBuffer->GetBufferSize () / sizeof (glm::uvec4));

    memcpy (randomsBufferOut.data (), randomsBuffer->buffers[checkedResourceIndex]->bufferCPUMapping.Get (), randomsBuffer->GetBufferSize ());
}


TEST_F (HeadlessTestEnvironment, RenderGraph_TwoOperationsRenderingToOutput)
{
    /*
        firstPass  ---> presented
        secondPass  _/
    */

    // will turn into

    /*
        firstPass ---> presented ---> secondPass ---> presented
    */

    const std::string frag1 = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec4 outColor;

void main () {
    outColor = vec4 (1, 0, 0, 0.5);
}
    )";

    const std::string frag2 = R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec4 outColor;

void main () {
    outColor = vec4 (0, 0, 1, 0.5);
}
    )";

    std::shared_ptr<RG::RenderOperation> firstPass = RG::RenderOperation::Builder (GetDevice ())
                                                         .SetPrimitiveTopology (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                         .SetVertices (std::make_unique<RG::DrawableInfo> (1, 6))
                                                         .SetVertexShader (passThroughVertexShader)
                                                         .SetFragmentShader (frag1)
                                                         .SetBlendEnabled (false)
                                                         .SetName ("FIRST")
                                                         .Build ();

    std::shared_ptr<RG::RenderOperation> secondPass = RG::RenderOperation::Builder (GetDevice ())
                                                          .SetPrimitiveTopology (VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                          .SetVertices (std::make_unique<RG::DrawableInfo> (1, 6))
                                                          .SetVertexShader (passThroughVertexShader)
                                                          .SetFragmentShader (frag2)
                                                          .SetBlendEnabled (true)
                                                          .SetName ("SECOND")
                                                          .Build ();

    std::shared_ptr<RG::WritableImageResource> presented = std::make_unique<RG::WritableImageResource> (VK_FILTER_LINEAR, 512, 512, 1, VK_FORMAT_R8G8B8A8_SRGB);

    RG::GraphSettings s (GetDeviceExtra (), 1);

    auto& aTable2 = firstPass->compileSettings.attachmentProvider;
    aTable2->table.push_back ({ "outColor", RG::ShaderKind::Fragment, { presented->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_CLEAR, presented->GetImageViewForFrameProvider (), presented->GetInitialLayout (), presented->GetFinalLayout () } });

    s.connectionSet.Add (firstPass, presented);

    auto& aTable3 = secondPass->compileSettings.attachmentProvider;
    aTable3->table.push_back ({ "outColor", RG::ShaderKind::Fragment, { presented->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_LOAD, presented->GetImageViewForFrameProvider (), presented->GetInitialLayout (), presented->GetFinalLayout () } });

    s.connectionSet.Add (secondPass, presented);

    RG::RenderGraph graph;
    graph.Compile (std::move (s));

    EXPECT_EQ (2, graph.GetPassCount ());

    RG::ImageData referenceImage (ReferenceImagesFolder / "pink.png");

    const size_t renderCount = 1;
    size_t       matchCount  = 0;

    for (size_t i = 0; i < renderCount; ++i) {
        std::this_thread::sleep_for (std::chrono::milliseconds (200));

        graph.Submit (0);

        env->Wait ();

        RG::ImageData (GetDeviceExtra (), *presented->GetImages ()[0], 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL).SaveTo (ReferenceImagesFolder / "presentedTwo.png");

        if (RG::ImageData (GetDeviceExtra (), *presented->GetImages ()[0], 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) == referenceImage) {
            ++matchCount;
        }
    }

    EXPECT_EQ (renderCount, matchCount);
}


// no window, swapchain, surface
class HeadlessTestEnvironmentWithExt : public TestEnvironmentBase {
protected:
    virtual void SetUp () override;
    virtual void TearDown () override;
};


void HeadlessTestEnvironmentWithExt::SetUp ()
{
    env = std::make_unique<RG::VulkanEnvironment> (testDebugCallback, RG::GetGLFWInstanceExtensions (), std::vector<const char*> { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME });
}


void HeadlessTestEnvironmentWithExt::TearDown ()
{
    env.reset ();
}


TEST_F (HeadlessTestEnvironmentWithExt, Swapchain_Create)
{
    RG::GLFWWindow glfwWindow;

    RG::Surface surface (*env->instance, glfwWindow.GetSurface (*env->instance));

    RG_ASSERT (env->physicalDevice->CheckSurfaceSupported (surface));

    RG::RealSwapchain swapchain (*env->physicalDevice, *env->device, surface, std::make_unique<RG::DefaultSwapchainSettings> ());
}


TEST_F (HiddenWindowTestEnvironment, RenderGraph_RenderingToSwapchain)
{
    RG::DeviceExtra& device    = GetDeviceExtra ();
    RG::Swapchain&   swapchain = GetSwapchain ();


    RG::GraphSettings s (device, swapchain.GetImageCount ());
    RG::RenderGraph   graph;

    auto sp = std::make_unique<RG::ShaderPipeline> (device);
    sp->SetVertexShaderFromString (passThroughVertexShader);

    sp->SetFragmentShaderFromString (R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outCopy;

void main () {
    vec4 result = vec4 (uv, 0, 1);
    outColor = result;
    outCopy = result;
}
    )");

    std::shared_ptr<RG::RenderOperation> redFillOperation = std::make_unique<RG::RenderOperation> (std::make_unique<RG::DrawableInfo> (1, 6), std::move (sp));

    std::shared_ptr<RG::WritableImageResource>  presentedCopy = std::make_unique<RG::WritableImageResource> (800, 600);
    std::shared_ptr<RG::SwapchainImageResource> presented     = std::make_unique<RG::SwapchainImageResource> (*presentable);

    auto& aTable3 = redFillOperation->compileSettings.attachmentProvider;
    aTable3->table.push_back ({ "outColor", RG::ShaderKind::Fragment, { presented->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_CLEAR, presented->GetImageViewForFrameProvider (), presented->GetInitialLayout (), presented->GetFinalLayout () } });
    aTable3->table.push_back ({ "outCopy", RG::ShaderKind::Fragment, { presentedCopy->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_CLEAR, presentedCopy->GetImageViewForFrameProvider (), presentedCopy->GetInitialLayout (), presentedCopy->GetFinalLayout () } });

    s.connectionSet.Add (redFillOperation, presented);
    s.connectionSet.Add (redFillOperation, presentedCopy);

    graph.Compile (std::move (s));

    RG::BlockingGraphRenderer renderer (device, swapchain);
    uint32_t                  count = 0;
    auto                      cb    = renderer.GetConditionalDrawCallback ([&] () -> RG::RenderGraph& { return graph; }, [&] () -> bool { return ++count > 10; });
    window->DoEventLoop (cb);
}


TEST_F (HiddenWindowTestEnvironment, RenderGraph_VertexAndIndexBuffer)
{
    RG::DeviceExtra& device    = GetDeviceExtra ();
    RG::Swapchain&   swapchain = GetSwapchain ();

    RG::GraphSettings s (device, swapchain.GetImageCount ());
    RG::RenderGraph   graph;

    auto sp = std::make_unique<RG::ShaderPipeline> (device);
    sp->SetVertexShaderFromString (R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in float asd;

layout (location = 0) out vec2 textureCoords;
layout (location = 1) out float asdout;


void main() {
    gl_Position = vec4 (position, 0.0, 1.0);
    textureCoords = uv;
    asdout = asd;
}
    )");

    sp->SetFragmentShaderFromString (R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 1) in float asdout;
layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outCopy;

void main () {
    vec4 result = vec4 (vec3 (uv, 0.f), 1);
    outColor = result;
    outCopy = result;
}
    )");

    struct Vert {
        glm::vec2 position;
        glm::vec2 uv;
        float     asd;
    };

    RG::VertexBufferTransferable<Vert> vbb (device, 4, { RG::ShaderTypes::Vec2f, RG::ShaderTypes::Vec2f, RG::ShaderTypes::Float }, VK_VERTEX_INPUT_RATE_VERTEX);
    vbb = std::vector<Vert> {
        { glm::vec2 (-1.f, -1.f), glm::vec2 (0.f, 0.f), 0.1f },
        { glm::vec2 (-1.f, +1.f), glm::vec2 (0.f, 1.f), 0.2f },
        { glm::vec2 (+1.f, +1.f), glm::vec2 (1.f, 1.f), 0.3f },
        { glm::vec2 (+1.f, -1.f), glm::vec2 (1.f, 0.f), 0.6f },
    };
    vbb.Flush ();

    RG::IndexBufferTransferable ib (device, 6);
    ib.data = { 0, 1, 2, 0, 3, 2 };
    ib.Flush ();

    std::shared_ptr<RG::RenderOperation> redFillOperation = std::make_unique<RG::RenderOperation> (std::make_unique<RG::DrawableInfo> (1, static_cast<uint32_t> (vbb.data.size ()), vbb.buffer.GetBufferToBind (), vbb.info.bindings, vbb.info.attributes, static_cast<uint32_t> (ib.data.size ()), ib.buffer.GetBufferToBind ()),
                                                                                                   std::move (sp));

    std::shared_ptr<RG::WritableImageResource>  presentedCopy = std::make_unique<RG::WritableImageResource> (800, 600);
    std::shared_ptr<RG::SwapchainImageResource> presented     = std::make_unique<RG::SwapchainImageResource> (*presentable);

    auto& aTable2 = redFillOperation->compileSettings.attachmentProvider;
    aTable2->table.push_back ({ "outColor", RG::ShaderKind::Fragment, { presented->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_CLEAR, presented->GetImageViewForFrameProvider (), presented->GetInitialLayout (), presented->GetFinalLayout () } });
    aTable2->table.push_back ({ "outCopy", RG::ShaderKind::Fragment, { presentedCopy->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_CLEAR, presentedCopy->GetImageViewForFrameProvider (), presentedCopy->GetInitialLayout (), presentedCopy->GetFinalLayout () } });

    s.connectionSet.Add (redFillOperation, presented);
    s.connectionSet.Add (redFillOperation, presentedCopy);

    graph.Compile (std::move (s));

    RG::BlockingGraphRenderer renderer (device, swapchain);
    uint32_t                  count = 0;
    auto                      cb    = renderer.GetConditionalDrawCallback ([&] () -> RG::RenderGraph& { return graph; }, [&] () -> bool { return ++count > 10; });
    window->DoEventLoop (cb);

    CompareImages ("uv", *presentedCopy->images[0]->image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}


TEST_F (HiddenWindowTestEnvironment, RenderGraph_BasicUniformBuffer)
{
    RG::DeviceExtra& device    = GetDeviceExtra ();
    RG::Swapchain&   swapchain = GetSwapchain ();

    RG::GraphSettings s (device, swapchain.GetImageCount ());
    RG::RenderGraph   graph;

    auto sp = std::make_unique<RG::ShaderPipeline> (device);
    sp->SetVertexShaderFromString (R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 0) uniform Time {
    float time;
} time;

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in float asd;

layout (location = 0) out vec2 textureCoords;
layout (location = 1) out float asdout;


void main() {
    gl_Position = vec4 (position + vec2 (time.time), 0.0, 1.0);
    textureCoords = uv;
    asdout = asd;
}
    )");

    sp->SetFragmentShaderFromString (R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 1) in float asdout;
layout (location = 0) in vec2 uv;

layout (location = 2) out vec4 presented;
layout (location = 0) out vec4 copy[2];

void main () {
    vec4 result = vec4 (vec3 (uv, 0.f), 1);
    presented = result;
    copy[0] = result;
    copy[1] = result;
}
    )");

    struct Vert {
        glm::vec2 position;
        glm::vec2 uv;
        float     asd;
    };

    std::shared_ptr<RG::VertexBufferTransferable<Vert>> vbb = std::make_unique<RG::VertexBufferTransferable<Vert>> (
        device, 4, std::vector<VkFormat> { RG::ShaderTypes::Vec2f, RG::ShaderTypes::Vec2f, RG::ShaderTypes::Float }, VK_VERTEX_INPUT_RATE_VERTEX);

    *vbb = std::vector<Vert> {
        { glm::vec2 (-1.f, -1.f), glm::vec2 (0.f, 0.f), 0.1f },
        { glm::vec2 (-1.f, +1.f), glm::vec2 (0.f, 1.f), 0.2f },
        { glm::vec2 (+1.f, +1.f), glm::vec2 (1.f, 1.f), 0.3f },
        { glm::vec2 (+1.f, -1.f), glm::vec2 (1.f, 0.f), 0.6f },
    };
    vbb->Flush ();

    RG::IndexBufferTransferable ib (device, 6);
    ib.data = { 0, 1, 2, 0, 3, 2 };
    ib.Flush ();

    std::shared_ptr<RG::RenderOperation> redFillOperation = std::make_unique<RG::RenderOperation> (std::make_unique<RG::DrawableInfo> (1, *vbb, ib), std::move (sp));

    std::shared_ptr<RG::SwapchainImageResource> presented     = std::make_unique<RG::SwapchainImageResource> (*presentable);
    std::shared_ptr<RG::WritableImageResource>  presentedCopy = std::make_unique<RG::WritableImageResource> (VK_FILTER_LINEAR, 800, 600, 2);
    std::shared_ptr<RG::CPUBufferResource>      unif          = std::make_unique<RG::CPUBufferResource> (4);

    s.connectionSet.Add (unif);

    auto& aTable2 = redFillOperation->compileSettings.attachmentProvider;
    aTable2->table.push_back ({ "presented", RG::ShaderKind::Fragment, { presented->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_CLEAR, presented->GetImageViewForFrameProvider (), presented->GetInitialLayout (), presented->GetFinalLayout () } });
    aTable2->table.push_back ({ "copy", RG::ShaderKind::Fragment, { presentedCopy->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_CLEAR, presentedCopy->GetImageViewForFrameProvider (), presentedCopy->GetInitialLayout (), presentedCopy->GetFinalLayout () } });

    s.connectionSet.Add (redFillOperation, presented);
    s.connectionSet.Add (redFillOperation, presentedCopy);

    auto& table = redFillOperation->compileSettings.descriptorWriteProvider;
    table->bufferInfos.push_back ({ std::string ("Time"), RG::ShaderKind::Vertex, unif->GetBufferForFrameProvider (), 0, unif->GetBufferSize () });

    graph.Compile (std::move (s));

    RG::BlockingGraphRenderer renderer (device, swapchain);

    RG::EventObserver obs;
    obs.Observe (renderer.preSubmitEvent, [&] (RG::RenderGraph&, uint32_t frameIndex, uint64_t) {
        float time = 0.5f;
        unif->GetMapping (frameIndex).Copy (time);
    });

    uint32_t count = 0;
    auto     cb    = renderer.GetConditionalDrawCallback ([&] () -> RG::RenderGraph& { return graph; }, [&] () -> bool { return ++count > 10; });
    window->DoEventLoop (cb);

    CompareImages ("uvoffset", *presentedCopy->images[0]->image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}
