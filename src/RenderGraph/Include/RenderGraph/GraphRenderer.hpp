#ifndef GRAPHRENDERER_HPP
#define GRAPHRENDERER_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include "RenderGraph/Window/Window.hpp"

#include "RenderGraph/Utils/Event.hpp"
#include "RenderGraph/Utils/Time.hpp"

#include <memory>

namespace RG {
class DeviceExtra;
class Semaphore;
class Swapchain;
class Fence;
} // namespace RG

namespace RG {

class RenderGraph;
class GraphSettings;


class RENDERGRAPH_DLL_EXPORT IFrameDisplayObserver {
public:
    virtual ~IFrameDisplayObserver () = default;

    virtual void OnImageFenceWaitStarted (uint32_t) {}
    virtual void OnImageFenceWaitEnded (uint32_t) {}
    virtual void OnImageAcquisitionStarted () {}
    virtual void OnImageAcquisitionReturned (uint32_t) {}
    virtual void OnImageAcquisitionFenceSignaled (uint32_t) {}
    virtual void OnImageAcquisitionEnded (uint32_t) {}
    virtual void OnRenderStarted (uint32_t) {}
    virtual void OnPresentStarted (uint32_t) {}
};

extern RENDERGRAPH_DLL_EXPORT IFrameDisplayObserver noOpFrameDisplayObserver;


class RENDERGRAPH_DLL_EXPORT Renderer {
public:
    RG::Event<RenderGraph&, uint32_t, uint64_t> preSubmitEvent;

    virtual ~Renderer () = default;

    virtual uint32_t GetNextRenderResourceIndex () const = 0;
    virtual uint32_t RenderNextFrame (RenderGraph& graph, IFrameDisplayObserver& observer = noOpFrameDisplayObserver) = 0;

    Window::DrawCallback GetInfiniteDrawCallback (std::function<RenderGraph&()> graphProvider);

    Window::DrawCallback GetConditionalDrawCallback (std::function<RenderGraph&()> graphProvider, std::function<bool ()> shouldStop);
};


class RENDERGRAPH_DLL_EXPORT RecreatableGraphRenderer : public Renderer {
protected:
    RG::Swapchain& swapchain;

public:
    RecreatableGraphRenderer (RG::Swapchain& swapchain);
    virtual ~RecreatableGraphRenderer () = default;

    void Recreate (RenderGraph& graph);

    uint32_t         RenderNextFrame (RenderGraph& graph, IFrameDisplayObserver& observer = noOpFrameDisplayObserver) override;
    virtual uint32_t RenderNextRecreatableFrame (RenderGraph& graph, IFrameDisplayObserver& observer = noOpFrameDisplayObserver) = 0;
};


class RENDERGRAPH_DLL_EXPORT BlockingGraphRenderer final : public RecreatableGraphRenderer {
private:
    std::unique_ptr<RG::Semaphore> s;
    RG::TimePoint                  lastDrawTime;

public:
    BlockingGraphRenderer (const RG::DeviceExtra& device, RG::Swapchain& swapchain);

    virtual uint32_t GetNextRenderResourceIndex () const override { return 0; }
    uint32_t         RenderNextRecreatableFrame (RenderGraph& graph, IFrameDisplayObserver& observer = noOpFrameDisplayObserver) override;
};


class RENDERGRAPH_DLL_EXPORT SynchronizedSwapchainGraphRenderer final : public RecreatableGraphRenderer {
private:
    // number of render operations able to run simultaneously
    // optimally equal to imageCount, but may be lower.
    // doesnt make sense to be higher than imageCount
    const uint32_t framesInFlight;

    // number of images in Swapchain
    const uint32_t imageCount;

    uint32_t currentResourceIndex;

    // size is framesInFlight
    // synchronization objects for each frame in flight
    std::vector<std::unique_ptr<RG::Semaphore>> imageAvailableSemaphore; // present signals, submit  waits
    std::vector<std::unique_ptr<RG::Semaphore>> renderFinishedSemaphore; // submit  signals, present waits
    std::vector<std::unique_ptr<RG::Fence>>     inFlightFences;          // waited before submit, signaled by submit

    std::unique_ptr<RG::Fence> presentationEngineFence;

    // size is imageCount
    // determines what frame is rendering to each swapchain image
    // each value is [0, framesInFlight)
    std::vector<uint32_t> imageToFrameMapping;

    RG::Swapchain& swapchain;
    RG::TimePoint  lastDrawTime;

public:
    SynchronizedSwapchainGraphRenderer (const RG::DeviceExtra& device, RG::Swapchain& swapchain);

    ~SynchronizedSwapchainGraphRenderer ();

    void Wait ();

    virtual uint32_t GetNextRenderResourceIndex () const override { return currentResourceIndex; }
    uint32_t         GetFramesInFlight () { return framesInFlight; }

    uint32_t         RenderNextRecreatableFrame (RenderGraph& graph, IFrameDisplayObserver& observer = noOpFrameDisplayObserver) override;
};

} // namespace RG

#endif