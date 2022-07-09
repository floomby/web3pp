#pragma once

#include <iostream>

#include "account.h"

namespace Web3 {
class Contract {
   public:
    virtual const char *__data() = 0;
    inline void deploy(Account &account) {
        // TODO find gas price and estimate gas
        const auto nonce = account.getTransactionCount();
        Web3::Transaction tx{nonce, 0x04a817c800, 0x0210000, std::vector<unsigned char>({}), Web3::fromString("00"), Web3::hexToBytes(this->__data())};

        auto signedTx = tx.sign(account);
        auto h = account.sendRawTransaction(Web3::toString(signedTx));
        std::cout << "Hash: " << h << std::endl;
        std::cout << "Deployed contract address is: " << account.deployedContract(nonce) << std::endl;
        h.getReceipt();
    }
};
}  // namespace Web3