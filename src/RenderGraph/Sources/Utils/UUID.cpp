#include "UUID.hpp"

#include "Assert.hpp"
#include "CompilerDefinitions.hpp"
#include "Platform.hpp"

#ifdef PLATFORM_WINDOWS
#include <cstring>
#include <rpc.h>

static std::array<uint8_t, 16> GenerateUUID ()
{
    static_assert (sizeof (UUID) == 16);

    UUID uuid;

    RPC_STATUS err = UuidCreate (&uuid);
    GVK_VERIFY (err == RPC_S_OK);

    std::array<uint8_t, 16> result;
    memcpy (result.data (), &uuid, 16);
    return result;
}

#elif (__has_include(<uuid/uuid.h>))

#include <cstring>
#include <uuid/uuid.h>

static std::array<uint8_t, 16> GenerateUUID ()
{
    static_assert (sizeof (uuid_t) == 16);

    uuid_t uuid;
    uuid_generate_random (uuid);

    std::array<uint8_t, 16> result;
    memcpy (result.data (), &uuid, 16);
    return result;
}

#else

#include <cstring>
#include <random>

static std::array<uint8_t, 16> GenerateUUID ()
{
    static std::random_device rd;
    static std::mt19937_64    gen (rd ());

    static std::uniform_int_distribution<uint64_t> dis (
        std::numeric_limits<std::uint64_t>::min (),
        std::numeric_limits<std::uint64_t>::max ());

    std::array<uint8_t, 16> result;
    *reinterpret_cast<uint64_t*> (result.data ())     = dis (gen);
    *reinterpret_cast<uint64_t*> (result.data () + 8) = dis (gen);
    return result;
}

#endif


GVK::UUID::UUID ()
    : data (GenerateUUID ())
{
}


GVK::UUID::UUID (std::nullptr_t)
    : data ({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 })
{
}


std::string GVK::UUID::GetValue () const
{
    char str[39] = {};
    snprintf (str, sizeof (str),
             "{%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
             data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
    return str;
}
