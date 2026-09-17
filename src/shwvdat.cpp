// shwvdat.cpp : implementation of database parts of the CShwView class
//

#include "stdafx.h"
#include "toolbox.h"
#include "shw.h"
#include "project.h"
#include "mainfrm.h"
#include "shwdoc.h"
#include "shwview.h"
#include "kmn.h"
#include "seldb_d.h"
#include "ind.h"
#include "progress.h"
#include "crngset.h"  // CRangeSet
#include "cck.h"  // CFieldConsistencyChecker

#include "browse.h"  //09-26-1997 CBrowseFieldList

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern void ActivateWindow( CMDIChildWnd* pwnd ); // Activate window after jump

void CShwView::FixCursorPosForHidden( BOOL bScrollUp ) // 1.0br Fix cursor position for hidden fields after Database Next, Prev, etc.// 1.5.0fn Scroll up after jump or turn off browse
{
	if ( ptyp()->bHideFields() ) 
		{
		CRecPos rpsTemp = m_rpsCur;
		EditCtrlHome(); // Go to top
		while ( m_rpsCur.pfld && m_rpsCur.pfld != rpsTemp.pfld )
			{
			if ( !bMoveDown( TRUE ) ) // 1.0bv Fix bug of hang on interlinear when some interlinear fields hidden
				break;
			}
		if ( !m_rpsCur.pfld ) // 1.1rb Fix possible crash on setup for hide fields, if m_rpsCur not valid, go back to a known place
			{
			EditCtrlHome(); // Go to top
			return;
			}
		BOOL bSucc = FALSE;
		while ( TRUE ) // 1.0bx Fix bug of not allowing for multi-line fields in fix cursor pos
			{
			bSucc = bMoveDown( TRUE ); // 1.0bx See if it can move down
			if ( !bSucc ) // 1.0bx If it can't move down, then break as non-success
				break;
			if ( m_rpsCur.iChar > rpsTemp.iChar || m_rpsCur.pfld != rpsTemp.pfld ) // 1.0bx If past the desired place, then break as success
				break;
			}
		if ( bSucc ) // 1.0bx If success, then we are one below the desired place, so move back up
			bMoveUp( TRUE );
		m_rpsCur.iChar = rpsTemp.iChar; // 1.0bw Fix bug of interlinearize not moving forward correctly if hidden fields
		}
	if ( bScrollUp ) // 1.5.0fn 
		{
	    CRect rect; // 1.5.0fn 
        GetClientRect( &rect ); // 1.5.0fn 
		long lWindowHeight = rect.bottom - rect.top; // 1.5.0fn 
		int iCaretLocY = m_pntCaretLoc.y; // 1.5.0fn 
		long lPartwayDown = lWindowHeight / 2; // 1.5.0fn 
		if ( iCaretLocY > lPartwayDown ) // 1.5.0fn If caret more than partway down, scroll it up
			{
			if ( ptyp()->bTextFile() && m_pind->pmkrPriKey() == ptyp()->pmkrTextRef() ) // 1.5.0gm If jump to text file ref, place at top of window
				lPartwayDown = 0; // 1.5.0gm 
			long lScroll = iCaretLocY - lPartwayDown; // 1.5.0fn 
			lVScroll( lScroll ); // 1.5.0fn 
			}
		}
}

void CShwView::DatabaseNextRecord()
{
    ASSERT( m_prelCur ); // Should be disabled if no current record
	CNumberedRecElPtr prelStart = m_prelCur; // Remember starting record
    if ( !bValidateCur( dirNext ) )
        return;
	if ( ptyp()->bTextFile() ) // 1.1ky If text and it was already in last record, correct to first ref in record
		if ( prelStart && m_prelCur && ( prelStart->prec() == m_prelCur->prec() ) ) // If it didn't move, use last record code to correct it // 1.4gzn Fix bug of crash on last filtered item gone in text file
			DatabaseLastRecord();
	FixCursorPosForHidden(); // 1.0br Fix cursor pos if hidden fields
//	OnEditPlaysound(); // 1.5.0ae Auto play sound on next and prev record move // 1.5.0ag
	}

void CShwView::DatabasePreviousRecord()
{
    ASSERT( m_prelCur ); // Should be disabled if no current record
    if ( !bValidateCur( dirPrev ) )
        return;
	if ( ptyp()->bTextFile() ) // 1.1ky If text, go back one more and then forward to end at first ref
		{
		CNumberedRecElPtr prelStart = m_prelCur; // Remember starting record
		bValidateCur( dirPrev, FALSE );
		if ( prelStart->prec() != m_prelCur->prec() ) // If it moved to prev record, move to first of next
			bValidateCur( dirNext );
		}
	FixCursorPosForHidden(); // 1.0br Fix cursor pos if hidden fields
//	OnEditPlaysound(); // 1.5.0ae Auto play sound on next and prev record move // 1.5.0ag 
}

void CShwView::DatabaseFirstRecord()
{
    ASSERT( m_prelCur ); // Should be disabled if no current record
        
    // 2000/05/02 TLB 7 MRP - bValidateCur now takes only one argument, which defaults to dirSameRecord
    if ( !bValidateCur(/* eSameRecord, &prel, TRUE */) ) // TLB 4/24/2000 -  - Let bValidateCur set the current rel; then we'll change it
        return;
    CNumberedRecElPtr prel = m_pind->pnrlFirst();

    SetCur(prel); // Set up to display new record       
	FixCursorPosForHidden(); // 1.0br Fix cursor pos if hidden fields
//	OnEditPlaysound(); // 1.5.0ae Auto play sound on next and prev record move // 1.5.0ag 
}

void CShwView::DatabaseLastRecord()
{
    ASSERT(m_prelCur); // Should be disabled if no current record
                
    // 2000/05/02 TLB 7 MRP - bValidateCur now takes only one argument, which defaults to dirSameRecord
    if ( !bValidateCur(/* eSameRecord, &prel, TRUE */) ) // TLB 4/24/2000 -  - Let bValidateCur set the current rel; then we'll change it
        return;
    CNumberedRecElPtr prel = m_pind->pnrlLast();

    SetCur(prel); // Set up to display new record       
	if ( ptyp()->bTextFile() ) // 1.1kh Make database last go to top of record in text file
		{
		CNumberedRecElPtr prelStart = m_prelCur; // 1.1ky Make last record go to first ref in record // Remember starting record
		bValidateCur( dirPrev, FALSE );
		if ( prelStart->prec() != m_prelCur->prec() ) // If it moved to prev record, move to first of next
			bValidateCur( dirNext );
		}
	FixCursorPosForHidden(); // 1.0br Fix cursor pos if hidden fields
//	OnEditPlaysound(); // 1.5.0ae Auto play sound on next and prev record move // 1.5.0ag Undo 1.5.0ae
}

void CShwView::ViewInsertedRecord(CNumberedRecElPtr prel)
{
    m_pind->IncrementRecElNumber(&m_prelUL);

    if (m_bBrowsing) // switch to record view to allow them to fill in record
        SwitchToRecordView(); 
    SetCur(prel); // Set up to display new record       

    HideCaret();
    bEditDown(); // set caret at start of second field if using a template or end of key field if not
    SetCaretPosAdj(); // Move caret
	FixCursorPosForHidden(); // 1.0br Fix cursor pos if hidden fields
    ShowCaret();

    if (m_rpsCur.pfld)
        Shw_pProject()->pkmnManager()->ActivateKeyboard(m_rpsCur.pmkr());

    SetModifiedFlag();
    m_bCanUndoAll = FALSE; // not modified in that sense
        
    CRecordHint hint(prel->prec(), CHint::New);
    GetDocument()->UpdateAllViews(this, 0L, &hint); // Update other views

    CIndexUpdate iup( m_pind->pindsetMyOwner(), prel->prec(), INDEX_UPDATE_ADD );
    extern void Shw_Update(CUpdate& up);        
    // 1996-04-11 MRP: Moved Shw_Update below UpdateAllViews
    // because it causes the m_nrllsts to be emptied.
    Shw_Update(iup);  // Notify interlinear processes that this record changed
}

