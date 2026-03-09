
#include "doctest.h"
#include <iostream>
#include <stddef.h> // For NULL
#include <string>
#include "mock_stdafx.h" // Mocked stdafx.h for testing
#include "Str8.h"
#include "Mkr.h"

TEST_CASE("CMString::Delete logic")
{
    SUBCASE("Delete from the middle")
    {
        CMString s(nullptr, "Hello World");
        s.Delete(5, 1);
        CHECK(s == "HelloWorld");
    }
    SUBCASE("Delete more characters than exist (truncate to end)")
    {
        CMString s(nullptr, "Hello");
        s.Delete(2, 100);
        CHECK(s == "He");
    }

    SUBCASE("Delete zero characters (no-op)")
    {
        CMString s(nullptr, "Test");
        s.Delete(1, 0);
        CHECK(s == "Test");
    }

    SUBCASE("Delete entire string")
    {
        CMString s(nullptr, "DeleteMe");
        s.Delete(0, 8);
        CHECK(s.GetLength() == 0);
    }

    SUBCASE("Delete first byte in Devanagari UTF-8 string") {
        std::string original = u8"नमस्ते";
        CMString s(nullptr, original.c_str());
        s.Delete(0, 1);
        std::string expected = original.substr(1);
        CHECK(s == expected.c_str());
    }

    SUBCASE("Delete middle bytes in UTF-8 string") {
        std::string original = u8"aनमस्तेe";
        CMString s(nullptr, original.c_str());
        s.Delete(1, original.size() - 2);
        CHECK(s == "ae");
    }
}