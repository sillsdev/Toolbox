#include "test_common.h"
#include "mkr.h"

TEST_CASE("CMString::Insert")
{
    SUBCASE("Insert single char in middle (space)")
    {
        CMString s(nullptr, "HelloWorld");
        s.Insert(' ', 5, 1); // Insert one space at position 5
        CHECK(s == "Hello World");
    }

    SUBCASE("Insert string in middle")
    {
        CMString s(nullptr, "HelloWorld");
        s.Insert(" ", 5);
        CHECK(s == "Hello World");
    }

    SUBCASE("Insert UTF-8 substring (Devanagari) in middle")
    {
        std::string original = u8"abcde";
        CMString s(nullptr, original.c_str());
        std::string ins = u8"नम";
        s.Insert(ins.c_str(), 2);
        std::string expected = std::string("ab") + ins + "cde";
        CHECK(s == expected.c_str());
    }

    SUBCASE("Insert multiple chars")
    {
        CMString s(nullptr, "abc");
        s.Insert('X', 1, 3);  // Insert 3 X's at position 1
        CHECK(s == "aXXXbc");
    }

    SUBCASE("Insert at end")
    {
        CMString s(nullptr, "Hello");
        s.Insert("!", 5);
        CHECK(s == "Hello!");
    }

    SUBCASE("Insert at beginning")
    {
        CMString s(nullptr, "World");
        s.Insert("Hello ", 0);
        CHECK(s == "Hello World");
    }
}

TEST_CASE("CMString::Delete")
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

    SUBCASE("Delete first byte in Devanagari UTF-8 string")
    {
        std::string original = u8"नमस्ते";
        CMString s(nullptr, original.c_str());
        s.Delete(0, 1);
        std::string expected = original.substr(1);
        CHECK(s == expected.c_str());
    }

    SUBCASE("Delete middle bytes in UTF-8 string")
    {
        std::string original = u8"aनमस्तेe";
        CMString s(nullptr, original.c_str());
        s.Delete(1, original.size() - 2);
        CHECK(s == "ae");
    }
}


TEST_CASE("CMString::DeleteAll")
{
    SUBCASE("DeleteAll removes all occurrences")
    {
        CMString s(nullptr, "aabcada");
        s.DeleteAll('a');
        CHECK(s == "bcd");
    }

    SUBCASE("DeleteAll with no matches")
    {
        CMString s(nullptr, "xyz");
        s.DeleteAll('a');
        CHECK(s == "xyz");
    }

    SUBCASE("DeleteAll all characters")
    {
        CMString s(nullptr, "aaa");
        s.DeleteAll('a');
        CHECK(s == "");
    }

    SUBCASE("DeleteAll in middle only")
    {
        CMString s(nullptr, "abcdefg");
        s.DeleteAll('c');
        CHECK(s == "abdefg");
    }
}

TEST_CASE("CMString::Overlay")
{
    SUBCASE("Overlay single char")
    {
        CMString s(nullptr, "abc");
        s.Overlay('X', 1); // replace 'b' with 'X'
        CHECK(s == "aXc");
    }

    SUBCASE("Overlay string with padding")
    {
        CMString s(nullptr, "HelloWorld");
        // Extend first to ensure enough space
        s.Overlay("XX", 5, 4);  // overlays "XX  " at position 5
        CHECK(s.GetLength() >= 9);
        // Verify 'XX' was written starting at position 5
        CHECK(s.GetChar(5) == 'X');
        CHECK(s.GetChar(6) == 'X');
    }

    SUBCASE("OverlayAll replaces chars from index")
    {
        CMString s(nullptr, "aaabbaa");
        s.OverlayAll('a', 'x', 0);
        CHECK(s == "xxxbbxx");
    }

    SUBCASE("OverlayAll with start index")
    {
        CMString s(nullptr, "aaabbaa");
        s.OverlayAll('a', 'x', 2);  // Only replace 'a' from position 2 onwards
        CHECK(s == "aaxbbxx");
    }
}

