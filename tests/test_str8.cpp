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

    SUBCASE("insert or prepend")
    {
        Str8 s1("ab");
        Str8 s3("f");

#ifdef UseCharStar
        s3.Prepend(s1);
        CHECK(s3 == "abf");

        s3.Prepend("c");
        CHECK(s3 == "cabf");
#else
        s3.Insert(0, s1);
        CHECK(s3 == "abf");

        s3.Insert(0, "c");
        CHECK(s3 == "cabf");
#endif
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
}