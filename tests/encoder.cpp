#define BOOST_TEST_DYN_LINK
#include "../encoder.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(Encoder) {
    BOOST_CHECK(Web3::Encoder::encode(std::vector<unsigned char>{0x00, 0x45, 0x02, 0x00}) == "0x00450200");
    BOOST_CHECK(Web3::Encoder::encode(std::vector<unsigned char>{0x00}) == "0x00");
    BOOST_CHECK(Web3::Encoder::encode(0x00) == "0x0");
    BOOST_CHECK(Web3::Encoder::encode(0x0010004593ULL) == "0x10004593");

    // Encoding of uint256_t
    BOOST_CHECK(Web3::Encoder::encode(Web3::fromString("0")) == "0x0");
    BOOST_CHECK(Web3::Encoder::encode(Web3::fromString("1234567812345678")) == "0x462d537e7ef4e");
    BOOST_CHECK(Web3::Encoder::encode(Web3::fromString("0x12341234123412341234")) == "0x12341234123412341234");

    Web3::Encoder::RLPEncodeInput emptyString(std::vector<unsigned char>({}));
    BOOST_CHECK(Web3::Encoder::encode(emptyString) == std::vector<unsigned char>({0x80}));
    Web3::Encoder::RLPEncodeInput emptyList(std::vector<Web3::Encoder::RLPEncodeInput>({}));
    BOOST_CHECK(Web3::Encoder::encode(emptyList) == std::vector<unsigned char>({0xc0}));
    Web3::Encoder::RLPEncodeInput zero(std::vector<unsigned char>({0x00}));
    BOOST_CHECK(Web3::Encoder::encode(zero) == std::vector<unsigned char>({0x80}));

    Web3::Encoder::RLPEncodeInput input1(std::vector<unsigned char>({0x43, 0x44, 0x45}));
    BOOST_CHECK(Web3::Encoder::encode(input1) == std::vector<unsigned char>({0x83, 0x43, 0x44, 0x45}));

    Web3::Encoder::RLPEncodeInput input2({emptyString, emptyList, zero});
    BOOST_CHECK(Web3::Encoder::encode(input2) == std::vector<unsigned char>({0xc3, 0x80, 0xc0, 0x80}));

    // Check byte length length encoding
    std::vector<unsigned char> vec3;
    for (auto i = 0; i <= 256; i++) vec3.push_back(i);
    Web3::Encoder::RLPEncodeInput input3(vec3);
    auto encoded3 = Web3::Encoder::encode(input3);
    BOOST_CHECK(encoded3[0] == 0xb9 && encoded3[1] == 0x01 && encoded3[2] == 0x01 && encoded3.size() == 260);

    std::vector<Web3::Encoder::RLPEncodeInput> vec4;
    for (auto i = 0; i <= 256; i++) vec4.push_back(emptyList);
    Web3::Encoder::RLPEncodeInput input4(vec4);
    auto encoded4 = Web3::Encoder::encode(input4);
    BOOST_CHECK(encoded4[0] == 0xf9 && encoded4[1] == 0x01 && encoded4[2] == 0x01 && encoded4.size() == 260);
}