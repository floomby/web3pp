#pragma once

#include <boost/json.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/random/random_device.hpp>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "keccak.h"

namespace Web3 {

template <size_t N>
std::array<unsigned char, N> randomBytes() {
    boost::random::random_device rd;
    std::array<unsigned char, N> tmp;
    rd.generate(tmp.data(), tmp.data() + tmp.size());
    return tmp;
}

inline boost::multiprecision::uint256_t fromBytes(const std::vector<unsigned char> &bytes) {
    boost::multiprecision::uint256_t ret;
    boost::multiprecision::import_bits(ret, bytes.data(), bytes.data() + bytes.size(), 0, true);
    return ret;
}

template <size_t N>
boost::multiprecision::uint256_t fromBytes(const std::array<unsigned char, N> &bytes) {
    static_assert(N == 32, "Only 32 bytes are supported");
    boost::multiprecision::uint256_t ret;
    boost::multiprecision::import_bits(ret, bytes.data(), bytes.data() + bytes.size(), 0, true);
    return ret;
}

template <typename T>
boost::multiprecision::uint256_t fromBytes(const T &bytes) {
    if (bytes.size() > 32) {
        throw std::runtime_error("Up to 32 bytes are supported");
    }
    boost::multiprecision::uint256_t ret;
    boost::multiprecision::import_bits(ret, bytes.data(), bytes.data() + bytes.size(), 0, true);
    return ret;
}

inline boost::multiprecision::uint256_t fromString(const std::string &str) {
    return boost::multiprecision::cpp_int(str).convert_to<boost::multiprecision::uint256_t>();
}

template <size_t N>
std::string toString(const std::array<unsigned char, N> &bytes) {
    std::stringstream ss;
    for (unsigned i = 0; i < bytes.size(); i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)bytes[i];
    }
    return ss.str();
}

inline std::string toString(const std::vector<unsigned char> &bytes) {
    std::stringstream ss;
    for (unsigned i = 0; i < bytes.size(); i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)bytes[i];
    }
    return ss.str();
}

inline std::string toString(const boost::multiprecision::uint256_t &num) {
    std::stringstream ss;
    ss << std::hex << std::setw(64) << std::setfill('0') << std::showbase << num;
    return ss.str();
}

template <typename T>
std::vector<T> &unpadFront(std::vector<T> &v) {
    auto it = v.begin();
    while (!v.empty() && v.front() == 0) {
        it++;
    }
    v.erase(v.begin(), it);
    if (it != v.begin()) {
        std::cout << "unpadded" << std::endl;
    }
    return v;
}

inline std::vector<unsigned char> hexToBytes(const std::string &hex) {
    std::vector<unsigned char> bytes;
    if (hex.length() % 2 != 0) {
        throw std::runtime_error("Invalid hex string");
    }
    for (unsigned int i = 0; i < hex.length(); i += 2) {
        char byte = (char)std::strtol(hex.substr(i, 2).c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

inline void prettyPrint(std::ostream &os, boost::json::value const &jv, std::string *indent = nullptr) {
    std::string indent_;
    if (!indent)
        indent = &indent_;
    switch (jv.kind()) {
        case boost::json::kind::object: {
            os << "{\n";
            indent->append(4, ' ');
            auto const &obj = jv.get_object();
            if (!obj.empty()) {
                auto it = obj.begin();
                for (;;) {
                    os << *indent << boost::json::serialize(it->key()) << " : ";
                    prettyPrint(os, it->value(), indent);
                    if (++it == obj.end())
                        break;
                    os << ",\n";
                }
            }
            os << "\n";
            indent->resize(indent->size() - 4);
            os << *indent << "}";
            break;
        }

        case boost::json::kind::array: {
            os << "[\n";
            indent->append(4, ' ');
            auto const &arr = jv.get_array();
            if (!arr.empty()) {
                auto it = arr.begin();
                for (;;) {
                    os << *indent;
                    prettyPrint(os, *it, indent);
                    if (++it == arr.end())
                        break;
                    os << ",\n";
                }
            }
            os << "\n";
            indent->resize(indent->size() - 4);
            os << *indent << "]";
            break;
        }

        case boost::json::kind::string: {
            os << boost::json::serialize(jv.get_string());
            break;
        }

        case boost::json::kind::uint64:
            os << jv.get_uint64();
            break;

        case boost::json::kind::int64:
            os << jv.get_int64();
            break;

        case boost::json::kind::double_:
            os << jv.get_double();
            break;

        case boost::json::kind::bool_:
            if (jv.get_bool())
                os << "true";
            else
                os << "false";
            break;

        case boost::json::kind::null:
            os << "null";
            break;
    }

    if (indent->empty())
        os << "\n";
}

inline std::vector<unsigned char> arithmeticToBytes(const boost::multiprecision::uint256_t &val) {
    std::vector<unsigned char> ret;
    boost::multiprecision::export_bits(val, std::back_inserter(ret), 8);
    return ret;
}

template <typename T>
std::vector<unsigned char> arithmeticToBytes(T val) {
    static_assert(std::is_arithmetic<T>::value, "T must be arithmetic");
    std::vector<unsigned char> bytes;
    if constexpr (std::endian::native == std::endian::big) {
        val = std::byteswap(val);
    }
    while (val) {
        bytes.push_back(val & 0xff);
        val >>= 8;
    }
    std::reverse(bytes.begin(), bytes.end());
    return bytes;
}

}  // namespace Web3
