#include <iostream>
#include <windows.h>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

// Helper to set Registry values for file association
bool SetRegistryValue(HKEY hKeyParent, LPCSTR subKey, LPCSTR valueName, LPCSTR data) {
    HKEY hKey;
    if (RegCreateKeyExA(hKeyParent, subKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
        return false;
    }
    if (RegSetValueExA(hKey, valueName, 0, REG_SZ, (const BYTE*)data, strlen(data) + 1) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return false;
    }
    RegCloseKey(hKey);
    return true;
}

int main() {
    std::string installDir = "C:\\Program Files\\Delta";
    std::string exeName = "delta.exe";
    std::string fullExePath = installDir + "\\" + exeName;

    std::cout << "--- Delta Language Installer ---" << std::endl;

    try {
        if (!fs::exists(installDir)) {
            fs::create_directories(installDir);
        }

        if (fs::exists(exeName)) {
            fs::copy_file(exeName, fullExePath, fs::copy_options::overwrite_existing);
            std::cout << "[+] Copied delta.exe to " << installDir << std::endl;
        } else {
            std::cerr << "[-] Error: delta.exe not found in current directory." << std::endl;
            return 1;
        }

        SetRegistryValue(HKEY_CLASSES_ROOT, ".delta", "", "Delta.File");
        std::string command = "\"" + fullExePath + "\" \"%1\"";
        SetRegistryValue(HKEY_CLASSES_ROOT, "Delta.File\\shell\\open\\command", "", command.c_str());
        
        std::cout << "[+] Registered .delta file association." << std::endl;

        HKEY hKey;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            char oldPath[4096];
            DWORD size = sizeof(oldPath);
            if (RegQueryValueExA(hKey, "Path", NULL, NULL, (LPBYTE)oldPath, &size) == ERROR_SUCCESS) {
                std::string newPath = std::string(oldPath);
                if (newPath.find(installDir) == std::string::npos) {
                    newPath += ";" + installDir;
                    RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ, (const BYTE*)newPath.c_str(), newPath.length() + 1);
                    SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
                    std::cout << "[+] Added Delta to PATH." << std::endl;
                } else {
                    std::cout << "[!] Delta already in PATH." << std::endl;
                }
            }
            RegCloseKey(hKey);
        }

        std::cout << "\nInstallation Complete! Restart your terminal to use 'delta'." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[-] Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}