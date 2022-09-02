// Well now this is a bit of a mess
// I should have designed it before just coding it like a monkey (Monkey no think, monkey just do)
// (I will try and rewrite this at some point)

import fs from "fs";
import assert from "assert";
import keccak256 from "keccak256";

const abi = fs.readFileSync(process.argv[2] + ".abi", "utf8");
const bin = fs.readFileSync(process.argv[2] + ".bin", "utf8").trim().replace(/^0x/, "");
const parsed = JSON.parse(abi);

const retType = (t: string) => {
  const fixedMatcher = /^(u?fixed)(([\d]+)x([\d]+))?$/;
  const match = fixedMatcher.exec(t);

  if (t === "address") {
    return `Web3::Address`;
  } else if (t.match(/^uint/)) {
    return `boost::multiprecision::uint256_t`;
  } else if (t.match(/^int/)) {
    return `boost::multiprecision::int256_t`;
  } else if (t === "bool") {
    return `bool`;
  } else if (t === "string") {
    return `std::string`;
  } else if (t === "bytes") {
    return `std::vector<unsigned char>`;
  } else if (match) {
    let tp: string;
    if (match[1] === "fixed") {
      tp = "Web3::Fixed<true,"
    } else {
      assert(match[1] === "ufixed");
      tp = "Web3::Fixed<false,"
    }
    const m = match[3] || "128";
    const n = match[4] || "18";
    return `${tp} ${m}, ${n}>`;
  } else {
    throw new Error(`Unsupported type: ${t}`);
  }
};

const argType = (t: string) => {
  const fixedMatcher = /^(u?fixed)(([\d]+)x([\d]+))?$/;
  const match = fixedMatcher.exec(t);

  if (t === "address") {
    return `const Web3::Address &`;
  } else if (t.match(/^uint/)) {
    return `const boost::multiprecision::uint256_t &`;
  } else if (t.match(/^int/)) {
    return `const boost::multiprecision::int256_t &`;
  } else if (t === "bool") {
    return `bool`;
  } else if (t === "string") {
    return `const std::string &`;
  } else if (t === "bytes") {
    return `const std::vector<unsigned char> &`;
  } else if (match) {
    let tp: string;
    if (match[1] === "fixed") {
      tp = "Web3::Fixed<true,"
    } else {
      assert(match[1] === "ufixed");
      tp = "Web3::Fixed<false,"
    }
    const m = match[3] || "128";
    const n = match[4] || "18";
    return `const ${tp} ${m}, ${n}> &`;
  } else {
    throw new Error(`Unsupported type: ${t}`);
  }
};

// const retTypeComposite = (ts: string[]) => {
//   return "void";
//   if (ts.length === 0) {
//     return "void";
//   } else if (ts.length === 1) {
//     return retType(ts[0]);
//   } else {
//     return `std::tuple<${ts.map(retType).join(", ")}>`;
//   }
// };

const indent = (x: string) => {
  return x.split("\n").map((l: string) => `    ${l}`).join("\n");
};

const argTypes = (args: string[]) => {
  return args.map((y: string, idx: number) => argType(y) + `arg${idx}`).join(", ");
};

const tuplize = (x: string, count: number) => {
  if (count > 1) return `std::make_tuple(${x})`;
  return x;
}

const fm = (x: string) => `    ${x}\n`;

