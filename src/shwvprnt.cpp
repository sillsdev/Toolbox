// shwvprnt.cpp : implementation of printing parts of the CShwView class
//

#include "stdafx.h"
#include "toolbox.h"
#include "shw.h"
#include <limits.h>

#include "shwdoc.h"
#include "shwview.h"
#include "project.h"
#include "page_d.h"

#include "font.h"

#include <ctype.h>


#ifdef _MAC
#include "resmac.h" // Mac specific resources
#else
#include "respc.h"
#endif


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define ALLRECORDS


// !!!! Temporary situation.  Move these to shwview.h !!!!
#define TEXTOUT_NORMAL      1
#define TEXTOUT_HIGHLIGHT   2

void CShwView::PageSetup()
{
    ASSERT(ptyp());
    ASSERT(ptyp()->pprtp());
    CPageSetupDlg dlgPage(ptyp()->pprtp());
    dlgPage.DoModal();
}

void CShwView::Print_MoveToPage(CPrintInfo* pInfo, CDC* pDC)
{
    CPrintProperties* pprtp = (CPrintProperties*)pInfo->m_lpUserData;

    if ((UINT)(pprtp->pptplst()->lGetCount() + 1) == pInfo->m_nCurPage)
    // Save the current position as the top of this page.
        pprtp->SaveCurrentPositionAtEnd();
    else // else a different page, probably an earlier one.
        {
        // Start looking from beginning.
        UINT nPagesTo = 1;
        CPrintPosition* pptp = pprtp->pptplst()->pptpFirst();

        while ( (nPagesTo < pInfo->m_nCurPage) && (pptp) )
            {
            pptp = pprtp->pptplst()->pptpNext(pptp);
            nPagesTo++;
            }

        // What I need to do here is to "print" up to the current page.
        // By "print", I mean go through the print process without printing
        // or displaying anything.

        if (nPagesTo < pInfo->m_nCurPage)
            {
            UINT nWhereStopped;

            // If this is our first attempt at printing, set the top of the first page.
            // (This assumes that OnBeginPrinting has already established the starting point.)
            if (pprtp->pptplst()->bIsEmpty())
                pprtp->SaveCurrentPositionAtEnd();

            pptp = pprtp->pptplst()->pptpLast();
            // Start again from the last page printed.
            ASSERT(pptp != NULL);


            pprtp->prel() = pptp->prel();
            pprtp->rps() = pptp->rps();


            Print_SkipTo(pInfo, pDC, &nWhereStopped);
            }
        else
            {
            pprtp->prel() = pptp->prel();
            pprtp->rps() = pptp->rps();
            }
        }
}


void CShwView::Print_SkipTo(CPrintInfo* pInfo, CDC* pDC, UINT* pnWhereStopped)
{
    CPrintProperties* pprtp = (CPrintProperties*)pInfo->m_lpUserData;

    UINT nPageCurrent = (UINT)pprtp->pptplst()->lGetCount();

    // Set up the device context
    CShwScrollView::OnPrepareDC(pDC, pInfo);    // This sets the mapmode and viewport origin.
    pInfo->m_bContinuePrinting = TRUE;  // Because OnPrepareDC sets it to FALSE.
    pDC->SetTextAlign( TA_UPDATECP );

    // Set the drawing rectangle, which is needed for calculating the page.
        pInfo->m_rectDraw.SetRect(0, 0,
            pDC->GetDeviceCaps(HORZRES),
            pDC->GetDeviceCaps(VERTRES));
        pDC->DPtoLP(&pInfo->m_rectDraw);

    // "Print" to the end of the current page.
    DoPrint(pDC, pInfo, TRUE);

    nPageCurrent++;
    // Print subsequent pages until we reach the page to be printed, adding the top of each page
    // to the list.

    while (pInfo->m_bContinuePrinting)
        {
        // Check to see if we should continue printing.

        if (pprtp->bPrintingAllRecords())
            {
            if ( pprtp->rps().pfld == NULL)
                {
                if ((CRecLookEl*)pind()->pnrlNext(pprtp->prel()) == NULL)  // 1998-12-02 MRP
                    {
                    pInfo->m_bContinuePrinting = FALSE;
                    break;
                    }
                else // go to next element
                    {
                    pprtp->prel() = pind()->pnrlNext(pprtp->prel());
                    pprtp->rps().SetPos(0, pprtp->prel()->prec()->pfldFirst(), pprtp->prel()->prec());
                    }
                }
            }
        else
            {
            if ( pprtp->rps().pfld == NULL)
                {
                pInfo->m_bContinuePrinting = FALSE;
                break;
                }
            }

        if (nPageCurrent >= pInfo->m_nCurPage)
            {
            pprtp->SaveCurrentPositionAtEnd();
            break;
            }

        // Set up the device context
        CShwScrollView::OnPrepareDC(pDC, pInfo);    // This sets the mapmode and viewport origin.
        pInfo->m_bContinuePrinting = TRUE;  // Because OnPrepareDC sets it to FALSE.

        pprtp->SaveCurrentPositionAtEnd();
        DoPrint(pDC, pInfo, TRUE);
        nPageCurrent++;
        }


    *pnWhereStopped = nPageCurrent;

    if (nPageCurrent < pInfo->m_nCurPage)
        {
        ASSERT(pprtp->rps().pfld == NULL);
        }

}


