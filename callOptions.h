#pragma once

#include <optional>

#include "utility.h"
#include "account.h"

namespace Web3 {

struct CallOptions {
    std::shared_ptr<Account> account = nullptr;
    std::optional<boost::multiprecision::uint256_t> gasPrice = std::nullopt;
    std::optional<boost::multiprecision::uint256_t> gasLimit = std::nullopt;
    std::optional<boost::multiprecision::uint256_t> value = std::nullopt;
    std::optional<size_t> nonce = std::nullopt;
};

}