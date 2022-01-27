#ifndef SOURCELOCATION_HPP
#define SOURCELOCATION_HPP

#include "RenderGraph/RenderGraphExport.hpp"

namespace Utils {

// TODO replace std::source_location when c++20 happens

struct RENDERGRAPH_DLL_EXPORT SourceLocation {
    const char* file;
    int         line;
    const char* function;
};

} // namespace Utils

#endif