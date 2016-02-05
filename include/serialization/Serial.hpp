#ifndef SERIALIZATION_SERIAL_HPP
#define SERIALIZATION_SERIAL_HPP

#include "Features.hpp"
#include "Sequentialize.hpp"
#include "concept/Serializable.hpp"
#include "detail/ByteSequence.hpp"
#include "detail/Optional.hpp"
#include "detail/TypeTraits.hpp"

#include <boost/assert.hpp>
#include <boost/concept/assert.hpp>
#include <boost/mpl/size.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <boost/tti/has_member_function.hpp>

#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>

//============================================================================//
namespace serialization {
namespace detail {
//----------------------------------------------------------------------------//

template <typename T, typename SerializableData>
constexpr detail::Optional<std::int8_t> getTypeId(
        const std::size_t customTypeOffset) {
    constexpr bool isPackable = detail::IsPackable<T>::value;
    constexpr bool isTypeRegistered = boost::mpl::contains<SerializableData,
            T>::value;
    if (isPackable) {
        return detail::ElementIndex<detail::PackableData, T>::value;
    } else if (isTypeRegistered) {
        return detail::ElementIndex<SerializableData, T>::value +
                customTypeOffset;
    }
    return detail::none;
}

//----------------------------------------------------------------------------//
} // nemspace detail
//============================================================================//


// In order to compare arbitrary types in serial form, we need to prepend all
// raw data in a serial with type tags.
template <typename SerializableData = boost::mpl::vector<>,
        typename IntegerFeature = StronglyTypedIntegers>
class Serial : public Sequentializer<IntegerFeature> {
private:
    static_assert(detail::IsMplSequence<SerializableData>::value,
            "Template argument must be an mpl sequence!");

    static_assert(
            boost::mpl::size<SerializableData>::value +
                            boost::mpl::size<detail::PackableData>::value <
                    std::numeric_limits<std::int8_t>::max(),
            "Too many custom types!");

    constexpr static std::int8_t CUSTOM_TYPE_OFFSET =
            boost::mpl::size<detail::PackableData>::value;

public:
    Serial() = default;

    using Sequentializer<IntegerFeature>::Sequentializer;

    Serial(const Serial&) = delete;
    Serial(Serial&&) = default;
    Serial& operator=(const Serial&) = delete;
    Serial& operator=(Serial&&) = default;

    template <typename Packable>
    typename std::enable_if<detail::IsPackable<Packable>::value, Serial&>::type
    operator<<(const Packable& value) {
        packTypeId<Packable>();
        this->pack(value);
        return *this;
    }

    template <typename Serializable>
    typename std::enable_if<
            detail::IsSerializable<Serializable, SerializableData,
                    IntegerFeature>::value, Serial&>::type
    operator<<(const Serializable& serializable) {
        BOOST_CONCEPT_ASSERT((concept::Serializable<Serializable, Serial>));
        packTypeId<Serializable>();
        serializable.serialize(*this);
        return *this;
    }

    // Rather let go in here even if the user does not provide any mean to
    // serialize the supplied type. A meaningful error message will be shown
    // instead.
    template <typename Serializable>
    typename std::enable_if<
            // detail::IsSerializableNonIntrusive<Serializable>::value &&
            !detail::IsPackable<Serializable>::value &&
                    !detail::IsSerializable<Serializable,
                            SerializableData, IntegerFeature>::value,
            Serial&>::type
    operator<<(const Serializable& serializable) {
        static_assert(detail::IsSerializableNonIntrusive<Serializable,
                SerializableData, IntegerFeature>::value,
                "Don't know how to serialize T. Provide the free function "
                "'void serialize(const T&, Serial&)' or the member "
                "'void T::serialize(Serial&) const'!");
        packTypeId<Serializable>();
        serialize(serializable, *this); // TODO: eliminate duplications
        return *this;
    }

    template <typename Packable>
    typename std::enable_if<detail::IsPackable<Packable>::value, Serial&>::type
    operator>>(Packable& value) {
        unpackTypeId<Packable>();
        this->unpack(value);
        return *this;
    }

    template <typename Serializable>
    typename std::enable_if<
            detail::IsSerializable<Serializable, SerializableData,
                    IntegerFeature>::value, Serial&>::type
    operator>>(Serializable& serializable) {
        BOOST_CONCEPT_ASSERT((concept::Serializable<Serializable, Serial>));
        unpackTypeId<Serializable>();
        serializable.deserialize(*this);
        return *this;
    }

    // Rather let go in here even if the user does not provide any mean to
    // serialize the supplied type. A meaningful error message will be shown
    // instead.
    template <typename Serializable>
    typename std::enable_if<
            // detail::IsSerializableNonIntrusive<Serializable>::value &&
            !detail::IsPackable<Serializable>::value &&
                    !detail::IsSerializable<Serializable,
                            SerializableData, IntegerFeature>::value,
            Serial&>::type
    operator>>(Serializable& serializable) {
        static_assert(detail::IsSerializableNonIntrusive<Serializable,
                SerializableData, IntegerFeature>::value,
                "Don't know how to deserialize T. Provide the free function "
                "'void deserialize(const T&, Serial&)' or the member "
                "'void T::deserialize(Serial&)'!");
        unpackTypeId<Serializable>();
        deserialize(serializable, *this);
        return *this;
    }

private:
    template <typename T>
    void packTypeId() {
        constexpr detail::Optional<std::int8_t> typeId =
                detail::getTypeId<T, SerializableData>(CUSTOM_TYPE_OFFSET);
        if (typeId) { // no constexpr if
            this->pack(*typeId);
        }
    }

    template <typename T>
    void unpackTypeId() {
        constexpr detail::Optional<std::int8_t> typeId =
                detail::getTypeId<T, SerializableData>(CUSTOM_TYPE_OFFSET);
        if (typeId) { // no constexpr if
            std::int8_t unpackedTypeId = -1;
            this->unpack(unpackedTypeId);
            BOOST_ASSERT_MSG(unpackedTypeId == *typeId,
                    "Type Id does not match with the expected one.");
        }
    }
};

//----------------------------------------------------------------------------//
} // namespace serialization
//============================================================================//

#endif // SERIALIZATION_SERIAL_HPP