void CShwView::OnPrint(CDC* pDC, CPrintInfo* pInfo )
{
    DoPrint(pDC, pInfo);
}


BOOL CShwView::OnPreparePrinting(CPrintInfo* pInfo)
{
    BOOL bResult = DoPreparePrinting(pInfo);
    pInfo->m_strPageDesc = ""; //"Page %d";
    return bResult;
}

void CShwView::OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo)
{
    // Set the first record and field to be printed on the first page.
    CPrintProperties* pprtp = ptyp()->pprtp();

    ASSERT( (pprtp->rps().pfld == NULL) && (pprtp->rps().prec == NULL));

    // Prepare margin information.
    //CRect rectMargins = pprtp->GetTruePageMargin(pDC);

    pprtp->SetEndOfBrowse(FALSE);

    if (pprtp->bPrintingAllRecords())
        pprtp->prel() = pind()->pnrlFirst();
    else
        pprtp->prel() = m_prelCur;

    if (pprtp->prel())
        pprtp->rps().SetPos(0, pprtp->prel()->prec()->pfldFirst(), pprtp->prel()->prec());
    else    // Empty recordset
        pprtp->rps().SetPos(0, NULL, NULL);

    // Parse header and footer info.
    pprtp->Parse(this);

    pInfo->m_lpUserData = (LPVOID)pprtp;
}


void CShwView::OnEndPrinting(CDC* pDC, CPrintInfo* pInfo)
{
    CPrintProperties* pprtp = (CPrintProperties*)pInfo->m_lpUserData;
    if (pprtp)
        {
        pprtp->ClearRecPos();
        pprtp->EmptyHeaders();
        pInfo->m_lpUserData = NULL;
        }
}


void CShwView::DoPrint(CDC* pDC, CPrintInfo* pInfo, BOOL bDontPrint)
{
    pDC->SetBkMode( TRANSPARENT );
    pDC->SetTextAlign( TA_UPDATECP );

    // RNE 1996-10-31
    // For some reason, it is necessary to create and select a new font
    // for this device context.  Otherwise in print preview the text
    // will not be displayed properly until text of a different font
    // gets displayed.
    // It appears that creating and selecting a font makes it work.
    // I chose to create a new marker font object because the marker
    // font is always available.
    CFont* pfntMkr = Shw_pProject()->pfntMkrs();
    CFontDef fntdef(pfntMkr);
    CFont fnt;
    fntdef.iCreateFont(&fnt);
    CFont* pfntOld = (CFont*)pDC->SelectObject((CFont*)&fnt);

    // Determine page area info, etc.
    // !!! RNE This should really go in OnBeginPrinting or possibly in OnPrepareDC.
    CRect   rectPage,
            rectMarkerPrintArea,
            rectFieldPrintArea,
            rectBrowseArea,
            rectHeader,
            rectFooter;
    CalculatePrintAreas(pDC, pInfo, rectPage, rectMarkerPrintArea, rectFieldPrintArea,
                        rectBrowseArea, rectHeader, rectFooter);

    if (!bDontPrint)
        PrintHeader(pDC, rectHeader, pInfo);

    if (m_bBrowsing)
        {
        pDC->MoveTo( rectBrowseArea.left, rectBrowseArea.top );
        PrintBrowse(pDC, pInfo, rectBrowseArea, bDontPrint);
        }
    else
        {
        pDC->MoveTo( rectMarkerPrintArea.left, rectMarkerPrintArea.top );
        PrintRecord( pDC, rectMarkerPrintArea, rectFieldPrintArea, pInfo, bDontPrint);
        }

    if (!bDontPrint)
        PrintFooter(pDC, rectFooter, pInfo);
}


