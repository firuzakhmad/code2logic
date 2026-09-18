#ifndef LOGGER_HPP
#define LOGGER_HPP


/**
 * @brief Logging system with file and console output
 *
 * Features:
 * - Thread-safe logging
 * - Multiple log levels (TRACE to FATAL)
 * - Color-coded console output
 * - Timestamp precision to milliseconds
 * - Interface-based design for testability
 * - Format string support with {} placeholders
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <string>
#include <atomic>
#include <vector>


namespace c2l::core
{
    /**
     * @enum LogLevel
     * @brief Severity levels for log messages
     *        with lower severity than the current
     *        threshold will be filtered out.
     */
    enum class LogLevel
    {
        TRACE,
        DEBUG,
        INFO,
        WARNING,
        ERR,
        FATAL
    };

    /**
     * @class ILogger
     * @brief Abstract interface for logging functionality
     *
     * Provides a contract for logger implementations,
     * enabling dependency injection and easy mocking for
     * unit testing.
     */
    class ILogger
    {
    public:
        virtual ~ILogger() = default;

        /**
         * @brief Initialize the logger
         * @param file_name Path to log file (emtpy for no file logging)
         * @param level Minimum severity level to log
         */
        virtual void init(const std::string& file_name, LogLevel level = LogLevel::INFO) = 0;

        /**
         * @brief Set the minimum log level
         * @param level Message below this level will be filtered
         */
        virtual void set_log_level(LogLevel level) = 0;

        /**
         * @brief Log a message with specified severity level
         * @param level Severity level of the message
         * @param message The message to log
         */
        virtual void log(LogLevel level, const std::string& message) = 0;

        [[nodiscard]] virtual LogLevel get_log_level() const = 0;

        /**
         * @brief Log a formatted message with {} placeholders
         * @tparams Args Types of the arguments to format
         * @param level Security level of the message
         * @param format Format string with {} placeholder
         * @param args Arguments to replace placeholders
         */
        template<typename... Args>
        void log_format(LogLevel level, const std::string& format, Args&&... args)
        {
            if (level < get_log_level()) return;
            log(level, format_message(format, std::forward<Args>(args)...));
        }

        /**
         * @brief Converts a single argument to its string representations.
         */
        template<typename T>
        static std::string to_string(T&& arg)
        {
        std::ostringstream oss;
        oss << std::forward<T>(arg);
        return oss.str();
        }

        /**
         * @brief Formats a message by replacing {} placeholders with arguments.
         */
        template<typename... Args>
        static std::string format_message(const std::string& format, Args&&... args)
        {
            std::vector<std::string> formatted_args = {to_string(std::forward<Args>(args))...};
            return format_string(format, formatted_args);
        }

        /**
         * @brief Replaces {} placeholders in format string with provided arguments.
         */
        static std::string format_string(const std::string& format, const std::vector<std::string>& args);
    };


    /**
     * @class Logger
     * @brief Concrete thread-safe logger implementation
     *
     * Supports simultaneous console (with color) and file output with configurable
     * log level filtering.
     */
    class Logger final : public ILogger
    {
    public:
        /**
         * @brief Construct a new Logger object
         *
         * Note: Logger must be initialized with init() before use
         */
        Logger();

        /**
         * @brief Destroy the Logger object
         *
         * Ensures log file is properly closed
         */
        ~Logger() override;

        // Delete copy/move operations to ensure thread safety
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

        /**
         * @copydoc ILogger::init
         */
        void init(const std::string& file_name, LogLevel level = LogLevel::INFO) override;

        /**
         * @copydoc ILogger::set_log_level
         */
        void set_log_level(LogLevel level) override;

        /**
         * @copydoc ILogger::log
         */
        void log(LogLevel level, const std::string& message) override;

        [[nodiscard]] LogLevel get_log_level() const override { return m_log_level.load(); }

        /**
         * @brief Logs a formatted message with {} placeholders at the specified severity level.
         *
         * This method supports format strings with {} placeholders that will be replaced
         * with the provided arguments in order.
         *
         * Example: logger.log_format(LogLevel::INFO, "User {} has {} messages", "John", 5);
         * Output: "User John has 5 messages"
         *
         * @tparam Args Types of the message arguments
         * @param level Severity level of the log message
         * @param format Format string with {} placeholders
         * @param args Arguments to replace placeholders
         */
        template<typename... Args>
        void log_format(LogLevel level, const std::string& format, Args&&... args)
        {
            // Early exit if message level is below threshold
            if (level < m_log_level.load()) { return; }

            // Format the message with placeholders
            std::string formatted_message = ILogger::format_message(format, std::forward<Args>(args)...);

            // Thread-safe output operations
            std::lock_guard<std::mutex> lock(m_mutex);

            // Creating final log message with timestamp and level
            std::ostringstream oss;
            oss << get_time_stamp()
                << " [" << log_level_to_string(level) << "] "
                << formatted_message;

            const std::string log_message = oss.str();

            // Colorized console output
            std::cout << log_level_to_color(level)
                      << log_message
                      << "\033[0m" << std::endl;

            // File output (if enabled)
            if (m_log_file.is_open())
            {
                m_log_file << log_message << std::endl;
            }
        }

    private:
        static std::string get_time_stamp();
        static std::string log_level_to_string(LogLevel level);
        static std::string log_level_to_color(LogLevel level);

        std::ofstream m_log_file;
        std::atomic<LogLevel> m_log_level;
        std::mutex m_mutex;
    };


    /**
     * @class NullLogger
     * @brief Null object pattern implementation of ILogger
     *
     * Provides a no-op logger for testing or when logging needs to be disabled.
     */
    class NullLogger final : public ILogger
    {
    public:
        void init(const std::string&, LogLevel) override {}
        void set_log_level(LogLevel) override {}
        void log(LogLevel, const std::string&) override {}
        [[nodiscard]] LogLevel get_log_level() const override { return LogLevel::FATAL; }
    };

