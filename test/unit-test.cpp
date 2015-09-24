#include <serialization/ByteSequence.hpp>
#include <serialization/Sequentialize.hpp>
#include <serialization/Serial.hpp>

#include <gtest/gtest.h>

#include <boost/endian/conversion.hpp>

#include <limits>
#include <type_traits>
#include <vector>

//============================================================================//

template<typename Int>
class SequentializeIntTest : public ::testing::Test {
protected:
    const std::vector<std::pair<std::int64_t, std::int64_t>> testData{
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

template<typename Int>
typename std::make_unsigned<Int>::type swapValue(const Int& value) {
    static_assert(std::is_signed<Int>::value, "Not a signed int");

    using UnsignedInt = typename std::make_unsigned<Int>::type;

    if (value < 0) {
        return boost::endian::native_to_big(
                static_cast<UnsignedInt>(value +
                        std::numeric_limits<Int>::max()
                        + static_cast<UnsignedInt>(1)));
    } else {
        UnsignedInt unsignedInt = static_cast<UnsignedInt>(value)
            + static_cast<UnsignedInt>(std::numeric_limits<Int>::max());

        return boost::endian::native_to_big(unsignedInt);
    }
}

template<typename Int>
void testPack(const Int& value) {
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

TYPED_TEST_P(SequentializeIntTest, PackAndUnpack) {
    using PackedInt = typename std::make_unsigned<TypeParam>::type;

    for (const auto& pair : this->testData) {
        if (pair.first <= std::numeric_limits<TypeParam>::max() &&
                pair.first >= std::numeric_limits<TypeParam>::min()) {
            testPack(static_cast<TypeParam>(pair.first));
        }

        if (pair.second <= std::numeric_limits<TypeParam>::max() &&
                pair.second >= std::numeric_limits<TypeParam>::min()) {
            testPack(static_cast<TypeParam>(pair.second));
        }
    }
}

//----------------------------------------------------------------------------//

REGISTER_TYPED_TEST_CASE_P(SequentializeIntTest, PackAndUnpack);

using PackableIntTypes = ::testing::Types<std::int8_t, std::int16_t,
        std::int32_t, std::int64_t>;

INSTANTIATE_TYPED_TEST_CASE_P(PackableIntTestcase, SequentializeIntTest,
        PackableIntTypes);
