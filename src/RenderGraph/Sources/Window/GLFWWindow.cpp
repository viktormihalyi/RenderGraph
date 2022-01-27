#include "GLFWWindow.hpp"

#include <vulkan/vulkan.h>

#pragma warning (push, 0)
#ifdef _WIN32
#include <Windows.h>
#endif
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#pragma warning(pop)

#include "Utils/Assert.hpp"
#include "Utils/Timer.hpp"
#include <iostream>

#include "spdlog/spdlog.h"

namespace RG {


static void error_callback (int error, const char *description)
{
    spdlog::error ("GLFW ERROR: {}, {}", error, description);
}


struct GLFWInitializer {
private:
    bool initialized;

public:
    GLFWInitializer ()
        : initialized (false)
    {
    }

    void EnsureInitialized ()
    {
        if (!initialized) {
            initialized = true;
            glfwInitHint (GLFW_JOYSTICK_HAT_BUTTONS, GLFW_FALSE);
            int result = glfwInit ();
            if (RG_ERROR (result != GLFW_TRUE)) {
                std::terminate ();
            }
            
            glfwSetErrorCallback (error_callback);
        }
    }

    ~GLFWInitializer ()
    {
        if (initialized) {
            glfwTerminate ();
        }
    }
};


static GLFWInitializer globalGLFWInitializer;


constexpr uint32_t InitialWindowWidth  = 800;
constexpr uint32_t InitialWindowHeight = 600;


struct GLFWWindowBase::Impl {
    GLFWwindow*  window;
    VkSurfaceKHR surface;

    size_t   width;
    size_t   height;
    uint32_t refreshRate;

    uint32_t widthWindowed;
    uint32_t heightWindowed;
    int32_t  posXWindowed;
    int32_t  posYWindowed;

    Window::Mode mode;

    Impl ()
        : window (nullptr)
        , surface (VK_NULL_HANDLE)

        , width (0)
        , height (0)
        , refreshRate (0)

