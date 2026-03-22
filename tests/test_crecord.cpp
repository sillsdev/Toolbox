#include "test_common.h"
#include "crecord.h"

void CheckField(CField* pfldResult, CField* pfldExpected, const char* szLabel = "")
{
    if (szLabel && *szLabel) {
        INFO("Scenario: " << szLabel);
    }
    if (pfldExpected == nullptr) {
        if (pfldResult == nullptr) {
            // pass
            CHECK(pfldResult == pfldExpected);
        } else {
            // fail
            CHECK_MESSAGE(pfldResult == nullptr, "expected NULL but got '" << *pfldResult << "')");
        }
    } else if (pfldResult == nullptr) {
        // fail
        CHECK_MESSAGE(pfldResult != nullptr, "got NULL but expected '" << *pfldExpected << "')");
    } else {
        // test to determine outcome
        CHECK(*pfldResult == *pfldExpected);
    }
}

TEST_CASE("CRecord Hierarchy")
{
    CLangEncSet lngset("1.0", "");
    CMarkerSet mkrset(&lngset);

    // Setup hierarchy: \r (record) -> \s (section) -> \p (paragraph)
    CMarker* pmkrR = mkrset.pmkrAdd_MarkAsNew("r");
    CMarker* pmkrS = mkrset.pmkrAdd_MarkAsNew("s");
    CMarker* pmkrP = mkrset.pmkrAdd_MarkAsNew("p");
    CMarker_Test::SetParent(pmkrS, pmkrR);
    CMarker_Test::SetParent(pmkrP, pmkrS);

    // Setup data
    CField* fKey = new CField(pmkrR, "RootKey");
    CRecord rec(fKey);
    CField* fS1 = new CField(pmkrS, "Sec1");
    CField* fP1 = new CField(pmkrP, "Para1");
    CField* fS2 = new CField(pmkrS, "Sec2");
    CField* fP2 = new CField(pmkrP, "Para2");
    CField* fP3 = new CField(pmkrP, "Para3");
    CField* fS3 = new CField(pmkrS, "Sec3");
    rec.Add(fS1);
    rec.Add(fP1);
    rec.Add(fS2);
    rec.Add(fP2);
    rec.Add(fP3);
    rec.Add(fS3);
    
    SUBCASE("Key Access")
    {
        CHECK(rec.sKey() == "RootKey");
    }

    SUBCASE("pfldFirstInSubRecord")
    {
        CheckField(rec.pfldFirstInSubRecord(pmkrP, fS1), fP1, "Marker under key");
        CheckField(rec.pfldFirstInSubRecord(pmkrP, fS2), fP2, "Marker under key");
        CheckField(rec.pfldFirstInSubRecord(pmkrP, fS3), nullptr, "Marker not under key");
        CheckField(rec.pfldFirstInSubRecord(pmkrS, fS3), fS3, "Same marker as key");
    }

    SUBCASE("ApplyDateStamp")
    {
        CMarker* pmkrDate = mkrset.pmkrAdd_MarkAsNew("dt");
        rec.ApplyDateStamp(pmkrDate);

        CField* fDate = rec.pfldFirstWithMarker(pmkrDate);
        REQUIRE(fDate != nullptr);
        CHECK(fDate->sContents().GetLength() > 0);
    }
}

/*
 Record Structure (example from the main code):
 \w ball
   \s 1
     \p n
       \g spherical toy
       \i The pitcher threw the ball.
   \s 2
     \p n
       \g party
         \i The princess went to the ball.
       \g fun
         \i She had a ball.
         \t Se gustaba mucho.

 | Mkr | Start Key | Return    | Logic Description                               |
 |-----|-----------|-----------|-------------------------------------------------|
 | g   | nullptr   | \g spher..| No key: return first occurrence in record.      |
 | g   | \g party  | \g party  | Key IS the desired marker: return key.          |
 | g   | \s 2      | \g party  | Key OVER marker: search forward for first child.|
 | g   | \i She h..| \g fun    | Marker OVER key: search backward for parent.    |
 | i   | \t Se gu..| \i She h..| Siblings: backward for common parent, then fwd. |
 */
