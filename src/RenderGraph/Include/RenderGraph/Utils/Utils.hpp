#ifndef UTILS_HPP
#define UTILS_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include <memory>
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>


namespace RG {

RENDERGRAPH_DLL_EXPORT
std::vector<std::string> SplitString (const std::string& str, const char delim, const bool keepEmpty = false);

RENDERGRAPH_DLL_EXPORT
std::vector<std::string> SplitString (const std::string& str, const std::string& delim, const bool keepEmpty = false);

RENDERGRAPH_DLL_EXPORT
std::string ReplaceAll (const std::string& str, const std::string& substringToReplace, const std::function<std::string ()>& replacementSubstring);

RENDERGRAPH_DLL_EXPORT
bool StringContains (const std::string& str, const std::string& substr);

template<typename SourceType, typename DestType>
std::set<DestType> ToSet (const std::vector<SourceType>& vec)
{
    std::set<DestType> result;
    std::transform (std::begin (vec), std::end (vec), std::inserter (result, std::end (result)), [] (const SourceType& x) {
        return DestType (x);
    });
    return result;
}


template<typename SourceType, typename DestType>
std::set<DestType> ToSet (const std::vector<SourceType>& vec, const std::function<DestType (const SourceType&)>& converter)
{
    std::set<DestType> result;
    std::transform (std::begin (vec), std::end (vec), std::inserter (result, std::end (result)), converter);
    return result;
}


template<typename T>
std::set<T> SetDiff (const std::set<T>& left, const std::set<T>& right)
{
    std::set<T> diff;
    std::set_difference (std::begin (left), std::end (left), std::begin (right), std::end (right), std::inserter (diff, std::end (diff)));
    return diff;
}


template<typename CastedType, typename Container, typename Processor>
void ForEach (Container&& container, Processor&& processor)
{
    if constexpr (std::is_pointer_v<CastedType>) {
        for (auto& elem : container) {
            auto castedElem = dynamic_cast<CastedType> (elem);
            if (castedElem != nullptr) {
                processor (castedElem);
            }
        }
    } else {
        for (auto& elem : container) {
            auto castedElem = std::dynamic_pointer_cast<CastedType> (elem);
            if (castedElem != nullptr) {
                processor (castedElem);
            }
        }
    }
}


} // namespace RG

#endif