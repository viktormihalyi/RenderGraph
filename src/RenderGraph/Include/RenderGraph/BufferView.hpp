#ifndef UNIFORM_VIEW_HPP
#define UNIFORM_VIEW_HPP

#include "RenderGraph/Utils/Assert.hpp"
#include "RenderGraph/Utils/Noncopyable.hpp"
#include <memory>

#include "RenderGraph/RenderGraphExport.hpp"

#include <cstdint>
#include <cstring>
#include <vector>
#include <array>
#include <string_view>


namespace RG {
class ShaderModule;
}


namespace RG {
namespace Refl {

class FieldContainer;
class Field;
class BufferObject;

// view (size and offset) to a single variable (could be primitive, struct, array) in a uniform block
// we can walk down the struct hierarchy with operator[](std::string_view)
// we can select an element in an array with operator[](uint32_t)
// we can set the variable with operator=(T)

class RENDERGRAPH_DLL_EXPORT BufferView final {
public:
    static const BufferView invalidUview;

private:
    enum class Type {
        Variable,
        Array,
    };

    Type                          type;
    uint8_t*                      data;
    uint32_t                      offset;
    uint32_t                      size;
    uint32_t                      nextArraySizeIndex;
    std::array<uint32_t, 8>       arraySizeIndices;
    const FieldContainer&         parentContainer;
    const std::unique_ptr<Field>& currentField;

public:
    BufferView (Type                           type,
           uint8_t*                       data,
           uint32_t                       offset,
           uint32_t                       size,
           uint32_t                       nextArraySizeIndex,
           const std::array<uint32_t, 8>& arraySizeIndices,
           const FieldContainer&          parentContainer,
           const std::unique_ptr<Field>&  currentField = nullptr);

    BufferView (const std::shared_ptr<BufferObject>& root, uint8_t* data);

    BufferView (const BufferView&);

    template<typename T>
    void operator= (const T& other)
    {
        static_assert (sizeof (T) >= 4, "there are no data types in glsl with less than 4 bytes");

        if (RG_ERROR (data == nullptr)) {
            return;
        }

        RG_ASSERT (type == Type::Variable);
        RG_ASSERT (sizeof (T) == size);

        memcpy (data + offset, &other, size);
    }

    uint32_t GetOffset () const;

    BufferView operator[] (std::string_view str);

    BufferView operator[] (uint32_t index);

    std::vector<std::string> GetFieldNames () const;
};


// view to a single BufferObject + properly sized byte array for it
class RENDERGRAPH_DLL_EXPORT IBufferData {
public:
    virtual ~IBufferData () = default;

    template<typename T>
    void operator= (const T& other)
    {
        static_assert (sizeof (T) >= 4, "there are no data types in glsl with less than 4 bytes");

        if (GetData () == nullptr) {
            RG_BREAK ();
            return;
        }

        RG_ASSERT (GetSize () == sizeof (T));
        memcpy (GetData (), &other, GetSize ());
    }

    template<typename T>
    void operator= (const std::vector<T>& other)
    {
        static_assert (sizeof (T) >= 4, "there are no data types in glsl with less than 4 bytes");

        if (GetData () == nullptr) {
            RG_BREAK ();
            return;
        }

        RG_ASSERT (sizeof (T) * other.size () == GetSize ());
        memcpy (GetData (), other.data (), GetSize ());
    }

    bool IsAllZero ()
    {
        if (GetData () == nullptr) {
            RG_BREAK ();
            return true;
        }

        const uint8_t* dataPtr = GetData ();
        const uint32_t size    = GetSize ();

        for (uint32_t byteIndex = 0; byteIndex < size; ++byteIndex) {
            if (dataPtr[byteIndex] != 0) {
                return false;
            }
        }

        return true;
    }

    virtual BufferView                    operator[] (std::string_view str) = 0;
    virtual std::vector<std::string> GetNames ()                       = 0;
    virtual uint8_t*                 GetData ()                        = 0;
    virtual uint32_t                 GetSize () const                  = 0;
};


// view to a single BufferObject + properly sized byte array for it
class RENDERGRAPH_DLL_EXPORT DummyBufferData final : public IBufferData {
public:
    virtual BufferView operator[] (std::string_view) override
    {
        return BufferView::invalidUview;
    }

    virtual std::vector<std::string> GetNames () override
    {
        return {};
    }

    virtual uint8_t* GetData () override
    {
        return nullptr;
    }

    virtual uint32_t GetSize () const override
    {
        return 0;
    }
};


extern RENDERGRAPH_DLL_EXPORT DummyBufferData dummyBufferData;


class RENDERGRAPH_DLL_EXPORT BufferDataExternal final : public IBufferData, public Noncopyable {
private:
    BufferView    root;
    uint8_t* bytes;
    uint32_t size;

public:
    BufferDataExternal (const std::shared_ptr<BufferObject>& ubo, uint8_t* bytes, uint32_t size);

    virtual BufferView operator[] (std::string_view str) override;

    virtual std::vector<std::string> GetNames () override;

    virtual uint8_t* GetData () override;

    virtual uint32_t GetSize () const override;
};


class RENDERGRAPH_DLL_EXPORT BufferDataInternal final : public IBufferData, public Noncopyable {
private:
    std::vector<uint8_t> bytes;
    BufferView                root;

public:
    BufferDataInternal (const std::shared_ptr<BufferObject>& ubo);

    virtual BufferView operator[] (std::string_view str) override;

    virtual std::vector<std::string> GetNames () override;

    virtual uint8_t* GetData () override;

    virtual uint32_t GetSize () const override;
};


// view and byte array for _all_ UBOs in a shader
// we can select a single BufferObject with operator[](std::string_view)

class RENDERGRAPH_DLL_EXPORT ShaderBufferData final : public Noncopyable {
private:
    std::vector<std::unique_ptr<IBufferData>>  udatas;
    std::vector<std::string>              uboNames;
    std::vector<std::shared_ptr<BufferObject>> ubos;

public:
    ShaderBufferData (const std::vector<std::shared_ptr<BufferObject>>& ubos);

    IBufferData& operator[] (std::string_view str);

    std::shared_ptr<BufferObject> GetUbo (std::string_view str);
};


} // namespace Refl
} // namespace RG


#endif