const caller = (tx: boolean, name: string, outputs: string[]) => {
  return tx ? `
    boost::multiprecision::uint256_t gasLimit;
    boost::multiprecision::uint256_t txValue;
    if (options.value) {
        txValue = *options.value;
    } else {
        txValue = 0;
    }
    std::shared_ptr<Web3::Account> signer;
    if (options.account) {
        signer = options.account;
    } else {
        if (!context || context->signers.empty()) throw std::runtime_error("No primary signer set");
        signer = context->signers.front();
    }
    size_t nonce;
    if (options.nonce) {
        nonce = *options.nonce;
    } else {
        nonce = signer->getTransactionCount();
    }
    if (options.gasLimit) {
        gasLimit = *options.gasLimit;
    } else {
        auto gas = Web3::GasEstimator::estimateGas(signer->address, Web3::toString(txValue).c_str(), encoded, this->address, this->context);
        boost::multiprecision::cpp_dec_float_50 gasF = Web3::fromString(gas).convert_to<boost::multiprecision::cpp_dec_float_50>() * gasMult;
        gasLimit = gasF.convert_to<boost::multiprecision::uint256_t>();
    }
    boost::multiprecision::uint256_t gasPrice;
    if (options.gasPrice) {
        gasPrice = *options.gasPrice;
    } else {
        gasPrice = Web3::Units::gwei(10);
    }
    Web3::Transaction tx{nonce, gasPrice, gasLimit, this->address, txValue, encoded};
    auto signedTx = signer->signTx(tx);
    auto h = Web3::Ethereum::sendRawTransaction(Web3::toString(signedTx));
    return h.getReceipt();` : `
    std::shared_ptr<Web3::Account> signer;
    if (options.account) {
        signer = options.account;
    } else {
        if (!context || context->signers.empty()) throw std::runtime_error("No primary signer set");
        signer = context->signers.front();
    }
    auto str = context->buildRPCJson("eth_call", "[" + Web3::optionBuilder({
        {"from", signer->address.asString()},
        {"to", address.asString()},
        {"data", "0x" + Web3::toString(encoded)}
    }) + ",\\"latest\\"]");
    boost::property_tree::ptree results;
    try {
        results = std::make_shared<Web3::Net::SyncRPC>(context, std::move(str))->call();
    } catch (const std::exception &e) {
        throw std::runtime_error("Unable to call function (${name}): " + std::string(e.what()));
    }
    if (results.get_child_optional( "error")) {
        throw std::runtime_error("Unable to call function (${name}): " + results.get<std::string>("error.message"));
    }
    auto bytes = Web3::hexToBytes(results.get<std::string>("result"));
    return Web3::Encoder::ABIDecodeTo<true, ${outputs.map(retType).join(", ")}>(bytes);`;
}

// Confusing, high class mess right here, hopefully I don't have to touch this except for adding support for other transaction types
const caller_async = (tx: boolean, name: string, outputs: string[]) => {
  return `
    auto promise = std::make_shared<std::promise<return_type_t<F>>>();` +
  (tx ? `
    std::shared_ptr<Web3::Account> signer;
    if (options.account) {
        signer = options.account;
    } else {
        if (!context || context->signers.empty()) throw std::runtime_error("No primary signer set");
        signer = context->signers.front();
    }
    boost::multiprecision::uint256_t txValue;
    if (options.value) {
        txValue = *options.value;
    } else {
        txValue = 0;
    }
    boost::multiprecision::uint256_t gasPrice;
    if (options.gasPrice) {
        gasPrice = *options.gasPrice;
    } else {
        gasPrice = Web3::Units::gwei(10);
    }
    std::optional<boost::multiprecision::uint256_t> gasLimit = std::nullopt;
    if (options.gasLimit) {
        gasLimit = *options.gasLimit;
    }
    auto rawSender = [address = this->address, context = this->context, signer, txValue, gasPrice, encoded, func = std::move(func), promise](size_t nonce, const boost::multiprecision::uint256_t &gasLimit) {
        Web3::Transaction tx{nonce, gasPrice, gasLimit, address, txValue, encoded};
        auto signedTx = signer->signTx(tx);
        auto str = context->buildRPCJson("eth_sendRawTransaction", "[\\"0x" + Web3::toString(signedTx) + "\\"]");
        std::make_shared<Web3::Net::AsyncRPC<decltype(func), false>>(context, std::move(func), std::move(str), promise)->call();
    };
    auto gasGetter = [address = this->address, context = this->context, gasMult = this->gasMult, signer, txValue, gasPrice, gasLimit, encoded, rawSender](size_t nonce) {
        if (gasLimit) {
            rawSender(nonce, *gasLimit);
        } else {
            auto handler = [rawSender, nonce, gasMult](const std::string &gasEstimation) {
                boost::multiprecision::cpp_dec_float_50 gasF = Web3::fromString(gasEstimation).convert_to<boost::multiprecision::cpp_dec_float_50>() * gasMult;
                rawSender(nonce, gasF.convert_to<boost::multiprecision::uint256_t>());
            };
            Web3::GasEstimator::estimateGas_async(std::move(handler), signer->address, Web3::toString(txValue).c_str(), encoded, address, context);
        }
    };
    if (options.nonce) {
        gasGetter(*options.nonce);
    } else {
        signer->getTransactionCount_async(std::move(gasGetter));
    }` : `
    std::shared_ptr<Web3::Account> signer;
    if (options.account) {
        signer = options.account;
    } else {
        if (!context || context->signers.empty()) throw std::runtime_error("No primary signer set");
        signer = context->signers.front();
    }
    auto str = context->buildRPCJson("eth_call", "[" + Web3::optionBuilder({
        {"from", signer->address.asString()},
        {"to", address.asString()},
        {"data", "0x" + Web3::toString(encoded)}
    }) + ",\\"latest\\"]");
    try {
        if constexpr (std::is_same_v<return_type_t<F>, void>) {
            auto handler = Web3::CallWrapper<decltype(func), return_type_t<F>, ${outputs.map(retType).join(", ")}>(std::move(func), "${name}");
            std::make_shared<Web3::Net::AsyncRPC<decltype(handler)>>(context, std::move(handler), std::move(str))->call();
        } else {
            auto handler = Web3::CallWrapper<decltype(func), return_type_t<F>, ${outputs.map(retType).join(", ")}>(std::move(func), "${name}", promise);
            std::make_shared<Web3::Net::AsyncRPC<decltype(handler)>>(context, std::move(handler), std::move(str))->call();
        }
    } catch (const std::exception &e) {
        throw std::runtime_error("Unable to call function (${name}): " + std::string(e.what()));
    }`) + `
    return promise;`;
}