void CShwView::PrintRecord(CDC* pDC, CRect &rectMarkerPrintArea,
                            CRect &rectFieldsPrintArea, CPrintInfo* pInfo,
                            BOOL bDontPrint)
{
// Set up for printing this record.
    /*
    What I am going to have to do is use OnBeginPrinting to set up record
    and field position information that will be remembered on each call to
    OnPrint.  I think I should store that info  in the CPrintProperties
    object.  This could be retained in the pInfo auxilieary data pointer.
    */
    BOOL bEndOfRecord = FALSE;
    BOOL bPageBreak = FALSE;

    CPrintProperties* pprtp = (CPrintProperties*)pInfo->m_lpUserData;
	RememberInterlinearWidths( TRUE );
    do 
		{
		PrintRec(pDC, rectMarkerPrintArea, rectFieldsPrintArea, pprtp->prel(),
					pprtp->rps(), &bEndOfRecord, &bPageBreak, bDontPrint);

		if (bEndOfRecord && pprtp->bPrintingAllRecords())
			{
			// If there are more records
			if ((CRecLookEl*)pind()->pnrlNext(pprtp->prel()) != NULL)  // 1998-12-02 MRP
				{
				if ( !ptyp()->bTextFile() ) // If not text file, move to next index // 1.6.0ab 
					pprtp->prel() = pind()->pnrlNext(pprtp->prel());
				else // If text file, move forward until start of next record is found // 1.6.0ab Fix bug of repeated records on print of text
					{
					CRecord* precCur = m_rpsCur.prec; // 1.6.0ab 
					do {
						pprtp->prel() = pind()->pnrlNext(pprtp->prel()); // 1.6.0ab 
						} while ( pprtp->prel() && pprtp->prel()->prec() == precCur ); // 1.6.0ab 
					}
				pprtp->rps().SetPos(0, pprtp->prel()->prec()->pfldFirst(), pprtp->prel()->prec());

				// Put space between this and the next record.
				CPoint pntCurrent = pDC->GetCurrentPosition();
				pntCurrent.y -= pprtp->iSpaceBetweenRecords();

				if (pntCurrent.y < rectMarkerPrintArea.bottom)
					return;
				else
					pDC->MoveTo(pntCurrent);    
				}
			else    // no more records.
				return;
			}
		}
        while (pprtp->bPrintingAllRecords() && (!bPageBreak) && (!pprtp->bBreakBetweenRecords()));
// End printing of record.
	RememberInterlinearWidths( FALSE );
}


