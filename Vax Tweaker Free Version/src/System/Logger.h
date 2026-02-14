
#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace Vax::System {

    enum class LogLevel {
        Info,
        Success,
        Warning,
        Error
    };

    struct LogEntry {
        LogLevel level;
        std::string message;
        std::string timestamp;
    };

    class Logger {
    public:
        static void Info(const std::string& message);
        static void Success(const std::string& message);
        static void Warning(const std::string& message);
        static void Error(const std::string& message);

        static const std::vector<LogEntry>& GetEntries();

        static std::vector<LogEntry> GetByLevel(LogLevel level);

        static bool ExportToFile(const std::string& filePath);

        static void Clear();

    private:
        static void Log(LogLevel level, const std::string& message);
        static std::string GetTimestamp();
        static std::string LevelToString(LogLevel level);

        static std::vector<LogEntry> s_entries;

        Logger() = default;
    };

}
