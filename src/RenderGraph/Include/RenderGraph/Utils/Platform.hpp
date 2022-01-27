#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#ifdef __linux__
#define PLATFORM_LINUX
#elif _WIN32
#define PLATFORM_WINDOWS
#elif __APPLE__
#define PLATFORM_MACOS
#else
#error Platform not supported
#endif

#endif