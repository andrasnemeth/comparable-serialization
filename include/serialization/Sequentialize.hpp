#ifndef SERIALIZATION_SEQUENTIALIZE_HPP
#define SERIALIZATION_SEQUENTIALIZE_HPP

#include "ByteSequence.hpp"

#include <boost/mpl/at.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/mpl/placeholders.hpp>

#include <cstdint>
#include <cstring> // remove
#include <limits>
#include <string>

//============================================================================//

#define SERIALIZATON_LITTLE_ENDIAN 0
#define SERIALIZATON_BIG_ENDIAN 1

//============================================================================//

// Mac OS X: <libkern/OSByteOrder.h>

#define SERIALIZATION_IDENTITY(x) x

#ifdef __GNUC__
#  if defined __APPLE__
#    error "OS X is not yet supported!"
#  endif
#  if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#    include "byteswap.h"
#    define SERIALIZATION_NATIVE_ENDIANNESS = SERIALIZATION_LITTLE_ENDIAN
#    define SWAP_U16(x) bswap_16(x)
#    define SWAP_U32(x) bswap_32(x)
#    define SWAP_U64(x) bswap_64(x)
#  elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#    define SERIALIZATION_NATIVE_ENDIANNESS = SERIALIZATION_BIG_ENDIAN
#    define SWAP_U16(x) SERIALIZATION_IDENTITY(x)
#    define SWAP_U16(x) SERIALIZATION_IDENTITY(x)
#    define SWAP_U16(x) SERIALIZATION_IDENTITY(x)
#  else
#    error "CPU architecture not supported!"
#  endif
#else
#  error "Compiler not yet supported!"
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

// in MSVC double is a long double?
// Just to be sure, size of double must be 8 at least until tested.
static_assert(sizeof(double) == 8, "Extended double precision floating point "
        "numbers are not supported!");

//============================================================================//
namespace serialization {
//----------------------------------------------------------------------------//

template<typename Int>
Int swapBytes(const Int& value);

template<>
inline std::uint8_t swapBytes(const std::uint8_t& value) {
    return value;
}

template<>
inline std::uint16_t swapBytes(const std::uint16_t& value) {
    return SWAP_U16(value);
}

template<>
inline std::uint32_t swapBytes(const std::uint32_t& value) {
    return SWAP_U32(value);
}

template<>
inline std::uint64_t swapBytes(const std::uint64_t& value) {
    return SWAP_U64(value);
}

//----------------------------------------------------------------------------//

#undef SERIALIZATION_IDENTITY
#undef SERIALIZATION_LITTLE_ENDIAN
#undef SERIALIZATION_BIG_ENDIAN
#undef SERIALIZATION_NATIVE_ENDIANNESS
#undef SWAP_U16
#undef SWAP_U32
#undef SWAP_U64

//----------------------------------------------------------------------------//

using ConversionPackMap = boost::mpl::map<
        boost::mpl::pair<std::int8_t, std::uint8_t>,
        boost::mpl::pair<std::int16_t, std::uint16_t>,
        boost::mpl::pair<std::int32_t, std::uint32_t>,
        boost::mpl::pair<std::int64_t, std::uint64_t>,
        boost::mpl::pair<double, std::uint64_t>>;

//----------------------------------------------------------------------------//

template<typename Int>
struct UnableToConvert;

//============================================================================//

template<typename UnsignedInt>
constexpr UnsignedInt getZero() {
    static_assert(!std::is_signed<UnsignedInt>::value,
            "This is not an unsigned int!");
    return std::numeric_limits<UnsignedInt>::max() -
        std::numeric_limits<
            typename std::make_signed<UnsignedInt>::type>::max();
}

template<typename Int, typename PackedValue>
constexpr PackedValue getOffset() {
    return static_cast<PackedValue>(std::numeric_limits<Int>::max())
            + static_cast<PackedValue>(1);
}

//----------------------------------------------------------------------------//

template<typename PackedValue>
void appendToSequence(ByteSequence& sequence, const PackedValue& packedValue) {

    const byte* valueArray = reinterpret_cast<const byte*>(&packedValue);
    sequence.insert(sequence.end(), valueArray,
        valueArray + sizeof(PackedValue));
}

//----------------------------------------------------------------------------//

template<typename Int>
void pack(ByteSequence& sequence, const Int& value) {
    using PackedValue = typename boost::mpl::at<ConversionPackMap, Int>::type;
    static_assert(!std::is_same<PackedValue,
            boost::mpl::end<ConversionPackMap>::type>::value,
            "Cannot pack this type!");

    PackedValue packedValue = 0;//getZero<PackedValue>();
    if (value >= 0) {
        packedValue = swapBytes<PackedValue>(static_cast<PackedValue>(value)
                + getOffset<Int, PackedValue>());
    } else {
        packedValue = swapBytes<PackedValue>(static_cast<PackedValue>(value
                + getOffset<Int, PackedValue>()));
    }

    appendToSequence(sequence, packedValue);
}

// IEEE-754: double floating point representation
// sign: 1 bit, exponent: 11 bit, fraction: 52 bit
// It seems good as it is now, no reason why to change that
template<>
inline void pack<double>(ByteSequence& sequence, const double& value) {
    using PackedValue = typename boost::mpl::at<ConversionPackMap,
            double>::type;
    // trick the first bit :)
    double copiedValue = value * -1;
    PackedValue packedValue = *reinterpret_cast<const PackedValue*>(
            &copiedValue);
    packedValue = swapBytes(packedValue);

    appendToSequence(sequence, packedValue);
}

template<>
inline void pack<std::string>(ByteSequence& sequence,
        const std::string& value) {
    const byte* valueArray = reinterpret_cast<const byte*>(value.c_str());
    sequence.insert(sequence.end(), valueArray,
            valueArray + value.length() + 1); // \0 byte
}

//----------------------------------------------------------------------------//

template<typename Int>
std::size_t unpack(const ByteSequence::const_iterator& iterator, Int& value) {
    using PackedValue = typename boost::mpl::at<ConversionPackMap, Int>::type;
    static_assert(!std::is_same<PackedValue,
            boost::mpl::end<ConversionPackMap>::type>::value,
            "Cannot pack this type!");

    PackedValue packedValue = swapBytes(*reinterpret_cast<const PackedValue*>(
                    &*iterator));

    if (packedValue >= getZero<PackedValue>()) {
        value = static_cast<Int>(packedValue - getOffset<Int, PackedValue>());
    } else {
        value = static_cast<Int>(packedValue - getOffset<Int, PackedValue>());
    }

    return sizeof(value);
}

template<>
inline std::size_t unpack<double>(const ByteSequence::const_iterator& iterator,
        double& value) {
    using PackedValue = typename boost::mpl::at<ConversionPackMap,
            double>::type;

    PackedValue packedValue =
            swapBytes(*reinterpret_cast<const PackedValue*>(&*iterator));
    value = (*reinterpret_cast<const double*>(&packedValue)) * -1;
    return sizeof(value);
}

template<>
inline std::size_t unpack<std::string>(
        const ByteSequence::const_iterator& iterator, std::string& value) {
    const char* valueArray = reinterpret_cast<const char*>(&*iterator);
    value.assign(valueArray);
    return value.length() + 1; // \0 byte
}

//----------------------------------------------------------------------------//
} // namespace serialization
//============================================================================//

#endif // SERIALIZATION_SEQUENTIALIZE_HPP
