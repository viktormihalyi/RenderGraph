#ifndef COMMANDLINEFLAG_HPP
#define COMMANDLINEFLAG_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include <string>
#include <vector>
#include <functional>
#include <optional>

namespace RG {


class RENDERGRAPH_DLL_EXPORT Matcher {
public:
    virtual ~Matcher () = default;

    virtual void SignMatch (int index) = 0;
};

class RENDERGRAPH_DLL_EXPORT CommandLineFlag {
public:
    CommandLineFlag ();
    virtual ~CommandLineFlag () = default;


    virtual void Match (int argc, char** argv, Matcher& matcher) = 0;

    virtual std::string GetHelpText () { return ""; }

    static void MatchAll (int argc, char** argv, bool printUnusedWarning = true);

protected:
    static std::optional<size_t> MatchOne (int argc, char** argv, Matcher& matcher, const std::string& flag);
};


class CommandLineOnOffFlag : public CommandLineFlag {
protected:
    bool                           on;
    const std::vector<std::string> flags;
    const std::string              helpText;

public:
    RENDERGRAPH_DLL_EXPORT CommandLineOnOffFlag (const std::string& flag, const std::string& helpText = "");
    RENDERGRAPH_DLL_EXPORT CommandLineOnOffFlag (const std::vector<std::string>& flags, const std::string& helpText = "");

    virtual RENDERGRAPH_DLL_EXPORT ~CommandLineOnOffFlag () override = default;

    bool RENDERGRAPH_DLL_EXPORT IsFlagOn () const;

    virtual std::string RENDERGRAPH_DLL_EXPORT GetHelpText () override;

protected:
    virtual void Match (int argc, char** argv, RG::Matcher& matcher) override;
    virtual void MatchParameters (int, char**, RG::Matcher&, size_t) {}
};


class CommandLineOnOffCallbackFlag : public CommandLineOnOffFlag {
private:
    
    std::function<void ()> onCallback;

public:

    RENDERGRAPH_DLL_EXPORT CommandLineOnOffCallbackFlag (const std::string& flag, const std::string& helpText, const std::function<void ()>& onCallback);
    RENDERGRAPH_DLL_EXPORT CommandLineOnOffCallbackFlag (const std::vector<std::string>& flags, const std::string& helpText, const std::function<void ()>& onCallback);

private:
    virtual void Match (int argc, char** argv, RG::Matcher& matcher) override;
};


/*
inline std::istringstream& get_istringstream ()
{
    static thread_local std::istringstream stream;
    stream.str ("");
    return stream;
}


template<typename T>
inline T from_string (const std::string& s)
{
    auto& iss (get_istringstream ());
    iss.str (s);
    T result;
    iss >> result;
    return result;
}

template<typename... Parameters>
class RENDERGRAPH_DLL_EXPORT CommandLineFlagWithNumberParameter : public CommandLineOnOffFlag {
private:
    std::tuple<Parameters...> values;
    int64_t number;

public:
    CommandLineFlagWithNumberParameter (const std::string& flag, const std::string& helpText = "")
        : CommandLineOnOffFlag { std::vector<std::string> { flag }, helpText }
        , number { 0 }
    {
    }

    int64_t GetValue () { return number; }

private:
    // virtual void MatchParameters (int argc, char** argv, RG::Matcher& matcher, size_t matched) override;

    virtual void MatchParameters (int argc, char** argv, RG::Matcher& matcher, size_t matched) override
    {

        if (RG_VERIFY (matched + 1 < argc)) {
            for (int i = 0; i < std::tuple_size<decltype (values)> {}; ++i) {
                std::get<i> (values) = from_string (argv[matched+i]);
            }
            number = std::stoi (argv[matched + 1]);
            matcher.SignMatch (matched + 1);
        }
    }
};
*/


} // namespace RG


#endif