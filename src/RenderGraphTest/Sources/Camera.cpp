#include "Camera.hpp"

#include "RenderGraph/Utils/Assert.hpp"

#include <algorithm>
#include <sstream>

#pragma warning(push, 0)
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#pragma warning(pop)

namespace GVK {

class GlobalUpVectorProvider {
public:
    virtual ~GlobalUpVectorProvider () = default;

    virtual void             ProcessMouseInput (Camera& camera, const glm::vec2& delta) const = 0;
    virtual glm::vec3        GetAheadVector (float pitch, float yaw) const                    = 0;
    virtual float            GetYaw (const glm::vec3& ahead) const                            = 0;
    virtual float            GetPitch (const glm::vec3& ahead) const                          = 0;
    virtual const glm::vec3& GetUpVector () const                                             = 0;
};


class GlobalZ final : public GlobalUpVectorProvider {
private:
    const glm::vec3 upVector;

public:
    GlobalZ ()
        : upVector (0, 0, 1)
    {
    }

    virtual void ProcessMouseInput (Camera& camera, const glm::vec2& delta) const override
    {
        camera.yaw -= delta.x * camera.sensitivity;
        camera.pitch += delta.y * camera.sensitivity;
        camera.pitch = std::clamp (camera.pitch, -89.f, 89.f);
    }

    virtual glm::vec3 GetAheadVector (float pitch, float yaw) const override
    {
        using std::cos, std::sin;

        glm::vec3 ahead;
        ahead.x = cos (glm::radians (pitch)) * cos (glm::radians (yaw));
        ahead.y = cos (glm::radians (pitch)) * sin (glm::radians (yaw));
        ahead.z = sin (glm::radians (pitch));
        return glm::normalize (ahead);
    }

    virtual float GetYaw (const glm::vec3& ahead) const override
    {
        if (ahead.y == 0) {
            return 0;
        }
        return ahead.x / -ahead.y;
    }

    virtual float GetPitch (const glm::vec3& ahead) const override
    {
        return std::clamp (std::tanh (glm::length (glm::vec2 (ahead.x, ahead.y)) / ahead.z), -89.f, 89.f);
    }

    virtual const glm::vec3& GetUpVector () const override
    {
        return upVector;
    }
};


const GlobalZ upVector;


Camera::Camera (const glm::vec3& position,
                const glm::vec3& ahead,
                float            aspect)
    : position (position)
    , ahead (ahead)
    , frustum (new PerspectiveFrustum (100.f, 0.001f, 80.f, aspect))
    , speed (1.f)
    , yaw (upVector.GetYaw (ahead))
    , pitch (upVector.GetPitch (ahead))
    , sensitivity (0.2f)
    , viewMatrix ([&] () { return glm::lookAt (this->position, this->position + this->ahead, up); })
    , projectionMatrix ([&] () { return frustum->GetMatrix (); })
    , viewProjectionMatrix ([&] () { return projectionMatrix.Get () * viewMatrix.Get (); })
    , rayDirMatrix ([&] () { return glm::inverse (viewProjectionMatrix.Get () * glm::translate (glm::mat4 (1.f), this->position)); })
{
    UpdateVectors ();

    //positionChanged += [&] (glm::vec3) {
    //    std::cout << "position: " << this->position << std::endl;
    //    std::cout << "ahead:    " << this->ahead << std::endl;
    //};
}


void Camera::InvalidateMatrices ()
{
    viewMatrix.Invalidate ();
    projectionMatrix.Invalidate ();
    rayDirMatrix.Invalidate ();
    viewProjectionMatrix.Invalidate ();
}


void Camera::UpdateVectors ()
{
    ahead = upVector.GetAheadVector (pitch, yaw);


    right = glm::normalize (glm::cross (upVector.GetUpVector (), ahead));
    up    = glm::normalize (glm::cross (right, ahead));

    InvalidateMatrices ();
}


void Camera::Move (Camera::MovementDirection dir, float dt)
{
    const float speedDt = speed * dt;

    switch (dir) {
        case MovementDirection::Forward:
            position += ahead * speedDt;
            break;
        case MovementDirection::Backward:
            position -= ahead * speedDt;
            break;
        case MovementDirection::Left:
            position -= right * speedDt;
            break;
        case MovementDirection::Right:
            position += right * speedDt;
            break;
        case MovementDirection::Up:
            position += upVector.GetUpVector () * speedDt;
            break;
        case MovementDirection::Down:
            position -= upVector.GetUpVector () * speedDt;
            break;

        default:
            GVK_BREAK ();
            break;
    }

    InvalidateMatrices ();

    positionChanged (position);
}


void Camera::ProcessMouseInput (const glm::vec2& delta)
{
    upVector.ProcessMouseInput (*this, delta);

    UpdateVectors ();
}


void Camera::SetAspectRatio (float value)
{
    PerspectiveFrustum* f = dynamic_cast<PerspectiveFrustum*> (frustum.get ());
    if (GVK_ERROR (f == nullptr)) {
        return;
    }

    f->SetAspectRatio (value);

    InvalidateMatrices ();
}


float Camera::GetBackPlane () const
{
    PerspectiveFrustum* f = dynamic_cast<PerspectiveFrustum*> (frustum.get ());
    if (GVK_ERROR (f == nullptr)) {
        return 0.f;
    }

    return f->GetBackPlane ();
}


float Camera::GetFrontPlane () const
{
    PerspectiveFrustum* f = dynamic_cast<PerspectiveFrustum*> (frustum.get ());
    if (GVK_ERROR (f == nullptr)) {
        return 0.f;
    }

    return f->GetFrontPlane ();
}


void Camera::SetFrontAndBackPlane (float front, float back)
{
    PerspectiveFrustum* f = dynamic_cast<PerspectiveFrustum*> (frustum.get ());
    if (GVK_ERROR (f == nullptr)) {
        return;
    }

    f->SetFrontPlane (front);
    f->SetBackPlane (back);

    InvalidateMatrices ();
}

} // namespace GVK