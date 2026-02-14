
#pragma once

#include <windows.h>
#include <string>

namespace Vax {

    class Admin {
    public:
        static bool IsUserInAdminGroup();

        static bool IsProcessElevated();

        static bool IsElevated();

        static bool RequestElevation();

        static std::string GetExecutablePath();

    private:
        Admin() = default;
    };

}
