#include "test_common.h"
#define private public
#include "shw.h"
#include "mainfrm.h"
#include "project.h"
#undef private

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
            pApp->m_pProject = nullptr;
            bool ok = CProject::s_bConstruct(
                mockFile.c_str(),
                TRUE,
                "1.2.3",
                &(pApp->m_pProject));
            CHECK_MESSAGE(ok, "Project construction returned FALSE");
            if (pApp->m_pProject != nullptr) {
                CProject* pprj = pApp->m_pProject;
                CHECK(pprj->m_bAutoWrap == tc.expected);
                CHECK(std::string(pprj->m_sProjectPath) == mockFile.c_str());
                CHECK(TestFile::normalize_path((const char*)pprj->m_sSettingsDirPath) == TestFile::normalize_path(mockFile.dir_str()));
                CHECK(std::string(pprj->m_sProjectName) == mockFile.filename_str());
                delete pApp->m_pProject;
                pApp->m_pProject = nullptr;
            }
            else {
                CHECK_MESSAGE(false, "pProject is NULL");
            }
        }
    }
    pApp->m_pMainWnd = pOldWnd;
}