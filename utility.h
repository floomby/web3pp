#pragma once

#include <boost/json.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <boost/random/random_device.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "keccak.h"

#define divRoundUp(n, d) (((n) / (d)) + (n % (d) != 0))

template <class... T> constexpr bool always_false = false;

namespace Web3 {

template <bool isSigned> struct Signedness {};
template <> struct Signedness<true> { typedef boost::multiprecision::int256_t type; };
template <> struct Signedness<false> { typedef boost::multiprecision::uint256_t type; };

template <size_t N> std::array<unsigned char, N> randomBytes() {
    boost::random::random_device rd;
    std::array<unsigned char, N> tmp;
    rd.generate(tmp.data(), tmp.data() + tmp.size());
    return tmp;
}

template <bool isSigned = false>
inline auto fromBytes(const std::vector<unsigned char> &bytes) {
    typename Signedness<isSigned>::type ret;
    boost::multiprecision::import_bits(ret, bytes.data(), bytes.data() + bytes.size(), 0, true);
    return ret;
}

template <bool isSigned = false>
inline auto fromBytes(const std::vector<unsigned char>::const_iterator &begin, size_t size = 32) {
    typename Signedness<isSigned>::type ret;
    boost::multiprecision::import_bits(ret, begin, begin + size, 0, true);
    return ret;
}

template <size_t N, bool isSigned = false> auto fromBytes(const std::array<unsigned char, N> &bytes) {
    static_assert(N == 32, "Only 32 bytes are supported");
    typename Signedness<isSigned>::type ret;
    boost::multiprecision::import_bits(ret, bytes.data(), bytes.data() + bytes.size(), 0, true);
    return ret;
}

template <typename T, bool isSigned = false> auto fromBytes(const T &bytes) {
    if (bytes.size() > 32) {
        throw std::runtime_error("Up to 32 bytes are supported");
    }
    typename Signedness<isSigned>::type ret;
    boost::multiprecision::import_bits(ret, bytes.data(), bytes.data() + bytes.size(), 0, true);
    return ret;
}

template <bool isSigned = false> Signedness<isSigned>::type fromString(const std::string &str) {
    return boost::multiprecision::cpp_int(str).convert_to<typename Signedness<isSigned>::type>();
}

template <size_t N> std::string toString(const std::array<unsigned char, N> &bytes) {
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
    ss << std::hex << std::setfill('0') << std::showbase << num;
    return ss.str();
}

inline std::string toString(const boost::multiprecision::int256_t &num) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::showbase << num;
    return ss.str();
}

template <typename T, typename U> std::string toString(const std::pair<T, U> &pair) {
    return toString(pair.first) + toString(pair.second);
}

// template <typename T, typename U> std::string toString(std::pair<T, U> pair) {
//     return toString(pair.first) + toString(pair.second);
// }

template <typename T> std::vector<T> &unpadFront(std::vector<T> &v) {
    auto it = v.begin();
    while (!v.empty() && *it == 0) {
        it++;
    }
    v.erase(v.begin(), it);
    if (it != v.begin()) {
        std::cout << "unpadded" << std::endl;
    }
    return v;
}

template <typename T> std::vector<T> padFrontTo(std::vector<T> &&v, size_t n, bool isSigned = false) {
    T pad = isSigned && v[0] & T(1) << (sizeof(T) * 8 - 1) ? -1 : 0;
    // compiler probably vectorizes this
    while (v.size() < n) v.insert(v.begin(), pad);
    return v;
}

template <typename T, size_t N> std::vector<T> padFrontTo(const std::array<T, N> &a, size_t n) {
    std::vector<T> v(a.begin(), a.end());;
    return padFrontTo(std::move(v), n);
}

template <typename T> std::vector<T> padBackTo(std::vector<T> &&v, size_t n) {
    if (v.size() < n) v.resize(n, 0);
    return v;
}

template <typename T, size_t N> std::vector<T> padBackTo(const std::array<T, N> &a, size_t n) {
    std::vector<T> v(a.begin(), a.end());;
    return padBackTo(std::move(v), n);
}

template <class T> struct is_std_tuple : std::false_type {};
template <class ...Ts> struct is_std_tuple<std::tuple<Ts...>> : std::true_type {};

template <class T> struct is_std_array : std::false_type {};
template <class T, std::size_t N> struct is_std_array<std::array<T, N>> : std::true_type {};

template <class T> struct std_array_size { static_assert(always_false<T>, "Not a std::array"); };
template <class T, size_t N> struct std_array_size<std::array<T, N>> { static constexpr size_t value = N; };

template <class T> struct std_array_type { static_assert(always_false<T>, "Not a std::array"); };
template <class T, size_t N> struct std_array_type<std::array<T, N>> { typedef T type; };

