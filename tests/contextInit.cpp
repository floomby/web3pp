#define BOOST_TEST_DYN_LINK

#define WEB3_IMPLEMENTATION
#include "../base.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(ContextInit) {
    try {
        Web3::defaultContext = std::make_shared<Web3::Context>("127.0.0.1", "7545", 1);
        // Web3::defaultContext = std::make_shared<Web3::Context>("rpc.sepolia.dev", "443", 11155111, true);
        Web3::defaultContext->run();        
    } catch (std::exception &e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}