void CShwView::PrintRec(CDC* pDC, CRect &rectMarkerPrintArea,
                                CRect &rectFieldsPrintArea,
                        CNumberedRecElPtr& prel, CRecPos& rps,
                        BOOL *pbEndOfRecord, BOOL *pbPageBreak,
                        BOOL bDontPrint)
{
    //int iRet;  09-09-1997 Get's never used
    *pbEndOfRecord = FALSE;
    *pbPageBreak = FALSE;

    int iCurPosY;
    const char* pszAfterBreak;

    TEXTMETRIC tm;
    pDC->SelectObject( Shw_pProject()->pfntMkrs() );
    pDC->GetTextMetrics(&tm);
    int iMkrAscent = tm.tmAscent; // get measurements of font used to print marker names
    int iThisMkrHt = tm.tmHeight;

    CFont* pfntFld = (CFont*)rps.pfld->pmkr()->pfnt(); // had to cast away const attribute
    pDC->SelectObject( pfntFld ); // get font for displaying field contents
    pDC->GetTextMetrics(&tm); // measure fonts being used so we can align them nicely
    int iThisLineHt = tm.tmHeight;

    // Finish printing a previous field as needed.
    if (rps.psz() != rps.pszFieldBeg())
        {
        pDC->MoveTo( rectFieldsPrintArea.left, pDC->GetCurrentPosition().y);
        if ( rps.bInterlinear() ) //  If an interlinear field, do aligned output
            {
            // Also put this in the initial loop.
            PrintFldInterlinear(pDC, rps, rectFieldsPrintArea, bDontPrint);
            }
        else
            {
            pszAfterBreak = rps.psz();
            while ( TRUE ) // while more lines in field
                {
                const char* pszStart = pszAfterBreak; // Remember start of line
                pszAfterBreak = pszNextLineBrk( pszStart ); // Find line break

                // If we will exceed the bottom of the page, return with needs page break.
                iCurPosY = pDC->GetCurrentPosition().y;

                if (iCurPosY - iThisLineHt < rectMarkerPrintArea.bottom)
                    {
                    // Set the rps back to the current field, position at beginning of this line.
                    rps.SetPos(pszStart, rps.pfld);
                    *pbPageBreak = TRUE;
                    return;
                    }
                    if (!bDontPrint)
                        // 09-09-1997 Replaced the following line by the other one
                        // Note that i Ret isn't used in the code
                        //iRet = pDC->ExtTextOut( 0, 0, ETO_CLIPPED, &rectFieldsPrintArea, pszStart, pszAfterBreak - pszStart, NULL );
                        TextOutClip(pDC, rps.pfld->plng(), pszStart, 
                                    pszAfterBreak - pszStart, &rectFieldsPrintArea,
                                    rps.pmkr(), 0, ETO_CLIPPED);    
                    CPoint pnt = pDC->GetCurrentPosition();

                    pDC->MoveTo( rectFieldsPrintArea.left, pnt.y - iThisLineHt );
                    if ( !*pszAfterBreak ) // If at end of field, break
                         break;
                    pszAfterBreak++; // Move past CR
                }
            }
        // Move to next field in record.
		do { // 1.5.0aa Make print not print hidden fields
			rps = rps.rpsNextField(); // 1.5.0aa  
			} while ( rps.pfld && bHiddenField( rps.pfld ) ); // 1.5.0aa Move to next non-hidden field in record
        }


    // Print each field...
    for (; rps.pfld != NULL; ) // rps = rps.rpsNextField()) // 1.5.0aa 
        {
        // First, make sure this line won't break a page.
        // Must test on both the marker font and the field font.
        pfntFld = (CFont*)rps.pfld->pmkr()->pfnt(); // had to cast away const attribute
        pDC->SelectObject( pfntFld ); // get font for displaying field contents

        int iYOffset = 0;
        pDC->GetTextMetrics(&tm); // measure fonts being used so we can align them nicely
        iThisLineHt = tm.tmHeight; // Assumes constant height for each line of this field.
        int iFldAscent = tm.tmAscent;
        pDC->SelectObject( Shw_pProject()->pfntMkrs() );
        if ( iMkrAscent < iFldAscent )
            {
            iYOffset = iFldAscent - iMkrAscent; // make baselines align
            iYOffset -= iYOffset/5;
            }

        iCurPosY = pDC->GetCurrentPosition().y;
        if ( (iCurPosY - iThisLineHt < rectFieldsPrintArea.bottom)
            || (iCurPosY - iThisMkrHt < rectMarkerPrintArea.bottom) )
            {
            // Set the rps back to the current field, position zero.
            rps.SetPos(0,rps.pfld);
            *pbPageBreak = TRUE;
            return;
            }

          // Print out the marker here.
        pDC->MoveTo( rectMarkerPrintArea.left, iCurPosY - iYOffset);
        CRect rectMarker(rectMarkerPrintArea.left, iCurPosY - iYOffset,
                         rectMarkerPrintArea.right, iCurPosY - iThisMkrHt - iYOffset);
        // Bounding rectangle for drawing the marker alone.

        if (!bDontPrint)
            rps.pfld->pmkr()->DrawMarker(pDC, rectMarker,
                    m_bViewMarkerHierarchy, m_bViewMarkers, m_bViewFieldNames);

        pDC->SelectObject( pfntFld ); // set font for displaying field contents
        pDC->MoveTo( rectFieldsPrintArea.left, iCurPosY );

        if ( rps.bInterlinear() ) //  If an interlinear field, do aligned output
            {
            // Also put this in the initial loop.
            PrintFldInterlinear(pDC, rps, rectFieldsPrintArea, bDontPrint);
            }
        else
            {
            // Print each line of the field...
            pszAfterBreak = rps.pszFieldBeg();

            while ( TRUE ) // while more lines in field
                {
                const char* pszStart = pszAfterBreak; // Remember start of line
                pszAfterBreak = pszNextLineBrk( pszStart ); // Find line break

                // If we will exceed the bottom of the page, return with needs page break.
                int iCurPosY = pDC->GetCurrentPosition().y;

                if (iCurPosY - iThisLineHt < rectMarkerPrintArea.bottom)
                    {
                    // Set the rps back to the current field, position at beginning of this line.
                    rps.SetPos(pszStart, rps.pfld);
                    *pbPageBreak = TRUE;
                    return;
                    }
                int iLen = pszAfterBreak - pszStart;
                const char* pszPrint = pszStart;
                if (!bDontPrint)
                    {
                    CField* pfld = rps.pfld;
                    // 1998-03-03 MRP: Removed explicit rendering calls
                    // because TextOutClip does it automatically.
                    // 09-09-1997 Replaced the following line by the other one
                    // Note that i Ret isn't used in the code    
                    //iRet = pDC->ExtTextOut( 0, 0, ETO_CLIPPED, &rectFieldsPrintArea, pszPrint, iLen, NULL );
                    TextOutClip(pDC, rps.pfld->plng(), pszPrint, iLen,
                                &rectFieldsPrintArea, rps.pmkr(), 0, ETO_CLIPPED);
                    }
                CPoint pnt = pDC->GetCurrentPosition();

                pDC->MoveTo( rectFieldsPrintArea.left, pnt.y - iThisLineHt ); // Was an addition operator.
                if ( !*pszAfterBreak ) // If at end of field, break
                     break;
                pszAfterBreak++; // Move past CR
                }
            }
		do { // 1.5.0aa 
			rps = rps.rpsNextField(); // 1.5.0aa  
			} while ( rps.pfld && bHiddenField( rps.pfld ) ); // 1.5.0aa Move to next non-hidden field in record
        }

    *pbEndOfRecord = TRUE;
}


