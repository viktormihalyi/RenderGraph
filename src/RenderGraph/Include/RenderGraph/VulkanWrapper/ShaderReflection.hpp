#ifndef SHADERREFLECTION_HPP
#define SHADERREFLECTION_HPP

#include "RenderGraph/RenderGraphExport.hpp"
#include "RenderGraph/Utils/Noncopyable.hpp"
#include <memory>

#include <cstdint>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>


namespace RG {
namespace Refl {


// clang-format off

enum class FieldType : uint16_t {
    // Scalars
    Bool,
    Int,
    Uint,
    Float,
    Double,
    i64,
    u64,

    // vectors
    Bvec2, Ivec2, Uvec2, Vec2, Dvec2, i64_vec2, u64_vec2,
    Bvec3, Ivec3, Uvec3, Vec3, Dvec3, i64_vec3, u64_vec3,
    Bvec4, Ivec4, Uvec4, Vec4, Dvec4, i64_vec4, u64_vec4,

    // float matrices
    Mat2x2, Mat2x3, Mat2x4,
    Mat3x2, Mat3x3, Mat3x4,
    Mat4x2, Mat4x3, Mat4x4,

    // double matrices
    Dmat2x2, Dmat2x3, Dmat2x4,
    Dmat3x2, Dmat3x3, Dmat3x4,
    Dmat4x2, Dmat4x3, Dmat4x4,

    // ...
    Image,
    Struct,
    Unknown
};

// clang-format on

RENDERGRAPH_DLL_EXPORT
std::string FieldTypeToString (FieldType fieldType);


class Field;

class RENDERGRAPH_DLL_EXPORT FieldContainer {
public:
    virtual ~FieldContainer () = default;

    virtual const std::vector<std::unique_ptr<Field>>& GetFields () const = 0;
};


class RENDERGRAPH_DLL_EXPORT Field final : public FieldContainer, public Noncopyable {
public:
    std::string name;
    FieldType   type;

    // with respect to parent struct
    uint32_t offset;

    // 0 for structs
    // single element size for arrays
    // otherwise the number of bytes a variable takes up (eg. 4 for floats)
    uint32_t size;

    std::vector<uint32_t> arraySize;
    std::vector<uint32_t> arrayStride;

    std::vector<std::unique_ptr<Field>> structFields; // when type == FieldType::Struct

    Field ();

    bool IsArray () const;
    
    bool IsFixedSizeArray () const;

    bool IsMultiDimensionalArray () const;

    bool IsStruct () const;

    uint32_t GetSize () const; // returns 0 for undefined size arrays

    virtual const std::vector<std::unique_ptr<Field>>& GetFields () const override;
};


class RENDERGRAPH_DLL_EXPORT BufferObject final : public FieldContainer, public Noncopyable {
public:
    uint32_t                            binding;
    uint32_t                            descriptorSet;
    std::string                         name;
    std::vector<std::unique_ptr<Field>> fields;
    bool                                hasDeclaredStructSize;
    
    bool HasFixedSize () const; // contains an undefined size array

    uint32_t GetFullSize () const; // returns 0 for undefined sized structs

    virtual const std::vector<std::unique_ptr<Field>>& GetFields () const override;
};


class RENDERGRAPH_DLL_EXPORT Sampler final {
public:
    enum class Type {
        Sampler1D,
        Sampler2D,
        Sampler3D,
        SamplerCube
    };

    std::string name;
    uint32_t    binding;
    uint32_t    descriptorSet;
    Type        type;
    uint32_t    arraySize; // 1 for non-arrays, 0 for undefined size. multidimensional arrays are flattened
};


class RENDERGRAPH_DLL_EXPORT Output {
public:
    std::string   name;
    uint32_t      location;
    FieldType     type;
    uint32_t      arraySize; // 1 for non-arrays, 0 for undefined size. multidimensional arrays are flattened
};


class RENDERGRAPH_DLL_EXPORT Input {
public:
    std::string   name;
    uint32_t      location;
    FieldType     type;
    uint32_t      arraySize; // 1 for non-arrays, 0 for undefined size. multidimensional arrays are flattened
    uint32_t      sizeInBytes;
};


class RENDERGRAPH_DLL_EXPORT SubpassInput {
public:
    std::string name;
    uint32_t    binding;
    uint32_t    subpassIndex;
    FieldType   type;
    uint32_t    arraySize; // 1 for non-arrays, 0 for undefined size. multidimensional arrays are flattened
};


// constructing a spirv_cross::Compiler is expensive
class RENDERGRAPH_DLL_EXPORT SpirvParser {
public:

    struct Impl; // hide spirv_cross::Compiler

    std::unique_ptr<Impl> impl;

    SpirvParser (const std::vector<uint32_t>& binary);
    ~SpirvParser ();
};


RENDERGRAPH_DLL_EXPORT
std::vector<std::shared_ptr<BufferObject>> GetUBOsFromBinary (SpirvParser& compiler);

RENDERGRAPH_DLL_EXPORT
std::vector<std::shared_ptr<BufferObject>> GetStorageBuffersFromBinary (SpirvParser& compiler);

RENDERGRAPH_DLL_EXPORT
std::vector<Sampler> GetSamplersFromBinary (SpirvParser& compiler);

RENDERGRAPH_DLL_EXPORT
std::vector<SubpassInput> GetSubpassInputsFromBinary (SpirvParser& compiler);

RENDERGRAPH_DLL_EXPORT
std::vector<Input> GetInputsFromBinary (SpirvParser& compiler);

RENDERGRAPH_DLL_EXPORT
std::vector<Output> GetOutputsFromBinary (SpirvParser& compiler);

RENDERGRAPH_DLL_EXPORT
VkFormat FieldTypeToVkFormat (FieldType fieldType);


} // namespace Refl
} // namespace RG

#endif