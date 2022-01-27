#ifndef RENDERGRAPH_HPP
#define RENDERGRAPH_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include "RenderGraph/Utils/Event.hpp"
#include "RenderGraph/VulkanWrapper/Utils/VulkanUtils.hpp"
#include <memory>

#include "RenderGraph/GraphSettings.hpp"
#include "RenderGraph/RenderGraphPass.hpp"

#include <set>
#include <unordered_set>


namespace RG {
class CommandBuffer;
class Swapchain;
}

namespace RG {
class Operation;
class Resource;
class GraphSettings;
}


namespace RG {

class RENDERGRAPH_DLL_EXPORT RenderGraph final : public Noncopyable {
public:// TODO
    bool                       compiled;
    std::vector<Pass>          passes;
    std::vector<RG::CommandBuffer> commandBuffers;
    
    std::unordered_map<VkImage, std::vector<VkImageLayout>> imageLayoutSequence;

public:
    GraphSettings graphSettings;

public:
    RenderGraph ();

    void Compile (GraphSettings&& settings);

    void Submit (uint32_t frameIndex, const std::vector<VkSemaphore>& waitSemaphores = {}, const std::vector<VkSemaphore>& signalSemaphores = {}, VkFence fence = VK_NULL_HANDLE);
    void Present (uint32_t imageIndex, RG::Swapchain& swapchain, const std::vector<VkSemaphore>& waitSemaphores = {});

    uint32_t GetPassCount () const;

    RG::ConnectionSet& GetConnectionSet () { return graphSettings.connectionSet; }

private:
    void CompileResources ();
    void CompileOperations ();
    Pass GetNextPass (const Pass& lastPass) const;
    Pass GetFirstPass () const;
    void CreatePasses ();
    void SeparatePasses ();
    void DebugPrint ();
};


} // namespace RG

#endif