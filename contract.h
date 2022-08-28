#pragma once

#include <boost/multiprecision/cpp_dec_float.hpp>
#include <iostream>

#include "account.h"
#include "callOptions.h"
#include "ethereum.h"
#include "transaction.h"
#include "units.h"

namespace Web3 {

template <typename F, typename... Ts> struct CallWrapper {
    std::string name;
    F f;
    CallWrapper(F &&f, std::string &&name) : name(std::move(name)), f(std::move(f)) {}
    void operator()(const std::string &str){ 
        boost::iostreams::stream<boost::iostreams::array_source> stream(str.c_str(), str.size());
        boost::property_tree::ptree results;
        boost::property_tree::read_json(stream, results);
        if (results.get_child_optional( "error")) {
            std::clog << "Unable to call function (" + name + "): " + results.get<std::string>("error.message") + "\\n";
        }
        auto bytes = Web3::hexToBytes(results.get<std::string>("result"));
        // std::cout << "Result: " << toString(bytes) << std::endl;
        f(Web3::Encoder::ABIDecodeTo<Ts...>(bytes));
    };
};

// TODO We should have deploy_async 
// TODO deploy should not be a template because that defeats the purpose of having this static typing
class Contract {
   protected:
    template<typename...Ts> void deploy(const CallOptions &options, Ts... args) {
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
            auto gas = this->estimateGas(context->signers.front()->address, "0", data.c_str());
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
        auto signedTx = tx.sign(*signer);
        auto h = Ethereum::sendRawTransaction(Web3::toString(signedTx));
        std::cout << "Hash: " << h << std::endl;
        address = signer->deployedContract(nonce);
        h.getReceipt();
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
    std::string estimateGas(Address from, const char *value, const char *data = "", Address to = Address{}) {
        std::stringstream args;
        if (to.isZero()) {
            args << "[{\"from\":\"" << from.asString() << "\",\"value\":\"" << value << "\",\"data\":\"" << data << "\"},\"latest\"]";
        } else {
            args << "[{\"from\":\"" << from.asString() << "\",\"to\":\"" << to.asString() << "\",\"value\":\"" << value << "\",\"data\":\"" << data << "\"},\"latest\"]";
        }
        auto str = context->buildRPCJson("eth_estimateGas", args.str());
        boost::property_tree::ptree results;
        try {
            results = std::make_shared<Web3::Net::SyncRPC>(context, std::move(str))->call();
        } catch (const std::exception &e) {
            throw std::runtime_error("Unable to estimate gas: " + std::string(e.what()));
        }
        if (results.get_child_optional( "error")) {
            throw std::runtime_error("Unable to estimate gas: " + results.get<std::string>("error.message"));
        }
        return results.get<std::string>("result");
    }
    std::string estimateGas(Address from, const char *value, const std::vector<unsigned char> &data, Address to = Address{}) {
        return estimateGas(from, value, toString(data).c_str(), to);
    }
    // This needs to be static since we are using it in other async code where the object may have been destroyed and we want to avoid dereferencing a bad this pointer
    template <typename F>
    static void estimateGas_async(F &&func, Address from, const char *value = "", const char *data = "", Address to = Address{}, std::shared_ptr<Context> context = defaultContext) {
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
            boost::iostreams::stream<boost::iostreams::array_source> stream(str.c_str(), str.size());
            boost::property_tree::ptree results;
            boost::property_tree::read_json(stream, results);
            if (results.get_child_optional( "error")) {
                std::clog << "Unable to estimate gas: " + results.get<std::string>("error.message") + "\n";
            } else {
                func(results.get<std::string>("result"));
            }
        };
        try {
            std::make_shared<Web3::Net::AsyncRPC<decltype(handler)>>(context, std::move(handler), std::move(str))->call();
        } catch (const std::exception &e) {
            std::clog << "Unable to estimate gas: " + std::string(e.what()) << std::endl;
        }
    }
    template <typename F>
    static void estimateGas_async(F &&func, Address from, const char *value, const std::vector<unsigned char> &data, Address to = Address{}, std::shared_ptr<Context> context = defaultContext) {
        estimateGas_async(std::move(func), from, value, toString(data).c_str(), to, context);
    }
};

}  // namespace Web3