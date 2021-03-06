#pragma once

#include "utility.h"

#include <bit>
#include <cassert>
#include <numeric>
#include <optional>

namespace Web3 {

namespace Encoder {

inline std::string encode(const std::vector<unsigned char> &data) {
    std::stringstream ss;
    ss << "0x";
    for (const auto &byte : data) {
        ss << std::setw(2) << std::setfill('0') << std::hex << (int)byte;
    }
    return ss.str();
}

template <typename T>
std::string encode(const T &data) {
    static_assert(std::is_integral<T>::value, "T must be integral");
    if (data == 0) {
        return "0x0";
    }
    std::stringstream ss;
    ss << std::showbase << std::hex << data;
    return ss.str();
}

inline std::string encode(const boost::multiprecision::uint256_t &data) {
    if (data == 0) {
        return "0x0";
    }
    std::stringstream ss;
    ss << std::showbase << std::hex << data;
    return ss.str();
}

// TODO Fix how crappy this is
struct RLPEncodeInput {
    std::optional<std::vector<unsigned char>> value;
    std::optional<std::vector<RLPEncodeInput>> list;
    RLPEncodeInput(std::vector<unsigned char> &&value) : value(std::move(value)){};
    RLPEncodeInput(std::vector<RLPEncodeInput> &&list) : list(std::move(list)){};
    RLPEncodeInput(const std::vector<unsigned char> &value) : value(value){};
    RLPEncodeInput(const std::vector<RLPEncodeInput> &list) : list(list){};
    RLPEncodeInput(const Address &val) {
        value = std::vector<unsigned char>(val.bytes.begin(), val.bytes.end());
    }
    // TODO catch all like for abi encoding
    template <typename T> RLPEncodeInput(const T &val) : value(integralToBytes(val)) {
        static_assert(std::is_integral<T>::value, "T must be integral");
    }
};

inline void RLPEncodeLength(std::vector<unsigned char> &acm, size_t length, unsigned char prefix) {
    auto lengthLength = divRoundUp(31 - __builtin_clz(static_cast<unsigned int>(length)), 8);
    acm.push_back(prefix + lengthLength);
    ;
    if constexpr (std::endian::native == std::endian::big) {
        length = __builtin_bswap64(length);
    }
    for (auto i = lengthLength - 1; i >= 0; i--) {
        acm.push_back(length >> (8 * i));
    }
}

inline void RLPEncode_(std::vector<unsigned char> &acm, const RLPEncodeInput &data) {
    if (data.value.has_value()) {
        if (data.value->size() == 1) {
            if (data.value.value()[0] == 0) {
                acm.push_back(0x80);
                return;
            }
            if (data.value.value()[0] < 0x80) {
                acm.push_back(data.value.value()[0]);
                return;
            }
        }
        if (data.value->size() < 56) {
            acm.push_back(data.value->size() + 0x80);
            std::copy(data.value->begin(), data.value->end(), std::back_inserter(acm));
        } else if (data.value->size() < 1ULL + std::numeric_limits<unsigned int>::max()) {
            RLPEncodeLength(acm, data.value->size(), 0xB7);
            std::copy(data.value->begin(), data.value->end(), std::back_inserter(acm));
        } else {
            throw std::runtime_error("RLP: string is too large");
        }
    } else {
        assert(data.list.has_value());
        std::vector<unsigned char> tmp;
        for (auto &item : data.list.value()) {
            RLPEncode_(tmp, item);
        }
        if (tmp.size() < 56) {
            acm.push_back(0xC0 + tmp.size());
        } else if (tmp.size() < 1ULL + std::numeric_limits<unsigned int>::max()) {
            RLPEncodeLength(acm, tmp.size(), 0xF7);
        } else {
            throw std::runtime_error("RLP: list is too large");
        }
        std::copy(tmp.begin(), tmp.end(), std::back_inserter(acm));
    }
}

inline std::vector<unsigned char> encode(const RLPEncodeInput &data) {
    std::vector<unsigned char> acm;
    RLPEncode_(acm, data);
    return acm;
}

template <typename T>
std::vector<unsigned char> RLPEncode__(RLPEncodeInput &acm, const T &first) {
    assert(acm.list.has_value());
    acm.list.value().push_back(RLPEncodeInput(first));
    return encode(acm);
}

template <typename T, typename... Rest>
std::vector<unsigned char> RLPEncode__(RLPEncodeInput &acm, const T &first, const Rest &... rest) {
    assert(acm.list.has_value());
    acm.list.value().push_back(RLPEncodeInput(first));
    return RLPEncode__(acm, rest...);
}

template <typename T, typename... Rest>
std::vector<unsigned char> RLPEncode(const T &first, const Rest &... rest) {
    Web3::Encoder::RLPEncodeInput acm(std::vector<Web3::Encoder::RLPEncodeInput>({}));
    return RLPEncode__(acm, first, rest...);
}

template <typename T>
std::vector<unsigned char> RLPEncode(const T &first) {
    Web3::Encoder::RLPEncodeInput acm(std::vector<Web3::Encoder::RLPEncodeInput>({}));
    return encode(RLPEncodeInput(first));
}

inline std::vector<unsigned char> RLPEncode() {
    return {0xc0};
}

}  // namespace Encoder
};  // namespace Web3