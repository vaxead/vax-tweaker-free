
#pragma once

#include <windows.h>
#include <string>
#include <optional>
#include <vector>

namespace Vax::System {

    struct RegistryBackupEntry {
        HKEY rootKey;
        std::string subKey;
        std::string valueName;
        DWORD type;
        std::vector<BYTE> data;
        bool existed;
    };

    class Registry {
    public:

        static std::optional<DWORD> ReadDword(HKEY root, const std::string& subKey,
                                              const std::string& valueName);

        static std::optional<std::string> ReadString(HKEY root, const std::string& subKey,
                                                     const std::string& valueName);

        static std::optional<std::vector<BYTE>> ReadBinary(HKEY root, const std::string& subKey,
                                                           const std::string& valueName);

        static bool KeyExists(HKEY root, const std::string& subKey);

        static bool ValueExists(HKEY root, const std::string& subKey,
                                const std::string& valueName);

        static bool WriteDword(HKEY root, const std::string& subKey,
                               const std::string& valueName, DWORD value);

        static bool WriteString(HKEY root, const std::string& subKey,
                                const std::string& valueName, const std::string& value);

        static bool WriteBinary(HKEY root, const std::string& subKey,
                                const std::string& valueName,
                                const std::vector<BYTE>& data);

        static bool DeleteValue(HKEY root, const std::string& subKey,
                                const std::string& valueName);

        static bool DeleteValueNoBackup(HKEY root, const std::string& subKey,
                                        const std::string& valueName);

        static bool WriteDwordNoBackup(HKEY root, const std::string& subKey,
                                       const std::string& valueName, DWORD value);

        static bool WriteStringNoBackup(HKEY root, const std::string& subKey,
                                         const std::string& valueName, const std::string& value);

        static bool WriteBinaryNoBackup(HKEY root, const std::string& subKey,
                                        const std::string& valueName,
                                        const std::vector<BYTE>& data);

        static const std::vector<RegistryBackupEntry>& GetBackupEntries();

        static bool RestoreEntry(const RegistryBackupEntry& entry);

        static bool RestoreAll();

        static void ClearBackups();

        static void ReplaceBackups(std::vector<RegistryBackupEntry> entries);

        static bool PersistToDisk();

        static bool LoadFromDisk();

        static std::string GetAppDataDir();

        static std::string GetBackupFilePath();

    private:
        static void BackupCurrentValue(HKEY root, const std::string& subKey,
                                       const std::string& valueName);

        static bool DeleteValueInternal(HKEY root, const std::string& subKey,
                                        const std::string& valueName);

        static std::vector<RegistryBackupEntry> s_backups;

        Registry() = default;
    };

}
