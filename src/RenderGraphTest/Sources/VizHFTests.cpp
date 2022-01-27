#include "Camera.hpp"
#include "CameraControl.hpp"
#include "TestEnvironment.hpp"

#include "RenderGraph/Drawable/Drawable.hpp"
#include "RenderGraph/Drawable/FullscreenQuad.hpp"
#include "RenderGraph/GraphRenderer.hpp"
#include "RenderGraph/GraphSettings.hpp"
#include "RenderGraph/Operation.hpp"
#include "RenderGraph/RenderGraph.hpp"
#include "RenderGraph/Resource.hpp"
#include "RenderGraph/UniformReflection.hpp"
#include "RenderGraph/BufferView.hpp"
#include "RenderGraph/ShaderPipeline.hpp"
#include "RenderGraph/VulkanEnvironment.hpp"
#include "RenderGraph/Window/GLFWWindow.hpp"
#include "RenderGraph/Window/SDLWindow.hpp"

#include "RenderGraph/Utils/Assert.hpp"
#include "RenderGraph/Utils/MultithreadedFunction.hpp"
#include "RenderGraph/Utils/Noncopyable.hpp"
#include "RenderGraph/Utils/Time.hpp"
#include "RenderGraph/Utils/Timer.hpp"
#include "RenderGraph/Utils/Utils.hpp"
#include "RenderGraph/Utils/UUID.hpp"

#include "RenderGraph/VulkanWrapper/DeviceExtra.hpp"
#include "RenderGraph/VulkanWrapper/ShaderReflection.hpp"
#include "RenderGraph/VulkanWrapper/Utils/ImageData.hpp"
#include "RenderGraph/VulkanWrapper/VulkanWrapper.hpp"

#pragma warning(push, 0)
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#pragma warning(pop)

#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>


using VizHFTests = HiddenWindowTestEnvironment;


