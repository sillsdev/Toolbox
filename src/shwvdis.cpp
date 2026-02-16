// shwvdis.cpp : implementation of window display parts of the
// CShwView class
//

#ifdef _MAC
#include "script.h"
#endif

#include "stdafx.h"
#include "toolbox.h"
#include "project.h"
#include <limits.h>

#include "shw.h"

#include "mainfrm.h"
#include "shwdoc.h"
#include "shwview.h"
#include "status.h"  // class CShwStatusBar
#include <ctype.h>

#include "mkrfed_d.h"
#include "jumpdlg.h"
#include "tools.h"
#include "typ.h"
#include "filsel_d.h"
#include "find_d.h"
#include "tplt_d.h"
#include "kmn.h"
#include "crngset.h"  // CRangeSet
#include "corpus.h" // 1.4vzn 
// #include "usp10.h" // Uniscribe ScriptString functions

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

//------------------------------------------------------
int CShwView::iCharToPixel( CDC* pDC, CField* pfld, const char* pszLine, int iLineLen, int iCharPos ) // get pixel length of a chunk of text
{
    ASSERT( iCharPos <= iLineLen );
	int iPixel = 0;
#ifdef _MAC
    int iDirection = pfld->plng()->bRightToLeft() ? -1 : 1;
    return Char2Pixel( (char*)pszLine, iLineLen, 0, iCharPos, iDirection );
#else
    CLangEnc* plng = pfld->plng();
	if ( plng->bRightToLeft() ) // If right to left, calculate from end of string
        // The following measures from the current position to the left end of the line.
        // This gives the distance to place the cursor from the left end.
        // This also gives a good number for Arabic because the first character measured
        // is treated as initial. Initial widths are very close to medial widths.
        {
        iPixel = plng->GetTextWidth( pDC, pszLine + iCharPos, iLineLen - iCharPos );
        }
    else
        iPixel = plng->GetTextWidth( pDC, pszLine, iCharPos );
    return iPixel;
#endif
}

//------------------------------------------------------
int CShwView::iPixelToChar( CDC* pDC, CField* pfld, const char* pszLine, int iLineLen, int iPixel ) // get character offset closest to iPixel
{
    BOOL bRL = pfld->plng()->bRightToLeft();
    int iFullExtent = pfld->plng()->GetTextWidth( pDC, pszLine, iLineLen );
    if ( iPixel >= iFullExtent ) // If to right of line, return right end
        return bRL ? 0 : iLineLen;
    if ( iPixel <= 0 ) // If to left of line, return left end
        return bRL ? iLineLen : 0;

#ifdef _MAC
    Boolean bLeadingEdge;
    return Pixel2Char( (char*)pszLine, iLineLen, 0, iPixel, &bLeadingEdge );
#else
    int iPosTry = 0;
	int iPos = 0;
	int iPosLeftmost = 0; // Earliest suitable position found, used to put cursor before cluster instead of inside
	int iPixelCur = 0; // Current pixel position
	int iPixelLeftmost = 0; // Earliest pixel position to help find best position

    if ( bRL ) // If right to left, comparison logic is reversed, see iCharToPixel for more info
        {
        while ( iPosTry <= iLineLen && iCharToPixel( pDC, pfld, pszLine, iLineLen, iPosTry ) + 2 > iPixel )
			{
			iPos = iPosTry; // Remember nearest place
            iPosTry += pfld->plng()->iCharNumBytes( pszLine + iPosTry );
			}
        }
    else
        { 
        while ( iPosTry <= iLineLen )
			{
			iPixelCur = iCharToPixel( pDC, pfld, pszLine, iLineLen, iPosTry ) - 2; // Adjust by 2 is just enough to make it more likely to hit in the nearest place
			if ( iPixelCur > iPixel )
				break;
//			if ( iPixelCur > iPixelLeftmost ) // If pos moved, remember new place // 1.4gzf Change default cursor pos on mouse click to after overstrikes
				{
				iPixelLeftmost = iPixelCur;
				iPosLeftmost = iPosTry;
				}
			iPos = iPosLeftmost; // Remember nearest place
            iPosTry += pfld->plng()->iCharNumBytes( pszLine + iPosTry );
			}
        }
    return iPos;
#endif
	// Algorithm for leftmost is:
	// If this is same as preve leftmost, then don't move leftmost
	//	else move leftmost to here

}
//------------------------------------------------------
void CShwView::ClearSelection( int iDirection ) // Deselect any current selection
{
    if ( bSelecting( eAnyText ) )
        {
        if ( iDirection ) // if zero, just leave caret at m_rpsCur
            {
            if ( iDirection > 0 ) // set caret to end of selection
                {
                if ( m_rpsCur < m_rpsSelOrigin )
                    {
                    m_rpsCur.SetPos( m_rpsSelOrigin.iChar, m_rpsSelOrigin.pfld );
                    m_pntCaretLoc = pntPosToPnt( m_rpsCur );
                    }
                }
            else // place caret at beginning of selection
                {
                if ( m_rpsCur > m_rpsSelOrigin )
                    {
                    m_rpsCur.SetPos( m_rpsSelOrigin.iChar, m_rpsSelOrigin.pfld );
                    m_pntCaretLoc = pntPosToPnt( m_rpsCur );
                    }
                }
            }
        m_iSelection = 0; // 1.5.8b Fix bug of not remembering find in specific field
        }
} 
//------------------------------------------------------
void CShwView::ShowCaret() // override to add more intelligent caret handling
{
    if ( m_bHaveFocus && m_bCaretHidden && !bSelecting(eAnyText) )
        {
        CWnd::ShowCaret();
        m_bCaretHidden = FALSE;
        }
}
//------------------------------------------------------
void CShwView::HideCaret()
{
    if ( m_bHaveFocus && !m_bCaretHidden )
        {
        CWnd::HideCaret();
        m_bCaretHidden = TRUE;
        }
}
//------------------------------------------------------
void CShwView::SetCaretSizeCreate( BOOL bForce ) // Create caret based on the current field font
{
    int iHt;
    if ( m_bBrowsing ) // AB 6-3-97 This is a protective return in case of error.
        return;
    if ( m_prelCur )
        iHt = iLineHt( m_rpsCur.pfld ); // set caret height to height of font at caret pos
    else
        iHt = m_iCaretHt;
    if ( ( m_iCaretHt != iHt || bForce ) )
        {
        m_bCaretHidden = TRUE; // caret is created hidden
#ifdef _MAC
        CreateSolidCaret( 1, m_iCaretHt = iHt );    // BJY Set width of caret more intelligently!?!
#else
        CreateSolidCaret( 2, m_iCaretHt = iHt );    // BJY Set width of caret more intelligently!?!
#endif
        }
}
//------------------------------------------------------
void CShwView::SetCaretPosAdj() // SetCaretPos adjusted for scrolling
{
	if ( m_bBrowsing ) // 1.1tb Fix possible crash on insert field in browse view
		return;
    ASSERT( m_prelCur );

    CalcCaretLocX(); // resync caret position with m_rpsCur

    CLPoint pnt = m_pntCaretLoc;
    CPoint pntOrg = GetScrollPosition(); // Get current origin, upper left corner postion
    CPoint pntOrgSav = pntOrg; // Remember it
    CRect rect;
    GetClientRect( &rect ); // Get size of client area of current window
    CSize siz = rect.Size();
    if ( pnt.x == m_iFldStartCol || bSelecting( eLines|eFields ) ) // caret at start of line, scroll marker column into view
        pntOrg.x = 0;
    else if ( pnt.x > pntOrg.x + siz.cx - 4 ) // If cursor moved off the right, scroll into view, allowing for width of char
        pntOrg.x = pnt.x + 20 - siz.cx; // Scroll a bit past cursor if possible, like WinWord // 1.1hu Reduce jumping effect by changing 50 down to 20
    else if ( pnt.x < pntOrg.x ) // If cursor moved off the left, scroll into view
        pntOrg.x = __max( pnt.x - 20, 0 ); // Scroll a bit past cursor if possible, like WinWord // 1.1hu Reduce jumping effect
    if ( pntOrg.x != pntOrgSav.x ) // If origin moved, scroll to it - unless selecting whole lines
        {
        ScrollToPosition( pntOrg );
        pntOrg = GetScrollPosition(); // Find out what really happened
        }
    pnt.x -= pntOrg.x; // Adjust for scroll origin

    if ( pnt.y > siz.cy - iLineHt( m_rpsCur.pfld ) ) // If cursor moved off the bottom, scroll into view, allowing for height of line
        pnt.y -= lVScroll( pnt.y - siz.cy + iLineHt( m_rpsCur.pfld ) ); // Allow for height of line
    else if ( pnt.y < 0 ) // If cursor moved off the top, scroll into view
        pnt.y -= lVScroll( pnt.y );

    if ( GetFocus() == this )  // don't mess with caret if another window has focus
        {
        if ( m_rpsCur.pfld )
            SetCaretSizeCreate();
        SetCaretPos( pnt );
        }
}
// #define YSCROLL_HT 10000      // Logical value for 100% of window height
static float fYScrollHt = 10000; // Logical value for 100% of window height, 1.1hp Make flost to handle very large records and db's
static BOOL bInSetScroll = FALSE; // True if called from within SetScroll
//------------------------------------------------------
void CShwView::SetScroll( BOOL bCalc ) // Set scroll sizes, recalculate if bCalc is true
{
    ASSERT( !m_bBrowsing );

    if (bInSetScroll)
        return;
    bInSetScroll = TRUE;

    CSize siz;
    siz.cx = iMeasureRecord( bCalc ); // m_lTotalHt gets set as well if bCalc T
    siz.cx += 10;
  
    CSize sizSb, sizView;
    GetTrueClientSize(sizView, sizSb); // get size of view including scroll bars if any
    BOOL bHasHorizBar = (siz.cx > sizView.cx - sizSb.cx); // 2000-09-12 TLB: used to be ... - sizSb.cy);
    if (bHasHorizBar) // make logical view wider if we already have a scroll bar
        siz.cx += 50;
    if ( sizView.cy < m_lTotalHt
        || (bHasHorizBar && sizView.cy - sizSb.cy <= m_lTotalHt)) // 2000-09-12 TLB: used to be ... - sizSb.cx...
        {
        if (bHasHorizBar)
            sizView.cy -= sizSb.cy;
        siz.cy = (int)( ( ( m_lTotalHt - sizView.cy ) * fYScrollHt ) / m_lTotalHt + sizView.cy );
        siz.cy = max(siz.cy, sizView.cy+1); // siz.cy can end up negative in strange circumstances BJY?
		siz.cy += 120; // ALB 5.96r Add enough to be sure last line shows fully in Unicode
        }
    else
        siz.cy = 1; // no vertical scroll bar needed

    SetScrollSizes( MM_TEXT, siz );
    if ( siz.cy != 1 )
        SetScrollPos( SB_VERT, (int)( m_lVPosUL * fYScrollHt / m_lTotalHt ) );
    else if ( m_lVPosUL ) // no scroll bar now, make sure whole record is viewable
        {
        m_pntCaretLoc.y += m_lVPosUL;
        m_lVPosUL = 0; // don't call lVScroll() in SetScroll()!
        if ( m_prelCur )
            m_rpsUpperLeft.SetPos(0, m_prelCur->prec()->pfldFirst());
        SetCaretLoc();
        Invalidate(FALSE);
        }
    bInSetScroll = FALSE;
}
//------------------------------------------------------
void CShwView::SetCaretLoc() // Setup to display new record
{
    CLPoint pnt = m_pntCaretLoc - GetScrollPosition();
    if (pnt.y < -32000 ||   // SetCaretPos() gets passed a CPoint which in the 16-bit version is
        pnt.y > 32000)      // made up of 16 bit integers for x and y. We have to guard against passing
                            // in an out of range y value.
        {
        SetCaretPos( CPoint(-100,-100) ); // just put it out of viewable area
        return;
        }
    SetCaretPos( pnt );
}
//------------------------------------------------------
void CShwView::SetCur(CNumberedRecElPtr prelCur, BOOL bCenterBrowseCur, BOOL bDuplicating) // Setup to display new record
{
//    ASSERT( !bModifiedSinceValidated() ); // AB 5.1g disable because it gave a false assertion
    m_bScrollOverflow = FALSE;       // BW 2/3/98 clear error flags
    m_bScrollOverflowWarning = FALSE;

    if ( m_bBrowsing ) // don't need to do much for record change in browse view
        {
        m_bCanUndoAll = FALSE;
        m_precUndoAll = NULL;
        m_prelCur = prelCur;
        if ( !m_prelCur )
            {
            SetScrollSizes( MM_TEXT, CSize( 1, 1 ) ); // Set no scroll bars
            m_prelUL = m_prelCur;
            Invalidate(TRUE);
            }
        else
            InitBrowse(bCenterBrowseCur);
            
        SetStatusRecordAndPrimaryKey(m_prelCur);
        Invalidate(FALSE);
        return;
        }

    HideCaret();
    if ( !bDuplicating )
        {
        m_prelCur = prelCur; // Move to a new current record [element]
        ClearSelection();
        m_iMaxLineWd = 100;
        }

    CopyToUndoAllBuffer();
    ResetUndo();

    if ( !m_prelCur )
        {                                                              
        // m_rpsCur.prec = NULL;
        // m_rpsCur.pfld = NULL;
        // m_rpsCur.iChar = 0;
        m_rpsCur.SetPos(0, NULL, NULL);  // 1996-10-15 MRP
        SetScrollSizes( MM_TEXT, CSize( 1, 1 ) ); // Set no scroll bars
        Invalidate(TRUE); // Force repaint of window
        m_pntCaretLoc = CLPoint( -100, -100 ); // Move caret to out of window
        SetCaretLoc();
        ShowCaret(); // Show must be once for each Hide
        SetStatusRecordAndPrimaryKey(m_prelCur);
        return;
        }
    
    if ( !bDuplicating )
        {
        m_lVPosUL = 0;
        m_rpsCur.prec = prelCur->prec();
        m_rpsUpperLeft.SetPos( 0, m_prelCur->prec()->pfldFirst(), prelCur->prec() );
        MoveCtrlHome(); // Start at top
        }
    
    SetScroll( TRUE );  // Calculate and establish scroll bar settings
    
    if ( !bDuplicating )
        {
        CField* pfld = m_prelCur->pfldPriKey(); // Get field used for key in this record
        if ( pfld ) // If field exists
            {
            if ( m_iInitLine ) // 1.1kx Fix bug of jump fail || ptyp()->bTextFile() ) // 1.1kj Fix bug of wrongly placing cursor at start if at top of record in text file // If initialization value, init position
                {
                for ( ; ; m_iInitLine-- ) // Move down to current location
                    {
                    if ( m_iInitLine == m_iInitLineScroll ) // If we come to the scroll upper left field, set it
                        m_rpsUpperLeft.pfld = m_rpsCur.pfld;
                    if ( !m_iInitLine )
                        break;
                    iDown( m_rpsCur ); 
                    }
                if ( m_iInitChar <= m_rpsCur.pfld->GetLength() ) // If char position within field, set it, otherwise leave at 0
                    m_rpsCur.iChar = m_iInitChar;
                }
            else // Else move to primary key
                {
                while ( m_rpsCur.pfld != pfld ) // Move down to key field
                    {
                    BOOL bMoved = bMoveDown( TRUE ); // 1.1pc Fix bug of text parallel move showing wrong place if hidden fields
					if ( !bMoved )  // 1.5.8wa Fix possible crash on parallel move
						{
						m_rpsCur.pfld = m_prelCur->prec()->pfldFirst(); // 1.5.8wa If fld not found, set to first
						break; // 1.5.8wa 
						}
                    // ASSERT( bMoved );  // 1995-11-11 MRP // 1.5.8wa Change from assert to fix problem
                    }
                }
            }
        ASSERT( m_rpsCur.pfld ); // Must find it, or leave it valid
        Invalidate(FALSE); // Force repaint of window
        SetCaretPosAdj(); // Move caret
        }
    SetStatusRecordAndPrimaryKey(m_prelCur);
    
    ShowCaret();
    if (m_rpsCur.pfld)
        {
        Shw_pProject()->pkmnManager()->ActivateKeyboard(m_rpsCur.pmkr());
        }
}
//------------------------------------------------------
void CShwView::InitializeRec() // Implementation for OnInitialUpdate()
{
    CShwDoc* pdoc = GetDocument(); // 1.4qzb Fix bug of crash on file open
	ASSERT( pdoc->pindset() ); // 1.4qzd
    if ( s_pobsAutoload )
        {
        bReadProperties(*s_pobsAutoload);
        // s_pobsAutoload = FALSE;  // 1996-09-05 MRP: Cleared in shwdoc.cpp
        }
    else if ( s_pViewToDuplicate )
        {
        Duplicate();
        VerifyBrowseFields();
        if ( s_prelJumpTo ) // doing a jump
            {
            // reset some settings set in Duplicate()
            ASSERT(s_pindJumpTo);
            m_pind = s_pindJumpTo;
            m_pindUnfiltered = m_pind;
            m_bBrowsing = FALSE;
            m_bIsJumpTarget = TRUE; // AB 12-14-01 Change default to jump target
			m_bFocusTarget = FALSE; // 1.4zag Make new window opened by jump not default to parallel move target
            m_bViewLocked = FALSE; // 1.2bn
            m_bModifiedSinceValidated = FALSE;
            CNumberedRecElPtr prel = s_prelJumpTo;
            // 2000/04/28 TLB & MRP - Removed function: ReNumberElement. ASSERT (and fix-up) now done here.
            prel = m_pind->prelCorrespondingTo(prel);
            if ( !prel )
                {
                ASSERT(prel);
                prel = m_pind->prelFirst();
                }
            SetCur( prel ); // goto desired record
            s_prelJumpTo = NULL;
            }
        else if ( s_pszJumpInsert ) // doing a jump insert
            {
            // reset some settings set in Duplicate()
            ASSERT(s_pindJumpTo);
            m_pind = s_pindJumpTo;
            m_pindUnfiltered = m_pind;
            m_bModifiedSinceValidated = FALSE;
            m_bIsJumpTarget = TRUE; // AB 12-14-01 Change default to jump target
			m_bFocusTarget = FALSE; // 1.4zag Make new window opened by jump not default to parallel move target
            m_bViewLocked = FALSE; // 1.2bn
            JumpInsert( s_pszJumpInsert ); // insert new record and go to it
            s_pszJumpInsert = NULL;
            }
        else
            { // true window duplicate
            SetCur(m_prelCur, TRUE, TRUE);
            }
        return;
        }
    else
        {
        // File New, File Open
        m_pind = pdoc->pindset()->pindRecordOwner(); // Index
        m_pindUnfiltered = m_pind;  // 1995-01-23 MRP
		if ( ptyp()->bTextFile() ) // 1.1mc If text file, open sorted by reference
			{
			CIndex* pind = m_pind; // Set up to make adjusted index
			CIndex* pindUnfiltered = m_pindUnfiltered;
			CIndex** ppind = &pind;
			CIndex** ppindUnfiltered = &pindUnfiltered;
			CIndexSet* pindset = pind->pindsetMyOwner();
			CMarker* pmkrPriKey = pind->pmkrPriKey();
			CMarkerSet* pmkrset = pmkrPriKey->pmkrsetMyOwner();
			CMarkerRefList	mrflstNewSecKeys;
			CFilter* pfil = pind->pfil();
			UINT	iSortKeyLen = pind->maxlenSortKeyComponent();
			BOOL bSortFromEnd = FALSE;

			pmkrPriKey = ptyp()->pmkrTextRef(); // Make text reference marker the key
			CSortOrder* psrtPri = pmkrPriKey->plng()->psrtDefault();
			pindset->GetIndex(pmkrPriKey, psrtPri, // Make new index
				&mrflstNewSecKeys, (Length)iSortKeyLen, bSortFromEnd,
				pfil, ppind, ppindUnfiltered);
			SetIndex( pind, pindUnfiltered ); // Change index to new
			TextParallelJump(); // 1.2pb Send external reference when a file is opened
			}
        m_prelCur = m_pind->pnrlFirst(); // Current record
        }
    VerifyBrowseFields(); // validate field widths and setup default browse field list if necessary
    SetCur(m_prelCur); // Set up to display new record  
    UpdateWindow();
}
//------------------------------------------------------
void CShwView::ResetUpperLeft() // attempt to set upper left position to previous pixel depth in record
{
    m_rpsUpperLeft.SetPos( 0, m_prelCur->prec()->pfldFirst() ); // Start at the top
    int iMove;
    for ( int i = 0; i < m_lVPosUL; i+=iMove ) // try to keep view at same depth into record
        {
        if ( !( iMove = iDown( m_rpsUpperLeft ) ) )
            break;
        }
}
//------------------------------------------------------
BOOL CShwView::bPositionValid( const CRecPos& rps ) const  // verify position exists in current record
{
	if ( !rps.pfld ) // 1.5.0kg 
		return FALSE; // 1.5.0kg 
    // RNE added m_prelCur in case it's null on an empty db or filter.
    return ( m_prelCur && m_prelCur->prec()->bIsMember( rps.pfld ) && rps.pfld->GetLength() >= rps.iChar );
}
//------------------------------------------------------
BOOL CShwView::bHome( CRecPos& rps ) // go to beginning of line, doesn't set caret loc
{
    const char* psz = pszPrevLineBrk( rps.pszFieldBeg(), rps.psz() ); // Find beginning of line
    if ( psz == rps.psz() )
        return FALSE; // didn't move
    rps.SetPos( psz ); // Move past line break
    return TRUE;
}
//------------------------------------------------------
int CShwView::iDnLine( CRecPos& rps ) // move rps down a line returning height of line moved from
{
    const char* pszStart = rps.psz(); // Calculate start position
    const char* psz = strchr( pszStart, '\n' ); // Find end of line. For speed does not call pszNextLineBrk 
    if ( psz ) // If line break in line
        {
        rps.SetPos( psz + 1 ); // Move past line break
        return iLineHt( rps.pfld );
        }
    return 0;
}
//------------------------------------------------------
int CShwView::iDnField( CRecPos& rps, BOOL bHide ) // move rps down a field returning height of line moved from
{
	if ( !rps.pfld ) // 1.5.0kg Fix possible crash on hide fields
		return 0; // 1.5.0kg 
    CField* pfld = rps.prec->pfldNext( rps.pfld );
	if ( bHide ) // 1.0bc Allow for hidden fields in cursor move
		while ( pfld && bHiddenField( pfld ) ) // 1.0bc If moved into hidden field, go on to next
		    pfld = rps.prec->pfldNext( pfld );
    if ( pfld ) // If another field, move to it
        {
        int iHt = iLineHt( rps.pfld ); // get height of line we're leaving
        rps.SetPos( 0, pfld ); // Move to beginning of next field
        return iHt;
        }   
    return 0;
}
//------------------------------------------------------
int CShwView::iUpLine( CRecPos& rps ) // move rps up a line returning height of line moved to
{
    if ( !rps.iChar ) // scroll to previous line in field if not at beginning of field
        return 0;
    const char* psz;
    if ( *(rps.psz()-1) == '\n' ) // save some time if in left column
        psz = rps.psz();
    else
        psz = pszPrevLineBrk( rps.pszFieldBeg(), rps.psz() );
    if ( psz != rps.pszFieldBeg() )
        {
        rps.SetPos( pszPrevLineBrk( rps.pszFieldBeg(), psz-1 ) );
        return iLineHt( rps.pfld );
        }
    return 0;
}
//------------------------------------------------------
int CShwView::iUpField( CRecPos& rps, BOOL bHide ) // move rps up a field returning height of line moved to
{
	if ( !rps.pfld ) // 1.5.0kg Fix possible crash on hide fields
		return 0; // 1.5.0kg 
    CField* pfld = rps.prec->pfldPrev( rps.pfld ); // try to go up a field
	if ( bHide ) // 1.0bc Allow for hidden fields in cursor move
		while ( pfld && bHiddenField( pfld ) ) // 1.0bc If moved into hidden field, go on to next
		    pfld = rps.prec->pfldPrev( pfld );
    if ( pfld )
        {
        rps.pfld = pfld;
        rps.SetPos( pszPrevLineBrk( rps.pszFieldBeg(), rps.pszFieldEnd() ) );
        return iLineHt( rps.pfld ); // return height of this line
        }
    return 0;
}
//------------------------------------------------------
int CShwView::iDown( CRecPos& rps )
{
    int iHt = iDnLine( rps );
    if ( !iHt )
        iHt = iDnField( rps, TRUE ); // 1.1ke Skip hidden fields on move down
    return iHt; // return height of this line
}
//------------------------------------------------------
int CShwView::iUp( CRecPos& rps )
{
    int iHt = iUpLine( rps );
    if ( !iHt )
        iHt = iUpField( rps, TRUE ); // 1.1ke Skip hidden fields on move up
    return iHt; // return height of this line
}
//------------------------------------------------------
long CShwView::lVScroll( long lPixels, BOOL bForce ) // perform actual window scroll of at least iPixels
{
    long lActual = 0;

    if ( !bForce )
        {
        int yMin, yMax, y = GetScrollPos(SB_VERT);
        GetScrollRange(SB_VERT, &yMin, &yMax);
        ASSERT(yMin == 0);
        long lPixelMax = (long)((float)yMax * m_lTotalHt / fYScrollHt);
        long lTarget = m_lVPosUL + lPixels;
        if ( lTarget < 0 )
            lPixels = -m_lVPosUL;
        else if ( lTarget > lPixelMax && lPixels > 0 )
            lPixels = max( 0, lPixelMax - m_lVPosUL );
        }

    if ( lPixels > 0 ) // scroll down
        {
        while ( lActual < lPixels )
            {
            int i = iDown( m_rpsUpperLeft );
            if ( !i )
                break;  // can't scroll as far as requested
            lActual += i;
            }
        }
    else if ( lPixels < 0 ) // scroll up
        {
        while ( lActual > lPixels )
            {
            int i = iUp( m_rpsUpperLeft );
            if ( !i )
                break;  // can't scroll as far as requested
            lActual -= i;
            }
        }

    if ( lActual )
        {
// TLB 01-05-2000: The following three lines were commented-out to correct a
// bug whereby caret remains visible when it should be outside the viewable
// region of the window after doing a page-up or page-down by clicking in the
// gray region of the scroll bar. I hope this doesn't break anything -- I can't
// figure out why this logic was ever added.
//        if (labs(lActual) < 500) 
            ScrollWindow( 0, -(int)lActual ); // perform actual window scroll 
//        else
//            Invalidate(FALSE);
        m_lVPosUL += lActual;
        m_pntCaretLoc.y -= lActual;
        SetScrollPos( SB_VERT, (int)( m_lVPosUL * fYScrollHt / m_lTotalHt ) );
        }
    return lActual;
}
//------------------------------------------------------
CPoint CShwView::GetScrollPosition() const   // override base class GetScrollPosition()
{
    CPoint pnt = CShwScrollView::GetScrollPosition();
    pnt.y = 0;                          
    return pnt;
}                               
//------------------------------------------------------
BOOL CShwView::OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll) // handle scroll bar manipulation
{
    if ( m_bBrowsing )
        {
        BrowseOnScroll( nScrollCode, nPos, bDoScroll );
        return TRUE;
        }
    // calc new x position
    int x = GetScrollPos(SB_HORZ);
    int xOrig = x;
    CRect rect;

    switch (LOBYTE(nScrollCode))
    {
    case SB_TOP:
        x = 0;
        break;
    case SB_BOTTOM:
        x = INT_MAX;
        break;
    case SB_LINEUP:
        x -= m_lineDev.cx;
        break;
    case SB_LINEDOWN:
        x += m_lineDev.cx;
        break;
    case SB_PAGEUP:
        x -= m_pageDev.cx;
        break;
    case SB_PAGEDOWN:
        x += m_pageDev.cx;
        break;
    case SB_THUMBTRACK:
        x = nPos;
        break;
    }

    // calc new y position
    int y = GetScrollPos(SB_VERT);
    int yOrig = y;

    long lScrolled = 0;

    switch (HIBYTE(nScrollCode))
    {
    case SB_TOP:
        y = 0;
        break;
    case SB_BOTTOM:
        y = INT_MAX;
        break;
    case SB_LINEUP:
        lScrolled = lVScroll( -1 );
        break;
    case SB_LINEDOWN:
        lScrolled = lVScroll( 1 ); // iLineHt( m_rpsUpperLeft.pfld ) );
        break;
    case SB_PAGEUP:
        GetClientRect( &rect );
        lScrolled = lVScroll( min( 0, -rect.bottom + iLineHt( m_rpsUpperLeft.pfld ) * 2 ) );
        break;
    case SB_PAGEDOWN:
        GetClientRect( &rect );
        lScrolled = lVScroll( max( 0, rect.bottom - iLineHt( m_rpsUpperLeft.pfld ) * 2 ) );
        break;
    case SB_THUMBTRACK:
        long lPos = (long)((float)nPos * m_lTotalHt / fYScrollHt );
        CRecPos rps = m_rpsUpperLeft;
        if ( lPos < m_lVPosUL )
            iUp( rps );
        int yMin, yMax;
        GetScrollRange(SB_VERT, &yMin, &yMax); // check for bottom of scroll bar

        if ( labs( lPos - m_lVPosUL ) >= iLineHt( rps.pfld ) || ( (int)nPos >= yMax ) )
            lScrolled = lVScroll( lPos - m_lVPosUL );
        break;
    }

    BOOL bResult = OnScrollBy(CSize(x - xOrig, 0), bDoScroll) || lScrolled != 0;
    if (bResult)
        {
        m_bDontCallSetCaretPosAdj = TRUE; // signal DrawRec() to NOT call SetCaretPosAdj()
        UpdateWindow();
        m_bDontCallSetCaretPosAdj = FALSE;
        }

    ASSERT(bDoScroll); // this should be FALSE only for OLE targets according to docs

    return bResult;
}
//------------------------------------------------------
// Get starting position of selection for pfld.
// Please note that this will return 0 if selection starts at the
// beginning of the field or if there is no selection at all.
// iGetSelEnd() should be called to determine if there is any
// selected text in a field.
int CShwView::iGetSelBeg( CField* pfld )
{
    if ( bSelecting( eAnyText ) ) // If anything selected
        {           
        CRecPos rps = rpsSelBeg(); // Get beginning of selection
        if ( rps.pfld == pfld ) // If selection begins in this field, return begin position (may be 0)
            return rps.iChar;
        rps.SetPos( 0, pfld ); // Set temp to begin field
        if ( rpsSelEnd() <= rps ) // If sel end is earlier, no selectin here
            return 0;
        }
    return 0; // If nothing selected at all, return 0
}
//------------------------------------------------------
// get end position of any selected text in pfld (0 if no selection in pfld)
int CShwView::iGetSelEnd( CField* pfld )
{
    if ( bSelecting( eAnyText ) ) // If anything selected
        {
        CRecPos rpsEnd = rpsSelEnd(); // Get end of selection
        if ( rpsEnd.pfld == pfld ) // If selection ends in this field, return end position (never 0?)
            return rpsEnd.iChar;
        CRecPos rps = rpsEnd; // Initialize temp rps to record
        rps.SetPos( 0, pfld ); // Set temp rps to begin field
        if ( rpsEnd <= rps ) // If sel end is earlier, no selection here
            return 0;
        rps.SetPos( rps.pszFieldEnd() ); // Set temp to end field
        CRecPos rpsBeg = rpsSelBeg(); // Get begin of selection
        if ( rps <= rpsBeg // If sel begin is later, or here and not selection whole line or field, no selection here
                || ( !bSelecting( eLines|eFields ) && rpsBeg == rps ) )
            return 0;
        return rps.pfld->GetLength(); // If selection ends later than this field, return end of field; this guarantees non-zero if anything in this field is selected
        }
    return 0; // If nothing selected at all, return 0
}
//------------------------------------------------------
// return pointer to beginning of selection
const CRecPos& CShwView::rpsSelBeg()
{
    ASSERT( bSelecting( eAnyText ) );
    if ( m_rpsCur < m_rpsSelOrigin )
        return m_rpsCur;
    else
        return m_rpsSelOrigin;

#ifdef COMPILER_BUG_IN_RELEASE_VERSION
    return m_rpsCur < m_rpsSelOrigin ? m_rpsCur : m_rpsSelOrigin;
#endif
}
//------------------------------------------------------
// return pointer to end of selection
const CRecPos& CShwView::rpsSelEnd()
{
    ASSERT( bSelecting( eAnyText ) );

    if ( m_rpsCur > m_rpsSelOrigin )
        return m_rpsCur;
    else
        return m_rpsSelOrigin;
}

