#include "GraphRenderer.hpp"
#include "ShaderPipeline.hpp"
#include "GraphSettings.hpp"
#include "Operation.hpp"
#include "Drawable.hpp"
#include "RenderGraph.hpp"
#include "Resource.hpp"
#include "Drawable.hpp"

#include "VulkanWrapper/DescriptorSet.hpp"
#include "VulkanWrapper/DescriptorSetLayout.hpp"
#include "VulkanWrapper/DeviceExtra.hpp"
#include "VulkanWrapper/Fence.hpp"
#include "VulkanWrapper/Image.hpp"
#include "VulkanWrapper/GraphicsPipeline.hpp"
#include "VulkanWrapper/ComputePipeline.hpp"
#include "VulkanWrapper/PipelineLayout.hpp"
#include "VulkanWrapper/RenderPass.hpp"
#include "VulkanWrapper/Semaphore.hpp"
#include "VulkanWrapper/ShaderModule.hpp"
#include "VulkanWrapper/Swapchain.hpp"


namespace RG {

IFrameDisplayObserver noOpFrameDisplayObserver;

Window::DrawCallback Renderer::GetInfiniteDrawCallback (std::function<RenderGraph&()> graphProvider)
{
    return [=] (bool&) -> void {
        RenderNextFrame (graphProvider ());
    };
}


Window::DrawCallback Renderer::GetConditionalDrawCallback (std::function<RenderGraph&()> graphProvider, std::function<bool ()> shouldStop)
{
    return [=] (bool& stopFlag) -> void {
        stopFlag = shouldStop ();

        RenderNextFrame (graphProvider ());
    };
}


RecreatableGraphRenderer::RecreatableGraphRenderer (RG::Swapchain& swapchain)
    : swapchain (swapchain)
{
}


BlockingGraphRenderer::BlockingGraphRenderer (const RG::DeviceExtra& device, RG::Swapchain& swapchain)
    : RecreatableGraphRenderer (swapchain)
    , s (std::make_unique<RG::Semaphore> (device))
{
}


uint32_t BlockingGraphRenderer::RenderNextRecreatableFrame (RenderGraph& graph, IFrameDisplayObserver&)
{
    const uint32_t currentImageIndex = swapchain.GetNextImageIndex (*s);

    {
        const RG::TimePoint currentTime = RG::TimePoint::SinceApplicationStart ();
        preSubmitEvent.Notify (graph, currentImageIndex, currentTime - lastDrawTime);
        lastDrawTime = currentTime;
    }

    graph.Submit (currentImageIndex);
    vkQueueWaitIdle (graph.graphSettings.device->GetGraphicsQueue ());
    vkDeviceWaitIdle (graph.graphSettings.device->GetDevice ());

    if (swapchain.SupportsPresenting ()) {
        graph.Present (currentImageIndex, swapchain, { *s });
        vkQueueWaitIdle (graph.graphSettings.device->GetGraphicsQueue ());
        vkDeviceWaitIdle (*graph.graphSettings.device);
    }

    return currentImageIndex;
}


SynchronizedSwapchainGraphRenderer::SynchronizedSwapchainGraphRenderer (const RG::DeviceExtra& device, RG::Swapchain& swapchain)
    : RecreatableGraphRenderer { swapchain }
    , framesInFlight { swapchain.GetImageCount () }
    , imageCount { swapchain.GetImageCount () }
    , currentResourceIndex { 0 }
    , swapchain { swapchain }
    , presentationEngineFence { std::make_unique<RG::Fence> (device, false) }
{
    presentationEngineFence->SetName (device, "presentationEngineFence");
    
    RG_ASSERT (imageCount <= framesInFlight);

    for (uint32_t i = 0; i < framesInFlight; ++i) {
        imageAvailableSemaphore.push_back (std::make_unique<RG::Semaphore> (device));
        renderFinishedSemaphore.push_back (std::make_unique<RG::Semaphore> (device));
        inFlightFences.push_back (std::make_unique<RG::Fence> (device));
        inFlightFences.back ()->SetName (device, std::string ("inFlightFence ") + std::to_string (i));
    }

    for (uint32_t i = 0; i < imageCount; ++i) {
        imageToFrameMapping.push_back (UINT32_MAX);
    }
}


void RecreatableGraphRenderer::Recreate (RenderGraph& graph)
{
    GraphSettings settings = std::move (graph.graphSettings);

    vkDeviceWaitIdle (settings.GetDevice ());
    vkQueueWaitIdle (settings.GetDevice ().GetGraphicsQueue ());

    swapchain.Recreate ();

    settings.framesInFlight = swapchain.GetImageCount ();

    graph.Compile (std::move (settings));
}


uint32_t RecreatableGraphRenderer::RenderNextFrame (RenderGraph& graph, IFrameDisplayObserver& observer)
{
    try {
        return RenderNextRecreatableFrame (graph, observer);
    } catch (RG::OutOfDateSwapchain&) {
        throw;
    }
}


uint32_t SynchronizedSwapchainGraphRenderer::RenderNextRecreatableFrame (RenderGraph& graph, IFrameDisplayObserver& frameDisplayObserver)
{
    frameDisplayObserver.OnImageFenceWaitStarted (currentResourceIndex);
    frameDisplayObserver.OnImageFenceWaitEnded (currentResourceIndex);

    frameDisplayObserver.OnImageAcquisitionStarted ();
    const uint32_t currentImageIndex = swapchain.GetNextImageIndex (*imageAvailableSemaphore[currentResourceIndex], *presentationEngineFence);
    frameDisplayObserver.OnImageAcquisitionReturned (currentResourceIndex);

    presentationEngineFence->Wait ();
    frameDisplayObserver.OnImageAcquisitionFenceSignaled (currentResourceIndex);
    presentationEngineFence->Reset ();

    // the image was last drawn by this frame, wait for its fence
    const uint32_t previousFrameIndex = imageToFrameMapping[currentImageIndex];
    if (previousFrameIndex != UINT32_MAX) {
        inFlightFences[previousFrameIndex]->Wait ();
    }

    frameDisplayObserver.OnImageAcquisitionEnded (currentResourceIndex);

    // update mapping
    imageToFrameMapping[currentImageIndex] = currentResourceIndex;

    const std::vector<VkSemaphore> submitWaitSemaphores   = { *imageAvailableSemaphore[currentResourceIndex] };
    const std::vector<VkSemaphore> submitSignalSemaphores = { *renderFinishedSemaphore[currentResourceIndex] };
    const std::vector<VkSemaphore> presentWaitSemaphores  = submitSignalSemaphores;

    inFlightFences[currentResourceIndex]->Reset ();

    {
        const RG::TimePoint currentTime = RG::TimePoint::SinceApplicationStart ();
        preSubmitEvent.Notify (graph, currentResourceIndex, currentTime - lastDrawTime);
        lastDrawTime = currentTime;
    }

    frameDisplayObserver.OnRenderStarted (currentResourceIndex);
    graph.Submit (currentResourceIndex, submitWaitSemaphores, submitSignalSemaphores, *inFlightFences[currentResourceIndex]);

    RG_ASSERT (swapchain.SupportsPresenting ());

    frameDisplayObserver.OnPresentStarted (currentResourceIndex);
    graph.Present (currentImageIndex, swapchain, presentWaitSemaphores);

    const uint32_t usedResourceIndex = currentResourceIndex;

    currentResourceIndex = (currentResourceIndex + 1) % framesInFlight;

    return usedResourceIndex;
}


SynchronizedSwapchainGraphRenderer::~SynchronizedSwapchainGraphRenderer ()
{
}


void SynchronizedSwapchainGraphRenderer::Wait ()
{
    for (auto& fence : inFlightFences) {
        fence->Wait ();
    }
}

} // namespace RG
