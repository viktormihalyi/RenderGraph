#ifndef UTILS_FILESYSTEMUTILS_HPP
#define UTILS_FILESYSTEMUTILS_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>


namespace Utils {

RENDERGRAPH_DLL_EXPORT
std::optional<std::string> ReadTextFile (const std::filesystem::path& filePath);

RENDERGRAPH_DLL_EXPORT
std::optional<std::vector<char>> ReadBinaryFile (const std::filesystem::path& filePath);

RENDERGRAPH_DLL_EXPORT
bool WriteBinaryFile (const std::filesystem::path& filePath, const std::vector<uint8_t>& data);

RENDERGRAPH_DLL_EXPORT
bool WriteBinaryFile (const std::filesystem::path& filePath, const void* data, size_t size);

RENDERGRAPH_DLL_EXPORT
bool WriteTextFile (const std::filesystem::path& filePath, const std::string&);

RENDERGRAPH_DLL_EXPORT
std::optional<std::vector<uint32_t>> ReadBinaryFile4Byte (const std::filesystem::path& filePath);

RENDERGRAPH_DLL_EXPORT
void EnsureParentFolderExists (const std::filesystem::path& filePath);


} // namespace Utils

#endif