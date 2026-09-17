// shwvbrs.cpp : implementation of browse parts of the CShwView class
//

#include "stdafx.h"
#include "toolbox.h"
#include <limits.h>

#include "shwdoc.h"
#include "shwview.h"
#include <ctype.h>
#include "project.h"
#include "mkr_d.h"   //09-12-1997
#include "browse.h"  //09-26-1997
#include "doclist.h"
#include "mainfrm.h" // 02-29-2000 TLB : Shw_pmainframe (remove if we decide not to use this in SwitchToRecordView)
#include "shw.h"     // 02-29-2000 TLB : "
#include "kmn.h" // 1.4qzfb

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

static void log( char* format, ...) // for debugging release versions... yuk!
{
    char buf[256];
    char* arguments = (char*)&format+sizeof(format);
    wvsprintfA( buf, format, arguments); // 1.4qwp Upgrade wvsprintf for Unicode build
	FILE* fp = nullptr;
	fopen_s(&fp, "shw.log", "a+");
    ASSERT( fp );
    fputs( buf, fp );
    fclose( fp );
}

//------------------------------------------------------
void CShwView::InitBrowse(BOOL bCenterBrowseCur) // find current record and center it in view
{
    ASSERT(m_prelCur);
	m_rpsCur.prec = m_prelCur->prec(); // 1.1cg Fix big of parallel move not working from browse when first started
    if (!m_iBrowseHeaderHeight)
        CalculateBrowseHeaderHeight();  //09-15-1997
    if (bCenterBrowseCur)
        {
        m_prelUL = m_pind->pnrlFirst();
        while ( m_prelUL != m_prelCur && iDownRec( &m_prelUL ) ) // search through record set for current record
            ;
        CRect rect;
        GetClientRect( &rect );
        
          //09-12-1997 Added m_iBrowseHeaderHeight
        int iLines = (rect.bottom - m_iBrowseHeaderHeight) / m_brflst.iLineHt() / 2; // scroll back up half a window so current rec is in middle of window
        int i;
        for ( i = 0; i < iLines; i++ )
            if ( !iUpRec( &m_prelUL ) )
                break;
        }
    BrowseSetScroll( TRUE );
}
//------------------------------------------------------
void CShwView::BrowseScrollToCur() // make so current record is visible in browse view
{
    CRect rect;
    GetClientRect( &rect ); // Get size of client area of current window
    
      //09-12-1997 Added m_iBrowseHeaderHeight
    int iLinesViewable = (rect.bottom - m_iBrowseHeaderHeight) / m_brflst.iLineHt(); // calc how many lines fit in view
    
      //09-15-1997 The next command provides right behavior if the window is so
      //small, that only the Header and one Record are visible (this record is
      //not fully visible
    if (!iLinesViewable && (rect.bottom > m_iBrowseHeaderHeight) )    
        iLinesViewable = 1;
    
    long lDiff = m_prelCur - m_prelUL; // calc distance between current and first visible records

    if ( lDiff+1 > iLinesViewable ) // If cursor moved off the bottom, scroll into view, allowing for height of line
        lBrowseVScroll( ( (lDiff+1) - iLinesViewable ) * m_brflst.iLineHt() ); // Allow for height of line
    else if ( lDiff < 0 ) // If current record moved off the top, scroll into view
        lBrowseVScroll( lDiff * m_brflst.iLineHt() );
}

