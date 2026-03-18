#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include <crtdbg.h>
#include <windows.h>

int main(int argc, char** argv) {
    // 1. SILENCE THE POPUPS: Redirect CRT/MFC asserts to stderr
    // This prevents the "Abort, Retry, Ignore" dialogs that hang CI
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);

    // Direct standard Windows error boxes to stderr too
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_error_mode(_OUT_TO_STDERR);

    // 2. RUN THE TESTS
    doctest::Context context;
    context.applyCommandLine(argc, argv);

    int res = context.run(); // Run all registered tests

    // 3. EXIT CODE: Important for GitHub Actions to know if it failed
    if (context.shouldExit()) {
        return res;
    }

    return res;
}