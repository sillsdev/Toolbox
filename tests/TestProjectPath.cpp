#include "doctest.h"
#include "WinProjectPathHelper.h"
#include <string>

// Helper lambdas used in tests: simulate filesystem existence and AppData presence.
static auto AlwaysFalse = [](const std::string&) { return false; };
static auto AlwaysTrue = [](const std::string&) { return true; };

TEST_CASE("Portable project found in app folder (Roman filename)")
{
    std::string appPath = R"(C:\Program Files\Toolbox)";
    bool toolboxPrjChecked = false;
    auto fileExists = [&](const std::string& p) {
        if (p.find("Toolbox.prj") != std::string::npos) {
            toolboxPrjChecked = true;
            return true;
        }
        return false;
        };
    std::string result = DetermineWinProjectPath_Helper(appPath, "", "", fileExists, []() { return std::string(); });

    // REQUIRE in doctest stops the test immediately on failure, just like Catch2
    REQUIRE(toolboxPrjChecked);
    CHECK(result.find("Toolbox.prj") != std::string::npos);
}

TEST_CASE("Command-line project with IPA characters preserved")
{
    // IPA string (UTF-8). Note: u8 literals in C++20 return char8_t, 
    // but MFC/Legacy projects often use C++14/17 where they are still char.
    std::string ipaName = u8"project-ʃɔbɛks.prj";
    auto fileExists = [&](const std::string& p) { return p == ipaName; };

    std::string result = DetermineWinProjectPath_Helper(R"(C:\app)", ipaName, "", fileExists, []() { return std::string(); });

    CHECK(result == ipaName);
}

TEST_CASE("Cached Devanagari project missing clears cached value")
{
    // Devanagari: "प्रोजेक्ट.prj"
    std::string devName = u8"C:\\Users\\User\\प्रोजेक्ट.prj";
    auto fileExists = [&](const std::string& p) { return false; };

    std::string result = DetermineWinProjectPath_Helper(R"(C:\app)", "", devName, fileExists, []() { return std::string(); });

    CHECK(result.empty());
}

TEST_CASE("Cached Hebrew project that exists is returned")
{
    // Hebrew: "פרוייקט.prj"
    std::string hebName = u8"C:\\Data\\פרוייקט.prj";
    auto fileExists = [&](const std::string& p) { return p == hebName; };

    std::string result = DetermineWinProjectPath_Helper(R"(C:\app)", "", hebName, fileExists, []() { return std::string(); });

    CHECK(result == hebName);
}

TEST_CASE("When command line empty and no ini cached path, return empty")
{
    std::string result = DetermineWinProjectPath_Helper(R"(C:\app)", "", "", AlwaysFalse, []() { return std::string(); });

    CHECK(result.empty());
}