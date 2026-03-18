// test_common.h :  needed for tests

#pragma once

#define _WIN32_WINNT 0x0A00	// target windows 10 and later

#include <filesystem>
#include <afxwin.h>  // BOOL, CString, MFC base classes
#include "Str8.h"
#include "shwdefs.h"
#include "doctest.h"  // lightweight modern testing framework

namespace fs = std::filesystem;

// test_common.h
#include <fstream>
#include <filesystem>
#include <string>

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