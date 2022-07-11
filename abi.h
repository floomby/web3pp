#pragma once

#include <string_view>
#include <type_traits>
#include <list>

#include "utility.h"

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

template <typename T, size_t Idx>
bool constexpr is_dynamic_t();

template <typename T>
bool constexpr is_dynamic() {
    using t = std::remove_cv_t<std::remove_reference_t<T>>;
    if (is_std_vector<t>::value || std::is_same_v<t, std::string>) {
        return true;
    }
    if constexpr (is_std_tuple<t>::value) {
        return is_dynamic_t<t, std::tuple_size_v<T> - 1>();
    }
    return false;
}

template <size_t N, typename... Ts>
struct get_tuple_type;

template <size_t N, typename T, typename... Ts>
struct get_tuple_type<N, std::tuple<T, Ts...>> {
    using type = typename get_tuple_type<N - 1, std::tuple<Ts...>>::type;
};

template <typename T, typename... Ts>
struct get_tuple_type<0, std::tuple<T, Ts...>> {
    using type = T;
};

template <typename T, size_t Idx>
bool constexpr is_dynamic_t() {
    static_assert(is_std_tuple<T>::value, "T must be a std::tuple");
    if (Idx == 0) {
        return false;
    }
    return is_dynamic<get_tuple_type<Idx, T>::type>() || is_dynamic_t<T, Idx - 1>();
}

template <typename T>
std::vector<unsigned char> ABIEncode_(const T &data);

template <typename T, size_t Idx, size_t N> std::vector<std::pair<bool, std::vector<unsigned char>>> &applier(std::vector<std::pair<bool, std::vector<unsigned char>>> &acm, const T &t) {
    auto res = ABIEncode_(get<N - Idx>(t));
    acm.push_back({ is_dynamic<decltype(get<N - Idx>(t))>(), res });
    if constexpr (Idx > 0) {
        return applier<T, Idx - 1, N>(acm, t);
    }
    return acm;
}

// inline std::vector<unsigned char> ABIEncode_() {
//     return {};
// }

/**
 * @brief ABIEncode_ elements
 * 
 * @tparam T type to encode (note that std::vector<unsigned char> is bytes and not uint8[] likewise std::array<unsigned char, N> is not uint8[N]
 * but is actually bytesN) I have an idea how to maybe fix this
 * @param data 
 * @return std::vector<unsigned char> abiencoded data
 */

