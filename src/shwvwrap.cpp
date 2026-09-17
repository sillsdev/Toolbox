// shwvwrap.cpp : implementation of wrapping parts of the CShwView class
//

#include "stdafx.h"
#include "toolbox.h"
#include "shwdoc.h"
#include "shwview.h"
#include "progress.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

static BOOL bWdEnd( const char* psz ) // True if at white space or end of line. Wrap can't use language space for wrapping because it would lose information about what kind of space had been wrapped.
{
    return ( !*psz || *psz == ' ' || *psz == '\n' );
}

const char* pszPrevWdEnd( const char* pszStart, const char* psz ) // Find beginning of previous word - NULL if no previous word
{
    ASSERT( psz > pszStart );
    for ( ; bWdEnd( psz ); psz-- ) // scan back through any whitespace
        if ( psz <= pszStart )
            return NULL;

    for ( ; psz > pszStart; psz-- ) // look for beginning of this word
        if ( bWdEnd( psz ) )
            return psz; // return pointer to whitespace just prior to the word found

    return NULL;
}

char* pszNextWdStart( char* psz ) // Find start of next word
{
    for ( ; *psz && *psz != ' '; psz++ ) // hunt for space
        if ( *psz == '\n' ) // at end of line
            return psz;

    for ( ; *psz && *psz == ' '; psz++ ) // Hunt for start of next word
        if ( *psz == '\n' ) // at end of line
            return psz;

    return psz;
}

static BOOL bLineEnd( const char* psz ) // True if at end of line.
{
	return ( !*psz || *psz == '\n' );
}

char* pszFindLineEnd( char* psz ) // Return end of line
	{
	while ( !bLineEnd( psz ) )
		psz++;
	return psz;
	}

BOOL CShwView::bAtParagraphEnd( const char* psz )
{
    return( *psz == '\0' ||
            ( *psz == '\n' && ( *(psz+1) == '\n' || *(psz+1) == '\0' ) ) );
}

BOOL CShwView::bAtParagraphStart( CRecPos* prps )
{
    if ( !prps->iChar ) // start of field == start of paragraph?!?
        return TRUE;
    const char* psz = prps->psz();
    if ( !*psz || *psz == '\n' || *(psz-1) != '\n' )
        return FALSE;
    if ( prps->iChar >= 2 && *(psz-2) != '\n' )
        return FALSE;
    return TRUE;
}

BOOL CShwView::bInParagraph( CRecPos* prps )
{
    const char* psz = prps->psz();
    if ( !prps->iChar )
        {
        if ( *psz && *psz != '\n' )
            return TRUE;
    
        }
    else
        if ( ( *psz && *psz != '\n' ) || *(psz-1) != '\n' )
            return TRUE;
    return FALSE;
}

BOOL CShwView::bParagraphStart( CRecPos* prps )
{
    if ( !bInParagraph( prps ) )
        return FALSE;

    bHome( *prps ); // goto beginning of line
    while ( !bAtParagraphStart( prps ) )
        {
        if ( !iUpLine( *prps ) )
            break;
        }

    return TRUE;
}

BOOL CShwView::bUpParagraph()
{
    if ( !bEditHome() )
        if ( !bMoveUp( TRUE ) ) // 1.4pa Fix bug of Ctrl Down moving into hidden field
            return FALSE; // must be at start of record

    while ( !bAtParagraphStart( &m_rpsCur ) )
        if ( !bMoveUp( TRUE ) ) // 1.4pa Fix bug of Ctrl Down moving into hidden field
            return FALSE; // must be at start of record

    return TRUE; // success
}

BOOL CShwView::bDnParagraph()
{
    do  {
        if ( !bMoveDown( TRUE ) ) // 1.4pa Fix bug of Ctrl Down moving into hidden field
            return FALSE; // must be at end of record
        } while ( !bAtParagraphStart( &m_rpsCur ) );
    return TRUE; // success
}

static int s_iHeightChange; // adjusted in low level wrap routines and then used to recalculate scroll position

