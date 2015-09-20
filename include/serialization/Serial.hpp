#ifndef SERIALIZATION_SERIAL_HPP
#define SERIALIZATION_SERIAL_HPP

#include "ByteSequence.hpp"
#include "Sequentialize.hpp"
#include "concept/Serializable.hpp"

#include <boost/assert.hpp>
#include <boost/concept/assert.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/deque.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/operators.hpp>

#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>

//============================================================================//
namespace serialization {
namespace detail {
//----------------------------------------------------------------------------//

using PackableData = boost::mpl::vector<std::int8_t, std::int16_t, std::int32_t,
        std::int64_t, double, std::string>;

//----------------------------------------------------------------------------//

// Just to remain compatible with C++14.
template<typename...>
using void_t = void;

//----------------------------------------------------------------------------//

template<typename T, typename = void, typename = void>
struct IsSerializable : std::false_type {
};

template<typename Serializable>
struct IsSerializable<Serializable,
        void_t<decltype(Serializable::serialize)>,
        void_t<decltype(Serializable::deserialize)>>
        : std::true_type {
};

//----------------------------------------------------------------------------//

template<typename T, typename = void, typename = void>
struct IsSerializableNonIntrusive : std::false_type {
};

template<typename Serializable>
struct IsSerializableNonIntrusive<Serializable,
        void_t<decltype(serialize(std::declval<Serializable>()))>,
        void_t<decltype(deserialize(std::declval<Serializable>()))>>
        : std::true_type {
};

//----------------------------------------------------------------------------//

template<typename T, typename = void>
struct IsPackable : std::false_type {
};

template<typename Packable>
struct IsPackable<Packable,
        void_t<typename boost::mpl::contains<PackableData, Packable>::type>>
        : std::true_type {
};

//----------------------------------------------------------------------------//

template<typename Sequence, typename Element>
struct ElementIndex {
    enum { value =
           boost::mpl::distance<typename boost::mpl::begin<Sequence>::type,
                   typename boost::mpl::find<Sequence, Element>::type>::value };
};

//----------------------------------------------------------------------------//

template<typename T>
struct IsMplSequence : std::false_type {
};

template<typename ...Ts>
struct IsMplSequence<boost::mpl::vector<Ts...>> : std::true_type {
};

template<typename ...Ts>
struct IsMplSequence<boost::mpl::set<Ts...>> : std::true_type {
};

template<typename ...Ts>
struct IsMplSequence<boost::mpl::list<Ts...>> : std::true_type {
};

template<typename ...Ts>
struct IsMplSequence<boost::mpl::deque<Ts...>> : std::true_type {
};

//----------------------------------------------------------------------------//
} // namespace detail
//============================================================================//

// In order to compare arbitrary types in serial form, we need to prepend all
// raw data in a serial with type tags.
template<typename SerializableData = boost::mpl::vector<>>
class Serial : boost::totally_ordered<Serial<SerializableData>> {
private:
    static_assert(detail::IsMplSequence<SerializableData>::value,
            "Template argument must be an mpl sequence!");

    static_assert(boost::mpl::size<SerializableData>::value
            + boost::mpl::size<detail::PackableData>::value
            < std::numeric_limits<std::int8_t>::max(),
            "Too many custom types!");

    constexpr static const std::uint8_t CUSTOM_TYPE_OFFSET =
            boost::mpl::size<detail::PackableData>::value;

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
    typename std::enable_if<detail::IsPackable<Packable>::value, Serial&>::type
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

    // Rather let go in here even if the user does not provide any mean to
    // serialize the supplied type. A meaningful error message will be shown
    // instead.
    template<typename Serializable>
    typename std::enable_if<
            //detail::IsSerializableNonIntrusive<Serializable>::value &&
            !detail::IsPackable<Serializable>::value &&
            !detail::IsSerializable<Serializable>::value,
            Serial&>::type
    operator<<(Serializable& serializable) {
        static_assert(detail::IsSerializableNonIntrusive<Serializable>::value,
                "Don't know how to serialize T. Provide "
                "'Serial& serialize(const T&, Serial&)' or make it "
                "'Serializable'!");
        constexpr std::uint8_t typeId = detail::ElementIndex<
                SerializableData, Serializable>::value + CUSTOM_TYPE_OFFSET;
        constexpr bool hasTypeId = typeId ==
                boost::mpl::size<SerializableData>::type::value
                + CUSTOM_TYPE_OFFSET;
        if (hasTypeId) {
            pack(byteSequence, typeId);
        }
        serialize(serializable, *this); // TODO: eliminate duplications
        return *this;
    }

    template<typename Packable>
    typename std::enable_if<detail::IsPackable<Packable>::value, Serial&>::type
    operator>>(Packable& value) {
        BOOST_ASSERT_MSG(readOffset + sizeof(value) <= byteSequence.size(),
                "Cannot unpack more data of this type.");
        constexpr std::int8_t typeId = detail::ElementIndex<
                detail::PackableData, Packable>::value;
        std::int8_t unpackedTypeId;
        safeUnpack(unpackedTypeId);
        BOOST_ASSERT_MSG(unpackedTypeId == typeId,
                "Type Id does not match with the expected one.");
        safeUnpack(value);
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
            safeUnpack(unpackedTypeId);
            BOOST_ASSERT_MSG(unpackedTypeId == typeId,
                    "Type Id does not match with the expected one.");
        }
        serializable.deserialize(*this);
        return *this;
    }

    // Rather let go in here even if the user does not provide any mean to
    // serialize the supplied type. A meaningful error message will be shown
    // instead.
    template<typename Serializable>
    typename std::enable_if<
            //detail::IsSerializableNonIntrusive<Serializable>::value &&
            !detail::IsPackable<Serializable>::value &&
            !detail::IsSerializable<Serializable>::value,
            Serial&>::type
    operator>>(Serializable& serializable) {
        static_assert(detail::IsSerializableNonIntrusive<Serializable>::value,
                "Don't know how to serialize T. Provide "
                "'Serial& serialize(const T&, Serial&)' or make it "
                "'Serializable'!");
        BOOST_CONCEPT_ASSERT((concept::Serializable<Serializable, Serial>));
        constexpr std::int8_t typeId = detail::ElementIndex<
                SerializableData, Serializable>::value + CUSTOM_TYPE_OFFSET;
        constexpr bool hasTypeId = typeId ==
                boost::mpl::size<SerializableData>::type::value
                + CUSTOM_TYPE_OFFSET;
        if (hasTypeId) {
            std::int8_t unpackedTypeId;
            safeUnpack(unpackedTypeId);
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
    template<typename Value>
    void safeUnpack(Value& value) {
        std::size_t bytesProcessed =
            unpack(byteSequence.begin() + readOffset, value);
        readOffset += bytesProcessed;
    }

    ByteSequence byteSequence; // TODO: move ByteSequence into detail
    std::size_t readOffset;
};

//----------------------------------------------------------------------------//
}  // namespace serialization
//============================================================================//

#endif // SERIALIZATION_SERIAL_HPP