#define TEXTOUT_NORMAL      1
#define TEXTOUT_HIGHLIGHT   2

void TextOutClip( CDC* pDC, CLangEnc* plng, const char* psz, int len, 
                  CRect* prect, CMarker* pmkr, int iSelect,
                  UINT nOptions ,int ixBegin, int iyBegin) 
{
    // 1998-03-05 MRP
    ASSERT( plng );
    if ( iSelect == TEXTOUT_NORMAL || iSelect == TEXTOUT_HIGHLIGHT )
        {
        BOOL bHighlight = ( iSelect == TEXTOUT_HIGHLIGHT );
        if (pmkr)
            pmkr->SetTextAndBkColorInDC( pDC, bHighlight );
        else
            plng->SetTextAndBkColorInDC( pDC, bHighlight );
        }

    plng->ExtTextOutLng( pDC, ixBegin, iyBegin, nOptions, prect, psz, len, 0 );

    if ( iSelect == TEXTOUT_HIGHLIGHT ) // If highlighted, restore
        {  
        if (pmkr)
            pmkr->SetTextAndBkColorInDC( pDC, FALSE );
        else
            plng->SetTextAndBkColorInDC( pDC, FALSE );
        }
}

void CShwView::DrawSubstring( CDC* pDC, const CRect& rect,
        const CRecPos& rpsB, const CRecPos& rpsE,
        BOOL bDrawSpace, BOOL bHighlight )
{
    int iTmpPos = 0; // !!! Debug show
    ASSERT( pDC );
    CMarker* pmkr = rpsB.pmkr(); // Get substring marker
    const char* psz = rpsB.psz(); // Get beginning of substring
    int iLen = rpsE.psz() - psz; // Get length of substring
    if ( iLen == 0 )
        return;
    CFont* pfnt = (CFont*)pmkr->pfnt(); // Get font of substring
    pDC->SelectObject( pfnt );
    int xLeft = pDC->GetCurrentPosition().x; // Get starting pos of draw
    int xRight = xLeft + pmkr->plng()->GetTextWidth( pDC, psz, iLen); // Calculate end of draw
    CRect rectClip = rect;
    if ( xLeft >= rectClip.left && xLeft < rectClip.right ) // If starting pos is inside clip region, set clip left to starting pos
        rectClip.left = xLeft;
    else // Else (starting pos outside clip region) don't display at all
        return;
    if ( xRight >= rectClip.left && xRight < rectClip.right ) // If ending pos is inside clip region, set clip right to starting pos
        rectClip.right = xRight + 1; // 5.98k Fix bug of not displaying final overstrikes in Win98

    // Align the baselines
    int yTop = pDC->GetCurrentPosition().y; // Get current baseline
    int iAscentCur = pmkr->iAscent(); // Get current font ascent (height from baseline to top of character cell)
    int iAscentField = rpsB.pfld->pmkr()->iAscent(); // Get ascent of font of surrounding field
    int iBaselineAdjustment = iAscentField - iAscentCur; // Calculate difference
    if ( iBaselineAdjustment != 0 ) // If difference, move starting pos up or down to compensate
        pDC->MoveTo( pDC->GetCurrentPosition().x, yTop + iBaselineAdjustment );

    rpsB.pmkr()->ExtTextOut( pDC, 0, 0, ETO_OPAQUE | ETO_CLIPPED, &rectClip,
        psz, iLen, iLen);  // 1998-03-06 MRP
    // int iSelect = bHighlight ? TEXTOUT_HIGHLIGHT : TEXTOUT_NORMAL; // Set select based on highlight
    //09-09-1997 Changed the Parameterlist
    // TextOutClip( pDC, rpsB.pfld->plng(), psz, iLen, 
    //              &rectClip, rpsB.pfld->pmkr(), iSelect ); // Output text in clip rectangle
        
    static const char s_pszSpace[] = " ";
    if ( bDrawSpace ) // If space should be drawn after the text, do it
        {
        xLeft = pDC->GetCurrentPosition().x; // Get starting pos 
        int dx = pmkr->plng()->GetTextWidth( pDC, s_pszSpace, 1 ); // Measure length of space
        xRight = xLeft + dx; // Calculate end of draw
        rectClip = rect;
        if ( xLeft > rectClip.left && xLeft < rectClip.right ) // If starting pos is inside clip region, set clip left to starting pos
            rectClip.left = xLeft;
        else // Else (starting pos outside clip region) don't display at all
            return;
        if ( xRight > rectClip.left && xRight < rectClip.right ) // If ending pos is inside clip region, set clip right to starting pos
            rectClip.right = xRight;

        rpsB.pmkr()->ExtTextOut( pDC, 0, 0, ETO_OPAQUE | ETO_CLIPPED,
            &rectClip, s_pszSpace, 1, 1);  // 1998-03-06 MRP
        //09-09-1997 Changed the Parameterlist
        // TextOutClip( pDC, rpsB.pfld->plng(), s_pszSpace, 1, 
        //             &rectClip, rpsB.pfld->pmkr(), iSelect ); // Output text in clip rectangle

        iTmpPos = pDC->GetCurrentPosition().x; // !!! See where it went
        }

    if ( iBaselineAdjustment != 0 ) // If position was moved up or down to adjust for baseline, restore it
        pDC->MoveTo( pDC->GetCurrentPosition().x, yTop );
}  // CShwView::DrawSubstring                           
//---------------------------------------------------------
//09-02-1997
BOOL CShwView::bDrawStringEnd(CDC* pDC, const CRect& rect, 
                              CRecPos& rpsBegin, CRecPos& rpsEnd, 
                              BOOL bDrawSpace, BOOL bHighlight)                                      
{
    ASSERT(pDC);
    BOOL bNoStringCut = TRUE;             //Becomes FALSE if the string is 
                                          //longer than the remaining Paintarea
    CMarker* pmkr = rpsBegin.pmkr();        //Get marker from the string
    CFont* pfnt = (CFont*)pmkr->pfnt();   //Get font of string
    const char* psz = rpsBegin.psz();
    int iLen = rpsEnd.psz() - psz;
    if (iLen == 0)
        return TRUE;
    int ixEndLastDraw = pDC->GetCurrentPosition().x;  //Get end position of draw
        
    //Allign the baselines
    int yTop = pDC->GetCurrentPosition().y;  //Get current baseline
    int iAscentCur = pmkr->iAscent();      //Get current font ascent
    int iAscentField = rpsBegin.pfld->pmkr()->iAscent();  // Get ascent of font of surrounding field
    int iBaselineAdjustment = iAscentField - iAscentCur; // Calculate difference
   
    // Calculate the start position for drawing
    // 1998-03-06 MRP
    int ixWidth = ixEndLastDraw - rect.left;
    if ( ixWidth < 0 )
        return FALSE;  // bClipAlignedText requires 0 <= ixWidth

    bNoStringCut = !pmkr->plng()->bClipAlignedSubstring(pDC, TRUE,
        &psz, &iLen, &ixWidth);
    if ( iLen == 0 )
        return FALSE;  // No characters will fit into the available width

    int ixStartDraw = ixEndLastDraw - ixWidth;  // Right-align

    ASSERT(rect.left <= ixStartDraw);
    pDC->SelectObject(pfnt);

      //the string shall appear in rectClip
    CRect rectClip = rect;
    rectClip.right = ixEndLastDraw;
    rectClip.left = ixStartDraw;
   
      // Move to start position for drawing
    pDC->MoveTo( ixStartDraw, yTop + iBaselineAdjustment );

    // Output text in clip rectangle
    rpsBegin.pmkr()->ExtTextOut( pDC, 0, 0, ETO_OPAQUE | ETO_CLIPPED,
        &rectClip, psz, iLen, iLen);  // 1998-03-06 MRP
    // Set select based on highlight
    // int iSelect = bHighlight ? TEXTOUT_HIGHLIGHT : TEXTOUT_NORMAL; 
    // TextOutClip( pDC, rpsBegin.pfld->plng(), psz, iLen, 
    //             &rectClip, rpsBegin.pfld->pmkr(), iSelect ); // Output text in clip rectangle

      // Update the position of the first character
    pDC->MoveTo (ixStartDraw,yTop);
   
    if ( bNoStringCut && bDrawSpace)
        {
        //Draw a space
        ixEndLastDraw = ixStartDraw;
        static const char s_pszSpace[] = " ";
        //Calculation for the point for start drawing
        int ixStartDraw = ixEndLastDraw - pmkr->plng()->GetTextWidth(pDC, s_pszSpace, 1);
        if (ixStartDraw < rect.left)
             // There is no place for a new space on the screen
            return FALSE;

        //There is place for space on the screen
        // Set rectClip
        rectClip = rect;
        rectClip.right = ixEndLastDraw;
        rectClip.left = ixStartDraw;
        
        // Set start position
        pDC->MoveTo(rectClip.left, yTop + iBaselineAdjustment);
        
        // Output space in clip rectangle
        rpsBegin.pmkr()->ExtTextOut( pDC, 0, 0, ETO_OPAQUE | ETO_CLIPPED,
            &rectClip, s_pszSpace, 1, 1);  // 1998-03-06 MRP
        // TextOutClip( pDC, rpsBegin.pfld->plng(), s_pszSpace, 1, 
        //             &rectClip, rpsBegin.pfld->pmkr(), iSelect ); 
                    
        //Update
        pDC->MoveTo(ixStartDraw, yTop);
        } // if - Draw a space
          
    return bNoStringCut;

} // CShwView::bDrawStringEnd
//------------------------------------------------------
void CShwView::FldContOut( CDC* pDC, CField* pfld, const char* pszStart ) // Output field contents, pszStart is used if top of field is not visible in view
{
#ifdef _MAC
    GrafPtr pgrafport = CheckoutPort( *pDC, CA_FONT );
    CheckinPort( *pDC, CA_NONE );
#endif
    CRect rectClip;
    pDC->GetClipBox( &rectClip ); // Get the clip rectangle to be repainted, for optimization to eliminate painting lines of field that are not visible
    CBrush brush( ::GetSysColor( COLOR_WINDOW ) );
    CBrush brushSel( ::GetSysColor( COLOR_HIGHLIGHT ) );
    BOOL bStart = pszStart != NULL; // True if pszStart was passed in
    if ( !pszStart )
        pszStart = (const char*)*pfld;
    CRecPos rpsCur( 0, pfld, m_prelCur->prec() ); 
    rpsCur.SetPos( pszStart );

    pDC->MoveTo( m_iFldStartCol, pDC->GetCurrentPosition().y );
    const char* pszSelBeg = NULL;
    const char* pszSelEnd = NULL;
    if ( iGetSelEnd( pfld ) ) // If any selection in this field (returns end of any selected text in the field)
        {
        int iOffset = pszStart - *pfld;
        pszSelBeg = pszStart + iGetSelBeg( pfld ) - iOffset; // Set up select begin and end pointers
        pszSelEnd = pszStart + iGetSelEnd( pfld ) - iOffset;
        }
    BOOL bFillMkrArea = bStart; // Set to fill marker area if not on first line where marker was painted

    if ( rpsCur.bInterlinear() ) //  If an interlinear field, do aligned output
        {
		CRecPos rpsFirstLine = rpsCur; // 1.5.1cc 
		while ( rpsFirstLine.pfldPrev() && rpsFirstLine.bInterlinear() && !rpsFirstLine.bFirstInterlinear() ) // 1.5.1cc Find first interlinear of this bundle
			rpsFirstLine.pfld = rpsFirstLine.pfldPrev(); // 1.5.1cc Fix bug of crash from pfld becoming null
		BOOL bRTLInterlinear = rpsFirstLine.pfld->plng()->bRightToLeft();  // 1.5.1cc See if rtl interlinear bundle // 1.5.1ce 

        const char* pszEnd = pszStart; // General end of word or word plus space
        const char* pszLineStart = pszStart; // Remember start of line
        pszEnd = pfld->plng()->pszSkipSpace( pszEnd ); // Move to start of first word
        BOOL bBlankBeforeWord = pszEnd > pszLineStart; // if blank before word, we need to blank out area up to first word
        int iLine = pDC->GetCurrentPosition().y; // Current line position
        CRect rectWd = rectClip; // Clip rectangle for word
        rectWd.left = m_iFldStartCol - 1; // Set left to field start // AB 6-26-97 Fix bug of not showing first overstrike
        rectWd.top = pDC->GetCurrentPosition().y;
        rectWd.bottom = rectWd.top + iLineHt( pfld );
        BOOL bEmptyField = TRUE; // Signal to clear empty field

        if ( bFillMkrArea ) // If blank line, fill in marker area with correct color 
            // AB: Putting this here fixes a bug of not blanking marker area if top line of window is second line of interlinear. This may be more efficiently handled lower down, but ps
            { // AB: This code may be more efficient than the fill marker area below in non-interlinear.
            CBrush* pbrush = &brush; // Clear marker area, default to unselected brush
            if ( ( bSelecting( eLines|eFields ) // If marker is selected, set highlight colors for it
                    && ( pszSelBeg <= pszEnd && pszSelEnd >= pszEnd ) )
                || ( bSelecting( eText ) 
                    && ( pszSelBeg == pszEnd - 1 && pszSelBeg != pszSelEnd ) ) )
                pbrush = &brushSel;
            CRect rect(0, rectWd.top, m_iFldStartCol - eRecordDividerWidth, rectWd.top + iLineHt(pfld)); // Make marker rect
            pDC->FillRect( &rect, pbrush ); // Fill in marker area
            bFillMkrArea = FALSE; // Don't do marker area again until next line
            }
		if ( bRTLInterlinear ) // 1.5.1cc If rtl, clear full width
			{
            CBrush* pbrush = &brush; // Clear marker area, default to unselected brush
            CRect rect( m_iFldStartCol, rectWd.top, rectClip.right, rectWd.top + iLineHt(pfld)); // Make marker rect
            pDC->FillRect( &rect, pbrush ); // Fill in marker area
//            pDC->FillRect( rectClip, pbrush ); // Fill in marker area
			}

        while ( TRUE ) // While more words in field
            {
            if ( !*pszEnd || *pszEnd == '\n' ) // If end of line, move to next line
                {
                if ( bEmptyField ) // If field was empty, clear line
                    pDC->FillRect( &rectWd, &brush ); // Blank out line
                iLine += iLineHt( pfld ); // Move to next line
                pDC->MoveTo( m_iFldStartCol, iLine );
                if ( !*pszEnd ) // If at end of field, break
                    break;
                // Blank out marker area and line
                rectWd.top = pDC->GetCurrentPosition().y; // Set word to whole line boundaries for blanking out line
                rectWd.bottom = rectWd.top + iLineHt( pfld );
                rectWd.left = m_iFldStartCol - 1; // Set left to field start // AB 6-26-97 Fix bug of not showing first overstrike
                rectWd.right = rectClip.right; // Set right to end of line
                CBrush* pbrush = &brush; // Clear marker area, default to unselected brush
                if ( ( bSelecting( eLines|eFields ) // If marker is selected, set highlight colors for it
                        && ( pszSelBeg <= pszEnd && pszSelEnd > pszEnd ) )
                    || ( bSelecting( eText ) 
                        && ( pszSelBeg == pszEnd - 1 && pszSelBeg != pszSelEnd ) ) )
                    pbrush = &brushSel;
                CRect rect(0, rectWd.top, m_iFldStartCol - eRecordDividerWidth, rectWd.top + iLineHt(pfld)); // Make marker rect
                pDC->FillRect( &rect, pbrush ); // Fill in marker area
                pDC->FillRect( &rectWd, &brush ); // Blank out line

                iLine += iLineHt( pfld ); // Move to next line
                pDC->MoveTo( m_iFldStartCol, iLine );
				if ( *pszEnd == '\n' && *(pszEnd + 1) ) // 5.97k If more after nl, cut it off
					{
			        rpsCur.pfld->Trim(); // Trim trailing nl's and spaces
			        rpsCur.pfld->SetContent( rpsCur.pfld->sContents() + '\n' ); // Add nl at end of bundle
					}
				break; // Don't show any more after nl
                }
            bEmptyField = FALSE; // Not an empty field

            const char* pszStart = pszEnd; // Remember start of word
            pszEnd = pfld->plng()->pszSkipNonWhite( pszEnd ); // Find end of word
            int iWdLen = pszEnd - pszStart; // Find length of word
            int iFixDist = pszStart - pszLineStart; // Find fixed position distance from start
            int iCol = m_iFldStartCol + iInterlinPixelPos( rpsCur, iFixDist ); // 6.0a Get word start column

            const char* pszNextWd = pfld->plng()->pszSkipSpace( pszEnd ); // find start of next word
            int iColEnd = m_iFldStartCol + iInterlinPixelPos( rpsCur, pszNextWd - pszLineStart ); // 6.0a calculate width of this column

            if ( bBlankBeforeWord ) // first char in line is space, need to blank out over to first word
                {
                iCol = m_iFldStartCol; // Start blanking rect at front of line
                bBlankBeforeWord = FALSE;
                }
            rectWd.left = iCol - 1; // Set left of word rect // AB 6-26-97 Fix bug of not showing first overstrike
            if ( !*pszNextWd || *pszNextWd == '\n' ) // If last word in line, blank out space after word
                rectWd.right = rectClip.right; // Set end to far right of line
            else
                rectWd.right = iColEnd;

			CPoint pntBeg;
			pDC->MoveTo( iCol, iLine ); // Move to starting point of word
			pntBeg = pDC->GetCurrentPosition();
			if ( bRTLInterlinear ) // 1.5.1cc If rtl, adjust positioning for rtl
				{
				int iTextWidth = pfld->plng()->GetTextWidth( pDC, pszStart, iWdLen ); // 1.5.1cc Measure width to right justify
				CRect rectWdRTL = rectWd; // 1.5.1cc 
				rectWdRTL.left = iMargin() + m_iFldStartCol - rectWd.right; // 1.5.1cc Adjust left for rtl
				rectWdRTL.right = iMargin() + m_iFldStartCol - rectWd.left; // 1.5.1cc Adjust right for rtl
				if ( rectWdRTL.left > rectWdRTL.right - iTextWidth ) // 1.5.1cc Correct for cutting off left side
					rectWdRTL.left = rectWdRTL.right - iTextWidth; // 1.5.1cc 
				if ( rectWdRTL.left < m_iFldStartCol ) // 1.5.1cc Correct for last box too long
					rectWdRTL.left = m_iFldStartCol; // 1.5.1cc 
				if ( rectWdRTL.right < rectWdRTL.left ) // 1.5.1cc 
					rectWdRTL.right = rectWdRTL.left; // 1.5.1cc 
				int iColRTL = rectWdRTL.right - iTextWidth; // 1.5.1cc Find starting point to right justify
				pDC->MoveTo( iColRTL, iLine ); // Move to starting point of word
				TextOutClip( pDC, pfld->plng(), pszStart, iWdLen, &rectWdRTL, pfld->pmkr(), TEXTOUT_NORMAL ); // Paint entire word normally first, then repaint entire word with rectangle on highlighted part
				}
			else
				{
				TextOutClip( pDC, pfld->plng(), pszStart, iWdLen, &rectWd, pfld->pmkr(), TEXTOUT_NORMAL ); // Paint entire word normally first, then repaint entire word with rectangle on highlighted part
				}

			pDC->MoveTo( pntBeg ); // Move back to start
            if ( pszSelBeg < pszSelEnd && pszSelEnd > pszStart && pszSelBeg < pszEnd )  // If selection on and selected text in this run, highlight selected part
                {
                int xLeft = iCol; // Get starting pos of draw
                int xRight = iCol + iCharToPixel( pDC, pfld, pszStart, iWdLen, iWdLen ); // Get end of draw
                CRect rectSel( xLeft, pntBeg.y, xRight, pntBeg.y + iLineHt( pfld ) ); // Init rect to entire area

                int xSelLeft = xLeft; // Initialize left end of selection
                int xSelRight = xRight; // Initialize right end of selection
                if ( pszSelBeg > pszStart ) // If selection starts in this run, set left to start
                    {
                    int x = pntBeg.x + iCharToPixel( pDC, pfld, pszStart, iWdLen, pszSelBeg - pszStart ); // Get pos of low end of selection
                    if ( pfld->plng()->bRightToLeft() ) // If right to left, this is the right end
                        xSelRight = x;
                    else
                        xSelLeft = x;
                    }
                if ( pszSelEnd < pszEnd ) // If selection ends in this run, set right to end
                    {
                    int x = pntBeg.x + iCharToPixel( pDC, pfld, pszStart, iWdLen, pszSelEnd - pszStart ); // Get pos of low end of selection
                    if ( pfld->plng()->bRightToLeft() ) // If right to left, this is the left end
                        xSelLeft = x;
                    else
                        xSelRight = x;
                    }
                if ( pszSelEnd > pszEnd && *pszEnd && *pszEnd != '\n' ) // If select is past end of word, highlight inter-word gap, don't select past last word in line unless followed by a space
                    {
                    CRect rect( xRight, rectWd.top, iColEnd, rectWd.bottom );
                    if ( rect.left < rect.right )
                        pDC->FillRect( &rect, &brushSel ); // blank out over to next word
                    }
                rectSel.SetRect( xSelLeft, pntBeg.y, xSelRight, pntBeg.y + iLineHt( pfld ) ); // Set rect to selected part
                
                //09-09-1997 Changed the Parameterlist
                TextOutClip( pDC, pfld->plng(), pszStart, iWdLen,
                             &rectSel, pfld->pmkr(), TEXTOUT_HIGHLIGHT ); // Highlight selected part
                }

            pszEnd = pszNextWd; // Move to start of next word
            m_iMaxLineWd = max( iColEnd, m_iMaxLineWd );
			if ( pfld->plng()->bRightToLeft() ) // 1.1hn If right to left, add extra because rtl ends at left side
	            m_iMaxLineWd = max( iColEnd + 100, m_iMaxLineWd );
            }
        }   
    else // Else, not interlinear field
        {
        CRect rectLine = rectClip; // Clip rectangle for whole line
        rectLine.left = m_iFldStartCol - 1; // Set left to field start // AB 6-26-97 Fix bug of not showing initial overstrike
        CRecPos rpsB; // Begin substring, set by substring move
        CRecPos rpsE; // End substring, set by substring move

        do  {
            rpsCur.MoveRightPastViewSubstring( &rpsB, &rpsE ); // Move rps past substring and set begin and end
            pszStart = rpsB.psz(); // Remember start of substring
            const char* pszEnd = rpsE.psz(); // Remember end of substring
            int iLen = pszEnd - pszStart;
            rectLine.top = pDC->GetCurrentPosition().y;
            rectLine.bottom = rectLine.top + iLineHt( pfld );

            if ( bFillMkrArea ) // fill in marker area with correct color
                {
                if ( ( bSelecting( eLines|eFields ) // If marker is selected, set highlight colors for it
                        && ( pszSelBeg <= rpsB.psz() && pszSelEnd >= rpsB.psz() ) )
                    || ( bSelecting( eText ) 
                        && ( pszSelBeg == rpsB.psz() - 1 && pszSelBeg != pszSelEnd ) ) )
                    {
                    pDC->SetTextColor( ::GetSysColor( COLOR_HIGHLIGHTTEXT ) );
                    pDC->SetBkColor( ::GetSysColor( COLOR_HIGHLIGHT ) );
                    }
                CPoint pnt = pDC->GetCurrentPosition(); // Save current pos for restoration
                pDC->MoveTo( 0, pnt.y ); // Move to marker start pos
                CRect rect(0, pnt.y, m_iFldStartCol - eRecordDividerWidth, pnt.y + iLineHt(pfld)); // Make marker rect
				Str8 sText = ""; // 1.4qta
				CString swText = swUTF16( sText ); // 1.4qta
                pDC->ExtTextOut( 0, 0, ETO_CLIPPED | ETO_OPAQUE, &rect, swText, 0, NULL ); // Fill marker area with correct color // 1.4qta
                pDC->MoveTo( pnt ); // Restore position for line contents
                bFillMkrArea = FALSE; // Don't do marker area again until next line
                }

            CFont* pfnt = (CFont*)rpsB.pmkr()->pfnt();  // substring font
            pDC->SelectObject( pfnt );
#ifdef _MAC
            GrafPtr pgrafport = CheckoutPort( *pDC, CA_FONT );
            CheckinPort( *pDC, CA_NONE );
#endif
            // Align the baselines of subfields with the primary baseline of the field
            int iBaselineAdjustment = 0;
            int yTop = pDC->GetCurrentPosition().y; // Remember primary top position
            if ( rpsB.bInSubfield() )
                {
                int iAscentCur = rpsB.pmkr()->iAscent();
                int iAscentField = rpsB.pfld->pmkr()->iAscent();
                int iBaselineAdjustment = iAscentField - iAscentCur;
                if ( iBaselineAdjustment != 0 )
                    pDC->MoveTo( pDC->GetCurrentPosition().x, yTop + iBaselineAdjustment );
                }

            CPoint pntBeg = pDC->GetCurrentPosition();
            
	        pDC->FillRect( &rectLine, &brush ); // Blank out line
			int iRTLOffset = 0; // 1.2ba Show right to left fields on right margin
			if ( pfld->plng()->bRTLRightJustify() ) // 1.2ba Show right to left fields on right margin // 1.2bd Make optional
				{
				iRTLOffset = iMargin() - pfld->plng()->GetTextWidth( pDC, pszStart, iLen ); // 1.2ba Calculate offset needed to put right to left on right margin
	            CPoint pntBegRTL = pDC->GetCurrentPosition();
				pntBegRTL.x += iRTLOffset;
	            pDC->MoveTo( pntBegRTL ); // 1.2ba Show right to left fields on right margin
				}
            TextOutClip( pDC, pfld->plng(), pszStart, iLen,
                         &rectLine, pfld->pmkr(), TEXTOUT_NORMAL ); // Paint entire run normally first, then repaint entire run with rectangle on highlighted part
            CPoint pnt = pDC->GetCurrentPosition();
            m_iMaxLineWd = max( pnt.x, m_iMaxLineWd ); // Note length of longest line
            pDC->MoveTo( pntBeg ); // Move back to start

            if ( pszSelBeg < pszSelEnd && pszSelEnd > pszStart && pszSelBeg < pszEnd )  // If selection on and selected text in this run, highlight selected part
                {
                int xLeft = pntBeg.x; // Get starting pos of draw
                int xRight = pntBeg.x + iCharToPixel( pDC, pfld, pszStart, iLen, iLen ); // Calculate end of draw
                CRect rectSel( xLeft, pntBeg.y, xRight, pntBeg.y + iLineHt( pfld ) ); // Set rect to entire run

                int xSelLeft = xLeft; // Initialize left end of selection
                int xSelRight = xRight; // Initialize right end of selection
                if ( pfld->plng()->bRightToLeft() ) // If right to left, this is the right end
					{
					xSelLeft += iRTLOffset; // 1.2ba Show right to left fields on right margin
					xSelRight += iRTLOffset; // 1.2ba Show right to left fields on right margin
                    xSelRight = pntBeg.x + iCharToPixel( pDC, pfld, pszStart, iLen, 0 ); // Correct for right-to-left;
					}
                if ( pszSelBeg >= pszStart ) // If selection starts in this run, set left to start // 1.2ba Show right to left fields on right margin
                    {
                    int x = pntBeg.x + iCharToPixel( pDC, pfld, pszStart, iLen, pszSelBeg - pszStart ); // Get pos of low end of selection
                    if ( pfld->plng()->bRightToLeft() ) // If right to left, this is the right end
                        xSelRight = x + iRTLOffset; // 1.2ba Show right to left fields on right margin
                    else
                        xSelLeft = x;
                    }
                if ( pszSelEnd < pszEnd ) // If selection ends in this run, set right to end
                    {
                    int x = pntBeg.x + iCharToPixel( pDC, pfld, pszStart, iLen, pszSelEnd - pszStart ); // Get pos of low end of selection
                    if ( pfld->plng()->bRightToLeft() ) // If right to left, this is the left end
                        xSelLeft = x + iRTLOffset; // 1.2ba Show right to left fields on right margin
                    else
                        xSelRight = x;
                    }
                rectSel.SetRect( xSelLeft, pntBeg.y, xSelRight, pntBeg.y + iLineHt( pfld ) ); // Set rect to selected part
                
				if ( pfld->plng()->bRTLRightJustify() ) // 1.2ba Show right to left fields on right margin //1.2bd Make optional
					{
					CPoint pntBegRTL = pDC->GetCurrentPosition();
					pntBegRTL.x += iRTLOffset;
					pDC->MoveTo( pntBegRTL ); // 1.2ba Show right to left fields on right margin
					}
                TextOutClip( pDC, pfld->plng(), pszStart, iLen, 
                             &rectSel, pfld->pmkr(), TEXTOUT_HIGHLIGHT ); // Highlight selected part
                }

            if ( iBaselineAdjustment != 0 ) // If baseline was adjusted, restore it to standard height
                pDC->MoveTo( pDC->GetCurrentPosition().x, yTop - iBaselineAdjustment );

            if ( rpsE.bEndOfFieldOrLine() )
                {
                pDC->MoveTo( m_iFldStartCol, pnt.y + iLineHt( pfld ) ); 
                bFillMkrArea = TRUE;
                }
            } while ( !rpsE.bEndOfField() );
        }
}   
//------------------------------------------------------
void CShwView::DrawRec( CDC* pDC ) // Draw the record
{
    int iOldMaxLineWd = m_iMaxLineWd; // remember setting - FldContOut() may modify it

    CFont* pfntOld = pDC->SelectObject( Shw_pProject()->pfntMkrs() ); // save view's default font

    TEXTMETRIC tm;
    pDC->GetTextMetrics( &tm );
    int iMkrHt = tm.tmHeight; // get height of marker font for horizontal alignment with field contents

    CRecord* prec = m_prelCur->prec();
    ASSERT( prec ); 
    
    CRect rectClip;
    pDC->GetClipBox( &rectClip ); // get coords for area that needs repainted
    CRect rect;
    GetClientRect( &rect );
    rect.OffsetRect( GetScrollPosition() );
    rect.right = m_iFldStartCol-eRecordDividerWidth;  // clip marker and style names at start of field contents
    pDC->SetTextAlign( TA_UPDATECP );
    pDC->SetTextColor( ::GetSysColor( COLOR_WINDOWTEXT ) );
    pDC->SetBkColor( ::GetSysColor( COLOR_WINDOW ) );
    pDC->MoveTo( 0, 0 );

//  for ( CField* pfld = prec->pfldFirst(); pfld; pfld = prec->pfldNext( pfld ) )

    // 01/12/2000 TLB : The following section of code replaces some former ASSERT lines. We now take
    // the approach of detecting the problems and forcing an on-the-fly fix rather than sending up
    // an ASSERT that effectively causes a crash. A better fix would probably be to try to keep these
    // values from becoming invalid, but we couldn't come up with a way to do this.
    // 1) This first part fixes bugs caused by a ref field or datestamp field in the upper left corner
    //    of a window that gets shortened and thereby invalidates the upper left character position.
    if ( !bPositionValid( m_rpsUpperLeft ) ) // our frame of reference may have been deleted by another view
        {
		if ( m_rpsUpperLeft.pfld ) // 1.5.0kg 
			{
	        m_rpsUpperLeft.iChar = m_rpsUpperLeft.pfld->GetLength();
			m_rpsUpperLeft.bMoveHome();
			ASSERT(bPositionValid( m_rpsUpperLeft ));
			}
        }
    // 2) The remaining fix corrects problems where datestamps, ref fields, etc. are shortened (by Shoebox)
    //    such that a previously valid selection range now has an invalid beginning and/or ending point.
    if ( bSelecting( eAnyText ) )
        {
        if ( !bPositionValid( m_rpsCur ) )
            {
            if ( bPositionValid( m_rpsSelOrigin ) )
                m_rpsCur = m_rpsSelOrigin;
            else
                m_rpsCur = m_rpsUpperLeft;
            m_iSelection = 0; // 1.5.8b Fix bug of not remembering find in specific field 
            }
        else if ( !bPositionValid( m_rpsSelOrigin ) )
            {
            m_iSelection = 0; // 1.5.8b Fix bug of not remembering find in specific field
            }
        }

    const char* pszStart;
    if ( m_rpsUpperLeft.iChar ) // 1.1ke Undo wrong attempt to fix bog of crash on multi-line hidden field // !bHiddenField( m_rpsUpperLeft.pfld ) ) // 1.1hh Fix bug of crash on hide fields
        pszStart = m_rpsUpperLeft.psz(); // handle start of window being in middle of field
    else
        pszStart = NULL;

	RememberInterlinearWidths( TRUE );
    for ( CField* pfld = m_rpsUpperLeft.pfld; pfld; pfld = prec->pfldNext( pfld ) )
        {
        if ( bHiddenField( pfld ) ) // If hidden field, don't show it 1.0ba
			continue;
        if ( pszStart )
            {
            CFont* pfnt = (CFont*)pfld->pmkr()->pfnt(); // had to cast away const attribute
            pDC->SelectObject( pfnt ); // get font for displaying field contents
            FldContOut( pDC, pfld, pszStart ); // Output content
            pszStart = NULL;
            continue;
            }

        int iMkrOffset = (iLineHt( pfld ) - iMkrHt + 1) / 2; // if marker font is smaller than field
        int iYPos = pDC->GetCurrentPosition().y; // font, adjust vertically so marker is aligned with contents

        pDC->SelectObject( Shw_pProject()->pfntMkrs() );

        pDC->MoveTo( eDividerPad, pDC->GetCurrentPosition().y + iMkrOffset );
        pDC->SetTextColor( ::GetSysColor( COLOR_WINDOWTEXT ) );
        if ( bSelecting( eAnyText ) )
            {
            CRecPos rps( 0, pfld, m_prelCur->prec() );
            CRecPos rpsBeg = rpsSelBeg();
            if ( ( bSelecting(eLines|eFields) && rps == rpsBeg ) // hilite marker if selecting by line
                    || rps > rpsBeg && rps <= rpsSelEnd() )
                {
                pDC->SetTextColor( ::GetSysColor( COLOR_HIGHLIGHTTEXT ) ); // first marker included in selection
                pDC->SetBkColor( ::GetSysColor( COLOR_HIGHLIGHT ) );
                }
            }
        rect.top = pDC->GetCurrentPosition().y - iMkrOffset;
        rect.bottom = rect.top + iLineHt( pfld );
        pfld->pmkr()->DrawMarker(pDC, rect,
            m_bViewMarkerHierarchy, m_bViewMarkers, m_bViewFieldNames);
        CFont* pfnt = (CFont*)pfld->pmkr()->pfnt(); // had to cast away const attribute
        pDC->SelectObject( pfnt ); // get font for displaying field contents

        if ( iMkrOffset ) // marker was displayed lower, need to move back up for field painting
            {
            CPoint pnt = pDC->GetCurrentPosition();
            pnt.y -= iMkrOffset;
            pDC->MoveTo( pnt );
            }
        FldContOut( pDC, pfld ); // Output content
        if ( pDC->GetCurrentPosition().y >= rectClip.bottom )
            break; // don't draw any more than we need to
        }
	RememberInterlinearWidths( FALSE );

    CBrush brushPad( ::GetSysColor( COLOR_WINDOW ) );
    CBrush brushBar( ::GetSysColor( COLOR_WINDOWTEXT ) );
    rect = rectClip;
    rect.left = m_iFldStartCol - eRecordDividerWidth;
    rect.right = rect.left + eDividerPad;
    pDC->FillRect( &rect, &brushPad ); // draw pad on left side of vertical bar
    rect.left += eDividerPad;
    rect.right = rect.left + eDividerBar;
    pDC->FillRect( &rect, &brushBar ); // draw vertical bar
    rect.left += eDividerBar;
    rect.right = rect.left + eDividerPad + eDividerTextPad;
    pDC->FillRect( &rect, &brushPad ); // draw pad on right side of vertical bar
    
    int y = pDC->GetCurrentPosition().y;
    if ( y < rectClip.bottom )
        {
        rect = rectClip;
        rect.top = y;
        rect.right = m_iFldStartCol - eRecordDividerWidth;
        pDC->FillRect( &rect, &brushPad ); // blank out unused portion of view
        rect.left = rect.right + eRecordDividerWidth;
        rect.right = rectClip.right;
        pDC->FillRect( &rect, &brushPad ); // blank out unused portion of view
        }
    // we just painted a line longer than before
    if ( GetFocus() == this && iOldMaxLineWd != m_iMaxLineWd )
        {
        SetScroll( FALSE ); // need to update horizontal scroll bar
        if ( !m_bDontCallSetCaretPosAdj ) // calling SetCaretPosAdj() causes problems
            SetCaretPosAdj(); //  when scrolling down into longer text using the scroll bar. 
        }
    pDC->SelectObject( pfntOld );
}

