#pragma once

#include <type_traits>

#include "utility.h"

namespace Web3 {

template <bool isSigned = true, size_t M = 128, size_t N = 18> struct Fixed {
    static_assert(8 <= M && M <= 256 && M % 8 == 0 && N <= 80, "Invalid Fixed size");
    static constexpr std::string TypeName() {
        return (isSigned ? std::string("fixed") : std::string("ufixed")) + std::to_string(M) + "x" + std::to_string(N);
    }
    Signedness<isSigned>::type underlying;
    decltype(underlying) abiEncodable() const {
        return underlying * boost::multiprecision::pow(boost::multiprecision::cpp_int(10), N).convert_to<typename Signedness<isSigned>::type>();
    }
};

template <class T> struct is_fixed : std::false_type {};
template <bool isSigned, size_t M, size_t N> struct is_fixed<Fixed<isSigned, M, N>> : std::true_type {};

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
    } else if constexpr (is_fixed<T>::value) {
        return ABIEncode(data.abiEncodable());
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