#ifndef SERIALIZATION_SEQUENTIALIZE_HPP
#define SERIALIZATION_SEQUENTIALIZE_HPP

#include "ByteSequence.hpp"

#include <boost/mpl/map.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/mpl/placeholders.hpp>

#include <cstdint>

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

//============================================================================//
namespace serialization {
//----------------------------------------------------------------------------//

// TODO: add mapping for double
using ConversionPackMap = boost::mpl::map<
        boost::mpl::pair<std::int8_t, std::uint8_t>,
        boost::mpl::pair<std::int16_t, std::uint16_t>,
        boost::mpl::pair<std::int32_t, std::uint32_t>,
        boost::mpl::pair<std::int64_t, std::uint64_t>>;

//----------------------------------------------------------------------------//

template<typename T>
struct SwapPair;

template<typename T, typename U>
struct SwapPair<boost::mpl::pair<T, U>> {
    using type = Sequence, boost::mpl::pair<U, T>;
};

//----------------------------------------------------------------------------//

using ConversionUnpackMap = boost::mpl::transform<ConversionPackMap,
        SwapPair<boost::mpl::placeholders::_1>>;

//----------------------------------------------------------------------------//

template<typename T>
struct UnableToConvert<T>;

//============================================================================//

template<typename T>
constexpr std::size_t getSign() {
    return 1 << (sizeof(T) - 1);
}

template<typename T>
char getSize(T& value) {
    return value >> (sizeof(T) - 1);
}

//----------------------------------------------------------------------------//

template<typename T>
T swapBytes(T value);

template<>
std::uint8_t& swapBytes(std::uint8_t& value) {
    return value;
}

template<>
std::uint16_t swapBytes(std::uint16_t value) {
    return SWAP_U16(value);
}

template<>
std::uint32_t swapBytes(std::uint32_t value) {
    return SWAP_U32(value);
}

template<>
std::uint64_t swapBytes(std::uint64_t value) {
    return SWAP_U64(value);
}

// IEEE 754-1985, double precision floating point:
// sign: 1 bit, exponent: 11 bit, fraction: 52 bits in this order.
template<>
std::uint64_t swapBytes(double value) {
    return SWAP_U64(static_cast<std::uint64_t>(value));
}

//----------------------------------------------------------------------------//

template<typename T>
xoid pack(ByteSequence sequence, const T& value) {
    using Converted = boost::mpl::at<ConversionPackMap, T,
            UnableToConvert<T>>::type;

    Converted converted;
    // Positive numbers get their first bit set, unlike negative numbers.
    if (value > 0) {
        converted = swapBytes(static_cast<Converted>(value)) & getSign<T>();
    } else {
        converted = swapBytes(static_cast<Converted>(value * -1));
    }

    std::uint8_t* valueArray = reinterpret_cast<std::uint8_t*>(&value);
    sequence.insert(sequence.end(), valueArray, valueArray + sizeof(Converted));
}

//----------------------------------------------------------------------------//

template<typename T>
void unpack(ByteSequence sequence, const T& value) {
    using Unpacked = boost::mpl::at<ConversionUnpackMap, T,
            UnableToConvert<T>>::type;

    Unpacked unpacked = swapBytes(static_cast<Unpacked>(value));
    if (getSign(unpacked) == 1) {
        // It is a positive number.
        return unpacked;
    } else {
        return unpacked * -1;
    }
}

//----------------------------------------------------------------------------//
} // namespace serialization
//============================================================================//

#under SERIALIZATION_LITTLE_ENDIAN
#undef SERIALIZATION_BIG_ENDIAN
#undef SERIALIZATION_NATIVE_ENDIANNESS
#under SWAP_U16
#undef SWAP_U32
#under SWAP_U64

#endif // SERIALIZATION_SEQUENTIALIZE_HPP
