#include "RenderGraph/Utils/StaticInit.hpp"
#include "RenderGraph/Utils/SetupLogger.hpp"

StaticInit testsLogInitializer (std::bind (spdlog::set_default_logger, Utils::GetLogger ()));
