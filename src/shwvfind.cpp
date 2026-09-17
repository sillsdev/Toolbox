// shwvfind.cpp : implementation of find and replace parts of the CShwView class 
//

#include "stdafx.h"
#include "toolbox.h"
#include "project.h"
#include "shwdoc.h"
#include "shwview.h"
#include "status.h"
#include "progress.h"
#include "find_d.h"
#include "kmn.h"
#include "loc.h"  // CMStringLoc

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

//------------------------------------------------------
// BJY 10/1/96  Why I added the CRecPtrList class...
// There appears to be no reasonable way to use a normal index (even an owning index)
// to walk thru a database for a replace or replace all operation. A replace operation can
// change a sort field (including the record key) resulting in possible movement of the
// current record element within the index being used. This can result in records being
// skipped or even possible bad behaviour from elements dropping out of a filtered index.
// Instead we build a list of record pointers from the current index before any modifications
// take place.  Using this list we don't have to worry about things moving, etc. - the
// record pointer list stays constant even though all our indexes may be changing. This also
// eliminates the problem of searching a record multiple times if it has multiple sort fields.

class CRecPtr : public CDblListEl // Hungarian: rcp
{
private:
    CRecord* m_prec; // pointer to a record

    friend class CRecPtrList;

    CRecPtr( CRecord* prec )
        { ASSERT( prec ); m_prec = prec; }
public:
    CRecord* prec() { return m_prec; }
};
    
class CRecPtrList : public CDblList // Hungarian: rcplst
{
public:
    CRecPtrList() {}
    ~CRecPtrList() {}
    
    CRecPtr* prcpFirst() const  // first (key) field
        { return (CRecPtr*) pelFirst(); }
    CRecPtr* prcpNext( const CRecPtr* prcpCur ) const   // field after current
        { return (CRecPtr*) pelNext( prcpCur ); }

    void Add( CRecord* prec ) // Insert rec ptr at end
        { CDblList::Add( new CRecPtr( prec ) ); }

    BOOL bContains( CRecord* prec ) // return TRUE if prec already in list
        {
        for ( CRecPtr* prcp = prcpFirst(); prcp; prcp = prcpNext( prcp ) )
            if ( prcp->prec() == prec )
                return TRUE;
        return FALSE;
        }
};

CRecPtrList* CShwView::prcpBuildList( BOOL bIsReplaceAll )
{
    CRecPtrList* prcplst = new CRecPtrList;

    BOOL bEliminateDups = m_pind->pmkrPriKey() != m_pind->pindsetMyOwner()->ptyp()->pmkrRecord();

    CRecLookEl* prel;
    if ( bIsReplaceAll )
        prel = m_pind->pnrlFirst(); // start at top and do all records
    else
        {
        // 08-27-1997
        // To be sure that all items becomes replaced -> Set the caret to the 
        // begin of the first field in the current record.
        m_rpsCur.pfld = m_prelCur->prec()->pfldFirstWithMarker(m_rpsCur.pfld->pmkr());
        EditSetCaret( m_rpsCur.pfld, 0);
        prel = m_prelCur;
        }
    for ( ; prel != NULL; prel = m_pind->pnrlNext( prel ) ) // walk thru index
        {
        if ( !bEliminateDups || !prcplst->bContains( prel->prec() ) ) // don't add a record twice
            prcplst->Add( prel->prec() ); // add record ptr to list
        }

    //08-28-1997 The replace operation should go through the whole database
    //Add the record exitisting for the current one to the List
    if (!bIsReplaceAll)
        // Do it only if the user choose replace (not replace all!)
        {
        prel = m_pind->pnrlFirst();  //Start at the begin
        for(;prel != m_prelCur; prel = m_pind->pnrlNext( prel ) )  //walk through until you reach the current one
            {
            if ( !bEliminateDups || !prcplst->bContains( prel->prec() ) ) // don't add a record twice
            prcplst->Add( prel->prec() ); // add record ptr to list
            }
        }

    m_prcpCur = prcplst->prcpFirst();
    return prcplst;
}

