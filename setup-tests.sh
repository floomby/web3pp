#!/bin/bash

pushd test-resources
echo "Compiling test contracts..."
solcjs erc20-test.sol --bin --abi --base-path .. --include-path ../node_modules/
cp test-resources_erc20-test_sol_TestToken.abi ../erc20-test.abi
cp test-resources_erc20-test_sol_TestToken.bin ../erc20-test.bin
popd

echo "Running code generation..."
ts-node gen.ts erc20-test >erc20-test.h