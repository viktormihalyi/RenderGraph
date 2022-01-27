#include "RenderGraph.hpp"

#include "GraphSettings.hpp"
#include "Operation.hpp"
#include "Drawable.hpp"
#include "Resource.hpp"
#include "ShaderPipeline.hpp"

#include "Utils/Utils.hpp"
#include "Utils/CommandLineFlag.hpp"

#include "VulkanWrapper/Swapchain.hpp"
#include "VulkanWrapper/CommandBuffer.hpp"
#include "VulkanWrapper/Commands.hpp"
#include "VulkanWrapper/GraphicsPipeline.hpp"
#include "VulkanWrapper/ComputePipeline.hpp"
#include "VulkanWrapper/RenderPass.hpp"
#include "VulkanWrapper/ShaderModule.hpp"
#include "VulkanWrapper/PipelineLayout.hpp"
#include "VulkanWrapper/DescriptorSet.hpp"
#include "VulkanWrapper/DescriptorSetLayout.hpp"

#include "spdlog/spdlog.h"

#include <iostream>
#include <sstream>


namespace RG {
    
RenderGraph::RenderGraph ()
    : compiled (false)
{
}


void RenderGraph::CompileResources ()
{
    RG::ForEach<Resource> (graphSettings.connectionSet.GetNodesByInsertionOrder (), [&] (std::shared_ptr<Resource>& res) {
        res->Compile (graphSettings);
    });
}


void RenderGraph::CompileOperations ()
{
    for (Pass& pass : passes) {
        for (Operation* op : pass.GetAllOperations ()) {
            ImageResource* firstImgRes = nullptr;

            for (Resource* res : pass.GetAllOutputs ()) {
                if (ImageResource* imgres = dynamic_cast<ImageResource*> (res)) {
                    if (firstImgRes == nullptr) {
                        firstImgRes = imgres;
                    } else {
                        const uint32_t firstWidth  = firstImgRes->GetImages ()[0]->GetWidth ();
                        const uint32_t firstHeight = firstImgRes->GetImages ()[0]->GetHeight ();

                        const uint32_t currentWidth  = imgres->GetImages ()[0]->GetWidth ();
                        const uint32_t currentHeight = imgres->GetImages ()[0]->GetHeight ();

                        if (firstWidth != currentWidth || firstHeight != currentHeight) {
                            throw std::runtime_error ("inconsistent output image extents");
                        }
                    }
                }
            }

            if (firstImgRes == nullptr) {
                op->Compile (graphSettings);
            } else {
                op->CompileWithExtent (graphSettings, firstImgRes->GetImages ()[0]->GetWidth (), firstImgRes->GetImages ()[0]->GetHeight ());
            }
        }
    }
}


Pass RenderGraph::GetNextPass (const Pass& lastPass) const
{
    Pass result;

    for (Operation* op : lastPass.GetAllOperations ()) {
        for (const std::shared_ptr<Resource>& res : graphSettings.connectionSet.GetPointingTo<Resource> (op)) {
            for (const std::shared_ptr<Operation>& nextOp : graphSettings.connectionSet.GetPointingTo<Operation> (res.get ())) {
                result.AddInput (nextOp.get (), res.get ());
            }
        }
    }

    for (auto& op : result.GetAllOperations ()) {
        std::vector<std::shared_ptr<Resource>> allInputs  = graphSettings.connectionSet.GetPointingHere<Resource> (op);
        std::vector<std::shared_ptr<Resource>> allOutputs = graphSettings.connectionSet.GetPointingTo<Resource> (op);
        for (std::shared_ptr<Resource> input : allInputs) {
            result.AddInput (op, input.get ());
        }
        for (std::shared_ptr<Resource> output : allOutputs) {
            result.AddOutput (op, output.get ());
        }
    }

    return result;
}


Pass RenderGraph::GetFirstPass () const
{
    std::set<Operation*>    allOpSet;
    std::vector<Operation*> allOp;

    RG::ForEach<Operation> (graphSettings.connectionSet.GetNodesByInsertionOrder (), [&] (const std::shared_ptr<Operation>& op) {
        const std::vector<std::shared_ptr<Resource>> opInputs = graphSettings.connectionSet.GetPointingHere<Resource> (op.get ());

        const bool allInputsAreFirstWrittenByThisOp = std::all_of (opInputs.begin (), opInputs.end (), [&] (const std::shared_ptr<Resource>& res) {
            return graphSettings.connectionSet.GetPointingHere<Node> (res.get ()).empty ();
        });

        if (allInputsAreFirstWrittenByThisOp || opInputs.empty ())
            if (allOpSet.insert (op.get ()).second)
                allOp.push_back (op.get ());
    });

    Pass actualResult;

    for (Operation* op : allOp) {
        std::vector<std::shared_ptr<Resource>> allInputs  = graphSettings.connectionSet.GetPointingHere<Resource> (op);
        std::vector<std::shared_ptr<Resource>> allOutputs = graphSettings.connectionSet.GetPointingTo<Resource> (op);
        for (std::shared_ptr<Resource> input : allInputs) {
            actualResult.AddInput (op, input.get ());
        }
        for (std::shared_ptr<Resource> output : allOutputs) {
            actualResult.AddOutput (op, output.get ());
        }
    }

    return actualResult;
}


void RenderGraph::CreatePasses ()
{
    passes.clear ();
    
    Pass nextPass = GetFirstPass ();
    do {
        passes.push_back (nextPass);
        nextPass = GetNextPass (nextPass);
    } while (!nextPass.GetAllOperations ().empty ());

    SeparatePasses ();
}


struct OutputHitCount {
    std::unordered_map<Resource*, std::vector<Operation*>> hitCount;
    std::vector<Resource*>                                 insertionOrder;
};

static OutputHitCount GetOutputHitCount (Pass pass)
{
    const std::vector<Operation*> operations = pass.GetAllOperations ();

    OutputHitCount result;

    for (const auto op : operations) {
        const auto opIO = pass.GetOperationIO (op);
        for (Resource* output : opIO->outputs) {
            const bool contains = result.hitCount.find (output) != result.hitCount.end ();
            result.hitCount[output].push_back (op);
            if (!contains) {
                result.insertionOrder.push_back (output);
            }
        }
    }

    return result;
}


static bool HasMultipleOperationsToOneOutput (Pass pass)
{
    const OutputHitCount outputCount = GetOutputHitCount (pass);

    for (const auto& oc : outputCount.hitCount) {
        if (oc.second.size () > 1) {
            return true;
        }
    }

    return false;
}


void RenderGraph::SeparatePasses ()
{
    std::vector<Pass> newPasses { passes };

    bool seperationHappened = false;

    for (size_t i = 0; i < newPasses.size (); ++i) {
        const bool needsSeperation = HasMultipleOperationsToOneOutput (newPasses[i]);
        if (needsSeperation) {
            const OutputHitCount hitCount = GetOutputHitCount (newPasses[i]);

            if (i == newPasses.size () - 1) {
                newPasses.emplace_back ();
            }

            Resource*               firstResourceToSplitOn = hitCount.insertionOrder[0];
            std::vector<Operation*> operationsToSplit      = hitCount.hitCount.find (firstResourceToSplitOn)->second;

            // Operation* firstOperation  = operationsToSplit[0];
            Operation* secondOperation = operationsToSplit[1];

            Pass::OperationIO* toMove = newPasses[i].GetOperationIO (secondOperation);

            newPasses[i + 1].AddOperationIO (toMove);
            newPasses[i].RemoveOperationIO (toMove);

            seperationHappened = true;
            break;
        }
    }

    passes = newPasses;

    if (seperationHappened)
        SeparatePasses ();
}


void RenderGraph::DebugPrint ()
{
    std::stringstream logString;
    for (size_t i = 0; i < passes.size (); ++i) {
        const Pass& pass = passes[i];
        logString << "Pass " << i << std::endl;
        for (const Operation* op : pass.GetAllOperations ()) {
            logString << "\tOperation \"" << op->GetName () << "\" (debugInfo: \"" << op->GetDebugInfo () << "\", " << op->GetUUID ().GetValue () << ")" << std::endl;
            logString << "\tInputs:" << std::endl;
            auto resinp = graphSettings.connectionSet.GetPointingHere<Resource> (op);
            for (auto res : resinp) {
                logString << "\t\tInput Resource \"" << res->GetName () << "\" (debugInfo: \"" << res->GetDebugInfo () << "\", id: " << res->GetUUID ().GetValue () << ")" << std::endl;
            }
            logString << "\tOutputs:" << std::endl;
            auto resout = graphSettings.connectionSet.GetPointingTo<Resource> (op);
            for (auto res : resout) {
                logString << "\t\tOutput Resource \"" << res->GetName () << "\" (debugInfo: \"" << res->GetDebugInfo () << "\", id: " << res->GetUUID ().GetValue () << ")" << std::endl;
            }
        }
    }

    spdlog::info ("======= Render graph begin =======");
    spdlog::info ("{}", logString.str ());
    spdlog::info ("======= Render graph end =========");
}


RG::CommandLineOnOffFlag printRenderGraphFlag { "--printRenderGraph", "Prints render graph passes, operatins, resources." };


void RenderGraph::Compile (GraphSettings&& graphSettings_)
{
    graphSettings = std::move (graphSettings_);

    graphSettings.GetDevice ().Wait ();
    graphSettings.GetDevice ().GetGraphicsQueue ().Wait ();

    CreatePasses ();

    if (printRenderGraphFlag.IsFlagOn ()) {
        DebugPrint ();
    }

    CompileResources ();

    CompileOperations ();

    imageLayoutSequence.clear ();

    for (Pass& p : passes) {
        RG::ForEach<ImageResource*> (p.GetAllInputs (), [&] (ImageResource* img) {
            for (RG::Image* image : img->GetImages ()) {
                imageLayoutSequence[*image].push_back (img->GetInitialLayout ());
            }
        });
        RG::ForEach<ImageResource*> (p.GetAllOutputs (), [&] (ImageResource* img) {
            for (RG::Image* image : img->GetImages ()) {
                imageLayoutSequence[*image].push_back (img->GetInitialLayout ());
            }
        });
    }

    const VkAccessFlags fullMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
                                   VK_ACCESS_INDEX_READ_BIT |
                                   VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
                                   VK_ACCESS_UNIFORM_READ_BIT |
                                   VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
                                   VK_ACCESS_SHADER_READ_BIT |
                                   VK_ACCESS_SHADER_WRITE_BIT |
                                   VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                   VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                   VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                                   VK_ACCESS_TRANSFER_READ_BIT |
                                   VK_ACCESS_TRANSFER_WRITE_BIT;

    VkMemoryBarrier flushAllMemory = {};
    flushAllMemory.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    flushAllMemory.srcAccessMask   = fullMask;
    flushAllMemory.dstAccessMask   = fullMask;

    commandBuffers.clear ();

    for (uint32_t frameIndex = 0; frameIndex < graphSettings.framesInFlight; ++frameIndex) {
        RG::CommandBuffer& currentCmdbuffer = commandBuffers.emplace_back (graphSettings.GetDevice ());

        currentCmdbuffer.SetName (*graphSettings.device, fmt::format ("CommandBuffer {}/{}", frameIndex, graphSettings.framesInFlight));

        currentCmdbuffer.Begin ();

        for (Pass& p : passes) {
            for (auto op : p.GetAllOperations ()) {
                auto allInputs  = graphSettings.connectionSet.GetPointingHere<Resource> (op);
                auto allOutputs = graphSettings.connectionSet.GetPointingTo<Resource> (op);

                std::unique_ptr<RG::CommandPipelineBarrier> barrier = std::make_unique<RG::CommandPipelineBarrier> (VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,  // TODO maybe VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT?
                                                                                                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT); // TODO maybe VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT?
                barrier->AddMemoryBarrier (flushAllMemory);
                RG::ForEach<ImageResource> (allInputs, [&] (const std::shared_ptr<ImageResource>& img) {
                    for (RG::Image* image : img->GetImages (frameIndex)) {
                        const VkImageLayout currentLayout = imageLayoutSequence[*image].back ();
                        const VkImageLayout newLayout     = op->GetImageLayoutAtStartForInputs (*img);
                        barrier->AddImageMemoryBarrier (image->GetBarrier (currentLayout, newLayout, fullMask, fullMask));
                        imageLayoutSequence[*image].push_back (newLayout);
                    }
                });

                RG::ForEach<ImageResource> (allOutputs, [&] (const std::shared_ptr<ImageResource>& img) {
                    for (RG::Image* image : img->GetImages (frameIndex)) {
                        const VkImageLayout currentLayout = imageLayoutSequence[*image].back ();
                        const VkImageLayout newLayout     = op->GetImageLayoutAtStartForOutputs (*img);
                        barrier->AddImageMemoryBarrier (image->GetBarrier (currentLayout, newLayout, fullMask, fullMask));
                        imageLayoutSequence[*image].push_back (newLayout);
                    }
                });

                currentCmdbuffer.RecordCommand (std::move (barrier))
                    .SetName ("Transition for next Pass");

                RG::ForEach<ImageResource> (allInputs, [&] (const std::shared_ptr<ImageResource>& img) {
                    for (RG::Image* image : img->GetImages (frameIndex)) {
                        imageLayoutSequence[*image].push_back (op->GetImageLayoutAtEndForInputs (*img)); // TODO VkAttachmentDescription.finalLayout
                    }
                });

                RG::ForEach<ImageResource> (allOutputs, [&] (const std::shared_ptr<ImageResource>& img) {
                    for (RG::Image* image : img->GetImages (frameIndex)) {
                        imageLayoutSequence[*image].push_back (op->GetImageLayoutAtEndForOutputs (*img)); // TODO VkAttachmentDescription.finalLayout
                    }
                });
            }

            for (auto op : p.GetAllOperations ()) {
                op->Record (graphSettings.connectionSet, frameIndex, currentCmdbuffer);
            }
        }

        {
            std::unique_ptr<RG::CommandPipelineBarrier> barrier = std::make_unique<RG::CommandPipelineBarrier> (VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,  // TODO maybe VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT?
                                                                                                                  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT); // TODO maybe VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT?
            barrier->AddMemoryBarrier (flushAllMemory);
            for (Pass& p : passes) {
                RG::ForEach<ImageResource*> (p.GetAllInputs (), [&] (ImageResource* img) {
                    for (RG::Image* image : img->GetImages (frameIndex)) {
                        const VkImageLayout currentLayout = imageLayoutSequence[*image].back ();
                        barrier->AddImageMemoryBarrier (image->GetBarrier (currentLayout, img->GetInitialLayout (), fullMask, fullMask));
                    }
                });
            }
            currentCmdbuffer.RecordCommand (std::move (barrier));
        }

        currentCmdbuffer.End ();
    }

    compiled = true;
}


void RenderGraph::Submit (uint32_t frameIndex, const std::vector<VkSemaphore>& waitSemaphores, const std::vector<VkSemaphore>& signalSemaphores, VkFence fenceToSignal)
{
    if (RG_ERROR (!compiled)) {
        return;
    }

    if (RG_ERROR (frameIndex >= graphSettings.framesInFlight)) {
        return;
    }

    std::vector<VkPipelineStageFlags> waitDstStageMasks (waitSemaphores.size (), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    graphSettings.device->GetGraphicsQueue ().Submit (waitSemaphores, waitDstStageMasks, { &commandBuffers[frameIndex] }, signalSemaphores, fenceToSignal);
}


void RenderGraph::Present (uint32_t imageIndex, RG::Swapchain& swapchain, const std::vector<VkSemaphore>& waitSemaphores)
{
    RG_ASSERT (swapchain.SupportsPresenting ());

    // TODO itt present queue kene
    swapchain.Present (graphSettings.device->GetGraphicsQueue (), imageIndex, waitSemaphores);
}


uint32_t RenderGraph::GetPassCount () const
{
    RG_ASSERT (compiled);

    return static_cast<uint32_t> (passes.size ());
}

} // namespace RG