BOOL CShwView::bHiddenField( CField* pfld ) // Return true if hidden field, including tests for various exceptoinal cases
{
	if ( !ptyp()->bHideFields() ) // 1.0be If not hiding any fields, return false
		return FALSE;
	CMarker* pmkr = pfld->pmkr();
	if ( pmkr == ptyp()->pmkrRecord() ) // If field is record marker, don't hide it
		return FALSE;
	if ( pmkr == pind()->pmkrPriKey() ) // If field is primary sort key, don't hide it
		return FALSE;
	return pmkr->bHidden(); // If all else OK, then return hidden attribute of marker
}

void CShwView::SetStatusRecordAndPrimaryKey(const CNumberedRecElPtr& pnrl)
{
    if ( (CRecLookEl*)pnrl != NULL)  // 1998-12-02 MRP
        {
        m_sRecordFld = pnrl->prec()->sKey();
        CSortOrder::s_SortableSubString(&m_sRecordFld);
        m_sKeyFld = (pnrl->pfldPriKey()) ? 
                        pnrl->pfldPriKey()->sContents(): "";
        CSortOrder::s_SortableSubString(&m_sKeyFld);
        }
    else
        {
        m_sRecordFld.Empty();
        m_sKeyFld.Empty();
        }
}

//09-15-1997 Make some changes. For more details see status.h comments 
//refering this date.
void CShwView::UpdateStatusBar(CShwStatusBar* pStatus)
{
    CMarker* pmkrRec = ((CShwDoc*)GetDocument())->pindset()->pmkrRecord();
    // Update the record key.
    if ( m_prelCur )     //09-15-1997 Changed the conditions
        pStatus->SetRecordInfo( m_sRecordFld, pmkrRec->sMarker(),
            *pmkrRec->pfntDlg(), 
            pmkrRec->plng() );                          //09-15-1997
    else
        pStatus->SetRecordInfo( "", "", NULL, NULL );   //09-15-1997 Added parameter 4
    
    // Update the primary marker.  Leave blank if record marker and primary marker
    // are the same.
    // RNE 1996-05-21  Also there is the case where we have a primary marker but
    // its contents are empty.
    if ( m_prelCur && pmkrRec != m_pind->pmkrPriKey() )     // 09-15-1997 Changed the conditions
        pStatus->SetPriKeyInfo( m_sKeyFld, m_pind->pmkrPriKey()->sMarker(),
            *m_pind->pmkrPriKey()->pfntDlg(),
            m_pind->pmkrPriKey()->plng() );             // 09-15-1997
    else
        pStatus->SetPriKeyInfo( "", "", NULL, NULL );   // 09-15-1997 Added parameter 4

    // Record position
    long lCurrentRec = m_prelCur ? lGetCurrentRecNum() : 0L;
    pStatus->SetRecordPosInfo( lCurrentRec, lGetRecordCount() );
    pStatus->SetProjectName( (Shw_papp()->bProjectOpen()) ? 
                                sGetFileName(Shw_pProject()->pszPath()) : "");
}

