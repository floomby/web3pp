#pragma once

#include <type_traits>

#include "utility.h"

namespace Web3 {

namespace Encoder {

template <typename T>
std::vector<unsigned char> ABIEncode(const T &data) {
    if constexpr (std::is_same_v<T, boost::multiprecision::uint256_t> || std::is_unsigned_v<T>) {
        return padFrontTo(integralToBytes(data), 32);
    } else if constexpr (std::is_same_v<T, boost::multiprecision::int256_t> || std::is_signed_v<T>) {
        // Need to sign extend signed numbers
        return padFrontTo(integralToBytes(data), 32, true);
    } else if constexpr (std::is_same_v<T, Address>) {
        return padFrontTo(data.bytes, 32);
    } else if constexpr (is_std_array<T>::value) {
        // internally we use array<unsigned char, N> to represent bytesN
        static_assert(std_array_size<T>::value <= 32, "bytesN can only have up to 32 bytes");
        return padBackTo(data, 32);
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