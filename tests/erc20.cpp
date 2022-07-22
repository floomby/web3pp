#define BOOST_TEST_MODULE Web3Tests
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_NO_MAIN
#include "../erc20-test.h"

#include <boost/test/unit_test.hpp>

#include <semaphore>

// entry point for all the tests
int main(int argc, char* argv[], char* envp[]) {
    return boost::unit_test::unit_test_main(&init_unit_test, argc, argv);
}

BOOST_AUTO_TEST_CASE(Erc20, *boost::unit_test::depends_on("ContextInit")) {
    Erc20_test erc20_test;
    erc20_test.deploy("Something", "SMT");
    BOOST_CHECK(erc20_test.decimals() == 18);
    BOOST_CHECK(erc20_test.name() == "Something");
    BOOST_CHECK(erc20_test.symbol() == "SMT");

    std::binary_semaphore testSemaphore(1);

    size_t decimals = 0;
    testSemaphore.acquire();
    erc20_test.decimals_async([&decimals, &testSemaphore](const boost::multiprecision::uint256_t &value) {
        decimals = value.convert_to<size_t>();
        // std::this_thread::sleep_for(std::chrono::seconds(8));
        testSemaphore.release();
    });

    BOOST_CHECK(testSemaphore.try_acquire_until(std::chrono::system_clock::now() + std::chrono::seconds(5)));
    BOOST_CHECK(decimals == 18);

    erc20_test.approve("0x1af831cf22600979B502f1b73392f41fc4328Dc4", Web3::Units::ether(1));
    BOOST_CHECK(Web3::Units::ether(1) == erc20_test.allowance(Web3::defaultContext->signers.front()->address, "0x1af831cf22600979B502f1b73392f41fc4328Dc4"));

    boost::multiprecision::uint256_t approvedAmount{0};
    auto handler = [&approvedAmount, &testSemaphore](const boost::multiprecision::uint256_t &value) {
        approvedAmount = value;
        testSemaphore.release();
    };

    erc20_test.approve_async("0xB2C049842476F85334ff58456CAc30ABa6b9B0b8", Web3::Units::gwei(40), [&erc20_test, handler = std::move(handler)](const std::string &res) {
        erc20_test.allowance_async(Web3::defaultContext->signers.front()->address, "0xB2C049842476F85334ff58456CAc30ABa6b9B0b8", std::move(handler));
    });
    BOOST_CHECK(testSemaphore.try_acquire_until(std::chrono::system_clock::now() + std::chrono::seconds(6)));
    testSemaphore.release();
    BOOST_CHECK(approvedAmount == Web3::Units::gwei(40));
}