BOOL CShwView::bBreakLine( CLangEnc* plng, CDC* pDC, const char* pszLine, char** ppszLineEnd, CSize sizLine )
{
    BOOL bWrap = FALSE;
    char* pszLineEnd = *ppszLineEnd;
    char* pszWdEnd = pszLineEnd;
    int iMaxLineWidth = iMargin(); // get margin setting from document

    ASSERT(sizLine.cx > iMaxLineWidth); // that's why this function should be called!
	CSize sizPartLine = sizLine; // used to measure part to find right place to break
    while (sizPartLine.cx > iMaxLineWidth) // need to wrap this line
        {
        const char* psz = pszPrevWdEnd( pszLine, pszWdEnd );
        if (!psz) // we're down to first word in line - must be a big one!
            break;
        sizPartLine = plng->GetTextExtent( pDC, pszLine, psz - pszLine ); // measure line
//		if ( plng->bUnicodeLang() && plng->bRightToLeft() ) // 1.4nd Fix bug of not wrapping correctly on Unicode right to left // If right to left Unicode, compensate for it measuring the wrong end of the line
//			sizPartLine.cx = sizLine.cx - sizPartLine.cx; // 1.4nd
        pszWdEnd = (char*)psz; // move back and prepare to check another word if necessary
        bWrap = TRUE;
        }

    if (bWrap)
        {
        *pszWdEnd = '\n'; // change space to line break
        if ( !bAtParagraphEnd( pszLineEnd ) ) // if at end of field or paragraph
            *pszLineEnd = ' '; // convert line break to space
        else
            s_iHeightChange += sizLine.cy; // added a line to view
        *ppszLineEnd = pszWdEnd; // return new end of line to caller!
        return TRUE; // success
        }
    return FALSE; // couldn't wrap it! (must be one long word or very narrow margin)
}

BOOL CShwView::bGrowLine( CLangEnc* plng, CDC* pDC, const char* pszLine, char** ppszLineEnd, CSize sizLine )
{
    int iSpaceWidth = plng->GetTextWidth( pDC, " ", 1 ); // measure a space
    BOOL bWrapped = FALSE;
    char* pszLineEnd = *ppszLineEnd;
	char* pszNextLine = pszLineEnd + 1;
	const char* pszNextLineEnd = pszFindLineEnd( pszNextLine );
    char* pszWdStart;
    int iMaxLineWidth = iMargin(); // get margin setting from document

    ASSERT(sizLine.cx < iMaxLineWidth); // that's why this function should be called!
//	int iNextLineLen = 0; // Needed to compensate for right-to-left Unicode measuring the wrong end of the line // 1.4nd Not needed
//	if ( plng->bUnicodeLang() && plng->bRightToLeft() ) // If right to left Unicode, measure full line
//		{
//		CSize siz = plng->GetTextExtent( pDC, pszNextLine, pszNextLineEnd - pszNextLine ); // measure line
//		iNextLineLen = siz.cx;
//		}
    while (TRUE)
        {              
        if ( bAtParagraphEnd( pszLineEnd ) ) // can't bring any more up
            break;
        pszWdStart = pszNextWdStart( pszLineEnd+1 ); // find end of first word and/or whitespace on next line
		CSize sizPartLine = plng->GetTextExtent( pDC, pszLineEnd+1, pszWdStart - ( pszLineEnd+1 ) ); // measure from line start to next word
//		if ( plng->bUnicodeLang() && plng->bRightToLeft() ) // 1.4nd Not needed // If right to left Unicode, compensate for measuring the wrong end
//			if ( pszWdStart != pszNextLineEnd ) // If at end of line, measurement is correct
//				sizPartLine.cx = iNextLineLen - sizPartLine.cx;
        if ( sizLine.cx + sizPartLine.cx < iMaxLineWidth ) // will this word fit on previous line?
            {
            *pszLineEnd = ' '; // replace \n with space
            if ( *pszWdStart && *pszWdStart != '\n' ) // don't bother if at end of field or already a line break
                {
                *(pszWdStart-1) = '\n'; // set new line break;
                pszLineEnd = pszWdStart-1;
                }
            else
                {
                s_iHeightChange -= sizLine.cy; // we just deleted a line!
                pszLineEnd = pszWdStart;
                sizPartLine.cx += iSpaceWidth; // we're really adding a space - it just looks like a \n!
                }
            sizLine.cx += sizPartLine.cx;
            bWrapped = TRUE;
            }
        else
            break;
        }

    *ppszLineEnd = pszLineEnd; // return new end of line to caller
    return bWrapped;
}