static float fYScrollHt = 10000; // Logical value for 100% of window height, 1.1hp Make flost to handle very large records and db's
static BOOL bInBrowseSetScroll = FALSE;
//------------------------------------------------------
void CShwView::BrowseSetScroll( BOOL bCalc ) // Set scroll sizes, calculate if bCalc is true
{
    BOOL bWasInBrowseSetScroll = bInBrowseSetScroll;
    bInBrowseSetScroll = TRUE;
    CRect rect;
    GetClientRect( &rect );
      
    int iHt;
    
      // 09-15-1997 Added m_iBrowseHeaderHeigth for correct appearance of the scroll bar
    if ( rect.bottom - m_iBrowseHeaderHeight > m_pind->lNumRecEls() * m_brflst.iLineHt() )
        iHt = 1; // no vertical scroll bar needed
    else // I'm sure this makes sense in some strange way
		{
		iHt = (int)(( (float)m_pind->lNumRecEls() * m_brflst.iLineHt() - rect.bottom + m_iBrowseHeaderHeight) 
				* fYScrollHt / ( m_pind->lNumRecEls() * m_brflst.iLineHt() ) + rect.bottom + 1 );
		}
    SetScrollSizes( MM_TEXT, CSize( 0, iHt ) ); // this may call OnSize which will call us again
    
    if ( bWasInBrowseSetScroll ) // avoid recursive action from OnSize()
        return;
    if ( iHt != 1 )
        SetScrollPos( SB_VERT, (int)( m_prelUL.lNum() * fYScrollHt / m_pind->lNumRecEls() ) );
    else if ( m_prelUL.lNum() ) // no scroll bar now, make sure whole record is viewable
        {
        if ( m_prelCur )
            m_prelUL = m_pind->pnrlFirst();
        Invalidate(FALSE);
        }
    GetClientRect( &rect ); // don't use settings from beginning of func, client area may have changed
    if ( m_iXWidth != rect.right ) // reformat fields if width changed
        {
        m_iXWidth = rect.right; // remember to avoid unnecessary recalculation of browse field widths
        ResizeBrowse();
        }
    bInBrowseSetScroll = FALSE;
}
//------------------------------------------------------
int CShwView::iBrowseWidth() // Get total width of browse view by summing field widths and divider widths
{
    int iFields = (int)m_brflst.lGetCount();
    if ( !iFields )
        return 0;

    int iDividerWidths = eDividerPad * 2 + eBrowseDividerWidth * (iFields-1); // account for the horizontal space used by dividers
    int iFldWidths = 0;
    for ( CBrowseField* pbrf = m_brflst.pbrfFirst(); pbrf && iFields--; pbrf = m_brflst.pbrfNext( pbrf ) )
        iFldWidths += pbrf->iWidth();

    return iFldWidths + iDividerWidths;
}
//------------------------------------------------------
void CShwView::VerifyBrowseFields() // Make sure browse field widths match window size, etc.
{
    if ( m_brflst.lGetCount() ) // anything in list yet?
        ResizeBrowse();
    else
        ResetBrowseFields(); // no field list established, go make one now
}
//------------------------------------------------------
void CShwView::ResizeBrowse() // handle window size change for browse
{
    int iFldWidths = iBrowseWidth(); // get width according to browse field list
    ASSERT( iFldWidths );
    CRect rect;
    GetClientRect( &rect ); // get width of window

    if ( iFldWidths == rect.right ) // do values match?
        return; // yep, nothing to do

    if ( iFldWidths > rect.right ) // need to shrink
        {
        int iShrink = iFldWidths - rect.right; // how much we need to shrink by
        CBrowseField* pbrf = m_brflst.pbrfLast();
        while ( pbrf && iShrink )
            {
            int iFldWidth = pbrf->iWidth(); // get current field width
            if ( iFldWidth >= iShrink ) // will adjusting this field be sufficient?
                {
                pbrf->SetWidth( iFldWidth - iShrink ); // shrink this field
                iShrink = 0;
                }
            else // take field to zero width and go on to next field
                {
                iShrink -= pbrf->iWidth();
                pbrf->SetWidth( 0 );
                }
            pbrf = m_brflst.pbrfPrev( pbrf ); // back up to field to our left
            }
        }
    else // window grew
        {
        CBrowseField* pbrf = m_brflst.pbrfLast();
        pbrf->SetWidth( pbrf->iWidth() + ( rect.right - iFldWidths ) ); // stretch rightmost field
        }
}
//------------------------------------------------------
void CShwView::ResetBrowseFields() // Set browse fields to default state - fields listed in sort order
{
    m_brflst.DeleteAll(); // clear any previous setup
	CShwView* pviewSameBrowse = NULL; // 1.4qnd Default browse same as another view of this db type
    CDocList doclst; // 1.4qnd
    for ( CShwDoc* pdocB = doclst.pdocFirst(); pdocB; pdocB = doclst.pdocNext() ) // 1.4qnd For every doc, see if same type as this
		{
		if ( pdocB != pdoc() && pdocB->ptyp() == ptyp() ) // 1.4qnd If same type, get view // 1.4qne Fix bug (1.4qnd) of divide by zero in new browse because of finding self
			{
			POSITION pos = pdocB->GetFirstViewPosition();
			while (pos != NULL)
				pviewSameBrowse = (CShwView*)pdocB->GetNextView(pos); // 1.4qnd Remember last view on list
			}
		}
	if ( pviewSameBrowse ) // 1.4qnd If view of same type found, use its browse fields
		m_brflst = *(pviewSameBrowse->pbrflst()); // 1.4qnd Copy browse list from view
	else
		{
		Str8 sTyp = ptyp()->sName(); // 1.4qnc Change default browse fields for MDF to lx, ps, ge
		if ( sTyp.Find( "MDF" ) == 0 ) // 1.4qnc If type starts with MDF, set its browse fields
			{
			CMarker* pmkr = pdoc()->pmkrset()->pmkrSearch_AddIfNew( "lx" ); // 1.4qnc Find a marker
			m_brflst.Add( pmkr ); // 1.4qnc Add marker to browse list
			pmkr = pdoc()->pmkrset()->pmkrSearch_AddIfNew( "ps" ); // 1.4qnc Find a marker
			m_brflst.Add( pmkr ); // 1.4qnc
			pmkr = pdoc()->pmkrset()->pmkrSearch_AddIfNew( "ge" ); // 1.4qnc Find a marker
			m_brflst.Add( pmkr ); // 1.4qnc
			}
		else // If not MDF, use sorting to set up browse fields
			{
			m_brflst.Add( m_pind->pmkrPriKey() ); // primary key in left column
			CMarkerRefList* pmrflst = m_pind->pmrflstSecKeys();
			for ( CMarkerRef* pmrf = pmrflst->pmrfFirst(); pmrf; pmrf = pmrflst->pmrfNext( pmrf ) )
				m_brflst.Add( pmrf->pmkr() ); // add any secondary keys
			}
		BrowseJustificationDefault();   //09-10-1997
		ResetBrowseFieldWidths();
		}
}
//------------------------------------------------------
void CShwView::ResetBrowseFieldWidths() // Set browse field widths to default - all equal based on windows size
{
    int iFields = (int)m_brflst.lGetCount();
    if ( !iFields )
        return; // gets called from OnSize() before list is established
    CRect rect;
    GetClientRect( &rect ); // get width of window
    int iWndWidth = rect.right;    
    int iDividerWidths = eDividerPad * 2 + eBrowseDividerWidth * (iFields-1); // account for the horizontal space used by dividers
    int iFldWidth = ( iWndWidth - iDividerWidths) / iFields; // make all fields of equal width
    CBrowseField* pbrf = m_brflst.pbrfFirst();
	for (; pbrf && --iFields; pbrf = m_brflst.pbrfNext( pbrf ) )
        {
        // Set field widths
        pbrf->SetWidth( iFldWidth );
        iWndWidth -= iFldWidth;
        }
    ASSERT( pbrf );
    pbrf->SetWidth( iWndWidth - iDividerWidths ); // set last field to whatever is left
}
//------------------------------------------------------
CBrowseField* CShwView::pbrfPtInDivider( CPoint pnt ) // return browse field containing point or NULL
{
    CBrowseField* pbrf = m_brflst.pbrfFirst();
    ASSERT( pbrf );
    int iX = eDividerPad + pbrf->iWidth(); // jump past first field and padding

    for ( pbrf = m_brflst.pbrfNext( pbrf ); pbrf; pbrf = m_brflst.pbrfNext( pbrf ) )
        {
        if ( pnt.x < iX ) // already past pnt?
            return NULL;
        if ( pnt.x < iX + eBrowseDividerWidth )
            {
            return pbrf; // pnt lies within vertical divider
            }
        iX += pbrf->iWidth() + eBrowseDividerWidth; // goto next field
        }
    return NULL;
}
//------------------------------------------------------
BOOL CShwView::bPtInDivider( CPoint pnt ) // Is point in a vertical divider?
{
    if ( m_bBrowsing )
        {
        return( pbrfPtInDivider( pnt ) != NULL );
        }
    else    // normal record view
        {
        pnt.x += GetScrollPosition().x;
        return( pnt.x < m_iFldStartCol-eDividerTextPad-2 && pnt.x >= m_iFldStartCol - eRecordDividerWidth+1 );
        }
}                             

static int iMouseOffset, iXMax, iXMin;
static CBrowseField* pbrfMoving;

