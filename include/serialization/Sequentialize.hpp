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

static_assert(sizeof(double) == 8, "Extended double precision floating point "
        "numbers are not supported!");

//============================================================================//
namespace serialization {
//----------------------------------------------------------------------------//

using ConversionPackMap = boost::mpl::map<
        boost::mpl::pair<std::int8_t, std::uint8_t>,
        boost::mpl::pair<std::int16_t, std::uint16_t>,
        boost::mpl::pair<std::int32_t, std::uint32_t>,
        boost::mpl::pair<std::int64_t, std::uint64_t>,
        boost::mpl::pair<double, std::uint64_t>>;

//----------------------------------------------------------------------------//

template<typename T>
struct UnableToConvert;

//============================================================================//

template<typename T>
constexpr T getSignShiftValue() {
    return 8 * sizeof(T) - 1;
}

template<typename T>
constexpr T getSignValue() {
    return static_cast<T>(1) << getSignShiftValue<T>();
}

template<typename T>
void addSign(T& value) {
    value |= getSignValue<T>();
}

template<typename T>
bool removeSign(T& value) {
    bool sign = value >> getSignShiftValue<T>();
    value &= getSignValue<T>() - 1;
    return sign;
}

//----------------------------------------------------------------------------//

template<typename T>
T swapBytes(const T& value);

template<>
std::uint8_t swapBytes(const std::uint8_t& value) {
    return value;
}

template<>
std::uint16_t swapBytes(const std::uint16_t& value) {
    return SWAP_U16(value);
}

template<>
std::uint32_t swapBytes(const std::uint32_t& value) {
    return SWAP_U32(value);
}

template<>
std::uint64_t swapBytes(const std::uint64_t& value) {
    return SWAP_U64(value);
}


//----------------------------------------------------------------------------//

template<typename PackedValue>
void appendToSequence(ByteSequence& sequence, const PackedValue& packedValue) {

    const byte* valueArray = reinterpret_cast<const byte*>(&packedValue);
    sequence.insert(sequence.end(), valueArray,
        valueArray + sizeof(PackedValue));
}

//----------------------------------------------------------------------------//

template<typename T>
void pack(ByteSequence& sequence, const T& value) {
    using PackedValue = typename boost::mpl::at<ConversionPackMap, T>::type;

    PackedValue packedValue;
    // Positive numbers get their first bit set, unlike negative numbers.
    if (value > 0) {
        packedValue = swapBytes(static_cast<const PackedValue>(value));
        addSign(packedValue);
    } else {
        packedValue = swapBytes(static_cast<const PackedValue>(value * -1));
    }

    appendToSequence(sequence, packedValue);
}

// IEEE-754: double floating point representation
// sign: 1 bit, exponent: 11 bit, fraction: 52 bit
// It seems good as it is now, no reason why to change that
void pack(ByteSequence& sequence, double value) {
    using PackedValue = typename boost::mpl::at<ConversionPackMap,
            double>::type;

    // trick the first bit :)
    value *= -1;
    PackedValue packedValue = *reinterpret_cast<const PackedValue*>(&value);
    packedValue = swapBytes(packedValue);

    appendToSequence(sequence, packedValue);
}

void pack(ByteSequence& sequence, const std::string& value) {
    const byte* valueArray = reinterpret_cast<const byte*>(value.c_str());
    sequence.insert(sequence.end(), valueArray,
            valueArray + value.length() + 1); // \0 byte
}

//----------------------------------------------------------------------------//

template<typename T>
std::size_t unpack(const ByteSequence::const_iterator& iterator, T& value) {
    using PackedValue = typename boost::mpl::at<ConversionPackMap, T>::type;

    PackedValue packedValue = *reinterpret_cast<const PackedValue*>(
            &*iterator);
    bool hasSign = removeSign(packedValue);
    value = static_cast<T>(swapBytes(packedValue));
    if (!hasSign) {
        // It is a negative number.
        value *= -1;
    }
    return sizeof(value);
}

std::size_t unpack(const ByteSequence::const_iterator& iterator, double& value) {
    using PackedValue = typename boost::mpl::at<ConversionPackMap,
            double>::type;

    PackedValue packedValue =
            swapBytes(*reinterpret_cast<const PackedValue*>(&*iterator));
    value = *reinterpret_cast<const double*>(&packedValue) * -1;
    return sizeof(value);
}

std::size_t unpack(const ByteSequence::const_iterator& iterator,
        std::string& value) {
    const char* valueArray = reinterpret_cast<const char*>(&*iterator);
    value.assign(valueArray);
    return value.length() + 1; // \0 byte
}

//----------------------------------------------------------------------------//
} // namespace serialization
//============================================================================//

#undef SERIALIZATION_IDENTITY
#undef SERIALIZATION_LITTLE_ENDIAN
#undef SERIALIZATION_BIG_ENDIAN
#undef SERIALIZATION_NATIVE_ENDIANNESS
#undef SWAP_U16
#undef SWAP_U32
#undef SWAP_U64

#endif // SERIALIZATION_SEQUENTIALIZE_HPP
