#include "logger.hpp"

#include <iomanip>
#include <chrono>

namespace c2l::core
{

Logger g_logger;

// Implementation of format_string method
std::string ILogger::format_string(const std::string& format, const std::vector<std::string>& args)
{
    std::string result;
    result.reserve(format.length() + args.size() * 16); // Pre-allocate memory
    
    size_t arg_index = 0;
    size_t pos = 0;
    
    while (pos < format.length()) {
        // Find the next placeholder
        size_t placeholder_start = format.find('{', pos);
        
        if (placeholder_start == std::string::npos) {
            // No more placeholders, append the rest of the string
            result.append(format, pos);
            break;
        }
        
        size_t placeholder_end = format.find('}', placeholder_start);
        
        if (placeholder_end == std::string::npos) {
            // No closing brace, append the rest and break
            result.append(format, pos);
            break;
        }
        
        // Check if it's an empty placeholder {}
        if (placeholder_end - placeholder_start == 1) {
            // Append text before placeholder
            result.append(format, pos, placeholder_start - pos);
            
            // Replace placeholder with argument
            if (arg_index < args.size()) {
                result.append(args[arg_index]);
                arg_index++;
            } else {
                // Not enough arguments, keep the placeholder
                result.append("{}");
            }
            
            pos = placeholder_end + 1;
        } else {
            // It's a non-empty placeholder like {0}, {name}, etc.
            // For now, we'll only support empty {} placeholders
            // Append text including the placeholder
            result.append(format, pos, placeholder_end + 1 - pos);
            pos = placeholder_end + 1;
        }
    }
    
    // If there are remaining arguments, append them (useful for debugging)
    if (arg_index < args.size()) {
        result.append(" [Extra args: ");
        for (size_t i = arg_index; i < args.size(); ++i) {
            if (i > arg_index) result.append(", ");
            result.append(args[i]);
        }
        result.append("]");
    }
    
    return result;
}

Logger::Logger()
{
    init("application.log", core::LogLevel::TRACE);
}

Logger::~Logger()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_log_file.is_open())
    {
        m_log_file.close();
    }
}

void Logger::init(const std::string& file_name, LogLevel level)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Close the existing log file if open
    if (m_log_file.is_open())
    {
        m_log_file.close();
    }

    // Open new log file if specified filename
    if (!file_name.empty())
    {
        m_log_file.open(file_name, std::ios::out | std::ios::app);
        if (!m_log_file.is_open())
        {
            throw std::runtime_error("Failed to open log file: " + file_name);
        }
    }

    m_log_level.store(level);
}

void Logger::set_log_level(LogLevel level)
{
    m_log_level.store(level);
}

void Logger::log(LogLevel level, const std::string& message) 
{
    // Early exit if message level is below threshold
    if (level < m_log_level.load()) { return; }

    // Format the log message
    std::ostringstream oss;
    oss << get_time_stamp() 
        << " [" << log_level_to_string(level) << "] " 
        << message;


    // Thread-safe output operations
    std::lock_guard<std::mutex> lock(m_mutex);
    
    const std::string log_message = oss.str();
    
    // Colorized console output
    std::cout << log_level_to_color(level) 
              << log_message 
              << "\033[0m" << std::endl; // Set back console color to default

    // File output (if enabled)
    if (m_log_file.is_open()) 
    {
        m_log_file << log_message << std::endl;
    }
}

std::string Logger::get_time_stamp() 
{
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto t_time = system_clock::to_time_t(now);
    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t_time), "%Y-%m-%d %H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

std::string Logger::log_level_to_string(LogLevel level) 
{
    switch (level) 
    {
        case LogLevel::TRACE:   return "TRACE";
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

std::string Logger::log_level_to_color(LogLevel level) 
{
    switch (level) 
    {
        case LogLevel::TRACE:   return "\033[37m";       // White
        case LogLevel::DEBUG:   return "\033[36m";       // Cyan
        case LogLevel::INFO:    return "\033[32m";       // Green
        case LogLevel::WARNING: return "\033[33m";       // Yellow
        case LogLevel::ERROR:   return "\033[31m";       // Red
        case LogLevel::FATAL:   return "\033[41m\033[97m"; // Red bg, White text
        default:                return "\033[0m";        // Reset
    }
}

} //namespace c2l::core