CNumberedRecElPtr CShwView::prelNext( CNumberedRecElPtr prel ) // get prel of next record to do find or replace operation on
{
    if ( m_prcplst )
        {
        if ( m_prcpCur )
            m_prcpCur = m_prcplst->prcpNext( m_prcpCur ); 
        else
            m_prcpCur = m_prcplst->prcpFirst();

        if ( !m_prcpCur )
            return NULL;
        else
            {
            CNumberedRecElPtr prelT = m_pind->prelFind( m_prcpCur->prec() );
            return prelT ? prelT : prelNext( prelT ); // handle record dropping from index because of non-unique filter
            }
        }
    else
        return m_pind->pnrlNext( prel ); // not using record list
}


//------------------------------------------------------
BOOL CShwView::bFindInField( CFindSettings* pfndset, CPatMChar* ppat, CRecPos& rps, int* piLen, BOOL bForward )
{
    // Move forward through the field using a marked string location
    BOOL bMatched = FALSE;
    CPatMCharMatch mat(pfndset->matsetCur());  // 1999-08-13 MRP
    CMStringLoc loc(rps.pfld, FALSE, rps.iChar);
    Length len = 0;
    while ( !loc.bAtEnd() )
        {
        const CMChar* pmch = NULL;
        int iMatLen = ppat->iMatchAt(loc, mat);

        if ( iMatLen && pfndset->bMatchWhole() )
            {
            if ( len ) // len should be non-zero if not at start of string
                {
                rps.iChar = loc.iPos() - len;
                if ( rps.bIsWdChar() ) // is there a word formation character preceding match?
                    iMatLen = 0; // signal match no good
                }
            if ( iMatLen )
                {
                rps.iChar = loc.iPos() + iMatLen; // examine char following match
                if ( rps.bIsWdChar() ) // shouldn't be a word formation char here either...
                    iMatLen = 0; // signal match no good
                }
            }

        if ( iMatLen )
            {
            // We want to know the starting offset and length
            rps.iChar = loc.iPos();  
            *piLen = iMatLen; // return match length to caller
            return TRUE;
            }
        loc.bMChar_NextIncluded(&pmch, &len, pfndset->matsetCur());  // 1999-08-13 MRPb
        }
    
    return FALSE;
}
//------------------------------------------------------
BOOL CShwView::bFindInRec( CFindSettings* pfndset, CPatMChar* ppat, CRecPos& rps, int* piLen, BOOL bForward )
{
    CMarker* pmkr = NULL;
    CLangEnc* plng = NULL;
    if ( pfndset->bSingleField() ) // search only fields based on specified marker
        {
        pmkr = ptyp()->pmkrset()->pmkrSearch( pfndset->pszMkr() );
        if ( !pmkr ) // marker doesn't exist in marker set for current view
            return FALSE;
        }
    else // search fields based on specified language
        plng = pfndset->plng( this );
        
    CField* pfld = rps.pfld;
    while ( pfld ) // while not at end of record
        {
        while ( ( pmkr && pmkr != pfld->pmkr() ) // look for field using pmkr
                || ( plng && plng != pfld->pmkr()->plng() ) ) // or field of plng
            {
            if ( bForward )
				do
					pfld = rps.prec->pfldNext( pfld );
				while ( pfld && bHiddenField( pfld ) ); // 1.0bu Don't find in hidden fields
            else
				do
	                pfld = rps.prec->pfldPrev( pfld );
				while ( pfld && bHiddenField( pfld ) ); // 1.0bu Don't find in hidden fields
            if ( !pfld ) // at end of record
                return FALSE;
            }

        if ( ppat->bSkip() )  // 1996-07-29 MRP
            {
            // Skip matches the entire field, even if it's empty
            rps.SetPos(0, pfld);
            *piLen = pfld->GetLength();
            return TRUE;
            }
        else if ( pfld != rps.pfld ) // need to resync rps to pfld
            rps.SetPos( bForward ? 0 : pfld->GetLength(), pfld );

        if ( bForward )
            {
            if ( bFindInField( pfndset, ppat, rps, piLen, bForward ) ) // any matches in this field?
                return TRUE;
            }
        else
            { // searching backward thru field
            int iEnd = rps.iChar;
            rps.iChar = 0;
            if ( bFindInField( pfndset, ppat, rps, piLen, bForward )  // any matches in this field?
                && rps.iChar < iEnd )
                {
                int iLastPos = rps.iChar; // remember where it was
                int iLastLen = *piLen;
                rps.bMove();
                while ( bFindInField( pfndset, ppat, rps, piLen, bForward )
                        && rps.iChar < iEnd )
                    {
                    iLastPos = rps.iChar; // remember where it was
                    iLastLen = *piLen;
                    rps.bMove(); // step to next (meta)character
                    }
                rps.iChar = iLastPos;
                *piLen = iLastLen;
                return TRUE;
                }
            }

        if ( bForward )
			do
				{
		        pfld = rps.pfldNext(); // goto next field
				if ( pfld )
					{
					rps.pfld = pfld; // 1.0bu Don't find in hidden fields
					rps.iChar = 0; // 1.0dh Fix bug of not starting at beginning of next field
					}
				}
			while ( pfld && bHiddenField( pfld ) ); // 1.0bu Don't find in hidden fields
        else
			do
				{
	            pfld = rps.pfldPrev(); // goto previous field
				if ( pfld ) // 1.0dk Fix bug of crash at top of record
					{
					rps.pfld = pfld; // 1.0bu Don't find in hidden fields
					rps.iChar = pfld->GetLength(); // 1.0dj Fix bug of not starting at end of previous field
					}
				}
			while ( pfld && bHiddenField( pfld ) ); // 1.0bu Don't find in hidden fields
        }
    return FALSE; // no matches in this record
}

