#include <serialization/Serial.hpp>

#include <gtest/gtest.h>

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

TEST_F(SerializableTest, TestFoo) {
    Serial serial;
    Foo foo, recreatedFoo;

    serial << foo;
    serial >> recreatedFoo;

    EXPECT_EQ(foo, recreatedFoo);
}

// TODO: add negative tests
// TODO: add int tests including promotion
// TODO: string tests

//----------------------------------------------------------------------------//
} // unnamed namespace
//============================================================================//