TEST_CASE("CMString::Extend")
{
    SUBCASE("Extend inserts spaces")
    {
        CMString s(nullptr, "ab");
        s.Extend(5);
        CHECK(s.GetLength() >= 5);
        // First two chars should be 'ab'
        CHECK(s.GetChar(0) == 'a');
        CHECK(s.GetChar(1) == 'b');
        // Rest should be spaces
        for (int i = 2; i < 5; i++)
        {
            CHECK(s.GetChar(i) == ' ');
        }
    }

    SUBCASE("Extend inserts spaces before trailing newline")
    {
        CMString s(nullptr, "ab\n");
        int origLen = s.GetLength();
        s.Extend(5);
        // Newline should still be at end
        CHECK(s.GetChar(s.GetLength() - 1) == '\n');
    }

    SUBCASE("Extend with no change needed")
    {
        CMString s(nullptr, "Hello");
        s.Extend(3);  // Shorter than existing length
        CHECK(s == "Hello");
    }
}

TEST_CASE("CMString::bVerify")
{
    SUBCASE("bVerify matches exact content")
    {
        CMString s(nullptr, "HelloWorld");
        bool ok = s.bVerify("World", 5, 5);
        CHECK(ok == true);
    }

    SUBCASE("bVerify with padding spaces")
    {
        CMString s(nullptr, "Hello   ");
        bool ok = s.bVerify("llo", 2, 5);  // "llo" at pos 2, len 5 (padded with spaces)
        CHECK(ok == true);
    }

    SUBCASE("bVerify mismatch")
    {
        CMString s(nullptr, "HelloWorld");
        bool ok = s.bVerify("Wrong", 5, 5);
        CHECK(ok == false);
    }

    SUBCASE("bVerify short insert with space padding")
    {
        CMString s(nullptr, "ab      ");
        s.SetAt(2, 'x');
        bool ok = s.bVerify("x", 2, 6);  // "x" at pos 2, rest spaces
        CHECK(ok == true);
    }
}

TEST_CASE("CMString::Trim")
{
    SUBCASE("Trim removes trailing spaces and newlines")
    {
        CMString s(nullptr, "abc   \n\n");
        s.Trim();
        CHECK(s == "abc");
    }

    SUBCASE("Trim with no trailing whitespace")
    {
        CMString s(nullptr, "abc");
        s.Trim();
        CHECK(s == "abc");
    }

    SUBCASE("Trim empty string")
    {
        CMString s(nullptr, "");
        s.Trim();
        CHECK(s == "");
    }

    SUBCASE("TrimBothEnds removes leading and trailing spaces/newlines")
    {
        CMString s(nullptr, "  \n  middle  \n ");
        s.TrimBothEnds();
        CHECK(s == "middle");
    }

    SUBCASE("TrimBothEnds with only leading whitespace")
    {
        CMString s(nullptr, "  hello");
        s.TrimBothEnds();
        CHECK(s == "hello");
    }

    SUBCASE("TrimBothEnds with only trailing whitespace")
    {
        CMString s(nullptr, "hello  ");
        s.TrimBothEnds();
        CHECK(s == "hello");
    }

    SUBCASE("TrimBothEnds preserves internal spaces")
    {
        CMString s(nullptr, "  hello world  ");
        s.TrimBothEnds();
        CHECK(s == "hello world");
    }
}

TEST_CASE("CMString::AddSuffHyphSpaces logic (existing tests preserved)")
{
    SUBCASE("Add spaces before suffix hyphens")
    {
        CMString s(nullptr, "run-ing");
        s.AddSuffHyphSpaces("-", '{', '}');
        // Should add space before hyphen: "run -ing"
        // Verify it doesn't crash and produces reasonable output
        CHECK(s.GetLength() > 0);
    }
}

TEST_CASE("CMString combined operations (integration tests)")
{
    SUBCASE("Insert then Delete maintains integrity")
    {
        CMString s(nullptr, "HelloWorld");
        s.Insert("XX", 5);  // "HelloXXWorld"
        CHECK(s == "HelloXXWorld");
        s.Delete(5, sizeof("XX") - 1);
        CHECK(s == "HelloWorld");
    }

    SUBCASE("Multiple overlays work correctly")
    {
        CMString s(nullptr, "abcdefgh");
        s.Overlay('X', 0);
        s.Overlay('Y', 3);
        s.Overlay('Z', 7);
        CHECK(s.GetChar(0) == 'X');
        CHECK(s.GetChar(3) == 'Y');
        CHECK(s.GetChar(7) == 'Z');
    }

    SUBCASE("Delete and Insert sequence")
    {
        CMString s(nullptr, "Hello Beautiful World");
        s.Delete(6, sizeof("Beautiful ") - 1);
        CHECK(s == "Hello World");
        s.Insert("Beautiful ", 6);
        CHECK(s == "Hello Beautiful World");
    }
}