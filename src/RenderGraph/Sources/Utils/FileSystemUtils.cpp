#include "FileSystemUtils.hpp"
#include "Assert.hpp"
#include "BuildType.hpp"

#include <cstring>
#include <fstream>
#include <string>


namespace RG {


template<typename T>
static T ReadOpenedFile (std::ifstream& file)
{
    if (RG_ERROR (!file.is_open ())) {
        return T ();
    }

    file.seekg (0, std::ios::end);
    T result;
    result.reserve (file.tellg ());
    file.seekg (0, std::ios::beg);

    result.assign (std::istreambuf_iterator<char> (file),
                   std::istreambuf_iterator<char> ());

    return result;
}


bool WriteBinaryFile (const std::filesystem::path& filePath, const std::vector<uint8_t>& data)
{
    return WriteBinaryFile (filePath, static_cast<const void*> (data.data ()), data.size ());
}


void EnsureParentFolderExists (const std::filesystem::path& filePath)
{
    if (!std::filesystem::exists (filePath.parent_path ())) {
        std::filesystem::create_directories (filePath.parent_path ());
    }
}


bool WriteBinaryFile (const std::filesystem::path& filePath, const void* data, size_t size)
{
    EnsureParentFolderExists (filePath);

    std::ofstream file (filePath, std::ios::out | std::ios::binary);

    if (!file.is_open ()) {
        return false;
    }

    file.write (reinterpret_cast<const char*> (data), size);

    return true;
}


bool WriteTextFile (const std::filesystem::path& filePath, const std::string& text)
{
    EnsureParentFolderExists (filePath);

    std::ofstream file (filePath, std::ios::out | std::ios::binary);

    if (!file.is_open ()) {
        return false;
    }

    file << text;

    return true;
}


template<typename T>
static std::optional<T> OpenAndReadFile (const std::filesystem::path& filePath)
{
    std::ifstream file (filePath, std::ios::binary);

    if (!file.is_open ()) {
        return std::nullopt;
    }

    T result = ReadOpenedFile<T> (file);

    if (RG_ERROR (file.fail ())) {
        return std::nullopt;
    }

    return result;
}


std::optional<std::string> ReadTextFile (const std::filesystem::path& filePath)
{
    return OpenAndReadFile<std::string> (filePath);
}


std::optional<std::vector<char>> ReadBinaryFile (const std::filesystem::path& filePath)
{
    return OpenAndReadFile<std::vector<char>> (filePath);
}


std::optional<std::vector<uint32_t>> ReadBinaryFile4Byte (const std::filesystem::path& filePath)
{
    const std::optional<std::vector<char>>
        readResult = OpenAndReadFile<std::vector<char>> (filePath);
    if (!readResult.has_value ()) {
        return std::nullopt;
    }

    RG_ASSERT (readResult->size () % 4 == 0);
    size_t binarySize = readResult->size () / 4;
    while (binarySize % 4 != 0) {
        ++binarySize;
    }
    std::vector<uint32_t> result (binarySize);
    memset (result.data (), 0, result.size () * sizeof (uint32_t));

    memcpy (result.data (), readResult->data (), readResult->size ());

    return result;
}

} // namespace RG