//------------------------------------------------------
void CShwView::UpdateFilterSetCombo(CFilterSetComboBox* pcboFilterSet) // set contents and current filter in filter combo on toolbar
{
    pcboFilterSet->ChangeSet( m_pind->pindsetMyOwner()->ptyp()->pfilset(), m_pind->pfil() );
}

/*
We decided that soft wrap based on window size is not the way to do it,
    since WFW does not do that. It does a soft wrap, but it is based on margin
    settings, not on the current window size. For the moment only hard wrap is
    implemented, and the ability to rewrap including automatic wrap will be
    added later.
*/                                    

/* Move routines are core primitives for editing.
They are separate from the editing routines because they are more controlled.
Specifically, the move by character routines do not leave the line,
and the move by line routines do not leave the field. This makes them
useful for some kinds of operations.
*/
//------------------------------------------------------
BOOL CShwView::bMoveRight() // Move right one char
{
#ifdef SUBFIELD
    (void) m_rpsCur.bMoveRightPastSubfieldTags();  // 1996-10-08 MRP
#endif // SUBFIELD
    const char* psz = m_rpsCur.psz(); // Get current char
    if ( !*psz || *psz == '\n' ) // If at end of field or end of line, return false
        return FALSE;
    if ( m_rpsCur.bInterlinear() ) // If interlinear
        {
        if ( *psz == ' ' ) // If space, move past spaces, calculate new position, and return
            {
            psz = m_rpsCur.pfld->plng()->pszSkipSpace( psz + 1 ); // Move to start of next word
            m_rpsCur.SetPos( psz ); // Move current position to start of next word, or end of field
            m_pntCaretLoc.x = m_iFldStartCol + iInterlinPixelPos( m_rpsCur, m_rpsCur.iChar ); // 6.0a Get  word start column
            return TRUE;
            }
        }
	m_rpsCur.iChar += m_rpsCur.pfld->plng()->iCharNumBytes( psz ); // Move text position
    return TRUE;
}
//------------------------------------------------------
BOOL CShwView::bMoveDownLine() // Move down one line in field
{
    int iHt = iDnLine( m_rpsCur ); // try to move down a line
    if ( iHt ) // height of line if succeeded
        {
        m_pntCaretLoc.y += iHt;
        m_pntCaretLoc.x = m_iFldStartCol;
        return TRUE;
        }
    return FALSE;
}
//------------------------------------------------------
BOOL CShwView::bMoveDownField( BOOL bHide ) // Move down one field in record
{
    int iHt = iDnField( m_rpsCur, bHide ); // try to move down a field
    if ( iHt ) // height of line if succeeded
        {
        m_pntCaretLoc.y += iHt;
        m_pntCaretLoc.x = m_iFldStartCol;
        return TRUE;
        }
    return FALSE;
}
//------------------------------------------------------
BOOL CShwView::bMoveDown( BOOL bHide ) // Move down one line, including moving into next field if necessary
{ // Can't use mouse click routine like bMoveUp because mouse click uses this routine
    if ( bMoveDownLine() || bMoveDownField( bHide ) )
        return TRUE;
    return FALSE;   
}
//------------------------------------------------------
BOOL CShwView::bMoveUpLine() // Move up one line in field
{
    int iHt = iUpLine( m_rpsCur ); // try to move up a line
    if ( iHt ) // height of line if succeeded
        {
        m_pntCaretLoc.y -= iHt;
        m_pntCaretLoc.x = m_iFldStartCol;
        return TRUE;
        }
    return FALSE;
}
//------------------------------------------------------
BOOL CShwView::bMoveUpField( BOOL bHide ) // Move up one field in record
{
    int iHt = iUpField( m_rpsCur, bHide ); // try to up down a field
    if ( iHt ) // height of line if succeeded
        {
        m_pntCaretLoc.y -= iHt;
        m_pntCaretLoc.x = m_iFldStartCol;
        return TRUE;
        }
    return FALSE;
}
//------------------------------------------------------
BOOL CShwView::bMoveUp( BOOL bHide ) // Move up one line, including moving into previous field if necessary
{
    if ( bMoveUpLine() || bMoveUpField( bHide ) )
        return TRUE;
    return FALSE;   
}
//------------------------------------------------------
void  CShwView::MoveCtrlHome() // Ctrl Home, beginning of record, core primitive
{
    CRecord* prec = m_prelCur->prec(); // Get current record
    ASSERT( prec );
    m_rpsCur.SetPos(0, prec->pfldFirst(), prec);  // 1996-10-15 MRP
    // m_rpsCur.pfld = prec->pfldFirst(); // Get key field
    ASSERT( m_rpsCur.pfld );
    // m_rpsCur.iChar = 0; // Set cur char to begin
    m_pntCaretLoc.x = m_iFldStartCol;
    m_pntCaretLoc.y = -m_lVPosUL;   // upper left of visible window is 0 in y coordinate
}

//------------------------------------------------------
BOOL  CShwView::bMoveCtrlRight() // Ctrl Right arrow move, word right but stay in line
{   
    BOOL bMoved = FALSE;
    while ( !m_rpsCur.bIsWdEnd() && bMoveRight() ) // Move past non-white
        bMoved = TRUE;  
    while ( m_rpsCur.bIsWdSep() && bMoveRight() ) // Move past white 
        bMoved = TRUE;  
    return ( bMoved ); // If no movement, return false
}
//------------------------------------------------------
void CShwView::CharInsert( UINT nChar ) // Simple insert char at current pos primitive
{
    m_rpsCur.pfld->Insert( nChar, m_rpsCur.iChar );
}
void CShwView::StringInsert( const char* szChar ) // Simple insert char at current pos primitive
{
    m_rpsCur.pfld->Insert( szChar, m_rpsCur.iChar );
}
//------------------------------------------------------
void CShwView::CharDelete() // Simple delete current char primitive
{
	int iNumBytes = m_rpsCur.pfld->plng()->iCharNumBytes( m_rpsCur.psz() );
	m_rpsCur.pfld->Delete( m_rpsCur.iChar, iNumBytes );
}
// End of core primitives
// The next routines are also used as primitives so should not change their semantics
//------------------------------------------------------
BOOL CShwView::bEditHome() // go to beginning of line
{
    BOOL bMoved = bHome( m_rpsCur );
    m_pntCaretLoc.x = m_iFldStartCol;
    return bMoved;
}
//------------------------------------------------------
void  CShwView::EditEnd() // End, end of line, used as primitive
{
    while ( bMoveRight() ) // Move to end of line
        ;
}
// End of primitives

//------------------------------------------------------
BOOL CShwView::bMoveLeft() // Move left one char
{
    if ( m_rpsCur.iChar == 0 ) // If at beginning of field, return false
        return FALSE;

    const char* psz = m_rpsCur.psz(); // Get current char
    psz--; // Move to char we are stepping over
    if ( *psz == '\n' ) // If at beginning of line, return false
        return FALSE;

    if ( m_rpsCur.bInterlinear() ) // If interlinear
        {
        if ( *psz == ' ' ) // If we are stepping over a boundary, move left to beginning of previous word and then right to end of word
            {
            m_rpsCur.iChar--; // Move text position to space we are stepping over
            m_rpsCur.bSkipWhiteLeft(); // Step over any more spaces 
            int iCharDesired = m_rpsCur.iChar; // Remember this as desired position
            bEditHome(); // Move to beginning of line
            while ( m_rpsCur.iChar < iCharDesired ) // Move out to desired position
                bMoveRight();
            return TRUE;    
            }
        }
    m_rpsCur.iChar -= m_rpsCur.pfld->plng()->iCharLeftNumBytes( psz + 1, m_rpsCur.iChar ); // Move text position // 1.4gz Fix bug of crash on left arrow at invalid Unicode char at front of field
#ifdef SUBFIELD
    (void) m_rpsCur.bMoveLeftPastSubfieldTags();  // 1996-10-08 MRP
#endif // SUBFIELD
    return TRUE;
}

/* AB My goal is to minimize the use of code that knows about the
    variables and maximize the use of primitives.
    My ideal is to have only one or two primitives that know what
    is really going on, and have everything else built on them.
    But in practice, that can be inefficient so that a couple more
    primitives probably need to handle the variables. But we should
    be cautious and add those only as needed.
*/
/* AB Edit routines that can fail to perform anything return BOOL.
    Those that cannot fail to operate do not return a value.
*/
//------------------------------------------------------
BOOL  CShwView::bEditRight() // Right arrow
{
    if ( bMoveRight() ) // Try to move right in line
        return TRUE;
    else if ( bEditDown() ) // Else try to move down a line
        {
        bEditHome(); // Move to beginning of line
        return TRUE;
        }
    return FALSE; // If nothing moved, return false
}
//------------------------------------------------------
BOOL  CShwView::bEditLeft() // Left arrow
{
    if ( bMoveLeft() ) // Try to move left in line
        return TRUE;
    else if ( bEditUp() ) // Else try to move up a line // 1.0bb Change to EditUp so hidden will work
        {
        EditEnd(); // Move to end of line
        return TRUE;
        }
    return FALSE; // If nothing moved, return false
}
//------------------------------------------------------
BOOL  CShwView::bEditUp() // Up arrow
{
    CLPoint pnt = m_pntCaretLoc;
    if ( bMoveUp( TRUE ) ) // 1.0bc If move successfully found non-hidden field, then all is well
        {
        int iPixelOffset = 0; // Pixel offset is used to try to keep the cursor from moving right or left on up or down arrows
        if ( m_rpsCur.pfld->plng()->bRightToLeft() )
            iPixelOffset = -4; // On right to left script, compensate the opposite way
        else
            iPixelOffset = 2;
		if ( pnt.x == m_iFldStartCol ) // 1.1hk Fix bug of cursor wandering away from left margin
			iPixelOffset = 0;
        m_pntCaretLoc.x = m_iFldStartCol + iCharsFit( m_rpsCur, pnt.x - m_iFldStartCol + iPixelOffset ); // set m_rpsCur to closest char
        return TRUE;
        }
    return FALSE; // 1.0bb Don't go to beginning of first line on up arrow
}
//------------------------------------------------------
BOOL  CShwView::bEditDown() // Down arrow
{
    CLPoint pnt = m_pntCaretLoc;
    if ( bMoveDown( TRUE ) ) // 1.0bc If move successfully found non-hidden field, then all is well
        {
        int iPixelOffset = 0; // Pixel offset is used to try to keep the cursor from moving right or left on up or down arrows
        if ( m_rpsCur.pfld->plng()->bRightToLeft() )
            iPixelOffset = -4; // On right to left script, compensate the opposite way
        else
            iPixelOffset = 2;
		if ( pnt.x == m_iFldStartCol ) // 1.1hk Fix bug of cursor wandering away from left margin
			iPixelOffset = 0;
        m_pntCaretLoc.x = m_iFldStartCol + iCharsFit( m_rpsCur, pnt.x - m_iFldStartCol + iPixelOffset ); // set m_rpsCur to closest char
        return TRUE;
        }
    return FALSE; // 1.0bb Don't go to end of last line on down arrow
}
//------------------------------------------------------
BOOL  CShwView::bEditPgUp() // PgUp, page up, 20 lines up
{
    CLPoint pnt = m_pntCaretLoc;
    CRect rect;
    GetClientRect( &rect );
    pnt.y -= rect.bottom - iLineHt( m_rpsCur.pfld ) * 2;
    
    if ( !bEditUp() ) // If can't move at all, return false
        return FALSE; 
    while ( m_pntCaretLoc.y > pnt.y && bEditUp() )
        ;
    return TRUE;        
}
//------------------------------------------------------
BOOL  CShwView::bEditPgDn() // PgDn, page down, 20 lines down
{
    CLPoint pnt = m_pntCaretLoc;
    CRect rect;
    GetClientRect( &rect );
    pnt.y += rect.bottom - iLineHt( m_rpsCur.pfld ) * 2;
    
    if ( !bEditDown() ) // If can't move at all, return false
        return FALSE; 
    while ( m_pntCaretLoc.y < pnt.y && bEditDown() )
        ;
    return TRUE;        
}
//------------------------------------------------------
void  CShwView::EditInsert() // Insert, toggle overlay mode
{ // !!Not yet
}
//------------------------------------------------------
BOOL  CShwView::bEditDel( BOOL bAddToUndoBuffer ) // Del, delete char
{
    if ( bSelecting( eAnyText ) )
        {
        DeleteSelection();
        return TRUE;
        }
    if ( m_rpsCur.iChar >= m_rpsCur.pfld->GetLength() ) // If at end of field, don't delete, except to join interlinear bundles
        {
        if ( m_rpsCur.bInterlinear() && m_rpsCur.bFirstInterlinear() ) // If first interlinear line, joint next to it
            {
            bCombineBundles( m_rpsCur, FALSE ); // Join next bundle to this one
            m_iWhatChanged |= eMultipleLines; // redraw everything
            SetScroll( TRUE ); // Recalculate scroll sizes
            ResetUndo(); // clear undo buffer
            return TRUE;
            }
        else
            return FALSE; // If nothing deleted, return false   
        }
    if ( bAddToUndoBuffer )
		{
		int iNumBytes = m_rpsCur.pfld->plng()->iCharNumBytes( m_rpsCur.psz() );
		Str8 s( m_rpsCur.psz(), iNumBytes );
        m_undlst.Add( m_rpsCur, s ); // add deletion to undo buffer
//        m_undlst.Add( m_rpsCur, *m_rpsCur.psz() ); // add deletion to undo buffer
		}
    if ( *m_rpsCur.psz() == '\n' ) // If deleting nl, decrement line count
        {
        m_iWhatChanged |= eMultipleLines; // redraw everything if deleting newline
        m_lTotalHt -= iLineHt( m_rpsCur.pfld );
        SetScroll( FALSE );
        }
    else
        m_iWhatChanged |= eSingleLine; // just redraw single line

    if ( m_rpsCur.bInterlinear() && *m_rpsCur.psz() != '\n' ) // If interlinear, add space before next word to maintain alignment
        {
		BOOL bSpaceDeleted = FALSE;
		do { // 1.1ag In interlinear, delete all spaces at once
			SaveCaretPos();
			int iNumBytes = m_rpsCur.pfld->plng()->iCharNumBytes( m_rpsCur.psz() ); // 1.1uc Fix bug of delete Unicode char not inserting enough spaces to maintain alignment
			if ( *(m_rpsCur.psz()) == ' ' ) // If on a space, move another word to allow deletion of last space between words
				m_rpsCur.bMoveToNextWd();
			if ( m_rpsCur.bMoveToNextWd() && *m_rpsCur.psz() && *m_rpsCur.psz() != '\n' ) // If another word in same line, insert space before it
				{
				for ( int i = iNumBytes; i; i-- ) // 1.1uc Fix bug of delete Unicode char not inserting enough spaces to maintain alignment
					CharInsert( ' ' );
				}
			RestoreCaretPos();

			bSpaceDeleted = ( *(m_rpsCur.psz()) == ' ' ); // 1.1ka Fix bug of deleting spaces on delete of last letter in word
			CharDelete(); // Delete char
			SetModifiedFlag(); // Note that something changed
        
			m_rpsCur.Tighten(); // Tighten anything after
			} while ( *(m_rpsCur.psz()) == ' ' && bSpaceDeleted ); // 1.1ka Fix bug of deleting spaces on delete of last letter in word // 1.1ag In interlinear, delete all spaces at once
        return TRUE;
        }
    CharDelete(); // Delete char
    if ( m_rpsCur.iChar > 0 && *(m_rpsCur.psz() - 1) == '-' ) // If delete after hyphen, it may change display position
        Invalidate(FALSE);
    SetModifiedFlag(); // Note that something changed
    return TRUE;
}

