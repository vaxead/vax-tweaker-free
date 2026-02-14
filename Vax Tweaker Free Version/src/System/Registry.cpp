
#include "Registry.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <iterator>

namespace Vax::System {

    std::vector<RegistryBackupEntry> Registry::s_backups;

    static constexpr REGSAM kNativeView = KEY_WOW64_64KEY;

    static void LogRegError(const std::string& operation, const std::string& path, LONG result) {
        if (result == ERROR_FILE_NOT_FOUND || result == ERROR_PATH_NOT_FOUND) {
            return;
        }
        if (result == ERROR_ACCESS_DENIED) {
            Logger::Warning("Registry " + operation + ": access denied for " + path);
        } else {
            Logger::Error("Registry " + operation + " failed: " + path
                          + " (error: " + std::to_string(result) + ")");
        }
    }

    std::optional<DWORD> Registry::ReadDword(HKEY root, const std::string& subKey,
                                              const std::string& valueName) {
        HKEY hKey;
        LONG result = RegOpenKeyExA(root, subKey.c_str(), 0, KEY_READ | kNativeView, &hKey);
        if (result != ERROR_SUCCESS) {
            LogRegError("read", subKey + "\\" + valueName, result);
            return std::nullopt;
        }

        DWORD value = 0;
        DWORD size = sizeof(DWORD);
        DWORD type = 0;
        result = RegQueryValueExA(hKey, valueName.c_str(), nullptr, &type,
                                       reinterpret_cast<LPBYTE>(&value), &size);
        RegCloseKey(hKey);

        if (result != ERROR_SUCCESS || type != REG_DWORD) {
            if (result != ERROR_SUCCESS) {
                LogRegError("query", subKey + "\\" + valueName, result);
            }
            return std::nullopt;
        }

        return value;
    }

    std::optional<std::string> Registry::ReadString(HKEY root, const std::string& subKey,
                                                     const std::string& valueName) {
        HKEY hKey;
        LONG result = RegOpenKeyExA(root, subKey.c_str(), 0, KEY_READ | kNativeView, &hKey);
        if (result != ERROR_SUCCESS) {
            LogRegError("read", subKey + "\\" + valueName, result);
            return std::nullopt;
        }

        DWORD size = 0;
        DWORD type = 0;
        result = RegQueryValueExA(hKey, valueName.c_str(), nullptr, &type, nullptr, &size);
        if (result != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ) || size == 0) {
            RegCloseKey(hKey);
            if (result != ERROR_SUCCESS) {
                LogRegError("query", subKey + "\\" + valueName, result);
            }
            return std::nullopt;
        }

        std::vector<char> buffer(size);
        result = RegQueryValueExA(hKey, valueName.c_str(), nullptr, nullptr,
                                  reinterpret_cast<LPBYTE>(buffer.data()), &size);
        RegCloseKey(hKey);

        if (result != ERROR_SUCCESS) {
            LogRegError("query", subKey + "\\" + valueName, result);
            return std::nullopt;
        }

