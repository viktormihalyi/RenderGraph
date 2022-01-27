#ifndef GRAPHSETTINGS_HPP
#define GRAPHSETTINGS_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include "RenderGraph/Node.hpp"
#include "RenderGraph/VulkanWrapper/DeviceExtra.hpp"

#include <memory>
#include <set>
#include <vulkan/vulkan.h>


namespace RG {

class RENDERGRAPH_DLL_EXPORT NodeConnection {
public:
    std::shared_ptr<Node> from;
    std::shared_ptr<Node> to;

    NodeConnection (const std::shared_ptr<Node>& from,
                    const std::shared_ptr<Node>& to)
        : from (from)
        , to (to)
    {
    }
};

class RENDERGRAPH_DLL_EXPORT ConnectionSet final : public Noncopyable {
private:

    std::vector<NodeConnection> connections;
    
    std::set<std::shared_ptr<Node>> nodeSet;
    
    std::vector<std::shared_ptr<Node>> insertionOrder;

public:
    
    ConnectionSet ();
    ConnectionSet (ConnectionSet&&) noexcept;
    ConnectionSet& operator= (ConnectionSet&&) noexcept;

    virtual ~ConnectionSet () override;

    std::shared_ptr<Node> GetNodeByName (std::string_view name) const;

    template <typename T>
    std::shared_ptr<T> GetByName (std::string_view name) const
    {
        return std::dynamic_pointer_cast<T> (GetNodeByName (name));
    }

    template<typename T>
    std::vector<std::shared_ptr<T>> GetPointingTo (const Node* node) const
    {
        std::vector<std::shared_ptr<T>> result;

        for (const NodeConnection& c : connections) {
            if (c.from.get () == node) {
                if (auto asCasted = std::dynamic_pointer_cast<T> (c.to)) {
                    result.push_back (asCasted);
                }
            }
        }

        return result;
    }
    template<typename T>
    std::vector<std::shared_ptr<T>> GetPointingHere (const Node* node) const
    {
        std::vector<std::shared_ptr<T>> result;

        for (const NodeConnection& c : connections) {
            if (c.to.get () == node) {
                if (auto asCasted = std::dynamic_pointer_cast<T> (c.from)) {
                    result.push_back (asCasted);
                }
            }
        }

        return result;
    }

    void Add (const NodeConnection& connection)
    {
        Add (connection.from);
        Add (connection.to);

        connections.push_back (connection);
    }

    void Add (const std::shared_ptr<Node>& from, const std::shared_ptr<Node>& to)
    {
        Add ({ from, to });
    }

    void Add (const std::shared_ptr<Node>& node)
    {
        if (nodeSet.insert (node).second)
            insertionOrder.push_back (node);
    }

    const std::vector<std::shared_ptr<Node>>& GetNodesByInsertionOrder () const
    {
        return insertionOrder;
    }

};


class RENDERGRAPH_DLL_EXPORT GraphSettings {
public:
    ConnectionSet           connectionSet;
    const GVK::DeviceExtra* device;
    uint32_t                framesInFlight;

    GraphSettings (const GVK::DeviceExtra& device, ConnectionSet&& connectionSet, uint32_t framesInFlight);
    GraphSettings (const GVK::DeviceExtra& device, uint32_t framesInFlight);

    GraphSettings ();
    GraphSettings (GraphSettings&&) noexcept;
    GraphSettings& operator= (GraphSettings&&) noexcept;

    const GVK::DeviceExtra& GetDevice () const;

    const GVK::Queue& GetGrahpicsQueue () const;

    const GVK::CommandPool& GetCommandPool () const;
};

} // namespace RG

#endif