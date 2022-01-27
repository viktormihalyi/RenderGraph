#ifndef VULKANWRAPPER_COMMANDS_HPP
#define VULKANWRAPPER_COMMANDS_HPP

#include "RenderGraph/RenderGraphExport.hpp"
#include "RenderGraph/VulkanWrapper/CommandBuffer.hpp"
#include "RenderGraph/VulkanWrapper/Image.hpp"

#include <vector>
#include <functional>
#include <vulkan/vulkan.h>

namespace GVK {

class RENDERGRAPH_DLL_EXPORT CommandBindVertexBuffers : public Command {
private:
    const uint32_t                  firstBinding;
    const uint32_t                  bindingCount;
    const std::vector<VkBuffer>     pBuffers;
    const std::vector<VkDeviceSize> pOffsets;

public:
    CommandBindVertexBuffers (const uint32_t                  firstBinding,
                              const uint32_t                  bindingCount,
                              const std::vector<VkBuffer>     pBuffers,
                              const std::vector<VkDeviceSize> pOffsets)
        : firstBinding (firstBinding)
        , bindingCount (bindingCount)
        , pBuffers (pBuffers)
        , pOffsets (pOffsets)
    {
    }

    virtual void Record (CommandBuffer&) override;

    virtual bool IsEquivalent (const Command& other) override
    {
        if (auto otherCommand = dynamic_cast<const CommandBindVertexBuffers*> (&other)) {
            // ignore VkBuffer
            return firstBinding == otherCommand->firstBinding &&
                   bindingCount == otherCommand->bindingCount &&
                   pOffsets == otherCommand->pOffsets;
        }

        return false;
    }

    virtual std::string ToString () const override;
};


class RENDERGRAPH_DLL_EXPORT CommandPipelineBarrier : public Command {
private:
    VkPipelineStageFlags               srcStageMask;
    VkPipelineStageFlags               dstStageMask;
    std::vector<VkMemoryBarrier>       memoryBarriers;
    std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers;
    std::vector<VkImageMemoryBarrier>  imageMemoryBarriers;

public:
    CommandPipelineBarrier (const VkPipelineStageFlags               srcStageMask,
                            const VkPipelineStageFlags               dstStageMask,
                            const std::vector<VkMemoryBarrier>       memoryBarriers       = {},
                            const std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers = {},
                            const std::vector<VkImageMemoryBarrier>  imageMemoryBarriers  = {})
        : srcStageMask (srcStageMask)
        , dstStageMask (dstStageMask)
        , memoryBarriers (memoryBarriers)
        , bufferMemoryBarriers (bufferMemoryBarriers)
        , imageMemoryBarriers (imageMemoryBarriers)
    {
    }

#pragma warning(disable : 26815)

    CommandPipelineBarrier& AddMemoryBarrier (const std::vector<VkMemoryBarrier>& barriers)
    {
        memoryBarriers.insert (memoryBarriers.end (), barriers.begin (), barriers.end ());
        return *this;
    }

    CommandPipelineBarrier& AddMemoryBarrier (const VkMemoryBarrier& barrier)
    {
        memoryBarriers.push_back (barrier);
        return *this;
    }

    CommandPipelineBarrier& AddImageMemoryBarrier (const std::vector<VkImageMemoryBarrier>& barriers)
    {
        imageMemoryBarriers.insert (imageMemoryBarriers.end (), barriers.begin (), barriers.end ());
        return *this;
    }

    CommandPipelineBarrier& AddImageMemoryBarrier (const VkImageMemoryBarrier& barrier)
    {
        imageMemoryBarriers.push_back (barrier);
        return *this;
    }

#pragma warning(default : 26815)

    virtual void Record (CommandBuffer&) override;


