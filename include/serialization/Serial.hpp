#ifndef SERIALIZATION_SERIAL_HPP
#define SERIALIZATION_SERIAL_HPP

#include "ByteSequence.hpp"
#include "Sequentialize.hpp"
#include "concept/Serializable.hpp"

#include <boost/assert.hpp>
#include <boost/concept/assert.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/operators.hpp>

#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

//============================================================================//
namespace serialization {
namespace detail {
//----------------------------------------------------------------------------//

using PackableData = boost::mpl::vector<std::int8_t, std::int16_t, std::int32_t,
        std::int64_t, double>;

//----------------------------------------------------------------------------//

// Just toA remain compatible with C++14
template<typename...>
using void_t = void;

template<typename T, typename = void, typename = void>
struct IsSerializable : std::false_type {
};

template<typename T>
struct IsSerializable<T, void_t<decltype(T::serialize)>,
        void_t<decltype(T::deserialize)>> : std::true_type {
};

//----------------------------------------------------------------------------//

template<typename Sequence, typename Element>
struct ElementIndex {
    enum {value =
          boost::mpl::distance<typename boost::mpl::begin<Sequence>::type,
                  typename boost::mpl::find<Sequence, Element>::type>::value };
};

//----------------------------------------------------------------------------//
} // namespace detail
//============================================================================//

// In order to compare arbitrary types in serial form, we need to prepend all
// raw data in a serial with type tags.
template<typename ...SerializableDataPack>
class Serial : boost::totally_ordered<Serial<SerializableDataPack...>> {
private:
    using SerializableData = boost::mpl::vector<SerializableDataPack...>;

    constexpr static const char CUSTOM_TYPE_OFFSET =
            boost::mpl::size<detail::PackableData>::value;

    static_assert(boost::mpl::size<SerializableData>::value
            + boost::mpl::size<detail::PackableData>::value
            < std::numeric_limits<std::uint8_t>::max(),
            "Too many custom types!");

public:
    Serial() : readOffset(0) {
    }

    Serial(const char* data, const std::size_t& size)
            : byteSequence(data, data + size),
            readOffset(0) {
    }

    Serial(const Serial&) = delete;
    Serial(Serial&&) = default;
    Serial& operator=(const Serial&) = delete;
    Serial& operator=(Serial&&) = default;

    template<typename Packable>
            typename std::enable_if<boost::mpl::contains<detail::PackableData,
                                            Packable>::value, Serial&>::type
            operator<<(const Packable& value) {
        constexpr std::int8_t typeId = detail::ElementIndex<
                detail::PackableData, Packable>::value;
        pack(byteSequence, typeId);
        pack(byteSequence, value);
        return *this;
    }

    template<typename Serializable>
            typename std::enable_if<detail::IsSerializable<Serializable>::value,
            Serial&>::type
            operator<<(const Serializable& serializable) {
        BOOST_CONCEPT_ASSERT((concept::Serializable<Serializable, Serial>));
        constexpr std::int8_t typeId = detail::ElementIndex<
                SerializableData, Serializable>::value + CUSTOM_TYPE_OFFSET;
        constexpr bool hasTypeId = typeId ==
                boost::mpl::size<SerializableData>::type::value
                + CUSTOM_TYPE_OFFSET;
        if (hasTypeId) {
            pack(byteSequence, typeId);
        }
        serializable.serialize(*this);
        return *this;
    }

    template<typename Packable>
            typename std::enable_if<boost::mpl::contains<detail::PackableData,
                                            Packable>::value, Serial&>::type
            operator>>(Packable& value) {
        BOOST_ASSERT_MSG(readOffset + sizeof(value) <= byteSequence.size(),
                "Cannot unpack more data of this type.");
        constexpr std::int8_t typeId = detail::ElementIndex<
                detail::PackableData, Packable>::value;
        std::int8_t unpackedTypeId;
        unpack(byteSequence.begin() + readOffset, unpackedTypeId);
        readOffset += sizeof(unpackedTypeId);
        BOOST_ASSERT_MSG(unpackedTypeId == typeId,
                "Type Id does not match with the expected one.");
        unpack(byteSequence.begin() + readOffset, value);
        readOffset += sizeof(value);
        return *this;
    }

    template<typename Serializable>
            typename std::enable_if<detail::IsSerializable<Serializable>::value,
                Serial&>::type
            operator>>(Serializable& serializable) {
        BOOST_CONCEPT_ASSERT((concept::Serializable<Serializable, Serial>));
        constexpr std::int8_t typeId = detail::ElementIndex<
                SerializableData, Serializable>::value + CUSTOM_TYPE_OFFSET;
        constexpr bool hasTypeId = typeId ==
                boost::mpl::size<SerializableData>::type::value
                + CUSTOM_TYPE_OFFSET;
        if (hasTypeId) {
            std::int8_t unpackedTypeId;
            unpack(byteSequence.begin() + readOffset, unpackedTypeId);
            BOOST_ASSERT_MSG(unpackedTypeId == typeId,
                    "Type Id does not match with the expected one.");
        }
        serializable.deserialize(*this);
        return *this;
    }

    bool operator==(const Serial& rhs) const {
        return memcmp(byteSequence.data(), rhs.byteSequence.data(),
                std::min(byteSequence.size(), rhs.byteSequence.size())) == 0;
    }

    bool operator<(const Serial& rhs) const {
        return memcmp(byteSequence.data(), rhs.byteSequence.data(),
                std::min(byteSequence.size(), rhs.byteSequence.size())) < 0;
    }

private:
    ByteSequence byteSequence; // TODO: move ByteSequence into detail
    std::size_t readOffset;
};

//----------------------------------------------------------------------------//
}  // namespace serialization
//============================================================================//

#endif // SERIALIZATION_SERIAL_HPP