        , widthWindowed (0)
        , heightWindowed (0)
        , posXWindowed (0)
        , posYWindowed (0)
        , mode (Window::Mode::Windowed)
    {
    }
};


GLFWWindowBase::GLFWWindowBase (const std::vector<std::pair<int, int>>& hints, bool useFullscreen, bool hideMouse)
    : GLFWWindowBase (InitialWindowWidth, InitialWindowHeight, hints, useFullscreen, hideMouse)
{
}


GLFWWindowBase::GLFWWindowBase (size_t width, size_t height, const std::vector<std::pair<int, int>>& hints, bool useFullscreen, bool hideMouse)
    : impl (std::make_unique<Impl> ())
{
    impl->width  = width;
    impl->height = height;

    globalGLFWInitializer.EnsureInitialized ();

    RG_ASSERT (glfwVulkanSupported () == GLFW_TRUE);

    glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint (GLFW_RESIZABLE, GLFW_TRUE);

    for (auto hint : hints) {
        glfwWindowHint (hint.first, hint.second);
    }

    // monitor settings

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor ();
    glfwSetMonitorCallback ([] (GLFWmonitor* /* monitor */, int /* event */) {
        // TODO
    });

    int width_mm, height_mm;
    glfwGetMonitorPhysicalSize (primaryMonitor, &width_mm, &height_mm);

    float xscale, yscale;
    glfwGetMonitorContentScale (primaryMonitor, &xscale, &yscale);
    RG_ASSERT (xscale == 1.f && yscale == 1.f); // TODO handle different content scale (window size != framebuffer size)

    int virtual_xpos, virtual_ypos;
    glfwGetMonitorPos (primaryMonitor, &virtual_xpos, &virtual_ypos);

    GLFWmonitor* usedMonitor = nullptr;

    const GLFWvidmode* mode = glfwGetVideoMode (primaryMonitor);
    impl->refreshRate       = mode->refreshRate;

    if (useFullscreen) {
        impl->width  = mode->width;
        impl->height = mode->height;
        usedMonitor  = primaryMonitor;
    }

    impl->window = glfwCreateWindow (static_cast<int> (impl->width), static_cast<int> (impl->height), "GearsVk", usedMonitor, nullptr);
    if (RG_ERROR (impl->window == nullptr)) {
        throw std::runtime_error ("failed to create window");
    }

    // window settings

    glfwSetWindowUserPointer (impl->window, this);
    if (hideMouse) {
        glfwSetInputMode (impl->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN); // GLFW_CURSOR_DISABLED
    }

    // callbacks

    glfwSetKeyCallback (impl->window, [] (GLFWwindow* window, int key, int /* scancode */, int action, int /* mods */) {
        GLFWWindowBase* self = static_cast<GLFWWindowBase*> (glfwGetWindowUserPointer (window));

        //const char* keyName = glfwGetKeyName (key, 0);

        if (action == GLFW_PRESS) {
            self->events.keyPressed (key);
        } else if (action == GLFW_RELEASE) {
            self->events.keyReleased (key);
        }
    });


    glfwSetCursorPosCallback (impl->window, [] (GLFWwindow* window, double xpos, double ypos) {
        GLFWWindowBase* self = static_cast<GLFWWindowBase*> (glfwGetWindowUserPointer (window));

        self->events.mouseMove (static_cast<uint32_t> (xpos), static_cast<uint32_t> (ypos));
    });


    glfwSetMouseButtonCallback (impl->window, [] (GLFWwindow* window, int button, int action, int /* mods */) {
        GLFWWindowBase* self = static_cast<GLFWWindowBase*> (glfwGetWindowUserPointer (window));

        double x, y;
        glfwGetCursorPos (window, &x, &y);

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            self->events.leftMouseButtonPressed (static_cast<uint32_t> (x), static_cast<uint32_t> (y));
        } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
            self->events.leftMouseButtonReleased (static_cast<uint32_t> (x), static_cast<uint32_t> (y));
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
            self->events.rightMouseButtonPressed (static_cast<uint32_t> (x), static_cast<uint32_t> (y));
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
            self->events.rightMouseButtonReleased (static_cast<uint32_t> (x), static_cast<uint32_t> (y));
        }
    });


    glfwSetScrollCallback (impl->window, [] (GLFWwindow* window, double /* xoffset */, double yoffset) {
        GLFWWindowBase* self = static_cast<GLFWWindowBase*> (glfwGetWindowUserPointer (window));

        self->events.scroll (static_cast<uint32_t> (yoffset));
    });

    glfwSetWindowSizeCallback (impl->window, [] (GLFWwindow* window, int width, int height) {
        GLFWWindowBase* self = static_cast<GLFWWindowBase*> (glfwGetWindowUserPointer (window));

        self->impl->width  = width;
        self->impl->height = height;

        spdlog::info ("window resized to ({}, {}", width, height);

        self->events.resized (width, height);
    });

    glfwSetWindowPosCallback (impl->window, [] (GLFWwindow* window, int xpos, int ypos) {
        GLFWWindowBase* self = static_cast<GLFWWindowBase*> (glfwGetWindowUserPointer (window));

        self->events.moved (xpos, ypos);
    });

    glfwSetWindowFocusCallback (impl->window, [] (GLFWwindow* window, int focused) {
        GLFWWindowBase* self = static_cast<GLFWWindowBase*> (glfwGetWindowUserPointer (window));

        if (focused) {
            spdlog::trace ("window focus gained ");
            self->events.focused ();
        } else {
            spdlog::trace ("window focus lost ");
            self->events.focusLost ();
        }
    });

    glfwSetWindowRefreshCallback (impl->window, [] (GLFWwindow* window) {
        GLFWWindowBase* self = static_cast<GLFWWindowBase*> (glfwGetWindowUserPointer (window));
        self->events.refresh ();
    });

    glfwSetFramebufferSizeCallback (impl->window, [] (GLFWwindow* /* window */, int /* width */, int /* height */) {
        // GLFWWindowBase* self = static_cast<GLFWWindowBase*> (glfwGetWindowUserPointer (window));

        spdlog::trace ("framebuffer resized");
    });
}


GLFWWindowBase::~GLFWWindowBase ()
{
    Close ();
}


void* GLFWWindowBase::GetHandle () const
{
    return impl->window;
}


uint32_t GLFWWindowBase::GetWidth () const
{
    return static_cast<uint32_t> (impl->width);
}


uint32_t GLFWWindowBase::GetHeight () const
{
    return static_cast<uint32_t> (impl->height);
}


