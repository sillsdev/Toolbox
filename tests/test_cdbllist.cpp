#include "test_common.h"
#include "cdbllist.h"

class CDblListEl_Test : public CDblListEl {
public:
    using CDblListEl::lNum;
    using CDblListEl::SetNum;
    using CDblListEl::ClearNum;

    bool bStableIndex() {
        return m_stableIndex.has_value();
    }
};

TEST_CASE("CDblList with CDblListEl")
{
    SUBCASE("InsertBefore correctly links elements")
    {
        CDblList list;
        auto pelCur = new CDblListEl;
        auto pelNew = new CDblListEl;
        list.Add(pelCur);
        list.InsertBefore(pelCur, pelNew);
        CHECK(list.pelNext(pelNew) == pelCur);
        CHECK(list.pelPrev(pelCur) == pelNew);
        CHECK(list.bIsFirst(pelNew));
        CHECK(list.bIsLast(pelCur));
    }

    SUBCASE("InsertAfter correctly links elements")
    {
        CDblList list;
        auto pelCur = new CDblListEl;
        auto pelNew = new CDblListEl;
        list.Add(pelCur);
        list.InsertAfter(pelCur, pelNew);
        CHECK(list.pelPrev(pelNew) == pelCur);
        CHECK(list.pelNext(pelCur) == pelNew);
        CHECK(list.bIsFirst(pelCur));
        CHECK(list.bIsLast(pelNew));
    }


    SUBCASE("bIsEmpty and Add")
    {
        CDblList list;
        CHECK(list.bIsEmpty());
        auto pel = new CDblListEl;
        list.Add(pel);
        CHECK(!list.bIsEmpty());
        CHECK(list.bIsFirst(pel));
        CHECK(list.bIsLast(pel));
        CHECK(list.bIsMember(pel));
    }

    SUBCASE("DeleteAll removes elements")
    {
        CDblList list;
        auto pel1 = new CDblListEl;
        auto pel2 = new CDblListEl;
        list.Add(pel1);
        list.Add(pel2);
        list.DeleteAll();
        CHECK(list.bIsEmpty());
    }

    SUBCASE("RemoveAll clears list without deleting elements")
    {
        CDblList list;
        auto pel1 = new CDblListEl_Test;
        auto pel2 = new CDblListEl_Test;
        list.Add(pel1);
        list.Add(pel2);
        list.RemoveAll();
        CHECK(list.bIsEmpty());
        // pointers still valid
        pel1->SetNum(1);
        pel2->SetNum(2);
    }

    SUBCASE("MoveElementsTo transfers elements")
    {
        CDblList list1;
        CDblList list2;
        auto pel1 = new CDblListEl;
        auto pel2 = new CDblListEl;
        list1.Add(pel1);
        list1.Add(pel2);
        list1.MoveElementsTo(&list2);
        CHECK(list1.bIsEmpty());
        CHECK(list2.bIsFirst(pel1));
        CHECK(list2.bIsLast(pel2));
    }

    SUBCASE("m_stableIndex behavior")
    {
        auto pel = new CDblListEl_Test;
        const int expected = 7;
        CHECK(!pel->bStableIndex());
        pel->SetNum(expected);
        CHECK(pel->bStableIndex());
        CHECK(pel->lNum() == expected);
        pel->ClearNum();
        CHECK(!pel->bStableIndex());
        pel->SetNum(expected);
        CHECK(pel->lNum() == expected);
        CHECK(pel->bStableIndex());
    }
}
