#ifndef SERIALIZATION_SEQUENTIALIZE_HPP
#define SERIALIZATION_SEQUENTIALIZE_HPP

#include "Features.hpp"
#include "detail/ByteSequence.hpp"
#include "detail/ConversionMap.hpp"

// Boost.Endian uses compiler intrinsics if available
#include <boost/endian/conversion.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/has_key.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/operators.hpp>
#include <boost/range/iterator_range_core.hpp>

#include <cstdint>
#include <limits>
#include <string>

//============================================================================//
namespace serialization {
namespace detail {
//----------------------------------------------------------------------------//

class PackableByteSequence
        : public boost::totally_ordered<PackableByteSequence> {
public:
    PackableByteSequence() = default;

    PackableByteSequence(const char* data, std::size_t size)
            : byteSequence(reinterpret_cast<const byte*>(data),
                    reinterpret_cast<const byte*>(data) + size) {
    }

    bool operator==(const PackableByteSequence& rhs) const {
        return byteSequence.size() == rhs.byteSequence.size() &&
                std::memcmp(byteSequence.data(), rhs.byteSequence.data(),
                        byteSequence.size()) == 0;
    }

    bool operator<(const PackableByteSequence& rhs) const {
        int difference = std::memcmp(byteSequence.data(), rhs.byteSequence.data(),
                std::min(byteSequence.size(), rhs.byteSequence.size()));
        if (difference == 0) {
            return byteSequence.size() < rhs.byteSequence.size();
        }
        return difference;
    }

protected:
    void appendToSequence(std::string data) {
        appendToSequence(reinterpret_cast<const byte*>(data.c_str()),
                data.size() + 1); // \0 byte
    }

    template <typename PackedValue>
    void appendToSequence(const PackedValue& packedValue) {
        const byte* valueArray = reinterpret_cast<const byte*>(&packedValue);
        byteSequence.insert(byteSequence.end(), valueArray,
                valueArray + sizeof(packedValue));
    }

    template <typename PackedValue>
    PackedValue readFromSequence() {
        return *reinterpret_cast<const PackedValue*>(checkAndGetNextPointer(
                        sizeof(PackedValue)));
    }

private:
    void appendToSequence(const byte* data, std::size_t size) {
        byteSequence.insert(byteSequence.end(),
                reinterpret_cast<const byte*>(data),
                reinterpret_cast<const byte*>(data) + size);
    }

    byte* getNextPointer() {
        return std::addressof(byteSequence[readOffset]);
    }

    byte* checkAndGetNextPointer() {
        byte* nextPointer = getNextPointer();
        std::size_t size = std::strlen(reinterpret_cast<const char*>(
                        nextPointer));
        BOOST_ASSERT_MSG(size <= byteSequence.size() - readOffset,
                "Invalid data in sequence.");
        readOffset += size;
        return nextPointer;
    }

    byte* checkAndGetNextPointer(std::size_t size) {
        BOOST_ASSERT_MSG(size <= byteSequence.size() - readOffset,
                "Cannot unpack more data.");
        byte* nextPointer = getNextPointer();
        readOffset += size;
        return nextPointer;
    }

    ByteSequence byteSequence;
    unsigned readOffset = 0;
};

//----------------------------------------------------------------------------//

template<>
inline std::string PackableByteSequence::readFromSequence() {
    return std::string{reinterpret_cast<const char*>(
                checkAndGetNextPointer())};
}

//----------------------------------------------------------------------------//
} // namespace detail
//============================================================================//

template <typename SequentializationMethod>
class Sequentializer;

//----------------------------------------------------------------------------//

template <>
class Sequentializer<StronglyTypedIntegers>
        : public detail::PackableByteSequence {
private:
    template <typename UnsignedInt>
    constexpr UnsignedInt getOffsetZero() const {
        static_assert(!std::is_signed<UnsignedInt>::value,
                "This is not an unsigned int!");
        return std::numeric_limits<UnsignedInt>::max() -
               std::numeric_limits<
                       typename std::make_signed<UnsignedInt>::type>::max();
    }

    template <typename Int, typename PackedValue>
    constexpr PackedValue getOffset() const {
        return static_cast<PackedValue>(std::numeric_limits<Int>::max()) +
               static_cast<PackedValue>(1);
    }

    template <typename UnsignedInteger, typename Integer>
    UnsignedInteger addOffset(const Integer& value) {
        UnsignedInteger packedValue = 0;

        if (value >= 0) {
            packedValue = static_cast<UnsignedInteger>(value) +
                    getOffset<Integer, UnsignedInteger>();
        } else {
            packedValue = static_cast<UnsignedInteger>(
                    value + getOffset<Integer, UnsignedInteger>());
        }
        return packedValue;
    }

    template <typename UnsignedInteger, typename Integer>
    void shiftAndPackInteger(const Integer& value) {
        UnsignedInteger packedValue = boost::endian::native_to_big(
                addOffset<UnsignedInteger>(value));
        // compressAndAppendToSequence(sequence, packedValue);
        appendToSequence(packedValue);
    }

    template <typename PackedValue, typename Integer>
    std::size_t unpackAndUnshiftInteger(
            const boost::iterator_range<detail::ByteSequence::const_iterator>&
                    range,
            Integer& value) {
        PackedValue packedValue = boost::endian::big_to_native(
                *reinterpret_cast<const PackedValue*>(&*range.begin()));

        if (packedValue >= getOffsetZero<PackedValue>()) {
            value = static_cast<Integer>(
                    packedValue - getOffset<Integer, PackedValue>());
        } else {
            value = static_cast<Integer>(packedValue) -
                    getOffset<Integer, PackedValue>();
        }

        return sizeof(value);
    }

public:
    using PackableByteSequence::PackableByteSequence;

    template <typename Integer>
    void pack(const Integer& value) {
        static_assert(boost::mpl::has_key<detail::ConversionMap, Integer>::value,
                "Cannot pack this type!");
        using PackedValue = typename boost::mpl::at<detail::ConversionMap,
                Integer>::type;
        shiftAndPackInteger<PackedValue>(value);
    }

    // IEEE-754: double floating point representation
    // sign: 1 bit, exponent: 11 bit, fraction: 52 bit
    // It seems to be good as it is now.
    void pack(const double& value) {
        using PackedValue =
                typename boost::mpl::at<detail::ConversionMap, double>::type;
        // trick the first bit :)
        double copiedValue = value * -1;
        PackedValue packedValue =
                *reinterpret_cast<const PackedValue*>(&copiedValue);
        packedValue = boost::endian::native_to_big(packedValue);
        appendToSequence(packedValue);
    }

    void pack(const std::string& value) {
        appendToSequence(value);
    }

    template <typename Integer>
    void unpack(Integer& value) {
        static_assert(boost::mpl::has_key<detail::ConversionMap, Integer>::value,
                "Cannot unpack this type!");
        using PackedValue = typename boost::mpl::at<detail::ConversionMap,
                Integer>::type;

        PackedValue packedValue = boost::endian::big_to_native(
                readFromSequence<PackedValue>());

        if (packedValue >= getOffsetZero<PackedValue>()) {
            value = static_cast<Integer>(packedValue -
                    getOffset<Integer, PackedValue>());
        } else {
            value = static_cast<Integer>(packedValue) -
                    getOffset<Integer, PackedValue>();
        }
    }

    void unpack(double& value) {
        using PackedValue =
                typename boost::mpl::at<detail::ConversionMap, double>::type;

        PackedValue packedValue = boost::endian::big_to_native(
                readFromSequence<PackedValue>());
        value = (*reinterpret_cast<const double*>(&packedValue)) * -1;
    }

    void unpack(std::string& value) {
        value = readFromSequence<std::string>();
    }
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

// TODO: compare all integers regardless of the representation size and in
// the
//       same time compress integers as follows:
//       one type tag for all integers: integer (8, 16, 32 and 64 bits)
//       sign bit: included in size, i.e. a positive number has a size with
//              the first bit set.
//       size: how many bytes are occupied excluding those has only 0 bits.
//              When comparing two numbers and one has a greater size - that
//              means it has a higher bit set then the other -, just
//              comparing
//              the size can be enough to conclude which is the bigger.
//       data: the actual bytes of the integer in big endian.
//

template <typename PackedValue>
void compressAndAppendToSequence(
        detail::ByteSequence& sequence, const PackedValue& packedValue) {
    unsigned char& size = sequence.back();
    const detail::byte* valueArray = reinterpret_cast<const detail::byte*>(&packedValue);
    constexpr unsigned char maxSize =
            static_cast<unsigned char>(sizeof(packedValue));
    for (unsigned char i = 0; i < maxSize; ++i) {
        if (*valueArray > 0) {
            unsigned char offset = maxSize - i;
            sequence.insert(sequence.end(), valueArray, valueArray + offset);
        }
    }
}

//----------------------------------------------------------------------------//

template <typename PackedValue, typename Integer>
void unpackAndUncompressInteger(
        const boost::iterator_range<detail::ByteSequence::const_iterator>&
                range,
        Integer& value) {
    unsigned char* valuePointer = reinterpret_cast<unsigned char*>(&value);
    detail::ByteSequence::const_iterator iterator = range.begin();
    const unsigned char& size = *iterator;
    ++iterator;
    constexpr unsigned char maxSize = static_cast<unsigned char>(sizeof(value));
    unsigned char leadingZeroBytes = maxSize - size;
    PackedValue packedValue;
    unsigned char* valueArray =
            reinterpret_cast<unsigned char*>(&packedValue) + leadingZeroBytes;
    std::copy(iterator, iterator + size, valueArray);
    //value = static_cast<Integer>(packedValue - getOffset<Integer>());
}

//----------------------------------------------------------------------------//
} // namespace serialization
//============================================================================//

#endif // SERIALIZATION_SEQUENTIALIZE_HPP
