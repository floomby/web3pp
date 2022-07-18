#pragma once

#include <boost/multiprecision/cpp_int.hpp>

#include "base.h"
#include "net.h"

namespace Web3 {
namespace Ethereum {

inline boost::multiprecision::uint256_t blockHeight(std::shared_ptr<Context> context = defaultContext) {
    auto str = context->buildRPCJson("eth_blockNumber", "[]");
    boost::json::value results;
    try {
        results = std::make_shared<Web3::Net::SyncRPC>(context, std::move(str))->call();
    } catch (const std::exception &e) {
        throw std::runtime_error("Unable to get block number: " + std::string(e.what()));
    }
    if (results.as_object().contains("error")) {
        throw std::runtime_error("Unable to get block number: " + value_to<std::string>(results.at("error").at("message")));
    }
    return fromString(value_to<std::string>(results.at("result")));
}

inline TransactionHash sendRawTransaction(const std::string &tx, std::shared_ptr<Context> context = defaultContext) {
    auto str = context->buildRPCJson("eth_sendRawTransaction", "[\"0x" + tx + "\"]");
    boost::json::value results;
    try {
        results = std::make_shared<Web3::Net::SyncRPC>(context, std::move(str))->call();
    } catch (const std::exception &e) {
        throw std::runtime_error("Unable to send raw transaction: " + std::string(e.what()));
    }
    if (results.as_object().contains("error")) {
        throw std::runtime_error("Unable to send raw transaction: " + value_to<std::string>(results.at("error").at("message")));
    }
    return {context, value_to<std::string>(results.at("result"))};
}

}  // namespace Ethereum
}  // namespace Web3