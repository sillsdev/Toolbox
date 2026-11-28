#include "test_common.h"
#include "mainfrm.h"
#include "not.h"
#include "obstream.h"
#include <memory>
#include <sstream>

static const char* psz_ShProjectSettings = "ShProjectSettings";
static const char* psz_AutoWrap = "AutoWrap";
static const char* psz_NoAutoWrap = "NoAutoWrap";

TEST_CASE("Project paths use the open databases to infer the root")
{
    std::unique_ptr<CProject> project(CProject_Test::Create("C:\\Old\\Moved\\Settings\\Sample.prj"));
    std::string dataFile = "C:\\Old\\Settings\\Local.txt";
    bool parentRoot = false;
    SUBCASE("A sibling data file widens the root")
    {
        dataFile = "C:\\OLD\\Data\\Dictionary.txt";
        parentRoot = true;
    }
    SUBCASE("A settings-local file keeps the root at Settings") {}

    // Put the legacy location after the open-file list to exercise the layout pass.
    std::istringstream settings("\\+ShProjectSettings\n"
        "\\+dblst\n\\+db " + dataFile + "\n\\-db\n\\-dblst\n"
        "\\ProjectPath C:\\Old\\Settings\\Sample.prj\n\\-ShProjectSettings\n");
    CNoteList notes;
    Object_istream stream(settings, notes);
    CProject_Test::ReadLayout(*project, stream);

    CHECK(project->sResolvedPath("C:\\Old\\Settings\\Local.txt", TRUE) ==
        "C:\\Old\\Moved\\Settings\\Local.txt");
    CHECK(project->sResolvedPath("..\\Data\\Dictionary.txt", TRUE) ==
        "C:\\Old\\Moved\\Data\\Dictionary.txt");
    CHECK(project->sResolvedPath("C:\\Older\\Shared.txt", TRUE) == "C:\\Older\\Shared.txt");
    CHECK(project->sResolvedPath("D:\\Shared\\External.txt", TRUE) == "D:\\Shared\\External.txt");
    CHECK(project->sResolvedPath("C:\\Old\\Settings\\External.txt", FALSE) ==
        "C:\\Old\\Settings\\External.txt");
    CHECK(project->sStoredPath("C:\\Old\\Moved\\Settings\\Local.txt") == ".\\Local.txt");
    CHECK(project->sStoredPath("D:\\Shared\\External.txt") == "D:\\Shared\\External.txt");

    if (parentRoot)
        {
        CHECK(project->sResolvedPath(dataFile.c_str(), TRUE) == "C:\\Old\\Moved\\Data\\Dictionary.txt");
        CHECK(project->sStoredPath("C:\\Old\\Moved\\Data\\Dictionary.txt") == "..\\Data\\Dictionary.txt");
        CProject_Test::SetSavingRoot(*project, "C:\\Old\\Moved\\Settings");
        CHECK(project->sStoredPath("C:\\Old\\Moved\\Data\\Dictionary.txt") ==
            "C:\\Old\\Moved\\Data\\Dictionary.txt");
        CProject_Test::SetSavingRoot(*project, "");
        CHECK(project->sStoredPath("C:\\Old\\Moved\\Data\\Dictionary.txt") == "..\\Data\\Dictionary.txt");
        }
    else
        {
        CHECK(project->sResolvedPath("C:\\Old\\Data\\Dictionary.txt", TRUE) ==
            "C:\\Old\\Data\\Dictionary.txt");
        CHECK(project->sStoredPath("C:\\Old\\Moved\\Data\\Dictionary.txt") ==
            "C:\\Old\\Moved\\Data\\Dictionary.txt");
        }
}

TEST_CASE("Create CProject")
{
    CShwApp* pApp = (CShwApp*)Shw_papp();
    CMainFrame dummyMainFrame;
    CWnd* pOldWnd = pApp->m_pMainWnd;
    pApp->m_pMainWnd = &dummyMainFrame;

    SUBCASE("Construction and AutoWrap")
    {
        struct {
            std::string label;
            std::string lineToInclude;
            BOOL expected;
        } testCases[] = {
            {"Missing (Default)", "", TRUE},
            {"Explicitly Disabled", "\\" + std::string(psz_NoAutoWrap), FALSE},
            {"Explicitly Enabled", "\\" + std::string(psz_AutoWrap), TRUE}
        };
        for (auto& tc : testCases) {
            INFO("Testing Case: " << tc.label);
            std::string filename = "adir/test.prj";
            std::string fileContent = std::string("\\+") + psz_ShProjectSettings + "\n"
                + tc.lineToInclude;
            TestFile mockFile(filename, fileContent);
            // When s_bConstruct reaches the line:
            // *ppprj = pprj;
            // It writes the new project's address into the global m_pProject variable.
            CShwApp_Test::ResetProject(pApp);
            bool ok = CProject::s_bConstruct(
                mockFile.c_str(),
                TRUE,
                "1.2.3",
                &CShwApp_Test::Project(pApp));
            CHECK_MESSAGE(ok, "Project construction returned FALSE");
            CProject* pprj = CShwApp_Test::Project(pApp);
            if (pprj != nullptr) {
                CHECK(CProject_Test::AutoWrap(pprj) == tc.expected);
                CHECK(std::string(CProject_Test::ProjectPath(pprj)) == mockFile.c_str());
                CHECK(TestFile::normalize_path((const char*)CProject_Test::SettingsDirPath(pprj))
                    == TestFile::normalize_path(mockFile.dir_str()));
                CHECK(std::string(CProject_Test::ProjectName(pprj)) == mockFile.filename_str());
                CShwApp_Test::ResetProject(pApp);
            }
            else {
                CHECK_MESSAGE(false, "pProject is NULL");
            }
        }
    }
    pApp->m_pMainWnd = pOldWnd;
}