void CShwView::PrintFldInterlinear(/*CPrintProperties* pprtp, */ CDC* pDC, CRecPos& rps, CRect &rectFieldsPrintArea, BOOL bDontPrint)
{
    CPoint pntCur = pDC->GetCurrentPosition();

    TEXTMETRIC tm;
    pDC->GetTextMetrics(&tm); // measure font being used
    int iThisLineHt = tm.tmHeight;

    const char* psz = rps.psz();
    const char* pszLineStart = psz; // Remember start of line
    psz = rps.pfld->plng()->pszSkipSpace( psz ); // Move to start of first word
    // Not needed BOOL bBlankBeforeWord = psz > pszLineStart; // if so, we need to blank out area up to first word
    int iLine = pDC->GetCurrentPosition().y;

    BOOL bEmptyLine = TRUE;
    while ( TRUE ) // while more words in field
        {

        if ( !*psz ) // If at end of field, break
            {
            iLine -= iThisLineHt; // changed to accomodate -y.
            pDC->MoveTo( rectFieldsPrintArea.left, iLine );

            CRecPos rps( psz - *rps.pfld, rps.pfld, m_prelCur->prec() );
            break;
            }
        if ( *psz == '\n' )
            {
            iLine -= iThisLineHt; // Changed to accomodate -y
            pDC->MoveTo( rectFieldsPrintArea.left, iLine );
            psz++; // Move past CR
            pszLineStart = psz; // Remember start of line
            continue; // Don't output CR
            }

        bEmptyLine = FALSE; // this line non-empty
        const char* pszWd = psz; // Remember start of word
        psz = rps.pfld->plng()->pszSkipNonWhite( psz ); // Find end of word
        int iFixDist = pszWd - pszLineStart; // Find fixed position distance from start or from last CR
        int iCol = rectFieldsPrintArea.left + iInterlinPixelPos( rps, iFixDist ); // 6.0d Make print use improved interlinear measure, get position of start
        pDC->MoveTo( iCol, iLine );

        const char* pszNextWd = rps.pfld->plng()->pszSkipSpace( psz ); // find start of next word
        int iColEnd = rectFieldsPrintArea.left + iInterlinPixelPos( rps, pszNextWd - pszLineStart ); // 6.0d calculate width of this column

        int iLen = psz - pszWd;
        if ( iLen > 0 )
            TextOutClip( pDC, rps.pfld->plng(), pszWd, iLen,
					&rectFieldsPrintArea, rps.pmkr(), 0, ETO_CLIPPED );

        psz = pszNextWd; // Move to start of next word

        m_iMaxLineWd = max( iColEnd, m_iMaxLineWd );
        }
}


