## Web3pp

Header only Ethereum interaction library for c++. Creates a typesafe interface to interact with contracts (analogous to typechain) by means of a code generator (written in typescript). Includes a promise based async interface as well as a synchronous interface.

This library is intentionally light on dependencies requiring only some boost libraries and openssl.

**NOTE: Only tested with gcc, clang may work with some small effort. MSVC is probably further off.**

### High priority TODOs

* Get reasonable documentation
* Better testing coverage
* Asan is complaining about leaking some ssl stuff - (I really should fix this)

### More things I need to finish

* Event subscriptions/past event searching
* Improve how transaction receipts are handled
* Allow getting pending transactions

### Longer term features

* Websocket support
* Some utility stuff (ecrecover, direct storage access, address checksums)
* Eip1559 and eip2930 transaction support

### Known issues

* Thrown exceptions in async code break things (it ruins the work queue)
* Make it compile under clang (It is throwing up on the template metaprogramming code as it is right now)
* The code generator could use some refactoring (There is new code in the account class that can be used to simplify this)
* Compilation is really slow and causes gcc to use large amounts of memory (I am 95% sure it is the template resolution and I have ideas on how to fix it)