//------------------------------------------------------
void CShwView::StartMovingDivider( CPoint pnt ) // setup to allow user to move vertical divider bars
{
    if ( m_bBrowsing )
        {
        pbrfMoving = pbrfPtInDivider( pnt ); // remember which divider we're moving
        ASSERT( pbrfMoving );
        iXMax = pnt.x + pbrfMoving->iWidth() - 1; // establish boundaries for moving this divider
        iXMin = pnt.x - m_brflst.pbrfPrev( pbrfMoving )->iWidth() - 1;
        }
    else // normal record view
        {
        iMouseOffset = m_iFldStartCol - ( pnt.x + GetScrollPosition().x );
        }
}
//------------------------------------------------------
void CShwView::MoveDivider( CPoint pnt ) // try to move divider to pnt.x
{
    if ( m_bBrowsing )
        {
        if ( pnt.x < iXMin )
            pnt.x = iXMin;
        else if ( pnt.x > iXMax )
            pnt.x = iXMax;
        
        CBrowseField* pbrfPrev = m_brflst.pbrfPrev( pbrfMoving ); // need to adjust previous field also
        if ( pnt.x - iXMin != pbrfPrev->iWidth() ) // did anything actually change?
            {
            pbrfPrev->SetWidth( pnt.x - iXMin ); // adjust the widths of the two fields on either side of this divider
            pbrfMoving->SetWidth( iXMax - pnt.x );
            Invalidate(FALSE);
            }
        }
    else // normal record view
        {
        CRect rect;
        GetClientRect( &rect );
        pnt.x += iMouseOffset + GetScrollPosition().x;
        if ( pnt.x < rect.left + eRecordDividerWidth )
            pnt.x = rect.left + eRecordDividerWidth;
        else if ( pnt.x > rect.right - eRecordDividerWidth )
            pnt.x = rect.right - eRecordDividerWidth;
        if ( pnt.x != m_iFldStartCol )
            {
            m_pntCaretLoc.x += pnt.x - m_iFldStartCol;
            m_iFldStartCol = pnt.x;
            SetCaretLoc();
            Invalidate(FALSE);
            }
        }
}
//------------------------------------------------------
int CShwView::iDownRec( CNumberedRecElPtr* pprel ) // move rel down a record returning height of line moved from
{
    CNumberedRecElPtr prel = m_pind->pnrlNext( *pprel );
    if ( prel ) // if another record, move to it
        {
        *pprel = prel;
        return m_brflst.iLineHt(); // return height of line  BJY this seems a little odd, maybe should rework
        }
    return 0;
}
//------------------------------------------------------
int CShwView::iUpRec( CNumberedRecElPtr* pprel ) // move rel up a record returning height of line moved to
{
    CNumberedRecElPtr prel = m_pind->pnrlPrev( *pprel );
    if ( prel ) // if another record, move to it
        {
        *pprel = prel;
        return m_brflst.iLineHt(); // return height of line  BJY this seems a little odd, maybe should rework
        }
    return 0;
}
//------------------------------------------------------
long CShwView::lBrowseVScroll( long lPixels, BOOL bForce ) // perform actual window scroll of at least iPixels
{
    long lActual = 0;

    if ( !bForce )
        {
        int yMin, yMax, y = GetScrollPos(SB_VERT);
        GetScrollRange(SB_VERT, &yMin, &yMax);
        ASSERT(yMin == 0);
        long lPixelMax = (long)( (float)yMax * m_pind->lNumRecEls() * m_brflst.iLineHt() / fYScrollHt );
        long lTarget = m_prelUL.lNum() * m_brflst.iLineHt() + lPixels;
        if ( lTarget < 0 )
            lPixels = -m_prelUL.lNum() * m_brflst.iLineHt();
        else if ( lTarget > lPixelMax && lPixels > 0 )
              //09-15-1997 Added m_iBrowseHeaderHeight
            lPixels = max( 0, lPixelMax - m_prelUL.lNum() * m_brflst.iLineHt() + m_iBrowseHeaderHeight );
        }

    if ( lPixels > 0 ) // scroll down
        {
        while ( lActual < lPixels )
            {
            int i = iDownRec( &m_prelUL );
            if ( !i )
                break;  // can't scroll as far as requested
            lActual += i;
            }
        }
    else if ( lPixels < 0 ) // scroll up
        {
        while ( lActual > lPixels )
            {
            int i = iUpRec( &m_prelUL );
            if ( !i )
                break;  // can't scroll as far as requested
            lActual -= i;
            }
        }
            
    if ( lActual )
        {
        CRect rect;
        GetClientRect( &rect );
        rect.top = m_iBrowseHeaderHeight;
        if ( labs( lActual ) < (rect.bottom - m_iBrowseHeaderHeight)) // don't bother scrolling if whole window would scroll off
             //09-10-1997 Changed the parameterlist in order to draw the header
            ScrollWindow( 0, (int)-lActual, &rect, &rect );   // perform actual window scroll
        else
            Invalidate( FALSE );
        SetScrollPos( SB_VERT, (int)( m_prelUL.lNum() * fYScrollHt / m_pind->lNumRecEls() ) );
        }
    return lActual;
}
//------------------------------------------------------
void CShwView::BrowseOnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll) // handle scroll bar manipulation
{
    // calc new y position
    int y = GetScrollPos(SB_VERT);
    int yOrig = y;

    CRect rect;
    switch (HIBYTE(nScrollCode))
    {
    case SB_TOP:
        y = 0;
        break;
    case SB_BOTTOM:
        y = INT_MAX;
        break;
    case SB_LINEUP:
        lBrowseVScroll( -1 );
        break;
    case SB_LINEDOWN:
        lBrowseVScroll( 1 );
        break;
    case SB_PAGEUP:
        GetClientRect( &rect );
        lBrowseVScroll( min( 0, -rect.bottom + m_brflst.iLineHt() * 2 ) );
        break;
    case SB_PAGEDOWN:
        GetClientRect( &rect );
        lBrowseVScroll( max( 0, rect.bottom - m_brflst.iLineHt() * 2 ) );
        break;
    case SB_THUMBTRACK:
        long lPos = (long)( (float)nPos * m_pind->lNumRecEls() * m_brflst.iLineHt() / fYScrollHt );
        int yMin, yMax;
        GetScrollRange(SB_VERT, &yMin, &yMax); // check for bottom of scroll bar

        if ( labs( lPos - m_prelUL.lNum() * m_brflst.iLineHt() ) >= m_brflst.iLineHt() || ( (int)nPos >= yMax ) )
			{
            lBrowseVScroll( lPos - m_prelUL.lNum() * m_brflst.iLineHt() );
			}
        break;
    }
	#ifdef _MAC
	UpdateWindow();	// Feb2000 Mac seems to require forced repaint of uncovered area
					//  when the mouse button is not yet released
	#endif

}
//------------------------------------------------------
void CShwView::DrawBrowse( CDC* pDC ) // Draw the record set
{
    CRect rectClip;
    pDC->GetClipBox( &rectClip ); // get coords for area that needs repainted
    CRect rectClient;
    GetClientRect( &rectClient );

    if ( !m_prelCur ) // If empty record set, draw only the header
        {
        DrawBrowseHeader(pDC);  //09-11-1997
        return;
        }
    
    CNumberedRecElPtr prel( NULL, 0 );
    int iLine;
    if ( m_prelCur > m_prelUL && 
            (long)rectClip.top >= 
            (( m_prelCur - m_prelUL ) * m_brflst.iLineHt() + m_iBrowseHeaderHeight) )  //09-11-1997 added m_iBrowseHeaderHeight
        {
        prel = m_prelCur; // can start draw at current record instead of top of view
        iLine = (int)( m_prelCur - m_prelUL );
        }
    else // otherwise just start drawing from top of view
        {
        prel = m_prelUL;
        iLine = 0;
        DrawBrowseHeader(pDC);  //09-11-1997
        }
       
    for ( ; prel; prel = m_pind->pnrlNext( prel ) )
        {
        DrawBrowseLine( pDC, iLine, prel, prel == m_prelCur); // draw another line
        iLine++; // goto next line
        if ( iLine * m_brflst.iLineHt() + m_iBrowseHeaderHeight >= rectClip.bottom ) // bail out if now out of update region
            break;
        }

    //09-10-1997 Added m_iBrowseHeaderHeight
    if ( !prel && (iLine * m_brflst.iLineHt() + m_iBrowseHeaderHeight) < rectClip.bottom ) // may need to blank out bottom of view
        {
        CBrush brush( ::GetSysColor( COLOR_WINDOW ) );
        CRect rect( eDividerPad, iLine * m_brflst.iLineHt() + m_iBrowseHeaderHeight,
            rectClip.right, rectClip.bottom );
        pDC->FillRect( &rect, &brush ); // blank out bottom of view
        }

    DrawBrowseDividers( pDC, &rectClip ); // draw field dividers
}
//------------------------------------------------------
void CShwView::DrawBrowseDividers( CDC* pDC, CRect* prectClip ) // Draw the vertical field dividers
{
    CBrush brushPad( ::GetSysColor( COLOR_WINDOW ) );
    CBrush brushBar( ::GetSysColor( COLOR_WINDOWTEXT ) );
    CRect rect = *prectClip;
    rect.left = 0;
    rect.right = eDividerPad;
    pDC->FillRect( &rect, &brushPad ); // Draw blank pad on left side of window
    rect.left = eDividerPad;
    for ( CBrowseField* pbrf = m_brflst.pbrfFirst(); pbrf; pbrf = m_brflst.pbrfNext( pbrf ) )
        {
        rect.left += pbrf->iWidth();
        rect.right = rect.left + eDividerPad;
        pDC->FillRect( &rect, &brushPad ); // draw pad on left side of vertical bar
        rect.left += eDividerPad;
        rect.right = rect.left + eDividerBar;
        pDC->FillRect( &rect, &brushBar ); // draw vertical bar
        rect.left += eDividerBar;
        rect.right = rect.left + eDividerPad;
        pDC->FillRect( &rect, &brushPad ); // draw pad on right side of vertical bar
        rect.left += eDividerPad; // add width of divider pad to get to next field
        }
}
//------------------------------------------------------
void CShwView::DrawBrowseLine( CDC* pDC, int iLine, CNumberedRecElPtr prel, BOOL bHilite, BOOL bDrawDividers ) // Draw a single browse line
{
    ASSERT( m_prelCur );

    int iY = iLine * m_brflst.iLineHt() + m_iBrowseHeaderHeight;
    CRect rect( eDividerPad, iY, 0, iY + m_brflst.iLineHt() );

    CBrowseField* pbrf = m_brflst.pbrfFirst();
    for ( ; pbrf; pbrf = m_brflst.pbrfNext( pbrf ) )
        {
        const CField* pfld = prel->pfldFirstInSubRecord(pbrf->pmkr());
        rect.right = rect.left + pbrf->iWidth(); // clip at field width
        // 1997-02-12 MRP: Move the color change from outside the loop
        // to account for a change to TextOutClip (in shwvdis.cpp)
        // which is used to draw individual fields.
        // It caused "empty" and "no field" strings to be drawn
        // in the colors of the data field to their left.
        pbrf->pmkr()->SetTextAndBkColorInDC( pDC, bHilite );  // 1998-03-06 MRP
        if ( !pfld )  // If field doesn't exist in this record
            {
            pDC->SelectObject( ::GetStockObject( SYSTEM_FONT ) ); // use system font
			Str8 s = "A"; // 1.4qwn
			CString sw = swUTF16( s ); // 1.4qwn
            int iDiff = m_brflst.iLineHt() - pDC->GetTextExtent(sw,1).cy; // 1.4qwn
            int iY = rect.top + ( iDiff / 2 );
            
			CString swText = swUTF16( m_brflst.GetNoFieldEntry() ); // 1.4qta
            pDC->ExtTextOut( rect.left, iY, ETO_CLIPPED | ETO_OPAQUE, &rect, swText, swText.GetLength(), NULL ); // 1.4qta
            }
        else if ( pfld->IsEmpty() )
            {
            pDC->SelectObject( ::GetStockObject( SYSTEM_FONT ) ); // use system font
			Str8 s = "A"; // 1.4qwn
			CString sw = swUTF16( s ); // 1.4qwn
            int iDiff = m_brflst.iLineHt() - pDC->GetTextExtent(sw,1).cy;
            int iY = rect.top + ( iDiff / 2 );
                
			CString swText = swUTF16( m_brflst.GetFieldEmptyEntry() ); // 1.4qta
            pDC->ExtTextOut( rect.left, iY, ETO_CLIPPED | ETO_OPAQUE, &rect, swText, swText.GetLength(), NULL ); // 1.4qta
            }
        else
            {
            int iDiff = m_brflst.iLineHt() - pfld->pmkr()->iLineHeight(); // this line shorter than line height?
            int iY = rect.top + ( iDiff / 2 ); // align text in center of line
            CFont* pfnt = (CFont*)pfld->pmkr()->pfnt(); // had to cast away const attribute
            pDC->SelectObject( pfnt );
//              pDC->ExtTextOut( rect.left, iY, ETO_CLIPPED | ETO_OPAQUE, &rect, *pfld, pfld->GetLength(), NULL );
            // ExtTextOut_AsOneLine( pDC, rect.left, iY,
            //     ETO_CLIPPED | ETO_OPAQUE, &rect, *pfld, pfld->GetLength(), NULL );
            DrawBrowseField( pDC, rect, iY, pfld, bHilite, pbrf->bIsRightJustified() );
            }
        rect.left = rect.right + eBrowseDividerWidth; // step over divider to next field
        }
    if ( bDrawDividers )    
        {
        rect.left = 0;
        DrawBrowseDividers( pDC, &rect ); // draw vertical dividers
        }
}
//------------------------------------------------------
void CShwView::DrawBrowseField( CDC* pDC, const CRect& rect, int y,
        const CField* pfld, BOOL bHilite, BOOL bDrawEnd )
{
    ASSERT( pDC );
    pDC->SetTextAlign( TA_UPDATECP );
    ASSERT( pfld );
    CRecPos rps( 0, (CField*)pfld );        
    CRecPos rpsBegin;
    CRecPos rpsEnd;
        
    if ( bDrawEnd )   
        //In the browse view should be the right border the fix one (=Right justified)
        {
        pDC->MoveTo(rect.right,y);
        
          // Now we have to destinguish between Left to Right Script or not
        if (!pfld->pmkr()->plng()->bRightToLeft())
            {
            //Its a Left to Right Script
            rps.SetPos(pfld->GetLength());       // Go to the end of field
            while ( rps.bMoveLeftPastViewSubstring( &rpsBegin, &rpsEnd ) )
                {
                BOOL bDrawSpace = (*(rps.psz()) == '\n');
                if (!bDrawStringEnd( pDC, rect, rpsBegin, rpsEnd, 
                                     bDrawSpace, bHilite) )
                    break;  //There is no more place for characters
                } //while 
            } // if (Left to Right)
        else
            {
            // Its a Rigth to Left Script
            while ( rps.bMoveRightPastViewSubstring( &rpsBegin, &rpsEnd ) )
                {
                BOOL bDrawSpace = (*(rpsEnd.psz()) == '\n');
                if (!bDrawStringEnd( pDC, rect, rpsBegin, rpsEnd, 
                                     bDrawSpace, bHilite) )
                    break;  //There is no more place for characters
                } //while 
            } // else - if (Right to Left)
        
        //Look for free space, that has to bo filled        
        if ( pDC->GetCurrentPosition().x > rect.left) 
            //There is free space
            {
            CRect rectFill = rect;
            rectFill.right = pDC->GetCurrentPosition().x;
            CBrush brush( ::GetSysColor( bHilite ? COLOR_HIGHLIGHT : COLOR_WINDOW ) );
            pDC->FillRect( &rectFill, &brush ); 
            } // if  
        
        } // if - Draw end
    else
        //In the browse view should be the left border, the fix one
        {
        pDC->MoveTo( rect.left, y );

          // Now we have to destinguish between Left to Right Script or not
        if (!pfld->pmkr()->plng()->bRightToLeft())
            {
            // Its a Left to Right Script
            while ( rps.bMoveRightPastViewSubstring( &rpsBegin, &rpsEnd ) )
                {
                BOOL bDrawSpace = (*(rpsEnd.psz()) == '\n');
                DrawSubstring( pDC, rect, rpsBegin, rpsEnd, bDrawSpace, bHilite );
                } //while
            } // if (Left to Right)
        else
            {
            rps.SetPos(pfld->GetLength());       // Go to the end of field
            // Its a Right to Left Script
            while ( rps.bMoveLeftPastViewSubstring( &rpsBegin, &rpsEnd ) )
                {
                BOOL bDrawSpace = (*(rps.psz()) == '\n');
                DrawSubstring( pDC, rect, rpsBegin, rpsEnd, bDrawSpace, bHilite );
                } //while
            } // else (Right to Left)    
 
          // Look for free space
        int x = pDC->GetCurrentPosition().x;
        if ( rect.left <= x && x < rect.right )
            {
            CRect rectFill = rect;
            rectFill.left = x;
            CBrush brush( ::GetSysColor( bHilite ? COLOR_HIGHLIGHT : COLOR_WINDOW ) );
            pDC->FillRect( &rectFill, &brush );
            } // if        
        } // else - Draw begin
    pDC->SetTextAlign( TA_NOUPDATECP );
} //DrawBrowseField