BOOL CShwView::bWrapParagraph( CRecPos rps )
{
    BOOL bChange = FALSE;
    int iMaxLineWidth = iMargin(); // get margin setting from document
    
    if ( rps.bInterlinear() )
        {
        bCombineBundles( m_rpsCur ); // pull interlinear bundles together
        m_rpsCur = rpsWrapInterlinear( m_rpsCur ); // Break interlinear lines as necessary
        return TRUE;
        }

    if ( !bParagraphStart( &rps ) ) // find start of paragraph
        return FALSE; // just return if not on a paragraph

    CDC dc;
    dc.CreateCompatibleDC( NULL ); // Create a temporary screen device context for measuring font
    dc.SelectObject( (CFont*)rps.pfld->pmkr()->pfnt() ); // select font into device context

    const char* psz = rps.psz(); // starting point for wrap

    if (!psz)
        return FALSE; // nothing to reshape

    const char* pszLine = pszPrevLineBrk( *rps.pfld, psz ); // find start of line
    do  {
        const char* pszLineEnd = pszNextLineBrk(pszLine);
        if ( !(pszLineEnd > pszLine) )
            break;
        CSize sizLine = m_rpsCur.pfld->plng()->GetTextExtent( &dc, pszLine, pszLineEnd - pszLine ); // measure line
        if ( sizLine.cx > iMaxLineWidth && bBreakLine( m_rpsCur.pfld->plng(), &dc, pszLine, (char**)&pszLineEnd, sizLine ) ) // If need to break this line, do wrap
			{
            bChange = TRUE;
			s_iHeightChange += sizLine.cy; // added a line to view // 1.4whf Fix bug of cursor going below bottom of window on auto wrap
			}
        if ( sizLine.cx < iMaxLineWidth && bGrowLine( m_rpsCur.pfld->plng(), &dc, pszLine, (char**)&pszLineEnd, sizLine ) ) // If need to join lines, take stuff from next line and add to this line
            bChange = TRUE;

        pszLine = pszLineEnd+1;

        } while ( !bAtParagraphEnd( pszLine-1 ) );

    return bChange;
}

// 2000-09-07 TLB: Do auto-wrap or emergency line breaking to 1000-chars max
void CShwView::LineWrap()
{
    ASSERT( m_prelCur );
    ASSERT( !m_bBrowsing );

    if ( m_rpsCur.bInterlinear() ) // If this is an interlinear line that appears to be part of a bundle...
        return; // ... don't do any kind of wrapping.

    // If auto-wrapping feature is turned off or if this field isn't set to wrap, then only do wrapping
    // if line length exceeds absolute maximum of 1000 characters.
    if ( !Shw_pProject()->bAutoWrap() || m_rpsCur.pfld->pmkr()->bNoWordWrap() )
        {
        LineLengthCheck();
        }
    else
        bDoAutoWrap();
}

BOOL CShwView::bDoAutoWrap()
{
    s_iHeightChange = 0;
    BOOL bWrapped = bWrapParagraph( m_rpsCur );
    if ( s_iHeightChange ) // lines were added or deleted
        {
        m_lTotalHt += s_iHeightChange;
        SetScroll(FALSE); // reset scroll bars
        }
    if ( bWrapped )
        {
        m_iWhatChanged |= eMultipleLines;
        EditSetCaret( m_rpsCur.pfld, m_rpsCur.iChar ); // caret may have jumped
		ResynchCaretAndCursor( FALSE ); // 1.4whf Fix bug of cursor going below bottom of window on auto wrap  // 1.5.1kh Don't reset keyboard on auto wrap
        }
    return bWrapped;
}

