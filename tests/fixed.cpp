#define BOOST_TEST_DYN_LINK
#include "../fixed-test.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(Fixed, *boost::unit_test::depends_on("ContextInit")) {
    Fixed_test fixed_test;

    fixed_test.deploy();
    std::cout << "Deployed contract at " << fixed_test.address << std::endl;
    auto res = fixed_test.mult(3.4);
    std::cout << get<0>(res) << " " << get<1>(res) << std::endl;
}
