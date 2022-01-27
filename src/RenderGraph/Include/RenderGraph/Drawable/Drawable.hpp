#ifndef DRAWRECORDABLE_HPP
#define DRAWRECORDABLE_HPP

#include "RenderGraph/RenderGraphExport.hpp"


namespace GVK {
class CommandBuffer;
}


namespace RG {

class RENDERGRAPH_DLL_EXPORT Drawable {
public:
    virtual ~Drawable ();

    virtual void Record (GVK::CommandBuffer&) const = 0;
};

} // namespace RG

#endif