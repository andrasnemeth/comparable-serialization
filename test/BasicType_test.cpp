#include <serialization/ByteSequence.hpp>
#include <serialization/Sequentialize.hpp>
#include <serialization/Serial.hpp>

#include <gtest/gtest.h>

#include <boost/endian/conversion.hpp>
#include <boost/mpl/vector.hpp>

#include <limits>
#include <type_traits>
#include <vector>

//============================================================================//
namespace {
//----------------------------------------------------------------------------//

template<typename Int>
class SequentializeIntTest : public ::testing::Test {
protected:
    typename std::make_unsigned<Int>::type swapValue(const Int& value) {
        static_assert(std::is_signed<Int>::value, "Not a signed int");

        using UnsignedInt = typename std::make_unsigned<Int>::type;

        if (value < 0) {
            return boost::endian::native_to_big(
                    static_cast<UnsignedInt>(value
                            + static_cast<UnsignedInt>(
                                    std::numeric_limits<Int>::max())
                            + static_cast<UnsignedInt>(1)));
        } else {
            UnsignedInt unsignedInt = static_cast<UnsignedInt>(value)
                + static_cast<UnsignedInt>(std::numeric_limits<Int>::max()
                        + static_cast<UnsignedInt>(1));

            return boost::endian::native_to_big(unsignedInt);
        }
    }

    void packAndUnpack(const Int& value) {
        using PackedInt = typename std::make_unsigned<Int>::type;

        serialization::ByteSequence byteSequence;
        PackedInt swappedValue = swapValue(value);
        serialization::pack(byteSequence, value);
        ASSERT_EQ(sizeof(Int), byteSequence.size());
        PackedInt packedValue = *reinterpret_cast<const PackedInt*>(
                byteSequence.data());

        EXPECT_EQ(swappedValue, packedValue) << "original value is: " << value;

        Int reconstructedValue = 0;
        EXPECT_EQ(sizeof(Int), serialization::unpack(byteSequence.begin(),
                        reconstructedValue));
        EXPECT_EQ(value, reconstructedValue);
    }

    const std::vector<std::pair<std::int64_t, std::int64_t>> testData{{0, -1},
        {5, 1}, {140, -45}, {std::numeric_limits<std::int8_t>::max(),
                std::numeric_limits<std::int8_t>::min() + 1},
        {5430, 4432}, {3322, -1122}, {std::numeric_limits<std::int16_t>::max(),
                std::numeric_limits<std::int16_t>::min() + 1},
        {324354534, 112222233}, {765522, -343546},
        {std::numeric_limits<std::int32_t>::max(),
                std::numeric_limits<std::int32_t>::min() + 1},
        {3234534656456452323L, -5256547647575563423L},
        {std::numeric_limits<std::int64_t>::max(),
                std::numeric_limits<std::int64_t>::min() + 1}};
};

TYPED_TEST_CASE_P(SequentializeIntTest);

//----------------------------------------------------------------------------//

TYPED_TEST_P(SequentializeIntTest, PackAndUnpack) {
    using PackedInt = typename std::make_unsigned<TypeParam>::type;

    for (const auto& pair : this->testData) {
        if (pair.first <= std::numeric_limits<TypeParam>::max() &&
                pair.first >= std::numeric_limits<TypeParam>::min()) {
            this->packAndUnpack(static_cast<TypeParam>(pair.first));
        }

        if (pair.second <= std::numeric_limits<TypeParam>::max() &&
                pair.second >= std::numeric_limits<TypeParam>::min()) {
            this->packAndUnpack(static_cast<TypeParam>(pair.second));
        }
    }
}

TYPED_TEST_P(SequentializeIntTest, SerializeAndCompareData) {
    for (const auto& pair : this->testData) {
        if (pair.first <= std::numeric_limits<TypeParam>::max() &&
                pair.first >= std::numeric_limits<TypeParam>::min() &&
                pair.second <= std::numeric_limits<TypeParam>::max() &&
                pair.second >= std::numeric_limits<TypeParam>::min()) {
            serialization::Serial<> serial1, serial2;
            serial1 << static_cast<TypeParam>(pair.first);
            serial2 << static_cast<TypeParam>(pair.second);
            EXPECT_NE(serial1, serial2);
            EXPECT_GT(serial1, serial2);
            TypeParam data1, data2;
            serial1 >> data1;
            serial2 >> data2;
            EXPECT_EQ(pair.first, data1);
            EXPECT_EQ(pair.second, data2);
        }
    }
}

//----------------------------------------------------------------------------//

REGISTER_TYPED_TEST_CASE_P(SequentializeIntTest, PackAndUnpack,
        SerializeAndCompareData);

using PackableIntTypes = ::testing::Types<std::int8_t, std::int16_t,
        std::int32_t, std::int64_t>;

INSTANTIATE_TYPED_TEST_CASE_P(PackableIntTestcase, SequentializeIntTest,
        PackableIntTypes);

//============================================================================//

class DoubleTest : public ::testing::Test {
protected:
    void packAndUnpack(const double& data) {
        serialization::ByteSequence byteSequence;
        serialization::pack(byteSequence, data);
        double unpackedData;
        EXPECT_EQ(sizeof(data), byteSequence.size());
        serialization::unpack(byteSequence.begin(), unpackedData);
        if (isnan(data)) {
            EXPECT_TRUE(isnan(unpackedData));
        } else {
            EXPECT_EQ(data, unpackedData);
        }
    }