    virtual bool IsEquivalent (const Command& other) override
    {
        if (auto otherCommand = dynamic_cast<const CommandPipelineBarrier*> (&other)) {
            if (memoryBarriers.size () != otherCommand->memoryBarriers.size ())
                return false;
            if (bufferMemoryBarriers.size () != otherCommand->bufferMemoryBarriers.size ())
                return false;
            if (imageMemoryBarriers.size () != otherCommand->imageMemoryBarriers.size ())
                return false;

            for (size_t i = 0; i < memoryBarriers.size (); ++i)
                if (memoryBarriers[i].srcAccessMask != otherCommand->memoryBarriers[i].srcAccessMask ||
                    memoryBarriers[i].dstAccessMask != otherCommand->memoryBarriers[i].dstAccessMask)
                    return false;

            // ignore VkBuffer
            for (size_t i = 0; i < bufferMemoryBarriers.size (); ++i)
                if (bufferMemoryBarriers[i].srcAccessMask != otherCommand->bufferMemoryBarriers[i].srcAccessMask ||
                    bufferMemoryBarriers[i].dstAccessMask != otherCommand->bufferMemoryBarriers[i].dstAccessMask ||
                    bufferMemoryBarriers[i].srcQueueFamilyIndex != otherCommand->bufferMemoryBarriers[i].srcQueueFamilyIndex ||
                    bufferMemoryBarriers[i].dstQueueFamilyIndex != otherCommand->bufferMemoryBarriers[i].dstQueueFamilyIndex ||
                    bufferMemoryBarriers[i].offset != otherCommand->bufferMemoryBarriers[i].offset ||
                    bufferMemoryBarriers[i].buffer != otherCommand->bufferMemoryBarriers[i].buffer ||
                    bufferMemoryBarriers[i].size != otherCommand->bufferMemoryBarriers[i].size)
                    return false;

            // ignore VkImage
            for (size_t i = 0; i < imageMemoryBarriers.size (); ++i)
                if (imageMemoryBarriers[i].srcAccessMask != otherCommand->imageMemoryBarriers[i].srcAccessMask ||
                    imageMemoryBarriers[i].dstAccessMask != otherCommand->imageMemoryBarriers[i].dstAccessMask ||
                    imageMemoryBarriers[i].image != otherCommand->imageMemoryBarriers[i].image ||
                    imageMemoryBarriers[i].oldLayout != otherCommand->imageMemoryBarriers[i].oldLayout ||
                    imageMemoryBarriers[i].newLayout != otherCommand->imageMemoryBarriers[i].newLayout ||
                    imageMemoryBarriers[i].srcQueueFamilyIndex != otherCommand->imageMemoryBarriers[i].srcQueueFamilyIndex ||
                    imageMemoryBarriers[i].dstQueueFamilyIndex != otherCommand->imageMemoryBarriers[i].dstQueueFamilyIndex ||
                    imageMemoryBarriers[i].subresourceRange.aspectMask != otherCommand->imageMemoryBarriers[i].subresourceRange.aspectMask ||
                    imageMemoryBarriers[i].subresourceRange.baseMipLevel != otherCommand->imageMemoryBarriers[i].subresourceRange.baseMipLevel ||
                    imageMemoryBarriers[i].subresourceRange.levelCount != otherCommand->imageMemoryBarriers[i].subresourceRange.levelCount ||
                    imageMemoryBarriers[i].subresourceRange.baseArrayLayer != otherCommand->imageMemoryBarriers[i].subresourceRange.baseArrayLayer ||
                    imageMemoryBarriers[i].subresourceRange.layerCount != otherCommand->imageMemoryBarriers[i].subresourceRange.layerCount)
                    return false;

            return srcStageMask == otherCommand->srcStageMask &&
                   dstStageMask == otherCommand->dstStageMask;
        }

        return false;
    }
};


class RENDERGRAPH_DLL_EXPORT CommandPipelineBarrierFull : public CommandPipelineBarrier {
private:
    static const VkAccessFlags flushAll;

public:
    CommandPipelineBarrierFull ()
        : CommandPipelineBarrier (VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                  std::vector<VkMemoryBarrier> { flushAll },
                                  std::vector<VkBufferMemoryBarrier> {},
                                  std::vector<VkImageMemoryBarrier> {})
    {
    }
};


class RENDERGRAPH_DLL_EXPORT CommandTranstionImage : public Command {
private:
    VkImageMemoryBarrier imageMemoryBarrier;

public:
    CommandTranstionImage (const Image&  image,
                           VkImageLayout oldLayout,
                           VkImageLayout newLayout)
        : imageMemoryBarrier (image.GetBarrier (oldLayout, newLayout, VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT))
    {
    }

    virtual void Record (CommandBuffer& commandBuffer) override
    {
        commandBuffer.Record<CommandPipelineBarrier> (VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                                      std::vector<VkMemoryBarrier> {},
                                                      std::vector<VkBufferMemoryBarrier> {},
                                                      std::vector<VkImageMemoryBarrier> { imageMemoryBarrier });
    }


