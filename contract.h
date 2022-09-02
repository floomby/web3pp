#pragma once

#include <boost/multiprecision/cpp_dec_float.hpp>
#include <iostream>

#include "account.h"
#include "callOptions.h"
#include "ethereum.h"
#include "transaction.h"
#include "units.h"
#include "gasEstimator.h"

namespace Web3 {

template <typename F, typename P, typename... Ts> struct CallWrapper {
    std::string name;
    F f;
    std::shared_ptr<std::promise<P>> promise;
    // CallWrapper(F &&f, std::string &&name) : name(std::move(name)), f(std::move(f)) {}
    CallWrapper(F &&f, std::string &&name, decltype(promise) prom) : name(std::move(name)), f(std::move(f)), promise(prom) {}
    void operator()(const std::string &str){ 
        boost::iostreams::stream<boost::iostreams::array_source> stream(str.c_str(), str.size());
        boost::property_tree::ptree results;
        boost::property_tree::read_json(stream, results);
        if (results.get_child_optional( "error")) {
            std::clog << "Unable to call function (" + name + "): " + results.get<std::string>("error.message") + "\\n";
        }
        auto bytes = Web3::hexToBytes(results.get<std::string>("result"));
        // std::cout << "Result: " << toString(bytes) << std::endl;
        if constexpr (std::is_same_v<P, void>) {
            f(Web3::Encoder::ABIDecodeTo<Ts...>(bytes));    
            promise->set_value();
        } else {
            promise->set_value(f(Web3::Encoder::ABIDecodeTo<Ts...>(bytes)));
        }
    };
};

// TODO We should have deploy_async 
// TODO deploy should not be a template because that defeats the purpose of having this static typing
class Contract {
   protected:
    template<typename...Ts> auto deploy(const CallOptions &options, Ts... args) {
        // TODO find gas price and estimate gas
        auto encoded = Web3::Encoder::ABIEncode(args...);
        std::string data = __data() + toString(encoded);
        boost::multiprecision::uint256_t txGas;
        size_t nonce;
        if (options.nonce) {
            nonce = *options.nonce;
        } else {
            nonce = context->signers.front()->getTransactionCount();
        }
        if (options.gasLimit) {
            txGas = *options.gasLimit;
        } else {
            auto gas = GasEstimator::estimateGas(context->signers.front()->address, "0", data.c_str(), {}, this->context);
            boost::multiprecision::cpp_dec_float_50 gasF = fromString(gas).convert_to<boost::multiprecision::cpp_dec_float_50>() * gasMult;
            txGas = gasF.convert_to<boost::multiprecision::uint256_t>();
        }
        boost::multiprecision::uint256_t txPrice;
        if (options.gasPrice) {
            txPrice = *options.gasPrice;
        } else {
            txPrice = Units::gwei(10);
        }
        boost::multiprecision::uint256_t txValue;
        if (options.value) {
            txValue = *options.value;
        } else {
            txValue = 0;
        }
        Web3::Transaction tx{nonce, txPrice, txGas, {}, txValue, Web3::hexToBytes(data.c_str())};
        std::shared_ptr<Account> signer;
        if (options.account) {
            signer = options.account;
        } else {
            if (!context || context->signers.empty()) throw std::runtime_error("No primary signer set");
            signer = context->signers.front();
        }
        auto signedTx = signer->signTx(tx);
        auto h = Ethereum::sendRawTransaction(Web3::toString(signedTx));
        address = signer->deployedContract(nonce);
        return h.getReceipt();
    }
    const boost::multiprecision::cpp_dec_float_50 gasMult = boost::multiprecision::cpp_dec_float_50(1.2);
    std::shared_ptr<Context> context;

   public:
    virtual const char *__data() = 0;
    Address address;
    Contract(std::shared_ptr<Context> context) : context(context) {
        if (!context) throw std::runtime_error("Contract construction requires valid context");
    }
    Contract(const Address &address, std::shared_ptr<Context> context) : context(context), address(address) {
        if (!context) throw std::runtime_error("Contract construction requires valid context");
    }
    Contract(Address &&address, std::shared_ptr<Context> context) : context(context), address(std::move(address)) {
        if (!context) throw std::runtime_error("Contract construction requires valid context");
    }
};

}  // namespace Web3