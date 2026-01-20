# sim-logger

A small, self-contained logging library for **C++ and C** simulation code.

This repository currently documents **stand-alone (non-Trick)** usage. Trick integration is planned for a
subsequent sprint.

## Features (stand-alone)

- Hierarchical loggers (dotted names) with inheritance/overrides
- Console and file sinks
- Rotating file sink (timestamp rename + retention)
- Asynchronous logging (opt-in) with bounded queue + overflow policy
- Pattern formatting (includes `{met}` token)
- C API for C models

## Build and test

### Build

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

### Run tests

```bash
ctest --test-dir build --output-on-failure
```

See `BUILDING.md` for offline Catch2 details.

## Integrate into your project

### Option A: add as a subdirectory

```cmake
add_subdirectory(path/to/sim-logger)

add_executable(my_sim main.cpp)
target_link_libraries(my_sim PRIVATE sim_logger::core)
```

For C models:

```cmake
add_executable(my_c_model model.c)
target_link_libraries(my_c_model PRIVATE sim_logger::c_api)
```

### Option B: FetchContent (recommended when you control the superbuild)

```cmake
include(FetchContent)

FetchContent_Declare(sim_logger
  GIT_REPOSITORY <your repo url>
  GIT_TAG        <tag or commit>
)
FetchContent_MakeAvailable(sim_logger)

target_link_libraries(my_sim PRIVATE sim_logger::core)
```

## C++ quick start

```cpp
#include "logger/async_sink.hpp"
#include "logger/console_sink.hpp"
#include "logger/log_macros.hpp"
#include "logger/logger_registry.hpp"
#include "logger/pattern_formatter.hpp"
#include "logger/rotating_file_sink.hpp"

using namespace sim_logger;

int main() {
  // 1) Get a root logger.
  auto root = LoggerRegistry::instance().get_logger("sim");
  root->set_level(Level::Info);

  // 2) Choose a pattern.
  PatternFormatter fmt("{met} {level} [{logger}] {msg} ({file}:{line})");

  // 3) Create sinks.
  auto console = std::make_shared<ConsoleSink>(fmt, ConsoleSink::ColorMode::Auto);

  RotatingFileSink::Options ropt;
  ropt.base_path = "sim.log";
  ropt.max_bytes = 5 * 1024 * 1024;     // rotate at 5 MB
  ropt.max_rotated_files = 10;          // keep last 10
  auto rotating = std::make_shared<RotatingFileSink>(ropt, fmt, /*durable_flush=*/false);

  // 4) (Optional) wrap sinks in async for high-rate logging.
  AsyncOptions aopt;
  aopt.capacity = 4096;
  aopt.overflow_policy = OverflowPolicy::Block;
  auto async_file = std::make_shared<AsyncSink>(rotating, aopt);

  // 5) Attach sinks and log.
  root->set_sinks({console, async_file});
  LOG_INFO(root, std::string("startup"));
  LOG_WARNF(root, "step=%d", 1);

  // 6) Ensure all buffered records are written.
  root->flush();
  return 0;
}
```

## C quick start

The C API is intended for **logging from C models** while configuration (sinks/levels) is typically done
from C++ during initialization.

```c
#include "sim_logger/c_api.h"

int main(void) {
  sim_logger_logger_t* lg = sim_logger_get("c.model");

  sim_logger_log(lg, SIM_LOGGER_LEVEL_INFO, __FILE__, (uint32_t)__LINE__, __func__, "hello");
  sim_logger_logf(lg, SIM_LOGGER_LEVEL_WARN, __FILE__, (uint32_t)__LINE__, __func__, "x=%d", 7);

  sim_logger_flush(lg);
  sim_logger_release(lg);
  return 0;
}
```

## Examples

Build and run:

```bash
cmake -S . -B build -G Ninja
cmake --build build

./build/examples/sim_logger_example_cpp
./build/examples/sim_logger_example_async_cpp
./build/examples/sim_logger_example_c
```

See `docs/standalone_usage.md` for a deeper guide.
