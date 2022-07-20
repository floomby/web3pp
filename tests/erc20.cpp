#define BOOST_TEST_DYN_LINK
#include "../erc20-test.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(Erc20, *boost::unit_test::depends_on("ContextInit")) {
    Erc20_test erc20_test;
    erc20_test.deploy("Something", "SMT");
    BOOST_CHECK(erc20_test.decimals() == 18);
    BOOST_CHECK(erc20_test.name() == "Something");
    BOOST_CHECK(erc20_test.symbol() == "SMT");
}
