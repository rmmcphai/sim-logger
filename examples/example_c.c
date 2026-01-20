#include "sim_logger/c_api.h"

#include <stdint.h>

int main(void) {
  sim_logger_logger_t* lg = sim_logger_get("c.model");

  sim_logger_log(lg,
                 SIM_LOGGER_LEVEL_INFO,
                 __FILE__,
                 (uint32_t)__LINE__,
                 __func__,
                 "hello from C");

  sim_logger_logf(lg,
                  SIM_LOGGER_LEVEL_WARN,
                  __FILE__,
                  (uint32_t)__LINE__,
                  __func__,
                  "x=%d y=%s",
                  7,
                  "ok");

  sim_logger_flush(lg);
  sim_logger_release(lg);
  return 0;
}