#ifndef NOINLINE_HPP
#define NOINLINE_HPP

#include "CompilerDefinitions.hpp"

#if defined(COMPILER_MSVC)
#define NOINLINE __declspec(noinline)
#elif defined(COMPILER_GCC)
#define NOINLINE __attribute__ ((noinline))
#elif defined(COMPILER_CLANG)
#define NOINLINE __attribute__ ((noinline))
#else
#define NOINLINE
#endif

#endif