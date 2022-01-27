#include "SetupLogger.hpp"
#include "StaticInit.hpp"
#include "Assert.hpp"
#include "CommandLineFlag.hpp"

#include <stdio.h>
#define __STDC_WANT_LIB_EXT1__ 1
#include <time.h>
#include <optional>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"


namespace Utils {

static CommandLineOnOffCallbackFlag logTraceFlag { "--logTrace", "Set logger to trace level.", [] () { GetLogger ()->set_level (spdlog::level::trace); } };
static CommandLineOnOffCallbackFlag logDebugFlag { "--logDebug", "Set logger to debug level.", [] () { GetLogger ()->set_level (spdlog::level::debug); } };
static CommandLineOnOffCallbackFlag logInfoFlag { "--logInfo", "Set logger to info level.", [] () { GetLogger ()->set_level (spdlog::level::info); } };
static CommandLineOnOffCallbackFlag logWarnFlag { "--logWarn", "Set logger to warn level.", [] () { GetLogger ()->set_level (spdlog::level::warn); } };
static CommandLineOnOffCallbackFlag logErrFlag { "--logErr", "Set logger to err level.", [] () { GetLogger ()->set_level (spdlog::level::err); } };
static CommandLineOnOffCallbackFlag logCriticalFlag { "--logCritical", "Set logger to critical level.", [] () { GetLogger ()->set_level (spdlog::level::critical); } };
static CommandLineOnOffCallbackFlag logOffFlag { "--logOff", "Turns off logger.", [] () { GetLogger ()->set_level (spdlog::level::off); } };


std::shared_ptr<spdlog::logger> GetLogger (const std::string& logFileName)
{
    static std::shared_ptr<spdlog::logger> logger;

    if (logger == nullptr) {
        const size_t maxFileSize = 1024 * 1024 * 10;
        const size_t maxFiles    = 1000;

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back (std::make_shared<spdlog::sinks::stdout_color_sink_st> ());
        sinks.push_back (std::make_shared<spdlog::sinks::rotating_file_sink_mt> (logFileName, maxFileSize, maxFiles));

        logger = std::make_shared<spdlog::logger> ("GearsVk", std::begin (sinks), std::end (sinks));

        logger->set_pattern ("[%T] [%^%l%$] [t %t] %v");
    }

    return logger;
}


std::shared_ptr<spdlog::logger> GetLogger ()
{
    time_t    now = time (0);
    struct tm tstruct;

#if _WIN32
    ::localtime_s (&tstruct, &now);
#else
    ::localtime_r (&now, &tstruct);
#endif

    const std::string defaultLogFileName = fmt::format ("GearsVk_{}-{}-{}_{}-{}-{}.txt", tstruct.tm_year + 1900, tstruct.tm_mon, tstruct.tm_mday, tstruct.tm_hour, tstruct.tm_min, tstruct.tm_sec);

    return GetLogger (defaultLogFileName);
}

} // namespace Utils


StaticInit utilsLogInitializer (std::bind (spdlog::set_default_logger, Utils::GetLogger ()));