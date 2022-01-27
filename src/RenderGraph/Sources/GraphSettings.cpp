#include "GraphSettings.hpp"
#include "Resource.hpp"

#include "VulkanWrapper/Image.hpp"
#include "VulkanWrapper/ImageView.hpp"

#include <unordered_set>

namespace RG {


ConnectionSet::~ConnectionSet () = default;


GraphSettings::GraphSettings (const RG::DeviceExtra& device, ConnectionSet&& connectionSet, uint32_t framesInFlight)
    : device (&device)
    , framesInFlight (framesInFlight)
    , connectionSet (std::move (connectionSet))
{
}


GraphSettings::GraphSettings (const RG::DeviceExtra& device, uint32_t framesInFlight)
    : device (&device)
    , framesInFlight (framesInFlight)
{
}


GraphSettings::GraphSettings ()
    : device (nullptr)
    , framesInFlight (0)
{
}

const RG::DeviceExtra& GraphSettings::GetDevice () const
{
    RG_ASSERT (device != nullptr);
    return *device;
}


const RG::Queue& GraphSettings::GetGrahpicsQueue () const
{
    return device->GetGraphicsQueue ();
}


const RG::CommandPool& GraphSettings::GetCommandPool () const
{
    return device->GetCommandPool ();
}


GraphSettings::GraphSettings (GraphSettings&& other) noexcept
    : connectionSet (std::move (other.connectionSet))
    , device (other.device)
    , framesInFlight (other.framesInFlight)
{
    other.device         = nullptr;
    other.framesInFlight = 0;
}


GraphSettings& GraphSettings::operator= (GraphSettings&& other) noexcept
{
    if (this != &other) {
        connectionSet  = std::move (other.connectionSet);
        device         = other.device;
        framesInFlight = other.framesInFlight;

        other.device         = nullptr;
        other.framesInFlight = 0;
    }

    return *this;
}


ConnectionSet::ConnectionSet () = default;


ConnectionSet::ConnectionSet (ConnectionSet&& other) noexcept
    : connections (std::move (other.connections))
    , nodeSet (std::move (other.nodeSet))
    , insertionOrder (std::move (other.insertionOrder))
{
    other.connections.clear ();
    other.nodeSet.clear ();
    other.insertionOrder.clear ();
}


ConnectionSet& ConnectionSet::operator= (ConnectionSet&& other) noexcept
{
    if (this != &other) {
        connections    = std::move (other.connections);
        nodeSet        = std::move (other.nodeSet);
        insertionOrder = std::move (other.insertionOrder);

        other.connections.clear ();
        other.nodeSet.clear ();
        other.insertionOrder.clear ();
    }

    return *this;
}


std::shared_ptr<Node> ConnectionSet::GetNodeByName (std::string_view name) const
{
    if constexpr (IsDebugBuild) {
        std::unordered_set<std::string> nameSet;
        for (const auto& node : insertionOrder) {
            if (node->GetName ().empty ())
                continue;

            RG_ASSERT (nameSet.count (node->GetName ()) == 0);
            nameSet.insert (node->GetName ());
        }
    }

    for (const auto& node : insertionOrder) {
        if (node->GetName () == name) {
            return node;
        }
    }

    return nullptr;
}

} // namespace RG