static CNumberedRecElPtr prelSave;
//------------------------------------------------------
void CShwView::RememberCurLine() // Remember current line position, for redrawing after current line changes
{
    prelSave = m_prelCur;
}
//------------------------------------------------------
void CShwView::DrawPrevAndCurLines() // redraw line moved from and line moved to, to change hiliting
{
    ASSERT( prelSave ); // must have been set by RememberCurLine()
    CRect rect;
    GetClientRect( &rect );
    CDC* pDC = GetDC();
    
      //09-12-1997 Added m_iBrowseHeadHeight
    if ( prelSave >= m_prelUL &&
            ( prelSave - m_prelUL ) * m_brflst.iLineHt() + m_iBrowseHeaderHeight
              < rect.bottom ) // don't draw if not visible
        DrawBrowseLine( pDC, (int)( prelSave - m_prelUL ), prelSave, FALSE, TRUE ); // remove hiliting on line we moved from
    BrowseScrollToCur();
    DrawBrowseLine( pDC, (int)( m_prelCur - m_prelUL ), m_prelCur, TRUE, TRUE ); // hilite new current line
    
      //09-12-1997 Added m_iBrowseHeadHeight
    rect.top = (int)( m_prelCur - m_prelUL ) * m_brflst.iLineHt() + m_iBrowseHeaderHeight;
    rect.bottom = rect.top + m_brflst.iLineHt();
    ValidateRect( &rect ); // avoid unnecessary WM_PAINT message
    ReleaseDC( pDC );
    prelSave.Set(NULL, 0L);
}
//------------------------------------------------------
BOOL CShwView::bBrowseMoveDown() // Move down one line in browse
{
    return( iDownRec( &m_prelCur ) );
}
//------------------------------------------------------
BOOL CShwView::bBrowseMoveUp() // Move up one line in browse
{
    return( iUpRec( &m_prelCur ) ); // try to move up a line
}
//------------------------------------------------------
void  CShwView::BrowseMoveCtrlHome() // Ctrl Home, beginning of record set, core primitive
{
    m_prelUL = m_prelCur = m_pind->pnrlFirst();
}
//------------------------------------------------------
BOOL  CShwView::bBrowseUp() // Up arrow
{
      //09-16-1997 Do nothing if the window shows only the header
    CRect rect;
    GetClientRect(&rect);
    if ( rect.bottom <= m_iBrowseHeaderHeight )
        return FALSE;    
    
    RememberCurLine();
    int bResult = bBrowseMoveUp();
    if ( bResult )
        DrawPrevAndCurLines(); // shift hiliting to line moved to
    else
        MessageBeep(0);  // 1996-10-10 MRP
    return bResult;
}
//------------------------------------------------------
BOOL  CShwView::bBrowseDown() // Down arrow
{
      //09-16-1997 Do nothing if the window shows only the header
    CRect rect;
    GetClientRect(&rect);
    if ( rect.bottom <= m_iBrowseHeaderHeight )
        return FALSE;    
        
    RememberCurLine();
    int bResult = bBrowseMoveDown();
    if ( bResult )
        DrawPrevAndCurLines(); // shift hiliting to line moved to
    else
        MessageBeep(0);  // 1996-10-10 MRP
    return bResult;
}
//------------------------------------------------------
BOOL  CShwView::bBrowsePgUp() // page up
{
    CRect rect;
    GetClientRect( &rect );
    int lines = max( 1, rect.bottom / m_brflst.iLineHt() ); // number of lines on one viewable page

    if ( !bBrowseMoveUp() ) // If can't move at all, return false
        {
        MessageBeep(0);  // 1996-10-10 MRP
        return FALSE;
        }
    for ( int i=1; i<lines && bBrowseMoveUp(); i++ )
        ;
    BrowseScrollToCur();
    Invalidate(FALSE);   
    return TRUE;        
}
//------------------------------------------------------
BOOL  CShwView::bBrowsePgDn() // page down
{
    CRect rect;
    GetClientRect( &rect );
    int lines = max( 1, rect.bottom / m_brflst.iLineHt() ); // number of lines on one viewable page
    
    if ( !bBrowseMoveDown() ) // If can't move at all, return false
        {
        MessageBeep(0);  // 1996-10-10 MRP
        return FALSE;
        }
    for ( int i=1; i<lines && bBrowseMoveDown(); i++ )
        ;   
    BrowseScrollToCur();
    Invalidate(FALSE);   
    return TRUE;        
}
//------------------------------------------------------
void  CShwView::BrowseCtrlHome() // Ctrl Home, beginning of record
{
    BrowseMoveCtrlHome();
    SetScrollPos( SB_VERT, 0 ); // force scroll bar since we're bypassing scroll routine
    Invalidate(FALSE);
}
//------------------------------------------------------
void  CShwView::BrowseCtrlEnd() // Ctrl End, end of record set
{
    while ( bBrowseMoveDown() )
        ;
    ASSERT( m_pind->lNumRecEls() == m_prelCur.lNum() + 1 );
    BrowseScrollToCur();
    Invalidate(FALSE);
}
//------------------------------------------------------
void CShwView::BrowseMouseLeftClick( UINT nFlags, CPoint pnt ) // Mouse left button click
{
    RememberCurLine();
    m_prelCur = m_prelUL; // start at first visible line
    ASSERT( pnt.y >= 0 );

      //09-11-1997 Added m_iBrowseHeaderHeight
    int iLines = (pnt.y - m_iBrowseHeaderHeight) / m_brflst.iLineHt(); // calc how many lines we need to move
    for ( int i=0; i < iLines && bBrowseMoveDown(); i++ )
        ;
    DrawPrevAndCurLines(); // shift hiliting to line moved to
    
    ASSERT( !bTracking( eAnything ) );
}
//------------------------------------------------------
void CShwView::BrowseEditMouseDblClk( UINT nFlags, CPoint pnt ) // Mouse left button double click
{
    ASSERT( !bTracking( eAnything ) );
}
//------------------------------------------------------
void CShwView::BrowseOnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
    if ( bTracking( eAnything ) )   // don't use keys if mouse button down
        return;

    if ( ::GetKeyState( VK_CONTROL ) >= 0 ) // If control key is up, do normal movements
        {
        switch ( nChar )
            {                       
            case VK_UP: bBrowseUp(); break;
            case VK_DOWN: bBrowseDown(); break;
            case VK_PRIOR: bBrowsePgUp(); break;
            case VK_NEXT: bBrowsePgDn(); break;   
            }
        }
    else // Else, control key is down, do control movements
        {   
        switch ( nChar )
            {                       
            case VK_HOME: BrowseCtrlHome(); break;
            case VK_END: BrowseCtrlEnd(); break;
            }
        }   

    // RNE 1995-11-09  Need to reflect possible movement to a new record
    // element on the status bar.
    SetStatusRecordAndPrimaryKey(m_prelCur);
	if ( !( nChar == 16 || nChar == 17 ) ) // 6.0zm Don't do jump on control or shift key
		ParallelJump(); // 6.0zd Do parallel jump on browse movements

}
//------------------------------------------------------
void CShwView::BrowseOnLButtonDown(UINT nFlags, CPoint pnt)
{
    if ( !m_prelCur ) // Handle empty view from filter
        return;

    if ( bPtInDivider( pnt ) )
        {
        StartTracking( eDivider );
        StartMovingDivider( pnt );
        }
    else
        {
        if (pnt.y <= m_iBrowseHeaderHeight) // 1.1mb On click in header, change sorting
			{
			CMarker* pmkrNew = pmkrBrowsePtInMarker(pnt);
			if (!pmkrNew)
				return;
			BOOL bSortFromEnd = FALSE;
#ifdef MacWine // 1.5.1ke Use shift click for browse from end only on Mac
			if ( (::GetKeyState( VK_SHIFT ) < 0 ) ) // 1.2bu On shift click on column header, sort from end // 1.5.1ff Change browse sort from end to shift click, works on Mac
#else
			if ( (::GetKeyState( VK_CONTROL ) < 0 ) ) // 1.2bu On ctrl click on column header, sort from end
#endif
				bSortFromEnd = TRUE;
			if ( pmkrNew == m_pind->pmkrPriKey() ) // If same as current key, don't do anything
				{
				if ( m_pind->bSortPriKeyFromEnd() == bSortFromEnd ) // 1.2bu If sorted from same end as previous, don't do anything
					return;
				}
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
			CSortOrder* psrtPri = pmkrNew->plng()->psrtDefault();

			mrflstNewSecKeys.AddMarkerRef( pmkrPriKey ); // Make old primary key secondary
			pmkrPriKey = pmkrNew; // Make new key primary
			pindset->GetIndex(pmkrPriKey, psrtPri, // Make new index
				&mrflstNewSecKeys, (Length)iSortKeyLen, bSortFromEnd,
				pfil, ppind, ppindUnfiltered);
			SetIndex( pind, pindUnfiltered ); // Change index to new
#ifdef HereIsHowDoneInDialogBox
			m_pindset->GetIndex(m_pmkrPriKey, m_psrtPri,
				&m_mrflstUsersChoice, (Length)m_iSortKeyLen, m_bSortFromEnd,
				m_pfil, m_ppind, m_ppindUnfiltered);
#endif
			CBrowseFieldList brflstTemp; // 1.4kp Fix bug of losing right justify of conc prec on sort by other field
			brflstTemp = m_brflst;   //Store the old one
			if (!m_brflst.bIsEmpty())
				{
				BrowseJustificationDefault();  // Check the PriKeyFields in the Browselist
				RememberUnusualJustificationExceptPriKeyField(&brflstTemp);
				}
//			BrowseJustificationDefault(); // 1.2bt Fix bug of click on browse header leaving previous sort column rt justified
			return;
			}
        
        BrowseMouseLeftClick( nFlags, pnt );
        BrowseScrollToCur(); // make current rec visible
        SetStatusRecordAndPrimaryKey(m_prelCur); // RNE Reflects keys on status bar.
		ParallelJump(); // 6.0zd Do parallel jump on browse click
        }
}
//------------------------------------------------------
//09-12-1997
void CShwView::BrowseOnRButtonDown(UINT nFlags, CPoint pnt) 
{
    if (pnt.y > m_iBrowseHeaderHeight) // This click is in the data
        {
		BrowseOnLButtonDown(nFlags, pnt); // 5.97v Fix bug of jumping to old select instead of cur
		BrowseOnLButtonDblClk(nFlags, pnt); // 5.97p Use double click to set cursor to word
		Str8 sWord;
        sWord = sGetCurText(); // Get word at caret position
		SwitchToBrowseView(); // Restore browse view
#ifdef MacWine // 1.5.1ke Use shift click for jump insert only on Mac
        EditJumpTo( (::GetKeyState( VK_SHIFT ) & 0x8000) != 0, // 1.5.1ga Change Ctrl+click to Shift+click for jump insert 
                    FALSE, sWord, m_rpsCur.pfld->pmkr() ); // do jump (jump insert if control pressed)
#else
        EditJumpTo( (::GetKeyState( VK_CONTROL ) & 0x8000) != 0, // 1.5.1ga Change Ctrl+click to Shift+click for jump insert 
                    FALSE, sWord, m_rpsCur.pfld->pmkr() ); // do jump (jump insert if control pressed)
#endif
		return;
        }
    else // This click is in the header row
        {
        CMarker* pmkr = pmkrBrowsePtInMarker(pnt);
        if (!pmkr)
            return;
#ifdef MacWine // 1.5.1ke Use shift click to move to browse only on Mac
		if ( (::GetKeyState( VK_SHIFT ) < 0 ) ) // 1.2br On ctrl right click, move to end of browse list // 1.5.1fg Change add marker to browse to shift click, works on Mac
#else
		if ( (::GetKeyState( VK_CONTROL ) < 0 ) ) // 1.2br On ctrl right click, move to end of browse list // 1.5.1fg Change add marker to browse to shift click, works on Mac
#endif
			{
			CBrowseFieldList* pbrflstT = pbrflst();
			if ( pbrflstT->lGetCount() <= 1 ) // 1.2bw Fix bug of crash on ctrl rt click on last field in browse
				return;
			CBrowseField* pbrf = pbrflst()->pbrfFirst();
			for ( ; pbrf; pbrf = pbrflst()->pbrfNext( pbrf ) )
				{
				if ( pbrf->pmkr() == pmkr ) // 1.2br Find marker in browse list
					break;
				}
			if ( pbrf )
				{
				BOOL bLast = pbrflstT->bIsLast( pbrf ); // 1.2br If last column, remove, else move to last column
				pbrflstT->Delete( pbrf );
				if ( !bLast ) 
					pbrflstT->Add( pmkr );
				}
			BrowseJustificationDefault(); // 1.2bq Keep right justification correct
			InitBrowse(); // 1.2bq recalculate scroll bars, etc.			
			ResizeBrowse(); // 1.2bx Adjust browse width if needed
			ResetBrowseFieldWidths(); // 1.2br Adjust browse widths to show new marker
			}
		else // Else, regular right click, show the marker properties dialog box
			{
			if ( !Shw_papp()->pProject()->bLockProject() ) // 1.2bs Don't show marker properties if project is locked
				{
				CMarkerPropertiesSheet dlg( pmkr );
				dlg.DoModal(); // allow user to modify this marker's properties
				}
			}
        Invalidate(FALSE);
        }
    return;
}

