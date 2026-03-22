#include "test_common.h"
#include "interlin.h"
#include "crecpos.h"
#include "crecord.h"
#include "typ.h"
#include "mainfrm.h"

class CDbTrie_Test {
public:
    static void SetLoaded(CDbTrie* ptri, BOOL bVal) {
        ptri->m_bLoaded = bVal;
    }
};

class CLookupProc_Test {
public:
    static void ManualAddEntry(CLookupProc* pProc, int trieType, const char* key, CField* pfld, CRecord* prec) {
        CRecPos rps(0, pfld, prec);
        CDbTrie* pTrie = pProc->ptri(trieType);

        // 1. Reverse suffix keys manually if the engine expects it
        // (The parser reverses 'jumped' to 'depmuj' and looks for 'de')
        pTrie->Add(key, rps, trieType);

        // 2. CDbTrie::bIsEmpty() checks the reference list.
        // If empty, the engine skips this trie.
        if (pTrie->pdrflst()->bIsEmpty()) {
            pTrie->pdrflst()->Add(new CDatabaseRef("mock.db", "lx"));
        }

        // 3. Prevent engine from attempting to actually open "mock.db"
        CDbTrie_Test::SetLoaded(pTrie, TRUE);

        // 4. Set language context (so the trie knows how to compare characters)
        if (pTrie->pmrflst()->bIsEmpty()) {
            pTrie->pmrflst()->AddMarkerRef(pfld->pmkr());
        }
    }
};

TEST_CASE("CLookupProc: Morpheme Parsing Logic") {
    CShwApp* pApp = (CShwApp*)Shw_papp();
    CMainFrame dummyMainFrame;
    CWnd* pOldWnd = pApp->m_pMainWnd;
    pApp->m_pMainWnd = &dummyMainFrame;

    std::string filename = "adir/test.prj";
    TestFile mockFile(filename, "\\+ShProjectSettings\n\\AutoWrap\n");
    CShwApp_Test::ResetProject(pApp);
    CProject::s_bConstruct(mockFile.c_str(), TRUE, "1.2.3", &CShwApp_Test::Project(pApp));

    // Use pointers so we can control exactly when they are deleted
    CField* fRoot = nullptr, * fSuff = nullptr, * fSource = nullptr;
    CRecord* recRoot = nullptr, * recSuff = nullptr, * recSource = nullptr;

    // We use a pointer for pProc because CInterlinearProcList will try to delete it
    // if we aren't careful, but here we manually add it to the list.
    CLookupProc* pProc = nullptr;

    // Get pointers from the project
    CProject* pprj = CShwApp_Test::Project(pApp);
    CNoteList notlst;
    CLangEnc* plng = pprj->plngset()->plngSearch_AddIfNew("TestLang", notlst);
    CDatabaseType* ptyp = pprj->ptypset()->ptypNew("Linguistic", "lx");
    CMarkerSet* mkrset = ptyp->pmkrset();

    CMarker* pmkrTx = mkrset->pmkrSearch_AddIfNew("tx");
    CMarker* pmkrMb = mkrset->pmkrSearch_AddIfNew("mb");

    CMarker_Test::SetInterlinear(pmkrTx, TRUE);
    CMarker_Test::SetInterlinear(pmkrMb, FALSE);

    CInterlinearProcList* pList = ptyp->pintprclst();
    pProc = new CLookupProc(pList, TRUE);
    pProc->SetMarkers(pmkrTx, pmkrMb);
    pList->Add(pProc);

    SUBCASE("Parse 'jumped' into 'jump' and '-ed'") {
        // A: Initialize break characters in the list
        pList->SetMorphBreakChars("-");

        // B: Lexicon entries (stay alive for the call)
        fRoot = new CField(pmkrMb, "jump");
        recRoot = new CRecord(fRoot);
        fSuff = new CField(pmkrMb, "-ed"); // Result content
        recSuff = new CRecord(fSuff);

        // ROOT: key "jump"
        // SUFF: key "ed" (what's in the word), content "-ed" (lexicon field)
        CLookupProc_Test::ManualAddEntry(pProc, ROOT, "jump", fRoot, recRoot);
        CLookupProc_Test::ManualAddEntry(pProc, SUFF, "ed", fSuff, recSuff);

        fSource = new CField(pmkrTx, "jumped nextword");
        recSource = new CRecord(fSource);
        CRecPos rpsStart(0, fSource, recSource);

        int processedLen = 0;
        int resultFlag = 0;

        BOOL bOk = pProc->bInterlinearize(rpsStart, &processedLen, 0, resultFlag);

        CHECK(bOk == TRUE);
        CHECK(processedLen == 9);  // "jump" + space + "-ed" + space

        CField* fResult = recSource->pfldNext(fSource);
        REQUIRE(fResult != nullptr);

        Str8 content = fResult->sContents();
        // The result should look like "jump -ed " or "jumped " with stars if it fails
        INFO("Engine output: [" << content << "]");

        CHECK(content.Find("jump") != -1);
        CHECK(content.Find("-ed") != -1);

        delete recSource;
    }

    SUBCASE("Handle unknown words (Failure Marks)") {
        fSource = new CField(pmkrTx, "unknown next");
        recSource = new CRecord(fSource);
        CRecPos rpsStart(0, fSource, recSource);

        int processedLen = 0;
        int resultFlag = 0;
        pProc->SetShowFailMark(TRUE);
        pProc->SetFailMark("***");

        pProc->bInterlinearize(rpsStart, &processedLen, 0, resultFlag);

        CField* fResult = recSource->pfldNext(fSource);
        REQUIRE(fResult != nullptr);
        INFO("result: [" << fResult->sContents() << "]");
        CHECK(fResult->sContents().Find("unknown") != -1);

        delete recSource;
    }

    // 3. FINAL CLEANUP

    // First, delete our manual lexicon records 
    // Their fields will find their Markers still alive in the Project.
    delete recRoot;
    delete recSuff;

    // Restore the MainWnd
    pApp->m_pMainWnd = pOldWnd;

    // Finally, reset the project (this deletes Markers/Types/Procs)
    CShwApp_Test::ResetProject(pApp);
}