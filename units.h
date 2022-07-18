#pragma once

#include <boost/multiprecision/cpp_int.hpp>

namespace Web3 {
namespace Units {

inline boost::multiprecision::uint256_t gwei(const boost::multiprecision::uint256_t &value) {
    return value * 1000000000;
}

inline boost::multiprecision::uint256_t ether(const boost::multiprecision::uint256_t &value) {
    return value * 1000000000000000000;
}

}  // namespace Units
}  // namespace Web3