#ifndef SERIALIZATION_SERIAL_HPP
#define SERIALIZATION_SERIAL_HPP

#include <boost/mpl/vector.hpp>

#include <cstdint>

//============================================================================//
namespace serialization {
namespace detail {
//----------------------------------------------------------------------------//

using PackableData = boost::mpl::vector<std::int8_t, std::int16_t, std::int32_t,
        std::int64_t, double, std::vector<std::int8_t>>;

//----------------------------------------------------------------------------//
} // namespace detail
//============================================================================//

class Serial {
public:
    Serial() = default;
    Serial(const char* data, const std::size_t& size)
            : byteSequence(data, size) {
    }

    Serial(const Serial&) = delete;
    Serial(Serial&&) = default;
    Serial& operator=(const Serial&) = delete;
    Serial& operator=(Serial&&) = default;

    template<typename Packable, typename std::enable_if<boost::mpl::contains<
            detail::PackableData, Packable>::value>::type* = nullptr>
    Serial& operator<<(const Packable& value) {
        pack(byteSequence, value);
    }

private:
    ByteSequence byteSequence; // TODO: move ByteSequence into detail
};

//----------------------------------------------------------------------------//
}  // namespace serialization
//============================================================================//

#endif // SERIALIZATION_SERIAL_HPP
