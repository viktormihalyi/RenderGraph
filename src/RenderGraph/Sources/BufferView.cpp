#include "BufferView.hpp"

#include "VulkanWrapper/ShaderModule.hpp"
#include "VulkanWrapper/ShaderReflection.hpp"

#include <algorithm>

#include "spdlog/spdlog.h"

namespace SR {

static const std::vector<std::unique_ptr<Field>> emptyFieldVector;

class EmptyFieldContainer : public FieldContainer {
public:
    virtual const std::vector<std::unique_ptr<Field>>& GetFields () const { return emptyFieldVector; }
} emptyFields;

const std::array<uint32_t, 8> emptyArraySizeIndexArray { 0, 0, 0, 0, 0, 0, 0, 0 };

const BufferView BufferView::invalidUview (BufferView::Type::Variable, nullptr, 777, 777, 777, emptyArraySizeIndexArray, emptyFields, nullptr);

DummyBufferData dummyBufferData;


BufferView::BufferView (Type                           type,
              uint8_t*                       data,
              uint32_t                       offset,
              uint32_t                       size,
              uint32_t                       nextArraySizeIndex,
              const std::array<uint32_t, 8>& arraySizeIndices,
              const FieldContainer&          parentContainer,
              const std::unique_ptr<Field>&  currentField)
    : type (type)
    , data (data)
    , offset (offset)
    , size (size)
    , nextArraySizeIndex (nextArraySizeIndex)
    , parentContainer (parentContainer)
    , currentField (currentField)
    , arraySizeIndices (arraySizeIndices)
{
}


BufferView::BufferView (const std::shared_ptr<BufferObject>& root, uint8_t* data)
    : BufferView (Type::Variable, data, 0, root->GetFullSize (), 0, emptyArraySizeIndexArray, *root, nullptr)
{
}


BufferView::BufferView (const BufferView& other)
    : type (other.type)
    , data (other.data)
    , offset (other.offset)
    , size (other.size)
    , parentContainer (other.parentContainer)
    , nextArraySizeIndex (other.nextArraySizeIndex)
    , currentField (other.currentField)
    , arraySizeIndices (other.arraySizeIndices)
{
}


uint32_t BufferView::GetOffset () const
{
    return offset;
}


BufferView BufferView::operator[] (std::string_view str)
{
    if (GVK_ERROR (data == nullptr)) {
        return invalidUview;
    }

    GVK_ASSERT (type != Type::Array);

    for (const std::unique_ptr<Field>& f : parentContainer.GetFields ()) {
        if (str == f->name) {
            if (f->IsArray ()) {
                return BufferView (Type::Array, data, offset + f->offset, f->size, 0, emptyArraySizeIndexArray, * f, f);
            } else {
                return BufferView (Type::Variable, data, offset + f->offset, f->size, 0, emptyArraySizeIndexArray, * f, f);
            }
        }
    }

    spdlog::error ("No \"{}\" uniform named on \"{}\".", str, currentField->name);

    GVK_BREAK ();
    return invalidUview;
}


BufferView BufferView::operator[] (uint32_t index)
{
    if (GVK_ERROR (data == nullptr)) {
        return invalidUview;
    }

    GVK_ASSERT (type == Type::Array);
    GVK_ASSERT (currentField != nullptr);
    GVK_ASSERT (currentField->arraySize.size () > 0);

    const uint32_t maxArraySizeIndex = static_cast<uint32_t> (currentField->arraySize.size () - 1);
    if (currentField->IsMultiDimensionalArray () && nextArraySizeIndex < maxArraySizeIndex) {
        const uint32_t newOffset = offset + index * currentField->arrayStride[nextArraySizeIndex];
        const uint32_t newSize   = size;

        BufferView resultView (Type::Array,
                          data,
                          newOffset,
                          newSize,
                          nextArraySizeIndex + 1,
                          arraySizeIndices,
                          parentContainer,
                          currentField);

        resultView.arraySizeIndices[nextArraySizeIndex] = index;

        return resultView;
    }

    // TODO
    return BufferView (Type::Variable, data, offset + index * currentField->arrayStride[nextArraySizeIndex], size, 0, arraySizeIndices, parentContainer, currentField);
}


std::vector<std::string> BufferView::GetFieldNames () const
{
    std::vector<std::string> result;

    for (auto& f : parentContainer.GetFields ()) {
        result.push_back (f->name);
    }

    return result;
}


BufferDataInternal::BufferDataInternal (const std::shared_ptr<BufferObject>& ubo)
    : bytes (ubo->GetFullSize (), 0)
    , root (ubo, bytes.data ())
{
}


BufferView BufferDataInternal::operator[] (std::string_view str)
{
    return root[str];
}


std::vector<std::string> BufferDataInternal::GetNames ()
{
    return root.GetFieldNames ();
}


uint8_t* BufferDataInternal::GetData ()
{
    return bytes.data ();
}


uint32_t BufferDataInternal::GetSize () const
{
    return static_cast<uint32_t> (bytes.size ());
}


BufferDataExternal::BufferDataExternal (const std::shared_ptr<BufferObject>& ubo, uint8_t* bytes, uint32_t size)
    : root (ubo, bytes)
    , bytes (bytes)
    , size (size)
{
    GVK_ASSERT (ubo->GetFullSize () == size);
    memset (bytes, 0, size);
}


BufferView BufferDataExternal::operator[] (std::string_view str)
{
    return root[str];
}


std::vector<std::string> BufferDataExternal::GetNames ()
{
    return root.GetFieldNames ();
}


uint8_t* BufferDataExternal::GetData ()
{
    return bytes;
}


uint32_t BufferDataExternal::GetSize () const
{
    return size;
}


ShaderBufferData::ShaderBufferData (const std::vector<std::shared_ptr<BufferObject>>& ubos)
    : ubos (ubos)
{
    for (auto& a : ubos) {
        udatas.push_back (std::make_unique<BufferDataInternal> (a));
        uboNames.push_back (a->name);
    }
}


IBufferData& ShaderBufferData::operator[] (std::string_view str)
{
    const ptrdiff_t index = std::distance (uboNames.begin (), std::find (uboNames.begin (), uboNames.end (), str));
    return *udatas[index];
}


std::shared_ptr<BufferObject> ShaderBufferData::GetUbo (std::string_view str)
{
    const ptrdiff_t index = std::distance (uboNames.begin (), std::find (uboNames.begin (), uboNames.end (), str));
    return ubos[index];
}

} // namespace SR