void CShwView::EditReshape()
{
    ASSERT( m_prelCur && !m_bBrowsing );
    // 01/07/2000: TLB - Added bChanged to remember if anything actually changed. This
    // combined with call to GetDocument()->RecordModified below to fix a bug where joining
    // of two fields messed up a browse view of the same DB, when the browse view was
    // sorted by the joined fields.
    BOOL  bChanged = FALSE;

    if ( bSelecting( eAnyText ) ) // reshape all paragraphs in selection
        {
        CRecPos rpsEnd = rpsSelEnd();
        m_rpsCur = rpsSelBeg();
        CField* pfldEnd = NULL;
        if ( rpsEnd.bInterlinear() ) // this position can disappear if it's interlinear
            {
            if ( rpsEnd.bNextBundle() ) // get field to stop at
                pfldEnd = rpsEnd.pfld;
            else
                pfldEnd = rpsEnd.rpsEndOfBundle().pfldNext(); // get field following end of bundle (may be NULL)
            do  {
                if ( !m_rpsCur.pfld->pmkr()->bNoWordWrap() )
                    bChanged |= bWrapParagraph( m_rpsCur ); // reshape another paragraph
                } while ( bDnParagraph() && m_rpsCur.pfld != pfldEnd );
            }
        else
            { // end of selection is in non-interlinear field
            do  {
                if ( !m_rpsCur.pfld->pmkr()->bNoWordWrap() )
                    bChanged |= bWrapParagraph( m_rpsCur ); // reshape another paragraph
                } while ( bDnParagraph() && m_rpsCur <= rpsEnd );
            }
        ClearSelection(); // 01/07/2000: TLB - Moved up for below (no need to clear selection if there isn't one)
        }
    else // just wrap current paragraph
        {
        CRecPos rps = m_rpsCur;
        bChanged = bWrapParagraph( rps );
        }

    if ( !bChanged ) // 01/07/2000: TLB - If nothing changed, we're done.
        return;

    m_iWhatChanged |= eMultipleLines; // cause other views to redraw whole record
    if ( !bPositionValid( m_rpsUpperLeft ) ) // our frame of reference may have been deleted by another view
        m_rpsUpperLeft.SetPos( 0, m_rpsCur.pfld ); // set to field containing current position
    m_lVPosUL = -pntPosToPnt( CRecPos( 0, m_prelCur->prec()->pfldFirst(), m_prelCur->prec() ), TRUE ).y;
    HideCaret();
    SetScroll(TRUE);
	int iCharSave = m_rpsCur.iChar; // 5.96t Remember iChar because it gets changed by temporary move below
    EditSetCaret( m_rpsCur.pfld, 0 ); // ALB 5.96t Temporarily set to fix bug of scroll staying off to the left
	m_rpsCur.iChar = iCharSave; // Restore iChar after temporary change
    EditSetCaret( m_rpsCur.pfld, m_rpsCur.iChar ); // caret may have jumped
    SetCaretPosAdj(); // Move caret
    ShowCaret();
    GetDocument()->RecordModified( m_prelCur ); // 01/07/2000: TLB - let everyone know we modified this record
    SetModifiedFlag();
    ResetUndo(); // clear undo buffer
}