static BOOL s_bChanged = FALSE;
// CProgressIndicator* s_pprogress = NULL; // 1.4ae Eliminate progress bar from replace all
long int s_lRec = 0;

//------------------------------------------------------
BOOL CShwView::bFind( CPatMChar* ppat, BOOL bForward )
{
    CFindSettings* pfndset = Shw_pProject()->pfndset();

    CNumberedRecElPtr prel = m_prelCur;
    CRecPos rps = m_rpsCur;
    ASSERT( ppat );
    if ( ppat->bSkip() )  // 1996-07-29 MRP
        {
        // Since skip matches the entire field, even if it's empty,
        // we must move off the current field so that Find Next and
        // Find Previous don't get stuck on an empty field.
        CField* pfld = rps.pfld;
        if ( bForward )
            pfld = rps.prec->pfldNext(pfld);
        else
            pfld = rps.prec->pfldPrev(pfld);
        
        if ( !pfld )
            {
            if ( pfndset->bSingleRecord() )
                return FALSE;
                
            if ( bForward )
                {
                if ( pfndset->bReplace() && s_bChanged )
                    {
                    if ( pfndset->bReplaceAll() )
                        GetDocument()->RecordModified( prel );
                    else if ( !bValidateCur() )
                        return FALSE;
                    s_bChanged = FALSE;
                    }

//                if ( s_pprogress )
//                    s_pprogress->bUpdateProgress(++s_lRec, FALSE); // don't process messages because m_prelCur, etc. are out of sync
                prel = prelNext(prel);
                if ( prel )
                    pfld = prel->prec()->pfldFirst();
                }
            else
                {
                prel = m_pind->pnrlPrev(prel);
                if ( prel )
                    pfld = prel->prec()->pfldLast();
                }
            }
        
        if ( !pfld )
            return FALSE;
        
        ASSERT( prel );
        rps.SetPos(0, pfld, prel->prec());
        }
    else if ( bSelecting( eAnyText ) )
        {
        if ( bForward )
			rps = rpsSelEnd(); // 1.4nc Fix bug of finding twice if preceding overstrike // If find forward, start at end of previous selection
		else
			rps = rpsSelBeg(); // If find backward, start previous selection
//            rps.iChar++; // advance one char so we don't find the same word again! // 1.4nc
        }
    ClearSelection();  // 1996-07-29 MRP

    while ( TRUE )
        {
        int iMatLen=0;
        if ( bFindInRec( pfndset, ppat, rps, &iMatLen, bForward ) ) // try to find match in this record
            { // found a match!
            if ( !pfndset->bReplaceAll() && prel != m_prelCur ) // moved onto different record
                {
                if ( pfndset->bReplace() || bValidateCur( dirSameRecord ) ) // moving to different record, need to validate previous // 1.4wc Fix U bug of skipping records on replace with filter
                    SetCur( prel ); // view new record
                else
                    break;
                }
            else
                m_prelCur = prel;
            m_rpsCur = rps;
             // hilite matching word
            if ( pfndset->bReplaceAll() ) // do it quick and dirty for replace all
                {
                m_rpsCur.SetPos( rps.iChar, rps.pfld, prel->prec() );
                StartSelection( eText );
                m_rpsCur.iChar += iMatLen;
                }
            else
				{
				if ( rps.bInterlinear() ) // 1.1ub On find in interlinear, scroll annotations into view
					{
					CRecPos rpsTmp = rps;
					while ( rpsTmp.bInterlinear() && !rpsTmp.bLastInBundle() )
						rpsTmp.NextField();
					EditSetCaret( rpsTmp.pfld, rpsTmp.iChar ); // 1.1ub On find in interlinear, scroll annotations into view
					}
                EditSetCaret( rps.pfld, rps.iChar, iMatLen );
				}
            if ( !pfndset->bReplace() )
                Shw_pProject()->pkmnManager()->ActivateKeyboard(m_rpsCur.pmkr());
            return TRUE;
            }
        
        if ( pfndset->bSingleRecord() )
            break;

		CRecord* precCur = m_rpsCur.prec; // 1.2mw Fix bug (1.2kd) of find not moving to next record in text // 1.2kd Fix bug (1.1mh) of Replace All failing in key field // 1.1mh Fix bug of find repeating same record if not sorted by key
		BOOL bValidateMoved = FALSE; // 1.4wc 
        if ( bForward )
            {
            if ( pfndset->bReplace() && s_bChanged )
                {
				CNumberedRecElPtr prelBefore = m_prelCur; // 1.4wc Remember cur before validate
                if ( pfndset->bReplaceAll() )
                    GetDocument()->RecordModified( prel );
                else if ( !bValidateCur( dirSameRecord ) ) // 1.4wc Fix U bug of skipping records on replace with filter
                    return FALSE;
				if ( m_prelCur != prelBefore ) // 1.4wc Note if validate moved
					bValidateMoved = TRUE; // 1.4wc 
				prel = m_prelCur; // 1.4gzh Fix bug of crash on replace of last found item in file
                s_bChanged = FALSE;
                }
			if ( pfndset->bReplaceAll() ) // 1.2mw Fix bug (1.2kd) of find not moving to next record in text
				prel = prelNext( prel ); // 1.2kd Fix bug (1.1mh) of Replace All failing in key field
			else
				{
				if ( !bValidateMoved ) // 1.4wc If validate moved, don't move further
					{
					precCur = prel->prec(); // 1.6.0aa Fix bug of find repeating same record in text file
					do { // 1.1mh Fix bug of find repeating same record if not sorted by key
						prel = m_pind->pnrlNext( prel ); // 1.1pf Change to pnrlNext from prelNext
						} while ( prel && prel->prec() == precCur );
					}
				}
            }
        else
			{
			if ( pfndset->bReplaceAll() ) // 1.2mw Fix bug (1.2kd) of find not moving to next record in text
				prel = m_pind->pnrlPrev( prel ); // 1.2kd Fix bug (1.1mh) of Replace All failing in key field
			else
				{
				precCur = prel->prec(); // 1.6.0aa Fix bug of find repeating same record in text file
				do { // 1.1mh Fix bug of find repeating same record if not sorted by key
					prel = m_pind->pnrlPrev( prel );
					} while ( prel && prel->prec() == precCur );
				}
			}
        if ( !prel )
            break; // at end (or beginning) of record set
        
        if ( bForward )
            rps.SetPos( 0, prel->prec()->pfldFirst(), prel->prec() ); // start find at beginning of this record
        else
            rps.SetPos( prel->prec()->pfldLast()->GetLength(),
                        prel->prec()->pfldLast(), prel->prec() ); // start find at end of this record
        
//      RouteMessages(); // don't lock up the machine!
        }
    return FALSE;
}

