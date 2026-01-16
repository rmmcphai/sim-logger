# sim-logger
Small library to handle logging and message handling for C/C++ and simulations.

## Recipe Cards

**Build with make**

```bash
cmake -S . -B build 
cmake --build build
```

**Build with Ninja**
```bash
cmake -S . -B build -G Ninja 
cmake --build build
```
*Note:  Ninja is generally more consistent across machines. If you can afford
        the overhead I would recommend using it over make.*

**Build and test**
```bash
cmake -S . -B build -G Ninja 
cmake --build build
ctest --test-dir build --output-on-failure
```

*Note:  --ouput-on-failuer only shows a tests stdout/stderr if the test fails.
        Keeps passing runs clean while letting us know when something breaks.*

**Alternate build types**

```bash
cmake -S . -B build -G <Generator> -DCMAKE_BUILD_TYPE=<Type>
```

Debug: -g (Often minimal optimization.)
Release: -O3 -DNDEBUG
RelWithDebInfo: -O2 -g -DNDEBUG


 