BOOL CShwView::bDatabaseSearch()
{
    // 1998-02-02 MRP: Moved this code to fix bug #203.
    // Must call sGetCurText before bValidateCur, because the selected text or
    // caret position changes when there's a range set violation and
    // when the record is reindexed.                     
    Str8 sText;
    if ( m_prelCur && !m_bBrowsing && bSelecting(eText) // 1.4wq On search, if selection, use it as default
            && m_rpsCur.pfld->plng() == m_pind->pindsetMyOwner()->pmkrRecord()->plng()) // 1.4yf Don't use selection for search default if wrong language 
        sText = sGetCurText(); // use selected text or word/item at caret position for search
//	sText = ""; // 1.4vxc Remove default from search and insert record edit controls

    // 1995-11-10 MRP: OnSetFocus needs up-to-date m_prelCur
//  CNumberedRecElPtr prel = m_prelCur;
//  if ( !bValidateCur(eSameRecord, &prel) )
    // 2000/05/02 TLB 7 MRP - bValidateCur now takes only one argument, which defaults to dirSameRecord
    if ( !bValidateCur(/*eSameRecord, &m_prelCur*/) )
        return FALSE; // 1.4wp 

    m_keySearch.SetKeyText(sText);

    CNumberedRecElPtr prel = m_prelCur;
    int iResult = m_pind->iSearch(&m_keySearch, &prel, m_pind->pfil() != NULL ); // 1.5.5b Fix bug of inserting duplicate record when filtered
	if ( iResult == CIndex::eCancel ) // 1.4wp Fix bug of moving cursor on cancel of search
		return FALSE; // 1.4wp 

    if (iResult == CIndex::eSearch) // search succeeded
        SetCur(prel); // Set up to display new record       
    else if (iResult == CIndex::eInsert)
        ViewInsertedRecord(prel); // new record was inserted
	FixCursorPosForHidden(); // 1.0br Fix cursor pos if hidden fields
	return TRUE; // 1.4wp 
}

void CShwView::DatabaseInsertRecord()
{
    // 1998-02-02 MRP: Moved this code to fix bug #203.
    // Must call sGetCurText before bValidateCur, because the selected text or
    // caret position changes when there's a range set violation and
    // when the record is reindexed.                     
    Str8 sText;      // use current text only if it is same language as record key
    if (m_prelCur && !m_bBrowsing && bSelecting(eText) // 1.4ye If selection, make it default for insert record
            && m_rpsCur.pfld->plng() == m_pind->pindsetMyOwner()->pmkrRecord()->plng())
        sText = sGetCurText(); // use selected text or word/item at caret position for search
//	sText = ""; // 1.4vxc Remove default from search and insert record edit controls
    
    // 1995-11-10 MRP: OnSetFocus needs up-to-date m_prelCur
//  CNumberedRecElPtr prel = m_prelCur;
//  if ( !bValidateCur(eSameRecord, &prel) )
    // 2000/05/02 TLB 7 MRP - bValidateCur now takes only one argument, which defaults to dirSameRecord
    if ( !bValidateCur(/*eSameRecord, &m_prelCur*/) )
        return;

    m_keyInsert.SetKeyText(sText);

    CNumberedRecElPtr prel = m_prelCur;
//    CIndex *pindSearch = m_pind->pindsetMyOwner()->pindRecordOwner(); // 1.6.1cj Search owning index to: Fix bug of not asking about existing record if not sorted by key
//    int iResult = pindSearch->iInsertRecord(&m_keyInsert, &prel); // 1.6.1cj Fix bug of not asking about existing reoord if not sorted by key
    int iResult = m_pindUnfiltered->iInsertRecord(&m_keyInsert, &prel); // 1.5.5b Fix bug of inserting duplicate record if filtered 
	if ( iResult == CIndex::eCancel ) // 1.5.5b 
		return; // 1.5.5b 
	else // 1.5.5b 
		{
		if ( m_pind->bUsingFilter() ) // 1.5.6a Fix bug of false message if filter already off
			{
			AfxMessageBox(_("Turning off filter.")); // 1.5.5b Warn user that filter is being turned off
			UseFilter( NULL ); // 1.5.5b If user inserted or selected record, turn off filter to show it
			}
		}
    if (iResult == CIndex::eSearch) // user found record already existing
        SetCur(prel); // Set up to display new record       
    else if (iResult == CIndex::eInsert)
        ViewInsertedRecord(prel); // new record was inserted
	FixCursorPosForHidden(); // 1.0br Fix cursor pos if hidden fields
}

void CShwView::DatabaseDeleteRecord()
{
    ASSERT( m_prelCur ); // Should be disabled if no current record
    CRecord* prec = m_prelCur->prec(); // 1.5.1nb 
    CField* pfld = prec->pfldFirst();
	if ( prec->lGetCount() > 10 && ptyp()->bTextFile() ) // 1.5.1nb  // 1.5.3b  // 1.5.3g Allow empty fields in text record
		{
	    Str8 sMessage = _("To delete a text record, first delete its contents"); // 1.5.1nb  // 1.5.3b 
		AfxMessageBox( sMessage ); // 1.5.1nb 
		return; // 1.5.1nb 
		}
	BOOL bWasBrowsing = bBrowsing(); // 1.5.3b 
	if ( bBrowsing() ) // 1.5.3b Make sure not browsing, so highlight will show
		SwitchToRecordView(); // 1.5.3b 
	OnEditSelectAll(); // 1.5.3b Select to highlight for user
    Str8 sMessage = _("Delete record?"); // 1.5.0fg  // 1.5.3b Allow delete record, highlight first
//	sMessage = sMessage + " " + sUTF8( pdoc()->GetPathName() ); // 1.5.0fg  // 1.5.3b 
    if ( AfxMessageBox( sMessage, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 ) != IDYES ) // Verify deletion // 5.98g Change default of delete record to No instead of Yes // 1.5.3b 
		{
		ClearSelection( -1 ); // 1.5.3b Clear selection if record not deleted
	    SetCaretPosAdj(); // Move caret // 1.5.3b 
		Invalidate( FALSE ); // 1.5.3b 
		if ( bWasBrowsing ) // 1.5.3b 
			SwitchToBrowseView(); // 1.5.3b 
        return;
		}
//	OnEditCut(); // 1.5.3b Put content in paste buffer for possible restore // 1.5.9mz Don't put deleted record in paste buffer
    DeleteCurRecord();
	if ( m_pind->bIsEmpty() ) // 1.5.5c Fix bug of crash on remove last record from filter
		{
		m_prelCur = NULL; // 1.5.5c 
		if ( m_pind->pfil() ) // 1.5.5c 
			UseFilter( m_pind->pfil() ); // 1.5.5c 
		}
	FixCursorPosForHidden(); // 1.0br Fix cursor pos if hidden fields
	if ( bWasBrowsing && m_prelCur ) // 1.5.3b Don't try to browse if last record gone
		SwitchToBrowseView(); // 1.5.3b 
	Invalidate( FALSE ); // 1.5.5c 
}