void CShwView::EditReshapeDocument()
{
    ASSERT( m_prelCur && !m_bBrowsing );
    if ( !GetDocument()->bValidateViews() )
        return;

    CIndex* pind = m_pind->pindsetMyOwner()->pindRecordOwner(); // use owning index to march thru database
    
    CProgressIndicator prg(pind->lGetCount(), NULL, sUTF8( GetDocument()->GetPathName() ), TRUE); // 1.4ad Eliminate resource messages sent to progress bar // 1.4qub Upgrade GetPathName for Unicode build
    long int lRec=0;
    for ( CNumberedRecElPtr prel = pind->pnrlFirst(); prel; prel = pind->pnrlNext( prel ), lRec++ )
        {
        if ( !prg.bUpdateProgress(lRec) ) // 1.2gr
            break;
        BOOL bChanged = FALSE;
        CRecord* prec = prel->prec();
        ASSERT( prec );
        m_rpsCur.SetPos( 0, prec->pfldFirst(), prec );
        do  {
            if ( !m_rpsCur.pfld->pmkr()->bNoWordWrap() )
                bChanged |= bWrapParagraph( m_rpsCur ); // reshape all paragraphs in this record
            } while ( bDnParagraph() );
        if ( bChanged )
            GetDocument()->RecordModified( prel ); // let everyone know we modified this record
        }

//    GetDocument()->SetModifiedFlag(TRUE); // TLB 02-29-2000 : Can you believe I actually made this change
                                            // on the second century-break leap day of all history? This line of
                                            // code is unnecessary (and bad actually) because RecordModified
                                            // already takes care of this, and having it in can cause the DB to
                                            // think it needs to be saved even when nothing changed.
    SetCur(m_prelCur);
}

BOOL CRecPos::bBundleSameAs( CRecPos rps2 ) const // return TRUE if bundles have same fields in same order
{
    CRecPos rps1 = *this;
    // assume rps1 and rps2 are both at start of bundles
    ASSERT( rps1.pfld != rps2.pfld ); // should be different bundles, too
    while (TRUE)
        {
        if ( rps1.pfld->pmkr() != rps2.pfld->pmkr() ) // fields should be based on same marker
            return FALSE;
        BOOL bLast1 = rps1.bLastInBundle();
        BOOL bLast2 = rps2.bLastInBundle();
        if ( bLast1 || bLast2 )
            return bLast1 == bLast2; // return TRUE only if both are at end of bundle
        rps1.pfld = rps1.pfldNext();
        rps2.pfld = rps2.pfldNext();
        ASSERT( rps1.bInterlinear() );
        ASSERT( rps2.bInterlinear() );
        }
}

BOOL CRecPos::bPrevBundle() // move to beginning of previous bundle, return TRUE if successful
{
    CRecPos rps = *this;
    while ( rps.pfld && rps.bInterlinear() && !rps.bFirstInterlinear() )
        rps.pfld = rps.pfldPrev();
    if ( !rps.pfld || !rps.bInterlinear() )
        return FALSE; // couldn't even find start of current bundle. probably not possible???

    while ( rps.pfld && rps.bInterlinear() ) // now look for start of previous bundle
        {
        rps.pfld = rps.pfldPrev();
        if ( rps.pfld && rps.bFirstInterlinear() )
            {
            SetPos( 0, rps.pfld ); // return start of new bundle
            return TRUE;
            }
        }
    return FALSE;
}

BOOL CRecPos::bNextBundle() // move to beginning of next bundle, return TRUE if successful
{
    CRecPos rps = *this;
    while ( rps.pfld && rps.bInterlinear() )
        {
        rps.pfld = rps.pfldNext();
        if ( rps.pfld && rps.bFirstInterlinear() )
            {
            SetPos( 0, rps.pfld ); // return start of new bundle
            return TRUE;
            }
        }
    return FALSE;
}

int CRecPos::iFldLenWithoutNL() const // return length of field minus ending nl
{
    int iLen = pfld->GetLength();
    if ( iLen && pfld->GetChar(iLen-1) == '\n' ) // don't count newline
        iLen--;
    return iLen;
}

int CRecPos::iBundleLength() const // return length of longest line in bundle, minus ending nl
{
    CRecPos rps = rpsFirstInBundle();
    int iBundleLen=0; // get length of bundle
    for ( ; ; rps.pfld = rps.pfldNext() )
        {
        iBundleLen = max( iBundleLen, rps.iFldLenWithoutNL() ); // find longest line in this bundle
        if ( rps.bLastInBundle() )
            break;
        }
    return iBundleLen;
}

