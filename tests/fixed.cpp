#define BOOST_TEST_DYN_LINK
#include "../fixed-test.h"

#include <boost/test/unit_test.hpp>

static const double epsilon = 0.00001;

BOOST_AUTO_TEST_CASE(Fixed, *boost::unit_test::depends_on("ContextInit")) {
    Fixed_test fixed_test;

    fixed_test.deploy();
    auto res = fixed_test.mult(3.4);
    BOOST_CHECK(abs(static_cast<double>(get<0>(res).underlying) - 3.4) < epsilon);
    BOOST_CHECK(abs(static_cast<double>(get<1>(res).underlying) - 6.8) < epsilon);
}
