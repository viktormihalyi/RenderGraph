#ifndef COMPILER_DEFINITIONS_HPP
#define COMPILER_DEFINITIONS_HPP

#ifdef _MSC_VER
#define COMPILER_MSVC
#endif

#ifdef __GNUC__
#define COMPILER_GCC
#endif

#ifdef __clang__
#define COMPILER_CLANG
#endif

#if !defined(COMPILER_MSVC) && !defined(COMPILER_GCC) && !defined(COMPILER_CLANG)
#error unknown compiler
#endif

#endif