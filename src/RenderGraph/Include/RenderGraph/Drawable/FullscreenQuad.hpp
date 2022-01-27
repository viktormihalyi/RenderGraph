#ifndef DR_FULLSCREENQUAD_HPP
#define DR_FULLSCREENQUAD_HPP

#include "RenderGraph/VulkanWrapper/DeviceExtra.hpp"
#include "RenderGraph/Drawable/DrawableInfo.hpp"

#include <glm/glm.hpp>

namespace RG {

class FullscreenQuad : public DrawableInfoProvider {
private:
    struct Vertex {
        glm::vec2 pos;
        glm::vec2 uv;
    };

    RG::VertexBufferTransferable<Vertex> vertexBuffer;
    RG::IndexBufferTransferable          indexBuffer;

    std::unique_ptr<DrawableInfo> info;

public:
    FullscreenQuad (const RG::DeviceExtra& device)
        : vertexBuffer (device, 4, { VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32_SFLOAT }, VK_VERTEX_INPUT_RATE_VERTEX)
        , indexBuffer (device, 6)
    {
        vertexBuffer = std::vector<Vertex> {
            { glm::vec2 (-1.f, -1.f), glm::vec2 (0.f, 0.f) },
            { glm::vec2 (-1.f, +1.f), glm::vec2 (0.f, 1.f) },
            { glm::vec2 (+1.f, +1.f), glm::vec2 (1.f, 1.f) },
            { glm::vec2 (+1.f, -1.f), glm::vec2 (1.f, 0.f) },
        };

        indexBuffer = { 0, 1, 2, 0, 3, 2 };

        vertexBuffer.Flush ();
        indexBuffer.Flush ();

        info = std::make_unique<DrawableInfo> (1, vertexBuffer, indexBuffer);
    }

private:
    virtual const DrawableInfo& GetDrawRecordableInfo () const override
    {
        return *info;
    }
};

} // namespace RG

#endif