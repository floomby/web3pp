#pragma once

#include <iostream>

#include "net.h"

namespace Web3 {
struct TransactionHash {
    std::shared_ptr<Context> context;
    std::string hash;
    void getReceipt() {
        auto str = context->buildRPCJson("eth_getTransactionReceipt", "[\"" + hash + "\"]");
        boost::json::value results;
        try {
            results = std::make_shared<Web3::Net::SyncRPC>(context, std::move(str))->call();
        } catch (const std::exception &e) {
            throw std::runtime_error("Unable to get transaction receipt: " + std::string(e.what()));
        }
        if (results.as_object().contains("error")) {
            throw std::runtime_error("Unable to get transaction receipt: " + value_to<std::string>(results.at("error").at("message")));
        }
        // std::cout << "Receipt: " << results.at("result") << std::endl;
    }
};
}

namespace std {
inline ostream &operator<<(ostream &os, const Web3::TransactionHash &transactionHash) {
    os << transactionHash.hash;
    return os;
}
}
