#include "test_common.h"
#include "crecpos.h"
#include "crecord.h"

TEST_CASE("CRecPos: Basic Navigation") {
    CLangEncSet lngset("1.0", "");
    CMarkerSet mkrset(&lngset);
    CMarker* pmkr = mkrset.pmkrAdd_MarkAsNew("tx");

    CField* pfld = new CField(pmkr, "Word1 Word2");
    CRecord rec(pfld);

    SUBCASE("Word skipping logic") {
        CRecPos rps(0, pfld, &rec);
        CHECK(rps.iChar == 0);

        rps.bSkipNonWhite();
        CHECK(rps.iChar == 5); // Index of space

        rps.bSkipWhite();
        CHECK(rps.iChar == 6); // Index of 'W'
        CHECK(rps.bMatch("Word2") == TRUE);
    }

    SUBCASE("SetPosEx padding logic") {
        CRecPos rps(0, pfld, &rec);
        rps.SetPosEx(15);

        CHECK(rps.iChar == 15);
        CHECK(pfld->sContents().GetChar(12) == ' ');
    }
}

TEST_CASE("CRecPos: Interlinear Bundle Navigation") {
    CLangEncSet lngset("1.0", "");
    CMarkerSet mkrset(&lngset);

    CMarker* pmkrTop = mkrset.pmkrAdd_MarkAsNew("f");
    CMarker* pmkrBot = mkrset.pmkrAdd_MarkAsNew("g");
    CMarker_Test::SetInterlinear(pmkrTop, TRUE);  // Sets bInterlinear=T, bFirstInterlinear=T
    CMarker_Test::SetInterlinear(pmkrBot, FALSE); // Sets bInterlinear=T, bFirstInterlinear=F

    CField* f1 = new CField(pmkrTop, "TopLine");
    CField* f2 = new CField(pmkrBot, "BotLine");
    CRecord rec(f1);
    rec.Add(f2);

    SUBCASE("Bundle boundaries") {
        CRecPos rps(0, f1, &rec);

        CHECK(rps.bFirstInterlinear() == TRUE);
        CHECK(rps.bLastInBundle() == FALSE);

        CRecPos rpsNext = rps.rpsNextField();
        CHECK(rpsNext.pfld == f2);
        CHECK(rpsNext.bFirstInterlinear() == FALSE);
        CHECK(rpsNext.bLastInBundle() == TRUE);
    }

    SUBCASE("Structure splitting (BreakBundle)") {
        // BreakBundle: Splits all fields in a bundle at iChunkLen
        // and inserts the "tails" as a new bundle after the current one.
        CRecPos rps(0, f1, &rec);
        int originalFieldCount = rec.lGetCount();

        // Split at "Top" | "Line"
        rps.BreakBundle(3);

        // After BreakBundle, rec should have:
        // [f: "Top"], [g: "Bot"], [f: "Line\n"], [g: "Line"]
        CHECK(rec.lGetCount() == originalFieldCount + 2);

        // Verify the content of the split fields
        CHECK(f1->sContents() == "Top");

        CField* f1Tail = rec.pfldNext(f2); // Get the first field of the new bundle
        // The logic in BreakBundle appends "\n" to the last field of the first bundle
        // depending on the bLastInBundle check.
        if (f1Tail == nullptr) {
            CHECK(f1Tail != nullptr); // fail
        } else {
            CHECK(f1Tail->sContents().Find("Line") != -1);
        }
    }
}

TEST_CASE("CRecPos: View Navigation") {
    CLangEncSet lngset("1.0", "");
    CMarkerSet mkrset(&lngset);
    CMarker* pmkr = mkrset.pmkrAdd_MarkAsNew("tx");

    // Testing \n as a "View Substring" delimiter
    CField* pfld = new CField(pmkr, "Line One\nLine Two");
    CRecord rec(pfld);
    CRecPos rps(0, pfld, &rec);

    SUBCASE("MoveRightPastViewSubstring") {
        CRecPos start, end;

        // This should move from 0 to the newline (pos 8)
        rps.MoveRightPastViewSubstring(&start, &end);
        CHECK(start.iChar == 0);
        CHECK(end.iChar == 8);
        CHECK(rps.iChar == 9); // Should be past the newline

        // Should move to end of field
        rps.MoveRightPastViewSubstring(&start, &end);
        CHECK(rps.bEndOfField() == TRUE);
    }
}