//------------------------------------------------------
BOOL CShwView::bEditFind( BOOL bForward ) // find implementation
{
    CFindSettings* pfndset = Shw_pProject()->pfndset();
    
    //08-28-1997
    if ( (pfndset->bReplaceAll() || pfndset->bReplace() ) && !bValidateCur() )
        return FALSE;
    
    if ( m_bBrowsing )
        SwitchToRecordView();

    if ( !m_prelCur || pfndset->pftxlst()->bIsEmpty() )
        {
        MessageBeep(0);
        return FALSE;
        }
    
    CRecord* precSave = m_prelCur->prec();
    s_bChanged = FALSE;
    CRecPos rpsSave = m_rpsCur;
    CNumberedRecElPtr prelSave = m_prelCur;

    CVarSet* pvarset = Shw_plngset()->pvarset();
    const char* pszText = pfndset->pftxlst()->pftxFirst()->pszText();
    CPatMChar* ppat = NULL;
    CNote* pnot = NULL;
    if ( !CPatMChar::s_bConstruct(&pszText, "", "", pvarset, &ppat, &pnot) )
        {
        ASSERT( !ppat );
        ASSERT( pnot );
        delete pnot;
        MessageBeep(0);
        return FALSE;
        }

    long int lReplaceCnt = 0;

    if ( pfndset->bReplace() && !pfndset->bSingleRecord() ) // need to build record list?
        {
        m_prcplst = prcpBuildList( pfndset->bReplaceAll() );
        if ( pfndset->bReplaceAll() ) // start at top of index
            {
            m_prcpCur = NULL; // make prelNext() return first in list
            m_prelCur = prelNext(m_prelCur); // get first prec in list
            }
        }
    else
        m_prcplst = NULL;

    if ( pfndset->bReplaceAll() )
        {
        ClearSelection();
        m_rpsCur.SetPos( 0, m_prelCur->prec()->pfldFirst(), m_prelCur->prec() ); // start at top of record
        Shw_pbarStatus()->Invalidate();
        Shw_pbarStatus()->UpdateWindow(); // update directly as we won't be processing messages
        s_lRec = 0; // record count for progress indicator

        if ( bFind( ppat, bForward ) )
            {
            do  {
                ASSERT( m_rpsSelOrigin.pfld == m_rpsCur.pfld ); // selection limited to one field?
                if ( m_rpsSelOrigin.iChar < m_rpsCur.iChar )
                    {
                    Str8 sTail = m_rpsCur.pfld->Mid( m_rpsCur.iChar ); // Save stuff following selection
                    m_rpsCur.pfld->ReleaseBuffer( m_rpsSelOrigin.iChar ); // Cut off tail part selection
                    *m_rpsCur.pfld += sTail; // Reattach stuff following deleted text
                    m_rpsCur.iChar = m_rpsSelOrigin.iChar;
                    }
                ClearSelection();
                m_prelCur->prec()->Paste( &m_rpsCur, Shw_pProject()->pfndset()->pszReplaceText(),
                                            GetDocument()->pmkrset() ); // insert into record at current caret position
                s_bChanged = TRUE;
                lReplaceCnt++;

                } while ( bFind( ppat, TRUE ) ); // find next occurence

            CNumberedRecElPtr prel = m_pind->prelFind( precSave ); // stay on same record ???
            if ( !prel ) // did it drop out of filtered index?
                prel = m_pind->pnrlFirst();
            SetCur( prel );
            GetDocument()->SetModifiedFlag();
            // tell user how many replacements were made
            char caNum[10];
            _ltoa_s(lReplaceCnt, caNum, (int)sizeof(caNum), 10);
			Str8 s = _("Number of occurrences replaced:"); // 1.5.0fv 
			s = s + " " + caNum; // 1.5.0fd 
            AfxMessageBox( s );
            }
        else
            {
            m_prelCur = prelSave; // nothing found, should be safe to just restore these
            m_rpsCur = rpsSave;
            MessageBeep(0);
            }
		pfndset->ClearReplace(); // 1.4vzh Fix bug of possibly doing replace all on find next
        }
    else // not replace all
        {
        BeginWaitCursor();
        if ( bFind( ppat, bForward ) )
            {
            if ( pfndset->bReplace() )
                {
                m_ppat = ppat; // make accessible to function called from replace dialog
                CPoint pnt = m_pntCaretLoc - GetScrollPosition(); // return caret position to dialog
                ClientToScreen( &pnt ); // convert to screen coordinates
                CAskReplaceDlg dlg( this, pnt ); // find out if user wants to replace this occurrence
                dlg.DoModal();
                if ( m_bModifiedSinceValidated )
                    {
                    EditSetCaret( m_rpsCur.pfld, m_rpsCur.iChar );
                    SetScroll( TRUE );
                    SetCaretPosAdj();
                    }
                m_ppat = NULL;
                }                
			ParallelJump(); // 1.1kd Do parallel move on find
            }
        else
            {
            MessageBeep(0);
            if ( pfndset->bReplace() )
                {
                m_prelCur = prelSave; // nothing found, should be safe to just restore these
                m_rpsCur = rpsSave;
                }
            }
        }

    delete ppat;
    if ( m_prcplst )
        {
        delete m_prcplst;
        m_prcplst = NULL;
        }

    EndWaitCursor();
    Invalidate(); // selection may have disappeared
    return FALSE;
}

