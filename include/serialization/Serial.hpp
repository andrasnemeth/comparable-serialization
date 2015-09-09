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
    Serial() : readIterator(byteSequence.begin()) {
    }

    Serial(const char* data, const std::size_t& size)
            : byteSequence(data, size),
              readIterator(byteSequence.begin()) {
    }

    Serial(const Serial&) = delete;
    Serial(Serial&&) = default;
    Serial& operator=(const Serial&) = delete;
    Serial& operator=(Serial&&) = default;

    template<typename Packable>
    typename std::enable_if<boost::mpl::contains<detail::PackableData,
            Packable>::value, Serial&>::type
    operator<<(const Packable& value) {
        pack(byteSequence, value);
        return *this;
    }

    template<typename Packable>
    typename std::enable_if<boost::mpl::contains<detail::PackableData,
            Packable>::value, Serial&>::type
    operator<<(Packable& value) {
        pack(readIterator, value);
        return *this;
    }

private:
    ByteSequence byteSequence; // TODO: move ByteSequence into detail
    ByteSequence::iterator readIterator;
};

//----------------------------------------------------------------------------//
}  // namespace serialization
//============================================================================//

#endif // SERIALIZATION_SERIAL_HPP
