// shwvscrl.cpp : implementation of the CShwScrollView class
//

#include "stdafx.h"
#include "toolbox.h"
#include <limits.h>
#include "shwvscrl.h"
#include "mainfrm.h"
#include "shwdtplt.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CShwScrollView

IMPLEMENT_DYNAMIC(CShwScrollView, CView)

BEGIN_MESSAGE_MAP(CShwScrollView, CView)
	//{{AFX_MSG_MAP(CShwScrollView)
	ON_WM_SIZE()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	//}}AFX_MSG_MAP
#ifndef _MAC
	ON_WM_MOUSEWHEEL()
#endif
END_MESSAGE_MAP()

// Special mapping modes just for CScrollView implementation
#define MM_NONE             0
        // standard GDI mapping modes are > 0

/////////////////////////////////////////////////////////////////////////////
// CShwScrollView construction/destruction

CShwScrollView::CShwScrollView()
{
	// Init everything to zero
	// no longer needed:
    //  AFX_ZERO_INIT_OBJECT(CView);

	m_nMapMode = MM_NONE;
}

CShwScrollView::~CShwScrollView()
{
}


/////////////////////////////////////////////////////////////////////////////
// CShwScrollView painting

void CShwScrollView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
	ASSERT_VALID(pDC);
	ASSERT(m_nMapMode > 0);
	pDC->SetMapMode(m_nMapMode);

	CPoint ptVpOrg(0, 0);       // assume no shift for printing
	if (!pDC->IsPrinting())
	{
		ASSERT(pDC->GetWindowOrg() == CPoint(0,0));

		// by default shift viewport origin in negative direction of scroll
		ptVpOrg = -GetDeviceScrollPosition();
	}
	ptVpOrg.y = 0; // BJY
	pDC->SetViewportOrg(ptVpOrg);

	CView::OnPrepareDC(pDC, pInfo);     // For default Printing behavior
}

/////////////////////////////////////////////////////////////////////////////
// Set mode and scaling/scrolling sizes

const SIZE CShwScrollView::sizeDefault = {0,0};

void CShwScrollView::SetScrollSizes(int nMapMode, SIZE sizeTotal,
	const SIZE& sizePage, const SIZE& sizeLine)
{
    // 2000-09-07 TLB & MRP: 32767 is the upper limit for x- and y-coordinates.
    if ( sizeTotal.cx > SHRT_MAX ) 
        sizeTotal.cx = SHRT_MAX;
    if ( sizeTotal.cx < 0 )  // Check for integer overflow (most likely in 16-bit Windows)
        sizeTotal.cx = SHRT_MAX;

    ASSERT(sizeTotal.cx >= 0);
    ASSERT(sizeTotal.cy >= 0);
	ASSERT(nMapMode > 0);
	ASSERT(nMapMode != MM_ISOTROPIC && nMapMode != MM_ANISOTROPIC);

	int nOldMapMode = m_nMapMode;
	m_nMapMode = nMapMode;
	m_totalLog = sizeTotal;

	//BLOCK: convert logical coordinate space to device coordinates
	{
		CWindowDC dc(NULL);
		ASSERT(m_nMapMode > 0);
		dc.SetMapMode(m_nMapMode);

		// total size
		m_totalDev = m_totalLog;
		dc.LPtoDP((LPPOINT)&m_totalDev);
		m_pageDev = sizePage;
		dc.LPtoDP((LPPOINT)&m_pageDev);
		m_lineDev = sizeLine;
		dc.LPtoDP((LPPOINT)&m_lineDev);
		if (m_totalDev.cy < 0)
			m_totalDev.cy = -m_totalDev.cy;
		if (m_pageDev.cy < 0)
			m_pageDev.cy = -m_pageDev.cy;
		if (m_lineDev.cy < 0)
			m_lineDev.cy = -m_lineDev.cy;
	} // release DC here

    // 2000-09-07 TLB: Assertion causes infinite loop because we're drawing. This should have been fixed above.
	ASSERT(m_totalDev.cx >= 0);
    ASSERT(m_totalDev.cy >= 0);

	// now adjust device specific sizes
    if (m_pageDev.cx == 0)
		m_pageDev.cx = m_totalDev.cx / 10;
	if (m_pageDev.cy == 0)
		m_pageDev.cy = m_totalDev.cy / 10;
	if (m_lineDev.cx == 0)
		m_lineDev.cx = m_pageDev.cx / 10;
	if (m_lineDev.cy == 0)
		m_lineDev.cy = m_pageDev.cy / 10;

	if (m_hWnd != NULL)
	{
		// window has been created, invalidate now
		UpdateBars();
		if (nOldMapMode != m_nMapMode)
			Invalidate(FALSE);
	}
}

