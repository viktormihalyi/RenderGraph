#include "CommandLineFlag.hpp"
#include "Assert.hpp"

#include <iostream>
#include <sstream>
#include <vector>
#include <memory>

namespace Utils {

class CommandLineFlagRegistry {
public:
    std::vector<CommandLineFlag*> flags;

    void Register (CommandLineFlag& flag);

    void MatchAll (int argc, char** argv, bool printUnusedWarning);
};


static void PrintCommandLineHelpMessage ();


void CommandLineFlagRegistry::MatchAll (int argc, char** argv, bool printUnusedWarning)
{
    GVK_ASSERT (argc >= 1);

    std::vector<bool> usedIndices (argc, false);

    if (!usedIndices.empty ()) {
        usedIndices[0] = true;
    }

    struct MatcherAdapter : Matcher {
        std::vector<bool>& usedIndices;
        MatcherAdapter (std::vector<bool>& usedIndices)
            : usedIndices { usedIndices }
        {
        }
        virtual void SignMatch (int index) override { usedIndices[index] = true; }
    } matcher { usedIndices };

    for (CommandLineFlag* flag : flags) {
        flag->Match (argc, argv, matcher);
    }

    if (printUnusedWarning) {
        bool wasUnused = false;
        for (int i = 0; i < argc; ++i) {
            if (!usedIndices[i]) {
                std::cout << "[COMMAND LINE ARGS] Argument \"" << argv[i] << "\" is unknown (at index " << i << ")" << std::endl;
                wasUnused = true;
            }
        }

        if (wasUnused) {
            PrintCommandLineHelpMessage ();
        }
    }
}


void CommandLineFlagRegistry::Register (CommandLineFlag& flag)
{
    flags.push_back (&flag);
}


static std::unique_ptr<CommandLineFlagRegistry> globalCommandLineFlagRegistry;


static void EnsureRegistryInitialized ()
{
    if (globalCommandLineFlagRegistry == nullptr) {
        globalCommandLineFlagRegistry = std::make_unique<CommandLineFlagRegistry> ();
    }
}


CommandLineFlag::CommandLineFlag ()
{
    EnsureRegistryInitialized ();
    globalCommandLineFlagRegistry->Register (*this);
}


void CommandLineFlag::MatchAll (int argc, char** argv, bool printUnusedWarning)
{
    EnsureRegistryInitialized ();
    globalCommandLineFlagRegistry->MatchAll (argc, argv, printUnusedWarning);
}


static std::string ASCIIToLower (const std::string& input)
{
    std::string result = input;

    const int lowerDist = 'a' - 'A';

    for (size_t i = 0; i < result.size (); ++i) {
        if ('A' <= result[i] && result[i] <= 'Z') {
            result[i] += lowerDist;
        }
    }

    return result;
}


std::optional<size_t> CommandLineFlag::MatchOne (int argc, char** argv, Matcher& matcher, const std::string& flag)
{
    for (int i = 0; i < argc; ++i) {
        if (ASCIIToLower (std::string (argv[i])) == ASCIIToLower (flag)) {
            matcher.SignMatch (i);
            return i;
        }
    }
    return std::nullopt;
}


CommandLineOnOffFlag::CommandLineOnOffFlag (const std::string& flag, const std::string& helpText)
    : CommandLineOnOffFlag (std::vector<std::string> { flag }, helpText)
{
}


CommandLineOnOffFlag::CommandLineOnOffFlag (const std::vector<std::string>& flags, const std::string& helpText)
    : flags (flags)
    , helpText (helpText)
    , on (false)
{
    if (GVK_ERROR (flags.empty ())) {
        throw std::runtime_error ("no flags provided");
    }
}


bool CommandLineOnOffFlag::IsFlagOn () const
{
    return on;
}


std::string CommandLineOnOffFlag::GetHelpText ()
{
    std::stringstream ss;
    for (size_t i = 0; i < flags.size (); ++i) {
        ss << flags[i];
        if (i != flags.size () - 1) {
            ss << "|";
        }
    }
    if (helpText.empty ()) {
        return std::string (ss.str ()) + ": -";
    }

    return std::string (ss.str ()) + ": " + helpText;
}


void CommandLineOnOffFlag::Match (int argc, char** argv, Utils::Matcher& matcher)
{
    bool   found     = false;
    size_t nextIndex = 0;
    while (!found && nextIndex < flags.size ()) {
        const std::optional<size_t> matchIdx = MatchOne (argc, argv, matcher, flags.at (nextIndex));

        found = matchIdx.has_value ();

        if (found) {
            MatchParameters (argc, argv, matcher, *matchIdx);
        }

        nextIndex++;
    }

    if (found) {
        on = true;
    }
}


CommandLineOnOffCallbackFlag::CommandLineOnOffCallbackFlag (const std::string& flag, const std::string& helpText, const std::function<void ()>& onCallback)
    : CommandLineOnOffFlag (std::vector<std::string> { flag }, helpText)
    , onCallback (onCallback)
{
}


CommandLineOnOffCallbackFlag::CommandLineOnOffCallbackFlag (const std::vector<std::string>& flags, const std::string& helpText, const std::function<void ()>& onCallback)
    : CommandLineOnOffFlag (flags, helpText)
    , onCallback (onCallback)
{
}


void CommandLineOnOffCallbackFlag::Match (int argc, char** argv, Utils::Matcher& matcher)
{
    CommandLineOnOffFlag::Match (argc, argv, matcher);

    if (on)
        onCallback ();
}


namespace {

class CommandLineHelpFlag : public CommandLineFlag {
public:
    virtual void Match (int argc, char** argv, Matcher& matcher)
    {
        if (MatchOne (argc, argv, matcher, "-h") || MatchOne (argc, argv, matcher, "--help")) {
            PrintCommandLineHelpMessage ();
        }
    }
} helpFlag;

} // namespace


static void PrintCommandLineHelpMessage ()
{
    std::cout << "Available command line flags: " << std::endl;
    for (CommandLineFlag* flag : globalCommandLineFlagRegistry->flags) {
        const std::string helpText = flag->GetHelpText ();
        if (!helpText.empty ()) {
            std::cout << "    " << helpText << std::endl;
        }
    }
    std::cout << std::endl;
}

} // namespace Utils
