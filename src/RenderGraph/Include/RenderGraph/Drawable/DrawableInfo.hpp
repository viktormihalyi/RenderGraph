#ifndef DRAWRECORDABLEINFO_HPP
#define DRAWRECORDABLEINFO_HPP

#include "RenderGraph/Drawable/Drawable.hpp"
#include "RenderGraph/RenderGraphExport.hpp"

#include "RenderGraph/VulkanWrapper/CommandBuffer.hpp"
#include "RenderGraph/VulkanWrapper/Utils/BufferTransferable.hpp"

#include <functional>
#include <memory>

#include <vulkan/vulkan.h>

namespace RG {

class VertexBufferList {
private:
    template<typename T>
    std::vector<T> Get (std::function<T (const std::shared_ptr<RG::VertexBufferTransferableUntyped>&)> getterFunc) const
    {
        std::vector<T> result;

        for (auto& vb : vertexBuffers) {
            result.push_back (getterFunc (vb));
        }

        return result;
    }

public:
    std::vector<std::shared_ptr<RG::VertexBufferTransferableUntyped>> vertexBuffers;

    VertexBufferList () = default;

    VertexBufferList (std::vector<std::shared_ptr<RG::VertexBufferTransferableUntyped>> vertexBuffers)
        : vertexBuffers (vertexBuffers)
    {
    }

    void Add (std::shared_ptr<RG::VertexBufferTransferableUntyped> vb)
    {
        vertexBuffers.push_back (vb);
    }

    std::vector<VkBuffer> GetHandles () const
    {
        return Get<VkBuffer> ([] (const std::shared_ptr<RG::VertexBufferTransferableUntyped>& vb) {
            return vb->buffer.GetBufferToBind ();
        });
    }
};


class RENDERGRAPH_DLL_EXPORT DrawableInfo : public Drawable {
public:
    const uint32_t instanceCount;

    const uint32_t              vertexCount;
    const std::vector<VkBuffer> vertexBuffer;

    const uint32_t indexCount;
    const VkBuffer indexBuffer;

    DrawableInfo (const uint32_t instanceCount,
                  uint32_t       vertexCount,
                  VkBuffer       vertexBuffer                                                       = VK_NULL_HANDLE,
                  const std::vector<VkVertexInputBindingDescription>& /* vertexInputBindings   */   = {},
                  const std::vector<VkVertexInputAttributeDescription>& /* vertexInputAttributes */ = {},
                  uint32_t indexCount                                                               = 0,
                  VkBuffer indexBuffer                                                              = VK_NULL_HANDLE)
        : instanceCount (instanceCount)
        , vertexCount (vertexCount)
        , vertexBuffer ((vertexBuffer == VK_NULL_HANDLE) ? std::vector<VkBuffer> {} : std::vector<VkBuffer> { vertexBuffer })
        , indexCount (indexCount)
        , indexBuffer (indexBuffer)
    {
    }

    DrawableInfo (const uint32_t                              instanceCount,
                  const RG::VertexBufferTransferableUntyped& vertexBuffer,
                  const RG::IndexBufferTransferable&         indexBuffer)
        : instanceCount (instanceCount)
        , vertexCount (static_cast<uint32_t> (vertexBuffer.data.size ()))
        , vertexBuffer ({ vertexBuffer.buffer.GetBufferToBind () })
        , indexCount (static_cast<uint32_t> (indexBuffer.data.size ()))
        , indexBuffer (indexBuffer.buffer.GetBufferToBind ())
    {
    }

    DrawableInfo (const uint32_t                              instanceCount,
                  const RG::VertexBufferTransferableUntyped& vertexBuffer)
        : instanceCount (instanceCount)
        , vertexCount (static_cast<uint32_t> (vertexBuffer.data.size ()))
        , vertexBuffer ({ vertexBuffer.buffer.GetBufferToBind () })
        , indexCount (0)
        , indexBuffer (VK_NULL_HANDLE)
    {
    }

    DrawableInfo (const uint32_t   instanceCount,
                  uint32_t         vertexCount,
                  VertexBufferList vertexBuffers,
                  uint32_t         indexCount,
                  VkBuffer         indexBuffer)
        : instanceCount (instanceCount)
        , vertexCount (vertexCount)
        , vertexBuffer (vertexBuffers.GetHandles ())
        , indexCount (indexCount)
        , indexBuffer (indexBuffer)
    {
    }

    virtual ~DrawableInfo () override = default;

    void Record (RG::CommandBuffer& commandBuffer) const override;
};

class DrawableInfoProvider : public Drawable {
public:
    virtual void Record (RG::CommandBuffer& commandBuffer) const override { GetDrawRecordableInfo ().Record (commandBuffer); }

private:
    virtual const DrawableInfo& GetDrawRecordableInfo () const = 0;
};

} // namespace RG

#endif