//------------------------------------------------------
BOOL CShwView::bEditBackspace() // Backspace, delete prev char
{  
    if ( bSelecting( eAnyText ) )
        {
        DeleteSelection();
        return TRUE;
        }
    if ( m_rpsCur.iChar == 0 ) // If at beginning of field, delete marker // 1.5.0cb Disable this, too confusing for users
		{
		if ( !Shw_pProject()->bDontAskChangeMarker() ) // 1.5.0kd Don't ask change marker if accepted once // 1.6.1ch 
			{
			int iAns = AfxMessageBox( _("Change Marker?"), MB_YESNO | MB_DEFBUTTON2 ); // 1.5.0eb On backspace into marker, ask if change marker
			if ( iAns == IDNO )
				return FALSE; // 1.5.0cb Don't allow backspace to go into marker, confuses users
			Shw_pProject()->ToggleDontAskChangeMarker(); // 1.6.1ch Don't ask change marker if accepted once
//			bAskChangeMarker = FALSE; // 1.5.0kd Don't ask change marker if accepted once
			}
        CField* pfld = m_rpsCur.pfld; // Remember field
        if ( pfld == m_prelCur->prec()->pfldFirst() )
            return FALSE; // If first field, don't delete

        if ( !bValidFldLen( pfld->GetLength() + m_rpsCur.pfldPrev()->GetLength() ) )
            return FALSE;
        
        RECT rcWindowPos;
        GetWindowRect( &rcWindowPos ); // Get window position
        CPoint pntOrg = GetScrollPosition(); // Get current origin, upper left corner postion
        CFieldMarkerEditDlg dlg( pfld, rcWindowPos,
            CPoint( m_pntCaretLoc - pntOrg ),
            m_prelCur->prec()->pfldFirst()->pmkr() );
        int iResult = dlg.DoModal();
        if ( iResult == IDOK )
			{
            SetModifiedFlag();  // Note that something changed
			if ( pfld && bHiddenField( pfld ) ) // 1.4hh Fix bug of inserted hidden field disappearing instantly
				{
				SetHideFields( FALSE ); // 1.4hh Fix bug of inserted hidden field disappearing instantly
				AfxMessageBox(_("Hide fields turned off to show inserted hidden field.")); // 1.4hh
				}
			}
        else if ( iResult == IDNO )    // IDNO means backspace was pressed on empty marker so delete marker
            {
            if ( pfld == m_rpsUpperLeft.pfld ) // bad news to delete our frame of reference
                lVScroll( -1 );
            bMoveUp(); // Move to prev field if possible
            EditEnd(); // Move to end of last line of prev field
			if ( !m_rpsCur.bInterlinear() ) // 1.6.4m Fix bug of messed up cursor on delete back into interlinear
	            *m_rpsCur.pfld += *pfld; // Append content of deleted field to end of previous field
            m_lTotalHt -= iLineHt( pfld );
            Str8 sUndo = Str8(pNL) + "\\" + pfld->pmkr()->sMarker() + " ";
            m_undlst.Add( m_rpsCur, sUndo ); // make marker deletion undoable
            m_pind->pindsetMyOwner()->DeleteField( m_prelCur, pfld ); // Delete field
            pntPosToPnt( m_rpsCur ); // Display caret at new place, this is necessary because line may have changed from non-interlinear spacing to interlinear
            SetModifiedFlag(); // Note that something changed
            SetScroll( FALSE );
            Invalidate(FALSE);
            }
        m_iWhatChanged |= eMultipleLines;
		SetFocus(); // 1.4pha Fix Linux Wine bug of not returning focus to main window
        return TRUE;
		}
    else // Else, not beginning of field, delete character
        {
        BOOL bScroll = m_rpsUpperLeft == m_rpsCur; // special case - need to scroll up before deletion
		BOOL bEndOfLine = ( m_rpsCur.iChar == m_rpsCur.pfld->GetLength() || *m_rpsCur.psz() == '\n' ); // 6.0v Remember if deleting last char in line
        if ( !bEditLeft() ) // Move left and delete char
            return FALSE;
        if ( bScroll )
            lVScroll( -1 );
        bEditDel(); // Delete char
        return TRUE;
        }
    return FALSE;           
}
//------------------------------------------------------
void CShwView::EditCharInsert( LPCSTR szChar ) // Insert or overlay char //!!! AB Add optional variable for overlay when that is added because this is used as a primitive and assumes insert
{
    UINT nChar = szChar[0];     // for the operator==() testing for control chsr below
    BOOL bInsertedNL = FALSE; // True if nl inserted by this routine
    BOOL bAlreadyIn = FALSE; // True if char inserted before the standard place
    if ( nChar == '\\' && m_rpsCur.iChar == 0 ) // If backslash at beginning of field, insert nl so backslash can insert marker on next line
        {
        CharInsert( '\n' ); // Insert nl so backslash can insert marker on next line
        bEditRight(); // Move past \n
        m_lTotalHt += iLineHt( m_rpsCur.pfld );
        bInsertedNL = TRUE;
        Invalidate(FALSE);
        }
    if ( nChar == '\\'  // If backslash after return (including one inserted above)
            && m_rpsCur.iChar > 0 && *(m_rpsCur.psz() - 1) == '\n' )
        {
        CField* pfld = NULL;
        NewMarker( &pfld ); // Get a new marker
		if ( pfld && bHiddenField( pfld ) ) // 1.4hh Fix bug of inserted hidden field disappearing instantly
			{
			SetHideFields( FALSE ); // 1.4hh
			AfxMessageBox(_("Hide fields turned off to show inserted hidden field.")); // 1.4hh
			}
        if ( !pfld ) // If they canceled, return without inserting backslash
            {
            if ( bInsertedNL ) // If we inserted an nl above, delete it again
                {
                bEditLeft();
                bEditDel(FALSE);
                }
            return;
            }
        m_iWhatChanged |= eMultipleLines;
        m_prelCur->prec()->InsertAfter( m_rpsCur.pfld, pfld ); // Insert the new field
        pfld->SetContent( m_rpsCur.pfld->Mid( m_rpsCur.iChar ) ); // Put tail of old field into new
        m_rpsCur.pfld->SetContent( m_rpsCur.pfld->Left( m_rpsCur.iChar ) ); // Cut off tail of old field
        bEditLeft();
        bEditDel(FALSE); // Eat off the return
        bEditRight(); // Move to front of new field
        m_lTotalHt += iLineHt( m_rpsCur.pfld ); // add height of new field
        SetScroll( FALSE );
        return;
        }
    if ( nChar == '\r' || nChar == '\n' ) // Translate Enter key into line feed for new line
        nChar = '\n';
    else if ( nChar < ' ' ) // If control character, refuse
        {         
        MessageBeep(0);
        return;
        }
    if ( m_rpsCur.bInterlinear() ) // If interlinear, treat some things special
        {
        if ( m_rpsCur.iChar > 0 && *(m_rpsCur.psz() - 1) == '\n' )  // If after return, refuse to insert
            {         
            MessageBeep(0);
            return;
            }
        if ( nChar == '\n' )
            {
            if ( *m_rpsCur.psz() != '\0' ) // If return not at end, break bundle
                {
                if ( !m_rpsCur.bFirstInterlinear() ) // Don't do it if not first interlinear line
                    return;
                m_rpsCur.bSkipWhite(); // Move to start of next word
                if ( !m_rpsCur.bAtWdBegInBundle() ) // If alignment point not here, don't break
                    return;
                m_rpsCur.BreakBundle( m_rpsCur.iChar ); // Break the bundle in two
                m_iWhatChanged |= eMultipleLines; // redraw everything
                HideCaret(); // Reset everything for display
                SetScroll(TRUE);
                EditSetCaret( m_rpsCur.pfld, m_rpsCur.iChar ); // caret may have jumped
                SetCaretPosAdj(); // Move caret
                ShowCaret();
                SetModifiedFlag();
                ResetUndo(); // clear undo buffer
                return;
                }
            else
                ; // Don't let nl go into the other interlinear processing  
            }   
        else
            {   
            const char* psz = m_rpsCur.psz();   
            if ( !*psz || *psz == '\n' ) // If at end of line, loosen or align
                {
                if ( !m_rpsCur.bFirstInterlinear() ) // If not first line, align, on first line don't loosen or anything
                    {
                    CRecPos rpsPrev = m_rpsCur.rpsPrevField(); // Get line above
                    const char* pszPrev = rpsPrev.psz();
                    if ( nChar == ' ' ) // If insert space, align with above
                        {
                        CharInsert( ' ' ); // Insert one space for sure
                        bEditRight();
                        const char* pszNextWd = rpsPrev.pfld->plng()->pszSkipNonWhite( pszPrev );
                        pszNextWd = rpsPrev.pfld->plng()->pszSkipSpace( pszNextWd );
                        int iCh = m_rpsCur.iChar + ( pszNextWd - pszPrev ) - 1; // Calculate pos to align, compensate for space already inserted
                        for ( ; m_rpsCur.iChar < iCh; bEditRight() ) // Align with above
                            CharInsert( ' ' );
                        bAlreadyIn = TRUE;  
                        }
                    else // Else loosen above
                        { // If above at same place is a space or hyphen with a non-space to its right, then loosen
                        if ( rpsPrev.pfld->plng()->bIsWhite( pszPrev )
                                && rpsPrev.pfld->plng()->bIsNonWhite( pszPrev + 1 ) )
                            m_rpsCur.Loosen( 1 );
                        }
                    }
				else // 6.0u If space at end of first line, add spaces past end of longest line
					{
                    if ( nChar == ' ' ) // If insert space, go out to correct place for next word alignment
                        {
						int iLongestLine = 0;
						CRecPos rpsLookForLongest = m_rpsCur;
						while ( TRUE ) // Find length of longest line in bundle
							{
							if ( rpsLookForLongest.pfld->GetLength() > iLongestLine )
								iLongestLine = rpsLookForLongest.pfld->GetLength();
							if ( rpsLookForLongest.bLastInBundle() )
								break;
							rpsLookForLongest = rpsLookForLongest.rpsNextField();
							}
                        for ( ; m_rpsCur.iChar < iLongestLine + 1; bEditRight() ) // Output spaces to go one past longest line in bundle
                            CharInsert( ' ' );
                        bAlreadyIn = TRUE;  
                        }
					}
                }    
            else // Else (not eol) remove compensating space to maintain alignment
                {
				int iNumBytes = strlen( szChar ); // 1.1uc Fix bug of insert Unicode char not deleting enough compensating spaces
				for ( int iCount = iNumBytes; iCount; iCount-- )  // 1.1uc Fix bug of insert Unicode char not deleting enough compensating spaces
					{
					const char* psz = m_rpsCur.pfld->plng()->pszSkipNonWhite( m_rpsCur.psz() ); // Move past non-white
	                int iOffset = psz - m_rpsCur.pszFieldBeg();
					if ( !( *psz == ' ' && *( psz + 1 ) == ' ' ) ) // If not two or more spaces after word, loosen other lines
						{
						CRecPos rps = m_rpsCur; // Get a temporary recpos
						rps.bMoveToNextWd(); // Move one forward so loosen will happen the right place
						rps.Loosen( 1 ); // Loosen other lines in bundle
						}
					if ( *psz && *( psz + 1 ) )  // If not at end of line, delete compensating space
						m_rpsCur.pfld->Delete( iOffset );    
					}
                if ( m_rpsCur.iChar > 0 && *(m_rpsCur.psz() - 1) == '-' ) // If insert after hyphen, it may change display position
                    Invalidate(FALSE);
                }   
            }
        }

    if ( nChar == '\n' )
        {
        m_iWhatChanged |= eMultipleLines;
        CMarker* pmkrFollowingThis = m_rpsCur.pfld->pmkr()->pmkrFollowingThis();
		if ( pmkrFollowingThis && ptyp()->bHideFields() && pmkrFollowingThis->bHidden() ) // 1.4gzg If automatically inserted field is hidden, don't insert it // 1.4gzj Fix crash (1.4gzg) on Enter at end of field
			pmkrFollowingThis = NULL;
        if ( pmkrFollowingThis && ::GetKeyState( VK_SHIFT ) >= 0 )
            {
            const char* psz = m_rpsCur.psz();
            Shw_bMatchWhiteSpaceAt(&psz);
            if ( !*psz )
                {
                // Typing the Enter (Mac: return) key at the end of a field
                // that has the "Marker for the following field" property
                // causes Shoebox to insert the appropriate new field and
                // move the caret to its beginning so that subsequent keystrokes
                // go directly into its content without interruption.
                CField* pfldFollowingThis = new CField(pmkrFollowingThis);
                m_prelCur->prec()->InsertAfter( m_rpsCur.pfld,
                    pfldFollowingThis ); // Insert the new field
                int i = m_rpsCur.iChar;
                pfldFollowingThis->SetContent( m_rpsCur.pfld->Mid( m_rpsCur.iChar ) ); // Put tail of old field into new
                m_rpsCur.pfld->SetContent( m_rpsCur.pfld->Left( m_rpsCur.iChar ) ); // Cut off tail of old field
                }
            else
                // 1997-10-06 MRP: If within the field's contents,
                CharInsert( nChar ); // Then insert the newline
            }
        else
            // 1997-10-06 MRP: If "Marker for following field: [None]"
            // or if it's a Shift+Enter
            CharInsert( nChar ); // Insert the newline
        bEditRight(); // Move cursor right
        m_lTotalHt += iLineHt( m_rpsCur.pfld );
        bAlreadyIn = TRUE;
        SetScroll( FALSE ); // Reset scroll sizes without remeasuring lines
        }
    else
        m_iWhatChanged |= eSingleLine;
    
    if ( !bAlreadyIn )
        {
		int iRestOfLineLen = m_rpsCur.pfld->GetLength() - m_rpsCur.iChar; // 1.5mf Measure rest of line, then move cursor to same place
        StringInsert( szChar ); // Insert the string
		while ( m_rpsCur.pfld->GetLength() - m_rpsCur.iChar > iRestOfLineLen ) // 1.5mf Move cursor till same distance from end as before insertion
	        bEditRight(); // Move cursor right
        }
    SetModifiedFlag(); // Note that something changed
}
//------------------------------------------------------
BOOL  CShwView::bEditCtrlRight() // Ctrl Right arrow, word right
{
    BOOL bMoved = FALSE;   
    if ( m_rpsCur.pfld->plng()->bRightToLeft() )
        { // Copied from bEditCtrlRight. Can't call it because it does revers!
        while ( ( m_rpsCur.bLeftIsWhite() || m_rpsCur.iChar == 0 ) 
                && ( !bMoved || ( *m_rpsCur.psz() && *m_rpsCur.psz() != '\n' ) )
                && bEditLeft() ) // Move past white 
            bMoved = TRUE;  
        if ( bMoved && ( !*m_rpsCur.psz() || *m_rpsCur.psz() == '\n' ) )
            return bMoved; // stop at end of field or end of line
        while ( m_rpsCur.bLeftIsNonWhite() && bEditLeft() ) // Move past non-white
            bMoved = TRUE;  
        return ( bMoved ); // If no movement, return false
        }
    while ( m_rpsCur.bIsNonWhite() && bEditRight() ) // Move past non-white
        bMoved = TRUE;  
    while ( !m_rpsCur.bIsNonWhite() 
            && ( !bMoved || ( *m_rpsCur.psz() && *m_rpsCur.psz() != '\n' ) )
            && bEditRight() ) // Move past white 
        bMoved = TRUE;
    return ( bMoved ); // If no movement, return false
}
//------------------------------------------------------
BOOL  CShwView::bEditCtrlLeft() // Ctrl Left arrow, word left
{
    BOOL bMoved = FALSE;
    if ( m_rpsCur.pfld->plng()->bRightToLeft() )
        { // Copied from bEditCtrlRight. Can't call it because it does revers!
        while ( m_rpsCur.bIsNonWhite() && bEditRight() ) // Move past non-white
            bMoved = TRUE;  
        while ( !m_rpsCur.bIsNonWhite() 
                && ( !bMoved || ( *m_rpsCur.psz() && *m_rpsCur.psz() != '\n' ) )
                && bEditRight() ) // Move past white 
            bMoved = TRUE;
        return ( bMoved ); // If no movement, return false
        }
    while ( ( m_rpsCur.bLeftIsWhite() || m_rpsCur.iChar == 0 ) 
            && ( !bMoved || ( *m_rpsCur.psz() && *m_rpsCur.psz() != '\n' ) )
            && bEditLeft() ) // Move past white 
        bMoved = TRUE;  
    if ( bMoved && ( !*m_rpsCur.psz() || *m_rpsCur.psz() == '\n' ) )
        return bMoved; // stop at end of field or end of line
    while ( m_rpsCur.bLeftIsNonWhite() && bEditLeft() ) // Move past non-white
        bMoved = TRUE;  
    return ( bMoved ); // If no movement, return false
}
//------------------------------------------------------
BOOL CShwView::bEditCtrlUp() // Ctrl Up arrow  go up a paragraph
{
    return bUpParagraph();
}   
//------------------------------------------------------
BOOL CShwView::bEditCtrlDown() // Ctrl Down arrow  go down a paragraph
{
    if ( !bDnParagraph() )
        EditEnd();
    return TRUE;
}
//------------------------------------------------------
void  CShwView::EditCtrlHome() // Ctrl Home, beginning of record
{
    MoveCtrlHome();
}
//------------------------------------------------------
void  CShwView::EditCtrlEnd() // Ctrl End, end of record
{
    while ( bMoveDown( TRUE ) ) // 1.0bd Don't move into hidden fields
        ;
    while ( bMoveRight() )
        ;
}
//------------------------------------------------------
void  CShwView::EditCtrlPgUp() // Ctrl PgUp, top left of window
{
    bEditHome(); // Move to left, scrolling into view if necessary
    CPoint pntOrg = GetScrollPosition(); // Get current origin, upper left corner postion
    EditMouseLeftClick( CPoint( m_iFldStartCol, pntOrg.y ) ); // Put cursor at top of window 
}
//------------------------------------------------------
void  CShwView::EditCtrlPgDn() // Ctrl PgDn, bottom left of window
{
    bEditHome(); // Move to left, scrolling into view if necessary
    CRect rect;
    GetClientRect( &rect ); // Get size of client area of current window
    CSize siz = rect.Size();
    CPoint pntOrg = GetScrollPosition(); // Get current origin, upper left corner postion
    EditMouseLeftClick( CPoint( m_iFldStartCol, pntOrg.y + siz.cy - iLineHt( m_rpsCur.pfld ) ) ); // Put cursor at bottom of window
                                                                    // ^^ BJY iLineHt() isn't really right
}
//------------------------------------------------------
void CShwView::EditMouseLeftClick( CPoint pnt ) // Mouse left button click
{
	CLPoint pntCaretLoc = pntPntToPos( pnt ); // 1.5.1cf 
    m_pntCaretLoc = pntCaretLoc; // 1.5.1cf 
}
//------------------------------------------------------
void CShwView::EditMouseDblClk( CPoint pnt ) // Mouse left button double click
{
    if ( (!*m_rpsCur.psz() || m_rpsCur.bIsWdSep()) && m_rpsCur.bLeftIsWdSep() )
        return; // not on a word
    
    SelectWd(); // Select the current word
}
//------------------------------------------------------
void CShwView::SelectWd( CRecPos rps )
{
    m_rpsCur = rps; // Set current to value passed in
	if ( m_rpsCur.bIsWhite() && m_rpsCur.bLeftIsWhite() ) // 5.99w If in white space, move left to word
		m_rpsCur.bSkipWhiteLeft();
    m_rpsCur.bSkipNonWhiteLeft(); // Move to begin word based on white space
	CRecPos rpsTemp( m_rpsCur );
	rpsTemp.bSkipPunc(); // Move past initial punctuation
	if ( !rpsTemp.bIsWhite() ) // If not punc all the way to white (such as fail mark), set cur to end of punc
		m_rpsCur = rpsTemp;
    StartSelection( eText ); // Set start of select
    rpsTemp.bSkipWdChar(); // Move to end of word characters
	if ( rpsTemp.iChar != m_rpsCur.iChar ) // If it moved past something, set cur to end
		m_rpsCur = rpsTemp;
    CRecPos rpsEnd( m_rpsCur ); // Select up to last word char before space, eg. eat.meat includes the embedded period
    while ( !rpsEnd.bIsWdEnd() ) // While not white space
        {
        if ( rpsEnd.bIsWdCharMove() ) // If a word char found, include it in select
            m_rpsCur.iChar = rpsEnd.iChar;
        else // Else (punc) keep looking
            rpsEnd.iChar++; 
        }

    if ( m_rpsSelOrigin == m_rpsCur )
        ClearSelection();   // clear selection if back at starting point
    EditSetCaret( m_rpsCur.pfld, m_rpsCur.iChar );
    Invalidate();
}

