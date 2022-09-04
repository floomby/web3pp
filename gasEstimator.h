#pragma once

#include "callOptions.h"
#include "ethereum.h"

namespace Web3 {
struct GasEstimator {
    template <typename F, typename P = std::shared_ptr<std::promise<void>>>
    static std::shared_ptr<std::promise<return_type_t<F>>> estimateGas_async(F &&func, Address from, P promiseSharedPtr = nullptr, const char *value = "", const char *data = "", Address to = Address{}, std::shared_ptr<Context> context = defaultContext) {
        std::vector<std::pair<std::string, std::string>> args;
        auto promise = std::make_shared<std::promise<return_type_t<F>>>();
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
        auto handler = [func = std::move(func), promise, promiseSharedPtr](const std::string &str) {
            boost::iostreams::stream<boost::iostreams::array_source> stream(str.c_str(), str.size());
            boost::property_tree::ptree results;
            boost::property_tree::read_json(stream, results);
            if (results.get_child_optional( "error")) {
                if (promiseSharedPtr) {
                    promiseSharedPtr->set_exception(std::make_exception_ptr("Unable to estimate gas: " + results.get<std::string>("error.message")));
                } else {
                    promise->set_exception(std::make_exception_ptr("Unable to estimate gas: " + results.get<std::string>("error.message")));
                }
            } else {
                if constexpr (std::is_same_v<return_type_t<F>, void>) {
                    func(results.get<std::string>("result"));
                    promise->set_value();
                } else {
                    promise->set_value(func(results.get<std::string>("result")));
                }
            }
        };
        if (promiseSharedPtr) {
            std::make_shared<Net::AsyncRPC<P, decltype(handler)>>(promiseSharedPtr, context, std::move(handler), std::move(str))->call();
        } else {
            std::make_shared<Net::AsyncRPC<decltype(promise), decltype(handler)>>(promise, context, std::move(handler), std::move(str))->call();
        }
        return promise;
    }
    template <typename F, typename P = std::shared_ptr<std::promise<void>>>
    static void estimateGas_async(F &&func, Address from, P promiseSharedPtr, const char *value, const std::vector<unsigned char> &data, Address to = Address{}, std::shared_ptr<Context> context = defaultContext) {
        estimateGas_async(std::move(func), from, promiseSharedPtr, value, toString(data).c_str(), to, context);
    }
    static std::string estimateGas(Address from, const char *value, const char *data = "", Address to = Address{}, std::shared_ptr<Context> context = defaultContext) {
        std::stringstream args;
        if (to.isZero()) {
            args << "[{\"from\":\"" << from.asString() << "\",\"value\":\"" << value << "\",\"data\":\"" << data << "\"},\"latest\"]";
        } else {
            args << "[{\"from\":\"" << from.asString() << "\",\"to\":\"" << to.asString() << "\",\"value\":\"" << value << "\",\"data\":\"" << data << "\"},\"latest\"]";
        }
        auto str = context->buildRPCJson("eth_estimateGas", args.str());
        boost::property_tree::ptree results;
        try {
            results = std::make_shared<Net::SyncRPC>(context, std::move(str))->call();
        } catch (const std::exception &e) {
            throw std::runtime_error("Unable to estimate gas: " + std::string(e.what()));
        }
        if (results.get_child_optional( "error")) {
            throw std::runtime_error("Unable to estimate gas: " + results.get<std::string>("error.message"));
        }
        return results.get<std::string>("result");
    }
    static std::string estimateGas(Address from, const char *value, const std::vector<unsigned char> &data, Address to = Address{}, std::shared_ptr<Context> context = defaultContext) {
        return estimateGas(from, value, toString(data).c_str(), to, context);
    }
};
} // namespace Web3