//------------------------------------------------------
void CShwView::BrowseOnLButtonDblClk(UINT nFlags, CPoint pnt)
{
      //09-11-1997 Handle Click in the Header
    if (pnt.y <= m_iBrowseHeaderHeight)
        return;
    
    SwitchToRecordView();
    
    CBrowseField* pbrf = m_brflst.pbrfFirst();
    ASSERT( pbrf );
	int iX = eDividerPad;
    for ( ; pbrf && ( iX + pbrf->iWidth() + eDividerPad < pnt.x );
            pbrf = m_brflst.pbrfNext( pbrf ) )
        iX += pbrf->iWidth() + eBrowseDividerWidth; // advance to next column
    ASSERT( pbrf );
    
    CField* pfld = m_prelCur->pfldPriKey();
    if (!pfld || pfld->pmkr() != pbrf->pmkr()) // if not in primary key
        // 03-14-2000 TLB : Set to correct location in record when user clicks on column that is not primary sort field
        pfld = (CField*) m_prelCur->pfldFirstInSubRecord(pbrf->pmkr()); // find first field containing this marker
    if ( !pfld )
        return;
    EditSetCaret( pfld, 0 ); // set caret at beginning of proper field
    
    HideCaret();
    m_pntCaretLoc.x = m_iFldStartCol + iCharsFit( m_rpsCur, pnt.x - iX + 2, TRUE ); // 5.97r Fix bug of wrong in interlinear // set m_rpsCur to closest char
    SetCaretPosAdj(); // Move caret
    ShowCaret();
    Shw_pProject()->pkmnManager()->ActivateKeyboard(m_rpsCur.pmkr()); // 1.4qzfb Fix bug of not changing keyboard when coming out of browse
}
//------------------------------------------------------
void CShwView::SwitchToRecordView()
{
    ASSERT( m_bBrowsing );

    m_iCaretHt = 0; // force creation of caret
    // TLB 1/7/2000 : The following changes attempt to solve several problems whereby a
    // browse views' index is not updated when it switches to record view, even though
    // edits have ocurred in another view on the same DB.
    // 04/05/2000 The following lines overstate the case when we would need to rebuild the index
    // since we only need to do it if another view of the same DB has its
    // m_bModifiedSinceValidated flag set. This can cause an ASSERT when inserting a record.
    // Therefore, the correct solution is to call bValidateViews and exclude this view. If
    // any pending changes affect the current prel of this view, it will be notified.

    //m_bModifiedSinceValidated = GetDocument()->IsModified();  // TLB 1/7/2000 !!!TEMPORARY??? Force this to make sure we re-validate
    //if ( !bValidate(FALSE, eSameRecord, &prel, TRUE) )// TLB 1/7/2000
    //    return;

    // 04/05/2000 - TLB
    CMDIChildWnd* pwnd = Shw_pmainframe()->MDIGetActive(); // remember active child window
    if ( pdoc()->bValidateViews(TRUE, this) ) // validate all other views of this doc
        if ( pwnd != Shw_pmainframe()->MDIGetActive() ) // was another view activated because of validation problem?
            Shw_pmainframe()->MDIActivate(pwnd); // re-activate current view

    m_bBrowsing = FALSE; // 2000-06-16 TLB: Moved from above to prevent ASSERT when switching from browse to record
                         //                 view when an edit has occurred in another window of the same DB.
    SetCur( m_prelCur );
    Invalidate(FALSE);
	FixCursorPosForHidden(); // 1.0br Fix cursor pos if hidden fields
}
//------------------------------------------------------
void CShwView::SwitchToBrowseView()
{
    ASSERT( !m_bBrowsing );
    ASSERT( m_prelCur ); // Should be disabled if no current record
        
    // 2000/05/02 TLB 7 MRP - bValidateCur now takes only one argument, which defaults to dirSameRecord
    //CNumberedRecElPtr prel = m_prelCur;
    if ( !bValidateCur(/* eSameRecord, &prel, TRUE */) ) // TLB 4/24/2000 - Let bValidateCur set the current rel
        return;
    // WARNING: prel may be bogus now because of a change in another window. Don't use!
    // 4/24/2000 TLB - Need to do these next three lines first so that when SetCur is called, the browse
    // view will be set up correctly.
    HideCaret();
    DestroyCaret(); // cursor not visible in browse view
    m_bBrowsing = TRUE;
    SetCur(m_prelCur); // TLB 4/24/2000 - Let bValidateCur set the current rel; this call just switches to browse view
}
//------------------------------------------------------
void CShwView::BrowseOnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    switch ( nChar )
        {
        case '\r':
        case '\n':
            SwitchToRecordView();
            break;
        }
}

