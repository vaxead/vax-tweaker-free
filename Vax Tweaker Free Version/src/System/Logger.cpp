
#include "Logger.h"
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Vax::System {

    std::mutex Logger::s_mutex;
    std::vector<LogEntry> Logger::s_entries;

    void Logger::Info(const std::string& message) {
        Log(LogLevel::Info, message);
    }

    void Logger::Success(const std::string& message) {
        Log(LogLevel::Success, message);
    }

    void Logger::Warning(const std::string& message) {
        Log(LogLevel::Warning, message);
    }

    void Logger::Error(const std::string& message) {
        Log(LogLevel::Error, message);
    }

    void Logger::Log(LogLevel level, const std::string& message) {
        LogEntry entry;
        entry.level = level;
        entry.message = message;
        entry.timestamp = GetTimestamp();
        std::lock_guard<std::mutex> lk(s_mutex);
        constexpr size_t kMaxEntries = 10000;
        if (s_entries.size() >= kMaxEntries) {
            s_entries.erase(s_entries.begin(), s_entries.begin() + (kMaxEntries / 10));
        }
        s_entries.push_back(std::move(entry));
    }

    std::vector<LogEntry> Logger::GetEntries() {
        std::lock_guard<std::mutex> lk(s_mutex);
        return s_entries;
    }

    std::vector<LogEntry> Logger::GetByLevel(LogLevel level) {
        std::lock_guard<std::mutex> lk(s_mutex);
        std::vector<LogEntry> filtered;
        for (const auto& entry : s_entries) {
            if (entry.level == level) {
                filtered.push_back(entry);
            }
        }
        return filtered;
    }

    bool Logger::ExportToFile(const std::string& filePath) {
        std::lock_guard<std::mutex> lk(s_mutex);
        std::ofstream file(filePath);
        if (!file.is_open()) {
            return false;
        }

        file << "VAX TWEAKER - Operation Log\n";
        file << "===========================\n\n";

        for (const auto& entry : s_entries) {
            file << "[" << entry.timestamp << "] "
                 << "[" << LevelToString(entry.level) << "] "
                 << entry.message << "\n";
        }

        file.close();
        return true;
    }

    void Logger::Clear() {
        std::lock_guard<std::mutex> lk(s_mutex);
        s_entries.clear();
    }

    std::string Logger::GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        
        struct tm timeinfo = {};
        localtime_s(&timeinfo, &time);
        
        std::ostringstream oss;
        oss << std::put_time(&timeinfo, "%H:%M:%S");
        return oss.str();
    }

    std::string Logger::LevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Info:    return "INFO";
            case LogLevel::Success: return "OK";
            case LogLevel::Warning: return "WARN";
            case LogLevel::Error:   return "ERROR";
            default:                return "?";
        }
    }

}
