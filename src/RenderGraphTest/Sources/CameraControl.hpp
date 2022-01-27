#ifndef CAMERACONTROL_HPP
#define CAMERACONTROL_HPP

#include "Camera.hpp"
#include "RenderGraph/Utils/Event.hpp"
#include "InputState.hpp"
#include "RenderGraph/Window/Window.hpp"

namespace RG {

class CameraControl : public EventObserver {
private:
    Camera&                        camera;
    MouseState                     mouse;
    std::unique_ptr<KeyboardState> keyboard;

public:
    CameraControl (Camera& camera, RG::Window::Events& windowEvents)
        : camera (camera)
        , keyboard (std::make_unique<KeyboardState> ())
    {
        using namespace std::placeholders;

        Observe (windowEvents.mouseMove, std::bind (&CameraControl::ProcessMouseInput, this, _1, _2));
        Observe (windowEvents.keyPressed, std::bind (&KeyboardState::SetPressed, keyboard.get (), _1));
        Observe (windowEvents.keyReleased, std::bind (&KeyboardState::SetReleased, keyboard.get (), _1));
        Observe (windowEvents.leftMouseButtonPressed, [&] (auto...) { mouse.leftButton = true; });
        Observe (windowEvents.leftMouseButtonReleased, [&] (auto...) { mouse.leftButton = false; });

        Observe (windowEvents.resized, [&] (uint32_t width, uint32_t height) {
            const float aspect = static_cast<float> (width) / height;
            camera.SetAspectRatio (aspect);
        });
    }

    void UpdatePosition (float dt)
    {
        if (keyboard->IsPressed ('W')) {
            camera.Move (Camera::MovementDirection::Forward, dt);
        }
        if (keyboard->IsPressed ('A')) {
            camera.Move (Camera::MovementDirection::Left, dt);
        }
        if (keyboard->IsPressed ('S')) {
            camera.Move (Camera::MovementDirection::Backward, dt);
        }
        if (keyboard->IsPressed ('D')) {
            camera.Move (Camera::MovementDirection::Right, dt);
        }
        if (keyboard->IsPressed ('E')) {
            camera.Move (Camera::MovementDirection::Up, dt);
        }
        if (keyboard->IsPressed ('Q')) {
            camera.Move (Camera::MovementDirection::Down, dt);
        }
    }

private:
    void ProcessMouseInput (int32_t x, int32_t y)
    {
        static glm::vec2 lastPos (x, y);
        glm::vec2        currentPos (x, y);
        if (mouse.leftButton) {
            camera.ProcessMouseInput (lastPos - currentPos);
        }
        lastPos = currentPos;
    }
};

} // namespace RG

#endif