void CShwView::DatabaseCopyRecord(BOOL bDeleteOld)
{
    // TLB 04/05/2000 - We need to get the current record before validation so that we don't lose it.
    ASSERT(m_prelCur);
    CRecord* prec = m_prelCur->prec();
    ASSERT(prec);
    // Make sure we can move/copy without causing problems
    if (!bValidateCur())
        return;
    // Have the user select a database to move or copy to
    CDatabaseRefList drflst(GetDocument(), !bDeleteOld); // 3/26/99 - TLB: For move (i.e., bDeleteOld = TRUE), don't include current DB in list

	CDatabaseRefList* pdrflst = &drflst; // 1.5.3c 
    CDatabaseRef* pdrf = pdrflst->pdrfFirst(); // 1.5.3c 
    for ( ; pdrf; pdrf = pdrflst->pdrfNext(pdrf) ) // only want docs that are currently open // 1.5.3c 
        {
        CShwDoc* pdoc = pdrf->pdoc(FALSE); // 1.5.3c 
        if ( pdoc == m_pdocDefaultForMove ) // 1.5.3c 
			break; // 1.5.3c 
        }
	if ( pdrf && pdrflst->lGetCount() > 1 ) // 1.5.3c  // 1.5.4f Fix bug of crash on move if only one database available
		{
		CDatabaseRef* pdrfDefaultForMove = pdrflst->pdrfRemove( &pdrf ); // 1.5.3c 
		pdrflst->InsertBefore( pdrflst->pdrfFirst(), pdrfDefaultForMove ); // 1.5.3c 
		}

    CShwDoc *pdocTarget;
    CSelectDbDlg dlg(&drflst, &pdocTarget, (bDeleteOld) ? CSelectDbDlg::actMove : CSelectDbDlg::actCopy);
    if ((dlg.DoModal() != IDOK) || !pdocTarget)
        return;
    POSITION posView = pdocTarget->GetFirstViewPosition();
    CShwView *pviewTarget = (CShwView*) pdocTarget->GetNextView(posView);
    if ( !pviewTarget->bValidate() ) // 11-7-97 AB Validate target view to prevent assert
        return; // *** But this still fails. For some reason the view's bModified flag is off by this point even if it has been modified.
    // Copy the record over
    CNumberedRecElPtr prel = pviewTarget->m_pind->prelCopyRecord(prec); // TLB 04/05/2000 - don't use m_prelCur->prec()
    // View the new record.  ViewInsertedRecord does some necessary 
    // processing, but it also takes the view out of browse mode, which we
    // may not want.  (Duplicating ViewInsertedRecord's code here and
    // modifying it is an option, but that did not seem worthwhile.)
    BOOL bWasBrowsing = pviewTarget->bBrowsing();
    pviewTarget->ViewInsertedRecord(prel);
    if (bWasBrowsing)
        pviewTarget->SwitchToBrowseView();
    // Delete the old record, if appropriate.
    if (bDeleteOld)
        DeleteCurRecord();
	FixCursorPosForHidden(); // 1.0br Fix cursor pos if hidden fields
	m_pdocDefaultForMove = pdocTarget; // 1.5.3c Remember choice as default for next
}

CField* pfldFldFoundInRec( CRecord* precDst, CField* pfldSrc ) // 1.5.0ea  // 1.5.0fc 
	{
	for ( CField* pfldDst = precDst->pfldFirst(); pfldDst; pfldDst = precDst->pfldNext( pfldDst ) )
		if ( pfldSrc->pmkr() == pfldDst->pmkr() && pfldSrc->sContents() == pfldDst->sContents() )
			return pfldDst; // 1.5.0fc 
	return NULL;
	}

void MergeRec( CRecord* precDst, CRecord* precSrc, CShwView* pviewSrc, CMarker* pmkrDate, BOOL bMergeEmpty ) // 1.5.0jg Make separate merge record function
	{
	CField* pfldFoundPrev = NULL; // 1.5.0fc Merge insert new field after previous field
	CField* pfldFound = NULL; // 1.5.0fc 
	for ( CField* pfldSrc = precSrc->pfldFirst(); pfldSrc; pfldSrc = precSrc->pfldNext( pfldSrc ) ) // 1.5.0ea For each field in source
		{
		pfldFoundPrev = pfldFound; // 1.5.0fc 
		pfldFound = pfldFldFoundInRec( precDst, pfldSrc ); // 1.5.0fc 
		if ( pmkrDate && pfldSrc->pmkr() == pmkrDate ) // 1.5.0eh Don't merge date field 
			continue;
		if ( pfldSrc->sContents().IsEmpty() && !bMergeEmpty ) // 1.5.0eh Don't merge empty fields // 1.5.0jg Allow exception
			continue;
		if ( pfldSrc->pmkr() == pfldSrc->pmkr()->pmkrsetMyOwner()->pmkrRecord() ) // 1.5.0ka Fix bug (1.5.0jg) of merge single record inserting key field
			continue;
		if ( pviewSrc->bHiddenField( pfldSrc ) ) // 1.5.0ek Don't merge hidden fields
			continue;
		if ( !pfldFound ) // 1.5.0ea If source field not in best match record, add it at end // 1.5.0fc 
			{
			CField* pfldDst = precDst->pfldFirst(); // 1.5.0eh  // 1.5.0ej Fix bug (1.5.0eh) of crash on merge
			for ( ; pfldDst; pfldDst = precDst->pfldNext( pfldDst ) ) // 1.5.0eh  // 1.5.0ej 
				{
				if ( pfldSrc->pmkr() == pfldDst->pmkr() && pfldSrc->sContents().IsEmpty() ) // 1.6.1cn If empty field merging into existing field, skip it
					break; // 1.6.1cn 
				if ( pfldSrc->pmkr() == pfldDst->pmkr() && pfldDst->sContents().IsEmpty() ) // 1.5.0eh If empty field found, put content in it
					break; // 1.5.0eh 
				}
			if ( pfldDst )  // 1.5.0eh If empty field found, put contents in it
				{
				if ( !pfldSrc->sContents().IsEmpty() ) // 1.6.1cn If source is empty, don't merge it
					pfldDst->SetContent( pfldSrc->sContents() ); // 1.5.0eh 
				}
			else // 1.5.0eh Else, no empty field found, add field at end of record
				{
				CField* pfldNew = new CField( *pfldSrc ); // 1.5.0ea Make a copy of the field
				if ( pfldFoundPrev ) // 1.5.0fc If prev field was found in dest, insert after it
					{
					precDst->InsertAfter( pfldFoundPrev, pfldNew ); // 1.5.0fc 
					pfldFound = pfldNew; // 1.5.0fc Remember newly inserted one as place to insert after
					}
				else
					{
					pfldDst = precDst->pfldLast(); // 1.5.0kh 
					if ( pmkrDate && pfldDst->pmkr() == pmkrDate ) // 1.5.0kh If last field in rec is date, merge before it
						precDst->InsertBefore( pfldDst, pfldNew ); // 1.5.0kh 
					else // 1.5.0kh If last field is not date, insert at end
						precDst->Add( pfldNew ); // 1.5.0ea Add it to the end of the destination record
					}
				}
			}
		}
	}

#define RECORDNOMATCH 1000 // 1.5.4a Define for clarity
int iCompareRecForMerge( CRecord* precDst, CRecord* precSrc, CMarker* pmkrDate = NULL ) // 1.5.0ea 
	{
	int iDiff = 0;
	for ( CField* pfldSrc = precSrc->pfldFirst(); pfldSrc; pfldSrc = precSrc->pfldNext( pfldSrc ) ) // For each field in first record, look in second record
		{
		if ( pfldSrc->sContents().IsEmpty() ) // Don't look for empty fields
			continue;
		if ( pmkrDate && pfldSrc->pmkr() == pmkrDate ) // Don't look for date field
			continue;
		BOOL bFldFound = FALSE;
		for ( CField* pfldDst = precDst->pfldFirst(); pfldDst; pfldDst = precDst->pfldNext( pfldDst ) )
			{
			if ( pfldSrc->pmkr() == pfldDst->pmkr() ) // 1.5.4a If same marker, check for same contents
				{
				if ( pfldSrc->sContents() == pfldDst->sContents() ) // 1.5.4a If same contents, field is found
					{
					bFldFound = TRUE;
					break;
					}
				else if ( pfldSrc->pmkr()->sMarker() == "hm" ) // 1.5.4a If different homonymn, don't allow match
					{
					iDiff = RECORDNOMATCH; // 1.5.4a Use high number to indicate no possible match
					return iDiff; // 1.5.4a Return exact number so no confusion in further comparisons
					}
				}
			}
		if ( !bFldFound ) // If field not found, count it as a difference
			iDiff++;
		}
	return iDiff;
	}

