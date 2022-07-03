#pragma once

#include <type_traits>

#include "utility.h"

namespace Web3 {

namespace Encoder {

template <typename T>
std::vector<unsigned char> ABIEncode(const T &data) {
    if constexpr (std::is_same_v<T, boost::multiprecision::uint256_t> || std::is_unsigned_v<T>) {
        return padFrontTo(arithmeticToBytes(data), 32);
    } else if constexpr (std::is_same_v<T, boost::multiprecision::int256_t> || std::is_signed_v<T>) {
        return padFrontTo(arithmeticToBytes(data), 32, true);
    } else if constexpr (std::is_same_v<T, Address>) {
        return padFrontTo(data, 32);
    } else {
        static_assert(always_false<T>, "Unsupported type");
    }
}

}

class Abi {
   public:
    inline Abi(const std::string &filename) {
        
    }
};
}