## Web3pp

Header only Ethereum interaction library for c++. Creates a typesafe interface to interact with contracts (think hardhat typechain) by means of a code generator (written in typescript). The synchronous code works well. The async interface experimental.

This library depends only on some boost libraries and openssl.

Documentation is missing, but look at the `setup-test.sh` script and the `tests/erc20.cpp` file to see an example.

**NOTE: This currently does not compile with clang or msvc.**

### High priority TODOs

* Call options testing coverage is poor (value, gas override, gas price)
* Get reasonable documentation
* Better testing coverage (generally, we always could use this...)
* Asan is complaining I leak some ssl stuff - (I really should fix this)

### More things I need to finish

* Event subscriptions/past event searching
* Improve how transaction receipts are handled
* Allow getting pending transactions

### Longer term features

* Websocket support
* Some utility stuff (ecrecover, direct storage access, address checksums)
* Eip1559 and eip2930 transaction support

### Chores

* Make some examples that are documented!
* Thrown exceptions in async code break things (it ruins the work queue)
* Make it compile under clang (It is throwing up on the template metaprogramming code as it is right now)
* The code generator for creating the c++ code from the abi is a pile of junk (Definitely in the running for my top 10 worst piles of junk I have made)
* Compilation is really slow and causes gcc to use insane amounts of memory (I am 95% sure it is the template resolution and I have ideas on how to fix it)