//------------------------------------------------------
void CShwView::SelectAmbig( CRecPos rps ) // Select ambiguity containing rps, scroll it up from bottom
{
    while ( !m_rpsCur.bLastInBundle() ) // Move to bottom of bundle so whole bundle will show
		bEditDown();
    SetCaretPosAdj(); // Scroll the bottom of bundle into view
    SelectWd( rps ); // Select word containing rps
    UpdateWindow(); // force immediate repaint for scroll size reasons
}
//------------------------------------------------------
void CShwView::EditInsertMarker()
{
    // 1998-12-02 MRP: cast
    ASSERT( (CRecLookEl*)m_prelCur != NULL ); // Should not be possible because command greys
    CMarker* pmkr = (CMarker*)m_rpsCur.pfld->pmkr(); // !! Maybe cast to non const somewhere else
    if ( !GetDocument()->pmkrset()->bModalView_Elements(&pmkr) )
        return;  // the user chose 'Close'
        
    const CMarker* pmkrRecord = m_prelCur->prec()->pfldFirst()->pmkr();
    if ( pmkr == pmkrRecord )
        {
        AfxMessageBox(_("Insertion of record marker not allowed. Use Database Insert Record."));
        return;
        }

    CField* pfld = new CField( pmkr, m_rpsCur.pfld->Mid( m_rpsCur.iChar ) ); // Make new field with tail of current
    m_rpsCur.pfld->SetContent( m_rpsCur.pfld->Left( m_rpsCur.iChar ) ); // Cut off tail of old field
    m_prelCur->prec()->InsertAfter( m_rpsCur.pfld, pfld ); // Insert the new field
    bEditRight(); // Move to front of new field
    Invalidate(FALSE);
}
//------------------------------------------------------
void CShwView::EditSetCaret( CField* pfld, int iChar, int iSelLen ) // Set the caret to a particular position
{
    HideCaret();
    m_rpsCur.SetPos( iChar, pfld );
    m_pntCaretLoc = pntPosToPnt( m_rpsCur );
    SetCaretPosAdj();

    if ( iSelLen )
        {
        StartSelection( eText );
        ASSERT( iSelLen > 0 && pfld->GetLength() >= iChar+iSelLen ); // assume selection within one field only
        EditSetCaret( pfld, iChar+iSelLen ); // a little recursion shouldn't hurt
        Invalidate(FALSE);
        }
    
/*
    MoveCtrlHome(); // Start at top
    while ( m_rpsCur.pfld != pfld && bMoveDown() ) // Find desired field
        ;
    ASSERT( m_rpsCur.pfld == pfld );    
    while ( m_rpsCur.iChar < iChar && ( bMoveRight() || bMoveDown() ) // Find desired char pos
        ;
*/  
    ShowCaret();
}
//------------------------------------------------------
void CShwView::EditInsertWord( const char* psz, BOOL bPad ) // Insert word at current position, adding space before and after if necessary
{
    if ( !bValidFldLen( m_rpsCur.pfld->GetLength() + strlen( psz ) ) )
        return;
    HideCaret();
    CRecPos rpsBefore = m_rpsCur;
    Str8 sTail = m_rpsCur.pfld->Mid( m_rpsCur.iChar ); // Save tail part
    m_rpsCur.pfld->ReleaseBuffer( m_rpsCur.iChar ); // Cut off tail part
    int iNewPos = m_rpsCur.iChar + strlen( psz );
    if ( bPad )
        {
        if ( m_rpsCur.bLeftIsNonWhite()) // If not after space, insert space before
            {
            *m_rpsCur.pfld += " ";
            iNewPos++;
            }
        if ( sTail.GetLength() && !m_rpsCur.pfld->plng()->bIsWhite( sTail ) ) // If not before space, insert space after
            {
            sTail = " " + sTail;
            iNewPos++;
            }
        }
    *m_rpsCur.pfld += psz; // Append string
    *m_rpsCur.pfld += sTail; // Append tail part
    SetModifiedFlag(); // Note that something changed
    Invalidate(FALSE);
    SetScroll( TRUE );
    EditSetCaret( m_rpsCur.pfld, iNewPos );
    m_undlst.Add( rpsBefore, m_rpsCur ); // add to undo list
    ShowCaret();
}
//------------------------------------------------------
void CShwView::EditInsertFromRangeSet()     // Let user insert a word from a Range Set
{
    ASSERT(m_prelCur);

    Str8 sText;      // to contain range set item
    if ( m_rpsCur.pfld->pmkr()->prngset()->bModalView_Insert( sText ) )
        {
        DeleteSelection();
        EditInsertWord( sText, TRUE ); // Insert word selected from listbox
        }
}
//------------------------------------------------------
void CShwView::EditMarker( CField* pfld ) // Edit the marker of a field
{   
    ASSERT( pfld );
    RECT rcWindowPos;
    GetWindowRect( &rcWindowPos ); // Get window position
    CPoint pntOrg = GetScrollPosition(); // Get current origin, upper left corner postion
    CFieldMarkerEditDlg dlg( pfld, rcWindowPos,
        CPoint( m_pntCaretLoc - pntOrg ),
        m_prelCur->prec()->pfldFirst()->pmkr() );
    if ( dlg.DoModal() == IDOK )
        SetModifiedFlag(); // Note that something changed
    Invalidate(FALSE);
	SetFocus(); // 1.4pha Fix Linux Wine bug of not returning focus to main window
}
//------------------------------------------------------
void CShwView::NewMarker( CField** ppfld ) // Insert a new field
{
    ASSERT( ppfld );
    RECT rcWindowPos;
    GetWindowRect( &rcWindowPos );
    CPoint pntOrg = GetScrollPosition(); // Get current origin, upper left corner postion
    CFieldMarkerEditDlg dlg( GetDocument()->pmkrset(), ppfld, rcWindowPos,
            CPoint( m_pntCaretLoc - pntOrg ),
            m_prelCur->prec()->pfldFirst()->pmkr() );
    if ( dlg.DoModal() == IDOK )
        SetModifiedFlag(); // Note that something changed
    Invalidate(FALSE);
	SetFocus(); // 1.4pha Fix Linux Wine bug of not returning focus to main window
}
//------------------------------------------------------
int CShwView::iMeasureRecord( BOOL bCalc )  // Get total size of record
{
    // 1998-12-02 MRP: cast
    if ( (CRecLookEl*)m_prelCur == NULL ) // If no records, return 0,0
        return 0;
    if ( bCalc ) // If calculation desired, measure and count lines
        {
        m_iInterlinCharWd = ptyp()->iInterlinCharWd(); // Get interlinear character width
        m_lTotalHt = lLineCountAndMeasure();
        }
    if ( !m_iMaxLineWd )
        return 10000; // default to displaying a scroll bar first time called
    else
        return m_iMaxLineWd; // Return max line width
}

static CLPoint pntCaretLocSave;
static int iCharCurSave = 0;
static CField* pfldCurSave;
//------------------------------------------------------
void CShwView::SaveCaretPos() // Save caret position so it can be restored
{
    pntCaretLocSave = m_pntCaretLoc;
    iCharCurSave = m_rpsCur.iChar;
    pfldCurSave = m_rpsCur.pfld;
}
//------------------------------------------------------
void CShwView::RestoreCaretPos() // Restore caret position from save
{
    m_pntCaretLoc = pntCaretLocSave;
    // m_rpsCur.iChar = iCharCurSave;
    // m_rpsCur.pfld = pfldCurSave;
    m_rpsCur.SetPos(iCharCurSave, pfldCurSave);  // 1996-10-15 MRP
}
//------------------------------------------------------
long CShwView::lLineCountAndMeasure() // Set line count and return record length
{
    CRecPos rps( 0, m_prelCur->prec()->pfldFirst(), m_prelCur->prec() ); // start at top of rec
    int iHt;
    long lTotalHt = iLineHt( rps.pfld );
    do {
        iHt = iDown( rps ); // work our way down the record
        lTotalHt += iHt;
        } while ( iHt );
    return lTotalHt;

}
//------------------------------------------------------
void CShwView::MessageStatusLine(const char* pszMessage) // 1.3ae Change MessageStatusLine to not use resource
{
    CShwStatusBar* pStatus = (CShwStatusBar*)
        AfxGetApp()->m_pMainWnd->GetDescendantWindow(IDW_SHW_STATUS_BAR);
    ASSERT(pStatus);
    pStatus->SetStatusMessage(pszMessage);
}
//------------------------------------------------------
BOOL CShwView::bPtInMarker( CPoint pnt )
{
    ASSERT( !m_bBrowsing );
    return( pnt.x + GetScrollPosition().x < m_iFldStartCol - eRecordDividerWidth+1 );
}
//------------------------------------------------------
// Start or extend selection as eText selection.
void CShwView::ShiftSelectText()
{
    if ( bSelecting( eLines|eFields ) )
        m_iSelection = eText; 
    else if ( !bSelecting( eText ) )
        StartSelection( eText );
}
//------------------------------------------------------
void CShwView::StartSelection( int type )
{
    m_iSelection = type;
    if ( bSelecting( eText ) )
        m_rpsSelOrigin = m_rpsCur;
    else if ( bSelecting( eLines ) )
        ;
    else if ( bSelecting( eFields ) )
        ;
    else
        ASSERT(0);
}
//------------------------------------------------------
BOOL CShwView::bEditCopy()
{
    ASSERT( bSelecting( eAnyText ) );
    if ( !OpenClipboard() )
        return FALSE;
    ::EmptyClipboard();

    CRecPos rpsBeg = rpsSelBeg();
    CRecPos rpsEnd = rpsSelEnd();

    // special case: selecting single line in line mode
    if ( rpsBeg.pfld == rpsEnd.pfld && bSelecting( eLines ) )
        {
        if ( *rpsEnd.psz() == '\n' )
            rpsEnd.iChar++; // copy following linebreak
        else if ( rpsBeg.iChar == rpsEnd.iChar && rpsBeg.iChar )
            {
            ASSERT( *( rpsBeg.psz() - 1 ) == '\n' );
            rpsBeg.iChar--; // copy preceding linebreak
            }
        }
    
    Str8 sSel;
    BOOL bCopyFirstMkr = rpsBeg.iChar == 0 && bSelecting( eLines|eFields );
    if ( !bCopy( rpsBeg, rpsEnd, sSel, bCopyFirstMkr ) )
        {
        ::CloseClipboard();
        return FALSE;
        }
	m_strCopied = sSel; // 1.2km

    // tell the clipboard that we support 3 formats: 1) our internal (UTF-8-capable) format
    //  2) UTF-16 for c/c/p to Word, etc. and 3) the original CF_TEXT (for backwards 
    //  compatibility)
    // by passing '0' for the 2nd param, we're telling the clipboard to send us the 
    //  WM_RENDERFORMAT message when a user (maybe Toolbox itself) asks us for a format.
    SetClipboardData( Shw_papp()->TlbxClipBdFormat(), 0 );
    OnRenderFormat(Shw_papp()->TlbxClipBdFormat()); // 1.1bp Fix bug of copy failing in various situations
	if ( rpsBeg.pfld->plng()->bUnicodeLang() ) // 1.0eb Fix bug of failing to paste uppper ANSI correctly into Word. Word was interpreting it as Unicode, which it isn't.
		{
		SetClipboardData( CF_UNICODETEXT, 0 );
		OnRenderFormat(CF_UNICODETEXT); // 1.1bp Fix bug of copy failing in various situations
		}
    SetClipboardData( CF_TEXT, 0 );
    OnRenderFormat(CF_TEXT); // 1.1bp Fix bug of copy failing in various situations
    CloseClipboard();
    return TRUE;
}
//------------------------------------------------------
// use static (global) members for these new c/c/p helpers, so we have one version for all
//  views (not one for each)
Str8    CShwView::m_strCopied;

void CShwView::OnRenderFormat(UINT nFormat)
{
    // get the last thing copied and render it
	Str8 s = m_strCopied; // 1.4qzhm
	void* p = (void*)(const char*)s;
	int iLen = s.GetLength() + 1;
	CString sw = swUTF16( s );
#ifdef UNICODE
    if( nFormat == CF_UNICODETEXT )
	    {
		p = (void*)(const wchar_t*)sw;
		iLen = sw.GetLength() * 2 + 2;
		}
#endif
    // Put it on the clipboard
    HGLOBAL hData = ::GlobalAlloc( GMEM_MOVEABLE, iLen );
    ASSERT( hData );
    void* pmem = (LPVOID)::GlobalLock( hData ); // get a pointer to allocated memory
    ASSERT( pmem );
    memcpy( pmem, p, iLen ); // copy text into global buffer
    SetClipboardData( nFormat, hData );
}

//------------------------------------------------------
void CShwView::CopyTemplate() // make template of current record contents
{
    BOOL bIncludeContents = FALSE; // default to getting list of fields only, no contents
    CTemplateDlg dlg( &bIncludeContents );
    if ( dlg.DoModal() != IDOK )
        return;

    Str8 sTemplate;
    CRecord* prec = m_prelCur->prec();
    CField* pfld = prec->pfldFirst(); // try to locate at end of key field
    if ( prec->pfldNext( pfld ) ) // is record non-empty?
        {
        CRecPos rpsBeg( (*pfld).GetLength(), pfld, m_prelCur->prec() );
        CField* pfldLast = prec->pfldLast();
        CRecPos rpsEnd( (*pfldLast).GetLength(), pfldLast, prec );
        if ( !bCopy( rpsBeg, rpsEnd, sTemplate, FALSE, !bIncludeContents ) ) // get copy of record contents minus record key
            return;
        }
    GetDocument()->ptyp()->SetTemplate( sTemplate ); // pass it on to database type
}

// return record contents between rpsBeg and rpsEnd
//------------------------------------------------------
BOOL CShwView::bCopy( CRecPos rpsBeg, CRecPos rpsEnd, Str8& sSel, BOOL bCopyFirstMkr, BOOL bMkrsOnly )
{
    sSel = "";

    if (!(rpsBeg < rpsEnd))
        return FALSE; // return empty string

    CRecPos rps = rpsBeg;
    BOOL bCopyMarker = bCopyFirstMkr;
    BOOL bLastLine = FALSE;

    do  {
        Str8 sTmp;
        if ( rps.pfld == rpsEnd.pfld )
            sTmp = Str8( rps.psz(), rpsEnd.psz() - rps.psz() );
        else
            sTmp = rps.psz();

        if ( bCopyMarker ) // at beginning of field?
            {
            if ( sSel.GetLength() ) // is a marker the first thing selected?
                sSel += pNL; // no, replace with \n replacement
            sSel += "\\" + rps.pfld->sMarker() + " ";
            }
#ifdef _MAC
        if ( !bMkrsOnly )
            sSel += sTmp;
#else
        const char* p;
        for ( const char* s = p = sTmp; p; s = p+1 ) // convert all \n to \r\n
            {                  
            p = strchr( s, '\n' );
            Str8 sAdd;
            if ( p )
                sAdd = Str8( s, p-s ) + pNL;
            else
                sAdd = Str8( s ); // no more \n's just copy rest of string
            if ( !bMkrsOnly )
                sSel += sAdd;
            }
#endif
        bLastLine = !iDnField( rps ); // will fail on last line in record
        bCopyMarker = TRUE; // copy field markers from now on
        } while ( rps < rpsEnd && !bLastLine );

    if ( rps == rpsEnd && rps.iChar == 0 ) // special case - marker is last thing selected
        {
        sSel += pNL;
        sSel += "\\" + rps.pfld->sMarker() + " ";
        }

    return TRUE; // return the stuff copied
}
//------------------------------------------------------
void FixInterlinAfterCut( CRecPos rpsStart ) // Fix interlinear line after cut, not allowed to have material after NL
{
    // Find end of first line and take care of anything after it
    CRecPos rps = rpsStart;
    if ( rps.bInterlinear() )
        {
        rps.iChar = 0; // Start at beginning of field
        rps.bMoveEnd(); // Move to end of first line
        if ( rps.bMove() && rps.bMove() ) // If we can move past the nl, deal with the following stuff
            {
            while ( *rps.psz() ) // Eat any extra material, maybe later save it and add question marker
                rps.pfld->Delete( rps.iChar );
            }
        }
}
//------------------------------------------------------
void CShwView::DeleteSelection( BOOL bAddToUndoBuffer )
{
    if ( !bSelecting( eAnyText ) )
        return;

    m_iWhatChanged |= eMultipleLines;
    CRecPos rpsBeg = rpsSelBeg();
    CRecPos rpsEnd = rpsSelEnd();
    
    if ( bSelecting( eLines|eFields ) )
        {
        if ( rpsBeg.iChar == 0 )
            {
            if ( rpsBeg.pfld != rpsBeg.prec->pfldFirst() ) // can't delete record marker!
                {
                CField* pfld = rpsBeg.pfldPrev();
                rpsBeg.SetPos( pfld->GetLength(), pfld ); // put at end of previous field to make normal selection code work
                }
            else
                {
                if ( rpsBeg.pfld == rpsEnd.pfld && rpsEnd.iChar == 0 ) // 1.5.3f Fix problem (1.5.3b) of false message on delete record
                    // Only the recordmarker is selected
                    {
                    MessageBeep(0);
                    AfxMessageBox(_("The record marker cannot be deleted."));
                    return;
                    }                                       //08-20-1997
                }
            }
        else if ( rpsBeg.pfld == rpsEnd.pfld ) // line within a field selected?
            {
            if ( *rpsEnd.psz() == '\n' )
                rpsEnd.iChar++; // delete following linebreak
            else
                {
                ASSERT( *( rpsBeg.psz() - 1 ) == '\n' );
                rpsBeg.iChar--; // delete preceding linebreak
                }
            }
        }

    if ( !bValidFldLen( rpsBeg.iChar + ( rpsEnd.pfld->GetLength() - rpsEnd.iChar ) ) )
        {
        ClearSelection();
        Invalidate( FALSE );
        return;
        }
    
    if ( bAddToUndoBuffer )
        {
        Str8 s;
        bCopy( rpsBeg, rpsEnd, s, FALSE ); // get a copy of what's being deleted
        ASSERT( s.GetLength() );
        m_undlst.Add( rpsBeg, s ); // make it undeletable!
        }

    // The value of bDeletingUL was incorrect in the following scenario:
    // 1. In the record view, scroll an empty field (not the first field)
    //    so that it is the first line in the window.
    // 2. In the marker pane, click the marker to select the field.
    // 3. Press the Delete key.
    //    Shoebox 4.3 asserts at Line 2930 in shwvdis.cpp
    // Although we might be able to fix the problem by changing following line
    // to m_rpsUpperLeft <= rpsEnd so that bDeletingUL is TRUE,
    // we will instead compare each deleted field directly to m_rpsUpperLeft.
    // BOOL bDeletingUL = m_rpsUpperLeft > rpsBeg && m_rpsUpperLeft < rpsEnd;
    BOOL bDeletingUpperLeft = FALSE;  // 2000-08-23 MRP
    // Adjusting the upper left record position also fixes a scrolling problem
    // that occurred when deleting a selection that is above the upper left.
    // In some cases where there hadn't been a scrolling problem,
    // the insertion point is not displayed at the bottom of the window
    // instead of at the top. We decided that users can tolerate this change,
    // if they even notice it.
    BOOL bDeletingAboveUpperLeft = ( rpsEnd < m_rpsUpperLeft );  // 2000-08-23 MRP
    
    if ( rpsBeg.pfld == rpsEnd.pfld ) // selection limited to one field?
        {
        ASSERT( rpsEnd > rpsBeg );
        Str8 sTail = m_rpsCur.pfld->Mid( rpsEnd.iChar ); // Save stuff following selection
        m_rpsCur.pfld->ReleaseBuffer( rpsBeg.iChar ); // Cut off tail part selection
        *m_rpsCur.pfld += sTail; // Reattach stuff following deleted text
        }
    else  // selection crosses field boundaries
        {
        CRecPos rps = rpsBeg;
        rps.pfld->ReleaseBuffer( rpsBeg.iChar ); // cut off tail of this field
        iDnField( rps );
        while ( rps.pfld != rpsEnd.pfld )
            {
            CField* pfld = rps.pfld; // field to delete
            int i = iDnField( rps );
            ASSERT( i );
            if ( m_rpsUpperLeft.pfld == pfld )
                bDeletingUpperLeft = TRUE;  // 2000-08-23 MRP
            m_pind->pindsetMyOwner()->DeleteField( m_prelCur, pfld ); // Delete field
            }
        *rpsBeg.pfld += rps.pfld->Mid( rpsEnd.iChar ); // Append end of deleted field to end of previous field
        if ( m_rpsUpperLeft.pfld == rps.pfld )
            bDeletingUpperLeft = TRUE;  // 2000-08-23 MRP
        m_pind->pindsetMyOwner()->DeleteField( m_prelCur, rps.pfld ); // Delete field
        }
    ClearSelection();
    //  SetScroll( TRUE );
    // if ( bDeletingUL ) // lost our frame of reference!
    if ( bDeletingUpperLeft || bDeletingAboveUpperLeft )  // 2000-08-23 MRP
        {
        m_lVPosUL = 0; // BJY - This should be made more friendly! Maybe just goto start of rpsBeg field???
        m_rpsUpperLeft.SetPos( 0, m_prelCur->prec()->pfldFirst() );
        MoveCtrlHome(); // Start at top
        }
	while ( bHiddenField( rpsBeg.pfld ) ) // 1.1tn Fix bug of cursor wrong on delete field after hidden field
		rpsBeg.PrevField( 1000 ); // 1.1tn While in hidden field, move back to end of previous field

    SetScroll( TRUE );  // 2000-08-23 MRP
       
    FixInterlinAfterCut( rpsBeg ); // Fix interlinear line after cut, not allowed to have material after NL

    EditSetCaret( rpsBeg.pfld, rpsBeg.iChar ); // reset caret position
    SetModifiedFlag(); // Note that something changed
    Invalidate(FALSE);
}                             
//------------------------------------------------------
BOOL CShwView::bEditPaste() // insert contents of clipboard into record
{
    ASSERT( m_prelCur );
    Str8 sClip = sGetClipBoardData( this, m_rpsCur.pfld->plng()->bUnicodeLang() );  // 1.4rbc Encapsulate get clipboard data
    
    if ( m_rpsCur.bInterlinear() && !( *(const char*)sClip == '\\' ) )
        {
        int iChar = m_rpsCur.iChar;
        if ( iChar > 0 && *m_rpsCur.psz( iChar - 1 ) == '\n' ) // Don't paste after nl of interlinear
            return FALSE;
        if ( sClip.Find( '\n' ) >= 0 ) // Don't paste multiple lines into interlinear
            return FALSE;
        }
    DeleteSelection();
	if ( sClip.GetChar( 0 ) == '\\' && m_rpsCur.iChar == 0 && m_rpsCur.pfld->GetLength() > 0 ) // 1.5.0jb If paste field at start of field, put before field // 1.5.0jf Fix bug (1.5.0jb) of paste field wrong in empty field
		bEditLeft(); // 1.5.0jb 
    CRecPos rpsBefore = m_rpsCur;
    m_prelCur->prec()->Paste( &m_rpsCur, sClip, GetDocument()->pmkrset() ); // insert into record at current caret position
    m_undlst.Add( rpsBefore, m_rpsCur ); // make it undoable
    EditSetCaret( m_rpsCur.pfld, m_rpsCur.iChar );
    SetScroll( TRUE );
    SetCaretPosAdj();
    SetModifiedFlag(); // Note that something changed
    m_iWhatChanged = eMultipleLines;
    return TRUE;
}

