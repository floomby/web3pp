## Web3pp

Header only Ethereum interaction library for c++. Creates a typesafe interface to interact with contracts (analogous to typechain) by means of a code generator (written in typescript). Includes a promise based async interface as well as a synchronous interface.

This library is intentionally light on dependencies requiring only some boost libraries and openssl.

**NOTE: Only tested with gcc, clang may work with some small effort. MSVC is probably further off.**

### Usage

Proper documentation is missing right now, but the tests (specifically the initialization test and the erc20 test) illustrate how to use the library.

### Running the tests

Vyper (easy to install through pip), solcjs, and node are required to compile the test contracts and generate the c++ interface.

```
# compile the test contracts outputting the bytecode and abi, and then generate the c++ code
./setup-tests.sh

make
# or alternatively if on windows
make windows-test

# The tests are can be run against gananche (It is easy enough to use them with something else though, just change the keys/accounts in the test)
ganache-cli --port 7545 -m "volcano again sheriff turtle wealth theme short assume island course knee cheese" --chainId 1

# Running the tests is done with ./<platform>-test -- <rpc hostname> <rpc port> <chain id> <use ssl>
# So on linux for running against ganache it is
./linux-test -- localhost 7545 1 false
```

On windows I recommend building with building against msys2 using the msys2 build toolchain.

It is noteworthy that on windows it uses the windows system certificate store for ssl rather than the openssl store. On all platforms it will reject invalid certificates and refuse to connect.

### Things on the roadmap

* Get reasonable documentation
* Improving testing coverage
* Event subscriptions/past event searching
* Improve how transaction receipts are handled
* Allow getting pending transactions
* Websocket support
* Some utility stuff (ecrecover, direct storage access, address checksums)
* Eip1559 and eip2930 transaction support

### Known issues

* Make it compile under clang (It is failing right now, some errors from boost code, some from mine, I have not investigated at all really)
* The code generator could use some refactoring (There is new code in the account class that can be used to simplify this)
* Asan is complaining about leaking some ssl stuff - (Should be easy fixes)