#if _WIN32
#define _WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#include <chrono>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <stdio.h>
#include "LogVisor/LogVisor.hpp"

/* ANSI sequences */
#define RED "\x1b[1;31m"
#define YELLOW "\x1b[1;33m"
#define GREEN "\x1b[1;32m"
#define MAGENTA "\x1b[1;35m"
#define CYAN "\x1b[1;36m"
#define BOLD "\x1b[1m"
#define NORMAL "\x1b[0m"

namespace LogVisor
{

static std::unordered_map<std::thread::id, const char*> ThreadMap;
void RegisterThreadName(const char* name)
{
    ThreadMap[std::this_thread::get_id()] = name;
#if __APPLE__
    pthread_setname_np(name);
#elif __linux__
    pthread_setname_np(pthread_self(), name);
#elif _MSC_VER
    struct
    {
        DWORD dwType; // Must be 0x1000.
        LPCSTR szName; // Pointer to name (in user addr space).
        DWORD dwThreadID; // Thread ID (-1=caller thread).
        DWORD dwFlags; // Reserved for future use, must be zero.
    } info = {0x1000, name, (DWORD)-1, 0};
    __try
    {
        RaiseException(0x406D1388, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
#endif
}

std::vector<std::unique_ptr<ILogger>> MainLoggers;
static std::chrono::steady_clock MonoClock;
static std::chrono::steady_clock::time_point GlobalStart = MonoClock.now();
static inline std::chrono::steady_clock::duration CurrentUptime()
{return MonoClock.now() - GlobalStart;}
uint64_t FrameIndex = 0;

#if _WIN32
static HANDLE Term = 0;
#else
static const char* Term = nullptr;
#endif
static bool XtermColor = false;
struct ConsoleLogger : public ILogger
{
    std::mutex m;
    ConsoleLogger()
    {
#if _WIN32
        if (!Term)
            Term = GetStdHandle(STD_ERROR_HANDLE);
#else
        if (!Term)
        {
            Term = getenv("TERM");
            if (Term && !strncmp(Term, "xterm", 5))
            {
                XtermColor = true;
                putenv((char*)"TERM=xterm-16color");
            }
        }
#endif
    }

    static void _reportHead(const char* modName, const char* sourceInfo, Level severity)
    {
        std::chrono::steady_clock::duration tm = CurrentUptime();
        double tmd = tm.count() *
            std::chrono::steady_clock::duration::period::num /
            (double)std::chrono::steady_clock::duration::period::den;
        std::thread::id thrId = std::this_thread::get_id();
        const char* thrName = nullptr;
        if (ThreadMap.find(thrId) != ThreadMap.end())
            thrName = ThreadMap[thrId];

#if _WIN32
        SetConsoleTextAttribute(Term, FOREGROUND_INTENSITY);
        fprintf(stderr, "[");
        SetConsoleTextAttribute(Term, FOREGROUND_INTENSITY | FOREGROUND_GREEN);
        fprintf(stderr, "%5.4f ", tmd);
        if (FrameIndex)
            fprintf(stderr, "(%llu) ", FrameIndex);
        switch (severity)
        {
        case Info:
            SetConsoleTextAttribute(Term, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE);
            fprintf(stderr, "INFO");
            break;
        case Warning:
            SetConsoleTextAttribute(Term, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
            fprintf(stderr, "WARNING");
            break;
        case Error:
            SetConsoleTextAttribute(Term, FOREGROUND_INTENSITY | FOREGROUND_RED);
            fprintf(stderr, "ERROR");
            break;
        case FatalError:
            SetConsoleTextAttribute(Term, FOREGROUND_INTENSITY | FOREGROUND_RED);
            fprintf(stderr, "FATAL ERROR");
            break;
        default:
            break;
        };
        SetConsoleTextAttribute(Term, FOREGROUND_INTENSITY);
        fprintf(stderr, " %s", modName);
        SetConsoleTextAttribute(Term, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
        if (sourceInfo)
            fprintf(stderr, " {%s}", sourceInfo);
        SetConsoleTextAttribute(Term, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE);
        if (thrName)
            fprintf(stderr, " (%s)", thrName);
        SetConsoleTextAttribute(Term, FOREGROUND_INTENSITY);
        fprintf(stderr, "] ");
        SetConsoleTextAttribute(Term, 0);
#else
        if (XtermColor)
        {
            fprintf(stderr, BOLD "[");
            fprintf(stderr, GREEN "%5.4f ", tmd);
            if (FrameIndex)
                fprintf(stderr, "(%lu) ", FrameIndex);
            switch (severity)
            {
            case Info:
                fprintf(stderr, BOLD CYAN "INFO");
                break;
            case Warning:
                fprintf(stderr, BOLD YELLOW "WARNING");
                break;
            case Error:
                fprintf(stderr, RED BOLD "ERROR");
                break;
            case FatalError:
                fprintf(stderr, BOLD RED "FATAL ERROR");
                break;
            default:
                break;
            };
            fprintf(stderr, NORMAL BOLD " %s", modName);
            if (sourceInfo)
                fprintf(stderr, BOLD YELLOW " {%s}", sourceInfo);
            if (thrName)
                fprintf(stderr, BOLD MAGENTA " (%s)", thrName);
            fprintf(stderr, NORMAL BOLD "] " NORMAL);
        }
        else
        {
            fprintf(stderr, "[");
            fprintf(stderr, "%5.4f ", tmd);
            if (FrameIndex)
                fprintf(stderr, "(%lu) ", FrameIndex);
            switch (severity)
            {
            case Info:
                fprintf(stderr, "INFO");
                break;
            case Warning:
                fprintf(stderr, "WARNING");
                break;
            case Error:
                fprintf(stderr, "ERROR");
                break;
            case FatalError:
                fprintf(stderr, "FATAL ERROR");
                break;
            default:
                break;
            };
            fprintf(stderr, " %s", modName);
            if (sourceInfo)
                fprintf(stderr, " {%s}", sourceInfo);
            if (thrName)
                fprintf(stderr, " (%s)", thrName);
            fprintf(stderr, "] ");
        }
#endif
    }

    void report(const char* modName, Level severity,
                const char* format, va_list ap)
    {
        std::unique_lock<std::mutex> lk(m);
        _reportHead(modName, nullptr, severity);
        vfprintf(stderr, format, ap);
        fprintf(stderr, "\n");
    }

    void report(const char* modName, Level severity,
                const wchar_t* format, va_list ap)
    {
        std::unique_lock<std::mutex> lk(m);
        _reportHead(modName, nullptr, severity);
        vfwprintf(stderr, format, ap);
        fprintf(stderr, "\n");
    }

    void reportSource(const char* modName, Level severity,
                      const char* file, unsigned linenum,
                      const char* format, va_list ap)
    {
        std::unique_lock<std::mutex> lk(m);
        char sourceInfo[128];
        snprintf(sourceInfo, 128, "%s:%u", file, linenum);
        _reportHead(modName, sourceInfo, severity);
        vfprintf(stderr, format, ap);
        fprintf(stderr, "\n");
    }

    void reportSource(const char* modName, Level severity,
                      const char* file, unsigned linenum,
                      const wchar_t* format, va_list ap)
    {
        std::unique_lock<std::mutex> lk(m);
        char sourceInfo[128];
        snprintf(sourceInfo, 128, "%s:%u", file, linenum);
        _reportHead(modName, sourceInfo, severity);
        vfwprintf(stderr, format, ap);
        fprintf(stderr, "\n");
    }
};

void RegisterConsoleLogger()
{
    /* Determine if console logger already added */
    for (auto& logger : MainLoggers)
    {
        if (typeid(logger.get()) == typeid(ConsoleLogger))
            return;
    }

    /* Otherwise construct new console logger */
    MainLoggers.emplace_back(new ConsoleLogger);
}

struct FileLogger : public ILogger
{
    FILE* fp;
    std::mutex m;
    virtual void openFile()=0;
    virtual void closeFile() {fclose(fp);}

    void _reportHead(const char* modName, const char* sourceInfo, Level severity)
    {
        std::chrono::steady_clock::duration tm = CurrentUptime();
        double tmd = tm.count() *
            std::chrono::steady_clock::duration::period::num /
            (double)std::chrono::steady_clock::duration::period::den;
        std::thread::id thrId = std::this_thread::get_id();
        const char* thrName = nullptr;
        if (ThreadMap.find(thrId) != ThreadMap.end())
            thrName = ThreadMap[thrId];

        fprintf(fp, "[");
        switch (severity)
        {
        case Info:
            fprintf(fp, "INFO");
            break;
        case Warning:
            fprintf(fp, "WARNING");
            break;
        case Error:
            fprintf(fp, "ERROR");
            break;
        case FatalError:
            fprintf(fp, "FATAL ERROR");
            break;
        default:
            break;
        };
        fprintf(fp, " %s", modName);
        if (sourceInfo)
            fprintf(stderr, " {%s}", sourceInfo);
        if (thrName)
            fprintf(fp, " (%s)", thrName);
        fprintf(fp, " %5.4f", tmd);
        if (FrameIndex)
            fprintf(fp, " (%lu)", FrameIndex);
        fprintf(fp, "] ");
    }

    void report(const char* modName, Level severity,
                const char* format, va_list ap)
    {
        std::unique_lock<std::mutex> lk(m);
        openFile();
        _reportHead(modName, nullptr, severity);
        vfprintf(fp, format, ap);
        fprintf(fp, "\n");
        closeFile();
    }

    void report(const char* modName, Level severity,
                const wchar_t* format, va_list ap)
    {
        std::unique_lock<std::mutex> lk(m);
        openFile();
        _reportHead(modName, nullptr, severity);
        vfwprintf(fp, format, ap);
        fprintf(fp, "\n");
        closeFile();
    }

    void reportSource(const char* modName, Level severity,
                      const char* file, unsigned linenum,
                      const char* format, va_list ap)
    {
        std::unique_lock<std::mutex> lk(m);
        openFile();
        char sourceInfo[128];
        snprintf(sourceInfo, 128, "%s:%u", file, linenum);
        _reportHead(modName, sourceInfo, severity);
        vfprintf(fp, format, ap);
        fprintf(fp, "\n");
        closeFile();
    }

    void reportSource(const char* modName, Level severity,
                      const char* file, unsigned linenum,
                      const wchar_t* format, va_list ap)
    {
        std::unique_lock<std::mutex> lk(m);
        openFile();
        char sourceInfo[128];
        snprintf(sourceInfo, 128, "%s:%u", file, linenum);
        _reportHead(modName, sourceInfo, severity);
        vfwprintf(fp, format, ap);
        fprintf(fp, "\n");
        closeFile();
    }
};

struct FileLogger8 : public FileLogger
{
    const char* m_filepath;
    FileLogger8(const char* filepath) : m_filepath(filepath) {}
    void openFile() {fp = fopen(m_filepath, "a");}
};

void RegisterFileLogger(const char* filepath)
{
    /* Determine if file logger already added */
    for (auto& logger : MainLoggers)
    {
        FileLogger8* filelogger = dynamic_cast<FileLogger8*>(logger.get());
        if (filelogger)
        {
            if (!strcmp(filepath, filelogger->m_filepath))
                return;
        }
    }

    /* Otherwise construct new file logger */
    MainLoggers.emplace_back(new FileLogger8(filepath));
}

#if LOG_UCS2

struct FileLogger16 : public FileLogger
{
    const wchar_t* m_filepath;
    FileLogger16(const wchar_t* filepath) : m_filepath(filepath) {}
    void openFile() {fp = _wfopen(m_filepath, L"a");}
};

void RegisterFileLogger(const wchar_t* filepath)
{
    /* Determine if file logger already added */
    for (auto& logger : MainLoggers)
    {
        FileLogger16* filelogger = dynamic_cast<FileLogger16*>(logger.get());
        if (filelogger)
        {
            if (!wcscmp(filepath, filelogger->m_filepath))
                return;
        }
    }

    /* Otherwise construct new file logger */
    MainLoggers.emplace_back(new FileLogger16(filepath));
}

#endif

}
