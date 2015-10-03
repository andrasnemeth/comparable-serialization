#ifndef SERIALIZATION_DETAIL_TYPETRAITS_HPP
#define SERIALIZATION_DETAIL_TYPETRAITS_HPP

#include "ConversionMap.hpp"

#include <boost/mpl/back_inserter.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/deque.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/vector.hpp>

//============================================================================//
namespace serialization {
//----------------------------------------------------------------------------//

template<typename T>
class Serial;

//============================================================================//
namespace detail {
//----------------------------------------------------------------------------//

using PackableData = boost::mpl::transform<ConversionMap,
        boost::mpl::first<boost::mpl::_1>,
        boost::mpl::back_inserter<boost::mpl::vector<>>>::type;

//----------------------------------------------------------------------------//

template<typename T, typename TypeSequence, typename = void,
        typename = void>
struct IsSerializable : std::false_type {
};

template<typename Serializable, typename TypeSequence>
struct IsSerializable<Serializable, TypeSequence,
        typename std::enable_if<std::is_void<decltype(
                std::declval<const Serializable&>().serialize(
                        std::declval<Serial<TypeSequence>&>()))>::value,
                void>::type,
        typename std::enable_if<std::is_void<decltype(
                std::declval<Serializable&>().deserialize(
                        std::declval<Serial<TypeSequence>&>()))>::value,
                void>::type>
        : std::true_type {
};

//----------------------------------------------------------------------------//

template<typename T, typename TypeSequence, typename = void, typename = void>
struct IsSerializableNonIntrusive : std::false_type {
};

template<typename Serializable, typename TypeSequence>
struct IsSerializableNonIntrusive<Serializable, TypeSequence,
        typename std::enable_if<std::is_void<decltype(serialize(
                        std::declval<const Serializable&>(),
                        std::declval<Serial<TypeSequence>&>()))>::value,
                void>::type,
        typename std::enable_if<std::is_void<decltype(deserialize(
                        std::declval<Serializable&>(),
                        std::declval<Serial<TypeSequence>&>()))>::value,
                void>::type>
        : std::true_type {
};

//----------------------------------------------------------------------------//

template<typename T, typename = void>
struct IsPackable : std::false_type {
};

template<typename Packable>
struct IsPackable<Packable,
        typename std::enable_if<boost::mpl::contains<PackableData,
                               Packable>::value>::type>
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
} // namespace serialization
//============================================================================//

#endif // SERIALIZATION_DETAIL_TYPETRAITS_HPP
