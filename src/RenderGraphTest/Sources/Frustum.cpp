#include "Frustum.hpp"

#pragma warning(push, 0)
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#pragma warning(pop)

namespace GVK {

PerspectiveFrustum::PerspectiveFrustum (float backPlane, float frontPlane, float fov, float aspectRatio)
    : backPlane (backPlane)
    , frontPlane (frontPlane)
    , fov (fov)
    , aspectRatio (aspectRatio)
{
}


glm::mat4 PerspectiveFrustum::GetMatrix () const
{
    return glm::perspective (glm::radians (fov), aspectRatio, frontPlane, backPlane);
}


OrthographicFrustum::OrthographicFrustum (float left, float right, float bottom, float top)
    : left (left)
    , right (right)
    , bottom (bottom)
    , top (top)
{
}


glm::mat4 OrthographicFrustum::GetMatrix () const
{
    return glm::ortho (left, right, bottom, top);
}

} // namespace GVK