/////////////////////////////////////////////////////////////////////////////
// Getting information

CPoint CShwScrollView::GetScrollPosition() const   // logical coordinates
{
	CPoint pt = GetDeviceScrollPosition();

	if (m_nMapMode != MM_TEXT)
	{
		ASSERT(m_nMapMode > 0); // must be set
		CWindowDC dc(NULL);
		dc.SetMapMode(m_nMapMode);
		dc.DPtoLP((LPPOINT)&pt);
	}
	return pt;
}

void CShwScrollView::ScrollToPosition(POINT pt)    // logical coordinates
{
	ASSERT(m_nMapMode > 0);     // not allowed for shrink to fit
	if (m_nMapMode != MM_TEXT)
	{
		CWindowDC dc(NULL);
		dc.SetMapMode(m_nMapMode);
		dc.LPtoDP((LPPOINT)&pt);
	}

	// now in device coordinates - limit if out of range
	int xMin, xMax; //, yMin, yMax;
	GetScrollRange(SB_HORZ, &xMin, &xMax);
//	GetScrollRange(SB_VERT, &yMin, &yMax);
//	ASSERT(xMin == 0 && yMin == 0);
	if (pt.x < 0)
		pt.x = 0;
	else if (pt.x > xMax)
		pt.x = xMax;
//	if (pt.y < 0)
//		pt.y = 0;
//	else if (pt.y > yMax)
//		pt.y = yMax;

	ScrollToDevicePosition(pt);
}

CPoint CShwScrollView::GetDeviceScrollPosition() const
{
	CPoint pt(GetScrollPos(SB_HORZ), GetScrollPos(SB_VERT));
	ASSERT(pt.x >= 0 && pt.y >= 0);

	return pt;
}

void CShwScrollView::GetDeviceScrollSizes(int& nMapMode, SIZE& sizeTotal,
			SIZE& sizePage, SIZE& sizeLine) const
{
	nMapMode = m_nMapMode;
	sizeTotal = m_totalDev;
	sizePage = m_pageDev;
	sizeLine = m_lineDev;
}

void CShwScrollView::ScrollToDevicePosition(POINT ptDev)
{
	ASSERT(ptDev.x >= 0);
	ASSERT(ptDev.y >= 0);

	// Note: ScrollToDevicePosition can and is used to scroll out-of-range
	//  areas as far as CShwScrollView is concerned -- specifically in
	//  the print-preview code.  Since OnScrollBy makes sure the range is
	//  valid, ScrollToDevicePosition does not vector through OnScrollBy.

	int xOrig = SetScrollPos(SB_HORZ, ptDev.x);
//	int yOrig = SetScrollPos(SB_VERT, ptDev.y);
	ScrollWindow(xOrig - ptDev.x, 0/*yOrig - ptDev.y*/);
}

/////////////////////////////////////////////////////////////////////////////
// Other helpers

void CShwScrollView::FillOutsideRect(CDC* pDC, CBrush* pBrush)
{
	ASSERT_VALID(pDC);
	ASSERT_VALID(pBrush);
	// Fill Rect outside the image
	CRect rect;
	GetClientRect(rect);
	ASSERT(rect.left == 0 && rect.top == 0);
	rect.left = m_totalDev.cx;
	if (!rect.IsRectEmpty())
		pDC->FillRect(rect, pBrush);    // vertical strip along the side
	rect.left = 0;
	rect.right = m_totalDev.cx;
	rect.top = m_totalDev.cy;
	if (!rect.IsRectEmpty())
		pDC->FillRect(rect, pBrush);    // horizontal strip along the bottom
}

