#ifndef SERIALIZATION_DETAIL_CONVERSIONMAP_HPP
#define SERIALIZATION_DETAIL_CONVERSIONMAP_HPP

#include <boost/mpl/map.hpp>
#include <boost/mpl/pair.hpp>

#include <string>

//============================================================================//
namespace serialization {
namespace detail {
//----------------------------------------------------------------------------//

using ConversionMap = boost::mpl::map<
        boost::mpl::pair<std::int8_t, std::uint8_t>,
        boost::mpl::pair<std::int16_t, std::uint16_t>,
        boost::mpl::pair<std::int32_t, std::uint32_t>,
        boost::mpl::pair<std::int64_t, std::uint64_t>,
        boost::mpl::pair<double, std::uint64_t>,
        boost::mpl::pair<std::string, void>>;

//----------------------------------------------------------------------------//
} // namespace detail
} // namespace serialization
//============================================================================//

#endif // SERIALIZATION_DETAIL_CONVERSIONMAP_HPP