template <typename T>
std::vector<unsigned char> ABIEncode_(const T &data) {
    // All uint types (includes bool)
    if constexpr (std::is_same_v<T, boost::multiprecision::uint256_t> || std::is_unsigned_v<T>) {
        return padFrontTo(integralToBytes(data), 32);
    // All int types
    } else if constexpr (std::is_same_v<T, boost::multiprecision::int256_t> || std::is_signed_v<T>) {
        // Need to sign extend signed numbers
        return padFrontTo(integralToBytes(data), 32, true);
    // Address type
    } else if constexpr (std::is_same_v<T, Address>) {
        return padFrontTo(data.bytes, 32);
    } else if constexpr (is_std_array<T>::value) {
        // BytesN types
        if constexpr (std::is_same_v<unsigned char, typename std_array_type<T>::type>) {
            // internally we use array<unsigned char, N> to represent bytesN for compile time types
            static_assert(std_array_size<T>::value <= 32, "bytesN can only have up to 32 bytes");
            return padBackTo(data, 32);
        // Type[N] type
        } else {
            std::vector<unsigned char> acm;
            for (int i = 0; i < std_array_size<T>::value; i++) {
                auto res = ABIEncode_(data[i]);
                acm.insert(acm.end(), res.begin(), res.end());
            }
            return acm;
        }
    // All fixed types
    } else if constexpr (is_fixed<T>::value) {
        return ABIEncode_(data.abiEncodable());
    // bytes type
    } else if constexpr (std::is_same_v<T, std::vector<unsigned char>>) {
        auto tmp = data;
        tmp = padBackTo(std::move(tmp), 32 * divRoundUp(data.size(), 32));
        auto lenEnc = ABIEncode_(data.size());
        tmp.insert(tmp.begin(), lenEnc.begin(), lenEnc.end());
        return tmp;
    // Type[] Type
    } else if constexpr (is_std_vector<T>::value) {
        if (data.size() == 0) {
            return ABIEncode_(0);
        }
        std::list<std::vector<unsigned char>> acm;
        for (const auto &item : data) {
            acm.push_back({ ABIEncode_(item) });
        }
        size_t dynSize = 0;
        std::vector<unsigned char> acm2, tails;
        for (const auto &val : acm) {
            if constexpr (is_dynamic<typename std_vector_type<T>::type>()) {
                auto offset = dynSize + data.size() * 32;
                auto offsetEnc = ABIEncode_(offset);
                acm2.insert(acm2.end(), offsetEnc.begin(), offsetEnc.end());
                dynSize += val.size();
                tails.insert(tails.end(), val.begin(), val.end());
            } else {
                acm2.insert(acm2.end(), val.begin(), val.end());
            }
        }
        auto sizeEnc = ABIEncode_(data.size());
        acm2.insert(acm2.begin(), sizeEnc.begin(), sizeEnc.end());
        acm2.insert(acm2.end(), tails.begin(), tails.end());
        return acm2;
    // string type
    } else if constexpr (std::is_same_v<T, std::string>) {
        std::vector<unsigned char> tmp;
        tmp.reserve(data.size());
        std::copy(data.begin(), data.end(), std::back_inserter(tmp));
        tmp = padBackTo(std::move(tmp), 32 * divRoundUp(data.size(), 32));
        auto lenEnc = ABIEncode_(data.size());
        tmp.insert(tmp.begin(), lenEnc.begin(), lenEnc.end());
        return tmp;
    // tuple type
    } else if constexpr (is_std_tuple<T>::value) { 
        // TODO I am not sure this is correct
        if constexpr (std::tuple_size_v<T> == 0) return {};
        std::vector<std::pair<bool, std::vector<unsigned char>>> acm;
        applier<T, std::tuple_size_v<T> - 1, std::tuple_size_v<T> - 1>(acm, data);
        size_t dynSize = 0;
        std::vector<unsigned char> acm2, tails;
        for (auto &[dyn, val] : acm) {
            if (dyn) {
                auto offset = dynSize + std::tuple_size_v<T> * 32;
                auto offsetEnc = ABIEncode_(offset);
                acm2.insert(acm2.end(), offsetEnc.begin(), offsetEnc.end());
                dynSize += val.size();
                tails.insert(tails.end(), val.begin(), val.end());
            } else {
                acm2.insert(acm2.end(), val.begin(), val.end());
            }
        }
        // auto dynSizeEnc = ABIEncode_(dynSize);
        // acm2.insert(acm2.begin(), dynSizeEnc.begin(), dynSizeEnc.end());
        acm2.insert(acm2.end(), tails.begin(), tails.end());
        return acm2;
    } else if constexpr (std::is_same_v<T, Function>) { 
        auto ret = data.asVector();
        return padBackTo(std::move(ret), 32);
    } else {
        static_assert(always_false<T>, "Unsupported type");
    }
}

template <typename... Args>
std::vector<unsigned char> ABIEncode(Args &&... args) {
    if constexpr (sizeof...(args) == 0) {
        return {};
    } else if constexpr (sizeof...(args) == 1) {
        return ABIEncode_(std::forward<Args>(args)...);
    } else return ABIEncode_(std::make_tuple(std::forward<Args>(args)...));
}

template <typename T> T ABIDecodeTo(const std::vector<unsigned char> &data, std::vector<unsigned char>::const_iterator &it) {
    if constexpr (std::is_same_v<T, boost::multiprecision::uint256_t>) {
        auto ret = fromBytes(it);
        it += 32;
        return ret;
    } else if constexpr (std::is_unsigned_v<T>) {
        auto ret = static_cast<T>(fromBytes(it));
        it += 32;
        return ret;
    } else if constexpr (std::is_same_v<T, boost::multiprecision::int256_t>) {
        auto ret = fromBytes<true>(it);
        it += 32;
        return ret;
    } else if constexpr (std::is_signed_v<T>) {
        auto ret = static_cast<T>(fromBytes<true>(it));
        it += 32;
        return ret;
    } else if constexpr (std::is_same_v<T, std::vector<unsigned char>>) {
        auto len = ABIDecodeTo<size_t>(data, it);
        std::vector<unsigned char> ret(len);
        auto bytesToRead = divRoundUp(len, 32);
        std::copy_n(it, len, ret.begin());
        it += bytesToRead;
        return ret;
    } else if constexpr (is_std_vector<T>::value) {
        auto len = ABIDecodeTo<size_t>(data, it);
        T ret;
        ret.reserve(len);
        for (size_t i = 0; i < len; i++) {
            ret.push_back(ABIDecodeTo<typename std_vector_type<T>::type>(data, it));
        }
        return ret;
    } else {
        static_assert(always_false<T>, "Unsupported type");
    }
}

template <typename T> T ABIDecodeTo(const std::vector<unsigned char> &data) {
    if (data.size() % 32 != 0) {
        throw std::runtime_error("Invalid decode data size");
    }
    auto it = data.begin();
    return ABIDecodeTo<T>(data, it);
}

}  // namespace Encoder

}  // namespace Web3