BOOL CShwView::bCombineBundles( CRecPos& rpsCur, BOOL bMult ) // pull interlinear bundles together in preparation for wrapping
{
    ASSERT( rpsCur.bInterlinear() );
    BOOL bModified = FALSE;

    CRecPos rpsBeg = rpsCur.rpsFirstInBundle();
    CRecPos rps = rpsBeg;
    if ( bMult ) // If multiple, move up to first in bundle set
        for ( ; rps.bPrevBundle(); rpsBeg = rps ) // find first bundle identical in structure to this one
            if ( !rps.bBundleSameAs( rpsBeg ) )
                break;

#ifdef NOTYET_COMBINE_MORPH_FIELDS // 1.4qpj Start combine morph fields on reshape, but not complete
	CMarker* pmkrLastInBundle = pdoc()->pintprclst()->pintprcLast()->pmkrTo(); // 1.4qpj
	CRecPos rpsMorphBundleStart = rpsBeg; // 1.4qpj See if morph bundle is next, after last bundle field, more interlinear comes next
	while ( rpsMorphBundleStart.pfldNext() ) // 1.4qpj Find possible morph bundle start
		{
		if ( rpsMorphBundleStart.pmkr == pmkrLastInBundle )
		rpsMorphBundleStart.NextField();
		}
    CRecPos rpsFirstBundleMorphs = rpsBeg.rpsFirstInBundle(); // 1.4qpj Make reshape pull morphs together after XML import
	rpsMorphBundleStart.NextField(); // 1.4qpj Look at next field
	BOOL bMorphBundleNext = FALSE;
	if ( rpsMorphBundleStart.bInterlinear() && !rpsMorphBundleStart.bFirstInterlinear() ) // 1.4qpj If interlin, but not first interlin, see if it is morphs
		{
		rpsFirstBundleMorphs.NextField(); // 1.4qpj Start looking at second field
		while ( TRUE ) // 1.4qpj Look through fields in first bundle for first field of second bundle
			{
			if ( rpsFirstBundleMorphs.pfld->pmkr() == rpsMorphBundleStart.pfld->pmkr() ) // 1.4qpj If found in bundle
				{
				bMorphBundleNext = TRUE; // 1.4qpj This is a possible morph bundle
				break;
				}
			if ( rpsFirstBundleMorphs.bLastInBundle() ) // 1.4qpj If end of bundle, don't look further
				break;
			rpsFirstBundleMorphs.NextField(); // 1.4qpj
			}
		if ( bMorphBundleNext ) // 1.4qpj If possible morph bundle, see if all fields match
			{
			bMorphBundleNext = FALSE; // 1.4qpj Not morph bundle next unless all fields match
			CRecPos rpsFirstBundleField = rpsFirstBundleMorphs; // 1.4qpj
			CRecPos rpsMorphBundleField = rpsMorphBundleStart; // 1.4qpj
			while ( rpsFirstBundleField.pfld->pmkr() == rpsMorphBundleField.pfld->pmkr() )
				{
				if ( rpsFirstBundleField.bLastInBundle() ) // 1.4qpj If end of bundle, they all matched
					{
					bMorphBundleNext = TRUE; // 1.4qpj
					break;
					}
				rpsFirstBundleField.NextField(); // 1.4qpj
				rpsMorphBundleField.NextField(); // 1.4qpj
				}
			}
		if ( bMorphBundleNext ) // 1.4qpj If morph bundle next, join it to first bundle
			{
			bModified = TRUE; // 1.4qpj This record has been modified
			CRecPos rpsFirstBundleField = rpsFirstBundleMorphs; // 1.4qpj
			int iBundleLen=0; // get length of bundle
			while ( TRUE )
				{
				iBundleLen = max( iBundleLen, rpsFirstBundleField.iFldLenWithoutNL() ); // find longest line in this bundle
				if ( rpsFirstBundleField.bLastInBundle() )
					break;
				rpsFirstBundleField.NextField(); // 1.4qpj
				}
			iBundleLen += 1; // 1.4qpj Add one for space separator
			rpsFirstBundleField = rpsFirstBundleMorphs; // 1.4qpj
			CRecPos rpsMorphBundleField = rpsMorphBundleStart; // 1.4qpj
			while ( TRUE ) // 1.4qpj For each field in morph bundle, join it
				{
				int iLen = rpsFirstBundleField.iFldLenWithoutNL();
				rpsFirstBundleField.pfld->SetContent( rpsFirstBundleField.pfld->Left( iLen ) ); // 1.4qzfr Upgrade GetBuffr for Unicode build // Cut off nl
				while ( rpsFirstBundleField.pfld->GetLength() < iBundleLen ) // 1.4qzfr Add spaces up to desired length
					rpsFirstBundleField.pfld->Append( " " ); // 1.4qzfr // 1.4qzkb
//				char* buf = rpsFirstBundleField.pfld->GetBuffr(iBundleLen); // force length to same as rest // 1.4qzfr Eliminate GetBuffr
//				memset( buf+iLen, ' ', iBundleLen-iLen ); // pad with spaces // 1.4qzfr
//				rpsFirstBundleField.pfld->ReleaseBuffer(iBundleLen); // 1.4qzfr
				*rpsFirstBundleField.pfld += *rpsMorphBundleField.pfld; // tack onto previous bundle
				CField* pfldNext = rpsMorphBundleField.pfldNext();
				m_pind->pindsetMyOwner()->DeleteField( m_prelCur, rpsMorphBundleField.pfld ); // Delete field
	            rpsMorphBundleField.pfld = pfldNext;

				if ( rpsFirstBundleField.bLastInBundle() ) // 1.4qpj If end of bundle, done joining
					break;
				rpsFirstBundleField.NextField(); // 1.4qpj
				rpsMorphBundleField.NextField(); // 1.4qpj
				}
			}
		}
#endif // NOTYET_COMBINE_MORPH_FIELDS // 1.4qpj Start combine morph fields on reshape, but not complete

    rps = rpsBeg.rpsFirstInBundle();
    for ( ; rps.bNextBundle() && rps.bBundleSameAs( rpsBeg ); rps = rpsBeg )
        {
        bModified = TRUE;
        int iBundleLen = rpsBeg.iBundleLength() + 1; // get length of longest line in first bundle, add 1 for space

        CField* pfldLast = rpsBeg.rpsEndOfBundle().pfld;
        CRecPos rpsB = rpsBeg;
        for ( ; ; rpsB.pfld = rpsB.pfldNext() )
            {
            ASSERT( rpsB.pfld );
            ASSERT( rps.pfld );
            int iLen = rpsB.iFldLenWithoutNL(); 
			rpsB.pfld->SetContent( rpsB.pfld->Left( iLen ) ); // 1.4qzfr Upgrade GetBuffr for Unicode build // Cut off nl
			while ( rpsB.pfld->GetLength() < iBundleLen ) // 1.4qzfr Add spaces up to desired length
				rpsB.pfld->Append( " " ); // 1.4qzfr // 1.4qzkb
//            char* buf = rpsB.pfld->GetBuffr(iBundleLen); // force length to same as rest // 1.4qzfr Eliminate GetBuffr
//            memset( buf+iLen, ' ', iBundleLen-iLen ); // pad with spaces // 1.4qzfr
//            rpsB.pfld->ReleaseBuffer(iBundleLen); // 1.4qzfr
            *rpsB.pfld += *rps.pfld; // tack onto previous bundle
            CField* pfldNext = rps.pfldNext();
            m_pind->pindsetMyOwner()->DeleteField( m_prelCur, rps.pfld ); // Delete field
            rps.pfld = pfldNext;

            if ( rpsB.pfld == pfldLast )
                break;
            }
        if ( !bMult ) // If not multiple, stop after first join
            break;
        }
    if ( bMult ) // If multiple, set cursor to end of bundle
        rpsCur = rpsBeg.rpsEndOfBundle();
    return bModified;
}

