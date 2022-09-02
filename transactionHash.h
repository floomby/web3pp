#pragma once

#include <iostream>

#include "net.h"

namespace Web3 {
struct TransactionHash {
    std::shared_ptr<Context> context;
    std::string hash;
    auto getReceipt() {
        auto str = context->buildRPCJson("eth_getTransactionReceipt", "[\"" + hash + "\"]");
        boost::property_tree::ptree results;
        try {
            results = std::make_shared<Web3::Net::SyncRPC>(context, std::move(str))->call();
        } catch (const std::exception &e) {
            throw std::runtime_error("Unable to get transaction receipt: " + std::string(e.what()));
        }
        if (results.get_child_optional( "error")) {
            throw std::runtime_error("Unable to get transaction receipt: " + results.get<std::string>("error.message"));
        }
        return results.get_child("result");
    }
};
}

namespace std {
inline ostream &operator<<(ostream &os, const Web3::TransactionHash &transactionHash) {
    os << transactionHash.hash;
    return os;
}
}
