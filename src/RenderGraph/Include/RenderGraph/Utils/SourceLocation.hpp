#ifndef SOURCELOCATION_HPP
#define SOURCELOCATION_HPP

#include "RenderGraph/RenderGraphExport.hpp"

namespace RG {

// TODO replace std::source_location when c++20 happens

struct RENDERGRAPH_DLL_EXPORT SourceLocation {
    const char* file;
    int         line;
    const char* function;
};

} // namespace RG

#endif