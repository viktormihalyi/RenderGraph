#include "Utils/StaticInit.hpp"
#include "Utils/SetupLogger.hpp"

StaticInit renderGraphLogInitializer (std::bind (spdlog::set_default_logger, RG::GetLogger ()));