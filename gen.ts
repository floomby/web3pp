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

const body = (argCount: number, sig: string) => {
  const h = keccak256(sig).toString('hex');
  const sel = `std::vector<unsigned char> selector{0x${h.substring(0,2)}, 0x${h.substring(2,4)}, 0x${h.substring(4,6)}, 0x${h.substring(6,8)}};`;
  const encoded = `auto encoded = Web3::Encoder::ABIEncode(${tuplize(Array(argCount).fill(1).map((v:number, idx:number) => `arg${idx}`).join(", "), argCount)});`;
  const append = `encoded.insert(encoded.begin(), selector.begin(), selector.end());`;
  return `{\n    ${sel}\n    ${encoded}\n    ${append}\n}`;
};

const className = ((x:string) => x.charAt(0).toUpperCase() + x.slice(1))(process.argv[2].split(".")[0]);

console.log(
`#pragma once
#include "abi.h"
#include "contract.h"

class ${className} : public Contract {
   public:`);

parsed.filter((x: any) => x.type === "function").forEach((x: any) => {
  const name = x.name;
  const inputs = x.inputs.map((y: any) => y.type);
  const outputs = x.outputs.map((y: any) => y.type);
  const signature = `${name}(${inputs.join(",")})`;
  const builder = indent(`inline ${retTypeComposite(outputs)} ${name}(${argTypes(inputs)}) ${body(inputs.length, signature)}`);
  console.log(builder);
});

console.log(indent(`virtual inline const char *__data() { return "0x${bin}"; }\n`));

console.log(`};`);