template <class T> struct std_vector_type { static_assert(always_false<T>, "Not a std::vector"); };
template <class T> struct std_vector_type<std::vector<T>> { typedef T type; };

template <typename T = std::vector<unsigned char>> T hexToBytes(const std::string &hex) {
    auto offset = hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X') ? 2 : 0;
    T bytes;
    if (hex.length() % 2 != 0) {
        throw std::runtime_error("Invalid hex string");
    }
    if constexpr (is_std_array<T>::value) {
        if ((hex.length()  - offset) / 2 != bytes.size()) {
            throw std::runtime_error("Invalid hex string (length mismatch to fixed size container)");
        }
    }
    for (unsigned int i = 0; i < hex.length(); i += 2) {
        char byte = (char)std::strtol(hex.substr(i + offset, 2).c_str(), NULL, 16);
        if constexpr (is_std_array<T>::value) {
            bytes[i / 2] = byte;
        } else {
            bytes.push_back(byte);
        }
    }
    return bytes;
}

inline boost::json::value parseFile(const std::string &filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        throw std::runtime_error("Could not open file");
    }
    std::stringstream ss;
    ss << ifs.rdbuf();
    return boost::json::parse(ss.str());
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

inline std::vector<unsigned char> integralToBytes(const boost::multiprecision::uint256_t &val) {
    std::vector<unsigned char> ret;
    boost::multiprecision::export_bits(val, std::back_inserter(ret), 8);
    return ret;
}

inline std::vector<unsigned char> integralToBytes(const boost::multiprecision::int256_t &val) {
    std::vector<unsigned char> ret;
    if (val.sign() == -1) {
        auto tmp = val + 1;
        boost::multiprecision::export_bits(tmp, std::back_inserter(ret), 8);
        for (auto &byte : ret) {
            byte = byte ^ 0xFF;
        }
    } else {
        boost::multiprecision::export_bits(val, std::back_inserter(ret), 8);
    }
    return ret;
}

template <typename T> std::vector<unsigned char> integralToBytes(T val) {
    static_assert(std::is_integral<T>::value, "T must be integral");
    std::vector<unsigned char> bytes;
    if constexpr (std::endian::native == std::endian::big) {
        val = std::byteswap(val);
    }
    // right shift on signed is implementation dependant as to whether it is logical or integral
    auto asUnsigned = static_cast<std::make_unsigned<T>::type>(val);
    while (asUnsigned) {
        bytes.push_back(asUnsigned & 0xff);
        asUnsigned >>= 8;
    }
    std::reverse(bytes.begin(), bytes.end());
    return bytes;
}

template <> inline std::vector<unsigned char> integralToBytes<bool>(bool val) {
    if (val) return {1};
    return {0};
}

struct Address {
    std::array<unsigned char, 20> bytes;
    Address(const std::string &hex) : bytes(hexToBytes<std::array<unsigned char, 20>>(hex)) {}
    Address(const char *hex) : Address(std::string(hex)) {}
    Address() {
        std::fill(bytes.begin(), bytes.end(), 0);
    }
    template <typename T>
    Address(const T &val) {
        if (val.size() != 20) throw std::runtime_error("Address must be 20 bytes");
        std::copy_n(val.begin(), 20, bytes.begin());
    }
    bool operator==(const Address &other) const {
        return bytes == other.bytes;
    }
    bool isZero() const {
        return std::all_of(bytes.begin(), bytes.end(), [](unsigned char b) { return b == 0; });
    }
    std::string asString() const {
        return "0x" + Web3::toString(bytes);
    }
    std::vector<unsigned char> asVector() const {
        std::vector<unsigned char> ret(bytes.begin(), bytes.end());
        return ret;
    }
};

struct Selector {
    std::array<unsigned char, 4> bytes;
    Selector(const std::string &signature) {
        auto hash = keccak256(signature);
        std::copy_n(hash.begin(), 4, bytes.begin());
    }
};

struct Function {
    Address address;
    Selector selector;
    std::vector<unsigned char> asVector() const {
        std::vector<unsigned char> ret;
        ret.insert(ret.end(), address.bytes.begin(), address.bytes.end());
        ret.insert(ret.end(), selector.bytes.begin(), selector.bytes.end());
        return ret;
    }
};

inline std::string optionBuilder(std::vector<std::pair<std::string, std::string>> options) {
    std::stringstream ret;
    ret << "{";
    for (auto &[key, value] : options) {
        ret << "\"" << key << "\":\"" << value << "\",";
    }
    ret.seekp(-1, std::ios_base::end);
    ret << "}";
    return ret.str();
}

}  // namespace Web3

namespace std {
inline ostream &operator<<(ostream &os, const Web3::Address &address) {
    os << Web3::toString(address.bytes);
    return os;
}
}  // namespace std


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