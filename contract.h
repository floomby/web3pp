#pragma once

#include <boost/multiprecision/cpp_dec_float.hpp>
#include <iostream>

#include "account.h"

namespace Web3 {
class Contract {
    const boost::multiprecision::cpp_dec_float_50 gasMult = boost::multiprecision::cpp_dec_float_50(1.2);
    std::shared_ptr<Context> context;

   public:
    virtual const char *__data() = 0;
    Address address;
    inline void deploy() {
        if (!context || context->signers.empty()) throw std::runtime_error("No primary signer set");
        // TODO find gas price and estimate gas
        auto gas = this->estimateGas(context->signers.front()->address, "0", __data());
        const auto nonce = context->signers.front()->getTransactionCount();
        boost::multiprecision::cpp_dec_float_50 gasF = fromString(gas).convert_to<boost::multiprecision::cpp_dec_float_50>() * gasMult;
        Web3::Transaction tx{nonce, 0x04a817c800, gasF.convert_to<boost::multiprecision::uint256_t>(), std::vector<unsigned char>({}),
            Web3::fromString("00"), Web3::hexToBytes(this->__data())};

        auto signedTx = tx.sign(*context->signers.front());
        auto h = context->signers.front()->sendRawTransaction(Web3::toString(signedTx));
        std::cout << "Hash: " << h << std::endl;
        address = context->signers.front()->deployedContract(nonce);
        h.getReceipt();
    }
    Contract(std::shared_ptr<Context> context) : context(context) {
        if (!context) throw std::runtime_error("Contract construction requires valid context");
    }
    Contract(const Address &address, std::shared_ptr<Context> context) : context(context), address(address) {
        if (!context) throw std::runtime_error("Contract construction requires valid context");
    }
    Contract(Address &&address, std::shared_ptr<Context> context) : context(context), address(std::move(address)) {
        if (!context) throw std::runtime_error("Contract construction requires valid context");
    }
    std::string estimateGas(Address from, const char *value, const char *data = "", Address to = Address{}) {
        std::stringstream args;
        if (to.isZero()) {
            args << "[{\"from\":\"" << from.asString() << "\",\"value\":\"" << value << "\",\"data\":\"" << data << "\"},\"latest\"]";
        } else {
            args << "[{\"from\":\"" << from.asString() << "\",\"to\":\"" << to.asString() << "\",\"value\":\"" << value << "\",\"data\":\"" << data << "\"},\"latest\"]";
        }
        auto str = context->buildRPCJson("eth_estimateGas", args.str());
        boost::json::value results;
        try {
            results = std::make_shared<Web3::Net::SyncRPC>(context, std::move(str))->call();
        } catch (const std::exception &e) {
            throw std::runtime_error("Unable to estimate gas: " + std::string(e.what()));
        }
        if (results.as_object().contains("error")) {
            throw std::runtime_error("Unable to estimate gas: " + value_to<std::string>(results.at("error").at("message")));
        }
        std::cout << "Estimation: " << results.at("result") << std::endl;
        return value_to<std::string>(results.at("result"));
    }
};
}  // namespace Web3