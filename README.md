## Web3pp

Header only Ethereum interaction library for c++.

This library depends only on some boost libraries and openssl.

**NOTE: This currently does not compile with clang.**

### Things to finish

* Call options testing (value, gas override, gas price, alternative signer (more or less completed, but missing some tests))
* Better testing coverage
* Get reasonable documentation

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

* Fix building tests to not use primitive makefile (I suppose cmake is the best way to do this)
* Make some examples that are documented
* Fix style issues
* Don't throw out of anything in the async code (it ruins the work queue)
* Make it compile under clang (It is throwing up on the template metaprogramming code as it is right now)