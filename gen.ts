import fs from "fs";
import assert from "assert";
import keccak256 from "keccak256";

const abi = fs.readFileSync(process.argv[2] + ".abi", "utf8");
const bin = fs.readFileSync(process.argv[2] + ".bin", "utf8");
const parsed = JSON.parse(abi);

const retType = (t: string) => {
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
  } else {
    throw new Error(`Unsupported type: ${t}`);
  }
};

const argType = (t: string) => {
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
  } else {
    throw new Error(`Unsupported type: ${t}`);
  }
};

const retTypeComposite = (ts: string[]) => {
  return "void";
  if (ts.length === 0) {
    return "void";
  } else if (ts.length === 1) {
    return retType(ts[0]);
  } else {
    return `std::tuple<${ts.map(retType).join(", ")}>`;
  }
};

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

const caller = (tx: boolean, name: string) => {
  return tx ? `
    if (!context || context->signers.empty()) throw std::runtime_error("No primary signer set");
    const auto nonce = context->signers.front()->getTransactionCount();
    auto gas = estimateGas(context->signers.front()->address, "0", Web3::toString(encoded).c_str(), address);
    boost::multiprecision::cpp_dec_float_50 gasF = Web3::fromString(gas).convert_to<boost::multiprecision::cpp_dec_float_50>() * gasMult;
    Web3::Transaction tx{nonce, 0x04a817c800, gasF.convert_to<boost::multiprecision::uint256_t>(), address.asVector(), Web3::fromString("00"), encoded};
    auto signedTx = tx.sign(*context->signers.front());
    auto h = context->signers.front()->sendRawTransaction(Web3::toString(signedTx));
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
    std::cout << "Returned: " << results.at("result") << std::endl;
    auto bytes = Web3::hexToBytes(value_to<std::string>(results.at("result")));`
}

const body = (argCount: number, sig: string, tx: boolean) => {
  const callable = `if (address.isZero()) throw std::runtime_error("Contract must have an address");`;
  const h = keccak256(sig).toString('hex');
  const sel = `std::vector<unsigned char> selector{0x${h.substring(0,2)}, 0x${h.substring(2,4)}, 0x${h.substring(4,6)}, 0x${h.substring(6,8)}};`;
  const encoded = `auto encoded = Web3::Encoder::ABIEncode(${Array(argCount).fill(1).map((v:number, idx:number) => `arg${idx}`).join(", ")});`;
  const append = `encoded.insert(encoded.begin(), selector.begin(), selector.end());`;
  return `{\n${fm(callable)}${fm(sel)}${fm(encoded)}${fm(append)}${caller(tx, sig.split('(')[0])}\n}`;
};

const className = ((x:string) => x.charAt(0).toUpperCase() + x.slice(1))(process.argv[2].split(".")[0]);

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

parsed.filter((x: any) => x.type === "function").forEach((x: any) => {
  const name = x.name;
  const inputs = x.inputs.map((y: any) => y.type);
  const outputs = x.outputs.map((y: any) => y.type);
  const mut = x.stateMutability;
  const signature = `${name}(${inputs.join(",")})`;
  const builder = indent(`inline ${retTypeComposite(outputs)} ${name}(${argTypes(inputs)}) ${body(inputs.length, signature, mut == "payable" || mut == "nonpayable")}`);
  console.log(builder);
});

console.log(indent(`virtual inline const char *__data() { return "0x${bin}"; }\n`));

console.log(`};`);