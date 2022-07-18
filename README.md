## Web3pp

Header only Ethereum rpc library for c++.

This library depends only on some boost libraries and openssl.

**Is still a work in progress.**

### Things to finish

* Finish fixed type handling
* Make deployment not a variadic template so type hinting is available
* Call options (value, gas override, gas price, alternative signer)
* Come up with a good way to map solidity structs into c++ objects
* Work on library consistency
* Documentation
* Better testing coverage (abi decoding needs some work)

### More things I need to finish

* Event subscriptions/past event searching
* Improve getting transaction receipts
* Get pending transactions

### Longer term features

* Websocket support
* Some utility stuff
* Eip1559 and eip2930 transaction support