void CShwView::DatabaseMergeDb()
{
    // Make sure we can merge without causing problems.  
    if (!bValidateCur())
        return;
    // Have the user select a database to merge from
    CDatabaseRefList drflst(GetDocument());
    CShwDoc *pdocSrc = NULL;
    CSelectDbDlg dlg(&drflst, &pdocSrc, CSelectDbDlg::actMerge);
    if ((dlg.DoModal() != IDOK) || !pdocSrc)
        return;
    POSITION posView = pdocSrc->GetFirstViewPosition();
    CShwView *pviewSrc = (CShwView*) pdocSrc->GetNextView(posView);
    // Copy many records over
    CIndex *pindDst = m_pind->pindsetMyOwner()->pindRecordOwner();
    CIndex *pindSrc = pviewSrc->m_pind->pindsetMyOwner()->pindRecordOwner();
    DWORD dwRecNum = 0;
    CProgressIndicator prg(pindSrc->lNumRecEls(), NULL); // 1.4ad Eliminate resource messages sent to progress bar
    CNumberedRecElPtr prelSrc = pindSrc->pnrlFirst();

    BOOL bWasBrowsing;
    if (m_bBrowsing) {
        SwitchToRecordView();
        bWasBrowsing = TRUE;
    } else
        bWasBrowsing = FALSE;
    // 1999-01-21 MRP: Calling CView::SetModifiedFlag() implies that
    // range set checking would be done on the records as they are merged.
    // But the merge process doesn't check range sets, and so when
    // CShwView::SetCur is called below there is an assertion.
    // The solution: Indicate that the document has been modified,
    // but skip the logic relating to range set validation.
    // SetModifiedFlag();
    GetDocument()->SetModifiedFlag();
    m_bCanUndoAll = FALSE; // not modified in that sense

	BOOL bSmartMerge = TRUE; // 1.5.0ea Make Database, Merge do smart merge
	CMarker* pmkrDate = pdocSrc->ptyp()->pmkrDateStamp();

	if ( (CRecLookEl*)prelSrc != NULL && pindSrc->lNumRecEls() == 1 && prelSrc->pfldPriKey()->sContents() == "" ) // 1.5.0jg If source contains only a single empty record, merge it into all records
		{
		for ( CRecLookEl* prelDst = (CRecLookEl*)m_pind->prelFirst(); prelDst; prelDst = m_pind->prelNext(prelDst) ) // 1.5.0jg For each record in destination // 1.5mb Merge into all records apply to filtered subset
			MergeRec( prelDst->prec(), prelSrc->prec(), pviewSrc, pmkrDate, TRUE ); // 1.5.0jg Merge source record into it
		}
    else
		{
		while ((CRecLookEl*)prelSrc != NULL)  // 1998-12-02 MRP
			{
			CRecord* precBestMatch = NULL; // 1.5.0ea
			int iBestMatchDiff = RECORDNOMATCH; // 1.5.4a High number means records do not match
			if ( bSmartMerge )
				{
				CRecord* precSrc = prelSrc->prec(); // 1.5.0ea 
				for ( CRecLookEl* prelDst = (CRecLookEl*)pindDst->prelFirst(); prelDst; prelDst = pindDst->prelNext(prelDst) )
					{
					if ( prelDst->pfldPriKey()->sContents() == prelSrc->pfldPriKey()->sContents() )
						{
						int iDiff = iCompareRecForMerge( prelDst->prec(), prelSrc->prec(), pmkrDate );
						if ( iDiff < iBestMatchDiff )
							{
							iBestMatchDiff = iDiff;
							precBestMatch = prelDst->prec(); // Remember this as best match
							}
						}
					}
				if ( precBestMatch && iBestMatchDiff > 0 ) // 1.5.0ea If record found, but not exact match, insert fields
					{
					CField* pfldFoundPrev = NULL; // 1.5.0fc Merge insert new field after previous field
					CField* pfldFound = NULL; // 1.5.0fc 
					for ( CField* pfldSrc = precSrc->pfldFirst(); pfldSrc; pfldSrc = precSrc->pfldNext( pfldSrc ) ) // 1.5.0ea For each field in source
						{
						pfldFoundPrev = pfldFound; // 1.5.0fc 
						pfldFound = pfldFldFoundInRec( precBestMatch, pfldSrc ); // 1.5.0fc 
						if ( pmkrDate && pfldSrc->pmkr() == pmkrDate ) // 1.5.0eh Don't merge date field 
							continue;
						if ( pfldSrc->sContents().IsEmpty() ) // 1.5.0eh Don't merge empty fields
							continue;
						if ( pviewSrc->bHiddenField( pfldSrc ) ) // 1.5.0ek Don't merge hidden fields
							continue;
						if ( !pfldFound ) // 1.5.0ea If source field not in best match record, add it at end // 1.5.0fc 
							{
							CField* pfldDst = precBestMatch->pfldFirst(); // 1.5.0eh  // 1.5.0ej Fix bug (1.5.0eh) of crash on merge
							for ( ; pfldDst; pfldDst = precBestMatch->pfldNext( pfldDst ) ) // 1.5.0eh  // 1.5.0ej 
								{
								if ( pfldSrc->pmkr() == pfldDst->pmkr() && pfldDst->sContents().IsEmpty() ) // 1.5.0eh If empty field found, put content in it
									break; // 1.5.0eh 
								}
							if ( pfldDst )  // 1.5.0eh If empty field found, put contents in it
								pfldDst->SetContent( pfldSrc->sContents() ); // 1.5.0eh 
							else // 1.5.0eh Else, no empty field found, add field at end of record
								{
								CField* pfldNew = new CField( *pfldSrc ); // 1.5.0ea Make a copy of the field
								if ( pfldFoundPrev ) // 1.5.0fc If prev field was found in dest, insert after it
									{
									precBestMatch->InsertAfter( pfldFoundPrev, pfldNew ); // 1.5.0fc 
									pfldFound = pfldNew; // 1.5.0fc Remember newly inserted one as place to insert after
									}
								else
									precBestMatch->Add( pfldNew ); // 1.5.0ea Add it to the end of the destination record
								}
							}
						}
					}
				}
			CRecord* precDst = precBestMatch; // 1.5.0ea 
			if ( !precDst ) // 1.5.0ea 
				precDst = pindDst->prelCopyRecord(prelSrc->prec())->prec();
			if ( !precBestMatch || iBestMatchDiff > 0 ) // 1.5.0ea If record inserted or modified, update
				{
				pindDst->IncrementRecElNumber(&m_prelUL);
				if (m_rpsCur.pfld)
					Shw_pProject()->pkmnManager()->ActivateKeyboard(m_rpsCur.pmkr());
				CRecordHint hint(precDst, CHint::New);
				GetDocument()->UpdateAllViews(this, 0L, &hint); // Update other views
				CIndexUpdate iup(pindDst->pindsetMyOwner(), precDst, INDEX_UPDATE_ADD );
				extern void Shw_Update(CUpdate& up);        
				Shw_Update(iup);  // Notify interlinear processes that this record changed
				}

			prelSrc = pindSrc->pnrlNext(prelSrc);
			prg.bUpdateProgress(++dwRecNum);
			}
		}
    // 1999-01-21 MRP: m_prelCur hasn't been changed yet, so when merging
    // into an empty database, when DatabaseFirstRecord() was called below
    // to reset the record, it asserted at Line 59 in shwvdat.cpp.
    // The solution: Get the first record from the current window's index
    // and call SetCur() instead. This code is (now) correct even if the user
    // merges an empty database into an empty database.
    CNumberedRecElPtr prelDst = pindDst->pnrlFirst();
    SetCur(prelDst);
    if ( bWasBrowsing && (CRecLookEl*)prelDst != NULL )
        SwitchToBrowseView();
    // DatabaseFirstRecord();
}

