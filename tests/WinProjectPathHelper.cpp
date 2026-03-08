#include "WinProjectPathHelper.h"
#include <algorithm>

static std::string JoinPath(const std::string& base, const std::string& name)
{
#ifdef _WIN32
    if (base.empty()) return name;
    char last = base.back();
    if (last == '\\' || last == '/')
        return base + name;
    return base + "\\" + name;
#else
    if (base.empty()) return name;
    char last = base.back();
    if (last == '/')
        return base + name;
    return base + "/" + name;
#endif
}

std::string DetermineWinProjectPath_Helper(
    const std::string& appPath,
    const std::string& cmdLine,
    const std::string& lastProjectFromIni,
    std::function<bool(const std::string&)> fileExists,
    std::function<std::string()> getAppDataFolder)
{
    // 1) Look for standard .prj in folder with program (portable).
    for (const char* name : { "Toolbox.prj", "Toolbox Project.prj" })
    {
        std::string candidate = JoinPath(appPath, name);
        if (fileExists(candidate))
            return candidate; // portable override
    }

    // 2) Try to set INI path to APPDATA (not used directly here, but emulate detection)
    std::string appData = getAppDataFolder();
    if (!appData.empty())
    {
        // In the real app this sets s_sIniPath; we don't need to persist here.
        // Emulate success by ignoring errors.
    }

    // 3) Command-line argument takes precedence
    std::string projectName = cmdLine;
    if (projectName.empty())
        projectName = lastProjectFromIni;

    if (projectName.empty())
        return std::string(); // nothing to do

    // If cached value exists but file doesn't exist, clear it (return empty)
    if (!fileExists(projectName))
        return std::string();

    return projectName;
}
