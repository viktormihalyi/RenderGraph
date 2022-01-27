#ifndef WINDOWB_HPP
#define WINDOWB_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include "RenderGraph/Utils/Event.hpp"
#include "RenderGraph/Utils/Noncopyable.hpp"
#include <memory>

#include <functional>
#include <vector>

#include <vulkan/vulkan.h>

namespace RG {

class RENDERGRAPH_DLL_EXPORT Window : public Noncopyable, public Nonmovable {
public:
    struct Events {
        // window events
        RG::Event<>                   shown;
        RG::Event<>                   hidden;
        RG::Event<>                   closed;
        RG::Event<>                   focused;
        RG::Event<>                   focusLost;
        RG::Event<uint32_t, uint32_t> resized;
        RG::Event<uint32_t, uint32_t> moved;
        RG::Event<>                   refresh;

        // user input
        RG::Event<int32_t>           keyPressed;
        RG::Event<int32_t>           keyReleased;
        RG::Event<int32_t, int32_t>   mouseMove;
        RG::Event<uint32_t, uint32_t> leftMouseButtonPressed;
        RG::Event<uint32_t, uint32_t> leftMouseButtonReleased;
        RG::Event<uint32_t, uint32_t> rightMouseButtonPressed;
        RG::Event<uint32_t, uint32_t> rightMouseButtonReleased;
        RG::Event<int32_t>            scroll;
    } events;

public:
    using DrawCallback = std::function<void (bool& stopFlag)>;

    enum class Mode {
        Fullscreen,
        Windowed,
    };

    virtual ~Window () = default;

    virtual void  DoEventLoop (const DrawCallback&) = 0;
    virtual void* GetHandle () const                = 0;

    virtual VkSurfaceKHR GetSurface (VkInstance instance) = 0;

    virtual void PollEvents () = 0;

    virtual uint32_t GetWidth () const       = 0;
    virtual uint32_t GetHeight () const      = 0;
    virtual float    GetAspectRatio () const = 0;
    virtual double   GetRefreshRate () const   = 0;

    virtual void Show ()  = 0;
    virtual void Hide ()  = 0;
    virtual void Focus () = 0;
    virtual void Close () = 0;

    virtual void SetTitle (const std::string&) = 0;

    virtual void SetWindowMode (Mode) = 0;
    virtual Mode GetWindowMode ()     = 0;
};

} // namespace RG

#endif