float GLFWWindowBase::GetAspectRatio () const
{
    return static_cast<float> (impl->width) / impl->height;
}


double GLFWWindowBase::GetRefreshRate () const
{
    return impl->refreshRate;
}


void GLFWWindowBase::PollEvents ()
{
    glfwPollEvents ();
}


void GLFWWindowBase::Close ()
{
    if (impl->window == nullptr)
        return;
    
    spdlog::trace ("Destroying window");

    glfwDestroyWindow (impl->window);

    glfwPollEvents ();
    
    impl->surface = VK_NULL_HANDLE;
    impl->window = nullptr;
}


void GLFWWindowBase::DoEventLoop (const DrawCallback& drawCallback)
{
    if (RG_ERROR (impl->window == nullptr)) {
        return;
    }

    bool stop = false;
    while (!glfwWindowShouldClose (impl->window)) {

        drawCallback (stop);

        if (stop) {
            glfwSetWindowShouldClose (impl->window, GLFW_TRUE);
            glfwPollEvents ();
            break;
        }
   
        glfwPollEvents ();
    }
}


std::vector<const char*> GetGLFWInstanceExtensions ()
{
    globalGLFWInitializer.EnsureInitialized ();

    uint32_t     glfwExtensionCount = 0;
    const char** glfwExtensions     = glfwGetRequiredInstanceExtensions (&glfwExtensionCount);
    RG_ASSERT (glfwExtensionCount != 0);

    std::vector<const char*> result;
    for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
        result.push_back (glfwExtensions[i]);
    }
    return result;
}


VkSurfaceKHR GLFWWindowBase::GetSurface (VkInstance instance)
{
    RG_ASSERT (impl->window != nullptr);

    if (impl->surface != VK_NULL_HANDLE) {
        return impl->surface;
    }

    VkResult result = glfwCreateWindowSurface (instance, impl->window, nullptr, &impl->surface);
    if (RG_ERROR (result != VK_SUCCESS)) {
        throw std::runtime_error ("Failed to create glfw surface");
    }

    return impl->surface;
}


void GLFWWindowBase::Hide ()
{
    glfwHideWindow (impl->window);
}


void GLFWWindowBase::Show ()
{
    glfwShowWindow (impl->window);
}


void GLFWWindowBase::Focus ()
{
    glfwFocusWindow (impl->window);
}


void GLFWWindowBase::SetTitle (const std::string& title)
{
    glfwSetWindowTitle (impl->window, title.c_str ());
}


void GLFWWindowBase::SetWindowMode (Mode mode)
{
    GLFWmonitor*       primaryMonitor     = glfwGetPrimaryMonitor ();
    const GLFWvidmode* primaryMonitorMode = glfwGetVideoMode (primaryMonitor);

    if (mode != Window::Mode::Windowed) {
        glfwGetWindowPos (impl->window, &impl->posXWindowed, &impl->posYWindowed);
    }

    if (mode == Window::Mode::Fullscreen) {
        glfwSetWindowMonitor (impl->window, primaryMonitor, 0, 0, primaryMonitorMode->width, primaryMonitorMode->height, GLFW_DONT_CARE);
    } else if (mode == Window::Mode::Windowed) {
        glfwSetWindowMonitor (impl->window, nullptr, 0, 0, 800, 600, GLFW_DONT_CARE);
        glfwSetWindowPos (impl->window, impl->posXWindowed, impl->posYWindowed);
    } else {
        RG_BREAK_STR ("unexpected window mode type");
    }

    impl->mode = mode;
}


Window::Mode GLFWWindowBase::GetWindowMode ()
{
    return impl->mode;
}


GLFWWindow::GLFWWindow ()
    : GLFWWindowBase ({}, false, false)
{
}


HiddenGLFWWindow::HiddenGLFWWindow ()
    : GLFWWindowBase ({
          { GLFW_VISIBLE, GLFW_FALSE },
      }, false, false)
{
}


HiddenGLFWWindow::HiddenGLFWWindow (size_t width, size_t height)
    : GLFWWindowBase (width, height, { { GLFW_VISIBLE, GLFW_FALSE } }, false, false)
{
}


FullscreenGLFWWindow::FullscreenGLFWWindow ()
    : GLFWWindowBase ({}, true, true)
{
}


} // namespace RG