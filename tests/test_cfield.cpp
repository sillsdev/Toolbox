#include "test_common.h"
#include "cfield.h"
#include "crecpos.h"
#include "lng.h"

TEST_CASE("CField Tokenizing")
{
    CLangEncSet lngset("1.0", "");
    CMarkerSet mkrset(&lngset);
    CMarker* pmkr = mkrset.pmkrAdd_MarkAsNew("tx");

    SUBCASE("Standard Tokenization (bMultipleItemData = TRUE)")
    {
        CMarker_Test::SetMultipleItemData(pmkr, TRUE);
        CField field(pmkr, "  alpha  beta   gamma  ");
        CRecPos start, end;

        CHECK(field.bParseFirstItem(nullptr, start, end, TRUE) == TRUE);
        CHECK(field.sItem(start, end) == "alpha");

        CHECK(field.bParseNextItem(start, end, TRUE) == TRUE);
        CHECK(field.sItem(start, end) == "beta");

        CHECK(field.bParseNextItem(start, end, TRUE) == TRUE);
        CHECK(field.sItem(start, end) == "gamma");

        CHECK(field.bParseNextItem(start, end, TRUE) == FALSE);
    }

    SUBCASE("Single Item Mode (bMultipleItemData = FALSE)")
    {
        CMarker_Test::SetMultipleItemData(pmkr, FALSE);
        CField field(pmkr, "  item with spaces  ");
        CRecPos start, end;

        // In this mode, the entire non-white span is one item
        CHECK(field.bParseFirstItem(nullptr, start, end, FALSE) == TRUE);
        CHECK(field.sItem(start, end) == "item with spaces");

        // No subsequent items exist
        CHECK(field.bParseNextItem(start, end, FALSE) == FALSE);
    }

    SUBCASE("Multi-line Normalization")
    {
        CField field(pmkr, "line1\nline2");
        CRecPos start, end;
        start.SetPos(0, &field, nullptr);
        end.SetPos(field.GetLength(), &field, nullptr);

        // sItem converts '\n' to ' ' if bHandleMultiLineData is TRUE
        CHECK(field.sItem(start, end, TRUE) == "line1 line2");
        CHECK(field.sItem(start, end, FALSE) == "line1\nline2");
    }

    SUBCASE("Validation: Required Data")
    {
        CMarker_Test::SetMustHaveData(pmkr, TRUE);
        CField field(pmkr, "   "); // All whitespace
        CRecPos start, end;
        Str8 message;
        BOOL bExplicitNewlines;

        CHECK(field.bValidItemCount(nullptr, start, end, message, bExplicitNewlines) == FALSE);
        CHECK(message.Find("requires data") != -1);
    }

    SUBCASE("Edge Case: Empty Field")
    {
        CField field(pmkr, "");
        CRecPos start, end;

        // Parsing an empty field should return FALSE immediately
        CHECK(field.bParseFirstItem(nullptr, start, end, TRUE) == FALSE);
    }
}