//---------------------------------------------------------------------------
//09-05-1997
void CShwView::BrowseJustificationDefault()
{
       //Set the conditions for all entries in the list 
    for ( CBrowseField* pbrf = m_brflst.pbrfFirst(); pbrf; pbrf = m_brflst.pbrfNext( pbrf ) )
        {
        pbrf->SetRightJustified(FALSE);
        if ( (!pbrf->pmkr()->plng()->bRightToLeft()) &&
             (pbrf->pmkr() == m_pind->pmkrPriKey()) &&
             (m_pind->bSortPriKeyFromEnd()) )
            pbrf->SetRightJustified(TRUE);
        if (pbrf->pmkr()->plng()->bRightToLeft())
            {
            pbrf->SetRightJustified(TRUE);
            if ( (pbrf->pmkr() == m_pind->pmkrPriKey()) &&
                 (m_pind->bSortPriKeyFromEnd()) )
                pbrf->SetRightJustified(FALSE);
             }
        }
}

//----------------------------------------------------------------------
//09-05-1997
void CShwView::RememberUnusualJustificationExceptPriKeyField(CBrowseFieldList* pbrflstBefore)
{
    ASSERT (pbrflstBefore);
    //Restore unusual conditions
    if ( (pbrflstBefore->lGetCount() == m_brflst.lGetCount()) )  // For safty
        {
        for( CBrowseField *pbrf = m_brflst.pbrfFirst() , 
             *pbrfBefore = pbrflstBefore->pbrfFirst() ;
             pbrf && pbrfBefore;
             pbrf = m_brflst.pbrfNext( pbrf ),
             pbrfBefore = pbrflstBefore->pbrfNext( pbrfBefore ) )
            {
            if ( !pbrfBefore->pmkr()->plng()->bRightToLeft() &&
                 !(pbrfBefore->pmkr() == m_pind->pmkrPriKey()) &&
                 pbrfBefore->bIsRightJustified() )
                 pbrf->SetRightJustified(TRUE);
            if ( pbrfBefore->pmkr()->plng()->bRightToLeft() &&
                 !(pbrfBefore->pmkr() == m_pind->pmkrPriKey()) &&
                 !pbrfBefore->bIsRightJustified() )
                 pbrf->SetRightJustified(FALSE);
            }
        }
}   

