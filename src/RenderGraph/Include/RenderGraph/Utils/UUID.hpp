#ifndef UUID_HPP
#define UUID_HPP

#include <array>
#include <cstddef>
#include <cstring>
#include <string>

#include "RenderGraph/RenderGraphExport.hpp"

namespace RG {

class RENDERGRAPH_DLL_EXPORT UUID {
private:
    std::array<uint8_t, 16> data;

public:
    UUID ();
    UUID (std::nullptr_t);

    std::string GetValue () const;

    bool operator== (const UUID& other) const { return memcmp (data.data (), other.data.data (), 16) == 0; }
    bool operator!= (const UUID& other) const { return memcmp (data.data (), other.data.data (), 16) != 0; }

    friend struct std::hash<UUID>;
};

} // namespace RG


template<>
struct RENDERGRAPH_DLL_EXPORT std::hash<RG::UUID> {
    std::size_t operator() (const RG::UUID& uuid) const noexcept
    {
        const uint64_t firstHalf  = *reinterpret_cast<const uint64_t*> (uuid.data.data ());
        const uint64_t secondHalf = *reinterpret_cast<const uint64_t*> (uuid.data.data () + 8);
        return firstHalf + 31 * secondHalf;
    }
};

#endif