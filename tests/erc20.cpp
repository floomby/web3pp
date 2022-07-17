#define BOOST_TEST_DYN_LINK
#include "../erc20-test.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(Erc20, *boost::unit_test::depends_on("ContextInit")) {
    Erc20_test erc20_test;

    auto account = std::make_shared<Web3::Account>(std::string{"387e50a4fa783cb44d1d579a0810f169e81f5d2e705615c483aa3c208d25f966"});
    Web3::defaultContext->setPrimarySigner(account);

    erc20_test.deploy("Something", "SMT");
}
