## Web3pp

Header only Ethereum interaction library for c++.

This library depends only on some boost libraries and openssl.

**Is still a work in progress.**

### Things to finish

* Async calling (I need more thorough tests)
* Call options testing (value, gas override, gas price, alternative signer (more or less completed, but missing some tests))
* Better testing coverage
* Make deployment not a variadic template so type hinting is available

### More things I need to finish

* Event subscriptions/past event searching
* Improve getting transaction receipts
* Get pending transactions
* Dynamic typed abi encoding and decoding

### Longer term features

* Websocket support
* Some utility stuff (ecrecover, direct storage access, address checksums)
* Eip1559 and eip2930 transaction support

### Chores

* Get reasonable documentation
* Fix building tests to not use primitive makefile
* Make some examples that are documented
* Fix style issues
* Don't throw out of anything in the async code (it ruins the work queue)