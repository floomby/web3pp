#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/hmac.h>
#include <openssl/obj_mac.h>
#include <openssl/opensslv.h>
#include <openssl/sha.h>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/random/random_device.hpp>
#include <cstdlib>
#include <forward_list>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#define WEB3_IMPLEMENTATION
#include "account.h"
#include "encoder.h"
#include "transaction.h"
#include "utility.h"

int main(int argc, char **argv) {
    try {
        // Web3::defaultContext = std::make_shared<Web3::Context>("127.0.0.1", "7545", 1);
        Web3::defaultContext = std::make_shared<Web3::Context>("rpc.sepolia.dev", "443", 11155111, true);
        Web3::defaultContext->run();

        auto account2 = Web3::Account(std::string{"af817fc51a733a9387f135d605106f2a391cd689822cc5721f2d81df9fd64e1d"});
        std::cout << account2.getAddress() << " - " << account2.getBalance() << std::endl;

        Web3::Account account(std::string{"387e50a4fa783cb44d1d579a0810f169e81f5d2e705615c483aa3c208d25f966"});

        Web3::Transaction tx{std::stoul(argv[1]), 0x04a817c800, 0x021000, Web3::hexToBytes("bcc18C958EaE2fd41E21B59Efc626205d8982107"), Web3::fromString("0xDE0B6B3A7640000")};
        auto signedTx = tx.sign(account);

        std::cout << "Signed Tx: " << Web3::toString(signedTx) << std::endl;
        account.sendRawTransaction(Web3::toString(signedTx));

        auto handler = [](const std::string &str){ std::cout << "Completed" << std::endl; };
        std::make_shared<Web3::Net::AsyncRPC<decltype(handler)>>(Web3::defaultContext, handler, "{\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\",\"params\":[],\"id\":5777}")->call();
    } catch (...) {
        std::exception_ptr p = std::current_exception();
        std::cout << (p ? p.__cxa_exception_type()->name() : "null") << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}