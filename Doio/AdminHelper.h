#pragma once
#pragma once
#include <windows.h>
#include <shellapi.h>
#include <string>

inline bool IsRunningAsAdmin() {
    BOOL isElevated = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD dwSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, dwSize, &dwSize)) {
            isElevated = elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }
    return isElevated != FALSE;
}

inline bool RunAsAdmin(const std::string& exePath, const std::string& args) {
    SHELLEXECUTEINFOA sei = { sizeof(sei) };
    sei.lpVerb = "runas";
    sei.lpFile = exePath.c_str();
    sei.lpParameters = args.c_str();
    sei.nShow = SW_HIDE;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;

    if (ShellExecuteExA(&sei)) {
        if (sei.hProcess) {
            WaitForSingleObject(sei.hProcess, INFINITE);
            DWORD exitCode = 0;
            GetExitCodeProcess(sei.hProcess, &exitCode);
            CloseHandle(sei.hProcess);
            return exitCode == 0;
        }
    }
    return false;
}

inline bool RestartAsAdmin(int argc, char* argv[]) {
    if (!IsRunningAsAdmin()) {
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);

        std::string cmdLine;
        for (int i = 1; i < argc; i++) {
            if (i > 1) cmdLine += " ";
            cmdLine += "\"";
            cmdLine += argv[i];
            cmdLine += "\"";
        }

        SHELLEXECUTEINFOA sei = { sizeof(sei) };
        sei.lpVerb = "runas";
        sei.lpFile = exePath;
        sei.lpParameters = cmdLine.c_str();
        sei.nShow = SW_SHOWNORMAL;

        if (ShellExecuteExA(&sei)) {
            return true; // Admin version launched
        }
    }
    return false; // Already admin or restart failed
}