    const std::vector<std::pair<double, double>> testData{{1.0, 0.0},
            {0.034354543, -1.0}, {0.1, NAN},
            {12312434830249234123123.345453, -34345435345343454354.1234454564},
            {std::numeric_limits<double>::max(),
                    std::numeric_limits<double>::infinity() * -1},
            {234.342352454123123324554353443254543, -0.34353465465423423423423},
            {NAN, std::numeric_limits<double>::infinity() * -1},
            {std::numeric_limits<double>::infinity(),
                    std::numeric_limits<double>::max()},
            {0.3435453653432423423, std::numeric_limits<double>::min()}};
};

//----------------------------------------------------------------------------//

TEST_F(DoubleTest, PackAndUnpack) {
    for (const auto& pair : testData) {
        packAndUnpack(pair.first);
        packAndUnpack(pair.second);
    }
}

TEST_F(DoubleTest, SerializeAndCompareData) {
    for (const auto& pair: testData) {
        serialization::Serial<> serial1, serial2;
        serial1 << pair.first;
        serial2 << pair.second;
        EXPECT_NE(serial1, serial2);
        EXPECT_GT(serial1, serial2) << "Original pair: {" << pair.first
                << ", " << pair.second << "}";
        double data1, data2;
        serial1 >> data1;
        serial2 >> data2;
        if (isnan(data1)) {
            EXPECT_TRUE(isnan(pair.first));
        } else {
            EXPECT_EQ(pair.first, data1);
        }
        if (isnan(data2)) {
            EXPECT_TRUE(isnan(pair.second));
        } else {
            EXPECT_EQ(pair.second, data2);
        }
    }
}

//============================================================================//

class StringTest : public ::testing::Test {
protected:
    std::vector<std::pair<std::string, std::string>> testData{
        {"a", ""}, {"b", "a"}, {"abcdef", "abcde"}};
};

//----------------------------------------------------------------------------//

TEST_F(StringTest, PackAndUnpack) {
    for (const auto& pair : testData) {
        serialization::ByteSequence byteSequence;
        serialization::pack(byteSequence, pair.first);
        EXPECT_STREQ(pair.first.data(), reinterpret_cast<const char*>(
                        byteSequence.data()));
        std::string recreatedValue;
        EXPECT_EQ(pair.first.length() + 1, // \0 byte
                serialization::unpack(byteSequence.begin(), recreatedValue));
        EXPECT_EQ(pair.first, recreatedValue);
    }
}

// TODO: add int tests including promotion

//----------------------------------------------------------------------------//
} // unnamed namespace
//============================================================================//
