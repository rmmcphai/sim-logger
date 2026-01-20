#include <catch2/catch_test_macros.hpp>

extern "C" int sim_logger_c_api_smoke(void);

TEST_CASE("C API compiles and links", "[c_api]") {
  REQUIRE(sim_logger_c_api_smoke() == 0);
}