// bReplace() is a sort of callback function to be called from a modal dialog
BOOL CShwView::bReplace( BOOL bDoReplace, CPoint* ppntCaret )
{
    if ( bDoReplace ) // do replace
        {
#ifdef NOT_COMPLETE // 1.4ywg 
		if ( m_rpsCur.bInterlinear() && bSelecting( eText ) ) // 1.4ywg If replace in interlinear, maintain alignment
			{
			CRecPos rpsBeg = rpsSelBeg(); // 1.4ywg 
			CRecPos rpsEnd = rpsSelEnd(); // 1.4ywg 
			int iFindLen = rpsEnd.iChar - rpsBeg.iChar; // 1.4ywg Get length of find string
			const char* pszReplace = Shw_pProject()->pfndset()->pszReplaceText(); // 1.4ywg 
			int iReplaceLen = strlen( pszReplace ); // 1.4ywg Get length of replace string
			int iLonger = iFindLen - iReplaceLen; // 1.4ywg 
			int iShorter = iReplaceLen - iFindLen; // 1.4ywg 
			if ( iLonger > 0 ) // 1.4ywg If replace is longer, loosen to make space for it
				{

				}
			}
		else // 1.4ywg If not interlinear, do normal replace
#endif // NOT_COMPLETE // 1.4ywg
			{
			if (bSelecting( eAnyText) )
				{
				CRecPos rpsBeg = rpsSelBeg();
				DeleteSelection();
				m_rpsCur = rpsBeg; // fix problem with DeleteSelection() moving m_rpsCur to beginning
				}
								// of next word in interlinear field
			m_prelCur->prec()->Paste( &m_rpsCur, Shw_pProject()->pfndset()->pszReplaceText(),
										GetDocument()->pmkrset() ); // insert into record at current caret position
			}
        s_bChanged = TRUE;
        SetModifiedFlag(); // Note that something changed
        ResetUndo(); // clear undo buffer
        }
    if ( !bFind( m_ppat, TRUE ) ) // find next occurence
        {
        MessageBeep(0);
        return FALSE;
        }
    else
        UpdateStatusBar( Shw_pbarStatus() ); // force status bar update so users know what record they're on

    CPoint pnt = m_pntCaretLoc - GetScrollPosition(); // return caret position to dialog
    ClientToScreen( &pnt ); // convert to screen coordinates
    *ppntCaret = pnt;

    return TRUE;
}
