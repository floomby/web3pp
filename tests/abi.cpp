#define BOOST_TEST_DYN_LINK
#include "../abi.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(Abi) {
    BOOST_CHECK(Web3::toString(Web3::Encoder::ABIEncode(Web3::fromString<true>("-1"))) == "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    BOOST_CHECK(Web3::toString(Web3::Encoder::ABIEncode(Web3::fromString<true>("-123456789"))) == "fffffffffffffffffffffffffffffffffffffffffffffffffffffffff8a432eb");
    BOOST_CHECK(Web3::toString(Web3::Encoder::ABIEncode(Web3::fromString("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"))) == "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    BOOST_CHECK(Web3::toString(Web3::Encoder::ABIEncode(5)) == "0000000000000000000000000000000000000000000000000000000000000005");
    BOOST_CHECK(Web3::toString(Web3::Encoder::ABIEncode(-642)) == "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffd7e");
    BOOST_CHECK(Web3::toString(Web3::Encoder::ABIEncode(true)) == "0000000000000000000000000000000000000000000000000000000000000001");
    BOOST_CHECK(Web3::toString(Web3::Encoder::ABIEncode(false)) == "0000000000000000000000000000000000000000000000000000000000000000");

    BOOST_CHECK(Web3::toString(Web3::Encoder::ABIEncode(Web3::Address("0xbcc18C958EaE2fd41E21B59Efc626205d8982107"))) == "000000000000000000000000bcc18c958eae2fd41e21b59efc626205d8982107");

    BOOST_CHECK(Web3::toString(Web3::Encoder::ABIEncode(std::array<unsigned char, 3>({0x01, 0x02, 0x03}))) == "0102030000000000000000000000000000000000000000000000000000000000");

    // BOOST_CHECK(Web3::toString(Web3::Encoder::ABIEncode(Web3::Fixed<true, 128, 3>({{}, Web3::fromString<true>("-123456789")}))) == "ffffffffffffffffffffffffffffffffffffffffffffffffffffffe34166e5f8");
    // BOOST_CHECK(Web3::toString(Web3::Encoder::ABIEncode(Web3::Fixed<false, 128, 18>({{}, Web3::fromString("123456789")}))) == "000000000000000000000000000000000000000000661efdf12d1653cf340000");

    BOOST_CHECK(Web3::toString(Web3::Encoder::ABIEncode(Web3::hexToBytes("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"))) == 
        "00000000000000000000000000000000000000000000000000000000000000280123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef000000000000000000000000000000000000000000000000");
    BOOST_CHECK(Web3::toString(Web3::Encoder::ABIEncode(Web3::hexToBytes("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"))) == 
        "00000000000000000000000000000000000000000000000000000000000000200123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");

    // example taken from the solidity docs
    std::vector<std::vector<int>> v;
    v.push_back({1, 2});
    v.push_back({3});
    std::vector<std::string> s;
    s.push_back("one");
    s.push_back("two");
    s.push_back("three");
    BOOST_CHECK(Web3::toString(Web3::Encoder::ABIEncode(v, s)) == "000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000001400000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000000a0000000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000030000000000000000000000000000000000000000000000000000000000000003000000000000000000000000000000000000000000000000000000000000006000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000000000000000000000000000000000000000000e000000000000000000000000000000000000000000000000000000000000000036f6e650000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000374776f000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000057468726565000000000000000000000000000000000000000000000000000000");

    BOOST_CHECK(Web3::Encoder::ABIDecodeTo<boost::multiprecision::uint256_t>(Web3::hexToBytes("0000000000000000000000000000000000000000000000000000000000000001")) == 1);
    BOOST_CHECK(Web3::Encoder::ABIDecodeTo<unsigned int>(Web3::hexToBytes("0000000000000000000000000000000000000000000000000000000000000001")) == 1);

    auto test = Web3::Encoder::ABIDecodeTo<std::vector<unsigned char>>(Web3::hexToBytes("00000000000000000000000000000000000000000000000000000000000000030102030000000000000000000000000000000000000000000000000000000000"));
    BOOST_CHECK(test.size() == 3);
    BOOST_CHECK(test[0] == 0x01);
    BOOST_CHECK(test[1] == 0x02);
    BOOST_CHECK(test[2] == 0x03);

    auto test2 = Web3::Encoder::ABIDecodeTo<std::vector<unsigned int>>(Web3::hexToBytes("0000000000000000000000000000000000000000000000000000000000000003000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000003"));
    BOOST_CHECK(test2.size() == 3);
    BOOST_CHECK(test2[0] == 0x01);
    BOOST_CHECK(test2[1] == 0x02);
    BOOST_CHECK(test2[2] == 0x03);

    auto test3 = Web3::Encoder::ABIDecodeTo<std::vector<boost::multiprecision::int256_t>>(Web3::hexToBytes("0000000000000000000000000000000000000000000000000000000000000003fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffd"));
    BOOST_CHECK(test3.size() == 3);
    BOOST_CHECK(test3[0] == -1);
    BOOST_CHECK(test3[1] == -2);
    BOOST_CHECK(test3[2] == -3);

    BOOST_CHECK(Web3::Encoder::ABIDecodeTo<Web3::Address>(Web3::hexToBytes("000000000000000000000000bcc18c958eae2fd41e21b59efc626205d8982107")) == Web3::Address("0xbcc18C958EaE2fd41E21B59Efc626205d8982107"));
    BOOST_CHECK(Web3::Encoder::ABIDecodeTo<std::string>(Web3::hexToBytes("0000000000000000000000000000000000000000000000000000000000000000")) == "");
    BOOST_CHECK(Web3::Encoder::ABIDecodeTo<std::string>(Web3::hexToBytes("00000000000000000000000000000000000000000000000000000000000000034142430000000000000000000000000000000000000000000000000000000000")) == "ABC");
    
    auto test4 = Web3::Encoder::ABIDecodeTo<std::vector<std::string>>(Web3::hexToBytes("0000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000000800000000000000000000000000000000000000000000000000000000000000003414243000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000036162630000000000000000000000000000000000000000000000000000000000"));
    BOOST_CHECK(test4.size() == 2);
    BOOST_CHECK(test4[0] == "ABC");
    BOOST_CHECK(test4[1] == "abc");

    auto test5 = Web3::Encoder::ABIDecodeTo<std::tuple<int, std::string, bool>>(Web3::hexToBytes("00000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000000060000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000034142430000000000000000000000000000000000000000000000000000000000"));
    BOOST_CHECK(std::get<0>(test5) == 1);
    BOOST_CHECK(std::get<1>(test5) == "ABC");
    BOOST_CHECK(std::get<2>(test5) == true);
}
