#include "test_common.h"

TEST_CASE("Str8 validation")
{
    SUBCASE("basic construction and comparison")
    {
        Str8 s1("abc");
        s1.AssertValid();

        Str8 s2 = "abc";
        s2.AssertValid();

        CHECK(s1 == s2);
        CHECK(s1 == "abc");
        CHECK("abc" == s1);
        CHECK_FALSE(s1 != s2);
        CHECK_FALSE(s1 != "abc");
        CHECK_FALSE("abc" != s1);

        Str8 s3("a");
        CHECK(s3 == "a");
        CHECK(s3 == 'a');
        CHECK(s3 != 'b');
        CHECK(s3 != "ab");
    }

    SUBCASE("comparison operators")
    {
        Str8 s1("abc");
        Str8 s3("a");

        CHECK(s1 > s3);
        CHECK(s1 > "ab");
        CHECK("bc" > s1);

        CHECK(s3 < s1);
        CHECK("ab" < s1);
        CHECK(s1 < "bc");

        CHECK(s1 >= s3);
        CHECK(s1 >= "ab");
        CHECK("bc" >= s1);

        CHECK(s3 <= s1);
        CHECK("ab" <= s1);
        CHECK(s1 <= "bc");

        CHECK(s1 >= "abc");
        CHECK(s1 <= "abc");
    }

    SUBCASE("append and concatenation")
    {
        Str8 s1("ab");
        Str8 s3("a");
        Str8 s2 = s1 + s3;

        CHECK(s2 == "aba");

        s1.Append("c");
        CHECK(s1 == "abc");

        s2 = s1 + "d";
        s2.AssertValid();
        CHECK(s2 == "abcd");

        s2 = "e" + s2;
        CHECK(s2 == "eabcd");

        s2 = sTestCopy("abcd");
        s2.AssertValid();
        CHECK(s2 == "abcd");

        s2 += "e";
        CHECK(s2 == "abcde");

        s3 = "f";
        s2 += s3;
        CHECK(s2 == "abcdef");
    }

    SUBCASE("insert")
    {
        Str8 s1("abc");

        s1.Insert(0, "z");
        CHECK(s1 == "zabc");

        s1 = "abc";
        s1.Insert(1, "xyz");
        CHECK(s1 == "axyzbc");

        s1 = "abc";
        s1.Insert(10, "z");
        CHECK(s1 == "abcz");

        Str8 s2("ab");
        Str8 s3("f");

        // Prepending via Insert at index 0 (replaces the defunct Prepend)
        s3.Insert(0, s2);
        CHECK(s3 == "abf");

        s3.Insert(0, "c");
        CHECK(s3 == "cabf");

        // Inserting in the middle
        Str8 sMid("ac");
        sMid.Insert(1, "b");
        CHECK(sMid == "abc");

        // Inserting past end (should append or clamp to end)
        sMid.Insert(10, "d");
        CHECK(sMid == "abcd");
    }

    SUBCASE("char concatenation")
    {
        Str8 s1("a");

        CHECK(s1 == 'a');

        s1 = s1 + 'z';
        CHECK(s1 == "az");

        CHECK('y' + s1 == "yaz");

        s1 = "a";
        CHECK(s1 == 'a');
        CHECK(s1 != 'b');
    }

    SUBCASE("find")
    {
        Str8 s1("abcabcdef");

        CHECK(s1.Find('a') == 0);
        CHECK(s1.Find('b') == 1);
        CHECK(s1.Find('x') == -1);

        CHECK(s1.Find("a") == 0);
        CHECK(s1.Find("bc") == 1);
        CHECK(s1.Find("bx") == -1);

        CHECK(s1.Find('a', 1) == 3);
        CHECK(s1.Find("bc", 4) == 4);
        CHECK(s1.Find("bc", 5) == -1);
    }

    SUBCASE("mid substring")
    {
        Str8 s1("abcabcdef");

        CHECK(s1.Mid(1, 2) == "bc");

        CHECK(s1.Mid(0) == "abcabcdef");
        CHECK(s1.Mid(1) == "bcabcdef");
        CHECK(s1.Mid(2) == "cabcdef");
        CHECK(s1.Mid(5) == "cdef");
        CHECK(s1.Mid(8) == "f");
        CHECK(s1.Mid(9) == "");
        CHECK(s1.Mid(10) == "");

        CHECK(s1.Mid(0, 0) == "");
        CHECK(s1.Mid(0, 1) == "a");
        CHECK(s1.Mid(0, 9) == "abcabcdef");
        CHECK(s1.Mid(0, 10) == "abcabcdef");

        CHECK(s1.Mid(1, 1) == "b");
        CHECK(s1.Mid(3, 4) == "abcd");
        CHECK(s1.Mid(4, 10) == "bcdef");
    }

    SUBCASE("truncate")
    {
        Str8 s1("abcabcdef");
        s1.Truncate(3);
        CHECK(s1 == "abc");
    }

    SUBCASE("replace")
    {
        Str8 s1("abcabcdef");

        s1.Replace("abc", "xyz");
        CHECK(s1 == "xyzxyzdef");

        s1 = "abcabcdef";
        s1.Replace("abc", "xy");
        CHECK(s1 == "xyxydef");

        s1 = "abcabcdef";
        s1.Replace("abc", "xyzw");
        CHECK(s1 == "xyzwxyzwdef");

        s1 = "abcabcdef";
        s1.Replace("abc", "");
        CHECK(s1 == "def");

        s1 = "abcabcdef";
        s1.Replace("a", "");
        CHECK(s1 == "bcbcdef");
    }

    SUBCASE("set character")
    {
        Str8 s1("abc");

        s1.SetAt(5, 'z');
        CHECK(s1 == "abc");

        s1.SetAt(0, 'x');
        CHECK(s1 == "xbc");

        s1 = "abc";
        s1.SetAt(2, 'z');
        CHECK(s1 == "abz");
    }

    SUBCASE("trim")
    {
        Str8 s1(" \nabc");
        s1.TrimLeft();
        CHECK(s1 == "abc");

        s1 = " \nabc \n";
        s1.TrimLeft();
        s1.TrimRight();
        CHECK(s1 == "abc");

        s1 = "";
        s1.TrimLeft();
        s1.TrimRight();
        CHECK(s1 == "");
    }

    SUBCASE("delete")
    {
        Str8 s1("abc");

        s1.Delete(0, 0);
        CHECK(s1 == "abc");

        s1.Delete(1, 0);
        CHECK(s1 == "abc");

        s1.Delete(10, 0);
        CHECK(s1 == "abc");

        s1.Delete(0, 1);
        CHECK(s1 == "bc");

        s1 = "abc";
        s1.Delete(0, 2);
        CHECK(s1 == "c");

        s1 = "abc";
        s1.Delete(0, 3);
        CHECK(s1 == "");

        s1 = "abc";
        s1.Delete(1, 1);
        CHECK(s1 == "ac");

        s1 = "abc";
        s1.Delete(1, 2);
        CHECK(s1 == "a");

        s1 = "abc";
        s1.Delete(3, 10);
        CHECK(s1 == "abc");
    }

    SUBCASE("bNextWord")
    {
        Str8 s1("a b");
        Str8 sWord;
        int i = 0;

        CHECK(s1.bNextWord(sWord, i));
        CHECK(sWord == "a");

        CHECK(s1.bNextWord(sWord, i));
        CHECK(sWord == "b");

        CHECK_FALSE(s1.bNextWord(sWord, i));

        s1 = "\n abc.txt    1.23\n4.56\n   ";
        i = 0;

        CHECK(s1.bNextWord(sWord, i));
        CHECK(sWord == "abc.txt");

        CHECK(s1.bNextWord(sWord, i));
        CHECK(sWord == "1.23");

        CHECK(s1.bNextWord(sWord, i));
        CHECK(sWord == "4.56");

        CHECK_FALSE(s1.bNextWord(sWord, i));
    }

    SUBCASE("operator plus equals char")
    {
        Str8 s1("ab");
        s1 += 'c';
        CHECK(s1 == "abc");
    }

    SUBCASE("GetBuffer default param idiom - in-place truncate")
    {
        Str8 s("Hello World!");
        int iLen = s.GetLength();

        // Idiom: call GetBuffer() with default parameter (0) to get write access
        // without destroying existing buffer contents, then cut off the trailing char.
        char* p = s.GetBuffer();
        CHECK(p != nullptr);
        
        s.ReleaseBuffer(iLen - 1);

        CHECK(s == "Hello World");
        CHECK(s.GetLength() == 11);

        // Do it again down to empty to verify repeated truncation
        while (s.GetLength() > 0)
        {
            int curLen = s.GetLength();
            s.GetBuffer();
            s.ReleaseBuffer(curLen - 1);
        }
        CHECK(s.IsEmpty());
        CHECK(s.GetLength() == 0);
    }

    SUBCASE("Constructor with pointer arithmetic and token lengths (parser pattern)")
    {
        // Simulates reading markers from a buffer like: "field1\r\n\\marker content"
        const char* buffer = "field1\r\n\\marker rest of the data";
        const char* p = strstr(buffer, "\r\n");
        REQUIRE(p != nullptr);

        // Pattern 1: slice up to delimiter via pointer difference (p - s)
        int len = static_cast<int>(p - buffer);
        Str8 sField(buffer, len);
        CHECK(sField == "field1");
        CHECK(sField.GetLength() == 6);

        // Pattern 2: marker extraction via strcspn
        const char* pszMkr = p + 2 + 1; // Skip "\r\n\" to get to "marker"
        int mkrLen = static_cast<int>(strcspn(pszMkr, " \t\r\n"));
        Str8 sMkr(pszMkr, mkrLen);
        CHECK(sMkr == "marker");
        CHECK(sMkr.GetLength() == 6);
    }

    SUBCASE("Constructor safety: non-null-terminated buffer slice")
    {
        // A raw array with NO trailing null terminator '\0'
        // The old code (_data = pszInit) runs strlen() here, causing a buffer overread!
        const char rawChars[5] = { 'H', 'e', 'l', 'l', 'o' };

        // With the new code, assign(rawChars, 5) only reads 5 bytes:
        Str8 s(rawChars, 5);
        CHECK(s.GetLength() == 5);
        CHECK(s == "Hello");

        // Substring slice of non-null-terminated data
        Str8 sSub(rawChars + 1, 3);
        CHECK(sSub.GetLength() == 3);
        CHECK(sSub == "ell");
    }

    SUBCASE("Constructor with null and negative/default counts")
    {
        // Verify null pointer robustness
        Str8 sNull(nullptr);
        CHECK(sNull.IsEmpty());

        Str8 sNullCount(nullptr, 5);
        CHECK(sNullCount.IsEmpty());

        // Default count (-1) copies whole null-terminated string
        const char* text = "complete string";
        Str8 sFull(text, -1);
        CHECK(sFull == "complete string");
        CHECK(sFull.GetLength() == 15);

        // Explicit 0 length creates empty string without reading buffer
        Str8 sZero(text, 0);
        CHECK(sZero.IsEmpty());
    }

    SUBCASE("length, empty, and getchar")
    {
        Str8 sEmpty;
        CHECK(sEmpty.IsEmpty());
        CHECK(sEmpty.GetLength() == 0);

        Str8 s("Hello");
        CHECK_FALSE(s.IsEmpty());
        CHECK(s.GetLength() == 5);
        CHECK(s.GetChar(0) == 'H');
        CHECK(s.GetChar(4) == 'o');

        s.Empty();
        CHECK(s.IsEmpty());
        CHECK(s.GetLength() == 0);
        CHECK(s == "");
    }

    SUBCASE("Left, Right, and Trim")
    {
        Str8 s("HelloWorld");
        CHECK(s.Left(5) == "Hello");
        CHECK(s.Right(5) == "World");
        CHECK(s.Left(0) == "");
        CHECK(s.Right(0) == "");
        CHECK(s.Left(20) == "HelloWorld");
        CHECK(s.Right(20) == "HelloWorld");

        Str8 sSpaced("   \t spaced text \r\n  ");
        sSpaced.Trim();
        CHECK(sSpaced == "spaced text");
    }

    SUBCASE("ReverseFind and FindAtEndOfWord")
    {
        Str8 s("abc.def.ghi.def");
        CHECK(s.ReverseFind('.') == 11);
        CHECK(s.ReverseFind('z') == -1);

        Str8 sWords("cat category cat dog");
        CHECK(sWords.FindAtEndOfWord("cat", 0) == 0);
        // "category" contains "cat", but is not at the end of word:
        CHECK(sWords.FindAtEndOfWord("cat", 1) == 13);
    }

    SUBCASE("GetBuffer and ReleaseBuffer - implicit length (-1)")
    {
        Str8 s;
        // Request buffer space, write directly via pointer
        char* p = s.GetBuffer(64);
        CHECK(p != nullptr);
        
        // Use standard safe writing
        snprintf(p, 64, "%s", "buffer test");
        
        // ReleaseBuffer(-1) should calculate length using strlen()
        s.ReleaseBuffer(-1);

        CHECK(s == "buffer test");
        CHECK(s.GetLength() == 11);

        // Verify const char* operator yields the same
        CHECK(strcmp(s, "buffer test") == 0);
    }

    SUBCASE("GetBuffer and ReleaseBuffer - explicit length truncation")
    {
        Str8 s("initial text");
        char* p = s.GetBuffer(32);
        snprintf(p, 32, "%s", "abcdefghij");
        
        // Explicitly commit fewer characters than written
        s.ReleaseBuffer(4);

        CHECK(s == "abcd");
        CHECK(s.GetLength() == 4);
    }

    SUBCASE("GetBuffer pointer arithmetic and realloc safety")
    {
        Str8 s("short");
        // Expanding to a significantly larger size should safely reallocate 
        // without dangling pointers in std::string implementation
        char* p = s.GetBuffer(1024);
        for (int i = 0; i < 26; ++i)
        {
            *(p + i) = 'a' + i;
        }
        *(p + 26) = '\0';
        s.ReleaseBuffer();

        CHECK(s.GetLength() == 26);
        CHECK(s == "abcdefghijklmnopqrstuvwxyz");
    }

    SUBCASE("Format")
    {
        Str8 s;
        s.Format("%s: %d + %.2f", "Result", 42, 3.14);
        CHECK(s == "Result: 42 + 3.14");

        // Format overwriting existing string
        s.Format("%05d", 123);
        CHECK(s == "00123");
        CHECK(s.GetLength() == 5);
    }

    SUBCASE("operator += int (numeric addition)")
    {
        Str8 sNum("40");
        sNum += 2;
        CHECK(sNum == "402");

        Str8 sNegative("-10");
        sNegative += 5;
        CHECK(sNegative == "-105");
    }

    SUBCASE("Settings tag functions")
    {
        Str8 sMarkup("\\+config\n  \\port 8080\n\\-config");
        Str8 sContent;

        // Extract content of \port...
        CHECK(sMarkup.bGetSettingsTagContent(sContent, "port"));
        CHECK(sContent == "8080");

        // Extract entire section including tags
        Str8 sSection;
        int iStart = 0;
        CHECK(sMarkup.bGetSettingsTagSection(sSection, "config", iStart, TRUE));
        CHECK(sSection == sMarkup);

        // Delete tag section
        INFO("sMarkup contents:\n", sMarkup);
        CHECK(sMarkup.Find("\\+config") != -1);
        CHECK(sMarkup.Find("8080") != -1);
        CHECK(sMarkup.bDeleteSettingsTagSection("config"));
        INFO("sMarkup contents:\n", sMarkup);
        CHECK(sMarkup.Find("\\+config") == -1);
        CHECK(sMarkup.Find("8080") == -1);
    }
}
