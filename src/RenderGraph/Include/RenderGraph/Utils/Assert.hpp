#ifndef ASSERT_HPP
#define ASSERT_HPP


#include "RenderGraph/RenderGraphExport.hpp"

#include "SourceLocation.hpp"


#define LOGASSERTS


#ifndef NDEBUG
#define GVK_ASSERT(condition) (::Utils::detail::DebugBreakAssertFunc (condition, true, "Assertion Failed", #condition, { __FILE__, __LINE__, __func__ }))
#define GVK_VERIFY(condition) (::Utils::detail::DebugBreakAssertFunc (condition, true, "Assertion Failed", #condition, { __FILE__, __LINE__, __func__ }))
#define GVK_ERROR(condition) (::Utils::detail::DebugBreakAssertFunc (condition, false, "Assertion Failed", #condition, { __FILE__, __LINE__, __func__ }))
#define GVK_BREAK() (::Utils::detail::DebugBreakFunc ("Debug Break", "", { __FILE__, __LINE__, __func__ }))
#define GVK_BREAK_STR(message) (::Utils::detail::DebugBreakFunc ("Debug Break", message, { __FILE__, __LINE__, __func__ }))
#elif defined(LOGASSERTS)
#define GVK_ASSERT(condition) (::Utils::detail::LogAssertFunc (condition, true, "Assertion Failed", #condition, { __FILE__, __LINE__, __func__ }))
#define GVK_VERIFY(condition) (::Utils::detail::LogAssertFunc (condition, true, "Assertion Failed", #condition, { __FILE__, __LINE__, __func__ }))
#define GVK_ERROR(condition) (::Utils::detail::LogAssertFunc (condition, false, "Assertion Failed", #condition, { __FILE__, __LINE__, __func__ }))
#define GVK_BREAK() (::Utils::detail::LogDebugBreakFunc ("Debug Break", "", { __FILE__, __LINE__, __func__ }))
#define GVK_BREAK_STR(message) (::Utils::detail::LogDebugBreakFunc ("Debug Break", message, { __FILE__, __LINE__, __func__ }))
#else
#define GVK_ASSERT(condition)
#define GVK_VERIFY(condition) ((bool)(condition))
#define GVK_ERROR(condition) ((bool)(condition))
#define GVK_BREAK()
#define GVK_BREAK_STR(message)
#endif


namespace Utils {

namespace detail {


inline bool DebugBreakAssertFunc (bool condition, const bool shouldBe, const char* dialogTitle, const char* conditionString, const SourceLocation& location);


RENDERGRAPH_DLL_EXPORT
void DebugBreakFunc (const char* dialogTitle, const char* conditionString, const SourceLocation& location);


RENDERGRAPH_DLL_EXPORT
bool LogAssertFunc (const bool condition, const bool shouldBe, const char* dialogTitle, const char* conditionString, const SourceLocation& location);


RENDERGRAPH_DLL_EXPORT
void LogDebugBreakFunc (const char* dialogTitle, const char* conditionString, const SourceLocation& location);


inline bool DebugBreakAssertFunc (bool condition, const bool shouldBe, const char* dialogTitle, const char* conditionString, const SourceLocation& location)
{
    if (condition != shouldBe)
        DebugBreakFunc (dialogTitle, conditionString, location);

    return condition;
}


} // namespace detail

} // namespace Utils


#endif