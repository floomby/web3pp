#define BOOST_TEST_DYN_LINK
#include "../erc20-test.h"

#include <boost/test/unit_test.hpp>

#include <semaphore>

BOOST_AUTO_TEST_CASE(Erc20, *boost::unit_test::depends_on("ContextInit")) {
    Erc20_test erc20_test;
    erc20_test.deploy("Something", "SMT");
    BOOST_CHECK(erc20_test.decimals() == 18);
    BOOST_CHECK(erc20_test.name() == "Something");
    BOOST_CHECK(erc20_test.symbol() == "SMT");

    // I am dumb or something because I don't think I should need two syncronization primitives here, but I seem to be having compile ordering problems
    // It appears the construction of approvedAmount is somehow drifting down below the closures or something (Idk I am lazy to read the assembly, and hate windows gdb suckyness)
    std::binary_semaphore testSemaphore(1);
    std::mutex testMutex;
    
    testMutex.lock();
    size_t decimals = 0;
    boost::multiprecision::uint256_t approvedAmount = 0;
    testMutex.unlock();

    testSemaphore.acquire();
    erc20_test.decimals_async([&decimals, &testSemaphore, &testMutex](const boost::multiprecision::uint256_t &value) {
        std::lock_guard<std::mutex> lock(testMutex);
        decimals = value.convert_to<size_t>();
        testSemaphore.release();
    });

    BOOST_CHECK(testSemaphore.try_acquire_until(std::chrono::system_clock::now() + std::chrono::seconds(5)));
    BOOST_CHECK(decimals == 18);

    erc20_test.approve("0x1af831cf22600979B502f1b73392f41fc4328Dc4", Web3::Units::ether(1));
    BOOST_CHECK(Web3::Units::ether(1) == erc20_test.allowance(Web3::defaultContext->signers.front()->address, "0x1af831cf22600979B502f1b73392f41fc4328Dc4"));

    auto handler = [&approvedAmount, &testSemaphore, &testMutex](const boost::multiprecision::uint256_t &value) {
        std::lock_guard<std::mutex> lock(testMutex);
        approvedAmount = value;
        testSemaphore.release();
    };

    erc20_test.approve_async("0xB2C049842476F85334ff58456CAc30ABa6b9B0b8", Web3::Units::gwei(40), [&erc20_test, handler = std::move(handler)](const std::string &res) {
        erc20_test.allowance_async(Web3::defaultContext->signers.front()->address, "0xB2C049842476F85334ff58456CAc30ABa6b9B0b8", std::move(handler));
    });
    BOOST_CHECK(testSemaphore.try_acquire_until(std::chrono::system_clock::now() + std::chrono::seconds(6)));
    testSemaphore.release();
    BOOST_CHECK(approvedAmount == Web3::Units::gwei(40));

    auto otherAccount = std::make_shared<Web3::Account>("1a5260e233781932bef86e6a4defc201c61cd0e55258c2ed5e51702401d7e5d6");
    erc20_test.approve("0x1af831cf22600979B502f1b73392f41fc4328Dc4", Web3::Units::ether(1), {.account = otherAccount});
    BOOST_CHECK(Web3::Units::ether(1) == erc20_test.allowance(otherAccount->address, "0x1af831cf22600979B502f1b73392f41fc4328Dc4"));
}
