## Web3pp

Header only Ethereum rpc library for c++.

This library depends only on some boost libraries and openssl.

**Is still very much a work in progress, but currently does sign and send transactions as well as deploy contracts.**

### Near term features (In order)

* Contract calls
* Support async calls
* Call options (value, gas override, gas price)
* Work on library consistency
* Documentation
* Better testing coverage (abi encoding needs some work)
* Event subscriptions/past event searching
* Improve getting transaction receipts
* Get pending transactions

### Longer term features

* Websocket support
* Some utility stuff
* Eip1559 and eip2930 transaction support
