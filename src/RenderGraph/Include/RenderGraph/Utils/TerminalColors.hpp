#ifndef TERMINALCOLORS_HPP
#define TERMINALCOLORS_HPP

namespace TerminalColors {

// for Powershell:
// regedit [HKEY_CURRENT_USER\Console], create or set the VirtualTerminalLevel DWORD value to 1

constexpr const char* RESET       = "\033[0m";
constexpr const char* BLACK       = "\033[30m";
constexpr const char* RED         = "\033[31m";
constexpr const char* GREEN       = "\033[32m";
constexpr const char* YELLOW      = "\033[33m";
constexpr const char* BLUE        = "\033[34m";
constexpr const char* MAGENTA     = "\033[35m";
constexpr const char* CYAN        = "\033[36m";
constexpr const char* WHITE       = "\033[37m";
constexpr const char* BOLDBLACK   = "\033[1m\033[30m";
constexpr const char* BOLDRED     = "\033[1m\033[31m";
constexpr const char* BOLDGREEN   = "\033[1m\033[32m";
constexpr const char* BOLDYELLOW  = "\033[1m\033[33m";
constexpr const char* BOLDBLUE    = "\033[1m\033[34m";
constexpr const char* BOLDMAGENTA = "\033[1m\033[35m";
constexpr const char* BOLDCYAN    = "\033[1m\033[36m";
constexpr const char* BOLDWHITE   = "\033[1m\033[37m";

} // namespace TerminalColors

#endif