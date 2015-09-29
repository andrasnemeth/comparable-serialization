#include <serialization/Serial.hpp>
#include <serialization/concept/Serializable.hpp>

#include <gtest/gtest.h>

#include <boost/concept/assert.hpp>

//============================================================================//
namespace {
//----------------------------------------------------------------------------//

class SerializableTest : public ::testing::Test {
protected:
    class Foo;
    using Serial = serialization::Serial<boost::mpl::vector<>>;

    class Foo {
    public:
        Foo() : value1(1234), value2(0.0000043) {
        }

        void serialize(Serial& serial) const {
            serial << value1;
            serial << value2;
        }

        void deserialize(Serial& serial) {
            serial >> value1;
            serial >> value2;
        }

        bool operator==(const Foo& other) const {
            return value1 == other.value1 && value2 == other.value2;
        }

        bool operator>(const Foo& other) const {
            return value1 > other.value1 && value2 > other.value2;
        }

   private:
        int value1;
        double value2;
    };
};

//----------------------------------------------------------------------------//

TEST_F(SerializableTest, FooConceptCheck) {
    BOOST_CONCEPT_ASSERT((serialization::concept::Serializable<Foo, Serial>));
}

TEST_F(SerializableTest, TestFoo) {
    Serial serial;
    Foo foo, recreatedFoo;

    serial << foo;
    serial >> recreatedFoo;

    EXPECT_EQ(foo, recreatedFoo);
}

TEST_F(SerializableTest, AbortsOnTypeMismatch) {
    Serial serial;
    Foo foo;

    serial << foo;
    serial << 4;

    serial >> foo;
    EXPECT_DEATH({serial >> foo;},
            "Type Id does not match with the expected one.");
}

TEST_F(SerializableTest, AbortsWhenBytesRunOut) {
    Serial serial;
    Foo foo;

    serial << foo;

    serial >> foo;
    EXPECT_DEATH({serial >> foo;},
            "Cannot unpack more data of this type.");
}

//----------------------------------------------------------------------------//
} // unnamed namespace
//============================================================================//
