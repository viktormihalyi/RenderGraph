#ifndef FRUSTUM_HPP
#define FRUSTUM_HPP

#include <memory>

#pragma warning(push, 0)
#include <glm/glm.hpp>
#pragma warning(pop)

namespace RG {

class Frustum {
public:
    virtual ~Frustum () = default;

    virtual glm::mat4 GetMatrix () const = 0;
};


class PerspectiveFrustum : public Frustum {
private:
    float backPlane;
    float frontPlane;
    float fov;
    float aspectRatio;

public:
    PerspectiveFrustum (float backPlane, float frontPlane, float fov, float aspectRatio);

    void SetAspectRatio (float value) { aspectRatio = value; }
    void SetBackPlane (float value) { backPlane = value; }
    void SetFrontPlane (float value) { frontPlane = value; }

    float GetBackPlane () const { return backPlane; }
    float GetFrontPlane () const { return frontPlane; }

    glm::mat4 GetMatrix () const override;
};


class OrthographicFrustum : public Frustum {
private:
    float left;
    float right;
    float bottom;
    float top;

public:
    OrthographicFrustum (float left, float right, float bottom, float top);

    glm::mat4 GetMatrix () const override;
};

} // namespace RG

#endif