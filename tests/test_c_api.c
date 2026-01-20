// tests/test_c_api.c
#include "sim_logger/c_api.h"

int sim_logger_c_api_smoke(void) {
  sim_logger_logger_t* lg = sim_logger_get("c.api");
  sim_logger_log(lg, SIM_LOGGER_LEVEL_INFO, "file.c", 123u, "func", "hello");
  sim_logger_logf(lg, SIM_LOGGER_LEVEL_WARN, "file.c", 124u, "func", "x=%d", 7);
  sim_logger_flush(lg);
  sim_logger_release(lg);
  return 0;
}