// Deletes the current record and moves to the next record.  This code was
// originally in the DatabaseDeleteRecord method.
void CShwView::DeleteCurRecord()
{
    // 1995-07-14 MRP: Review the following     
    CRecord* prec = m_prelCur->prec();
    m_pind->pindsetMyOwner()->RecordToBeDeleted(prec);
    
    CRecordHint hint(m_prelCur->prec(), CHint::Deleted); // Make hint for record that will be deleted
    CShwDoc* pdoc = GetDocument();
    pdoc->UpdateAllViews(NULL, 0L, &hint); // Move ALL views off the deleted record, including the current one
    
    CIndexUpdate iup( m_pind->pindsetMyOwner(), prec, INDEX_UPDATE_DELETE );
    extern void Shw_Update(CUpdate& up);        
    // 1996-04-11 MRP: Moved this update below UpdateAllViews
    // because it causes the m_nrllsts to be emptied.
    Shw_Update(iup);  // Notify interlinear processes that this record changed

    //delete prec; // delete the actual record
    pdoc->SetModifiedFlag(); // Deletion modifies the document  
    m_bModifiedSinceValidated = FALSE;  // NOT the view's new current record
	FixCursorPosForHidden(); // 1.0br Fix cursor pos if hidden fields
}

void CShwView::UseFilter(CFilter* pfil) // set filter
{
	if ( !m_pind->bIsEmpty() ) // 1.5.5c Fix bug of crash on remove last record from filter
		{
		if ( m_pind->pfil() == pfil ) // selection didn't actually change
			return;
		// 2000/05/02 TLB 7 MRP - bValidateCur now takes only one argument, which defaults to dirSameRecord
		if ( !bValidateCur(/*eSameRecord, &m_prelCur*/) )
			return;
		}

    CIndex* pind = m_pind;
    CIndex* pindUnfiltered = m_pindUnfiltered;
    m_pind->pindsetMyOwner()->GetIndex(pfil, &pind, &pindUnfiltered);

    // 2000/05/01 TLB & MRP - SetIndex calls SetCur directly to reset m_prelCur
    SetIndex(pind, pindUnfiltered /*, &m_prelCur */);
//    SetCur(m_prelCur);
	if ( m_prelCur ) // 1.2dn Fix crash (in 1.2) when hide fields is on and filter shows no records
		FixCursorPosForHidden(); // 1.0br Fix cursor pos if hidden fields
}

void CShwView::DatabaseFiltering()
{
    // 1998-02-03 MRP: THERE'S A SUBTLE BUG HERE
    // If a filter that gets modified is used by *other* non-active windows
    // (perhaps even in other databases), they are not getting validated
    // before the filter update causes them to be reindexed.

    // 1995-11-10 MRP: OnSetFocus needs up-to-date m_prelCur
//  CNumberedRecElPtr prel = m_prelCur;
//  if ( !bValidateCur(eSameRecord, &prel) )
    // 2000/05/02 TLB 7 MRP - bValidateCur now takes only one argument, which defaults to dirSameRecord
    if ( !bValidateCur(/*eSameRecord, &m_prelCur*/) )
        return;

    CIndex* pind = m_pind;
    CIndex* pindUnfiltered = m_pindUnfiltered;
    if ( !CIndexSet::s_bViewFilteringProperties(&pind, &pindUnfiltered) )
        return;

    // 2000/05/01 TLB & MRP - SetIndex calls SetCur directly to reset m_prelCur
    SetIndex(pind, pindUnfiltered /*, &m_prelCur */);
//    SetCur(m_prelCur);
}

void CShwView::DatabaseSorting()
{
    // 2000/05/02 TLB 7 MRP - bValidateCur now takes only one argument, which defaults to dirSameRecord
    if ( !bValidateCur(/*eSameRecord, &m_prelCur*/) )
        return;

    CIndex* pind = m_pind;
    CIndex* pindUnfiltered = m_pindUnfiltered;
    if ( !CIndexSet::s_bViewIndexProperties(&pind, &pindUnfiltered) )
        return;
    // 2000/05/01 TLB & MRP - SetIndex calls SetCur directly to reset m_prelCur
    SetIndex(pind, pindUnfiltered /*, &m_prelCur */);
    
      //09-08-1997
    CBrowseFieldList brflstTemp;
    brflstTemp = m_brflst;   //Store the old one
    if (!m_brflst.bIsEmpty())
        {
        BrowseJustificationDefault();  // Check the PriKeyFields in the Browselist
        RememberUnusualJustificationExceptPriKeyField(&brflstTemp);
        }
    
    // 2000/05/01 TLB & MRP - SetIndex calls SetCur directly to reset m_prelCur
    // 4/24/2000 TLB - Always call SetCur because if there's a non-unique filter, the current
    // record might not match anymore. In fact, we could go from 0 records to n records or vice versa.
//    if (m_bBrowsing)
//        SetCur(m_prelCur);
//    else // RNE, we'll need to set the new key for the status bar's sake.    
//        SetStatusRecordAndPrimaryKey(m_prelCur);
	FixCursorPosForHidden(); // 1.0br Fix cursor pos if hidden fields
}

void CShwView::DatabaseMarkers()
{
    // 1995-08-12 MRP: Validate???
        
    CMarker* pmkr = ( m_prelCur && !m_bBrowsing ? (CMarker*)m_rpsCur.pfld->pmkr() : NULL);
    (void) GetDocument()->pmkrset()->bModalView_Elements(&pmkr);
}


void CShwView::Update(CHint* pHint)
{
    if (pHint != NULL)
        {
        // ??RNE This switch is short now, but it will grow.
        switch(pHint->iObjectType()) // Type of operation
            {   
            case CHint::Record:
                UpdateRecordHint((CRecordHint*)pHint);
                break;
           
            default:
                ASSERT(FALSE); // ERROR: Not yet processing these hints
                break;
            }   // end switch(pHint->iObjectType())
        } 
    else // pHint == NULL
        // 1995-07-06 MRP: Should really use 
        Invalidate(FALSE);  // keyboard input to a record window
}


