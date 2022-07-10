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
#include "abi.h"

#include "demo.h"

int main(int argc, char **argv) {
    try {
        Web3::defaultContext = std::make_shared<Web3::Context>("127.0.0.1", "7545", 1);
        // Web3::defaultContext = std::make_shared<Web3::Context>("rpc.sepolia.dev", "443", 11155111, true);
        Web3::defaultContext->run();

        Demo demo;

        auto account2 = Web3::Account(std::string{"af817fc51a733a9387f135d605106f2a391cd689822cc5721f2d81df9fd64e1d"});
        // std::cout << account2.getAddress() << " - " << account2.getBalance() << std::endl;

        auto account = std::make_shared<Web3::Account>(std::string{"387e50a4fa783cb44d1d579a0810f169e81f5d2e705615c483aa3c208d25f966"});
        Web3::defaultContext->setPrimarySigner(account);
        try {
            demo.deploy();
            demo.decimals();
            demo.approve(Web3::Address{"0xE9F1ad5781ae7421F07F01005ecDD8327d122136"}, Web3::fromString("1000000000000"));
        } catch (std::exception &e) {
            std::cout << "Error: " << e.what() << std::endl;
        }


        // return EXIT_SUCCESS;
        // auto nonce = account.nonce;
        // Web3::Transaction tx{nonce, 0x04a817c800, 0x021000, Web3::hexToBytes("0000000000000000000000000000000000000000"), Web3::fromString("00"), Web3::hexToBytes(demo.__data())};
        // Web3::Transaction tx{nonce, 0x04a817c800, 0x0210000, std::vector<unsigned char>({}), Web3::fromString("00"), Web3::hexToBytes(demo.__data())};
        // Web3::Transaction tx{std::stoul(argv[1]), 0x04a817c800, 0x021000, Web3::hexToBytes("bcc18C958EaE2fd41E21B59Efc626205d8982107"), Web3::fromString("0xDE0B6B3A7640000")};
        // auto signedTx = tx.sign(account);

        // try {
        //     std::cout << "Signed Tx: " << Web3::toString(signedTx) << std::endl;
        //     auto h = account.sendRawTransaction(Web3::toString(signedTx));
        //     std::cout << "Hash: " << h << std::endl;
        //     std::cout << "Address should be (assuming I coded correctly): " << Web3::toString(account.deployedContract(nonce).bytes) << std::endl;
        //     h.getReceipt();
        // } catch (std::exception &e) {
        //     std::cout << "Error: " << e.what() << std::endl;
        // }

        // auto handler = [](const std::string &str){ std::cout << "Completed" << std::endl; };
        // std::make_shared<Web3::Net::AsyncRPC<decltype(handler)>>(Web3::defaultContext, handler, "{\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\",\"params\":[],\"id\":5777}")->call();
    } catch (...) {
        std::exception_ptr p = std::current_exception();
        std::cout << (p ? p.__cxa_exception_type()->name() : "null") << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}