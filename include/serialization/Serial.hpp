#ifndef SERIALIZATION_SERIAL_HPP
#define SERIALIZATION_SERIAL_HPP

#include "ByteSequence.hpp"
#include "Sequentialize.hpp"
#include "concept/Serializable.hpp"
#include "detail/TypeTraits.hpp"

#include <boost/assert.hpp>
#include <boost/concept/assert.hpp>
#include <boost/mpl/size.hpp>
#include <boost/operators.hpp>
#include <boost/tti/has_member_function.hpp>

#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>

//============================================================================//
namespace serialization {
//----------------------------------------------------------------------------//

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

    constexpr static std::int8_t CUSTOM_TYPE_OFFSET =
            boost::mpl::size<detail::PackableData>::value;

public:
    Serial() = default;

    Serial(const char* data, const std::size_t& size)
            : byteSequence{data, data + size},
              readOffset{0} {
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
    typename std::enable_if<detail::IsSerializable<Serializable,
            SerializableData>::value, Serial&>::type
    operator<<(const Serializable& serializable) {
        BOOST_CONCEPT_ASSERT((concept::Serializable<Serializable, Serial>));
        packTypeId<Serializable>();
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
            !detail::IsSerializable<Serializable, SerializableData>::value,
            Serial&>::type
    operator<<(const Serializable& serializable) {
        static_assert(detail::IsSerializableNonIntrusive<Serializable,
                SerializableData>::value,
                "Don't know how to serialize T. Provide the free function "
                "'void serialize(const T&, Serial&)' or the member "
                "'void T::serialize(Serial&) const'!");
        packTypeId<Serializable>();
        serialize(serializable, *this); // TODO: eliminate duplications
        return *this;
    }

    template<typename Packable>
    typename std::enable_if<detail::IsPackable<Packable>::value, Serial&>::type
    operator>>(Packable& value) {
        constexpr std::int8_t typeId = detail::ElementIndex<
                detail::PackableData, Packable>::value;
        std::int8_t unpackedTypeId = -1;
        safeUnpack(unpackedTypeId);
        BOOST_ASSERT_MSG(unpackedTypeId == typeId,
                "Type Id does not match with the expected one.");
        safeUnpack(value);
        return *this;
    }

    template<typename Serializable>
    typename std::enable_if<detail::IsSerializable<Serializable,
            SerializableData>::value, Serial&>::type
    operator>>(Serializable& serializable) {
        BOOST_CONCEPT_ASSERT((concept::Serializable<Serializable, Serial>));
        unpackTypeId<Serializable>();
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
            !detail::IsSerializable<Serializable, SerializableData>::value,
            Serial&>::type
    operator>>(Serializable& serializable) {
        static_assert(detail::IsSerializableNonIntrusive<Serializable,
                SerializableData>::value,
                "Don't know how to deserialize T. Provide the free function "
                "'void deserialize(const T&, Serial&)' or the member "
                "'void T::deserialize(Serial&)'!");
        unpackTypeId<Serializable>();
        deserialize(serializable, *this);
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
        ByteSequence::const_iterator readIterator = byteSequence.cbegin();
        std::advance(readIterator, readOffset);
        BOOST_ASSERT_MSG(readOffset + sizeof(value) <= byteSequence.size(),
                "Cannot unpack more data of this type.");
        std::size_t bytesProcessed = unpack(readIterator, value);
        readOffset += bytesProcessed;
    }

    template<typename Serializable>
    void packTypeId() {
        constexpr bool hasTypeId = boost::mpl::contains<SerializableData,
                Serializable>::value;
        if (hasTypeId) { // no constexpr if
            constexpr std::int8_t typeId = detail::ElementIndex<
                    SerializableData, Serializable>::value + CUSTOM_TYPE_OFFSET;
            pack(byteSequence, typeId);
        }
    }

    template<typename Serializable>
    void unpackTypeId() {
        constexpr bool hasTypeId = boost::mpl::contains<SerializableData,
                Serializable>::value;
        if (hasTypeId) { // no constexpr if
            constexpr std::int8_t typeId = detail::ElementIndex<
                    SerializableData, Serializable>::value + CUSTOM_TYPE_OFFSET;
            std::int8_t unpackedTypeId = -1;
            safeUnpack(unpackedTypeId);
            BOOST_ASSERT_MSG(unpackedTypeId == typeId,
                    "Type Id does not match with the expected one.");
        }
    }

    ByteSequence byteSequence; // TODO: move ByteSequence into detail
    std::size_t readOffset = 0;
};

//----------------------------------------------------------------------------//
}  // namespace serialization
//============================================================================//

#endif // SERIALIZATION_SERIAL_HPP