void CShwView::PrintBrowse(CDC* pDC, CPrintInfo* pInfo, CRect& rectBrowseArea,
                        BOOL bDontPrint)
{
    CPrintProperties* pprtp = (CPrintProperties*)pInfo->m_lpUserData;

    // Look for the tallest line, base the line height on that.
    int iMaxLineHt = m_brflst.iLineHt();

    pDC->SetTextAlign( TA_NOUPDATECP );
    CPoint pntCur = pDC->GetCurrentPosition();

    pntCur.x = rectBrowseArea.left + eDividerPad;
    pDC->MoveTo(pntCur);

    // At the top of each page, put the markers.
    CRect rectMkrs( rectBrowseArea.left + eDividerPad, -rectBrowseArea.top,
                 0, -rectBrowseArea.top - iMaxLineHt );

    for ( CBrowseField* pbrf = m_brflst.pbrfFirst(); pbrf; pbrf = m_brflst.pbrfNext( pbrf ) )
        {
        CFont* pfnt = (CFont*)Shw_pProject()->pfntMkrs();
        pDC->SelectObject( pfnt );
        pDC->MoveTo( rectMkrs.left, pntCur.y );
                        // Adjust clipping rectangle.
        rectMkrs.right = rectMkrs.left + pbrf->iWidth(); // clip at field width
        rectMkrs.top = pntCur.y;
        rectMkrs.bottom = rectMkrs.top - iMaxLineHt;
        // Bounding rectangle for drawing the marker alone.

        pDC->SetTextAlign( TA_NOUPDATECP );
        if (!bDontPrint)
            //09-10-1997 Replaced the following command
            //pDC->ExtTextOut( rectMkrs.left, rectMkrs.top, ETO_CLIPPED | ETO_OPAQUE,
            //                 &rectMkrs, "\\" + pbrf->pmkr()->sMarker(), 
            //                 pbrf->pmkr()->sMarker().GetLength()+1, NULL );
            TextOutClip(pDC, pbrf->pmkr()->plng(), "\\" + pbrf->pmkr()->sMarker(), 
                        pbrf->pmkr()->sMarker().GetLength()+1, &rectMkrs, (CMarker*)pbrf->pmkr(),
                        0, ETO_CLIPPED | ETO_OPAQUE, rectMkrs.left, rectMkrs.top);
            
        rectMkrs.left = rectMkrs.right + eBrowseDividerWidth; // step over divider to next field
        }

    pntCur.y -= iMaxLineHt;  // Add vertical space.

    CRect rectBrowseFlds(rectBrowseArea.left, pntCur.y,
                        rectBrowseArea.right, rectBrowseArea.bottom);

    // While we're within the drawing area and there are more records to display.
    while( (pprtp->prel() )
            && (pntCur.y - iMaxLineHt >= rectBrowseArea.bottom) )
        {
        pntCur.x = rectBrowseFlds.left + eDividerPad;
        pDC->MoveTo(pntCur);

        // If we're not printing this page, skip the display of fields.
        if (!bDontPrint)
            {
            // Print the relevant fields of this record.
            CRect rect( rectBrowseFlds.left + eDividerPad, -rectBrowseFlds.top,
                         0, -rectBrowseFlds.top - iMaxLineHt );
            for ( CBrowseField* pbrf = m_brflst.pbrfFirst(); pbrf; pbrf = m_brflst.pbrfNext( pbrf ) )
                {
                const CField* pfld = pprtp->prel()->pfldFirstInSubRecord(pbrf->pmkr());

                // Adjust clipping rectangle.
                rect.right = rect.left + pbrf->iWidth(); // clip at field width
                rect.top = pntCur.y;
                rect.bottom = rect.top - iMaxLineHt;

                pntCur.x = rect.right;
                // pDC->MoveTo(pntCur);
                if ( pfld )
                    {
                    int iDiff = m_brflst.iLineHt() - pfld->pmkr()->iLineHeight(); // this line shorter than line height?
                    int iY = rect.top - ( iDiff / 2 ); // align text in center of line
                    CFont* pfnt = (CFont*)pfld->pmkr()->pfnt(); // had to cast away const attribute
                    CFont* pfntOld = pDC->SelectObject( pfnt );
                    pDC->SetTextAlign( TA_NOUPDATECP );
                    
                    // 09-10-1997 Replaced the following command
                    //ExtTextOut_AsOneLine( pDC, rect.left, iY,
                    //    ETO_CLIPPED | ETO_OPAQUE, &rect, *pfld, pfld->GetLength(), NULL );
                    ExtTextOut_AsOneLinePrnt(pDC, rect.left , iY, 
                                             ETO_CLIPPED | ETO_OPAQUE, &rect, 
                                             *pfld, pfld->GetLength(), 0 , pfld->pmkr() );                
                    }
                else
                    {
                    pDC->SetTextAlign( TA_NOUPDATECP );
					Str8 sText = ""; // 1.4qta
					CString swText = swUTF16( sText ); // 1.4qta
                    pDC->ExtTextOut( rect.left, rect.top, ETO_CLIPPED | ETO_OPAQUE, &rect, swText, 0, NULL ); // 1.4qta
                    }
                rect.left = rect.right + eBrowseDividerWidth; // step over divider to next field
                }
            }

        // Advance to the next record, if there is one.
        if (pind()->pnrlNext(pprtp->prel()))
            {
            pprtp->prel() = pind()->pnrlNext(pprtp->prel());
            pprtp->rps().SetPos(0, pprtp->prel()->prec()->pfldFirst(), pprtp->prel()->prec());
            }
        else
            {
            pprtp->SetEndOfBrowse(TRUE);
            break;
            }

        pntCur.y -= iMaxLineHt;
        };
}




