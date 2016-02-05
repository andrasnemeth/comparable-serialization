#ifndef SERIALIZATION_DETAIL_OPTIONAL_HPP
#define SERIALIZATION_DETAIL_OPTIONAL_HPP

#include <type_traits>

//============================================================================//
namespace serialization {
namespace detail {
//----------------------------------------------------------------------------//

struct None {
};

constexpr const None none{};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

template<typename T>
union OptionalStorage {
    static_assert(std::is_trivially_destructible<T>::value,
            "T must be trivially destructible");

    constexpr OptionalStorage() : none() {
    }

    constexpr OptionalStorage(const T& value) : value(value) {
    }

    ~OptionalStorage() = default;

    None none;
    T value;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

template<typename T>
class Optional {
public:
    constexpr Optional() = default;

    constexpr Optional(const None&) {
    }

    constexpr Optional(const T& value) : initialized{true}, storage{value} {
    }

    ~Optional() = default;

    constexpr operator bool() const {
        return initialized;
    }

    // UB if not initialized
    constexpr const T& operator*() const {
        return storage.value;
    }

private:
    bool initialized = false;
    OptionalStorage<T> storage{};
};

//----------------------------------------------------------------------------//
} // namespace detail
} // namespace serialization
//============================================================================//

#endif // SERIALIZATION_DETAIL_OPTIONAL_HPP