/////////////////////////////////////////////////////////////////////////////

void CShwScrollView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
		// UpdateBars() handles locking out recursion
	UpdateBars(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// Tie to scrollbars and CWnd behaviour

void CShwScrollView::GetScrollBarSizes(CSize& sizeSb)
{
	sizeSb.cx = sizeSb.cy = 0;
	DWORD dwStyle = GetStyle();

	if (GetScrollBarCtrl(SB_VERT) == NULL)
	{
		// vert scrollbars will impact client area of this window
		sizeSb.cx = GetSystemMetrics(SM_CXVSCROLL);
		if (dwStyle & WS_BORDER)
			sizeSb.cx -= GetSystemMetrics(SM_CXBORDER);
	}
	if (GetScrollBarCtrl(SB_HORZ) == NULL)
	{
		// horz scrollbars will impact client area of this window
		sizeSb.cy = GetSystemMetrics(SM_CYHSCROLL);
		if (dwStyle & WS_BORDER)
			sizeSb.cy -= GetSystemMetrics(SM_CYBORDER);
	}
}

BOOL CShwScrollView::GetTrueClientSize(CSize& size, CSize& sizeSb)
	// return TRUE if enough room to add scrollbars if needed
{
	CRect rect;
	GetClientRect(&rect);
	ASSERT(rect.top == 0 && rect.left == 0);
	size.cx = rect.right;
	size.cy = rect.bottom;
	DWORD dwStyle = GetStyle();

	// first get the size of the scrollbars for this window
	GetScrollBarSizes(sizeSb);

	// first calculate the size of a potential scrollbar
	// (scroll bar controls do not get turned on/off)
	if (sizeSb.cx != 0 && (dwStyle & WS_VSCROLL))
	{
		// vert scrollbars will impact client area of this window
		size.cx += sizeSb.cx;   // currently on - adjust now
	}
	if (sizeSb.cy != 0 && (dwStyle & WS_HSCROLL))
	{
		// horz scrollbars will impact client area of this window
		size.cy += sizeSb.cy;   // currently on - adjust now
	}

	// return TRUE if enough room
	return (size.cx > sizeSb.cx && size.cy > sizeSb.cy);
}

// helper to return the state of the scrollbars without actually changing
//  the state of the scrollbars
void CShwScrollView::GetScrollBarState(CSize sizeClient, CSize& needSb,
	CSize& sizeRange, CPoint& ptMove, BOOL bInsideClient)
{
	// get scroll bar sizes (the part that is in the client area)
	CSize sizeSb;
	GetScrollBarSizes(sizeSb);

	// enough room to add scrollbars
	sizeRange = m_totalDev - sizeClient;
		// > 0 => need to scroll
	ptMove = GetDeviceScrollPosition();
		// point to move to (start at current scroll pos)

	BOOL bNeedH = sizeRange.cx > 0;
	if (!bNeedH)
		ptMove.x = 0;                       // jump back to origin
	else if (bInsideClient)
		sizeRange.cy += sizeSb.cy;          // need room for a scroll bar

	BOOL bNeedV = sizeRange.cy > 0;
	if (!bNeedV)
		ptMove.y = 0;                       // jump back to origin
	else if (bInsideClient)
		sizeRange.cx += sizeSb.cx;          // need room for a scroll bar

	if (bNeedV && !bNeedH && sizeRange.cx > 0)
	{
		ASSERT(bInsideClient);
		// need a horizontal scrollbar after all
		bNeedH = TRUE;
		sizeRange.cy += sizeSb.cy;
	}

	// if current scroll position will be past the limit, scroll to limit
	if (sizeRange.cx > 0 && ptMove.x >= sizeRange.cx)
		ptMove.x = sizeRange.cx;
	if (sizeRange.cy > 0 && ptMove.y >= sizeRange.cy)
		ptMove.y = sizeRange.cy;

	// now update the bars as appropriate
	needSb.cx = bNeedH;
	needSb.cy = bNeedV;

	// needSb, sizeRange, and ptMove area now all updated
}

void CShwScrollView::UpdateBars(BOOL bSendRecalc)
{
	// UpdateBars may cause window to be resized - ignore those resizings
	if (m_bInsideUpdate)
		return;         // Do not allow recursive calls

	// Lock out recursion
	m_bInsideUpdate = TRUE;

	// update the horizontal to reflect reality
	// NOTE: turning on/off the scrollbars will cause 'OnSize' callbacks
	ASSERT(m_totalDev.cx >= 0 && m_totalDev.cy >= 0);

//	Rect rectClient;  3/1/99 - TLB: Section of code commented out below used this variable, but it's not needed now.
	BOOL bCalcClient = TRUE;
	
// allow parent to do inside-out layout first
	CWnd* pParentWnd = GetParent();
/*	if (pParentWnd != NULL)
	{
		// if parent window responds to this message, use just
		//  client area for scroll bar calc -- not "true" client area
		if ((BOOL)pParentWnd->SendMessage(WM_RECALCPARENT, 0,
			(LPARAM)(LPCRECT)&rectClient) != 0)
		{
			// use rectClient instead of GetTrueClientSize for
			//  client size calculation.
			bCalcClient = FALSE;
		}
	}
*/
	CSize sizeClient;
	CSize sizeSb;

/*	if (bCalcClient)   3/1/99 - TLB: Unnecessary
	{
*/
		// get client rect
		if (!GetTrueClientSize(sizeClient, sizeSb))
		{
			// no room for scroll bars (common for zero sized elements)
			CRect rect;
			GetClientRect(&rect);
			if (rect.right > 0 && rect.bottom > 0)
			{
				// if entire client area is not invisible, assume we have
				//  control over our scrollbars
				EnableScrollBarCtrl(SB_BOTH, FALSE);
			}
			m_bInsideUpdate = FALSE;
			return;
		}
/*	}   3/1/99 - TLB: This code is no longer necessary because of code commented out above - it was causing a compiler warning.
	else
	{
		// let parent window determine the "client" rect
		sizeClient.cx = rectClient.right - rectClient.left;
		sizeClient.cy = rectClient.bottom - rectClient.top;
	}
*/
	// enough room to add scrollbars
	CSize sizeRange;
	CPoint ptMove;
	CSize needSb;

	// get the current scroll bar state given the true client area
	GetScrollBarState(sizeClient, needSb, sizeRange, ptMove, bCalcClient);

	// first scroll the window as needed
	ScrollToDevicePosition(ptMove); // will set the scroll bar positions too

	// now update the bars as appropriate
	EnableScrollBarCtrl(SB_HORZ, needSb.cx);
	if (needSb.cx)
		SetScrollRange(SB_HORZ, 0, sizeRange.cx, TRUE);

	EnableScrollBarCtrl(SB_VERT, needSb.cy);
	if (needSb.cy)
		SetScrollRange(SB_VERT, 0, sizeRange.cy, TRUE);

	// Remove recursion lockout
	m_bInsideUpdate = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CShwScrollView scrolling

void CShwScrollView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	ASSERT(pScrollBar == GetScrollBarCtrl(SB_HORZ));    // may be null
	OnScroll(MAKEWORD(nSBCode, -1), nPos);
}

void CShwScrollView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	ASSERT(pScrollBar == GetScrollBarCtrl(SB_VERT));    // may be null
	OnScroll(MAKEWORD(-1, nSBCode), nPos);
}

