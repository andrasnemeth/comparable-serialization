#ifndef SERIALIZATION_BYTESEQUENCE_HPP
#define SERIALIZATION_BYTESEQUENCE_HPP

#include <cstdint>
#include <vector>

//============================================================================//
namespace serialization {
//----------------------------------------------------------------------------//

using byte = unsigned char;

// Each type has different size in memory. Therefore using a continuous memory
// space is needed (dequeue is not good here).
using ByteSequence = std::vector<byte>;

//----------------------------------------------------------------------------//
} // namespace serialization
//============================================================================//

#endif // SERIALIZATION_BYTESEQUENCE_HPP
