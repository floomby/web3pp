## Web3pp

Header only Ethereum interaction library for c++.

This library depends only on some boost libraries and openssl.

**Is still a work in progress.**

### Things to finish

* Call options (value, gas override, gas price, alternative signer)
* Work on library consistency
* Documentation
* Better testing coverage (abi decoding needs some work)
* Make deployment not a variadic template so type hinting is available

### More things I need to finish

* Event subscriptions/past event searching
* Improve getting transaction receipts
* Get pending transactions

### Longer term features

* Websocket support
* Some utility stuff (ecrecover, direct storage access, address checksums)
* Eip1559 and eip2930 transaction support