CRecPos CShwView::rpsWrapInterlinear( CRecPos rpsPar, BOOL bJoin ) // Break interlinear as necessary
{
    CRecPos rps = rpsPar.rpsFirstInBundle( 0 );    

    if ( rps.pfld->pmkr()->bNoWordWrap() )
        return rpsPar;

    BOOL bChange = FALSE;
    int iMaxLineWidth = iMargin(); // get margin setting from document
	if ( rps.pfld->plng()->bRightToLeft() ) // 1.5.1da Fix problem of rtl interlinear wrap too wide
		iMaxLineWidth -= m_iFldStartCol; // 1.5.1da Narrow wrap by field start width
	if ( iMaxLineWidth <= 0 ) // 1.0dg Fix bug of hang if wrap margin set to zero
		iMaxLineWidth = 1;
    
    int iBundleLen=0;
    for ( CRecPos rpsTemp = rps.rpsFirstInBundle(); ; rpsTemp.pfld = rpsTemp.pfldNext() )
        {
        if ( rpsTemp.pfld->pmkr() == ptyp()->pmkrRecord() ) // just to be safe!
            return rpsPar;
        if ( rpsTemp.pfld->GetLength() > iBundleLen ) // If line is longer...
            iBundleLen = rpsTemp.pfld->GetLength(); // remember it's length
        if ( rpsTemp.bLastInBundle() )
            break;
        }
	RememberInterlinearWidths( TRUE ); // 6.0j Speed up measure during wrap
	int iPixelBundleWidth = iInterlinPixelPos( rps, iBundleLen ); // 6.0b Make wrap use new interlinear measurement system

    while ( iPixelBundleWidth > iMaxLineWidth ) // while line wider than margin, break it
        {
        CRecPos rpsWd = rps;
        CRecPos rpsPrevWd = rps;
        CRecPos rpsStart = rps;
		int iLineLen = rps.pfld->GetLength();
		do // Find furthest place that still fits in margin, rpsPrevWd will have it
			{
			rpsPrevWd = rpsWd; // Remember starting place
			rpsWd.iChar++; // Step forward into word
			while ( rpsWd.iChar < iLineLen && !rpsWd.bAtWdBegInBundle() ) // look for next possible bundle break place
				rpsWd.iChar++;
			iPixelBundleWidth = iInterlinPixelPos( rpsWd, rpsWd.iChar ); // See if this place is beyond margin
			} while ( rpsWd.iChar < iLineLen && iPixelBundleWidth < iMaxLineWidth );
		if ( rpsPrevWd.iChar == rpsStart.iChar ) // If the first possible bundle break place is past the margin, break at it
			{
			rpsPrevWd = rpsWd;
			if ( rpsWd.bEndOfField() ) // 1.4qps If we are at the end of the line, don't break
				break; // 1.4qps Fix bug of interlinear wrap wrong on very narrow wrap 
			}
        bChange = TRUE;
		RememberInterlinearWidths( FALSE ); // 6.0j Speed up measure during wrap
		rps.BreakBundle( rpsPrevWd.iChar );
		RememberInterlinearWidths( TRUE ); // 6.0j Speed up measure during wrap
		iBundleLen=0; // Find longest in bundle, for measuring total length
		for ( CRecPos rpsTemp = rps.rpsFirstInBundle(); ; rpsTemp.pfld = rpsTemp.pfldNext() )
			{
			if ( rpsTemp.pfld->GetLength() > iBundleLen ) // If line is longer...
				iBundleLen = rpsTemp.pfld->GetLength(); // remember it's length
			if ( rpsTemp.bLastInBundle() )
				break;
			}
		iPixelBundleWidth = iInterlinPixelPos( rps, iBundleLen ); // Calculate width of rest after break
		}
		
    if ( bChange )
        {
        CRecPos rpsEnd = rps.rpsEndOfBundle(); // leave with rps at end of bundle just processed
        if ( !rpsEnd.pfld->GetLength() || *( rpsEnd.psz() - 1 ) != '\n' )
            {
            *rpsEnd.pfld += "\n"; // add line break at end of bundle
            rpsEnd.iChar++;
            }
        return rpsEnd;
        }
    else
        return rpsPar;
}

