#include "doctest.h"
#include <windows.h>
#include <string>
#include <vector>

// ccdll.h defines the C API exposed by ccengine
#include "ccdll.h"

// Static or lambda-compatible callback for compile errors
static int WINAPI SilentCompileErrorCallback(char* lpszMessage, unsigned iLine, unsigned iCharacter) {
    return 0;
}

static int WINAPI SilentCCErrorCallback(short nMsgIndex, unsigned short wParam, unsigned long lParam, long* lpUserData) {
    return 0;
}

TEST_SUITE("CCEngine") {
    TEST_CASE("Syntax error reporting in table") {
        const char* badScript = "begin > 'unclosed\n";
        HANDLE hTable = NULL;

        // Use the error-callback version to prevent the Windows MessageBox dialog
        int rc = CCLoadTableFromBufferWithErrorCallback(
            badScript,
            &hTable,
            SilentCompileErrorCallback
        );

        CHECK(rc == CC_SYNTAX_ERROR);
        if (hTable != NULL) {
            CCUnloadTable(hTable);
        }
    }

    TEST_CASE("Basic character substitution change table") {
        // Simple CCT table: replace 'a' with 'b', and "foo" with "bar"
        // In CC syntax, 'c' > 'd' or "foo" > "bar"
        const char* cctScript =
            "begin >\n"
            "'a' > 'b'\n"
            "\"foo\" > \"bar\"\n";

        HANDLE hTable = NULL;
        int rc = CCLoadTableFromBuffer(const_cast<char*>(cctScript), &hTable);
        REQUIRE(rc == CC_SUCCESS);
        REQUIRE(hTable != NULL);

        SUBCASE("Process buffer with single and multi-character replacements") {
            char input[] = "cat foo baz";
            int inputLen = static_cast<int>(strlen(input));

            char output[128] = { 0 };
            int outBufLen = sizeof(output) - 1;

            rc = CCProcessBuffer(hTable, input, inputLen, output, &outBufLen);
            CHECK(rc == CC_SUCCESS);
            output[outBufLen] = '\0';

            // 'c' -> 'c', 'a' -> 'b', 't' -> 't', "foo" -> "bar", "baz" -> "bbz"
            CHECK_EQ(std::string(output), "cbt bar bbz");
        }

        SUBCASE("Process buffer requiring table reset/reinitialization") {
            char input[] = "aaaa";
            int inputLen = 4;
            char output[16] = { 0 };
            int outLen = sizeof(output) - 1;

            rc = CCProcessBuffer(hTable, input, inputLen, output, &outLen);
            CHECK(rc == CC_SUCCESS);
            output[outLen] = '\0';
            CHECK_EQ(std::string(output), "bbbb");

            // Verify reinitialize works cleanly
            rc = CCReinitializeTable(hTable);
            CHECK(rc == CC_SUCCESS);
        }

        CHECK(CCUnloadTable(hTable) == CC_SUCCESS);
    }

    TEST_CASE("CCT with Store and QueryStore") {
        // When 'X' is encountered, put "InitialValue" into store(myStore)
        // and replace 'X' with that store's content.
        const char* cctScript =
            "begin >\n"
            "'X' > store(myStore) 'InitialValue' endstore out(myStore)\n";

        HANDLE hTable = NULL;
        int rc = CCLoadTableFromBuffer(const_cast<char*>(cctScript), &hTable);
        REQUIRE(rc == CC_SUCCESS);
        REQUIRE(hTable != NULL);

        // After CCLoadTable...
        CCSetErrorCallBack(hTable, SilentCCErrorCallback);

        // Execute transformation
        char input[] = "X";
        char output[64] = { 0 };
        int outLen = sizeof(output) - 1;

        rc = CCProcessBuffer(hTable, input, 1, output, &outLen);
        CHECK(rc == CC_SUCCESS);
        output[outLen] = '\0';
        CHECK_EQ(std::string(output), "InitialValue");

        // Now query the store that was populated during execution
        char storeVal[64] = { 0 };
        BOOL queried = CCQueryStore(hTable, const_cast<char*>("myStore"), storeVal, sizeof(storeVal));
        CHECK(queried == std::string_view("initialValue").size());
        CHECK_EQ(std::string(storeVal), "InitialValue");

        CCUnloadTable(hTable);
    }
}
