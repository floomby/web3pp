#pragma once

#include "encoder.h"

namespace Web3 {

struct Signature {
    std::vector<unsigned char> v;
    std::vector<unsigned char> r;
    std::vector<unsigned char> s;
};

class Transaction {
   public:
    size_t nonce;
    boost::multiprecision::uint256_t gasPrice;
    boost::multiprecision::uint256_t gasLimit;
    std::optional<Address> to;
    boost::multiprecision::uint256_t value;
    std::vector<unsigned char> data;
    Signature signature;
};

}  // namespace Web3