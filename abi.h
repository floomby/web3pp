#pragma once

#include <string_view>
#include <type_traits>

#include "utility.h"

template <typename T>
constexpr auto type_name() {
    std::string_view name, prefix, suffix;
#ifdef __clang__
    name = __PRETTY_FUNCTION__;
    prefix = "auto type_name() [T = ";
    suffix = "]";
#elif defined(__GNUC__)
    name = __PRETTY_FUNCTION__;
    prefix = "constexpr auto type_name() [with T = ";
    suffix = "]";
#elif defined(_MSC_VER)
    name = __FUNCSIG__;
    prefix = "auto __cdecl type_name<";
    suffix = ">(void)";
#endif
    name.remove_prefix(prefix.size());
    name.remove_suffix(suffix.size());
    return name;
}

namespace Web3 {

template <bool isSigned = true, size_t M = 128, size_t N = 18>
struct Fixed {
    static_assert(8 <= M && M <= 256 && M % 8 == 0 && N <= 80, "Invalid Fixed size");
    static constexpr std::string TypeName() {
        return (isSigned ? std::string("fixed") : std::string("ufixed")) + std::to_string(M) + "x" + std::to_string(N);
    }
    Signedness<isSigned>::type underlying;
    decltype(underlying) abiEncodable() const {
        return underlying * boost::multiprecision::pow(boost::multiprecision::cpp_int(10), N).convert_to<typename Signedness<isSigned>::type>();
    }
};

template <class T>
struct is_fixed : std::false_type {};
template <bool isSigned, size_t M, size_t N>
struct is_fixed<Fixed<isSigned, M, N>> : std::true_type {};

namespace Encoder {

template <typename T>
bool constexpr is_dynamic() {
    using t = std::remove_cv_t<std::remove_reference_t<T>>;
    return std::is_same_v<t, std::vector<unsigned char>>;
}

template <typename T>
std::vector<unsigned char> ABIEncode(const T &data);

template <typename T, size_t Idx> std::vector<std::pair<bool, std::vector<unsigned char>>> &applier(std::vector<std::pair<bool, std::vector<unsigned char>>> &acm, const T &t) {
    auto res = ABIEncode(get<Idx>(t));
    acm.push_back({ is_dynamic<decltype(get<Idx>(t))>(), res });
    if constexpr (Idx > 0) {
        return applier<T, Idx - 1>(acm, t);
    }
    return acm;
}

inline std::vector<unsigned char> ABIEncode() {
    return {};
}
/**
 * @brief This function is complicated and not right
 * 
 * @tparam T type to encode (note that std::vector<unsigned char> is bytes and not uint8[] likewise std::array<unsigned char, N> is not uint8[N]
 * but is actually bytesN) I have an idea how to maybe fix this
 * @param data 
 * @return std::vector<unsigned char> abiencoded data
 */

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
        if constexpr (std::is_same_v<unsigned char, typename std_array_type<T>::type>) {
            // internally we use array<unsigned char, N> to represent bytesN for compile time types
            static_assert(std_array_size<T>::value <= 32, "bytesN can only have up to 32 bytes");
            return padBackTo(data, 32);
        } else {
            static_assert(always_false<T>, "std::array<T, N> can only have T = unsigned char");
        }
    } else if constexpr (is_fixed<T>::value) {
        return ABIEncode(data.abiEncodable());
    } else if constexpr (std::is_same_v<T, std::vector<unsigned char>>) {
        auto tmp = data;
        tmp = padBackTo(std::move(tmp), 32 * divRoundUp(data.size(), 32));
        auto lenEnc = ABIEncode(data.size());
        tmp.insert(tmp.begin(), lenEnc.begin(), lenEnc.end());
        return tmp;
    } else if constexpr (std::is_same_v<T, std::string>) {
        std::vector<unsigned char> tmp;
        tmp.reserve(data.size());
        std::copy(data.begin(), data.end(), std::back_inserter(tmp));
        tmp = padBackTo(std::move(tmp), 32 * divRoundUp(data.size(), 32));
        auto lenEnc = ABIEncode(data.size());
        tmp.insert(tmp.begin(), lenEnc.begin(), lenEnc.end());
        return tmp;
    } else if constexpr (is_std_tuple<T>::value) { 
        // TODO I am not sure this is correct
        if constexpr (std::tuple_size_v<T> == 0) return {};
        std::vector<std::pair<bool, std::vector<unsigned char>>> acm;
        applier<T, std::tuple_size_v<T> - 1>(acm, data);
        size_t dynSize = 0;
        std::vector<unsigned char> acm2, tails;
        for (auto &[dyn, val] : acm) {
            if (dyn) {
                auto offset = dynSize + std::tuple_size_v<T> * 32;
                auto offsetEnc = ABIEncode(offset);
                acm2.insert(acm2.end(), offsetEnc.begin(), offsetEnc.end());
                dynSize += val.size();
                tails.insert(tails.end(), val.begin(), val.end());
            } else {
                acm2.insert(acm2.end(), val.begin(), val.end());
            }
        }
        // auto dynSizeEnc = ABIEncode(dynSize);
        // acm2.insert(acm2.begin(), dynSizeEnc.begin(), dynSizeEnc.end());
        acm2.insert(acm2.end(), tails.begin(), tails.end());
        return acm2;
    } else {
        static_assert(always_false<T>, "Unsupported type");
    }
}

}  // namespace Encoder

}  // namespace Web3
