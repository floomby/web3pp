#define BOOST_TEST_DYN_LINK

#define WEB3_IMPLEMENTATION
#include <boost/test/unit_test.hpp>

#include "../base.h"
#include "../account.h"

BOOST_AUTO_TEST_CASE(ContextInit) {
    try {
        Web3::defaultContext = std::make_shared<Web3::Context>("127.0.0.1", "7545", 1);
        // Web3::defaultContext = std::make_shared<Web3::Context>("rpc.sepolia.dev", "443", 11155111, true);
        Web3::defaultContext->run();

        auto account = std::make_shared<Web3::Account>(std::string{"387e50a4fa783cb44d1d579a0810f169e81f5d2e705615c483aa3c208d25f966"});
        Web3::defaultContext->setPrimarySigner(account);
    } catch (std::exception &e) {
        std::cout << "Error: " << e.what() << std::endl;
        BOOST_CHECK(false);
    }
}