//------------------------------------------------------
void CShwView::InvalidateLineContaining( CRecPos rpsTarget, BOOL bErase ) // Cause line containing rps to be redrawn
{
    ASSERT(rpsTarget.prec == m_prelCur->prec());

    if (rpsTarget < m_rpsUpperLeft) // don't bother if it's not visible
        return;

    CRect rect;
    GetClientRect(&rect); // get window dimensions

    CRecPos rpsSave = m_rpsCur;
    CPoint pntCaretLocSave = m_pntCaretLoc; // Save initial values
    
    m_rpsCur = m_rpsUpperLeft; // start at upper left of visible window
    m_pntCaretLoc.x = m_iFldStartCol;
    m_pntCaretLoc.y = 0;

    while ( rpsTarget.pfld != m_rpsCur.pfld && bMoveDown() ) // find desired field
        if (m_pntCaretLoc.y > rect.bottom)
            return; // don't need to draw if line is not visible

    while ( rpsTarget.iChar > m_rpsCur.iChar && bMoveDownLine() ) // find line within field
        ;
    if ( rpsTarget.iChar < m_rpsCur.iChar ) // move back up to line containing target pos
        bMoveUp();

    ASSERT( rpsTarget.iChar >= m_rpsCur.iChar );
    ASSERT( rpsTarget.pfld == m_rpsCur.pfld );

    rect.top = (int)m_pntCaretLoc.y; // cast should be safe with code above checking to make
                                    // sure we don't get here unless m_pntCaretLoc is visible
    rect.bottom = rect.top + rpsTarget.pfld->pmkr()->iLineHeight(); // just invalidate this line
    InvalidateRect(&rect, FALSE /*bErase*/);
    
    m_pntCaretLoc = pntCaretLocSave; // Restore initial values
    m_rpsCur = rpsSave;
}
//------------------------------------------------------
void CShwView::CalcCaretLocX() // calc x pos of caret loc using m_rpsCur
{
    CDC* pdc = GetDC(); // Get device context
    CField* pfld = m_rpsCur.pfld;
    CRecPos rps = m_rpsCur;
    rps.bMoveHome();  // beginning of current line
    if ( m_rpsCur.bInterlinear() ) // interlinear field takes a little more work
        {
		CRecPos rpsFirstLine = m_rpsCur; // 1.5.1cc 
		while ( rpsFirstLine.pfldPrev() && rpsFirstLine.bInterlinear() && !rpsFirstLine.bFirstInterlinear() ) // 1.5.1cc Find first interlinear of this bundle
			rpsFirstLine.pfld = rpsFirstLine.pfldPrev(); // 1.5.1cc Fix bug of crash from pfld becoming null
		BOOL bRTLInterlinear = rpsFirstLine.pfld->plng()->bRightToLeft();  // 1.5.1cc See if rtl interlinear bundle // 1.5.1ce 

        CFont* pfnt = (CFont*)pfld->pmkr()->pfnt();
        pdc->SelectObject( pfnt );  // select font used to display this field
        const char* pszLine = rps.psz();
        const char* psz = m_rpsCur.psz(); // find beginning of word
        while ( psz > pszLine && *(psz-1) != ' ' )
            psz--; // find beginning of word
        const char* pszWord = psz;
        while ( *psz && *psz != ' ' && *psz != '\n' )
            psz++; // find end of word
        int iXWord = iInterlinPixelPos( m_rpsCur, pszWord - pszLine ); // 6.0a width up to current word
		int iXCaretInWord = iCharToPixel( pdc, m_rpsCur.pfld, pszWord, psz-pszWord, m_rpsCur.psz() - pszWord ); // get length of word up to caret
		int iX = iXWord + iXCaretInWord; // 1.5.1cf 
        m_pntCaretLoc.x = m_iFldStartCol + iX; // Calculate new caret position
		if ( bRTLInterlinear ) // 1.5.1cf If rtl, adjust positioning for rtl
			{
			int iWdLen = psz - pszWord; // 1.5.1cf Find length of word 
			int iTextWidth = pfld->plng()->GetTextWidth( pdc, pszWord, iWdLen ); // 1.5.1ccf Measure width to right justify
			m_pntCaretLoc.x = iMargin() - iXWord - iTextWidth + iXCaretInWord;  // 1.5.1cf 
			}
        }
    else
        {
        int iX = 0; // length of text up to caret
        CRecPos rpsB;  // substring begin
        CRecPos rpsE;  // substring end
        do {
            rps.MoveRightPastViewSubstring( &rpsB, &rpsE );
            CFont* pfnt = (CFont*)rpsB.pmkr()->pfnt();  // substring font
            pdc->SelectObject( pfnt );
            const char* pszB = rpsB.psz();  // substring begin
            int iLen = rpsE.iChar - rpsB.iChar;  // substring length
            iX += m_rpsCur.iChar < rpsE.iChar ?
                    iCharToPixel( pdc, pfld, pszB, iLen, m_rpsCur.iChar - rpsB.iChar ) :
                    iCharToPixel( pdc, pfld, pszB, iLen, iLen );
            } while ( rpsE.iChar < m_rpsCur.iChar );
        m_pntCaretLoc.x = m_iFldStartCol + iX; // Calculate new caret position
        }
    ReleaseDC(pdc); // Release device context
}

//------------------------------------------------------
int CShwView::iCharsFit( CRecPos& rpsCur, int iPixels, BOOL bBrowse ) // return rps and pixel offset closest to iPixels. rps assumed to be at beginning of line
{
    if ( !bPositionValid(rpsCur) ) // 1.4zab Fix possible assert on mouse click
		return 0;  // 1.4zab 
    int iLen = pszNextLineBrk( rpsCur.psz() ) - rpsCur.psz();

    if ( !iLen ) // empty line
        return 0;

    if ( iPixels < 0 )
        iPixels = 0;

    int iX = 0;
    CDC* pdc = GetDC();
    CField* pfld = rpsCur.pfld;
    if ( rpsCur.bInterlinear() && !bBrowse ) // 5.97r Don't do interlinear calculation for browse // interlinear field takes a little more work
        {
		CRecPos rpsFirstLine = rpsCur; // 1.5.1cd 
		while ( rpsFirstLine.pfldPrev() && rpsFirstLine.bInterlinear() && !rpsFirstLine.bFirstInterlinear() ) // 1.5.1cd Find first interlinear of this bundle
			rpsFirstLine.pfld = rpsFirstLine.pfldPrev(); // 1.5.1cd Fix bug of crash from pfld becoming null
		BOOL bRTLInterlinear = rpsFirstLine.pfld->plng()->bRightToLeft();  // 1.5.1cd See if rtl interlinear bundle // 1.5.1ce 
		RememberInterlinearWidths( TRUE );
        CFont* pfnt = (CFont*)pfld->pmkr()->pfnt();
        pdc->SelectObject( pfnt );  // select font used to display this field
#ifdef _MAC
        GrafPtr pgrafport = CheckoutPort( *pdc, CA_FONT );
        CheckinPort( *pdc, CA_NONE );
#endif
		CRecPos rpsWord = rpsCur; // 6.0a Fix up for improved measurement of interlinear spacing
		rpsWord.iChar = rpsWord.pfld->GetLength(); // Start at end of word
        int iUpToWord = 0;
		int iEndPrevWord = 0;
		if ( bRTLInterlinear ) // 1.5.1cd 
			iPixels = iMargin() - m_iFldStartCol - iPixels; // 1.5.1cd Adjust for rtl interlinear

		do
			{
			rpsWord.bSkipWhiteLeft(); // Move back to end of previous word
			iEndPrevWord = rpsWord.iChar; // Remember end of word for measurement
			rpsWord.bSkipNonWhiteLeft(); // Move back to previous alignment point
			iUpToWord = iInterlinPixelPos( rpsWord, rpsWord.iChar ); // pixel offset to beginning of this word
			} while ( rpsWord.iChar > 0 && iUpToWord > iPixels );
		int iLenWord = iEndPrevWord - rpsWord.iChar;
        int iChar = iPixelToChar( pdc, pfld, rpsWord.psz(), iLenWord, iPixels - iUpToWord ); // find char at iPixels
        iX = iUpToWord + iCharToPixel( pdc, pfld, rpsWord.psz(), iLenWord, iChar ); // get exact point where caret should go
		rpsCur.iChar = rpsWord.iChar + iChar;
        ASSERT( rpsCur.iChar <= rpsCur.pfld->GetLength() );
		RememberInterlinearWidths( FALSE );
        }
    else
        {
        CRecPos rps = rpsCur;
        int iChar = 0;
        int iLen = 0;
        CRecPos rpsB;  // substring begin
        CRecPos rpsE;  // substring end
        do  {
            rps.MoveRightPastViewSubstring( &rpsB, &rpsE );
            CFont* pfnt = (CFont*)rpsB.pmkr()->pfnt();  // substring font
            pdc->SelectObject( pfnt );
#ifdef _MAC
            GrafPtr pgrafport = CheckoutPort( *pdc, CA_FONT );
            CheckinPort( *pdc, CA_NONE );
#endif
            const char* pszB = rpsB.psz();  // substring begin
            iLen = rpsE.iChar - rpsB.iChar;  // substring length
            iChar = iPixelToChar( pdc, pfld, pszB, iLen, iPixels ); // find char at iPixels
            int iSubstringX = iCharToPixel( pdc, pfld, pszB, iLen, iChar ); // get exact point where caret should go
            iX += iSubstringX;
            iPixels -= iSubstringX;
            } while ( iChar == iLen && !rpsE.bEndOfFieldOrLine() );
        rpsB.iChar += iChar;
        if ( iChar == 0 )
            // When the caret is at the boundary between two subfields,
            // consider it to be at the end of the preceding subfield.
            rpsB.bMoveLeftPastSubfieldTags();
        rpsCur = rpsB;
        }
    ReleaseDC(pdc);
    return iX; // return actual pixel offset to position returned in rps
}

//------------------------------------------------------
CLPoint CShwView::pntPosToPnt( CRecPos rpsTarget, int iRestore ) // Find caret point from internal position, iRestore asks it to restore initial values
{
    ASSERT( rpsTarget.iChar <= rpsTarget.pfld->GetLength() );
    CRecPos rpsSave = m_rpsCur;
    CLPoint pntCaretLocSave = m_pntCaretLoc; // Save initial values
    
    m_rpsCur = m_rpsUpperLeft; // start at upper left of visible window
    m_pntCaretLoc.x = m_iFldStartCol;
    m_pntCaretLoc.y = 0;

#ifdef LOOKED_GOOD_BUT_TOOK_FOREVER
    while ( rpsTarget > m_rpsCur && bMoveDown() ) // Find desired field
        ;
    while ( rpsTarget < m_rpsCur && bMoveUp() ) // back up one (or more)
        ;
    while ( rpsTarget > m_rpsCur && bMoveRight() ) // Find desired char pos
        ;
#endif

    if ( rpsTarget > m_rpsCur )
        {
        while ( rpsTarget.pfld != m_rpsCur.pfld && bMoveDown( TRUE ) ) // find desired field // 1.0bf Allow for hidden fields
            ;
        while ( rpsTarget.iChar > m_rpsCur.iChar && bMoveDownLine() ) // find line within field
            ;
        if ( rpsTarget.iChar < m_rpsCur.iChar ) // move back up to line containing target pos
            bMoveUp();
        }
    else // need to look backward in record
        {
        while ( rpsTarget.pfld != m_rpsCur.pfld && bMoveUp( TRUE ) ) // find desired field // 1.0bf Allow for hidden fields
            ;
        while ( rpsTarget.iChar < m_rpsCur.iChar && bMoveUpLine() ) // find line within field
            ;
        }
//    ASSERT( rpsTarget.iChar >= m_rpsCur.iChar ); 
//    ASSERT( rpsTarget.pfld == m_rpsCur.pfld ); // 1.0bf Temp allow it to miss

    // m_rpsCur.iChar = rpsTarget.iChar;
    m_rpsCur = rpsTarget;  // 1996-10-17 MRP: assign both offset and marker
    CalcCaretLocX(); // set m_pntCaretLoc

    CLPoint pnt = m_pntCaretLoc;
    
    if ( iRestore )
        {
        m_pntCaretLoc = pntCaretLocSave; // Restore initial values
        m_rpsCur = rpsSave;
        }
    
    return pnt;
}   
//------------------------------------------------------
CLPoint CShwView::pntPntToPos( CLPoint pnt, int iRestore, CRecPos* prps ) // Find internal position from caret point
{
    CRecPos rpsSave = m_rpsCur;
    CLPoint pntCaretLocSave = m_pntCaretLoc; // Save initial values

    //03/01/2000 TLB : The following corrects a potentially bogus upper left before we use it.
    if ( !bPositionValid( m_rpsUpperLeft ) ) // our frame of reference may have been deleted by another view
        {
        m_rpsUpperLeft.iChar = m_rpsUpperLeft.pfld->GetLength();
        m_rpsUpperLeft.bMoveHome();
        ASSERT(bPositionValid( m_rpsUpperLeft ));
        }
    m_rpsCur = m_rpsUpperLeft; // start at upper left of visible window
    m_pntCaretLoc.x = m_iFldStartCol;
    m_pntCaretLoc.y = 0;

    if ( pnt.y < 0 ) // need to go up?
        {
        while ( m_pntCaretLoc.y > pnt.y )
            if ( !bMoveUp( TRUE ) ) // Stop at beginning of record // 1.0bf Allow for hidden fields
                break;
        }
    else
        {   
        while ( m_pntCaretLoc.y + iLineHt( m_rpsCur.pfld ) <= pnt.y ) // Find nearest vertical position
            if ( !bMoveDown( TRUE ) ) // Stop at bottom // 1.0bf Allow for hidden fields
                break;
        }
    int iPixelOffset = 0; // Pixel offset is used to cause mouse click to better choose which side of character to place cursor
#ifndef _MAC
    if ( m_rpsCur.pfld->plng()->bRightToLeft() )
            iPixelOffset = -4; // On right to left script, compensate the opposite way
    else
        iPixelOffset = 2;
#endif
    m_pntCaretLoc.x += iCharsFit( m_rpsCur, pnt.x - m_iFldStartCol + iPixelOffset ); // set m_rpsCur to closest char to pnt.x
    pnt = m_pntCaretLoc;
    
    if ( prps ) // caller wants calculated location returned
        *prps = m_rpsCur;

    if ( iRestore )
        {
        m_pntCaretLoc = pntCaretLocSave; // Restore initial values
        m_rpsCur = rpsSave;
        }
    
    return pnt;
}

void CShwView::RememberInterlinearWidths( BOOL b ) // 6.0j Turn on and off the remembering of interlinear widths for speed
{
	if ( b ) // If turning remember on, force initial recalculate to start
		m_rpsPixelDisplayStarts.pfld = NULL;
	m_bRecalculatePixelDisplayStarts = !b; // Turn remembering on or off as requested
}

//----------------------------------------------------------------------------------------
int CShwView::iInterlinPixelPos( CRecPos rps, int iChar ) // 6.0a return interlinear display position in pixels of a char number position
{
	if ( iChar == 0 ) // 1.0dg Fix bug of hang on interlinear reshape if wrap margin is zero
		return 0;
    CDC* pDC = GetDC(); // Get device context
	if ( iChar > iMaxInterlinLineLen )
		iChar = iMaxInterlinLineLen;
	CRecPos rpsFirstLine = rps;
	while ( rpsFirstLine.pfldPrev() && rpsFirstLine.bInterlinear() && !rpsFirstLine.bFirstInterlinear() ) // Find first interlinear of this bundle
		rpsFirstLine.pfld = rpsFirstLine.pfldPrev(); // 6.0x Fix bug of crash from pfld becoming null
	if ( m_bRecalculatePixelDisplayStarts || rpsFirstLine.pfld != m_rpsPixelDisplayStarts.pfld ) // If current array is not the one being asked for, rebuild it
		{
		m_iPixelDisplayStarts[ 0 ] = 0; // Init first point in array
		CRecPos rpsPrev = rpsFirstLine;
		BOOL bMore = TRUE;
		int iCurPos = 1;
		for ( ; bMore; iCurPos++ ) // At each point check for alignment points
			{
			m_iPixelDisplayStarts[ iCurPos ] = 0; // Clear point in array
			CRecPos rpsLine = rpsFirstLine;
			rpsLine.iChar = iCurPos;
			bMore = FALSE;
			for ( ; ; rpsLine.NextField() )
				{
				int iLenrpsLine = rpsLine.pfld->GetLength();
				if ( iCurPos <= iLenrpsLine ) // If past end of line, skip this one
					{
					rpsLine.iChar = iCurPos; // 6.0t Set iChar back out if it was set in by shorter line above, fix bug of not showing annotations with spaces at end of line
					bMore = TRUE;
					rpsPrev = rpsLine;
					rpsPrev.iChar = rpsLine.iChar - 1;
					if ( ( rpsLine.bIsNonWhite() || rpsLine.iChar == iLenrpsLine || *rpsLine.psz() == '\n' ) // 6.0w Fix bug of cursor wrong if space at end of last line of interlinear bundle
							&& rpsPrev.bIsWhite() ) // For each line that has an alignment point here
						{												// Calculate from prev align point width to here
						rpsPrev.bSkipWhiteLeft(); // Move back to end of previous word
						int iEndPrevWord = rpsPrev.iChar; // Remember end of word for measurement
						if ( rpsLine.iChar == iLenrpsLine ) // 6.0c If at end of line, measure end spaces as well
							iEndPrevWord = iLenrpsLine;
						rpsPrev.bSkipNonWhiteLeft(); // Move back to previous alignment point
						int iLenPrevWord = iEndPrevWord - rpsPrev.iChar;
			            CFont* pfnt = (CFont*)rpsLine.pfld->pmkr()->pfnt(); // Get font for displaying field contents
						pDC->SelectObject( pfnt ); // Put it in device context to measure correctly
						int iWidthOfThisPiece = rpsLine.pfld->plng()->GetTextWidth( pDC, rpsPrev.psz(), iLenPrevWord ); // Measure width to it using rpsPrev.iChar and iEndPrevWord
						int iPlaceWithRoomForThisPiece = m_iPixelDisplayStarts[ rpsPrev.iChar ] + iWidthOfThisPiece + m_iInterlinCharWd; // Calculate place to start to give room for this piece
						if ( iPlaceWithRoomForThisPiece > m_iPixelDisplayStarts[ iCurPos ] )
							m_iPixelDisplayStarts[ iCurPos ] = iPlaceWithRoomForThisPiece;
						}
					}
				if ( rpsLine.bLastInBundle() )
					break;
				}
			}
		int iEndOfLongestLine = iCurPos - 1; // 6.0p Fix bug on space at end of line 6.0b Remember length of longest line for wrap
		rpsPrev.bSkipNonWhiteLeft(); // Move back to previous alignment point
		CFont* pfnt = (CFont*)rpsPrev.pfld->pmkr()->pfnt(); // Get font for displaying field contents
		pDC->SelectObject( pfnt ); // Put it in device context to measure correctly
		int iWidthOfThisPiece = rpsPrev.pfld->plng()->GetTextWidth( pDC, rpsPrev.psz(), iEndOfLongestLine - rpsPrev.iChar ); // Measure width to end of longest line
		m_iPixelDisplayStarts[ iCurPos - 1 ] = m_iPixelDisplayStarts[ rpsPrev.iChar ] + iWidthOfThisPiece; // 6.0k Fix bug of not getting longest line end right for wrap
		m_iPixelDisplayStarts[ iCurPos - 2 ] = m_iPixelDisplayStarts[ iCurPos - 1 ]; // 6.0k Fix bug of not getting longest line end right for wrap in some cases

		m_rpsPixelDisplayStarts.pfld = rpsFirstLine.pfld;
		}
	ReleaseDC( pDC ); // Release the device context to make more available
	return m_iPixelDisplayStarts[ iChar ]; // Return appropriate value from array
}

//------------------------------------------------------ 
// Return selected text (if any); otherwise current text
Str8 CShwView::sGetCurText(ECurTextType ctt)
{
    if (!m_prelCur || bSelecting(eLines|eFields))
        return Str8("");

    // (1) If text is selected, get selection.
    if ( bSelecting(eText) )
        {
        // If selection crosses more than one field, then punt.
        if ( m_rpsCur.pfld != m_rpsSelOrigin.pfld )
            return Str8("");
        return m_rpsCur.pfld->sItem(rpsSelBeg(), rpsSelEnd()); // get selected text
        }
    // (2) If nothing is selected and this is a single-item field, or if caller requested
    //     field contents, get entire contents.
    else if ( ((ctt & cttItem) && !m_rpsCur.pfld->pmkr()->bMultipleItemData()) ||
              (ctt & cttFieldContents) )
        {
        CRecPos rpsStart, rpsEnd;
        if ( m_rpsCur.pfld->bParseFirstItem(m_prelCur->prec(), rpsStart, rpsEnd, FALSE) )
            {
            return m_rpsCur.pfld->sItem(rpsStart, rpsEnd);
            }
        return Str8("");
        }
    // (3), Otherwise, return the word that the caret is in or touching.
    else
        {
        if ( m_rpsCur.bIsWhite() && ( !m_rpsCur.iChar || m_rpsCur.bLeftIsWhite() ) )
            return Str8(""); // not on a word
        CRecPos rps = m_rpsCur;
        rps.bSkipNonWhiteLeft(); // Find start of word, including leading undefined chars.
        rps.bSkipWdSep(); // Find start of word, stripping any leading undefined chars.
        const char* pszStart = rps.psz(); // Note start of word
        const char* pszEnd = rps.psz(); // Initialize end of word
        // Iteratively attempt to find the end of the word. If we find punctuation,
        // remember the last legitimate character, but keep looking because if the
        // punctuation is embedded (i.e., not followed by whitespace), we keep it as
        // part of the word and come back through looking once again for legitimate
        // characters in the sort order.
        while ( rps.bIsNonWhite() )
            {
            rps.bSkipWdChar(); // Move past all non-white characters in the sort order.
            pszEnd = rps.psz(); // This is the end of current word (iterative guess)
            rps.bSkipPunc(); // Move forward to space or word character
            }
        if ( pszStart == pszEnd )
            return Str8(""); // Must be on a string of undefined characters
        return Str8(pszStart, pszEnd - pszStart); // return the word
        }
}
//------------------------------------------------------
void CShwView::JumpInsert( const char* pszKey ) // insert new record and go to it
{
    Str8 sKey( pszKey ); // get a copy of key to insert
    CNumberedRecElPtr prel = m_pind->prelInsertRecord(sKey); // insert the record
    ViewInsertedRecord(prel); // setup to view new record
}
//------------------------------------------------------
BOOL CShwView::bCanBeJumpTarget() const // return TRUE if this view could be a jump target. i.e. not filtered, etc.
{
    if ( m_pind->bUsingFilter() ) // can't be a jump target if a filter is in use
        return FALSE;
    if ( !m_pind->pmrflstSecKeys()->bIsEmpty() ) // must be sorting on a single field
        return FALSE;

    return TRUE; // passed the test!
}

