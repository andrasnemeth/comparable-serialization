#include <serialization/Serial.hpp>
#include <serialization/concept/Serializable.hpp>

#include <gtest/gtest.h>

#include <boost/concept/assert.hpp>

#include <cstdint>
#include <string>

//============================================================================//
namespace {
//----------------------------------------------------------------------------//

class SerializableTest : public ::testing::Test {
protected:
    class Foo;
    using Serial = serialization::Serial<boost::mpl::vector<>>;

    class Foo {
    public:
        Foo() : value1(0), value2(.0) {
        }

        Foo(int value1, double value2) : value1(value1), value2(value2) {
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

    BOOST_CONCEPT_ASSERT((serialization::concept::Serializable<Foo, Serial>));
};

//----------------------------------------------------------------------------//

TEST_F(SerializableTest, TestFoo) {
    Serial serial;
    Foo foo{1234, 0.0000004}, recreatedFoo;

    serial << foo;
    serial >> recreatedFoo;

    EXPECT_EQ(foo, recreatedFoo);
}

TEST_F(SerializableTest, AbortsOnTypeMismatch) {
    Serial serial;
    Foo foo;

    serial << foo;
    serial << std::string{"ABCDEF"};

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
            "Cannot unpack more data.");
}

//============================================================================//
namespace foo {
//----------------------------------------------------------------------------//

struct Bar {
    int a;
    double b;
};

bool operator==(const Bar& lhs, const Bar& rhs) {
    return lhs.a == rhs.a && lhs.b == rhs.b;
}

using Serial = serialization::Serial<boost::mpl::vector<Bar>>;

void serialize(const Bar& bar, Serial& serial) {
    serial << bar.a;
    serial << bar.b;
}

void deserialize(Bar& bar, Serial& serial) {
    serial >> bar.a;
    serial >> bar.b;
}

//----------------------------------------------------------------------------//
} // namespace foo
//============================================================================//

TEST(ExternalSerializableTest, TestBar) {
    foo::Serial serial;
    foo::Bar bar, recreatedBar;
    serial << bar;
    serial >> recreatedBar;

    EXPECT_EQ(bar, recreatedBar);
}

//----------------------------------------------------------------------------//
} // unnamed namespace
//============================================================================//
