#define BOOST_TEST_DYN_LINK
#include "../erc20-test.h"

#include <boost/test/unit_test.hpp>

#include <semaphore>

BOOST_AUTO_TEST_CASE(Erc20, *boost::unit_test::depends_on("ContextInit")) {
    Erc20_test erc20_test;
    erc20_test.deploy("Something", "SMT");
    BOOST_CHECK(erc20_test.decimals() == 18);
    BOOST_CHECK(erc20_test.name() == "Something");
    BOOST_CHECK(erc20_test.symbol() == "SMT");

    auto promise0 = erc20_test.decimals_async([](const boost::multiprecision::uint256_t &value) {
        return value.convert_to<size_t>();
    });

    auto future0 = promise0->get_future();
    future0.wait();

    BOOST_CHECK(future0.get() == 18);

    erc20_test.approve("0x1af831cf22600979B502f1b73392f41fc4328Dc4", Web3::Units::ether(1));
    BOOST_CHECK(Web3::Units::ether(1) == erc20_test.allowance(Web3::defaultContext->signers.front()->address, "0x1af831cf22600979B502f1b73392f41fc4328Dc4"));

    auto handler = [](boost::multiprecision::uint256_t value) {
        return value;
    };

    auto promise1 = erc20_test.approve_async("0xB2C049842476F85334ff58456CAc30ABa6b9B0b8", Web3::Units::gwei(40), [&erc20_test, handler = std::move(handler)](const std::string &res) {
        return erc20_test.allowance_async(Web3::defaultContext->signers.front()->address, "0xB2C049842476F85334ff58456CAc30ABa6b9B0b8", std::move(handler));
    });

    auto future1 = promise1->get_future();
    future1.wait();
    auto promise2 = future1.get();
    auto future2 = promise2->get_future();
    future2.wait();

    BOOST_CHECK(future2.get() == Web3::Units::gwei(40));

    auto otherAccount = std::make_shared<Web3::Account>("1a5260e233781932bef86e6a4defc201c61cd0e55258c2ed5e51702401d7e5d6");
    erc20_test.approve("0x1af831cf22600979B502f1b73392f41fc4328Dc4", Web3::Units::ether(1), {.account = otherAccount});
    BOOST_CHECK(Web3::Units::ether(1) == erc20_test.allowance(otherAccount->address, "0x1af831cf22600979B502f1b73392f41fc4328Dc4"));
}
