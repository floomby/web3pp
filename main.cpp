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
#include "utility.h"
#include "account.h"
#include "encoder.h"
#include "transaction.h"

int main(int argc, char **argv) {
    // TODO Encoder tests
    // std::cout << Web3::Encoder::encode(std::vector<unsigned char>{0x00, 0x45, 0x02, 0x00}) << std::endl;
    // std::cout << Web3::Encoder::encode(0x00) << std::endl;
    // std::cout << Web3::Encoder::encode(0x0010004593) << std::endl;
    return 0;

    Web3::defaultContext = std::make_shared<Web3::Context>("127.0.0.1", "7545", 1);

    Web3::Account account(std::string{"387e50a4fa783cb44d1d579a0810f169e81f5d2e705615c483aa3c208d25f966"});

    Web3::Transaction tx{std::stoul(argv[1]), 0x04a817c800, 0x021000, Web3::hexToBytes("bcc18C958EaE2fd41E21B59Efc626205d8982107"), Web3::fromString("0xDE0B6B3A7640000") };
    auto signedTx = tx.sign(account);    

    std::cout << "Signed Tx: " << Web3::toString(signedTx) << std::endl;
    account.sendRawTransaction(Web3::toString(signedTx));

    // TODO RLP Encoding test
    // Web3::Encoder::RLPEncodeInput input1({ 0x43, 0x44, 0x45 });
    // auto encoded = Web3::Encoder::encode(input1);
    // std::cout << "RLP Encoded: " << toString(encoded) << std::endl;

    // Web3::Encoder::RLPEncodeInput input2({ 0x47, 0x48, 0x49 });
    // for (int i = 0; i < 253; i++) {
    //     input1.value.value().push_back( 0x55);
    // }
    // Web3::Encoder::RLPEncodeInput input3({ input1, input2 });
    // auto encoded2 = Web3::Encoder::encode(input3);
    // std::cout << "RLP Encoded: " << toString(encoded2) << std::endl;

    // Web3::Encoder::RLPEncodeInput input4(uint256ToBytes(fromString("1024")));
    // auto encoded3 = Web3::Encoder::encode(input4);
    // std::cout << "RLP Encoded: " << toString(encoded3) << std::endl;


    // auto handler = [](const std::string &str){ std::cout << "Completed" << std::endl; };
    // std::make_shared<Web3::Net::AsyncRPC<decltype(handler)>>(Web3::defaultContext, handler, "{\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\",\"params\":[],\"id\":5777}")->call();
    // Web3::defaultContext->run();

    auto account2 = Web3::Account(std::string{"af817fc51a733a9387f135d605106f2a391cd689822cc5721f2d81df9fd64e1d"});
    std::cout << account2.getAddress() << " - " << account2.getBalance() << std::endl;
    // std::cout << account.getAddress() << " - " << account.getBalance() << std::endl;



    return EXIT_SUCCESS;
}