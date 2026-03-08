#pragma once
#include <string>
#include <functional>

// A small testable helper that mirrors the decision logic in
// CShwApp::DetermineWinProjectPath in a pure, dependency-injectable way.
//
// Parameters:
// - appPath: directory where the application lives (should end with path separator or not — helper will handle)
// - cmdLine: the raw command-line argument (may be an UTF-8 project path or empty)
// - lastProjectFromIni: the cached project name read from INI (may be empty)
// - fileExists: injected function to test filesystem existence of paths (UTF-8 path)
// - getAppDataFolder: injected function returning user-local appdata path (UTF-8) or empty on failure
//
// Returns:
// - project path to open (UTF-8), or empty string if none determined.
std::string DetermineWinProjectPath_Helper(
    const std::string& appPath,
    const std::string& cmdLine,
    const std::string& lastProjectFromIni,
    std::function<bool(const std::string&)> fileExists,
    std::function<std::string()> getAppDataFolder);
