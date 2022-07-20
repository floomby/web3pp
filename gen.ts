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
    if (!context || context->signers.empty()) throw std::runtime_error("No primary signer set");
    const auto nonce = context->signers.front()->getTransactionCount();
    auto gas = estimateGas(context->signers.front()->address, "0", Web3::toString(encoded).c_str(), address);
    boost::multiprecision::cpp_dec_float_50 gasF = Web3::fromString(gas).convert_to<boost::multiprecision::cpp_dec_float_50>() * gasMult;
    Web3::Transaction tx{nonce, 0x04a817c800, gasF.convert_to<boost::multiprecision::uint256_t>(), address.asVector(), Web3::fromString("00"), encoded};
    auto signedTx = tx.sign(*context->signers.front());
    auto h = Web3::Ethereum::sendRawTransaction(Web3::toString(signedTx));
    std::cout << "Hash: " << h << std::endl;
    address = context->signers.front()->deployedContract(nonce);
    h.getReceipt();` : `
    auto str = context->buildRPCJson("eth_call", "[" + Web3::optionBuilder({
        {"from", context->signers.front()->address.asString()},
        {"to", address.asString()},
        {"data", "0x" + Web3::toString(encoded)}
    }) + ",\\"latest\\"]");
    boost::json::value results;
    try {
        results = std::make_shared<Web3::Net::SyncRPC>(context, std::move(str))->call();
    } catch (const std::exception &e) {
        throw std::runtime_error("Unable to call function (${name}): " + std::string(e.what()));
    }
    if (results.as_object().contains("error")) {
        throw std::runtime_error("Unable to call function (${name}): " + value_to<std::string>(results.at("error").at("message")));
    }
    auto bytes = Web3::hexToBytes(value_to<std::string>(results.at("result")));
    return Web3::Encoder::ABIDecodeTo<true, ${outputs.map(retType).join(", ")}>(bytes);`;
}

const caller_async = (tx: boolean, name: string, outputs: string[]) => {
  return tx ? `
    if (!context || context->signers.empty()) throw std::runtime_error("No primary signer set");
    auto handler = [this, encoded](const std::string &gas) {
        boost::multiprecision::cpp_dec_float_50 gasF = Web3::fromString(gas).convert_to<boost::multiprecision::cpp_dec_float_50>() * this->gasMult;
        Web3::Transaction tx{1234, 0x04a817c800, gasF.convert_to<boost::multiprecision::uint256_t>(), address.asVector(), Web3::fromString("00"), encoded};
        auto signedTx = tx.sign(*context->signers.front());
        std::cout << "Signed transaction: " << Web3::toString(signedTx) << std::endl;
    };
    estimateGas_async(std::move(handler), context->signers.front()->address, "0", Web3::toString(encoded).c_str(), address);
    ` : `
    auto str = context->buildRPCJson("eth_call", "[" + Web3::optionBuilder({
        {"from", context->signers.front()->address.asString()},
        {"to", address.asString()},
        {"data", "0x" + Web3::toString(encoded)}
    }) + ",\\"latest\\"]");
    auto handler = Web3::CallWrapper<decltype(func), ${outputs.map(retType).join(", ")}>(std::move(func), "${name}");
    try {
        std::make_shared<Web3::Net::AsyncRPC<decltype(handler)>>(context, std::move(handler), std::move(str))->call();
    } catch (const std::exception &e) {
        throw std::runtime_error("Unable to call function (${name}): " + std::string(e.what()));
    }`;
}

const body_ = (argCount: number, sig: string, tx: boolean, outputs: string[], c: (tx: boolean, name: string, outputs: string[]) => string) => {
  const callable = `if (address.isZero()) throw std::runtime_error("Contract must have an address");`;
  const h = keccak256(sig).toString('hex');
  const sel = `std::vector<unsigned char> selector{0x${h.substring(0,2)}, 0x${h.substring(2,4)}, 0x${h.substring(4,6)}, 0x${h.substring(6,8)}};`;
  const encoded = `auto encoded = Web3::Encoder::ABIEncode(${Array(argCount).fill(1).map((v:number, idx:number) => `arg${idx}`).join(", ")});`;
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

parsed.filter((x: any) => x.type === "function").forEach((x: any) => {
  const name = x.name;
  const inputs = x.inputs.map((y: any) => y.type);
  const outputs = x.outputs.map((y: any) => y.type as string);
  const mut = x.stateMutability;
  const signature = `${name}(${inputs.join(",")})`;
  const builder = indent(`inline auto ${name}(${argTypes(inputs)}) ${body(inputs.length, signature, mut == "payable" || mut == "nonpayable", outputs)}`);
  const builder_async = indent(`template <typename F> void ${name}_async(${comma(argTypes(inputs))}F &&func) ${body_async(inputs.length, signature, mut == "payable" || mut == "nonpayable", outputs)}`);
  console.log(builder);
  console.log(builder_async);
});

console.log(indent(`virtual inline const char *__data() { return "0x${bin}"; }`));

console.log(`};`);