// New macros with format string support
#define LOG_FORMAT_IMPL(logger, level, format, ...) \
    do { \
        if (logger) logger->log_format(c2l::core::LogLevel::level, format __VA_OPT__(,) __VA_ARGS__); \
    } while(0)

#define LOG_FORMAT_GLOBAL_IMPL(level, format, ...) \
    do { \
        c2l::core::get_logger().log_format(c2l::core::LogLevel::level, format __VA_OPT__(,) __VA_ARGS__); \
    } while(0)

// Formating string macros
#define LOG_TRACE_FORMAT(logger, format, ...)    LOG_FORMAT_IMPL(logger, TRACE, format, ##__VA_ARGS__)
#define LOG_DEBUG_FORMAT(logger, format, ...)    LOG_FORMAT_IMPL(logger, DEBUG, format, ##__VA_ARGS__)
#define LOG_INFO_FORMAT(logger, format, ...)     LOG_FORMAT_IMPL(logger, INFO, format, ##__VA_ARGS__)
#define LOG_WARNING_FORMAT(logger, format, ...)  LOG_FORMAT_IMPL(logger, WARNING, format, ##__VA_ARGS__)
#define LOG_ERROR_FORMAT(logger, format, ...)    LOG_FORMAT_IMPL(logger, ERR, format, ##__VA_ARGS__)
#define LOG_FATAL_FORMAT(logger, format, ...)    LOG_FORMAT_IMPL(logger, FATAL, format, ##__VA_ARGS__)

// Global format string macros
#define LOG_TRACE(format, ...)    LOG_FORMAT_GLOBAL_IMPL(TRACE, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...)    LOG_FORMAT_GLOBAL_IMPL(DEBUG, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)     LOG_FORMAT_GLOBAL_IMPL(INFO, format, ##__VA_ARGS__)
#define LOG_WARNING(format, ...)  LOG_FORMAT_GLOBAL_IMPL(WARNING, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...)    LOG_FORMAT_GLOBAL_IMPL(ERR, format, ##__VA_ARGS__)
#define LOG_FATAL(format, ...)    LOG_FORMAT_GLOBAL_IMPL(FATAL, format, ##__VA_ARGS__)

Logger& get_logger();

} // namespace c2l::core

#endif // LOGGER_HPP