BOOL CShwScrollView::OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll)
{
	// calc new x position
	int x = GetScrollPos(SB_HORZ);
	int xOrig = x;

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

	switch (HIBYTE(nScrollCode))
	{
	case SB_TOP:
		break;
	case SB_BOTTOM:
		break;
	case SB_LINEUP:
		break;
	case SB_LINEDOWN:
		break;
	case SB_PAGEUP:
		break;
	case SB_PAGEDOWN:
		break;
	case SB_THUMBTRACK:
		break;
	}

	BOOL bResult = OnScrollBy(CSize(x - xOrig, y - yOrig), bDoScroll);
	if (bResult && bDoScroll)
		UpdateWindow();

	return bResult;
}

BOOL CShwScrollView::OnScrollBy(CSize sizeScroll, BOOL bDoScroll)
{
	int xOrig, x;
	int yOrig, y;

	// don't scroll if there is no valid scroll range (ie. no scroll bar)
	CScrollBar* pBar;
	DWORD dwStyle = GetStyle();
	pBar = GetScrollBarCtrl(SB_VERT);
	if ((pBar != NULL && !pBar->IsWindowEnabled()) ||
		(pBar == NULL && !(dwStyle & WS_VSCROLL)))
	{
		// vertical scroll bar not enabled
		sizeScroll.cy = 0;
	}
	pBar = GetScrollBarCtrl(SB_HORZ);
	if ((pBar != NULL && !pBar->IsWindowEnabled()) ||
		(pBar == NULL && !(dwStyle & WS_HSCROLL)))
	{
		// horizontal scroll bar not enabled
		sizeScroll.cx = 0;
	}

	// adjust current x position
	xOrig = x = GetScrollPos(SB_HORZ);
	int xMin, xMax;
	GetScrollRange(SB_HORZ, &xMin, &xMax);
	ASSERT(xMin == 0);
	x += sizeScroll.cx;
	if (x < 0)
		x = 0;
	else if (x > xMax)
		x = xMax;

	// adjust current y position
	yOrig = y = GetScrollPos(SB_VERT);
	int yMin, yMax;
	GetScrollRange(SB_VERT, &yMin, &yMax);
	ASSERT(yMin == 0);
	y += sizeScroll.cy;
	if (y < 0)
		y = 0;
	else if (y > yMax)
		y = yMax;

	// did anything change?
	if (x == xOrig && y == yOrig)
		return FALSE;

	if (bDoScroll)
	{
		// do scroll and update scroll positions
		ScrollWindow(-(x-xOrig), -(y-yOrig));
		if (x != xOrig)
			SetScrollPos(SB_HORZ, x);
		if (y != yOrig)
			SetScrollPos(SB_VERT, y);
	}
	return TRUE;
}

