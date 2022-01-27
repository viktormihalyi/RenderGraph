#ifndef MESSAGEBOX_HPP
#define MESSAGEBOX_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include <string>

enum class MessageBoxResult {
    Yes,
    No,
    Third,
    Error,
};

RENDERGRAPH_DLL_EXPORT
MessageBoxResult ShowMessageBox (const std::string& title, const std::string& message);

#endif