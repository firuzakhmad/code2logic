/**
 * @brief ad's runtime diagnostic and failure handling system
 * 
 * Provides hardened assertion macros for:
 * - Debugging (C2L_ASSERT)
 * - Hardware/sensor validation (C2L_CHECK_SUCCESS)
 * - Platform-aware debug traps
 * 
 * Key Features:
 * - Detailed failure context (file, line, function)
 * - Integrated with ad logging system
 * - Immediate termination on critical failures
 * - Cross-platform debug break support
 * 
 * Usage:
 * @code
 * C2L_ASSERT(ptr != nullptr, "Received null pointer");
 * C2L_CHECK_SUCCESS(sensor_init(), "Sensor initialization failed");
 * @endcode
 */

#ifndef ASSERT_HPP
#define ASSERT_HPP

#include "core/utils/logger/logger.hpp"

#include <iostream>
#include <cstdlib>

/**
 * @def DEBUG_BREAK()
 * @brief Platform-specific debug trap instruction
 * 
 * Behavior:
 * - Windows: __debugbreak() (MSVC)
 * - Unix: __builtin_trap() (GCC/Clang)
 * - Fallback: std::abort()
 */
#ifdef _MSC_VER
    #define DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
    #define DEBUG_BREAK() __builtin_trap()
#else
    #define DEBUG_BREAK() std::abort()
#endif

namespace c2l::core
{
    /**
     * @brief Logs assertion failure details to stderr
     * @param expr The failed expression as string
     * @param file Source file name
     * @param line Source line number
     * @param function Enclosing function name
     * 
     * @note Used internally by C2L_ASSERT macro
     */
    inline void log_assertion_failure(const char* expr, const char* file, int line, const char* function)
    {
        std::cerr << "[ASSERTION FAILED] " << expr << "\n"
                  << "Location: " << file << ":" << line << "\n"
                  << "Function: " << function << "\n";
    }


    /**
     * @def C2L_ASSERT(expr, ...)
     * @brief Runtime assertion with optional message
     * 
     * Behavior:
     * 1. Evaluates expression
     * 2. On failure:
     *    - Logs to both stderr and ad logging system
     *    - Triggers debug break
     *    - Terminates program
     * 
     * @param expr Boolean expression to test
     * @param ... Optional format string and arguments
     */
    #define C2L_ASSERT(expr, ...) \
    do { \
        if (!(expr)) { \
            LOG_FATAL("Assertion failed: {}", #expr); \
            __VA_OPT__(LOG_FATAL("  Message: {}", __VA_ARGS__);) \
            LOG_FATAL("  Location: {}:{}\n  Function: {}", __FILE__, __LINE__, __FUNCTION__); \
            DEBUG_BREAK(); \
            std::abort(); \
        } \
    } while (false)

    /**
     * @def C2L_CHECK_SUCCESS(call, ...)
     * @brief Validates hardware/sensor operation results
     * 
     * Specialized for:
     * - Device driver calls
     * - Hardware interface verification
     * - Critical system operations
     * 
     * @param call Function call returning error code (0=success)
     * @param ... Optional context message
     */
    #define C2L_CHECK_SUCCESS(call, ...) \
    do { \
        auto result = call; \
        if (result) { \
            LOG_FATAL("[C2L Error] {} returned failure at {}:{}", #call, __FILE__, __LINE__); \
            __VA_OPT__(LOG_FATAL("Context: {}", __VA_ARGS__);) \
            DEBUG_BREAK(); \
            std::abort(); \
        } \
    } while (false)

} // namespace c2l::core

#endif // ASSERT_HPP
