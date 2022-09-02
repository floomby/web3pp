#pragma once

#include "callOptions.h"
#include "ethereum.h"

namespace Web3 {
struct GasEstimator {
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
} // namespace Web3