//------------------------------------------------------
//09-15-1997
void CShwView::CalculateBrowseHeaderHeight()
{
    CDC tempDC;
    tempDC.CreateCompatibleDC( NULL );
    CFont* pfnt = Shw_pProject()->pfntMkrs();
    tempDC.SelectObject(pfnt);
    TEXTMETRIC tm;
    tempDC.GetTextMetrics( &tm);
    m_iBrowseHeaderHeight = tm.tmHeight + eDividerPad;
}

//------------------------------------------------------
//09-11-1997
void CShwView::DrawBrowseHeader(CDC* pDC)
{
      // Remember the conditions
    UINT oldTextAlign = pDC->GetTextAlign();
   
      //Use this font and color
    CFont* pfnt = Shw_pProject()->pfntMkrs();
    pDC->SelectObject( pfnt );
    pDC->SetTextAlign( TA_UPDATECP );
    pDC->SetTextColor( ::GetSysColor( COLOR_WINDOWTEXT ) );
    pDC->SetBkColor( ::GetSysColor( COLOR_WINDOW ) );

      //Get marker hight
    TEXTMETRIC tm;
    pDC->GetTextMetrics( &tm );
    int iMkrHt = tm.tmHeight; // get height of marker font 
    m_iBrowseHeaderHeight = iMkrHt + eDividerPad;

       //We will paint in this rect
    CRect rect (eDividerPad, 0, 0 , m_iBrowseHeaderHeight); //Start configuration 
    
    for (CBrowseField* pbrf = m_brflst.pbrfFirst(); pbrf; pbrf = m_brflst.pbrfNext(pbrf) )
        {
        pDC->MoveTo(rect.left, 0);
        rect.right = rect.left + pbrf->iWidth();
        pbrf->pmkr()->DrawMarker(pDC, rect, FALSE, m_bViewMarkers, m_bViewFieldNames);
        rect.left = rect.right + eBrowseDividerWidth;  //step over divider to next field
        }
   
      //Draw the Line
    CBrush brushLine(::GetSysColor( COLOR_WINDOWTEXT ));
    GetClientRect( &rect);
    rect.top = iMkrHt + 1;  //Prepare rect for drawing
    rect.bottom = m_iBrowseHeaderHeight - 1;
    pDC->FillRect(&rect, &brushLine);  
    
      // Restore the old Condition
    pDC->SetTextAlign( oldTextAlign );
}

//---------------------------------------------------
//09-12-1997
CMarker* CShwView::pmkrBrowsePtInMarker(CPoint point) 
{
    CBrowseField* pbrf = m_brflst.pbrfFirst();
    ASSERT( pbrf );
    int iX = eDividerPad + pbrf->iWidth(); // jump past first field and padding
  
    if (point.x < iX) 
        return pbrf->pmkr();
     
    for ( pbrf = m_brflst.pbrfNext( pbrf ); pbrf; pbrf = m_brflst.pbrfNext( pbrf ) )
        {
        iX += pbrf->iWidth() + eBrowseDividerWidth; // goto next field
        if ( point.x < iX  )
            {
            return pbrf->pmkr(); // pnt lies within vertical divider
            }        
        }
    
    return NULL;
} 
  


    







