#pragma once

#include <boost/multiprecision/cpp_dec_float.hpp>
#include <iostream>

#include "account.h"
#include "transaction.h"

namespace Web3 {

template <typename F, typename... Ts> struct CallWrapper {
    std::string name;
    F f;
    CallWrapper(F &&f, std::string &&name) : name(std::move(name)), f(std::move(f)) {}
    void operator()(const std::string &str){ 
        auto results = boost::json::parse(str);
        if (results.as_object().contains("error")) {
            std::clog << "Unable to call function (" + name + "): " + value_to<std::string>(results.at("error").at("message")) + "\\n";
        }
        auto bytes = Web3::hexToBytes(value_to<std::string>(results.at("result")));
        // std::cout << "Result: " << toString(bytes) << std::endl;
        f(Web3::Encoder::ABIDecodeTo<Ts...>(bytes));
    };
};

class Contract {
   protected:
    const boost::multiprecision::cpp_dec_float_50 gasMult = boost::multiprecision::cpp_dec_float_50(1.2);
    std::shared_ptr<Context> context;

   public:
    virtual const char *__data() = 0;
    Address address;
    template<typename...Ts> void deploy(Ts... args) {
        if (!context || context->signers.empty()) throw std::runtime_error("No primary signer set");
        // TODO find gas price and estimate gas
        auto encoded = Web3::Encoder::ABIEncode(args...);
        std::string data = __data() + toString(encoded);
        auto gas = this->estimateGas(context->signers.front()->address, "0", data.c_str());
        const auto nonce = context->signers.front()->getTransactionCount();
        boost::multiprecision::cpp_dec_float_50 gasF = fromString(gas).convert_to<boost::multiprecision::cpp_dec_float_50>() * gasMult;
        Web3::Transaction tx{nonce, 0x04a817c800, gasF.convert_to<boost::multiprecision::uint256_t>(), std::vector<unsigned char>({}),
            Web3::fromString("00"), Web3::hexToBytes(data.c_str())};

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
        return value_to<std::string>(results.at("result"));
    }
    template <typename F>
    void estimateGas_async(F &&func, Address from, const char *value = "", const char *data = "", Address to = Address{}) {
        std::vector<std::pair<std::string, std::string>> args;
        args.push_back(std::make_pair("from", from.asString()));
        if (strlen(value) > 0) {
            args.push_back(std::make_pair("value", value));
        }
        if (strlen(data) > 0) {
            args.push_back(std::make_pair("data", data));
        }
        if (!to.isZero()) {
            args.push_back(std::make_pair("to", to.asString()));
        }
        auto str = context->buildRPCJson("eth_estimateGas", "[" + optionBuilder(args) + ",\"latest\"]");
        auto handler = [func = std::move(func)](const std::string &str) {
            auto results = boost::json::parse(str);
            if (results.as_object().contains("error")) {
                std::clog << "Unable to estimate gas: " + value_to<std::string>(results.at("error").at("message")) + "\n";
            } else {
                func(value_to<std::string>(results.at("result")));
            }
        };
        try {
            std::make_shared<Web3::Net::AsyncRPC<decltype(handler)>>(context, std::move(handler), std::move(str))->call();
        } catch (const std::exception &e) {
            throw std::runtime_error("Unable to estimate gas: " + std::string(e.what()));
        }
    }
};

}  // namespace Web3