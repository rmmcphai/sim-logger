# Stand-alone (Non-Trick) Usage Guide

This document describes how to use **sim-logger** in applications that do **not** run under Trick.

## Concepts

### Logger names and hierarchy

Loggers are identified by a **dotted name**:

- `sim`
- `sim.propulsion`
- `sim.propulsion.valve`

Child loggers inherit configuration (level, sinks, immediate flush) from parents unless overridden.

### Records and metadata

Each log call materializes a `LogRecord` containing:

- severity (`Level`)
- simulation time (`sim_time`)
- mission elapsed time (`met`)
- wall clock time (`wall_time_ns`)
- thread id
- logger name
- source location (file/line/function)
- message payload
- optional tags

In stand-alone mode, simulation time / MET come from the **global time source**. By default this uses a
safe fallback suitable for unit tests and tools; under Trick a dedicated time source will be provided by the
adapter module.

## Formatting

Formatting is controlled by a `PatternFormatter`:

```cpp
PatternFormatter fmt("{met} {level} [{logger}] {msg} ({file}:{line})");
```

Common tokens:

- `{sim_time}`
- `{met}`
- `{wall_time_ns}`
- `{level}`
- `{logger}`
- `{msg}`
- `{file}` `{line}` `{function}`

Unknown tokens are preserved verbatim.

## Sinks

### ConsoleSink

Writes formatted lines to stdout. Color mode:

- `Auto` (TTY-aware)
- `On`
- `Off`

### FileSink

Appends formatted lines to a file. Optional durable flush (fsync) is supported.

### RotatingFileSink

Extends FileSink with:

- size-based rotation
- timestamp rename
- retention (`max_rotated_files`)
- collision-safe naming for same-second rotations

## Asynchronous logging (recommended for high-rate logging)

Wrap any sink in an `AsyncSink`:

```cpp
auto async_file = std::make_shared<AsyncSink>(rotating, AsyncOptions{4096, OverflowPolicy::Block});
```

Overflow policy:

- **Block** (default): producers wait for space
- **DropNewest**: drop the incoming record when full
- **DropOldest**: evict the oldest queued record to admit the new one

`flush()` guarantees that once it returns, all queued records have been written and the wrapped sink has
been flushed.

## C models

The C API (`logger_c_api/include/sim_logger/c_api.h`) is for logging from C code. Typical pattern:

1. Configure sinks/levels from C++ initialization code.
2. C models call `sim_logger_get()` once (or cache the handle) and call `sim_logger_logf()`.

## Recommended recipes

### Minimal development setup

- Root logger level: `Debug`
- Console sink: enabled, color `Auto`
- Immediate flush: on (debug only)

### Production simulation setup

- Root logger level: `Info` or `Warn`
- Console sink: `Warn`+ only (via logger level or a dedicated logger)
- Rotating file sink: enabled
- File sink wrapped with `AsyncSink` using `Block` policy
- Immediate flush: off