    virtual bool IsEquivalent (const Command& other) override
    {
        if (auto otherCommand = dynamic_cast<const CommandTranstionImage*> (&other)) {
            return true;
        }

        return false;
    }
};

class RENDERGRAPH_DLL_EXPORT CommandGeneric : public Command {
private:
    std::function<void (VkCommandBuffer)> recordCallback;

public:
    CommandGeneric (const std::function<void (VkCommandBuffer)>& recordCallback)
        : recordCallback (recordCallback)
    {
    }

    virtual void Record (CommandBuffer& commandBuffer) override
    {
        recordCallback (commandBuffer.GetHandle ());
    }

    virtual bool IsEquivalent (const Command& other) override
    {
        if (auto otherCommand = dynamic_cast<const CommandGeneric*> (&other)) {
            return true;
        }

        return false;
    }
};


class RENDERGRAPH_DLL_EXPORT CommandDrawIndexed : public Command {
private:
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t  vertexOffset;
    uint32_t firstInstance;

public:
    CommandDrawIndexed (uint32_t indexCount,
                        uint32_t instanceCount,
                        uint32_t firstIndex,
                        int32_t  vertexOffset,
                        uint32_t firstInstance)
        : indexCount (indexCount)
        , instanceCount (instanceCount)
        , firstIndex (firstIndex)
        , vertexOffset (vertexOffset)
        , firstInstance (firstInstance)
    {
    }

