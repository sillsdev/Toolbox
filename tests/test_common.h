// test_common.h :  needed for tests

#pragma once

#define _WIN32_WINNT 0x0A00	// target windows 10 and later

#include <filesystem>
#include <fstream>
#include <string>
#include <afxwin.h>  // BOOL, CString, MFC base classes
#include "Str8.h"
#include "shwdefs.h"
#include "mkr.h"
#include "project.h"
#include "shw.h"
#include "doctest.h"  // lightweight modern testing framework

namespace fs = std::filesystem;

struct TestFile {
    fs::path fullPath;
    std::string persistedPath;

    inline TestFile(const std::string& relativePath, const std::string& contents) {
        fullPath = (fs::temp_directory_path() / relativePath).make_preferred();
        persistedPath = fullPath.string();

        fs::create_directories(fullPath.parent_path());

        std::ofstream ofs(fullPath, std::ios::binary);
        ofs << contents;
        ofs.close();
    }

    inline const char* c_str() const {
        return persistedPath.c_str();
    }

    static inline std::string normalize_path(std::string s) {
        while (!s.empty() && (s.back() == '\\' || s.back() == '/')) {
            s.pop_back();
        }
        return s;
    }

    inline std::string dir_str() const {
        return fullPath.parent_path().string();
    }

    inline std::string filename_str() const {
        return fullPath.filename().string();
    }
};

class CMarker_Test {
public:
    static void SetInterlinear(CMarker* pmkr, BOOL bIsFirst = FALSE) {
        if (pmkr) pmkr->SetInterlinear(bIsFirst);
    }

    static void SetParent(CMarker* pSub, CMarker* pParent) {
        if (pSub) pSub->SetMarkerOverThis(pParent);
    }

    static void SetMultipleItemData(CMarker* pmkr, BOOL bVal) {
        if (pmkr) pmkr->m_bMultipleItemData = bVal;
    }

    static void SetMustHaveData(CMarker* pmkr, BOOL bVal) {
        if (pmkr) pmkr->m_bMustHaveData = bVal;
    }
};

class CShwApp_Test {
public:
    static CProject*& Project(CShwApp* pApp) {
        return pApp->m_pProject;
    }

    static void ResetProject(CShwApp* pApp) {
        if (pApp && pApp->m_pProject) {
            pApp->m_pProject->SetExerciseNoSave(TRUE);
            CProject* pOld = pApp->m_pProject;
            delete pOld;
            pApp->m_pProject = nullptr;
        }
    }
};

class CProject_Test {
public:
    // Accessors for private members
    static BOOL AutoWrap(const CProject* pprj) {
        return pprj->m_bAutoWrap;
    }

    static const Str8& ProjectPath(const CProject* pprj) {
        return pprj->m_sProjectPath;
    }

    static const Str8& SettingsDirPath(const CProject* pprj) {
        return pprj->m_sSettingsDirPath;
    }

    static const Str8& ProjectName(const CProject* pprj) {
        return pprj->m_sProjectName;
    }
};