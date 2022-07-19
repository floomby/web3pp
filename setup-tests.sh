#!/bin/bash

solc_path=`which solcjs`
node_path=`which node`
vyper_path=`which vyper`

if [ ! -z "$solc_path" ]; then
  pushd test-resources
  echo "Compiling solidity test contracts..."
  solcjs erc20-test.sol --bin --abi --base-path .. --include-path ../node_modules/
  cp test-resources_erc20-test_sol_TestToken.abi ../erc20-test.abi
  cp test-resources_erc20-test_sol_TestToken.bin ../erc20-test.bin
  popd
else
  echo "solcjs not found"
fi

if [ ! -z "$vyper_path" ]; then
  echo "Compiling vyper test contracts..."
  vyper -f bytecode test-resources/fixed-test.vy > fixed-test.bin
  vyper -f abi test-resources/fixed-test.vy > fixed-test.abi
else
  echo "vyper not found"
fi

if [ ! -z "$node_path" ]; then
  echo "Running code generation..."
  if [ -f "erc20-test.abi" ]; then
    echo "Found erc20-test.abi"
    ts-node gen.ts erc20-test >erc20-test.h
  fi
  if [ -f "fixed-test.abi" ]; then
    echo "Found fixed-test.abi"
    ts-node gen.ts fixed-test >fixed-test.h
  fi
fi