void ActivateWindow( CMDIChildWnd* pwnd ) // Activate window after jump
	{
    Shw_pmainframe()->MDIActivate( pwnd ); // activate the view

    WINDOWPLACEMENT wplstruct; // restore if minimized
    wplstruct.length = sizeof(wplstruct);
    pwnd->GetWindowPlacement( &wplstruct );
    if ( wplstruct.showCmd == SW_SHOWMINIMIZED )
        {
        wplstruct.showCmd = SW_SHOWNORMAL; // change window from minimized to normal view
        pwnd->SetWindowPlacement( &wplstruct );
        }
	}

//------------------------------------------------------
BOOL CShwView::bExternalJump(const char* pszWordFocusRef, CNumberedRecElPtr prelFrom, BOOL bMultMatchMessage, BOOL bJump ) // Try external jump on this view
{
	ASSERT( pszWordFocusRef );
	if ( !bJump && !m_bFocusTarget ) // 1.4qzjd Fix problem (1.4qzjb) of jumping to non-jump targets // 1.4zah Fix bug of jump target doing parallel move when off
		return FALSE;
    CDatabaseType* ptyp = m_pind->pindsetMyOwner()->ptyp();
	Str8 sDBType = ptyp->sName();
	if ( sDBType.Find( "Concordance" ) == 0 ) // 5.98f Don't jump concordance window on reference jump
		return FALSE;
    if ( strlen(pszWordFocusRef) == 0 )
        return FALSE;
    // A word focus reference consists of up to three parts. Use newline to separate the parts.
    // 1. Word form, e.g. akka 2. Type, i.e. surface or lemma 3. Language encoding, e.g. Chulupi
    const char* psz = pszWordFocusRef;
    size_t len = strcspn(psz, "\n");
    Str8 sWord(psz, len);
	BOOL bMatchWhole = TRUE; // sWord.Find( ' ' ) >= 0; // If space included anywhere, then match whole thing
	sWord.TrimLeft();
	sWord.TrimRight();
    psz += len;
    if ( *psz == '\n' )
        psz++;
    len = strcspn(psz, "\n");
    Str8 sType(psz, len);
    psz += len;
    if ( *psz == '\n' )
        psz++;
    len = strcspn(psz, "\n");
    Str8 sLanguageEncodingRef(psz, len);
    CLangEnc* plng = m_pind->pmkrPriKey()->plng();
    const Str8& sLanguageEncodingPriKey = plng->sName();
    if ( !sLanguageEncodingRef.IsEmpty() &&
            !bEqual(sLanguageEncodingRef, sLanguageEncodingPriKey) )
        return FALSE;            
    WINDOWPLACEMENT wplstruct; // 1.1bv Reminimize when a minimized view does parallel movement
    GetParent()->GetWindowPlacement(&wplstruct); // 1.1bw Reminimize when a minimized view does parallel movement (1.1bv didn't quite work)
    BOOL bWasMinimized = wplstruct.showCmd == SW_SHOWMINIMIZED; // 1.1bv Reminimize when a minimized view does parallel movement
    BOOL bWasMaximized = wplstruct.showCmd == SW_SHOWMAXIMIZED; // 1.4zbh Remaximize when a maximized view does external jump
    m_keySearch.SetKeyText(sWord); // Get the text to jump to
    CNumberedRecElPtr prel = prelFrom; // 1.5.1nc Make parallel jump choose same record if possible
	if ( !prel ) // 1.5.1nc 
		prel = m_prelCur; // 1.5.1nc 
    int iResult = m_pind->iSearchFocusTarget(&m_keySearch, &prel, bMatchWhole, bMultMatchMessage );
    if (iResult != CIndex::eSearch) // If search failed, fail
        return FALSE;
	if ( prel == m_prelCur && !bJump ) // 1.1pa Don't flash window if parallel move is to current place // 1.4qzje Fix problem (1.4qzjd) of fail to jump to current ref
		return FALSE; // 1.1pa Return false to say that parallel move didn't move this window
    SetCur(prel); // Set up to display new record       
	FixCursorPosForHidden(); // 1.5.0fp Change to better way of positioning cursor after jump
// 	PlaceCursorAtTopOfWindow(); // 5.98d Go down and back up so that the material after the jump is showing // 1.5.0fp 
    if ( m_bIsJumpTarget ) // If jump, not just focus, make this the active window // 1.4qzjd
		{
		CMDIChildWnd* pwnd = pwndChildFrame();
		ActivateWindow( pwnd );
		if ( bWasMinimized ) // 1.1bv Reminimize when a minimized view does parallel movement
			{
			wplstruct.showCmd = SW_SHOWMINIMIZED; // change window back to minimized
			pwnd->SetWindowPlacement( &wplstruct );
			}
		if ( FALSE ) // bWasMaximized ) // 1.4zbh Remaximize when a maximized view does external jump // 1.5.0cd Undo 1.4bzh, caused problems
			{
			wplstruct.showCmd = SW_SHOWMAXIMIZED; // change window back to maximized // 1.4zbh 
			pwnd->SetWindowPlacement( &wplstruct ); // 1.4zbh 
			}
		}
	return TRUE;
}

#ifdef JUNK // 1.5.0fp Replaced by FixCursorPosForHidden(); // 1.0br Fix cursor pos if hidden fields
void CShwView::PlaceCursorAtTopOfWindow() // 6.0zh Place cursor at top of window after jump if in the middle of a long record
{
	if ( !m_rpsCur.prec ) // 1.0ea Fix bug of crash when last filtered record disappears
		return;
	if ( m_rpsCur.pfld == m_rpsCur.prec->pfldFirst() ) // 6.0zk Don't go down and up if already at top of record
		return;
	int iNumDownToTry = 30; // Number of lines down to try
	if ( !bBrowsing() ) // Don't bother if browsing
		{
		for ( int iNumDownActual = 0; iNumDownActual < iNumDownToTry; iNumDownActual++ )
			{
			EditEnd(); // 5.98e Go to end so edit down won't falsely claim to go down on last line
			if ( !bEditDown() ) // If it doesn't successfully go down, stop trying
				break;
			}
		SetCaretPosAdj(); // Scroll the cursor into view
		bEditHome();
		for ( int i = 0; i < iNumDownActual; i++ ) // Move back up same number as went down
			bEditUp();
		bEditHome(); // 5.98e Make sure the cursor is at beginning of ref
	    SetCaretPosAdj(); // Scroll the cursor into view
		}
}
#endif // JUNK // 1.5.0fp 

void CShwView::EditJumpTo( BOOL bIsJumpInsert, BOOL bDontShowDlg, BOOL bJumpEvenIfEmptyString ) // jump to record matching selected text
{
//    CKey* pkey = bIsJumpInsert ? &m_keyJumpInsert : &m_keyJump;

    // 1998-02-02 MRP: Moved this code to fix bug #203.
    // Must call sGetCurText and bSelecting before bValidateAllViews,
    // because the selected text or caret position changes
    // when there's a consistency violation and when the record is reindexed.
    Str8 sWord;
    if ( !m_bBrowsing )
        {
        // get selected text or item (current word or entire field contents) at caret position
        sWord = sGetCurText();
		int i = 0;
		while ( ( i = sWord.Find( "  " ) ) >= 0 ) // 1.2ce Don't include multiple spaces in jump or jump insert
			sWord = sWord.Left( i ) + sWord.Mid( i + 1 );
        }
    if ( sWord.IsEmpty() && !bJumpEvenIfEmptyString )
        {
        MessageBeep(0);
        return;
        }
    // If we have selected text or caller wants to unconditionally accept "current" text
    // without first displaying it for user, then don't show the Jump dialog box on the
    // first pass.
    BOOL bShowDlg = !bSelecting(eText) && !bDontShowDlg;
    CMarker* pmkr;
    if ( m_prelCur && !m_bBrowsing )
        pmkr = m_rpsCur.pfld->pmkr();
    else
        pmkr = NULL;
    EditJumpTo(bIsJumpInsert, bShowDlg, sWord, pmkr);
}

// 03-13-2000 TLB : Split EditJumpTo into two separate functions to enable iplementation of right-click jumping from browse view
void CShwView::EditJumpTo( BOOL bIsJumpInsert, BOOL bShowDlg, Str8 sWord, CMarker* pmkr ) // jump to record matching selected text
{
//    CKey* pkey = bIsJumpInsert ? &m_keyJumpInsert : &m_keyJump;

    // 1998-02-02 MRP: Moved this code to fix bug #203.
    // Must call sGetCurText and bSelecting before bValidateAllViews,
    // because the selected text or caret position changes
    // when there's a consistency violation and when the record is reindexed.

    if ( !Shw_papp()->bValidateAllViews() ) // valid everyone, including possible jump target! // 1.5.0kc  // 1.5mc
        return; // 1.5.0kc Don't do consistency check on jump // 1.5mc Undo 1.5.0kc, it caused a major bug

//    pkey->SetKeyText(sWord);

    CShwDoc* pdoc = GetDocument(); // pass in current document in case no jump path setup yet
    CNumberedRecElPtr prel = NULL;
    CDatabaseType* ptyp = m_pind->pindsetMyOwner()->ptyp();
    CLangEnc* plng;
    if ( pmkr )
        plng = pmkr->plng(); // get language for use in dialogs
    else
        plng = m_pind->pmkrPriKey()->plng(); // we have to set it to something
    CIndex* pind = NULL;

	Str8 sFileName = sUTF8( pdoc->GetTitle() ); // 1.6.4v Get file name
	if ( sFileName.Find( "WordParse" ) >= 0 ) // 1.6.4v If from WordParse, set WordParse return from jump
		Shw_papp()->SetViewJumpedFromWordParse( Shw_papp()->pviewJumpedFrom() ); // Save previous return from jump view // 1.6.4v 
	Shw_papp()->SetViewJumpedFrom( this ); // Remember this view as jump source for return from jump

	// 5.98r If this is a jump from a wordlist or conc reference, put word into find for Ctrl+G
	Str8 sMarker = m_rpsCur.pfld->sMarker();
	Str8 sDBType = ptyp->sName();
	CRecPos rps = m_rpsCur;
	BOOL bJumpFromRef = FALSE; // 1.1aj Automatically do find next after ref jump from wordlist or conc
	int iRefNum = 1; // 1.2ma Which number of reference this one is
	if ( sDBType.Find( "Word List" ) == 0 && sMarker == "r" ) 
		{
		bJumpFromRef = TRUE; // 1.1aj Automatically do find next after ref jump from wordlist or conc
		rps.pfld = rps.prec->pfldFirst(); // Find word field
		}
	if ( sDBType.Find( "Concordance" ) == 0 && sMarker == "concref" ) 
		{
		bJumpFromRef = TRUE; // 1.1aj Automatically do find next after ref jump from wordlist or conc
		CMarker* pmkrRef = m_rpsCur.pfld->pmkr(); // 1.2ma Remember reference marker
		CMarker* pmkrWord = ptyp->pmkrset()->pmkrSearch( "conctar" );
		rps.pfld = rps.prec->pfldFirstWithMarker( pmkrWord ); // Find word field // 1.2ma Remember word field
		Str8 sWordFind = rps.pfld->sContents(); // 1.2eb Collect info needed for find if jump from ref
		sWordFind.TrimLeft(); // 1.2cb Fix bug of jump from conc not always finding if space in front
		sWordFind.TrimRight(); // 1.2cb Fix bug of jump from conc not always finding if space in front
		CRecLookEl* prelT = m_pind->prelPrev( m_prelCur ); // 1.2ma On reference jump from concordance, count refs to find correct one
		for ( ; prelT; prelT = m_pind->prelPrev( prelT ) ) //AB 5.1a For each record in file (changed from only first record)
			{
			CRecord* prec = prelT->prec();
			CField* pfldRef = prec->pfldFirstWithMarker( pmkrRef );
			CField* pfldWord = prec->pfldFirstWithMarker( pmkrWord );
			Str8 sWord = pfldWord->sContents();
			sWord.TrimLeft();
			sWord.TrimRight();
			if ( pfldRef->sContents() == m_rpsCur.pfld->sContents() 
					&& sWord == sWordFind ) // 1.2ma If previous is same word on same ref, count it, otherwise break
				iRefNum++;
			else
				break;
			}
		}
	Str8 sFind = rps.pfld->sContents(); // 1.2eb Collect info needed for find if jump from ref
	sFind.TrimLeft(); // 1.2cb Fix bug of jump from conc not always finding if space in front
	sFind.TrimRight(); // 1.2cb Fix bug of jump from conc not always finding if space in front
	CLangEnc* plngFind = rps.pfld->plng();

	CFindSettings* pfndset = Shw_pProject()->pfndset(); // 1.2de Remember user's find settings to restore after ref jump
	MatchSetting matsetPrev = pfndset->matsetCur(); // 1.2eb Remember user's match settings to restore
	BOOL bFindMatchWholePrev = pfndset->bMatchWhole();
	BOOL bFindSingleFieldPrev = pfndset->bSingleField();
	BOOL bFindSingleRecordPrev = pfndset->bSingleRecord(); 
	Str8 sFindMkrPrev = pfndset->pszMkr();

	if ( pmkr && ( pmkr->pjmpPrimary() || pmkr->pjmpDefault() ) ) // If jump path, do jump
		{
		 // search for it via jump path
		CJumpPathSet::EJumpSearchResult jsrResult =
			GetDocument()->ptyp()->pjmpset()->jsrSearch(pmkr, sWord, &prel, &pind, &pdoc, ptyp,
														plng, bIsJumpInsert, bShowDlg);
		if (jsrResult == CJumpPathSet::jsrCancel)
			return;

		JumpToKey((jsrResult == CJumpPathSet::jsrJumpInsert), sWord, pind, pdoc, prel);
//		FixCursorPosForHidden(); // 1.5.0fp Change to better way of positioning cursor after jump
		}
	else // Else, no jump path, do focus instead
		{
		Str8 sFocus( sWord + "\n\n" + plng->sName() );
		BOOL bJumpFromBookRef = ( plng->sName() == "Book References" ); // 1.4ks
		BOOL bSucc = Shw_papp()->bExternalJump( sFocus, this, !bJumpFromBookRef ); // 1.4ks Don't show no matches box on jump from book ref // Pass in this view so it can be skipped
		if ( bSucc && bJumpFromRef ) // 1.1aj Automatically do find next after ref jump from wordlist or conc // 1.1ap Don't do automatic find if jump doesn't find anything
			{
			CShwView* pview = Shw_pmainframe()->pviewGetActive(); // 1.1aj Automatically do find next after ref jump from wordlist or conc
			pfndset->pftxlst()->Add( sFind, plngFind ); // 1.2eb Fix bug (in 1.2de) of ref jump not restoring user's find settings if jump failed
			MatchSetting matsetConc = CMCharOrder::matsetEvenIgnore; // 1.4vzn 
			if ( plngFind->bHasCase() ) // 1.4qzma Fix bug of wordlist jump failing to find some unicode words // 1.4vyz  // 1.4yr Fix U bug of jump from word list ref failing to highlight upper case
				matsetConc = CMCharOrder::matsetExactDisregardCase; // 1.4vzn Fix U bug of jump from concordance failing to highlight
//				pfndset->Set( CMCharOrder::matsetExactDisregardCase, TRUE, FALSE, TRUE, rps.pfld->pmkr()->sFieldName(), "", FALSE, FALSE ); // 1.2mx Fix bug of jump from word list ref failing to find capitalized word // 1.2ec Ref jump from wdlst include ignores to more often hit correct rec // 1.4vzn 
//			else // 1.4vzn 
			CCorpusSet* pcorset = Shw_pcorset(); // 1.4vzn 
			BOOL bWholeWord = !( pcorset->bConcordanceAlignMatchedStrings() && ( pcorset->nConcordanceSearchType() < 3 ) ); // 1.4vzn 
			pfndset->Set( matsetConc, bWholeWord, FALSE, TRUE, rps.pfld->pmkr()->sFieldName(), "", FALSE, FALSE ); // 1.2mx Fix bug of jump from word list ref failing to find capitalized word // 1.2ec Ref jump from wdlst include ignores to more often hit correct rec // 1.4qzma // 1.4vzn 
			for ( int i = 0; i < iRefNum; i++ ) // 1.2ma Repeat find as many times as word occurs earlier in same record
				pview->bEditFind( TRUE ); // have view do find operation
			pfndset->pftxlst()->Delete( pfndset->pftxlst()->pftxFirst() ); // 1.2de Delete the automatic find so it doesn' clutter user's find list
			pfndset->Set( matsetPrev, bFindMatchWholePrev, bFindSingleFieldPrev, bFindSingleRecordPrev, sFindMkrPrev, "", FALSE, FALSE ); // 1.2eb Restore user's find settings after wordlist ref jump

			pview->SetFocus(); // force focus back to view
			}
		if ( !bSucc && plng->sName() == "Book References" ) // 1.4kr Fix bug of jump not sending ref to Paratext
			{
			SendParallelMoveRefExternal( sWord + "\n\n" ); // 1.4kr If jump didn't succeed, then no other window triggered external parallel move, so send it out here // 1.5.4d Don't send language encoding on external parallel move
			// Make this the active view, since succesful jump will have left it at another view
			CMDIChildWnd* pwnd = pwndChildFrame(); // 1.4kr
			ActivateWindow( pwnd );
			}
		}
}

void CShwView::JumpToKey( BOOL bInsert, const Str8& sWord, CIndex* pind, CShwDoc* pdoc, CNumberedRecElPtr prel)
// used by EditJumpTo and CJumpPath::bCheckReferentialIntegrity to actually do the jump or jump-insert.
{
    ASSERT( pdoc );

    // 1998-06-18 MRP: First prefers a view whose index matches exactly;
    // otherwise returns a corresponding record element in the view's index.
    CShwView* pviewJumpTarget = pdoc->pviewJumpTarget( pind, &prel ); // try to find a jump target
    if ( pviewJumpTarget ) // jump target found, use it
        {
        if ( bInsert )
            pviewJumpTarget->JumpInsert( sWord );
        else // plain jump
            pviewJumpTarget->SetCur( prel );
        CMDIChildWnd* pwnd = pviewJumpTarget->pwndChildFrame();
		ActivateWindow( pwnd );
		pviewJumpTarget->FixCursorPosForHidden(); // 1.5.0fp Change to better way of positioning cursor after jump
        return;
        }

    // no valid jump target, make a new frame
    CDocTemplate* pTemplate = pdoc->GetDocTemplate();
    ASSERT_VALID(pTemplate);
    CFrameWnd* pFrame = pTemplate->CreateNewFrame(pdoc, NULL);
    ASSERT( pFrame );
    ASSERT( !s_pViewToDuplicate );
    if ( pdoc == GetDocument() ) // if jumping within current doc, use current view as source
        s_pViewToDuplicate = this;
    else
        { // otherwise, find an existing view for target doc to use as duplicating source
        POSITION pos = pdoc->GetFirstViewPosition();
        ASSERT( pos );
        CShwView* pshv = (CShwView*)pdoc->GetNextView(pos); // get first available view on this doc
        ASSERT( pshv );
        s_pViewToDuplicate = pshv;
        }

    s_pdocJumpTo = pdoc;
    s_pindJumpTo = pind; // index to be used in new view
    if ( bInsert )
        {
        s_pszJumpInsert = sWord; // set pointer to key to insert once we've jumped
        s_prelJumpTo = NULL;
        }
    else
        {
        s_pszJumpInsert = NULL;
        s_prelJumpTo = prel; // set record to go to in new view
        }
    pTemplate->InitialUpdateFrame(pFrame, pdoc);
        // causes OnInitialUpdate of new view, based on s_pViewToDuplicate
    s_prelJumpTo = NULL;
    s_pszJumpInsert = NULL;
}

void CShwView::OnEditReturnFromJump() 
	{
	BOOL bModified = pdoc()->IsModified();
    if ( !pdoc()->bValidateViews(TRUE, NULL) ) // validate all views of this doc
        return; // view failed the test
	CShwView* pview = Shw_papp()->pviewJumpedFrom(); // Get last view jumped from
	Shw_papp()->SetViewJumpedFrom( NULL ); // Clear last view jumped from
	Str8 sFileName = sUTF8( pview->pdoc()->GetTitle() ); // 1.6.4v Get file name
	if ( sFileName.Find( "WordParse" ) >= 0 ) // 1.6.4v If to WordParse, fix its return from jump
		Shw_papp()->SetViewJumpedFrom( Shw_papp()->pviewJumpedFromWordParse() ); // 1.6.4v Update jumped from of WordParse
	if ( pview && Shw_papp()->bViewValid( pview ) ) // If view jumped from is still valid, activate it
		{
		CMDIChildWnd* pwnd = pview->pwndChildFrame();
		ActivateWindow( pwnd );
		}
	// If this view was modified, and place jumped from is in interlinear, reinterlinearize
	if ( bModified && pview && pview->m_rpsCur.pfld && pview->m_rpsCur.bInterlinear() ) // ALB 5.96s Check for no field // 1.5.3e Fix bug of possible crash on hit tab in last field
		pview->OnEditInterlinearize();
	}

//--------------------------------------------------------------------------
//07-30-1997
void CShwView::LineLengthCheck()
{
	return; // 5.96b ALB Remove this to fix Bruce Cain bug of chopping in the middle of Unicode char. Needs test to see if it causes problems
}