void CShwView::UpdateRecordHint(CRecordHint* phRec)
{
    BOOL bCurRecUpdated = ( m_prelCur && m_prelCur->prec() == phRec->prec() );
    
    switch (phRec->iOperation())
        {
        case CHint::New: // Called only for other than current view
            if ( !m_prelCur ) // If record set was empty !!Also must allow for browse
                // If something in record set now, make it appear
                SetCur(m_pind->pnrlFirst()); // Set up to display new record        
            else
                {
                m_pind->IncrementRecElNumber( &m_prelCur );
                if (m_bBrowsing)
                    {
                    m_pind->IncrementRecElNumber( &m_prelUL );
                    SetCur(m_prelCur, FALSE);
                    }
                }
            break;
        
        case CHint::Deleted: // Called for current view as well as others
            {
            CNumberedRecElPtr prelCur = m_prelCur;
            if ( bCurRecUpdated ) // If current record is being deleted, move off it !!Also allow for browse
                {
                // Move off the deleted record.
                prelCur = m_pind->prelDifferentRecord(prelCur);
                if ( !prelCur ) // If moved off top or bottom, give status message
                    {
                    if (m_pind->bUsingFilter()) // If filtering, give filter message
                        MessageStatusLine(_("")); // 1.5.0ha 
                    else        
                        MessageStatusLine(_("")); // 1.5.0ha 
                    }
                m_bModifiedSinceValidated = FALSE; // Does not need validation
                }
            if ( prelCur )
                {
                if (m_bBrowsing)
                    {
                    if ( m_prelUL && m_prelUL->prec() == phRec->prec() )
                        m_prelUL = m_pind->prelDifferentRecord(m_prelUL);
                    m_pind->DecrementRecElNumber( &m_prelUL );
                    }
                m_pind->DecrementRecElNumber( &prelCur );
                }
            else // no current record element : empty database or filter.
                {
                // Cause m_prelUL to reflect empty condition.
                if ( m_bBrowsing && m_prelUL && m_prelUL->prec() == phRec->prec() )
                    m_prelUL = m_pind->prelDifferentRecord(m_prelUL);
                }
            if ( bCurRecUpdated || m_bBrowsing)
                SetCur(prelCur, FALSE); // Set up to display new record, don't set m_prelUL
            else
                {
                // 2000-09-06 TLB & MRP: When deleting a record other than the current one in another
                // view, we need to reset the m_prelCur to reflect the modified count.
                m_prelCur = prelCur;
                }
            break;  // End of   case CHint::recordDeleted
            }
            
        case CHint::Modified:
            {
            if (m_bBrowsing)
                {
                Invalidate(FALSE); // redraw without erase
                return;
                }

            if (!bCurRecUpdated) // don't do anything if current record not changed
                break;
                
            switch (phRec->iRedraw()) // how much needs redrawn?
                {
                case CRecordHint::eAll:
                    Invalidate(FALSE);
                    break;
                case CRecordHint::eLine:
                    InvalidateLineContaining(phRec->rps(), FALSE);
                    break;
                case CRecordHint::eLineErase:
                    InvalidateLineContaining(phRec->rps(), TRUE);
                    break;
                }
            break;
            }
            
        case CRecordHint::Validated:
            {
            BOOL bWasUpdated = FALSE;
            CNumberedRecElPtr prel = m_prelCur;
            if (prel)
                m_pind->IncrementRecElNumber( &prel );

            if ( bCurRecUpdated )
                {
                // Whenever the current record has been validated successfully,
                // its sort key and position in the index must be updated.
                m_bModifiedSinceValidated = FALSE;
                // NOTE!!! Come back and rethink this when filtering is implemented
                bWasUpdated = m_pind->bRecElUpdated(&prel);
                }
                
            if ( prel )
                {
                if ( m_bBrowsing )
                    {
                    m_pind->IncrementRecElNumber( &m_prelUL );
                    m_prelUL = m_pind->prel_UpToDate( m_prelUL );
                    m_pind->DecrementRecElNumber( &m_prelUL );
                    }
                m_pind->DecrementRecElNumber( &prel );
                }
            // 2000/05/03 TLB & MRP -
            // If we previously had no current record element or the record elements previously
            // available are no longer in the index (because of filtering or deletion), but other
            // updates or additions make it so the index will not be empty after applying updates,
            // we need to get a new current record element. We'll choose the first one that isn't
            // going to be deleted.
            else if ( (m_pind->lNumRecEls() - m_pind->lNumRecElsToBeDeleted() > 0) )
                {
                prel = m_pind->pnrlFirst();
                prel = m_pind->prel_UpToDate( prel );
                m_pind->DecrementRecElNumber( &prel );
                bWasUpdated = TRUE;
                if ( m_bBrowsing )
                    m_prelUL = prel;
                }

            // RNE This is a problem for different views where we're on a different
            // record. It will still need to have its m_prelCur set because the record
            // number of that element can and will be affected. For instance, if I insert
            // a rec el prior to this one, it's number will increase. But as you can
            // see in the first line of this member function, bWasUpdated is TRUE if
            // only we're dealing with the same record as the one that was modified.
            // Therefore, we'll assign the new numbered record element to m_prelCur so that
            // the status bar will be updated to correctly reflect the new number.
            if ( bWasUpdated || m_bBrowsing )
                SetCur(prel, FALSE); // FALSE means don't center current rec in browse, can't set m_prelUL
            else
                m_prelCur = prel;
            break;
            }
        default:
            ASSERT(FALSE); // ERROR: Not processing this operation
            break;
        }   // end switch
}           // RNE This is a problem for different views where we're on a different
            // record.  It will still need to have its m_prelCur set because the record
            // number of that element can and will be affected.  For instance, if I insert
            // a rec el prior to this one, it's number will increase.  But as you can
            // see in the first line of this member function, bWasUpdated is TRUE if
            // only we're dealing with the same record as the one that was modified.



void CShwView::Update(CUpdate& up)
{
    up.UpdateShwView(this);
}

void CShwView::MarkerUpdated(CMarkerUpdate& mup)
{
    if ( !mup.bModified() )
        return;
    if ( !m_prelCur )
        return;
        
    if ( m_pind->bMarkerUpdateRequiresReSorting(mup) )
        UpdateRequiredReSorting();
        
    //11-03-1997
    CBrowseFieldList brflstTemp;
    brflstTemp = m_brflst;   //Store the old one
    if (!m_brflst.bIsEmpty())
        {
        BrowseJustificationDefault();  // Check the PriKeyFields in the Browselist
        RememberUnusualJustificationExceptPriKeyField(&brflstTemp);
        }
    
    CMarker* pmkr = mup.pmkr();
    CRecord* prec = m_prelCur->prec();
    CField* pfld = prec->pfldFirstWithMarker(pmkr);
    if ( pfld )  // marker occurs in the current record
        Invalidate(FALSE);
    // NOTE NOTE NOTE: We are no longer considering a modification
    // to a marker to be a modification to the *view*, since that mechanism
    // seems really to be concerned with the _content_ of the fields.
    // The marker update is also sent to the document, which may be modified.
        
    int iOldHt = m_brflst.iLineHt();
    m_brflst.RecalcLineHt(); // user may have changed font size of a marker
    if ( m_bBrowsing && iOldHt != m_brflst.iLineHt() ) // if it affects line height in browse
        BrowseSetScroll( TRUE ); // recalculate for scroll bars, etc.

    // In any case, invalidate the status bar also.
    //UpdateStatusBar();
}

void CShwView::FilterUpdated(CFilterUpdate& fup)
{
    if ( m_pind->pfil() != fup.pfil() )
        return;
        
    if ( fup.bToBeDeleted() )
        {
        // Find the corresponding element in the unfiltered index.
        // 2000/05/01 TLB & MRP - SetIndex calls SetCur directly to reset m_prelCur (note: this code is probably never called)
        SetIndex(m_pindUnfiltered, m_pindUnfiltered /*, &m_prelCur */);
        //SetCur(m_prelCur);
        }
    else
        {
//        ASSERT( fup.bModified() );
        if ( m_pind->bFilterUpdateRequiresReFiltering(fup) )
            {
//          CRecLookEl* prel = NULL;
            CNumberedRecElPtr prel = NULL;  // 1996-07-31 MRP
            if ( m_prelCur )
                prel = m_pind->prelCorrespondingTo(m_prelCur);
            if ( !prel )
                prel = m_pind->pnrlFirst();
            SetCur(prel);
            }
        }
  
}

