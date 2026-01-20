#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdint.h>

#if defined(_WIN32) && defined(SIM_LOGGER_C_API_BUILD)
  #define SIM_LOGGER_C_API __declspec(dllexport)
#elif defined(_WIN32)
  #define SIM_LOGGER_C_API __declspec(dllimport)
#else
  #define SIM_LOGGER_C_API
#endif

// Opaque logger handle for C.
typedef struct sim_logger_logger sim_logger_logger_t;

// C-facing levels (stable ABI)
typedef enum sim_logger_level {
  SIM_LOGGER_LEVEL_DEBUG = 0,
  SIM_LOGGER_LEVEL_INFO  = 1,
  SIM_LOGGER_LEVEL_WARN  = 2,
  SIM_LOGGER_LEVEL_ERROR = 3,
  SIM_LOGGER_LEVEL_FATAL = 4
} sim_logger_level_t;

/**
 * @brief Acquire a logger by name (hierarchical dotted names supported).
 *
 * Ownership:
 * - Returns a reference-counted handle. Call sim_logger_release() when done.
 */
SIM_LOGGER_C_API sim_logger_logger_t* sim_logger_get(const char* name);

/**
 * @brief Release a logger handle acquired via sim_logger_get().
 * Safe to call with NULL.
 */
SIM_LOGGER_C_API void sim_logger_release(sim_logger_logger_t* logger);

/**
 * @brief Log a pre-formatted message.
 *
 * @param logger Logger handle (required).
 * @param level  Severity.
 * @param file   Source file (may be NULL).
 * @param line   Source line.
 * @param func   Function (may be NULL).
 * @param msg    Message (may be NULL -> treated as "").
 */
SIM_LOGGER_C_API void sim_logger_log(sim_logger_logger_t* logger,
                                     sim_logger_level_t level,
                                     const char* file,
                                     uint32_t line,
                                     const char* func,
                                     const char* msg);

/**
 * @brief printf-style formatted logging (C varargs).
 */
SIM_LOGGER_C_API void sim_logger_logf(sim_logger_logger_t* logger,
                                      sim_logger_level_t level,
                                      const char* file,
                                      uint32_t line,
                                      const char* func,
                                      const char* fmt,
                                      ...);

/**
 * @brief va_list variant of sim_logger_logf().
 */
SIM_LOGGER_C_API void sim_logger_vlogf(sim_logger_logger_t* logger,
                                       sim_logger_level_t level,
                                       const char* file,
                                       uint32_t line,
                                       const char* func,
                                       const char* fmt,
                                       va_list ap);

/**
 * @brief Flush all effective sinks for this logger (best-effort).
 */
SIM_LOGGER_C_API void sim_logger_flush(sim_logger_logger_t* logger);

#ifdef __cplusplus
}  // extern "C"
#endif
