#include "test_common.h"
#include "mainfrm.h"

static const char* psz_ShProjectSettings = "ShProjectSettings";
static const char* psz_AutoWrap = "AutoWrap";
static const char* psz_NoAutoWrap = "NoAutoWrap";

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