void CShwView::LangEncUpdated(CLangEncUpdate& lup)
{
    if ( !lup.bModified() ) // If an Add or Delete, can't affect the index (which does not exist when adding a missing language upon open)
        return;

      //11-03-1997
    CBrowseFieldList brflstTemp;
    brflstTemp = m_brflst;   //Store the old one
    if (!m_brflst.bIsEmpty())
        {
        BrowseJustificationDefault();  // Check the PriKeyFields in the Browselist
        RememberUnusualJustificationExceptPriKeyField(&brflstTemp);
        }
    
    m_brflst.RecalcLineHt(); // In case the font height changed
    if ( m_pind->bLangEncUpdateRequiresReSorting(lup) )
        UpdateRequiredReSorting();
}

void CShwView::SortOrderUpdated(CSortOrderUpdate& sup)
{
    if ( m_pind->bSortOrderUpdateRequiresReSorting(sup) )
        UpdateRequiredReSorting();
}

void CShwView::UpdateRequiredReSorting()
{
    // 2000/04/28 TLB & MRP - Function re-written to account for filtering
    // This view's index has been resorted (and possibly refiltered), therefore the element(s)
    // it shows must be re-synched.
    CNumberedRecElPtr prel = NULL;
    // If we had a current rel before, try to find corresponding element in updated index.
    if ( m_prelCur )
        prel = m_pind->prelCorrespondingTo(m_prelCur); // This returns NULL if no corresponding element
    if ( !prel )
        prel = m_pind->prelFirst();
    SetCur(prel);
}


void CShwView::SetModifiedFlag()
{
    GetDocument()->SetModifiedFlag();
    m_bModifiedSinceValidated = TRUE;
    m_bCanUndoAll = m_pszUndoAllBuffer != NULL;
}

/* 2000/05/02 TLB & MRP -- Caller never sets current. dirSameRecord is default
BOOL CShwView::bValidateCur()
{
    return bValidateCur(eSameRecord, &m_prelCur, FALSE);
}
*/

BOOL CShwView::bValidateCur( EDirection dir, BOOL bEditing )
{
	CNumberedRecElPtr prelStart = m_prelCur; // 1.1ef Remember starting record
    if ( !bValidate(TRUE, dir) ) // validate current view first
        return FALSE;
	if ( ptyp()->bTextFile() && dir != dirSameRecord // 1.1ef If text file, move until next record is found
		&& ( !( (m_prelCur == pind()->pnrlFirst() && dir == dirPrev )
				|| (m_prelCur == pind()->pnrlLast() && dir == dirNext ) ) ) )
		{
		while ( m_prelCur->prec() == prelStart->prec() ) // 1.1ef While still in same record
			{
			CNumberedRecElPtr prelCurStart = m_prelCur;
		    if ( !bValidate(bEditing, dir) // 1.1ef Move to next index element
					|| m_prelCur == prelCurStart ) // 1.1ef Test for no move
		        break;
			}
		}
    
    CMDIChildWnd* pwnd = Shw_pmainframe()->MDIGetActive(); // remember active child window

    if ( pdoc()->bValidateViews(TRUE, this) ) // validate all other views of this doc
        {   
        if ( pwnd != Shw_pmainframe()->MDIGetActive() ) // was another view activated because of validation problem?
            Shw_pmainframe()->MDIActivate(pwnd); // re-activate current view
        return TRUE;
        }
    else
        return FALSE; // another view failed the test
}

/* 2000/05/02 TLB & MRP -- Caller never sets current. dirSameRecord is default
BOOL CShwView::bValidate()
{
    return bValidate(TRUE, eSameRecord, &m_prelCur, FALSE);
}
*/

BOOL CShwView::bValidate(BOOL bEditing,
        EDirection dir /*, CNumberedRecElPtr* pprel,
        BOOL bCallerWillSetCur */)
{
    CNumberedRecElPtr prel = m_prelCur;
    if ( !prel )
        return TRUE;
    ASSERT(!m_pind->bIsEmpty());
    if ( !bModifiedSinceValidated() )
        {
        if ( dir != dirSameRecord )
            {
            if ( dir == dirNext )
                prel = m_pind->pnrlNext(prel);
            else
                {
                ASSERT(dir == dirPrev);
                prel = m_pind->pnrlPrev(prel);
                }
            if ( prel )
                SetCur(prel);
            else if ( bEditing )
                MessageBeep(0);
            }
        return TRUE;
        }
    
    ASSERT( !m_bBrowsing );
        
    CRecord* prec = prel->prec();
        
    // Validate the record's contents against any Range Sets, Referential Consistency paths, etc.
    // TLB - 07/29/1999 : CRecord::bCheckConsistency replaces CShwView::bValidateRangeSet
    BOOL bCheckConsistency = bEditing && Shw_pProject()->bCheckConsistencyWhenEditing();
    if ( bCheckConsistency && !prec->bCheckConsistency( m_prelCur, this ) )
        return FALSE;

    m_bModifiedSinceValidated = FALSE;
    
    CMarker* pmkrDateStamp = GetDocument()->ptyp()->pmkrDateStamp();
    if ( pmkrDateStamp )
        m_prelCur->prec()->ApplyDateStamp( pmkrDateStamp ); // freshen or add datestamp field
    
    // Update the sort keys of this record in all indexes of this database.
    // Phase I-- Add record elements which correspond to added or modified
    // primary key fields. Return a list of those record elements which
    // correspond to modified or deleted fields.
    (void) m_pind->pindsetMyOwner()->bUpdateSortKeys(prel);

    // Phase IIa-- Cause any other views of this document which are showing
    // this same record to clear their modified flags and to ensure that
    // their current record element is up-to-date.
    CRecordHint hint(prec, CRecordHint::Validated);
    GetDocument()->UpdateAllViews(this, 0L, &hint);

    // Phase IIb-- Ensure that this view's current element is up-to-date.
    m_pind->IncrementRecElNumber( &prel );

    if ( dir == dirSameRecord )
        (void) m_pind->bRecElUpdated(&prel);    
    else if ( dir == dirNext )
        prel = m_pind->prelNext_UpToDate(prel);
    else if ( dir == dirPrev )
        prel = m_pind->prelPrev_UpToDate(prel);

    m_pind->DecrementRecElNumber( &prel );

    if ( prel != m_prelCur )
        {
        // 1999-10-22 TLB & MRP: This code fixes a subtle bug that has existed
        // for a long time but did not often rear its ugly head in release
        // versions. If problems are found with this change or other changes
        // are required in this code, test the first, prev, next, and last
        // functions carefully. Specifically, use this scenario to attempt to
        // recreate the bug (guaranteed to fail in debug; might fail in release);
        // 1) Open two views of the same database, displaying different records,
        //    sorted on the record marker
        // 2) Modify a field (not the record marker field) in window #1
        // 3) Modify the record marker in window #2
        // 4) Use the First, Previous, Next, or Last function
        // 5) This bug caused an unhandled exception in
        //    CDblList::pelFirst or CDblList::pelNext
        //    called from CDblList::bIsMember
        //    called from CShwView::bPositionValid
        //    called from CShwView::OnUpdate
        if ( dir != dirSameRecord )
            {
            m_pind->IncrementRecElNumber(&m_prelCur);
            (void) m_pind->bRecElUpdated(&m_prelCur);
            m_pind->DecrementRecElNumber(&m_prelCur);
            // 4/24/2000 TLB - Move this code here to distinguish between last/first record
            // and records which no longer match filter.
            if ( prel || (m_pind->lNumRecEls() - m_pind->lNumRecElsToBeDeleted() == 0) )
                SetCur(prel); // Set up to display new record
            if ( !prel && bEditing)
                MessageBeep(0);
            }
        else
            {
            SetCur(prel);
            }
        }

    // Phase III-- Now that we are sure that no view has an obsolete
    // current record element, delete obsolete elements from all indexes.
    CIndexUpdate iup( m_pind->pindsetMyOwner(), prec, INDEX_UPDATE_MODIFY );
    extern void Shw_Update(CUpdate& up);        
    // 1996-04-11 MRP: We must keep Shw_Update below UpdateAllViews
    // because it causes the m_nrllsts to be emptied.
    Shw_Update(iup);  // Notify interlinear processes that this record changed
    return TRUE;
}