TEST_CASE("CRecord Ball Example")
{
    CLangEncSet lngset("1.0", "");
    CMarkerSet mkrset(&lngset);

    // 1. Define Hierarchy
    CMarker* pmkrW = mkrset.pmkrAdd_MarkAsNew("w");
    CMarker* pmkrS = mkrset.pmkrAdd_MarkAsNew("s");
    CMarker* pmkrP = mkrset.pmkrAdd_MarkAsNew("p");
    CMarker* pmkrG = mkrset.pmkrAdd_MarkAsNew("g");
    CMarker* pmkrI = mkrset.pmkrAdd_MarkAsNew("i");
    CMarker* pmkrT = mkrset.pmkrAdd_MarkAsNew("t");
    CMarker_Test::SetParent(pmkrS, pmkrW);
    CMarker_Test::SetParent(pmkrP, pmkrS);
    CMarker_Test::SetParent(pmkrG, pmkrP);
    CMarker_Test::SetParent(pmkrI, pmkrG);
    CMarker_Test::SetParent(pmkrT, pmkrI);

    // 2. Build the Record
    CField* fW = new CField(pmkrW, "ball");
    CRecord rec(fW);

    // \s 1 block
    CField* fS1 = new CField(pmkrS, "1");
    CField* fP1 = new CField(pmkrP, "n");
    CField* fG1 = new CField(pmkrG, "spherical toy");
    CField* fI1 = new CField(pmkrI, "The pitcher threw the ball.");
    rec.Add(fS1); rec.Add(fP1); rec.Add(fG1); rec.Add(fI1);

    // \s 2 block
    CField* fS2 = new CField(pmkrS, "2");
    CField* fP2 = new CField(pmkrP, "n");
    CField* fG2 = new CField(pmkrG, "party");
    CField* fI2 = new CField(pmkrI, "The princess went to the ball.");
    CField* fG3 = new CField(pmkrG, "fun");
    CField* fI3 = new CField(pmkrI, "She had a ball.");
    CField* fT1 = new CField(pmkrT, "Se gustaba mucho.");
    rec.Add(fS2); rec.Add(fP2); rec.Add(fG2); rec.Add(fI2); rec.Add(fG3); rec.Add(fI3); rec.Add(fT1);

    SUBCASE("Documentation Examples")
    {
        CheckField(rec.pfldFirstInSubRecord(pmkrG, nullptr), fG1, "No key");
        CheckField(rec.pfldFirstInSubRecord(pmkrG, fG2), fG2, "Key is target");
        CheckField(rec.pfldFirstInSubRecord(pmkrG, fS2), fG2, "Key over marker");
        CheckField(rec.pfldFirstInSubRecord(pmkrG, fI3), fG3, "Marker over key");
        CheckField(rec.pfldFirstInSubRecord(pmkrI, fT1), fI3, "Siblings");
    }
}

TEST_CASE("CFieldList")
{
    CLangEncSet lngset("1.0", "");
    CMarkerSet mkrset(&lngset);
    CMarker* pmkr = mkrset.pmkrAdd_MarkAsNew("tx");

    SUBCASE("List Management")
    {
        CFieldList list;
        CField* f1 = new CField(pmkr, "first");
        CField* f2 = new CField(pmkr, "second");

        list.Add(f1);
        list.Add(f2);

        CHECK(list.pfldFirst() == f1);
        CHECK(list.pfldLast() == f2);
        CHECK(list.pfldNext(f1) == f2);
        CHECK(list.pfldPrev(f2) == f1);

        list.Delete(f1);
        CHECK(list.pfldFirst() == f2);

        // Clean up remaining
        list.DeleteRest(list.pfldFirst());
    }

    SUBCASE("EliminateDuplicates")
    {
        CFieldList list;
        list.Add(new CField(pmkr, "unique"));
        list.Add(new CField(pmkr, "repeat  ")); // Note the extra space
        list.Add(new CField(pmkr, "repeat"));

        list.EliminateDuplicates(FALSE);

        int count = 0;
        for (CField* p = list.pfldFirst(); p; p = list.pfldNext(p)) count++;

        CHECK(count == 2);
        CHECK(list.pfldLast()->sContents() == "repeat");

        list.DeleteRest(list.pfldFirst());
    }

    SUBCASE("EliminateShorter")
    {
        CFieldList list;
        // List is expected to be "longest first" for this logic to trigger correctly
        list.Add(new CField(pmkr, "long content"));
        list.Add(new CField(pmkr, "short"));

        list.EliminateShorter();

        CHECK(list.pfldFirst() != nullptr);
        CHECK(list.pfldNext(list.pfldFirst()) == nullptr);

        list.DeleteRest(list.pfldFirst());
    }

    SUBCASE("CFieldIterator")
    {
        // Minimal skeleton for Iterator testing
        // This typically requires CIndex and CRecLookEl which are often heavy,
        // but we can test the state transitions.
        CFieldIterator it;
        CFieldIterator end;
        CHECK(it == end);
    }
}
