#pragma once

#include "encoder.h"
#include "account.h"

namespace Web3 {

class Transaction {
   public:
    size_t nonce;
    boost::multiprecision::uint256_t gasPrice;
    boost::multiprecision::uint256_t gasLimit;
    std::vector<unsigned char> to;
    boost::multiprecision::uint256_t value;
    std::vector<unsigned char> data;
    Signature signature;
    std::vector<unsigned char> sign(const Account &account, std::shared_ptr<Context> context = nullptr) {
        Encoder::RLPEncodeInput nonce(arithmeticToBytes(this->nonce));
        Encoder::RLPEncodeInput gasPrice(arithmeticToBytes(this->gasPrice));
        Encoder::RLPEncodeInput gasLimit(arithmeticToBytes(this->gasLimit));
        Encoder::RLPEncodeInput to(this->to);
        Encoder::RLPEncodeInput value(arithmeticToBytes(this->value));
        Encoder::RLPEncodeInput chainId(arithmeticToBytes(context ? context->chainId : defaultContext->chainId));
        Encoder::RLPEncodeInput empty(std::vector<unsigned char>({}));

        Encoder::RLPEncodeInput txrlp({ nonce, gasPrice, gasLimit, to, value, empty, chainId, empty, empty });
        auto txrlpEncoded = Encoder::encode(txrlp);
        auto txrlpHash = keccak256(txrlpEncoded);
        auto sig = account.sign(txrlpHash);

        // std::cout << "RLP Encoded: " << toString(txrlpEncoded) << std::endl;
        // std::cout << "RLP Hash: " << toString(txrlpHash) << std::endl;
        // std::cout << "Signature: " << sig << std::endl;

        unpadFront(sig.r);
        unpadFront(sig.s);
        unpadFront(sig.v);

        Encoder::RLPEncodeInput vEnc(sig.v);
        Encoder::RLPEncodeInput rEnc(sig.r);
        Encoder::RLPEncodeInput sEnc(sig.s);

        Encoder::RLPEncodeInput signedTx({ nonce, gasPrice, gasLimit, to, value, empty, vEnc, rEnc, sEnc });
        return Encoder::encode(signedTx);
    }
};

}  // namespace Web3