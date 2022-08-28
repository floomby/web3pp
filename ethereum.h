#pragma once

#include <boost/multiprecision/cpp_int.hpp>

#include "base.h"
#include "net.h"

namespace Web3 {
namespace Ethereum {

inline boost::multiprecision::uint256_t blockHeight(std::shared_ptr<Context> context = defaultContext) {
    auto str = context->buildRPCJson("eth_blockNumber", "[]");
    boost::property_tree::ptree results;
    try {
        results = std::make_shared<Web3::Net::SyncRPC>(context, std::move(str))->call();
    } catch (const std::exception &e) {
        throw std::runtime_error("Unable to get block number: " + std::string(e.what()));
    }
    if (results.get_child_optional( "error")) {
        throw std::runtime_error("Unable to get block number: " + results.get<std::string>("error.message"));
    }
    return fromString(results.get<std::string>("result"));
}

inline TransactionHash sendRawTransaction(const std::string &tx, std::shared_ptr<Context> context = defaultContext) {
    auto str = context->buildRPCJson("eth_sendRawTransaction", "[\"0x" + tx + "\"]");
    boost::property_tree::ptree results;
    try {
        results = std::make_shared<Web3::Net::SyncRPC>(context, std::move(str))->call();
    } catch (const std::exception &e) {
        throw std::runtime_error("Unable to send raw transaction: " + std::string(e.what()));
    }
    if (results.get_child_optional( "error")) {
        throw std::runtime_error("Unable to send raw transaction: " + results.get<std::string>("error.message"));
    }
    return {context, results.get<std::string>("result")};
}

}  // namespace Ethereum
}  // namespace Web3