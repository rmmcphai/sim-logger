## Dependencies

### Catch2 (tests)
This repo prefers an **offline** Catch2 checkout under:

- `third_party/Catch2/`

If not present, CMake will try `find_package(Catch2 3)`.
Network fetching is **opt-in** only:

- `-DSIM_LOGGER_USE_FETCHCONTENT=ON`

---

## Build

### Configure + build (Ninja example)

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

**NOTE: -G Ninja not required.**

### Some extra options
```bash
-DCMAKE_COLOR_DIAGNOSTICS=ON
```

### Run Tests
```bash
ctest --test-dir build
```

## Examples

Examples are built as part of the normal build.

```bash
./build/examples/sim_logger_example_cpp
./build/examples/sim_logger_example_async_cpp
./build/examples/sim_logger_example_c
```

## Targets

### Core Library
- Target: ```sim_logger::core```

### C API Wrapper
- Target: ```sim_logger::c_api```
- Public Header: ```logger_c_api/include/sim_logger/c_api.h```

### For CMake Projets:
```bash
target_link_libraries(your_target Private sim_logger::core) # C++
# Or
target_link_libraries(your_target Private sim_logger::c_api) # C
```

