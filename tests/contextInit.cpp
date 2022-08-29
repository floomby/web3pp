#define BOOST_TEST_MODULE Web3Tests
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_NO_MAIN

#define WEB3_IMPLEMENTATION
#include <boost/test/unit_test.hpp>

#include <mutex>

#include "../base.h"
#include "../account.h"

#include <iostream>

// entry point for all the tests
int main(int argc, char* argv[], char* envp[]) {
    return boost::unit_test::unit_test_main(&init_unit_test, argc, argv);
}

BOOST_AUTO_TEST_CASE(ContextInit) {
    try {
        std::cout << "Connecting to " << boost::unit_test::framework::master_test_suite().argv[1]
            << " on port " << boost::unit_test::framework::master_test_suite().argv[2]
            << " with chain id of " << boost::unit_test::framework::master_test_suite().argv[3] << std::endl;

        Web3::defaultContext = std::make_shared<Web3::Context>(
            boost::unit_test::framework::master_test_suite().argv[1],
            boost::unit_test::framework::master_test_suite().argv[2],
            std::stoul(boost::unit_test::framework::master_test_suite().argv[3]),
            strcmp("true", boost::unit_test::framework::master_test_suite().argv[4]) == 0
        );
        Web3::defaultContext->run();

        auto account = std::make_shared<Web3::Account>(std::string{"387e50a4fa783cb44d1d579a0810f169e81f5d2e705615c483aa3c208d25f966"});
        Web3::defaultContext->setPrimarySigner(account);

        std::timed_mutex testMutex;

        auto promise = account->getTransactionCount_async([](size_t value) {
            return value;
        });

        auto future = promise->get_future();
        future.wait();
        auto nonce = future.get();

        // BOOST_CHECK(testSemaphore.try_acquire_for(std::chrono::seconds(5)));
        BOOST_CHECK(nonce == account->getTransactionCount());
    } catch (std::exception &e) {
        std::cout << "Error: " << e.what() << std::endl;
        BOOST_CHECK(false);
    }
}
