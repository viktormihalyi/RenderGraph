#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include "RenderGraph/Utils/Event.hpp"
#include "Frustum.hpp"
#include "RenderGraph/Utils/Lazy.hpp"

#pragma warning(push, 0)
#include <glm/glm.hpp>
#pragma warning(pop)

namespace RG {

class Camera {
public:
    glm::vec3 position;
    glm::vec3 ahead;
    glm::vec3 up;
    glm::vec3 right;

    std::unique_ptr<Frustum> frustum;

    float speed;
    float yaw;
    float pitch;
    float sensitivity;

    Lazy<glm::mat4> viewMatrix;
    Lazy<glm::mat4> projectionMatrix;
    Lazy<glm::mat4> viewProjectionMatrix;
    Lazy<glm::mat4> rayDirMatrix;

    void InvalidateMatrices ();

    Event<glm::vec3> positionChanged;

public:
    enum class MovementDirection {
        Forward,
        Backward,
        Left,
        Right,
        Up,
        Down
    };

    Camera (const glm::vec3& position, const glm::vec3& ahead, float aspect);

    const glm::vec3& GetPosition () const { return position; }
    const glm::vec3& GetViewDirection () const { return ahead; }
    const glm::mat4& GetRayDirMatrix () { return rayDirMatrix; }

    void SetAspectRatio (float value);


    const glm::mat4& GetViewMatrix () { return viewMatrix; }
    const glm::mat4& GetProjectionMatrix () { return projectionMatrix; }
    const glm::mat4& GetViewProjectionMatrix () { return viewProjectionMatrix; }

    void Move (MovementDirection, float dt);
    void UpdateVectors ();
    void ProcessMouseInput (const glm::vec2& delta);

    float GetSpeed () const { return speed; }
    void  SetSpeed (float value) { speed = value; }

    float GetBackPlane () const;
    float GetFrontPlane () const;
    void  SetFrontAndBackPlane (float back, float front);
};

} // namespace RG

#endif