        size_t len = size;
        if (len > 0 && buffer[len - 1] == '\0') --len;
        return std::string(buffer.data(), len);
    }

    bool Registry::KeyExists(HKEY root, const std::string& subKey) {
        HKEY hKey;
        LONG result = RegOpenKeyExA(root, subKey.c_str(), 0, KEY_READ | kNativeView, &hKey);
        if (result == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true;
        }
        return false;
    }

    bool Registry::ValueExists(HKEY root, const std::string& subKey,
                                const std::string& valueName) {
        HKEY hKey;
        if (RegOpenKeyExA(root, subKey.c_str(), 0, KEY_READ | kNativeView, &hKey) != ERROR_SUCCESS) {
            return false;
        }

        LONG result = RegQueryValueExA(hKey, valueName.c_str(), nullptr, nullptr, nullptr, nullptr);
        RegCloseKey(hKey);
        return (result == ERROR_SUCCESS);
    }

    bool Registry::WriteDword(HKEY root, const std::string& subKey,
                               const std::string& valueName, DWORD value) {
        BackupCurrentValue(root, subKey, valueName);

        HKEY hKey;
        DWORD disposition;
        LONG result = RegCreateKeyExA(root, subKey.c_str(), 0, nullptr,
                                       REG_OPTION_NON_VOLATILE, KEY_WRITE | kNativeView, nullptr,
                                       &hKey, &disposition);
        if (result != ERROR_SUCCESS) {
            Logger::Error("Registry write failed: " + subKey + "\\" + valueName
                          + " (error: " + std::to_string(result) + ")");
            return false;
        }

        result = RegSetValueExA(hKey, valueName.c_str(), 0, REG_DWORD,
                                reinterpret_cast<const BYTE*>(&value), sizeof(DWORD));
        RegCloseKey(hKey);

        if (result != ERROR_SUCCESS) {
            Logger::Error("Registry set value failed: " + subKey + "\\" + valueName
                          + " (error: " + std::to_string(result) + ")");
        }
        return (result == ERROR_SUCCESS);
    }

    bool Registry::WriteString(HKEY root, const std::string& subKey,
                                const std::string& valueName, const std::string& value) {
        BackupCurrentValue(root, subKey, valueName);

        HKEY hKey;
        DWORD disposition;
        LONG result = RegCreateKeyExA(root, subKey.c_str(), 0, nullptr,
                                       REG_OPTION_NON_VOLATILE, KEY_WRITE | kNativeView, nullptr,
                                       &hKey, &disposition);
        if (result != ERROR_SUCCESS) {
            Logger::Error("Registry write failed: " + subKey + "\\" + valueName
                          + " (error: " + std::to_string(result) + ")");
            return false;
        }

        result = RegSetValueExA(hKey, valueName.c_str(), 0, REG_SZ,
                                reinterpret_cast<const BYTE*>(value.c_str()),
                                static_cast<DWORD>(value.length() + 1));
        RegCloseKey(hKey);

        if (result != ERROR_SUCCESS) {
            Logger::Error("Registry set value failed: " + subKey + "\\" + valueName
                          + " (error: " + std::to_string(result) + ")");
        }
        return (result == ERROR_SUCCESS);
    }

    bool Registry::DeleteValue(HKEY root, const std::string& subKey,
                                const std::string& valueName) {
        BackupCurrentValue(root, subKey, valueName);

        HKEY hKey;
        LONG result = RegOpenKeyExA(root, subKey.c_str(), 0, KEY_SET_VALUE | kNativeView, &hKey);
        if (result != ERROR_SUCCESS) {
            Logger::Error("Registry delete failed: " + subKey + "\\" + valueName
                          + " (error: " + std::to_string(result) + ")");
            return false;
        }

        result = RegDeleteValueA(hKey, valueName.c_str());
        RegCloseKey(hKey);

        if (result != ERROR_SUCCESS) {
            Logger::Error("Registry delete value failed: " + subKey + "\\" + valueName
                          + " (error: " + std::to_string(result) + ")");
        }
        return (result == ERROR_SUCCESS);
    }

    std::optional<std::vector<BYTE>> Registry::ReadBinary(HKEY root, const std::string& subKey,
                                                           const std::string& valueName) {
        HKEY hKey;
        LONG result = RegOpenKeyExA(root, subKey.c_str(), 0, KEY_READ | kNativeView, &hKey);
        if (result != ERROR_SUCCESS) {
            LogRegError("read", subKey + "\\" + valueName, result);
            return std::nullopt;
        }

        DWORD size = 0;
        DWORD type = 0;
        result = RegQueryValueExA(hKey, valueName.c_str(), nullptr, &type, nullptr, &size);
        if (result != ERROR_SUCCESS || type != REG_BINARY || size == 0) {
            RegCloseKey(hKey);
            if (result != ERROR_SUCCESS) {
                LogRegError("query", subKey + "\\" + valueName, result);
            }
            return std::nullopt;
        }

        std::vector<BYTE> data(size);
        result = RegQueryValueExA(hKey, valueName.c_str(), nullptr, nullptr, data.data(), &size);
        RegCloseKey(hKey);

        if (result != ERROR_SUCCESS) {
            LogRegError("query", subKey + "\\" + valueName, result);
            return std::nullopt;
        }

        return data;
    }

    bool Registry::WriteBinary(HKEY root, const std::string& subKey,
                                const std::string& valueName,
                                const std::vector<BYTE>& data) {
        BackupCurrentValue(root, subKey, valueName);

        HKEY hKey;
        DWORD disposition;
        LONG result = RegCreateKeyExA(root, subKey.c_str(), 0, nullptr,
                                       REG_OPTION_NON_VOLATILE, KEY_WRITE | kNativeView, nullptr,
                                       &hKey, &disposition);
        if (result != ERROR_SUCCESS) {
            Logger::Error("Registry write failed: " + subKey + "\\" + valueName
                          + " (error: " + std::to_string(result) + ")");
            return false;
        }

        result = RegSetValueExA(hKey, valueName.c_str(), 0, REG_BINARY,
                                data.data(), static_cast<DWORD>(data.size()));
        RegCloseKey(hKey);

        if (result != ERROR_SUCCESS) {
            Logger::Error("Registry set value failed: " + subKey + "\\" + valueName
                          + " (error: " + std::to_string(result) + ")");
        }
        return (result == ERROR_SUCCESS);
    }

    void Registry::BackupCurrentValue(HKEY root, const std::string& subKey,
                                      const std::string& valueName) {
        for (const auto& existing : s_backups) {
            if (existing.rootKey == root &&
                existing.subKey == subKey &&
                existing.valueName == valueName) {
                return;
            }
        }

        RegistryBackupEntry entry;
        entry.rootKey = root;
        entry.subKey = subKey;
        entry.valueName = valueName;
        entry.existed = false;
        entry.type = REG_NONE;

        HKEY hKey;
        if (RegOpenKeyExA(root, subKey.c_str(), 0, KEY_READ | kNativeView, &hKey) == ERROR_SUCCESS) {
            DWORD size = 0;
            DWORD type = 0;
            LONG result = RegQueryValueExA(hKey, valueName.c_str(), nullptr, &type, nullptr, &size);

            if (result == ERROR_SUCCESS && size > 0) {
                entry.existed = true;
                entry.type = type;
                entry.data.resize(size);
                RegQueryValueExA(hKey, valueName.c_str(), nullptr, nullptr, entry.data.data(), &size);
            }
            RegCloseKey(hKey);
        }

        s_backups.push_back(entry);

        PersistToDisk();
    }

    const std::vector<RegistryBackupEntry>& Registry::GetBackupEntries() {
        return s_backups;
    }

    bool Registry::DeleteValueInternal(HKEY root, const std::string& subKey,
                                        const std::string& valueName) {
        HKEY hKey;
        LONG result = RegOpenKeyExA(root, subKey.c_str(), 0, KEY_SET_VALUE | kNativeView, &hKey);
        if (result != ERROR_SUCCESS) return false;
        result = RegDeleteValueA(hKey, valueName.c_str());
        RegCloseKey(hKey);
        return (result == ERROR_SUCCESS);
    }

    bool Registry::DeleteValueNoBackup(HKEY root, const std::string& subKey,
                                       const std::string& valueName) {
        return DeleteValueInternal(root, subKey, valueName);
    }

    bool Registry::WriteDwordNoBackup(HKEY root, const std::string& subKey,
                                       const std::string& valueName, DWORD value) {
        HKEY hKey;
        DWORD disposition;
        LONG result = RegCreateKeyExA(root, subKey.c_str(), 0, nullptr,
                                       REG_OPTION_NON_VOLATILE, KEY_WRITE | kNativeView, nullptr,
                                       &hKey, &disposition);
        if (result != ERROR_SUCCESS) {
            Logger::Error("Registry write failed: " + subKey + "\\" + valueName
                          + " (error: " + std::to_string(result) + ")");
            return false;
        }

        result = RegSetValueExA(hKey, valueName.c_str(), 0, REG_DWORD,
                                reinterpret_cast<const BYTE*>(&value), sizeof(DWORD));
        RegCloseKey(hKey);

        if (result != ERROR_SUCCESS) {
            Logger::Error("Registry set value failed: " + subKey + "\\" + valueName
                          + " (error: " + std::to_string(result) + ")");
        }
        return (result == ERROR_SUCCESS);
    }

    bool Registry::WriteStringNoBackup(HKEY root, const std::string& subKey,
                                         const std::string& valueName, const std::string& value) {
        HKEY hKey;
        DWORD disposition;
        LONG result = RegCreateKeyExA(root, subKey.c_str(), 0, nullptr,
                                       REG_OPTION_NON_VOLATILE, KEY_WRITE | kNativeView, nullptr,
                                       &hKey, &disposition);
        if (result != ERROR_SUCCESS) {
            Logger::Error("Registry write failed: " + subKey + "\\" + valueName
                          + " (error: " + std::to_string(result) + ")");
            return false;
        }

        result = RegSetValueExA(hKey, valueName.c_str(), 0, REG_SZ,
                                reinterpret_cast<const BYTE*>(value.c_str()),
                                static_cast<DWORD>(value.length() + 1));
        RegCloseKey(hKey);

        if (result != ERROR_SUCCESS) {
            Logger::Error("Registry set value failed: " + subKey + "\\" + valueName
                          + " (error: " + std::to_string(result) + ")");
        }
        return (result == ERROR_SUCCESS);
    }

    bool Registry::WriteBinaryNoBackup(HKEY root, const std::string& subKey,
                                        const std::string& valueName,
                                        const std::vector<BYTE>& data) {
        HKEY hKey;
        DWORD disposition;
        LONG result = RegCreateKeyExA(root, subKey.c_str(), 0, nullptr,
                                       REG_OPTION_NON_VOLATILE, KEY_WRITE | kNativeView, nullptr,
                                       &hKey, &disposition);
        if (result != ERROR_SUCCESS) {
            Logger::Error("Registry write failed: " + subKey + "\\" + valueName
                          + " (error: " + std::to_string(result) + ")");
            return false;
        }

        result = RegSetValueExA(hKey, valueName.c_str(), 0, REG_BINARY,
                                data.data(), static_cast<DWORD>(data.size()));
        RegCloseKey(hKey);

        if (result != ERROR_SUCCESS) {
            Logger::Error("Registry set value failed: " + subKey + "\\" + valueName
                          + " (error: " + std::to_string(result) + ")");
        }
        return (result == ERROR_SUCCESS);
    }

    bool Registry::RestoreEntry(const RegistryBackupEntry& entry) {
        if (!entry.existed) {
            return DeleteValueInternal(entry.rootKey, entry.subKey, entry.valueName);
        }

        HKEY hKey;
        LONG result = RegCreateKeyExA(entry.rootKey, entry.subKey.c_str(), 0, nullptr,
                                       REG_OPTION_NON_VOLATILE, KEY_WRITE | kNativeView, nullptr,
                                       &hKey, nullptr);
        if (result != ERROR_SUCCESS) {
            return false;
        }

        result = RegSetValueExA(hKey, entry.valueName.c_str(), 0, entry.type,
                                entry.data.data(), static_cast<DWORD>(entry.data.size()));
        RegCloseKey(hKey);

        return (result == ERROR_SUCCESS);
    }

    bool Registry::RestoreAll() {
        bool allSuccess = true;
        std::vector<RegistryBackupEntry> failed;

        for (auto it = s_backups.rbegin(); it != s_backups.rend(); ++it) {
            if (!RestoreEntry(*it)) {
                allSuccess = false;
                failed.push_back(*it);
                Logger::Error("RestoreAll: failed to restore " + it->subKey + "\\" + it->valueName);
            }
        }

        if (allSuccess) {
            s_backups.clear();
            std::string path = GetBackupFilePath();
            DeleteFileA(path.c_str());
        } else {
            s_backups = std::move(failed);
            PersistToDisk();
        }
        return allSuccess;
    }

    void Registry::ClearBackups() {
        s_backups.clear();
        std::string path = GetBackupFilePath();
        DeleteFileA(path.c_str());
    }

    void Registry::ReplaceBackups(std::vector<RegistryBackupEntry> entries) {
        s_backups = std::move(entries);
        if (s_backups.empty()) {
            std::string path = GetBackupFilePath();
            DeleteFileA(path.c_str());
        } else {
            PersistToDisk();
        }
    }

    std::string Registry::GetAppDataDir() {
        char appData[MAX_PATH] = {};
        if (GetEnvironmentVariableA("APPDATA", appData, MAX_PATH) == 0) {
            GetModuleFileNameA(nullptr, appData, MAX_PATH);
            std::string fallback(appData);
            size_t pos = fallback.find_last_of("\\/");
            if (pos != std::string::npos) fallback = fallback.substr(0, pos);
            return fallback;
        }
        std::string path(appData);
        path += "\\VaxTweaker";
        CreateDirectoryA(path.c_str(), nullptr);
        return path;
    }

    std::string Registry::GetBackupFilePath() {
        return GetAppDataDir() + "\\vax_backup.dat";
    }

    static std::string RootKeyToString(HKEY root) {
        if (root == HKEY_LOCAL_MACHINE)  return "HKLM";
        if (root == HKEY_CURRENT_USER)   return "HKCU";
        if (root == HKEY_CLASSES_ROOT)   return "HKCR";
        if (root == HKEY_USERS)          return "HKU";
        return "UNKNOWN";
    }

    static HKEY StringToRootKey(const std::string& str) {
        if (str == "HKLM")  return HKEY_LOCAL_MACHINE;
        if (str == "HKCU")  return HKEY_CURRENT_USER;
        if (str == "HKCR")  return HKEY_CLASSES_ROOT;
        if (str == "HKU")   return HKEY_USERS;
        return nullptr;
    }

    static std::string BytesToHex(const std::vector<BYTE>& data) {
        std::ostringstream oss;
        for (BYTE b : data) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
        }
        return oss.str();
    }

    static std::vector<BYTE> HexToBytes(const std::string& hex) {
        std::vector<BYTE> data;
        for (size_t i = 0; i + 1 < hex.length(); i += 2) {
            unsigned int byte = 0;
            std::istringstream iss(hex.substr(i, 2));
            iss >> std::hex >> byte;
            data.push_back(static_cast<BYTE>(byte));
        }
        return data;
    }

    static uint32_t ComputeCrc32(const std::string& data) {
        uint32_t crc = 0xFFFFFFFF;
        for (unsigned char c : data) {
            crc ^= c;
            for (int i = 0; i < 8; ++i) {
                crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320u : 0u);
            }
        }
        return crc ^ 0xFFFFFFFF;
    }

    bool Registry::PersistToDisk() {
        std::string path = GetBackupFilePath();
        std::string tmpPath = path + ".tmp";

        std::ostringstream content;
        content << "VAX_BACKUP_V2\n";
        content << s_backups.size() << "\n";

        for (const auto& entry : s_backups) {
            content << RootKeyToString(entry.rootKey) << "\n";
            content << entry.subKey << "\n";
            content << entry.valueName << "\n";
            content << entry.type << "\n";
            content << BytesToHex(entry.data) << "\n";
            content << (entry.existed ? "1" : "0") << "\n";
        }

        std::string body = content.str();
        uint32_t crc = ComputeCrc32(body);

        std::ofstream file(tmpPath);
        if (!file.is_open()) {
            Logger::Error("Failed to persist backup to: " + tmpPath);
            return false;
        }

        file << body;
        file << "CRC32:" << std::hex << std::setw(8) << std::setfill('0') << crc << "\n";

        file.close();

        if (!MoveFileExA(tmpPath.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING)) {
            Logger::Error("Failed to finalize backup file");
            DeleteFileA(tmpPath.c_str());
            return false;
        }
        return true;
    }

    bool Registry::LoadFromDisk() {
        std::string path = GetBackupFilePath();
        std::ifstream file(path);
        if (!file.is_open()) {
            return true;
        }

        std::string fullContent((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
        file.close();

        if (fullContent.empty()) return true;

        std::istringstream stream(fullContent);

        std::string header;
        if (!std::getline(stream, header)) {
            Logger::Warning("Invalid backup file format, ignoring");
            return false;
        }

        bool hasChecksum = (header == "VAX_BACKUP_V2");
        if (header != "VAX_BACKUP_V1" && header != "VAX_BACKUP_V2") {
            Logger::Warning("Invalid backup file format, ignoring");
            return false;
        }

        if (hasChecksum) {
            size_t crcPos = fullContent.rfind("CRC32:");
            if (crcPos == std::string::npos) {
                Logger::Error("Backup file missing integrity checksum — file may be tampered");
                return false;
            }
            std::string body = fullContent.substr(0, crcPos);
            std::string crcLine = fullContent.substr(crcPos + 6);
            while (!crcLine.empty() && (crcLine.back() == '\n' || crcLine.back() == '\r'))
                crcLine.pop_back();
            uint32_t expectedCrc = 0;
            try {
                expectedCrc = static_cast<uint32_t>(std::stoul(crcLine, nullptr, 16));
            } catch (...) {
                Logger::Error("Backup file has invalid checksum format — file may be tampered");
                return false;
            }
            uint32_t actualCrc = ComputeCrc32(body);
            if (actualCrc != expectedCrc) {
                Logger::Error("Backup file integrity check failed — file may be tampered, ignoring");
                return false;
            }
        }

        std::string countStr;
        if (!std::getline(stream, countStr)) {
            return false;
        }

        try {
            int count = std::stoi(countStr);

            for (int i = 0; i < count; ++i) {
                RegistryBackupEntry entry;
                std::string rootStr, typeStr, hexData, existedStr;

                if (!std::getline(stream, rootStr)) break;
                if (!std::getline(stream, entry.subKey)) break;
                if (!std::getline(stream, entry.valueName)) break;
                if (!std::getline(stream, typeStr)) break;
                if (!std::getline(stream, hexData)) break;
                if (!std::getline(stream, existedStr)) break;

                entry.rootKey = StringToRootKey(rootStr);
                if (entry.rootKey == nullptr) continue;

                entry.type = static_cast<DWORD>(std::stoul(typeStr));
                entry.data = HexToBytes(hexData);
                entry.existed = (existedStr == "1");

                s_backups.push_back(entry);
            }
        } catch (const std::exception& e) {
            Logger::Warning("Corrupted backup file, starting fresh: " + std::string(e.what()));
            s_backups.clear();
            return false;
        }

        Logger::Info("Loaded " + std::to_string(s_backups.size()) + " backup entries from disk");
        return true;
    }

}
