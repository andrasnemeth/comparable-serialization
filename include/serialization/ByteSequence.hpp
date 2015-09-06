#ifndef SERIALIZATION_BYTESEQUENCE_HPP
#define SERIALIZATION_BYTESEQUENCE_HPP

#include <cstdint>
#include <vector>

//============================================================================//
namespace serialization {
//----------------------------------------------------------------------------//

// Each type has different size in memory. Therefore using a continuous memory
// space is needed (dequeue is not good here).
using ByteSequence = std::vector<std::uint8_t>;

//----------------------------------------------------------------------------//
} // namespace serialization
//============================================================================//

#endif // SERIALIZATION_BYTESEQUENCE_HPP
