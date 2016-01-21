#ifndef SERIALIZATION_SEQUENTIALIZE_HPP
#define SERIALIZATION_SEQUENTIALIZE_HPP

#include "ByteSequence.hpp"
#include "detail/ConversionMap.hpp"

#include <boost/mpl/at.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/range/iterator_range_core.hpp>

#include <cstdint>
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
void appendToSequence(ByteSequence& sequence,
        const PackedValue& packedValue) {
    const byte* valueArray = reinterpret_cast<const byte*>(&packedValue);
    sequence.insert(sequence.end(), valueArray,
            valueArray + sizeof(packedValue));
}

//----------------------------------------------------------------------------//

template<typename PackedValue>
void compressAndAppendToSequence(ByteSequence& sequence,
        const PackedValue& packedValue) {
    unsigned char& size = sequence.back();
    const byte* valueArray = reinterpret_cast<const byte*>(&packedValue);
    constexpr unsigned char maxSize = static_cast<unsigned char>(sizeof(
                    packedValue));
    for (unsigned char i = 0; i < maxSize; ++i) {
        if (*valueArray > 0) {
            unsigned char offset = maxSize - i;
            sequence.insert(sequence.end(), valueArray, valueArray + offset);
        }
    }
}

//----------------------------------------------------------------------------//

template<typename UnsignedInteger, typename Integer>
UnsignedInteger shiftInteger(const Integer& value) {
    UnsignedInteger packedValue = 0;

    if (value >= 0) {
        packedValue = swapBytes<UnsignedInteger>(static_cast<UnsignedInteger>(
                        value) + getOffset<Integer, UnsignedInteger>());
    } else {
        packedValue = swapBytes<UnsignedInteger>(static_cast<UnsignedInteger>(
                        value + getOffset<Integer, UnsignedInteger>()));
    }
    return packedValue;
}

template<typename UnsignedInteger, typename Integer>
void shiftAndPackInteger(ByteSequence& sequence, const Integer& value) {
    UnsignedInteger packedValue = shiftInteger<UnsignedInteger>(value);
    //compressAndAppendToSequence(sequence, packedValue);
    appendToSequence(sequence, packedValue);
}

//----------------------------------------------------------------------------//

// TODO: compare all integers regardless of the representation size and in the
//       same time compress integers as follows:
//       one type tag for all integers: integer (8, 16, 32 and 64 bits)
//       sign bit: included in size, i.e. a positive number has a size with
//              the first bit set.
//       size: how many bytes are occupied excluding those has only 0 bits.
//              When comparing two numbers when one has a greater size, that
//              means it has a higher bit set then the other so comparing the
//              size is enough to conclude which is the bigger.
//       data: the actual bytes of the integer in big endian.
//
template<typename Integer>
void pack(ByteSequence& sequence, const Integer& value) {
    using PackedValue = typename boost::mpl::at<
            detail::ConversionMap, Integer>::type;
    static_assert(!std::is_same<PackedValue,
            boost::mpl::end<detail::ConversionMap>::type>::value,
            "Cannot pack this type!");
    shiftAndPackInteger<PackedValue>(sequence, value);
}

// IEEE-754: double floating point representation
// sign: 1 bit, exponent: 11 bit, fraction: 52 bit
// It seems good as it is now, no reason why to change that
template<>
inline void pack<double>(ByteSequence& sequence, const double& value) {
    using PackedValue = typename boost::mpl::at<detail::ConversionMap,
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

template<typename PackedValue, typename Integer>
std::size_t unpackAndUnshiftInteger(
        const boost::iterator_range<ByteSequence::const_iterator>& range,
        Integer& value) {
    PackedValue packedValue = swapBytes(*reinterpret_cast<const PackedValue*>(
                    &*range.begin()));

    if (packedValue >= getZero<PackedValue>()) {
        value = static_cast<Integer>(packedValue -
                getOffset<Integer, PackedValue>());
    } else {
        value = static_cast<Integer>(packedValue) -
                getOffset<Integer, PackedValue>();
    }

    return sizeof(value);
}

template<typename PackedValue, typename Integer>
void unpackAndUncompressInteger(
        const boost::iterator_range<ByteSequence::const_iterator>& range,
        Integer& value) {
    unsigned char* valuePointer = reinterpret_cast<unsigned char*>(&value);
    ByteSequence::const_iterator iterator = range.begin();
    const unsigned char& size = *iterator;
    ++iterator;
    constexpr unsigned char maxSize = static_cast<unsigned char>(
            sizeof(value));
    unsigned char leadingZeroBytes = maxSize - size;
    PackedValue packedValue;
    unsigned char* valueArray = reinterpret_cast<unsigned char*>(&packedValue)
            + leadingZeroBytes;
    std::copy(iterator, iterator + size, valueArray);
    value = static_cast<Integer>(packedValue - getOffset<Integer>());
}

//----------------------------------------------------------------------------//

template<typename Int>
std::size_t unpack(const ByteSequence::const_iterator& iterator, Int& value) {
    using PackedValue = typename boost::mpl::at<
            detail::ConversionMap, Int>::type;
    static_assert(!std::is_same<PackedValue,
            boost::mpl::end<detail::ConversionMap>::type>::value,
            "Cannot unpack this type!");

    PackedValue packedValue = swapBytes(*reinterpret_cast<const PackedValue*>(
                    &*iterator));

    if (packedValue >= getZero<PackedValue>()) {
        value = static_cast<Int>(packedValue - getOffset<Int, PackedValue>());
    } else {
        value = static_cast<Int>(packedValue) - getOffset<Int, PackedValue>();
    }

    return sizeof(value);
}

// FIXME: inline?
template<>
inline std::size_t unpack<double>(const ByteSequence::const_iterator& iterator,
        double& value) {
    using PackedValue = typename boost::mpl::at<detail::ConversionMap,
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