#ifdef TLB_199_07_29 // Moved (and renamed) function to CRecord
//08-20-1997 Changed the whole Function in order to add the new range set features
BOOL CShwView::bValidateRangeSet( CRecord* prec )
{
    // 1999-06-03 MRP
    ASSERT( prec );
    CField* pfld = prec->pfldFirst();
    for ( ; pfld; pfld = prec->pfldNext(pfld) )
        {
        CMarker* pmkr = pfld->pmkr();
        if ( pmkr->bRangeSetEnabled() &&
                !pmkr->prngset()->bConsistent(pfld, m_prelCur, this) )
            return FALSE;
        // 1999-07-29 TLB
        if ( 
        }
    ASSERT( !pnote );
    return TRUE;
}
#endif  // TLB_1999_07_29

void CShwView::UpdateIndexes() // If current record modified, update indexes
{
// 1998-02-04 MRP: Calling CShwView::UpdateIndexes from CShwViewFrame::OnMDIActivate
// appears to cause bug #127 "Range set on jump not checked".
// I think we can remove both these functions.
// Jump, Adapt, Interlinearize, and Spell Check all call ValidateAllViews
// and this step gets in the way by incorrectly clearing bModifiedSinceValidated.

//    if ( bModifiedSinceValidated() )
//        GetDocument()->RecordModified( m_prelCur );
}

void CShwView::SetCurAndCaret(const CNumberedRecElPtr& pnrl,
        CField* pfld, int iChar, int lenSelection)
{
    ASSERT( pnrl );
    ASSERT( pfld );
    ASSERT( 0 <= iChar );
    ASSERT( iChar <= pfld->GetLength() );
    ASSERT( iChar + lenSelection <= pfld->GetLength() );

    // If the view is minimized, then restore it to normal.
    CMDIChildWnd* pwnd = pwndChildFrame();
    WINDOWPLACEMENT wplstruct;
    wplstruct.length = sizeof(wplstruct);
    pwnd->GetWindowPlacement(&wplstruct);
    BOOL bIsActive = ( pwnd == Shw_pmainframe()->MDIGetActive() );
    if ( !bIsActive ) // validating non-active view
        Shw_pmainframe()->MDIActivate(pwnd); // bring window to front for user to see
    if ( wplstruct.showCmd == SW_SHOWMINIMIZED )
        {
        wplstruct.showCmd = SW_SHOWNORMAL;
        pwnd->SetWindowPlacement(&wplstruct);
        }

    // If the view is in browse mode, change it to record mode.
    if ( m_bBrowsing )
        SwitchToRecordView();

    if ( pnrl != m_prelCur )
        SetCur(pnrl);

    UpdateStatusBar(Shw_pbarStatus());

    // TLB 1999-11-05 : Make sure there is no existing selection if lenSelection == 0
    // because we don't want Shoebox to accidentally extend the selection.
    if ( !lenSelection && bSelecting(eAnyText) )
        {
        m_iSelection = 0;
        Invalidate(FALSE);
        }

    EditSetCaret(pfld, iChar, lenSelection);
}

void CShwView::ReplaceSelectedInconsistency(const Str8& sReplacement)
{
    ASSERT( m_prelCur );
    // TLB - 09/30/99 - In case of Must-have-data error, the "selection"
    // will be an empty field, which doesn't qualify as a selection.
    CRecPos rps;
    if ( bSelecting( eAnyText ) )
        {
        rps = rpsSelBeg();
        DeleteSelection();
        }
    else
        rps = m_rpsCur;
    CField* pfld = rps.pfld;
    ASSERT( pfld );
    int iChar = rps.iChar;

    // These two functions both call Invalidate and SetModifiedFlag.
    // The flag is cleared when either pnrlNextToCheck or
    // ConsistencyCheckingCancelled is called (see below).
    // DeleteSelection(); <--- Done above
    EditInsertWord(sReplacement, FALSE);  // Don't pad with white space

    // Select the replacement in the active window.
    // Causes the data field to be rechecked.
// TLB : Not needed 10/11/99    EditSetCaret(pfld, iChar, sReplacement.GetLength());

    // Update any other windows that display this data field.
    UpdateBasedOnChange();  // 1999-06-26 MRP
}

CNumberedRecElPtr CShwView::pnrlNextToCheck(CNumberedRecElPtr pnrl)
{
    // 2000-08-18 TLB & MRP : For completeness' sake.
    ASSERT(pnrl);

    if ( !m_bModifiedSinceValidated )
        return m_pind->pnrlNext(pnrl);

    CRecord* prec = pnrl->prec();

    ASSERT(pnrl == m_prelCur); // This is true because we only get here if an inconsistency was found
    // 2000/05/02 TLB & MRP - bValidate function always operates on m_prelCur and calls SetCur
    BOOL b = bValidate(FALSE, dirNext);
    ASSERT( b );
    ASSERT( !m_bModifiedSinceValidated );  // Cleared by bValidate

    // 2000-08-16 TLB : If bValidate sets the m_prelCur to the same pnrl, we're at the last record
    // and we should return NULL (and stop checking).
    if ( m_prelCur == pnrl )
        {
        // Make sure we're really at the end.
        ASSERT(m_pind->prelNext(pnrl) == NULL);
        // Make sure that we haven't recycled a record element for a different record. The index update
        // framework should guarantee this.
        ASSERT(m_prelCur->prec() == prec);
        return NULL;  // At the end of the database
        }

    return m_prelCur;
}

void CShwView::ConsistencyCheckingCancelled(CNumberedRecElPtr pnrl)
{
    if ( !m_bModifiedSinceValidated )
        return;

    // 2000-08-18 TLB & MRP : Don't ASSERT the following because user may have presses Insert button.
    if (pnrl != m_prelCur) // As a result of a data link error, user inserted a new record into the current window.
        return; // No need to do anything because this function was already called for the pnrl.

//    ASSERT(pnrl == m_prelCur); // This is true because we only get here if an inconsistency was found
    // 2000/05/02 TLB & MRP - bValidate function always operates on m_prelCur and calls SetCur
    BOOL b = bValidate(FALSE, dirSameRecord);
    ASSERT( b );
    ASSERT( !m_bModifiedSinceValidated );  // Cleared by bValidate
}

void CShwView::CheckConsistency()
{
    if ( !Shw_papp()->bValidateAllViews() ) // valid everyone, including possible jump target!
        return;

    if ( !m_pcck )
        m_pcck = new CFieldConsistencyChecker(this);

    // If the user chose Cancel when an inconsistency was found,
    // checking continues from the data field containing either
    // the beginning of a selection or the insertion point.
    CField* pfldCur = NULL;
    if ( bSelecting(eAnyText) )
        pfldCur = rpsSelBeg().pfld;
    else
        pfldCur = m_rpsCur.pfld;

    BOOL bConsistent = m_pcck->bCheckConsistency(m_pind, m_prelCur, pfldCur);
}

void CShwView::RecheckConsistency()
{
    // 2000/05/02 TLB 7 MRP - bValidateCur now takes only one argument, which defaults to dirSameRecord
    if ( !bValidateCur(/*eSameRecord, &m_prelCur*/) )
        return;
    if ( !m_pcck )
        m_pcck = new CFieldConsistencyChecker(this);

    // Start checking from the first record in the index, even (especially)
    // if the user chose Cancel when an inconsistency was found.
    BOOL bConsistent = m_pcck->bRecheckConsistency(m_pind);
}
