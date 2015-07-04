#ifndef __LOG_VISOR_HPP__
#define __LOG_VISOR_HPP__

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <memory>

namespace LogVisor
{

#if _WIN32 && UNICODE
#define LOG_UCS2 1
#endif

/**
 * @brief Severity level for log messages
 */
enum Level
{
    Info,        /**< Non-error informative message */
    Warning,     /**< Non-error warning message */
    Error,       /**< Recoverable error message */
    FatalError   /**< Non-recoverable error message (calls abort) */
};

/**
 * @brief Backend interface for receiving app-wide log events
 */
struct ILogger
{
    virtual ~ILogger() {}
    virtual void report(const char* modName, Level severity,
                        const char* format, va_list ap)=0;
    virtual void report(const char* modName, Level severity,
                        const wchar_t* format, va_list ap)=0;
    virtual void reportSource(const char* modName, Level severity,
                              const char* file, unsigned linenum,
                              const char* format, va_list ap)=0;
    virtual void reportSource(const char* modName, Level severity,
                              const char* file, unsigned linenum,
                              const wchar_t* format, va_list ap)=0;
};

/**
 * @brief Assign calling thread a descriptive name
 * @param name Descriptive thread name
 */
void RegisterThreadName(const char* name);

/**
 * @brief Centralized logger vector
 *
 * All loggers added to this vector will receive reports as they occur
 */
extern std::vector<std::unique_ptr<ILogger>> MainLoggers;

/**
 * @brief Centralized frame index
 *
 * All log events include this as part of their timestamp if non-zero.
 * The default value is zero, the app is responsible for updating it
 * within its main loop.
 */
extern uint64_t FrameIndex;

/**
 * @brief Restore centralized logger vector to default state (silent operation)
 */
static inline void UnregisterLoggers() {MainLoggers.clear();}

/**
 * @brief Construct and register a real-time console logger singleton
 *
 * This will output to stderr on POSIX platforms and spawn a new console window on Windows.
 * If there's already a registered console logger, this is a no-op.
 */
void RegisterConsoleLogger();

/**
 * @brief Construct and register a file logger
 * @param filepath Path to write the file
 *
 * If there's already a file logger registered to the same file, this is a no-op.
 */
void RegisterFileLogger(const char* filepath);

#if LOG_UCS2

/**
 * @brief Construct and register a file logger (wchar_t version)
 * @param filepath Path to write the file
 *
 * If there's already a file logger registered to the same file, this is a no-op.
 */
void RegisterFileLogger(const wchar_t* filepath);

#endif

/**
 * @brief This is constructed per-subsystem in a locally centralized fashon
 */
class LogModule
{
    const char* m_modName;
public:
    LogModule(const char* modName) : m_modName(modName) {}

    /**
     * @brief Route new log message to centralized ILogger
     * @param severity Level of log report severity
     * @param format Standard printf-style format string
     */
    template <typename CharType>
    inline void report(Level severity, const CharType* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        for (auto& logger : MainLoggers)
            logger->report(m_modName, severity, format, ap);
        va_end(ap);
        if (severity == FatalError)
            abort();
    }

    /**
     * @brief Route new log message with source info to centralized ILogger
     * @param severity Level of log report severity
     * @param file Source file name from __FILE__ macro
     * @param linenum Source line number from __LINE__ macro
     * @param format Standard printf-style format string
     */
    template <typename CharType>
    inline void reportSource(Level severity, const char* file, unsigned linenum, const CharType* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        for (auto& logger : MainLoggers)
            logger->reportSource(m_modName, severity, file, linenum, format, ap);
        va_end(ap);
        if (severity == FatalError)
            abort();
    }
};

}

#endif // __LOG_VISOR_HPP__