const argLister = (argCount: number) => Array(argCount).fill(1).map((v:number, idx:number) => `arg${idx}`).join(", ");

const body_ = (argCount: number, sig: string, tx: boolean, outputs: string[], c: (tx: boolean, name: string, outputs: string[]) => string) => {
  const callable = `if (address.isZero()) throw std::runtime_error("Contract must have an address");`;
  const h = keccak256(sig).toString('hex');
  const sel = `std::vector<unsigned char> selector{0x${h.substring(0,2)}, 0x${h.substring(2,4)}, 0x${h.substring(4,6)}, 0x${h.substring(6,8)}};`;
  const encoded = `auto encoded = Web3::Encoder::ABIEncode(${argLister(argCount)});`;
  const append = `encoded.insert(encoded.begin(), selector.begin(), selector.end());`;
  return `{\n${fm(callable)}${fm(sel)}${fm(encoded)}${fm(append)}${c(tx, sig.split('(')[0], outputs)}\n}`;
};

const body = (argCount: number, sig: string, tx: boolean, outputs: string[]) => body_(argCount, sig, tx, outputs, caller);
const body_async = (argCount: number, sig: string, tx: boolean, outputs: string[]) => body_(argCount, sig, tx, outputs, caller_async);

const className = ((x:string) => x.charAt(0).toUpperCase() + x.slice(1))(process.argv[2].split(".")[0]).replace("-", "_");

console.log(
`// This is an auto-generated file.
#pragma once
#include "abi.h"
#include "contract.h"

class ${className} : public Web3::Contract {
   public:
    ${className}(std::shared_ptr<Web3::Context> context = Web3::defaultContext) : Contract(context) {};
    ${className}(const Web3::Address &address, std::shared_ptr<Web3::Context> context = Web3::defaultContext) : Contract(address, context) {}
    ${className}(Web3::Address &&address, std::shared_ptr<Web3::Context> context = Web3::defaultContext) : Contract(std::move(address), context) {}`);

const comma = (x:string) => (x.length > 0 ? `${x}, ` : "");

const con = parsed.filter((x: any) => x.type === "constructor")[0];
if (con) {
  const conInputs = con.inputs.map((y: any) => y.type);
  
  console.log(
`    void deploy(${argTypes(conInputs)}, const Web3::CallOptions &options = {}) {
        Web3::Contract::deploy(options, ${argLister(conInputs.length)});
    }`);
} else {
  // Even if we don't have a constructor in the abi we will still generate a deploy function
  console.log(
`    void deploy(const Web3::CallOptions &options = {}) {
        Web3::Contract::deploy(options);
    }`);  
}

parsed.filter((x: any) => x.type === "function").forEach((x: any) => {
  const name = x.name;
  const inputs = x.inputs.map((y: any) => y.type);
  const outputs = x.outputs.map((y: any) => y.type as string);
  const mut = x.stateMutability;
  const signature = `${name}(${inputs.join(",")})`;
  const builder = indent(`inline auto ${name}(${comma(argTypes(inputs))}const Web3::CallOptions &options = {}) ${body(inputs.length, signature, mut == "payable" || mut == "nonpayable", outputs)}`);
  const builder_async = indent(`template <typename F> std::shared_ptr<std::promise<return_type_t<F>>> ${name}_async(${comma(argTypes(inputs))}F &&func, const Web3::CallOptions &options = {}) ${body_async(inputs.length, signature, mut == "payable" || mut == "nonpayable", outputs)}`);
  console.log(builder);
  console.log(builder_async);
});

console.log(indent(`virtual inline const char *__data() { return "0x${bin}"; }`));

console.log(`};`);