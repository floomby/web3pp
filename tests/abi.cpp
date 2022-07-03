#define BOOST_TEST_MODULE Abi
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

    BOOST_CHECK(Web3::toString(Web3::Encoder::ABIEncode(Web3::hexToBytes<Web3::Address>("0xbcc18C958EaE2fd41E21B59Efc626205d8982107"))) == "000000000000000000000000bcc18c958eae2fd41e21b59efc626205d8982107");
}
