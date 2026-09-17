// shwvundo.cpp : implementation of the undo parts of the CShwView class
//

#include "stdafx.h"
#include "toolbox.h"
#include "shw.h"

#include "mainfrm.h"
#include "shwdoc.h"
#include "shwview.h"
#include "project.h"
#include "kmn.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


void CUndoList::Add( CRecPos rpsBegin, CRecPos rpsEnd ) // stuff inserted between rpsBegin and rpsEnd
{
	if ( rpsBegin >= rpsEnd ) // 1.0bt Fix bug on obscure case of user typing in a hidden field marker on Edit, Insert field
		return;
#ifdef _DEBUG
	ASSERT( rpsBegin < rpsEnd ); // debug only, this can be a bit time consuming
#endif
#ifdef IF_YOU_WANT_TO_CLEAR_UNDO_BUFFER_WHEN_ADDING_NEW_EDIT_ACTION_AFTER_AN_UNDO
	if ( m_bUsed )
		{
		DeleteAll();
		m_bUsed = FALSE;
		}
#endif
	long int lFieldNum=0;
	CField* pfld = rpsBegin.pfld;
	while ( pfld = rpsBegin.prec->pfldPrev( pfld ) ) // get location of this field into record
		lFieldNum++;
	int iChar = rpsBegin.iChar;
	long int lDeleteCount = 0;
	while ( rpsBegin.pfld != rpsEnd.pfld ) // count number of chars that would need to be deleted
		{
		lDeleteCount += rpsBegin.pfld->GetLength() - rpsBegin.iChar;
		lDeleteCount++; // count marker as 1
		rpsBegin.NextField(0); // advance to next field
		}
	lDeleteCount += rpsEnd.iChar - rpsBegin.iChar;
	CDblList::Add( new CUndo( lFieldNum, iChar, lDeleteCount ) ); // add to list
}

void CUndoList::Add( CRecPos rps, const char* pszText ) // string deleted at rps
{
#ifdef IF_YOU_WANT_TO_CLEAR_UNDO_BUFFER_WHEN_ADDING_NEW_EDIT_ACTION_AFTER_AN_UNDO
	if ( m_bUsed )
		{
		DeleteAll();
		m_bUsed = FALSE;
		}
#endif
	if ( lGetCount() > 100 ) // me thinks we need some reasonable limit
		CDblList::Delete( pelFirst() ); // get rid of oldest action

	long int lFieldNum=0;
	CField* pfld = rps.pfld;
	while ( pfld = rps.prec->pfldPrev( pfld ) ) // get location of this field into record
		lFieldNum++;
    CDblList::Add( new CUndo( lFieldNum, rps.iChar, 0, pszText ) ); // add to list
}

void CUndoList::Add( CRecPos rps, char c ) // single char deleted at rps
{
	char caBuf[2];
	caBuf[0] = c;
	caBuf[1] = '\0';
	Add( rps, caBuf );
}

CUndo* CUndoList::pundGetUndo() // get item to be undone from list, removes item from list, caller needs to delete!
{
	m_bUsed = TRUE;
	return (CUndo*)pelRemove( pelLast() );
}

BOOL CShwView::bEditUndo()
{
	ASSERT( !m_undlst.bIsEmpty() );
	CUndo* pund = m_undlst.pundGetUndo(); // get last action to be undone
	ClearSelection();
	if ( pund->bIsInsert() )
		{
		m_rpsCur.SetPos( 0, m_rpsCur.prec->pfldFirst() );
		for ( long int l = 0; l < pund->lFieldNum(); l++ ) // move to correct field
			{
			m_rpsCur.NextField();
			if ( m_rpsCur.pfld == m_rpsCur.prec->pfldLast() )
				break;
			}
		m_rpsCur.SetPos( min( m_rpsCur.pfld->GetLength(), pund->iChar() ) ); // move to place where it happened in field
		m_prelCur->prec()->Paste( &m_rpsCur, pund->pszText(), GetDocument()->pmkrset() ); // insert into record at current caret position
		}
	else // something was inserted, now we need to delete it
		{
		m_rpsCur.SetPos( 0, m_rpsCur.prec->pfldFirst() );
		for ( long int l = 0; l < pund->lFieldNum(); l++ ) // move to correct field
			{
			m_rpsCur.NextField();
			if ( m_rpsCur.pfld == m_rpsCur.prec->pfldLast() )
				break;
			}
		m_rpsCur.SetPos( min( m_rpsCur.pfld->GetLength(), pund->iChar() ) ); // move to place where it happened in field
		StartSelection( eText );
		long int lDelCnt = pund->lDeleteCount();
		for ( int iLen = m_rpsCur.pfld->GetLength() - m_rpsCur.iChar; lDelCnt > iLen; )
			{
			m_rpsCur.NextField(0);
			if ( m_rpsCur.pfld == m_rpsCur.prec->pfldLast() )
				break;
			lDelCnt -= iLen+1;
			iLen = m_rpsCur.pfld->GetLength();
			}
		m_rpsCur.SetPos( min( m_rpsCur.pfld->GetLength(), m_rpsCur.iChar + (int)lDelCnt ) ); // select correct number of chars
		DeleteSelection(FALSE);
		}
    GetDocument()->SetModifiedFlag(TRUE); // in case it was cleared in File-Save, etc.
    EditSetCaret( m_rpsCur.pfld, m_rpsCur.iChar );
    SetScroll( TRUE );
    SetCaretPosAdj();
	delete pund;
    LineWrap(); // 2000-09-07 TLB: Do auto-wrap or emergency line breaking to 1000-chars max
    return TRUE;
}

