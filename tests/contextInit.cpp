#define BOOST_TEST_DYN_LINK

#define WEB3_IMPLEMENTATION
#include <boost/test/unit_test.hpp>

#include <mutex>

#include "../base.h"
#include "../account.h"

#include <iostream>

BOOST_AUTO_TEST_CASE(ContextInit) {
    try {
        Web3::defaultContext = std::make_shared<Web3::Context>(
            boost::unit_test::framework::master_test_suite().argv[1],
            boost::unit_test::framework::master_test_suite().argv[2],
            std::stoul(boost::unit_test::framework::master_test_suite().argv[3]),
            true
        );
        // Web3::defaultContext = std::make_shared<Web3::Context>("rpc.sepolia.dev", "443", 11155111, true);
        Web3::defaultContext->run();

        auto account = std::make_shared<Web3::Account>(std::string{"387e50a4fa783cb44d1d579a0810f169e81f5d2e705615c483aa3c208d25f966"});
        Web3::defaultContext->setPrimarySigner(account);

        std::timed_mutex testMutex;

        // std::cout << "Transaction count: " << account->getTransactionCount() << std::endl;
        // return;
        testMutex.lock();
        size_t nonce = std::numeric_limits<size_t>::max();
        account->getTransactionCount_async([&nonce, &testMutex](size_t value) {
            nonce = value;
            std::cout << "Transaction count: " << value << std::endl;
            testMutex.unlock();
        });

        BOOST_CHECK(testMutex.try_lock_until(std::chrono::system_clock::now() + std::chrono::seconds(15)));
        BOOST_CHECK(nonce == account->getTransactionCount());
    } catch (std::exception &e) {
        std::cout << "Error: " << e.what() << std::endl;
        BOOST_CHECK(false);
    }
}
