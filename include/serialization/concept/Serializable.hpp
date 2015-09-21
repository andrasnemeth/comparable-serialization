#ifndef SERIALIZATION_CONCEPT_SERIALIZABLE_HPP
#define SERIALIZATION_CONCEPT_SERIALIZABLE_HPP

#include <boost/concept/usage.hpp>

//============================================================================//
namespace serialization {
namespace concept {
//----------------------------------------------------------------------------//

template<typename Model, typename Serial>
class Serializable {
public:
    BOOST_CONCEPT_USAGE(Serializable) {
        model.serialize(serial);
        model.deserialize(serial);
    }

private:
    Model model;
    Serial serial;
};

//----------------------------------------------------------------------------//
} // namespace concept
} // namespace serialization
//============================================================================//

#endif // SERIALIZATION_CONCEPT_SERIALIZABLE_HPP
