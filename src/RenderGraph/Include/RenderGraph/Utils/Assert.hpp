#ifndef ASSERT_HPP
#define ASSERT_HPP


#include "RenderGraph/RenderGraphExport.hpp"

#include "SourceLocation.hpp"


#define LOGASSERTS


#ifndef NDEBUG
#define RG_ASSERT(condition) (::RG::detail::DebugBreakAssertFunc (condition, true, "Assertion Failed", #condition, { __FILE__, __LINE__, __func__ }))
#define RG_VERIFY(condition) (::RG::detail::DebugBreakAssertFunc (condition, true, "Assertion Failed", #condition, { __FILE__, __LINE__, __func__ }))
#define RG_ERROR(condition) (::RG::detail::DebugBreakAssertFunc (condition, false, "Assertion Failed", #condition, { __FILE__, __LINE__, __func__ }))
#define RG_BREAK() (::RG::detail::DebugBreakFunc ("Debug Break", "", { __FILE__, __LINE__, __func__ }))
#define RG_BREAK_STR(message) (::RG::detail::DebugBreakFunc ("Debug Break", message, { __FILE__, __LINE__, __func__ }))
#elif defined(LOGASSERTS)
#define RG_ASSERT(condition) (::RG::detail::LogAssertFunc (condition, true, "Assertion Failed", #condition, { __FILE__, __LINE__, __func__ }))
#define RG_VERIFY(condition) (::RG::detail::LogAssertFunc (condition, true, "Assertion Failed", #condition, { __FILE__, __LINE__, __func__ }))
#define RG_ERROR(condition) (::RG::detail::LogAssertFunc (condition, false, "Assertion Failed", #condition, { __FILE__, __LINE__, __func__ }))
#define RG_BREAK() (::RG::detail::LogDebugBreakFunc ("Debug Break", "", { __FILE__, __LINE__, __func__ }))
#define RG_BREAK_STR(message) (::RG::detail::LogDebugBreakFunc ("Debug Break", message, { __FILE__, __LINE__, __func__ }))
#else
#define RG_ASSERT(condition)
#define RG_VERIFY(condition) ((bool)(condition))
#define RG_ERROR(condition) ((bool)(condition))
#define RG_BREAK()
#define RG_BREAK_STR(message)
#endif


namespace RG {

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

} // namespace RG


#endif