#ifndef RG_NODE_HPP
#define RG_NODE_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include "RenderGraph/Utils/Noncopyable.hpp"
#include "RenderGraph/Utils/UUID.hpp"
#include <string>

namespace RG {

class RENDERGRAPH_DLL_EXPORT Node : public Noncopyable {
private:
    GVK::UUID   uuid;
    std::string name;
    std::string debugInfo;

public:
    virtual ~Node () = default;

    const GVK::UUID& GetUUID () const { return uuid; }

    void SetName (const std::string& value) { name = value; }
    void SetDebugInfo (const std::string& value) { debugInfo = value; }

    const std::string& GetName () const { return name; }
    const std::string& GetDebugInfo () const { return debugInfo; }
};

} // namespace RG

#endif
