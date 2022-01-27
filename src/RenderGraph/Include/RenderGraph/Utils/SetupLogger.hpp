#ifndef UTILS_SETUPLOGGER_HPP
#define UTILS_SETUPLOGGER_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include "spdlog/spdlog.h"

#include <string>

namespace Utils {

RENDERGRAPH_DLL_EXPORT
std::shared_ptr<spdlog::logger> GetLogger ();

RENDERGRAPH_DLL_EXPORT
std::shared_ptr<spdlog::logger> GetLogger (const std::string& customFileName);

} // namespace Utils


#endif
