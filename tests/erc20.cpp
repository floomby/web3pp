#define BOOST_TEST_DYN_LINK
#include "../erc20-test.h"

#include <boost/test/unit_test.hpp>

#include <mutex>

BOOST_AUTO_TEST_CASE(Erc20, *boost::unit_test::depends_on("ContextInit")) {
    Erc20_test erc20_test;
    erc20_test.deploy("Something", "SMT");
    BOOST_CHECK(erc20_test.decimals() == 18);
    BOOST_CHECK(erc20_test.name() == "Something");
    BOOST_CHECK(erc20_test.symbol() == "SMT");

    std::timed_mutex testMutex;

    size_t decimals;
    testMutex.lock();
    erc20_test.decimals_async([&decimals, &testMutex](const boost::multiprecision::uint256_t &value) {
        decimals = value.convert_to<size_t>();
        testMutex.unlock();
    });

    BOOST_CHECK(testMutex.try_lock_until(std::chrono::system_clock::now() + std::chrono::seconds(5)));
    BOOST_CHECK(decimals == 18);

    erc20_test.approve_async(Web3::defaultContext->signers.front()->address, boost::multiprecision::uint256_t{1000000000000000000}, [](const std::string &res) {
        std::cout << "Approve callback: " << res << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::seconds(5));
}