BOOL CShwView::bEditUndoAll() // undo all changes since last SetCur
{
    if ( AfxMessageBox(_("Are you sure you want to undo ALL changes to this record?"), MB_YESNO | MB_ICONQUESTION) != IDYES )
        return FALSE;

    CRecord* prec = m_prelCur->prec();
    CField* pfld = prec->pfldFirst();
    pfld->Empty(); // clear key field
    // delete all fields after record key field
    for ( CField* pfldT = prec->pfldNext( pfld ); pfldT; )
        {
        CField* pfldNext = prec->pfldNext( pfldT );
        m_pind->pindsetMyOwner()->DeleteField( m_prelCur, pfldT ); // Delete field
        pfldT = pfldNext;
        }

    const char* p = m_pszUndoAllBuffer;
    while (TRUE)
        {
        int iLen = strlen(p); // length of field contents in undo buffer
        *pfld += Str8( p, iLen ); // add to field
        p = p + iLen + 1;

        if ( !*p ) // no more fields in undo buffer
            break;

        CMarker* pmkr = GetDocument()->pmkrset()->pmkrSearch( p );
        if (!pmkr) // marker doesn't currently exist, user must have deleted it
            pmkr = GetDocument()->pmkrset()->pmkrAdd_MarkAsNew( p );
        ASSERT(pmkr);
        if ( pmkr != prec->pfldFirst()->pmkr() ) // can't insert record marker!
            {
            CField* pfldNew = new CField( pmkr ); // create new field to insert
            prec->InsertAfter( pfld, pfldNew );
            pfld = pfldNew;
            }
        p += strlen(p) + 1; // move past marker
        }

    ClearSelection();
    GetDocument()->SetModifiedFlag(TRUE); // in case it was cleared in File-Save, etc.
    GetDocument()->RecordModified( m_prelCur, FALSE );
    m_bModifiedSinceValidated = m_bCanUndoAll = FALSE;
    m_rpsCur = m_rpsUpperLeft;
    EditSetCaret( m_rpsCur.pfld, m_rpsCur.iChar );
    SetScroll( TRUE );
    SetCaretPosAdj();
    return TRUE;
}

void CShwView::CopyToUndoAllBuffer() // copy contents of current record to undo buffer
{
    if ( m_prelCur && m_prelCur->prec() == m_precUndoAll )
        return; // SetCur() didn't really move records, don't do anything

    m_bCanUndoAll = FALSE;
    if ( m_pszUndoAllBuffer )
        {
        delete m_pszUndoAllBuffer;
        m_pszUndoAllBuffer = NULL;
        }

    if ( !m_prelCur )
        return;

    m_precUndoAll = m_prelCur->prec();
    CRecord* prec = m_prelCur->prec();
    CField* pfld = prec->pfldFirst();
    long int lRecordTextLen = -pfld->pmkr()->sMarker().GetLength();
    for ( ; pfld; pfld = prec->pfldNext( pfld ) )
        {
        lRecordTextLen += pfld->pmkr()->sMarker().GetLength();
        lRecordTextLen += pfld->GetLength();
        lRecordTextLen += 2; // 2 nulls
        }
//    if ( lRecordTextLen > 32000 ) // 1.4pw Removed
//        return; // undo won't work on this record // 1.4pw Removed

    m_pszUndoAllBuffer = new char[lRecordTextLen]; // allocate appropriately sized buffer

    char* p = m_pszUndoAllBuffer;
    for ( pfld = prec->pfldFirst(); pfld; )
        {
        int iLen = pfld->GetLength(); // length of field contents
        memcpy( p, (const char*)*pfld, iLen ); // copy marker name
        p += iLen;
        *p++ = '\0'; // null terminate
        pfld = prec->pfldNext( pfld ); // goto next field
        if ( pfld )
            {
            iLen = pfld->pmkr()->sMarker().GetLength(); // length of marker
            memcpy( p, (const char*)pfld->pmkr()->sMarker(), iLen ); // copy marker name
            p += iLen;
            *p++ = '\0'; // null terminate
            }
        else
            *p++ = '\0'; // final terminator
        }
    ASSERT( p-m_pszUndoAllBuffer == lRecordTextLen );
	ResetUndo(); // reset normal undo
}

