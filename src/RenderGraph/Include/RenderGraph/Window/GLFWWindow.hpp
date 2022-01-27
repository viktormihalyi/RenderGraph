#ifndef GLFWWINDOW_HPP
#define GLFWWINDOW_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include "Window.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace RG {

class RENDERGRAPH_DLL_EXPORT GLFWWindowBase : public Window {
private:
    struct Impl;
    std::unique_ptr<Impl> impl;

protected:
    GLFWWindowBase (size_t width, size_t height, const std::vector<std::pair<int, int>>& hints, bool useFullscreen, bool hideMouse);
    GLFWWindowBase (const std::vector<std::pair<int, int>>& hints, bool useFullscreen, bool hideMouse);

public:
    virtual ~GLFWWindowBase () override;

    void  DoEventLoop (const DrawCallback&) override;
    void* GetHandle () const override;

    virtual uint32_t GetWidth () const override;
    virtual uint32_t GetHeight () const override;
    virtual float    GetAspectRatio () const override;
    virtual double   GetRefreshRate () const override;

    virtual void SetWindowMode (Mode) override;
    virtual Mode GetWindowMode () override;

    virtual void Show () override;
    virtual void Hide () override;
    virtual void Focus () override;

    virtual void SetTitle (const std::string&) override;

    virtual VkSurfaceKHR GetSurface (VkInstance instance) override;
    virtual void         PollEvents () override;
    virtual void         Close () override;
};


class RENDERGRAPH_DLL_EXPORT GLFWWindow : public GLFWWindowBase {
public:
    GLFWWindow ();
    virtual ~GLFWWindow () = default;
};


class RENDERGRAPH_DLL_EXPORT FullscreenGLFWWindow : public GLFWWindowBase {
public:
    FullscreenGLFWWindow ();
    virtual ~FullscreenGLFWWindow () = default;
};

class RENDERGRAPH_DLL_EXPORT HiddenGLFWWindow : public GLFWWindowBase {
public:
    HiddenGLFWWindow ();
    HiddenGLFWWindow (size_t width, size_t height);
    virtual ~HiddenGLFWWindow () = default;
};


RENDERGRAPH_DLL_EXPORT
std::vector<const char*> GetGLFWInstanceExtensions ();

} // namespace RG

#endif