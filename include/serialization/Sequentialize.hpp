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
    value -= getSignValue<T>();
    return sign;
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
        packedValue = swapBytes(static_cast<PackedValue>(value));
        addSign(packedValue);
    } else {
        packedValue = swapBytes(static_cast<PackedValue>(value * -1));
    }

    appendToSequence(sequence, packedValue);
    // const byte* valueArray = reinterpret_cast<const byte*>(&packedValue);
    // sequence.insert(sequence.end(), valueArray,
    //         valueArray + sizeof(PackedValue));
}

// IEEE-754: double floating point representation
// sign: 1 bit, exponent: 11 bit, fraction: 52 bit
void pack(ByteSequence& sequence, double value) {
    using PackedValue = typename boost::mpl::at<ConversionPackMap,
            double>::type;

    // trick the first bit :)
    value *= -1;
    PackedValue packedValue = *reinterpret_cast<const std::uint64_t*>(&value);
    packedValue = swapBytes(packedValue);

    appendToSequence(sequence, packedValue);
}

//----------------------------------------------------------------------------//

template<typename T>
void unpack(const ByteSequence::iterator& iterator, T& value) {
    using PackedValue = typename boost::mpl::at<ConversionPackMap, T>::type;

    PackedValue* packedValue = reinterpret_cast<PackedValue*>(&*iterator);
    bool hasSign = removeSign(*packedValue);
    value = static_cast<T>(swapBytes(*packedValue));
    if (!hasSign) {
        // It is a negative number.
        value *= -1;
    }
}

void unpack(const ByteSequence::iterator& iterator, double& value) {
    using PackedValue = typename boost::mpl::at<ConversionPackMap,
            double>::type;

    PackedValue packedValue = swapBytes(*reinterpret_cast<PackedValue*>(
                    &*iterator));
    value = *reinterpret_cast<double*>(&packedValue) * -1;
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
