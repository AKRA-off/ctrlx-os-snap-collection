/*********************************************************************
* Software License Agreement (BSD License)
 *
 *  Copyright (c) 2025, Kassow Robots
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Kassow Robots nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

#ifndef KORD_API_LOGGER_HPP
#define KORD_API_LOGGER_HPP

#include <chrono>
#include <cstdarg>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

namespace kr2::kord {

/**
 * @enum LogLevel
 * @brief Represents the severity levels for logging.
 */
enum class LogLevel {
    DEBUG = 0, /**< Detailed information, typically of interest only when diagnosing problems. */
    INFO,      /**< Confirmation that things are working as expected. */
    WARN,      /**< Indication that something unexpected happened, or indicative of some problem in the near future. */
    ERROR,     /**< Error events that might still allow the application to continue running. */
    OFF        /**< Disables logging. */
};

/**
 * @class Logger
 * @brief Singleton class for managing logging with support for multiple log levels and output options.
 *
 * The Logger class provides thread-safe logging functionality, allowing messages to be logged
 * to the console and/or a file with different severity levels. It supports log rotation based on
 * file size and maintains a specified number of rotated log files.
 */
class Logger {
public:
    /**
     * @brief Retrieves the singleton instance of the Logger.
     *
     * @return Reference to the Logger instance.
     */
    static Logger &getInstance()
    {
        static Logger instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    Logger(const Logger &) = delete;                /**< Deleted copy constructor to enforce singleton pattern. */
    Logger &operator=(const Logger &) = delete;     /**< Deleted assignment operator to enforce singleton pattern. */

    /**
     * @brief Sets the minimum log level. Messages below this level will be ignored.
     *
     * @param level The desired minimum LogLevel.
     */
    void setLogLevel(LogLevel level)
    {
        std::lock_guard lock(mtx);
        currentLevel = level;
    }

    /**
     * @brief Enables logging to a specified file. If a file is already open, it will be closed first.
     *
     * @param filePath The path to the log file.
     * @return True if the file was successfully opened, false otherwise.
     */
    bool enableFileLogging(const std::string &filePath)
    {
        std::lock_guard lock(mtx);
        if (logFile.is_open()) {
            logFile.close();
        }
        logFile.open(filePath, std::ios::app);
        if (!logFile.is_open()) {
            log(LogLevel::ERROR, "Failed to open log file");
            return false;
        }
        currentLogFilePath = filePath;
        return true;
    }

    /**
     * @brief Disables logging to the file by closing any open log file.
     */
    void disableFileLogging()
    {
        std::lock_guard lock(mtx);
        if (logFile.is_open()) {
            logFile.close();
        }
        currentLogFilePath.clear();
    }

    /**
     * @brief Logs a debug message with formatted input.
     *
     * @param format The format string.
     * @param ... Variadic arguments corresponding to the format string.
     */
    void debug(const char *format, ...)
    {
        if (LogLevel::DEBUG < currentLevel || currentLevel == LogLevel::OFF)
            return;
        va_list args;
        va_start(args, format);
        std::string formattedMessage = formatString(format, args);
        va_end(args);
        log(LogLevel::DEBUG, formattedMessage);
    }

    /**
     * @brief Logs an info message with formatted input.
     *
     * @param format The format string.
     * @param ... Variadic arguments corresponding to the format string.
     */
    void info(const char *format, ...)
    {
        if (LogLevel::INFO < currentLevel || currentLevel == LogLevel::OFF)
            return;
        va_list args;
        va_start(args, format);
        std::string formattedMessage = formatString(format, args);
        va_end(args);
        log(LogLevel::INFO, formattedMessage);
    }

    /**
     * @brief Logs a warning message with formatted input.
     *
     * @param format The format string.
     * @param ... Variadic arguments corresponding to the format string.
     */
    void warn(const char *format, ...)
    {
        if (LogLevel::WARN < currentLevel || currentLevel == LogLevel::OFF)
            return;
        va_list args;
        va_start(args, format);
        std::string formattedMessage = formatString(format, args);
        va_end(args);
        log(LogLevel::WARN, formattedMessage);
    }

    /**
     * @brief Logs an error message with formatted input.
     *
     * @param format The format string.
     * @param ... Variadic arguments corresponding to the format string.
     */
    void error(const char *format, ...)
    {
        if (LogLevel::ERROR < currentLevel || currentLevel == LogLevel::OFF)
            return;
        va_list args;
        va_start(args, format);
        std::string formattedMessage = formatString(format, args);
        va_end(args);
        log(LogLevel::ERROR, formattedMessage);
    }

    /**
     * @brief Internal function to handle the actual logging of messages.
     *
     * Formats the message with a timestamp and log level, then outputs it to the console and/or log file.
     *
     * @param level The LogLevel of the message.
     * @param message The formatted log message.
     */
    void log(LogLevel level, const std::string &message)
    {
        std::lock_guard lock(mtx);

        if (level < currentLevel || currentLevel == LogLevel::OFF) {
            return; // Do not log messages below the current log level or if logging is off
        }

        // Get current time
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now{};
        localtime_r(&time_t_now, &tm_now);

        // Format the time
        std::ostringstream timeStream;
        timeStream << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");

        // Convert log level to string
        std::string levelStr;
        switch (level) {
        case LogLevel::DEBUG:
            levelStr = "DEBUG";
            break;
        case LogLevel::INFO:
            levelStr = "INFO";
            break;
        case LogLevel::WARN:
            levelStr = "WARN";
            break;
        case LogLevel::ERROR:
            levelStr = "ERROR";
            break;
        default:
            levelStr = "UNKNOWN";
        }

        // Compose the final log message with file and line info
        std::string finalMessage = "[" + timeStream.str() + "] [" + levelStr + "] " + message + "\n";

        // Output to console if enabled
        if (consoleLogging) {
            std::cout << finalMessage;
        }

        // Output to file if set
        if (logFile.is_open()) {
            logFile << finalMessage;
            logFile.flush(); // Ensure the message is written immediately
        }
    }

private:
    /**
     * @brief Private constructor for the singleton pattern.
     *
     * Initializes default logging settings.
     */
    Logger()
        : currentLevel(LogLevel::DEBUG), consoleLogging(true), maxFileSize(15 * 1024 * 1024), // 15 MB default
          maxFiles(3)                                                                         // Keep up to 3 log files
    {
    }

    /**
     * @brief Destructor that ensures the log file is closed if open.
     */
    ~Logger()
    {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    /**
     * @brief Helper function to format strings using variadic arguments.
     *
     * @param format The format string.
     * @param args The variadic arguments.
     * @return The formatted string.
     */
    static std::string formatString(const char *format, va_list args)
    {
        va_list args_copy;
        va_copy(args_copy, args);
        int length = std::vsnprintf(nullptr, 0, format, args_copy);
        va_end(args_copy);

        if (length < 0) {
            return "Formatting error";
        }

        std::string formatted;
        formatted.resize(length + 1);
        std::vsnprintf(&formatted[0], formatted.size(), format, args);
        formatted.pop_back(); // Remove the extra null terminator
        return formatted;
    }

    /**
     * @brief Rotates log files when the current log file exceeds the maximum size.
     *
     * Deletes the oldest log file if the maximum number of log files is reached,
     * and renames existing log files to maintain the rotation sequence.
     */
    void rotateLogs()
    {
        // Delete the oldest log file if maxFiles limit is reached
        std::string oldestLog = currentLogFilePath + "." + std::to_string(maxFiles - 1);
        if (std::filesystem::exists(oldestLog)) {
            std::filesystem::remove(oldestLog);
        }

        // Rename existing log files
        for (int i = maxFiles - 2; i >= 0; --i) {
            std::string oldName = (i == 0) ? currentLogFilePath : currentLogFilePath + "." + std::to_string(i);
            if (std::filesystem::exists(oldName)) {
                std::string newName = currentLogFilePath + "." + std::to_string(i + 1);
                std::filesystem::rename(oldName, newName);
            }
        }
    }

    LogLevel currentLevel;                /**< Current minimum log level. */
    bool consoleLogging;                  /**< Flag indicating if console logging is enabled. */
    std::size_t maxFileSize;              /**< Maximum size in bytes before log rotation occurs. */
    int maxFiles;                         /**< Maximum number of rotated log files to keep. */
    std::ofstream logFile;                /**< Output file stream for logging to a file. */
    std::string currentLogFilePath;       /**< Path to the current log file. */
    std::mutex mtx;                       /**< Mutex for ensuring thread safety. */

public:
    /**
     * @brief Sets the maximum file size in bytes before log rotation is triggered.
     *
     * @param bytes The maximum file size in bytes.
     */
    void setMaxFileSize(std::size_t bytes)
    {
        std::lock_guard lock(mtx);
        maxFileSize = bytes;
    }

    /**
     * @brief Sets the maximum number of rotated log files to retain.
     *
     * @param count The maximum number of log files.
     */
    void setMaxFiles(int count)
    {
        std::lock_guard lock(mtx);
        maxFiles = count;
    }

private:
    // Additional private members or helper functions can be added here
};

} // namespace kr2::kord

/**
 * @def KORD_LOG_DEBUG
 * @brief Macro for logging debug messages.
 *
 * Usage:
 * @code
 * KORD_LOG_DEBUG("Debug message: %d", debugValue);
 * @endcode
 */
#define KORD_LOG_DEBUG(message)                                                                                         \
    do {                                                                                                                \
        std::ostringstream oss__;                                                                                       \
        oss__ << message;                                                                                               \
        kr2::kord::Logger::getInstance().log(kr2::kord::LogLevel::DEBUG, oss__.str());                                  \
    } while (0)

/**
 * @def KORD_LOG_INFO
 * @brief Macro for logging info messages.
 *
 * Usage:
 * @code
 * KORD_LOG_INFO("Info message: %s", infoString.c_str());
 * @endcode
 */
#define KORD_LOG_INFO(message)                                                                                          \
    do {                                                                                                                \
        std::ostringstream oss__;                                                                                       \
        oss__ << message;                                                                                               \
        kr2::kord::Logger::getInstance().log(kr2::kord::LogLevel::INFO, oss__.str());                                   \
    } while (0)

/**
 * @def KORD_LOG_WARN
 * @brief Macro for logging warning messages.
 *
 * Usage:
 * @code
 * KORD_LOG_WARN("Warning message: %f", warningValue);
 * @endcode
 */
#define KORD_LOG_WARN(message)                                                                                          \
    do {                                                                                                                \
        std::ostringstream oss__;                                                                                       \
        oss__ << message;                                                                                               \
        kr2::kord::Logger::getInstance().log(kr2::kord::LogLevel::WARN, oss__.str());                                   \
    } while (0)

/**
 * @def KORD_LOG_ERROR
 * @brief Macro for logging error messages.
 *
 * Usage:
 * @code
 * KORD_LOG_ERROR("Error message: %s", errorString.c_str());
 * @endcode
 */
#define KORD_LOG_ERROR(message)                                                                                         \
    do {                                                                                                                \
        std::ostringstream oss__;                                                                                       \
        oss__ << message;                                                                                               \
        kr2::kord::Logger::getInstance().log(kr2::kord::LogLevel::ERROR, oss__.str());                                  \
    } while (0)

#endif // KORD_API_LOGGER_HPP