    virtual void Record (CommandBuffer& commandBuffer) override
    {
        vkCmdDrawIndexed (commandBuffer.GetHandle (), indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    virtual bool IsEquivalent (const Command& other) override
    {
        if (auto otherCommand = dynamic_cast<const CommandDrawIndexed*> (&other)) {
            return indexCount == otherCommand->indexCount && instanceCount == otherCommand->instanceCount && firstIndex == otherCommand->firstIndex && vertexOffset == otherCommand->vertexOffset && firstInstance == otherCommand->firstInstance;
        }

        return false;
    }
};


class RENDERGRAPH_DLL_EXPORT CommandDraw : public Command {
private:
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;

public:
    CommandDraw (uint32_t vertexCount,
                 uint32_t instanceCount,
                 uint32_t firstVertex,
                 uint32_t firstInstance)
        : vertexCount (vertexCount)
        , instanceCount (instanceCount)
        , firstVertex (firstVertex)
        , firstInstance (firstInstance)
    {
    }

    virtual void Record (CommandBuffer& commandBuffer) override
    {
        vkCmdDraw (commandBuffer.GetHandle (), vertexCount, instanceCount, firstVertex, firstInstance);
    }

    virtual bool IsEquivalent (const Command& other) override
    {
        if (auto otherCommand = dynamic_cast<const CommandDraw*> (&other)) {
            return vertexCount == otherCommand->vertexCount && instanceCount == otherCommand->instanceCount && firstVertex == otherCommand->firstVertex && firstInstance == otherCommand->firstInstance;
        }

        return false;
    }
};


class RENDERGRAPH_DLL_EXPORT CommandBindIndexBuffer : public Command {
private:
    VkBuffer     buffer;
    VkDeviceSize offset;
    VkIndexType  indexType;

public:
    CommandBindIndexBuffer (VkBuffer     buffer,
                            VkDeviceSize offset,
                            VkIndexType  indexType)
        : buffer (buffer)
        , offset (offset)
        , indexType (indexType)
    {
    }

    virtual void Record (CommandBuffer& commandBuffer) override
    {
        vkCmdBindIndexBuffer (commandBuffer.GetHandle (), buffer, offset, indexType);
    }

    virtual bool IsEquivalent (const Command& other) override
    {
        if (auto otherCommand = dynamic_cast<const CommandBindIndexBuffer*> (&other)) {
            // ignore VkBuffer
            return offset == otherCommand->offset && indexType == otherCommand->indexType;
        }

        return false;
    }
};


class RENDERGRAPH_DLL_EXPORT CommandEndRenderPass : public Command {
public:
    CommandEndRenderPass () = default;

    virtual void Record (CommandBuffer& commandBuffer) override
    {
        vkCmdEndRenderPass (commandBuffer.GetHandle ());
    }

    virtual bool IsEquivalent (const Command& other) override
    {
        if (auto otherCommand = dynamic_cast<const CommandEndRenderPass*> (&other)) {
            return true;
        }

        return false;
    }
};


class RENDERGRAPH_DLL_EXPORT CommandBeginRenderPass : public Command {
private:
    VkRenderPassBeginInfo     renderPassBegin;
    VkSubpassContents         contents;
    std::vector<VkClearValue> clearValues;

public:
    CommandBeginRenderPass (VkRenderPass                     renderPass,
                            VkFramebuffer                    framebuffer,
                            VkRect2D                         renderArea,
                            const std::vector<VkClearValue>& clearValues_,
                            VkSubpassContents                contents = VK_SUBPASS_CONTENTS_INLINE)
        : renderPassBegin ({})
        , contents (contents)
        , clearValues (clearValues_)
    {
        renderPassBegin.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBegin.pNext           = nullptr;
        renderPassBegin.renderPass      = renderPass;
        renderPassBegin.framebuffer     = framebuffer;
        renderPassBegin.renderArea      = renderArea;
        renderPassBegin.clearValueCount = static_cast<uint32_t> (clearValues.size ());
        renderPassBegin.pClearValues    = clearValues.data ();
    }

    virtual void Record (CommandBuffer& commandBuffer) override
    {
        vkCmdBeginRenderPass (commandBuffer.GetHandle (), &renderPassBegin, contents);
    }

    virtual bool IsEquivalent (const Command& other) override
    {
        if (auto otherCommand = dynamic_cast<const CommandBeginRenderPass*> (&other)) {
            // ignore VkRenderPass, VkFrameBuffer, VkClearValue // TODO
            return renderPassBegin.renderArea.extent.width == otherCommand->renderPassBegin.renderArea.extent.width && renderPassBegin.renderArea.extent.height == otherCommand->renderPassBegin.renderArea.extent.height && renderPassBegin.renderPass == otherCommand->renderPassBegin.renderPass && renderPassBegin.framebuffer == otherCommand->renderPassBegin.framebuffer && renderPassBegin.renderArea.offset.x == otherCommand->renderPassBegin.renderArea.offset.x && renderPassBegin.renderArea.offset.y == otherCommand->renderPassBegin.renderArea.offset.y;
        }

        return false;
    }
};


class RENDERGRAPH_DLL_EXPORT CommandBindPipeline : public Command {
private:
    VkPipelineBindPoint pipelineBindPoint;
    VkPipeline          pipeline;

public:
    CommandBindPipeline (VkPipelineBindPoint pipelineBindPoint,
                         VkPipeline          pipeline)
        : pipelineBindPoint (pipelineBindPoint)
        , pipeline (pipeline)
    {
    }

    virtual void Record (CommandBuffer& commandBuffer) override
    {
        vkCmdBindPipeline (commandBuffer.GetHandle (), pipelineBindPoint, pipeline);
    }

    virtual bool IsEquivalent (const Command& other) override
    {
        if (auto otherCommand = dynamic_cast<const CommandBindPipeline*> (&other)) {
            // ignore VkPipeline
            return pipelineBindPoint == otherCommand->pipelineBindPoint && pipeline == otherCommand->pipeline;
        }

        return false;
    }
};


class RENDERGRAPH_DLL_EXPORT CommandBindDescriptorSets : public Command {
private:
    VkPipelineBindPoint          pipelineBindPoint;
    VkPipelineLayout             layout;
    uint32_t                     firstSet;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<uint32_t>        dynamicOffsets;

public:
    CommandBindDescriptorSets (VkPipelineBindPoint                 pipelineBindPoint,
                               VkPipelineLayout                    layout,
                               uint32_t                            firstSet,
                               const std::vector<VkDescriptorSet>& descriptorSets,
                               const std::vector<uint32_t>&        dynamicOffsets)
        : pipelineBindPoint (pipelineBindPoint)
        , layout (layout)
        , firstSet (firstSet)
        , descriptorSets (descriptorSets)
        , dynamicOffsets (dynamicOffsets)
    {
    }

    virtual void Record (CommandBuffer& commandBuffer) override
    {
        vkCmdBindDescriptorSets (commandBuffer.GetHandle (), pipelineBindPoint, layout, firstSet,
                                 static_cast<uint32_t> (descriptorSets.size ()), descriptorSets.data (),
                                 static_cast<uint32_t> (dynamicOffsets.size ()), dynamicOffsets.data ());
    }

    virtual bool IsEquivalent (const Command& other) override
    {
        if (auto otherCommand = dynamic_cast<const CommandBindDescriptorSets*> (&other)) {
            // ignore VkPipelineLayout, VkDescriptorSet
            return pipelineBindPoint == otherCommand->pipelineBindPoint &&
                   firstSet == otherCommand->firstSet &&
                   dynamicOffsets == otherCommand->dynamicOffsets;
        }

        return false;
    }
};

class RENDERGRAPH_DLL_EXPORT CommandCopyImage : public Command {
private:
    VkImage                  srcImage;
    VkImageLayout            srcImageLayout;
    VkImage                  dstImage;
    VkImageLayout            dstImageLayout;
    std::vector<VkImageCopy> regions;

public:
    CommandCopyImage (VkImage                         srcImage,
                      VkImageLayout                   srcImageLayout,
                      VkImage                         dstImage,
                      VkImageLayout                   dstImageLayout,
                      const std::vector<VkImageCopy>& regions)
        : srcImage (srcImage)
        , srcImageLayout (srcImageLayout)
        , dstImage (dstImage)
        , dstImageLayout (dstImageLayout)
        , regions (regions)
    {
    }

    virtual void Record (CommandBuffer& commandBuffer) override
    {
        vkCmdCopyImage (commandBuffer.GetHandle (), srcImage, srcImageLayout, dstImage, dstImageLayout, static_cast<uint32_t> (regions.size ()), regions.data ());
    }

    virtual bool IsEquivalent (const Command& other) override
    {
        if (auto otherCommand = dynamic_cast<const CommandCopyImage*> (&other)) {
            // ignore VkImage, VkImageCopy // TODO
            return srcImageLayout == otherCommand->srcImageLayout &&
                   dstImageLayout == otherCommand->dstImageLayout;
        }

        return false;
    }
};

class RENDERGRAPH_DLL_EXPORT CommandCopyImageToBuffer : public Command {
private:
    VkImage                        srcImage;
    VkImageLayout                  srcImageLayout;
    VkBuffer                       dstBuffer;
    std::vector<VkBufferImageCopy> regions;

public:
    CommandCopyImageToBuffer (VkImage                               srcImage,
                              VkImageLayout                         srcImageLayout,
                              VkBuffer                              dstBuffer,
                              const std::vector<VkBufferImageCopy>& regions)
        : srcImage (srcImage)
        , srcImageLayout (srcImageLayout)
        , dstBuffer (dstBuffer)
        , regions (regions)
    {
    }


    virtual void Record (CommandBuffer& commandBuffer) override
    {
        vkCmdCopyImageToBuffer (commandBuffer.GetHandle (),
                                srcImage,
                                srcImageLayout,
                                dstBuffer,
                                static_cast<uint32_t> (regions.size ()), regions.data ());
    }

    virtual bool IsEquivalent (const Command& other) override
    {
        if (auto otherCommand = dynamic_cast<const CommandCopyImageToBuffer*> (&other)) {
            // ignore VkImage, VkBuffer, VkBufferImageCopy
            return srcImageLayout == otherCommand->srcImageLayout;
        }

        return false;
    }
};


class RENDERGRAPH_DLL_EXPORT CommandCopyBufferToImage : public Command {
private:
    VkBuffer                       srcBuffer;
    VkImage                        dstImage;
    VkImageLayout                  dstImageLayout;
    std::vector<VkBufferImageCopy> regions;

public:
    CommandCopyBufferToImage (VkBuffer                       srcBuffer,
                              VkImage                        dstImage,
                              VkImageLayout                  dstImageLayout,
                              std::vector<VkBufferImageCopy> regions)
        : srcBuffer (srcBuffer)
        , dstImage (dstImage)
        , dstImageLayout (dstImageLayout)
        , regions (regions)
    {
        GVK_ASSERT (dstImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL || dstImageLayout == VK_IMAGE_LAYOUT_GENERAL);
    }

    virtual void Record (CommandBuffer& commandBuffer) override
    {
        vkCmdCopyBufferToImage (commandBuffer.GetHandle (), srcBuffer, dstImage, dstImageLayout, static_cast<uint32_t> (regions.size ()), regions.data ());
    }

    virtual bool IsEquivalent (const Command& other) override
    {
        if (auto otherCommand = dynamic_cast<const CommandCopyBufferToImage*> (&other)) {
            // ignore VkImage, VkBuffer, VkBufferImageCopy // TODO
            return dstImageLayout == otherCommand->dstImageLayout;
        }

        return false;
    }
};


class RENDERGRAPH_DLL_EXPORT CommandCopyBuffer : public Command {
private:
    VkBuffer                  srcBuffer;
    VkBuffer                  dstBuffer;
    std::vector<VkBufferCopy> regions;

public:
    CommandCopyBuffer (VkBuffer                         srcBuffer,
                       VkBuffer                         dstBuffer,
                       const std::vector<VkBufferCopy>& regions)
        : srcBuffer (srcBuffer)
        , dstBuffer (dstBuffer)
        , regions (regions)
    {
    }

    virtual void Record (CommandBuffer& commandBuffer) override
    {
        vkCmdCopyBuffer (commandBuffer.GetHandle (), srcBuffer, dstBuffer, static_cast<uint32_t> (regions.size ()), regions.data ());
    }

    virtual bool IsEquivalent (const Command& other) override
    {
        if (auto otherCommand = dynamic_cast<const CommandCopyBuffer*> (&other)) {
            // ignore VkBuffer, VkBufferCopy // TODO
            return true;
        }

        return false;
    }
};


class RENDERGRAPH_DLL_EXPORT CommandDispatch : public Command {
private:
    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;

public:
    CommandDispatch (uint32_t groupCountX,
                     uint32_t groupCountY,
                     uint32_t groupCountZ)
        : groupCountX (groupCountX)
        , groupCountY (groupCountY)
        , groupCountZ (groupCountZ)
    {
    }

    virtual void Record (CommandBuffer& commandBuffer) override
    {
        vkCmdDispatch (commandBuffer.GetHandle (), groupCountX, groupCountY, groupCountZ);
    }

    virtual bool IsEquivalent (const Command& other) override
    {
        if (auto otherCommand = dynamic_cast<const CommandDispatch*> (&other)) {
            return groupCountX == otherCommand->groupCountX &&
                   groupCountY == otherCommand->groupCountY &&
                   groupCountZ == otherCommand->groupCountZ;
        }

        return false;
    }
};


class RENDERGRAPH_DLL_EXPORT CommandDispatchBase : public Command {
private:
    uint32_t baseGroupX;
    uint32_t baseGroupY;
    uint32_t baseGroupZ;
    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;

public:
    CommandDispatchBase (uint32_t baseGroupX,
                     uint32_t baseGroupY,
                     uint32_t baseGroupZ,
                     uint32_t groupCountX,
                     uint32_t groupCountY,
                     uint32_t groupCountZ)
        : baseGroupX (baseGroupX)
        , baseGroupY (baseGroupY)
        , baseGroupZ (baseGroupZ)
        , groupCountX (groupCountX)
        , groupCountY (groupCountY)
        , groupCountZ (groupCountZ)
    {
    }

    virtual void Record (CommandBuffer& commandBuffer) override
    {
        vkCmdDispatchBase (commandBuffer.GetHandle (), baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
    }

    virtual bool IsEquivalent (const Command& other) override
    {
        if (auto otherCommand = dynamic_cast<const CommandDispatchBase*> (&other)) {
            return baseGroupX == otherCommand->baseGroupX &&
                   baseGroupY == otherCommand->baseGroupY &&
                   baseGroupY == otherCommand->baseGroupZ &&
                   groupCountY == otherCommand->groupCountX &&
                   groupCountY == otherCommand->groupCountY &&
                   groupCountZ == otherCommand->groupCountZ;
        }

        return false;
    }
};


class RENDERGRAPH_DLL_EXPORT CommandDispatchIndirect : public Command {
private:
    VkBuffer     buffer;
    VkDeviceSize offset;

public:
    CommandDispatchIndirect (VkBuffer     buffer,
                             VkDeviceSize offset)
        : buffer (buffer)
        , offset (offset)
    {
    }

    virtual void Record (CommandBuffer& commandBuffer) override
    {
        vkCmdDispatchIndirect (commandBuffer.GetHandle (), buffer, offset);
    }

    virtual bool IsEquivalent (const Command& other) override
    {
        if (auto otherCommand = dynamic_cast<const CommandDispatchIndirect*> (&other)) {
            return buffer == otherCommand->buffer &&
                   offset == otherCommand->offset;
        }

        return false;
    }
};

} // namespace GVK

#endif