TEST_F (VizHFTests, HF1)
{
    RG::DeviceExtra& device        = *env->deviceExtra;

    RG::Swapchain& swapchain = presentable->GetSwapchain ();

    RG::Camera   c (glm::vec3 (-1, 0, 0.5f), glm::vec3 (1, 0.0f, 0), window->GetAspectRatio ());
    RG::CameraControl cameraControl (c, window->events);

    RG::GraphSettings s (device, swapchain.GetImageCount ());
    RG::RenderGraph   graph;


    // ========================= GRAPH OPERATIONS =========================

    std::unique_ptr<RG::ShaderPipeline> sp = std::make_unique<RG::ShaderPipeline> (device);

    sp->SetVertexShaderFromString (R"(#version 450

layout (std140, binding = 4) uniform Camera {
    mat4 viewMatrix;
    mat4 VP;
    mat4 rayDirMatrix;
    vec3 camPosition;
    vec3 viewDir;
    uint displayMode;
} camera;

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec2 textureCoords;
layout (location = 1) out vec3 rayDirection;

void main ()
{
    gl_Position   = camera.VP * vec4 (position + vec2 (0 / 100.f), 0.0, 1.0);
    textureCoords = uv;
    rayDirection  = (camera.rayDirMatrix * vec4 (position, 0, 1)).xyz;
})");

    sp->SetFragmentShaderFromString (R"(#version 450

layout (std140, binding = 0) uniform Time {
    float time;
} time;

layout (std140, binding = 1) uniform Camera {
    mat4 viewMatrix;
    mat4 VP;
    mat4 rayDirMatrix;
    vec3 position;
    vec3 viewDir;
    uint displayMode;
    float asd;
} camera;

struct TestStruct {
    mat4 asd;
    vec4 aa;
    vec3 as;
};

layout (std140, binding = 5) uniform Test {
    TestStruct data[5];
    float a3434sd[5];
} tests;


layout (binding = 2) uniform sampler3D agySampler;
layout (binding = 3) uniform sampler2D matcapSampler;

layout (location = 0) in vec2 uv;
layout (location = 1) in vec3 rayDirection;

layout (location = 0) out vec4 presented;


#define EPS 1.0e-5
#define FLT_MAX 3.402823466e+38
#define FLT_MIN 1.175494351e-38
#define DBL_MAX 1.7976931348623158e+308
#define DBL_MIN 2.2250738585072014e-308



float SurfaceIntersection (vec3 rayStart, vec3 rayDir, vec3 surfacePoint, vec3 surfaceNormal)
{
    float t = (dot (surfaceNormal, surfacePoint) - dot (surfaceNormal, rayStart)) / dot (rayDir, surfaceNormal);
    return t;
}


vec2 BoxIntersection (vec3 rayStart, vec3 rayDir, vec3 middle, float radius)
{
    vec3 normalTop    = vec3 (0, 0, +1);
    vec3 normalBottom = vec3 (0, 0, -1);
    vec3 normalRight  = vec3 (0, +1, 0);
    vec3 normalLeft   = vec3 (0, -1, 0);
    vec3 normalFront  = vec3 (+1, 0, 0);
    vec3 normalBack   = vec3 (-1, 0, 0);

    float top    = SurfaceIntersection (rayStart, rayDir, middle + normalTop    * radius, normalTop);
    float bottom = SurfaceIntersection (rayStart, rayDir, middle + normalBottom * radius, normalBottom);
    float right  = SurfaceIntersection (rayStart, rayDir, middle + normalRight  * radius, normalRight);
    float left   = SurfaceIntersection (rayStart, rayDir, middle + normalLeft   * radius, normalLeft);
    float front  = SurfaceIntersection (rayStart, rayDir, middle + normalFront  * radius, normalFront);
    float back   = SurfaceIntersection (rayStart, rayDir, middle + normalBack   * radius, normalBack);

    float belepT_X = min (left, right);
    float belepT_Y = min (front, back);
    float belepT_Z = min (bottom, top);
    float belepT   = max (belepT_X, max (belepT_Y, belepT_Z));

    float kilepT_X = max (left, right);
    float kilepT_Y = max (front, back);
    float kilepT_Z = max (bottom, top);
    float kilepT   = min (kilepT_X, min (kilepT_Y, kilepT_Z));
    
    if (belepT > kilepT) {
        return vec2 (-1.f, -1.f);
    }

    // rayStart is inside the box
    if (belepT < 0 && kilepT > 0) {
        belepT = 0.f;
    }

    return vec2 (belepT, kilepT);
}


vec3 GetNormalAt (vec3 rayPos) 
{
    float delta = 1.f/256.f * 5.f;
    vec3 gradient = vec3 (
        (texture (agySampler, vec3 (rayPos.x + delta, rayPos.y, rayPos.z)).r - texture (agySampler, vec3 (rayPos.x - delta, rayPos.y, rayPos.z)).r) / (2 * delta),
        (texture (agySampler, vec3 (rayPos.x, rayPos.y + delta, rayPos.z)).r - texture (agySampler, vec3 (rayPos.x, rayPos.y - delta, rayPos.z)).r) / (2 * delta),
        (texture (agySampler, vec3 (rayPos.x, rayPos.y, rayPos.z + delta)).r - texture (agySampler, vec3 (rayPos.x, rayPos.y, rayPos.z - delta)).r) / (2 * delta)
    );
    return normalize (gradient.rgb);
}


void main ()
{
    presented = tests.data[1].aa.xyzw;
    presented = vec4 (0, 0, 0, 1);

    const int DP_BEFOGLALO = 1;
    const int DP_SZINTFELULET = 2;
    const int DP_MATCAP = 3;
    const int DP_HAGYMA = 4;
    const int DP_ARNYEK = 5;

    const vec3 normRayDir = normalize (rayDirection);

    const vec2 boxT = BoxIntersection (camera.position, normRayDir, vec3 (0.5f, 0.5f, 0.5f), 0.5f);
    const bool boxHit = (boxT.x >= 0.f && boxT.y >= 0.f);

    const int   steps = 512;
    const float rayStep = abs (boxT.y - boxT.x) / steps;
    
    float rayT = boxT.x;
    vec3  rayPos  = camera.position + normRayDir * rayT;

    float insideBrain = 0.002f;
    if (camera.displayMode == DP_HAGYMA) {
        insideBrain = 0.09f;
    }

    const float minConsideredValue = 0.01f;

    int hagymaSampledCount = 0;
    const int maxHagymaSampleCount = 6;
     
    float timeScale = 2.f;
    vec3 lightDir = normalize (vec3 (1, 0, 0));
    if (camera.displayMode == DP_ARNYEK) {
        lightDir = normalize (vec3 (cos (time.time*timeScale), sin(time.time * timeScale), 0));
    }
   
    if (boxHit) {

        float volume = 0.f;
        int brainHitCount = 0;

        vec4 hagyma = vec4 (vec3 (1), 0.0);

        for (int i = 0; i < steps; ++i) {
            float sampled = texture (agySampler, rayPos).r;
            if (sampled > minConsideredValue) {
                brainHitCount++;
                volume += sampled / steps;
                
                if (camera.displayMode == DP_HAGYMA) {
                    if (volume > (hagymaSampledCount) * insideBrain) {
                        hagyma.w = max (hagyma.w, 1.f-max (0, dot (GetNormalAt (rayPos), normalize (rayPos - camera.position))));
                        hagymaSampledCount++;
                    }
                }
            }

            bool isHit = false;
            if (camera.displayMode == DP_HAGYMA) {
                isHit = (hagymaSampledCount == maxHagymaSampleCount);
            } else  if (camera.displayMode == DP_ARNYEK) {
                isHit = (brainHitCount > 0);
            } else {
                isHit = (volume > insideBrain);
            }

            if (isHit) {
                break;
            }
            
            rayT += rayStep;
            rayPos = camera.position + normRayDir * rayT;
        }
        
        if (camera.displayMode == 1) {
            if (brainHitCount > 0) {
                presented = vec4 (vec3 (0.3f), 1.f);
            } else {
                presented = vec4 (vec3 (0.7f), 1.f);
            }
        }

        if (brainHitCount > 0) {
            const vec3 gradient = GetNormalAt (rayPos);
            const vec3 viewDir = normalize (rayPos - camera.position);
            const float angle = max (0.5, dot (gradient, lightDir));

            const vec3 lightColor = vec3 (255, 241, 224)/255.f;
            const vec3 objectColor = vec3 (217,165,178)/255.f;

            const vec3 ambient = 0.1f * lightColor;
  	
            const float diff = max(dot(gradient, lightDir), 0.08);
            const vec3 diffuse = diff * lightColor;
    
            const vec3 reflectDir = reflect(-lightDir, gradient);  
            const float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8);
            const vec3 specular = 0.5f * spec * lightColor;  

            const vec3 phongColor = (ambient + diffuse + specular) * objectColor;

        

            if (camera.displayMode == DP_SZINTFELULET) {
                presented = vec4 (phongColor, 1.f);
            
            } else if (camera.displayMode == DP_MATCAP) {
                const vec4 n              = vec4 (gradient, 1) * camera.viewMatrix;
                const vec3 matcapTexture  = texture (matcapSampler, (n.rg + 1) * 0.5).rgb;
                presented = vec4 (matcapTexture.rgb, 1.f);
            
            } else if (camera.displayMode == DP_HAGYMA) {
                presented = hagyma;
            
            } else if (camera.displayMode == DP_ARNYEK) {

                // second ray towards light dir
            
                const vec3 ray2StartPos = rayPos;
                const vec2 box2T = BoxIntersection (ray2StartPos, lightDir, vec3 (0.5f, 0.5f, 0.5f), 0.5f);

                const int shadowSamples = 256;
                const float ray2StepSize = box2T.y / shadowSamples; 
                
                vec3 ray2Pos = ray2StartPos;
                float ray2T = 0.0f;

                float ray2AccumulatedVolume = 0;

                for (int i = 0; i < shadowSamples; ++i) {
                
                    float sampled = texture (agySampler, ray2Pos).r;
                    if (sampled > minConsideredValue) {
                        ray2AccumulatedVolume += sampled / shadowSamples;
                    }

                    ray2T += ray2StepSize;
                    ray2Pos = ray2StartPos + lightDir * ray2T;
                }

                if (ray2AccumulatedVolume > 0.001f) {
                    presented = vec4 (vec3 (0), 0.2f);
                } else {
                    presented = vec4 (vec3 (phongColor), 1.f);
                }

            }
        }
    }
}
)");

    std::shared_ptr<RG::RenderOperation> brainRenderOp = std::make_unique<RG::RenderOperation> (std::make_unique<RG::FullscreenQuad> (device), std::move (sp));


    // ========================= GRAPH RESOURCES =========================

    std::shared_ptr<RG::SwapchainImageResource> presented = std::make_unique<RG::SwapchainImageResource> (*presentable);
    std::shared_ptr<RG::ReadOnlyImageResource>  matcap    = std::make_unique<RG::ReadOnlyImageResource> (VK_FORMAT_R8G8B8A8_SRGB, 512, 512);
    std::shared_ptr<RG::ReadOnlyImageResource>  agy3d     = std::make_unique<RG::ReadOnlyImageResource> (VK_FORMAT_R8_SRGB, 256, 256, 256);

    auto& aTable2 = brainRenderOp->compileSettings.attachmentProvider;
    aTable2->table.push_back ({ "presented", RG::ShaderKind::Fragment, { presented->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_CLEAR, presented->GetImageViewForFrameProvider (), presented->GetInitialLayout (), presented->GetFinalLayout () } });

    s.connectionSet.Add (brainRenderOp, presented);

    s.connectionSet.Add (agy3d);
    s.connectionSet.Add (matcap);
    s.connectionSet.Add (brainRenderOp);

    RG::UniformReflection r (s.connectionSet);

    auto& table = brainRenderOp->compileSettings.descriptorWriteProvider;
    table->imageInfos.push_back ({ std::string ("agySampler"), RG::ShaderKind::Fragment, agy3d->GetSamplerProvider (), agy3d->GetImageViewForFrameProvider (), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
    table->imageInfos.push_back ({ std::string ("matcapSampler"), RG::ShaderKind::Fragment, matcap->GetSamplerProvider (), matcap->GetImageViewForFrameProvider (), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });


    // ========================= GRAPH RESOURCE SETUP =========================

    graph.Compile (std::move (s));

    matcap->CopyTransitionTransfer (RG::ImageData (std::filesystem::current_path () / "TestData" / "VizHF" / "matcap.jpg").data);

    std::vector<uint8_t> rawBrainData = RG::ImageData (std::filesystem::current_path () / "TestData" / "VizHF" / "brain.jpg", 1).data;

    std::vector<uint8_t> transformedBrainData (256 * 256 * 256);

    auto BrainDataIndexMapping = [] (uint32_t oirignalDataIndex) {
        const uint32_t x = oirignalDataIndex % 4096;
        const uint32_t y = oirignalDataIndex / 4096;

        const uint32_t row        = x % 256;
        const uint32_t column     = y % 256;
        const uint32_t sliceIndex = 16 * (y / 256) + (x / 256);

        return sliceIndex * 256 * 256 + row + column * 256;
    };

    {
        // RG::DebugTimerLogger l ("transforming volume data");
        // RG::TimerScope       s (l);
        MultithreadedFunction d ([&] (uint32_t threadCount, uint32_t threadIndex) {
            const uint32_t pixelCount = 4096 * 4096;
            for (uint32_t bidx = pixelCount / threadCount * threadIndex; bidx < pixelCount / threadCount * (threadIndex + 1); ++bidx) {
                transformedBrainData[BrainDataIndexMapping (bidx)] = rawBrainData[bidx];
            }
        });
    }

    agy3d->CopyTransitionTransfer (transformedBrainData);


    // ========================= RENDERING =========================

    RG::SynchronizedSwapchainGraphRenderer renderer (device, swapchain);

    enum class DisplayMode : uint32_t {
        Feladat1 = 1,
        Feladat2,
        Feladat3,
        Feladat4,
        Feladat5,
        Feladat6,
    };

#if 0
    std::cout << "1: Befoglalo" << std::endl;
    std::cout << "2: Szintfelulet (Phong)" << std::endl;
    std::cout << "3: Matcap" << std::endl;
    std::cout << "4: Hagymahej" << std::endl;
    std::cout << "5: Arnyek" << std::endl;
#endif

    DisplayMode currentDisplayMode = DisplayMode::Feladat2;

    bool quit = false;

    RG::EventObserver obs;
    obs.Observe (window->events.keyPressed, [&] (int32_t key) {
        constexpr uint8_t ESC_CODE = 27;
        if (key == ESC_CODE) {
            // window->ToggleFullscreen ();
            quit = true;
        }
        if (key == 'R') {
            #if 0
            // TODO use of moved
            std::cout << "waiting for device... " << std::endl;
            vkDeviceWaitIdle (s.GetDevice ());
            vkQueueWaitIdle (s.GetDevice ().GetGraphicsQueue ());
            renderer.Recreate (graph);
            #endif
        }
        switch (key) {
            case '1': currentDisplayMode = DisplayMode::Feladat1; break;
            case '2': currentDisplayMode = DisplayMode::Feladat2; break;
            case '3': currentDisplayMode = DisplayMode::Feladat3; break;
            case '4': currentDisplayMode = DisplayMode::Feladat4; break;
            case '5': currentDisplayMode = DisplayMode::Feladat5; break;
        }
    });

    r[*brainRenderOp][RG::ShaderKind::Vertex]["Camera"]["VP"] = glm::mat4 (1.f);
    r[*brainRenderOp][RG::ShaderKind::Fragment]["Camera"]["VP"] = glm::mat4 (1.f);

    obs.Observe (renderer.preSubmitEvent, [&] (RG::RenderGraph&, uint32_t frameIndex, uint64_t deltaNs) {
        RG::TimePoint delta (deltaNs);

        const float dt = static_cast<float> (delta.AsSeconds ());

        cameraControl.UpdatePosition (dt);

        {
            r[*brainRenderOp][RG::ShaderKind::Vertex]["Camera"]["viewMatrix"]   = c.GetViewMatrix ();
            r[*brainRenderOp][RG::ShaderKind::Vertex]["Camera"]["rayDirMatrix"] = c.GetRayDirMatrix ();
            r[*brainRenderOp][RG::ShaderKind::Vertex]["Camera"]["camPosition"]  = c.GetPosition ();
            r[*brainRenderOp][RG::ShaderKind::Vertex]["Camera"]["viewDir"]      = c.GetViewDirection ();
            r[*brainRenderOp][RG::ShaderKind::Vertex]["Camera"]["displayMode"]  = static_cast<uint32_t> (currentDisplayMode);

            r[*brainRenderOp][RG::ShaderKind::Fragment]["Camera"]["viewMatrix"]  = c.GetViewMatrix ();
            r[*brainRenderOp][RG::ShaderKind::Fragment]["Camera"]["viewMatrix"]  = c.GetRayDirMatrix ();
            r[*brainRenderOp][RG::ShaderKind::Fragment]["Time"]["time"]          = static_cast<float> (RG::TimePoint::SinceApplicationStart ().AsSeconds ());
            r[*brainRenderOp][RG::ShaderKind::Fragment]["Camera"]["position"]    = c.GetPosition ();
            r[*brainRenderOp][RG::ShaderKind::Fragment]["Camera"]["viewDir"]     = c.GetViewDirection ();
            r[*brainRenderOp][RG::ShaderKind::Fragment]["Camera"]["displayMode"] = static_cast<uint32_t> (currentDisplayMode);
        }

        r.Flush (frameIndex);
    });

    renderer.RenderNextRecreatableFrame (graph);
    renderer.RenderNextRecreatableFrame (graph);
    renderer.RenderNextRecreatableFrame (graph);
    renderer.RenderNextRecreatableFrame (graph);
    renderer.RenderNextRecreatableFrame (graph);
    renderer.RenderNextRecreatableFrame (graph);

    RG::ImageData sw (GetDeviceExtra (), *swapchain.GetImageObjects ()[0], 0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    sw.ConvertBGRToRGB ();

    CompareImages ("VizHF1_InitialState", sw);
}


static const glm::mat4 UnitMatrix = glm::mat4 (
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f);

class QuadricMat4 : public glm::mat4 {
public:
    QuadricMat4 ()
    {
    }

    QuadricMat4 (const glm::mat4& m)
    {
        glm::mat4::operator= (m);
    }

    QuadricMat4 Scale (const glm::vec3& v) const
    {
        return QuadricTransform (*this, glm::scale (UnitMatrix, v));
    }

    QuadricMat4 Rotate (float angle, const glm::vec3& v)
    {
        return QuadricTransform (*this, glm::rotate (UnitMatrix, angle, v));
    }

    QuadricMat4 Translate (const glm::vec3& v) const
    {
        return QuadricTransform (*this, glm::translate (UnitMatrix, v));
    }

private:
    static glm::mat4 QuadricTransform (const glm::mat4& surface, const glm::mat4& trafo)
    {
        const glm::mat4 tinv = glm::inverse (trafo);
        return glm::transpose (tinv) * surface * tinv;
    }
};

struct Quadric {
    QuadricMat4 surface;
    QuadricMat4 clipper;
    glm::vec4   kd { 0.0 };
    glm::vec4   reflectance { 0.0 };
    glm::vec4   ks { 0.0 };
    glm::vec4   transmittace { 0.0 };
};


TEST_F (VizHFTests, HF2)
{
    RG::DeviceExtra& device        = *env->deviceExtra;
    RG::DeviceExtra& deviceExtra   = device;

    RG::Swapchain& swapchain = presentable->GetSwapchain ();

    RG::Camera        c (glm::vec3 (-0.553508, -4.64034, 8.00851), glm::vec3 (0.621111, 0.629845, -0.466387), window->GetAspectRatio ());
    RG::CameraControl cameraControl (c, window->events);
    c.SetSpeed (3.f);

    RG::GraphSettings s (deviceExtra, swapchain.GetImageCount ());
    RG::RenderGraph   graph;


    // ========================= GRAPH OPERATIONS & RESOURCES =========================

    std::unique_ptr<RG::ShaderPipeline> sp = std::make_unique<RG::ShaderPipeline> (device);

    sp->SetVertexShaderFromString (R"(#version 450

layout (std140, binding = 4) uniform Camera {
    mat4 viewMatrix;
    mat4 VP;
    mat4 rayDirMatrix;
    vec3 camPosition;
    vec3 viewDir;
    uint displayMode;
} camera;

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec2 textureCoords;
layout (location = 1) out vec3 rayDirection;

void main ()
{
    gl_Position   = camera.VP * vec4 (position + vec2 (0 / 100.f), 0.0, 1.0);
    textureCoords = uv;
    rayDirection  = (camera.rayDirMatrix * vec4 (position, 0, 1)).xyz;
}
)");

    sp->SetFragmentShaderFromString (R"(
#version 450

layout (std140, binding = 1) uniform Camera {
    mat4 viewMatrix;
    mat4 VP;
    mat4 rayDirMatrix;
    vec3 position;
    vec3 viewDir;
    uint displayMode;
    float asd;
} camera;

struct Quadric {
    mat4 surface;
    mat4 clipper;
    vec4 kd;
    vec4 reflectance;
    vec4 ks;
    vec4 transmittace;
};

struct Light {
    vec4 position;
    vec4 powerDensity;
};

#define QUADRIC_COUNT 6
#define LIGHT_COUNT 4

layout (std140, binding = 2) uniform Quadrics {
    Quadric quadrics[QUADRIC_COUNT];
};


layout (std140, binding = 3) uniform Lights {
    Light lights[LIGHT_COUNT];
};


layout (location = 0) in vec2 uv;
layout (location = 1) in vec3 rayDirection;

layout (location = 0) out vec4 presented;


float IntersectClippedQuadric (const vec4 e, const vec4 d, const mat4 coeff, const mat4 clipper)
{
    const float a = dot (d * coeff, d);
    const float b = dot (e * coeff, d) + dot (d * coeff, e);
    const float c = dot (e * coeff, e);

    const float disc = b * b - 4.f * a * c;
    if (disc < 0.f)
      return -1.0;

    float t1 = (-b - sqrt (disc)) / (2.f * a);
    float t2 = (-b + sqrt (disc)) / (2.f * a);

    const vec4 h1 = e + d * t1;	
    const vec4 h2 = e + d * t2;

    if (dot (h1 * clipper, h1) > 0.0) t1 = -1.0;
    if (dot (h2 * clipper, h2) > 0.0) t2 = -1.0;	

    return (t1 < 0.f)
        ? t2
        : ((t2 < 0.f)
            ? t1
            : min (t1, t2));
}


struct Hit {
    float t;
    vec4  position;
    uint  quadricIndex;
};


Hit FindBestHit (vec4 e, vec4 d) {
    Hit bestHit;
    bestHit.t = 100000.0;
    bestHit.quadricIndex = 0;

    for (int i = 0; i < QUADRIC_COUNT; ++i) {
        const float t = IntersectClippedQuadric (e, d, quadrics[i].surface, quadrics[i].clipper);
        if (0.f < t && t < bestHit.t) {
            bestHit.t = t;
            bestHit.quadricIndex = i;
        }
    }

    if (bestHit.t > 100000.0 - 1) {
        bestHit.t = -1.0;
    } else {
        bestHit.position = e + d * bestHit.t;
    }

    return bestHit;
}


vec3 Shade (vec3 normal, vec3 viewDir, vec3 lightDir, vec3 powerDensity, vec3 kd, vec3 ks) 
{
    const vec3 halfway = normalize (viewDir + lightDir);
    
    if (dot (normal, lightDir) < 0.f) {
        return vec3 (0, 0, 0);
    }
    
    const float cosa = max (dot (normal, lightDir), 0.f);
    const float cosb = max (dot (normal, viewDir), 0.f);

    return 
        powerDensity * kd * cosa
        +
        powerDensity * ks * pow (max (dot (halfway, normal), 0), 20) * cosa / max (cosa, cosb)
        ;
}


#define EPS 1e-3

const bool showShadows = true;

vec3 DirectLighting (vec4 x, vec3 normal, vec3 viewDir, vec3 kd, vec3 ks)
{
    vec3 radiance = vec3 (0, 0, 0);
    for (int i = 0; i < LIGHT_COUNT; ++i) {

        const vec3 lightDiff    = lights[i].position.xyz - x.xyz * lights[i].position.w;
        const vec3 lightDir     = normalize (lightDiff);
        const vec3 powerDensity = lights[i].powerDensity.rgb / dot (lightDiff, lightDiff);
    
        const Hit shadowHit = FindBestHit (x + vec4 (normal, 0) * EPS, vec4 (lightDir, 0));
        if (!showShadows || shadowHit.t < 0.f) {
            radiance += Shade (normal, viewDir, lightDir, powerDensity, kd, ks);
        }

    }
    return radiance;
}


vec3 GetQuadricNormal (mat4 surface, vec4 hitPosition)
{
    return normalize ((hitPosition * surface + surface * hitPosition).xyz);
}


struct Ray {
    vec4 startPos;
    vec4 direction;
    vec3 reflectanceProduct;
    bool inside;
};


void main ()
{
    vec3 radiance = vec3 (0, 0, 0);
    
    const vec3 background = vec3 (0.02);

    #define MAX_RAY_COUNT 32
    Ray rays[MAX_RAY_COUNT];
    int rayCount = 0;

    #define ADD_RAY(r) if (rayCount < MAX_RAY_COUNT) {rays[rayCount] = r; rayCount++;} 

    Ray initialRay;
    initialRay.startPos           = vec4 (camera.position, 1);
    initialRay.direction          = vec4 (normalize (rayDirection.xyz), 0);
    initialRay.reflectanceProduct = vec3 (1, 1, 1);
    initialRay.inside             = false;
    ADD_RAY (initialRay);

    const int maxStartedRays = 16;
    int startedRays = 0;

    while (rayCount > 0 && startedRays++ <= maxStartedRays) {
        const Ray currentRay = rays[rayCount - 1];
        rayCount--;

        const Hit hit = FindBestHit (currentRay.startPos, currentRay.direction);

        if (hit.t > 0.f) {
            vec3 normal = GetQuadricNormal (quadrics[hit.quadricIndex].surface, hit.position);
        
            if (dot (normal, currentRay.direction.xyz) > 0.f) {
                normal = -normal;
            }
    
            radiance +=
                currentRay.reflectanceProduct * 
                DirectLighting (hit.position, normal, -currentRay.direction.xyz, quadrics[hit.quadricIndex].kd.rgb, quadrics[hit.quadricIndex].ks.rgb);

            {
                Ray reflectedRay;
                reflectedRay.startPos           = hit.position + vec4 (normal, 0) * EPS;
                reflectedRay.direction          = vec4 (reflect (currentRay.direction.xyz, normal), 0);
                reflectedRay.reflectanceProduct = currentRay.reflectanceProduct * quadrics[hit.quadricIndex].reflectance.rgb;
                reflectedRay.inside             = currentRay.inside;
                if (length (reflectedRay.reflectanceProduct) > EPS) {
                    ADD_RAY (reflectedRay);
                }
            }

            {
                const float refractionIndex = 1.333f;
                const float eta = (currentRay.inside) ? 1.0 / refractionIndex : refractionIndex;

                Ray refractedRay;
                refractedRay.startPos           = hit.position - vec4 (normal, 0) * EPS;
                refractedRay.direction          = vec4 (refract (currentRay.direction.xyz, normal, eta), 0);
                refractedRay.reflectanceProduct = currentRay.reflectanceProduct * quadrics[hit.quadricIndex].transmittace.rgb;
                refractedRay.inside             = !currentRay.inside;
                if (length (refractedRay.reflectanceProduct) > EPS) {
                    ADD_RAY (refractedRay);
                }
            }

        } else {
            radiance += currentRay.reflectanceProduct * background;
        }
    }

    presented = vec4 (radiance, 1);
}
)");

    std::shared_ptr<RG::RenderOperation> brainRenderOp = std::make_unique<RG::RenderOperation> (std::make_unique<RG::FullscreenQuad> (deviceExtra), std::move (sp));

    std::shared_ptr<RG::SwapchainImageResource> presented = std::make_unique<RG::SwapchainImageResource> (*presentable);

    // ========================= GRAPH CONNECTIONS =========================

    auto& aTable2 = brainRenderOp->compileSettings.attachmentProvider;
    aTable2->table.push_back ({ "presented", RG::ShaderKind::Fragment, { presented->GetFormatProvider (), VK_ATTACHMENT_LOAD_OP_CLEAR, presented->GetImageViewForFrameProvider (), presented->GetInitialLayout (), presented->GetFinalLayout () } });

    s.connectionSet.Add (brainRenderOp, presented);

    // ========================= UNIFORM REFLECTION =========================

    RG::UniformReflection refl (s.connectionSet);

    // ========================= GRAPH COMPILE =========================

    graph.Compile (std::move (s));

    // ========================= RENDERING =========================

    RG::SynchronizedSwapchainGraphRenderer renderer (device, swapchain);

    bool quit = false;

    RG::EventObserver obs;
    obs.Observe (window->events.keyPressed, [&] (int32_t key) {
        constexpr uint8_t ESC_CODE = 27;
        if (key == ESC_CODE) {
            // window->ToggleFullscreen ();
            quit = true;
        }
        if (key == 'R') {
            #if 0
            // TODO use of moved
            std::cout << "waiting for device... " << std::endl;
            vkDeviceWaitIdle (*graph.graphSettings.device);
            vkQueueWaitIdle (graph.graphSettings.device->GetGraphicsQueue ());
            sp->Reload ();
            renderer.Recreate (graph);
            #endif
        }
    });

    refl[brainRenderOp][RG::ShaderKind::Vertex]["Camera"]["VP"]   = glm::mat4 (1.f);
    refl[brainRenderOp][RG::ShaderKind::Fragment]["Camera"]["VP"] = glm::mat4 (1.f);

    enum class QuadricSurfaceType : uint32_t {
        Diffuse = 0,
        Tukor   = 1,
        Toro    = 2,
    };

    const QuadricMat4 zplane (glm::mat4 (
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, -1.0f));

    const QuadricMat4 unitSphere (glm::mat4 (
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, -1.0f));

    const QuadricMat4 cylinder (glm::mat4 (
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, -1.0f));

    const QuadricMat4 hyperboloid (glm::mat4 (
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, -1.0f));

    const QuadricMat4 paraboloid (glm::mat4 (
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.5f,
        0.0f, 0.0f, 0.5f, 0.0f));

    const QuadricMat4 all (glm::mat4 (
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, -1.0f));

    const QuadricMat4 outside (glm::mat4 (
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f));

    struct Light {
        glm::vec4 position;
        glm::vec4 powerDensity;
    };

    std::vector<Quadric> quadrics;
    {
        quadrics.emplace_back ();
#define QLAST quadrics.size () - 1
        quadrics[QLAST].surface = zplane.Translate (glm::vec3 (0, 0, -3));
        quadrics[QLAST].clipper = all;
        quadrics[QLAST].kd      = glm::vec4 (0.1, 0.2, 0.2, 0);

        quadrics.emplace_back ();
        quadrics[QLAST].surface = unitSphere.Scale (glm::vec3 (1.2, 1.2, 0.5)).Translate (glm::vec3 (4, -3, 3));
        quadrics[QLAST].clipper = all;
        quadrics[QLAST].kd      = glm::vec4 (1, 0.1, 0.1, 0);
        quadrics[QLAST].ks      = glm::vec4 (0.5, 0.5, 0, 0);

        quadrics.emplace_back ();
        quadrics[QLAST].surface = paraboloid.Translate (glm::vec3 (4, 0, 3));
        quadrics[QLAST].clipper = unitSphere.Translate (glm::vec3 (4, 0, 3));
        quadrics[QLAST].kd      = glm::vec4 (0.1, 1, 0.1, 0);

        // idealias tukor
        quadrics.emplace_back ();
        quadrics[QLAST].surface     = unitSphere.Scale (glm::vec3 (8, 1, 5)).Translate (glm::vec3 (4, 14, 6));
        quadrics[QLAST].clipper     = all;
        quadrics[QLAST].reflectance = glm::vec4 (1, 1, 1, 0);

        // uveg
        quadrics.emplace_back ();
        quadrics[QLAST].surface      = unitSphere.Translate (glm::vec3 (4, 0, 6));
        quadrics[QLAST].clipper      = all;
        quadrics[QLAST].reflectance  = glm::vec4 (0.75, 0.75, 0.75, 0);
        quadrics[QLAST].transmittace = glm::vec4 (0.75, 0.75, 0.75, 0);

        // idealis toro
        quadrics.emplace_back ();
        quadrics[QLAST].surface      = unitSphere.Translate (glm::vec3 (4, -3, 6));
        quadrics[QLAST].clipper      = all;
        quadrics[QLAST].transmittace = glm::vec4 (1, 1, 1, 0);
    }

    std::vector<Light> lights;
    {
        lights.emplace_back ();
        lights[0].position     = glm::normalize (glm::vec4 (0.6, -0.6, 1, 0));
        lights[0].powerDensity = glm::vec4 (0.1, 0.1, 0.1, 0);

        lights.emplace_back ();
        lights[1].position     = glm::vec4 (10, 0, 10, 1);
        lights[1].powerDensity = glm::vec4 (0.95, 0.87, 0.8, 0) * 300.f;

        lights.emplace_back ();
        lights[2].position     = glm::vec4 (0, 0, 10, 1);
        lights[2].powerDensity = glm::vec4 (0.1, 0.87, 0.34, 0) * 300.f;

        lights.emplace_back ();
        lights[3].position     = glm::vec4 (5, 5, 4, 1);
        lights[3].powerDensity = glm::vec4 (187 / 255.f, 143 / 255.f, 206 / 255.f, 0) * 100.f;
    }

    refl[brainRenderOp][RG::ShaderKind::Fragment]["Quadrics"] = quadrics;
    refl[brainRenderOp][RG::ShaderKind::Fragment]["Lights"]   = lights;

    obs.Observe (renderer.preSubmitEvent, [&] (RG::RenderGraph&, uint32_t frameIndex, uint64_t deltaNs) {
        RG::TimePoint delta (deltaNs);

        const float dt = static_cast<float> (delta.AsSeconds ());

        cameraControl.UpdatePosition (dt);

        {
            refl[brainRenderOp][RG::ShaderKind::Vertex]["Camera"]["viewMatrix"]   = c.GetViewMatrix ();
            refl[brainRenderOp][RG::ShaderKind::Vertex]["Camera"]["rayDirMatrix"] = c.GetRayDirMatrix ();
            refl[brainRenderOp][RG::ShaderKind::Vertex]["Camera"]["camPosition"]  = c.GetPosition ();
            refl[brainRenderOp][RG::ShaderKind::Vertex]["Camera"]["viewDir"]      = c.GetViewDirection ();

            refl[brainRenderOp][RG::ShaderKind::Fragment]["Camera"]["viewMatrix"]   = c.GetViewMatrix ();
            refl[brainRenderOp][RG::ShaderKind::Fragment]["Camera"]["rayDirMatrix"] = c.GetRayDirMatrix ();
            refl[brainRenderOp][RG::ShaderKind::Fragment]["Camera"]["position"]     = c.GetPosition ();
            refl[brainRenderOp][RG::ShaderKind::Fragment]["Camera"]["viewDir"]      = c.GetViewDirection ();
        }

        refl.Flush (frameIndex);
    });

    renderer.RenderNextRecreatableFrame (graph);
    renderer.RenderNextRecreatableFrame (graph);
    renderer.RenderNextRecreatableFrame (graph);
    renderer.RenderNextRecreatableFrame (graph);
    renderer.RenderNextRecreatableFrame (graph);
    renderer.RenderNextRecreatableFrame (graph);

    RG::ImageData sw (GetDeviceExtra (), *swapchain.GetImageObjects ()[0], 0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    sw.ConvertBGRToRGB ();

    CompareImages ("VizHF2_InitialState", sw);
}