BOOL CShwScrollView::OnMouseWheel(UINT fFlags, short zDelta, CPoint point)
{
	// don't scroll if there is no valid scroll range (ie. no scroll bar)
	CScrollBar* pBar;
	DWORD dwStyle = GetStyle();
	pBar = GetScrollBarCtrl(SB_VERT); // 1.4gq Fix bug of crash on wheel in browse filter no recs
	if ((pBar != NULL && !pBar->IsWindowEnabled()) ||
		(pBar == NULL && !(dwStyle & WS_VSCROLL)))
	{
		// vertical scroll bar not enabled
		return FALSE;
	}

    UINT nScrollCode = 0;

    if( zDelta > 0 )
        // scroll up
        nScrollCode = MAKEWORD(-1, SB_LINEUP);
    else
        nScrollCode = MAKEWORD(-1, SB_LINEDOWN);

    // the thumbwheel is usually 3 lines (there's a better way to do this, I'm sure,
    //  but I don't want to spend a lot of time learning the scrolling mechanism here).
    for( int i = 0; i < 3; i++ )
        OnScroll(nScrollCode, 0);

    return  TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CShwScrollView diagnostics

#ifdef _DEBUG
void CShwScrollView::AssertValid() const
{
	CView::AssertValid();

	switch (m_nMapMode)
	{
	case MM_NONE:
	case MM_TEXT:
	case MM_LOMETRIC:
	case MM_HIMETRIC:
	case MM_LOENGLISH:
	case MM_HIENGLISH:
	case MM_TWIPS:
		break;
	case MM_ISOTROPIC:
	case MM_ANISOTROPIC:
		ASSERT(FALSE); // illegal mapping mode
	default:
		ASSERT(FALSE); // unknown mapping mode
	}
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
