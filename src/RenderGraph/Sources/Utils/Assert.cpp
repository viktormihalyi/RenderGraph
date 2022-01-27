#include "Assert.hpp"

#include "CommandLineFlag.hpp"
#include "CompilerDefinitions.hpp"
#include "MessageBox.hpp"

#include <memory>
#include <set>
#include <string>
#include <sstream>
#include <vector>

#include "spdlog/spdlog.h"


namespace RG {

namespace detail {

static void DebugBreak ()
{
#if defined(COMPILER_MSVC)
    __debugbreak ();
#elif defined(COMPILER_CLANG)
    __builtin_debugtrap ();
#elif defined(COMPILER_GCC)
    __builtin_trap ();
#else
#error "unknown compiler for debug break"
#endif
}


static std::string SourceLocationToString (const SourceLocation& sourceLocation)
{
    return fmt::format ("{}: {} ({})", sourceLocation.file, sourceLocation.line, sourceLocation.function);
}


static void ShowAssertPopup (const std::string&    dialogTitle,
                             const std::string&    dialogMessage,
                             const SourceLocation& location)
{
    static std::set<std::string> ignoredAssertions;
    
    const std::string locationString = SourceLocationToString (location);

    if (ignoredAssertions.find (locationString) != std::end (ignoredAssertions)) {
        return;
    }

    bool ignoreNextTime = false;

    const std::string dialogLocationAndMessage = SourceLocationToString (location) + "\n" + dialogMessage;

    const MessageBoxResult result = ShowMessageBox (dialogTitle, dialogLocationAndMessage);

    switch (result) {
        case MessageBoxResult::Yes:
            DebugBreak ();
            break;

        case MessageBoxResult::Third: // "Ignore"
            ignoreNextTime = true;
            break;

        case MessageBoxResult::No:
        case MessageBoxResult::Error:
            break;
    }

    if (ignoreNextTime) {
        ignoredAssertions.insert (locationString);
    }
}


static CommandLineOnOffFlag disableAssertsFlag (std::vector<std::string> { "--disableAsserts", "-a" }, "Disables asserts.");


void DebugBreakFunc (const char* dialogTitle, const char* conditionString, const SourceLocation& location)
{
    if (disableAssertsFlag.IsFlagOn ()) {
        LogDebugBreakFunc (dialogTitle, conditionString, location);
    } else {
        ShowAssertPopup (dialogTitle, conditionString, location);
    }
}


bool LogAssertFunc (bool condition, const bool shouldBe, const char* dialogTitle, const char* conditionString, const SourceLocation& location)
{
    if (condition != shouldBe) {
        LogDebugBreakFunc (dialogTitle, conditionString, location);
    }
    return condition;
}


void LogDebugBreakFunc (const char* dialogTitle, const char* conditionString, const SourceLocation& location)
{
    spdlog::error ("[{}] {} - {}", dialogTitle, SourceLocationToString (location), conditionString);
}


} // namespace detail

} // namespace RG