void CShwView::PrintHeader(CDC* pDC, CRect& rect, CPrintInfo* pInfo)
{
    CPrintProperties* pprtp = (CPrintProperties*)pInfo->m_lpUserData;
    pprtp->phdrlstHeader()->Draw(pDC, pInfo, this, rect);
}


void CShwView::PrintFooter(CDC* pDC, CRect& rect, CPrintInfo* pInfo)
{
    CPrintProperties* pprtp = (CPrintProperties*)pInfo->m_lpUserData;
    pprtp->phdrlstFooter()->Draw(pDC, pInfo, this, rect, TRUE);
}



void CShwView::CalculatePrintAreas( CDC* pDC, CPrintInfo* pInfo, CRect &rectPage, CRect &rectMarkerPrintArea,
                                    CRect &rectFieldsPrintArea, CRect& rectBrowseArea,
                                    CRect &rectHeader, CRect& rectFooter )
{
    CSize extVP = pDC->GetViewportExt( );   // Printing area/view.
    int iFldStartCol = m_iFldStartCol;
    int iDividerWidth = eRecordDividerWidth;
    int iDividerPad = eDividerPad;

    rectPage = pInfo->m_rectDraw;

    CPrintProperties* pprtp = (CPrintProperties*)pInfo->m_lpUserData;

    // Prepare margin information.
    CRect rectMargins = pprtp->GetTruePageMargin(pDC);

    rectPage.left   += rectMargins.left;  // add these four lines
    rectPage.top    -= rectMargins.top;
    rectPage.right  -= rectMargins.right;
    rectPage.bottom += rectMargins.bottom;

    // Give a few lines at the top and bottom for a header/footer.

    rectHeader = rectPage;
    rectHeader.bottom = rectPage.top - 2*pprtp->phdrlstHeader()->iLineHt(pDC);
    // For now, it will be twice the height of the header line.

    rectFooter = rectPage;
    rectFooter.top = rectPage.bottom + 2*pprtp->phdrlstFooter()->iLineHt(pDC);
    // For now, it will be twice the height of the footer line.

    CRect rectUserData( rectPage.left, rectHeader.bottom,
                        rectPage.right, rectFooter.top);

    rectBrowseArea = rectUserData; // rectPage;

    rectMarkerPrintArea.SetRect(rectUserData.left, rectUserData.top,
                                rectUserData.left + iFldStartCol - iDividerWidth, rectUserData.bottom);

    rectFieldsPrintArea.SetRect(rectUserData.left + iFldStartCol,
                                rectUserData.top, rectUserData.right,
                                rectUserData.bottom);
}


void CShwView::ExtTextOut_AsOneLinePrnt(CDC* pDC, int xLeft, int y, 
                                        UINT nOptions, LPCRECT prect, LPCSTR psz,
                                        UINT len, int iSelected, CMarker* pmkr)
{
    ExtTextOut_AsOneLinePrnt(pDC, &xLeft, y, nOptions, prect, psz, len, iSelected,
                             pmkr);
}


void CShwView::ExtTextOut_AsOneLinePrnt(CDC* pDC, int* pxLeft, int y, 
                                        UINT nOptions, LPCRECT prect, LPCSTR psz,
                                        UINT len, int iSelected, CMarker* pmkr)
{
    static const char s_pszSpace[] = " ";
    ASSERT( pDC );
    ASSERT( pmkr );
    // NOTE: Logic below assumes the text is to be clipped to a rectangle.
    ASSERT( nOptions & ETO_CLIPPED );
    ASSERT( prect );
    CRect rectRemaining = *prect;
    const char* pszLineBegin = psz;
    ASSERT( pszLineBegin );
    UINT lenRemaining = len;
    ASSERT( pxLeft );
    int xLeft = *pxLeft;
    while ( TRUE )
        {
        // NOTE: This logic assumes that the string is null-terminated,
        // even when less than its entirety is to be written.
        const char* pszLineEnd = pszNextLineBrk(pszLineBegin);
        UINT lenLine = pszLineEnd - pszLineBegin;
        if ( lenRemaining < lenLine )
            lenLine = lenRemaining;
        TextOutClip(pDC, pmkr->plng(), pszLineBegin, lenLine, &rectRemaining,
                    pmkr, iSelected, nOptions, xLeft, y);
        // 1996-06-05 MRP: I first tried using SetTextAlign with TA_UPDATECP,
        // but calling GetTextExtent explicitly seems better.
        xLeft += pmkr->plng()->GetTextExtent( pDC, pszLineBegin, lenLine ).cx; // 1.4qtm Upgrade some GetTextExtent for Unicode build
        lenRemaining -= lenLine;

        if ( !*pszLineEnd || lenRemaining == 0)  // If it's all been written
            break;

        // Output a space in place of the newline, and then move past it.
        // Prevent ETO_OPAQUE from erasing previously written lines.
        // 1996-06-05 MRP: Review this logic when we support RTL scripts
        if ( xLeft < rectRemaining.left )
            ;  
        else if ( xLeft <= rectRemaining.right )
            rectRemaining.left = xLeft;
        else  // If outside the clipping rectangle
            break;
        TextOutClip(pDC, pmkr->plng(), s_pszSpace, 1, &rectRemaining,
                    pmkr, iSelected, nOptions, xLeft, y);        
        xLeft += pmkr->plng()->GetTextExtent( pDC, s_pszSpace, 1 ).cx; // 1.4qtm Upgrade some GetTextExtent for Unicode build
        if ( xLeft < rectRemaining.left )
            ;  
        else if ( xLeft <= rectRemaining.right )
            rectRemaining.left = xLeft;
        else  // If outside the clipping rectangle
            break;
        pszLineBegin = pszLineEnd + 1;  // Beginning of the next line
        }       
        
    *pxLeft = xLeft;
}




