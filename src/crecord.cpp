// crecord.cpp Implementation of Standard format record (1995-02-28)

#include "stdafx.h"
#include "toolbox.h"
#include "crecord.h"
#include "crecpos.h"
#include "ind.h"  // CIndex::pfldSubstituteForNullPriKey()
#include "date.h"  // CDateCon::s_DateStamp, s_bConvertDateField
#include "resource.h"
#include "cck_d.h"
#include "crngst_d.h"
#include "spath_d.h"

#include "shwview.h"

#ifdef _DEBUG
    #undef THIS_FILE
    static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#ifdef TEST_BUILD
    #undef ASSERT
    #define ASSERT(f) ((void)0)
#endif

CField* CFieldList::pfldFirstWithMarker(const CMarker* pmkr) const  // Find first field with a specified marker
{
    CField* pfld = pfldFirst();
    for ( ; pfld && pfld->pmkr() != pmkr; pfld = pfldNext(pfld))
        ;
    return pfld;
}

        // Delete element
void CFieldList::Delete( CField* pfld )
{
    ASSERT( !bIsFirst( pfld) ); // Don't delete key
    CDblList::Delete( (CDblListEl*) pfld );
}

void CFieldList::MoveFieldsFrom(CFieldList* pfldlst)
{
    ASSERT( pfldlst );
    ASSERT( pfldlst != this );
    pfldlst->MoveElementsTo(this);
}

void CFieldList::EliminateShorter() // Eliminate shorter analyses for disambiguation
{
    for ( CField* pfld = pfldFirst(); pfld && pfldNext( pfld ); pfld = pfldNext( pfld ) ) // For each field except the last
        if ( pfldNext( pfld )->GetLength() < pfld->GetLength() ) // If next is shorter, delete the rest (list is longest first
            {
            DeleteRest( pfldNext( pfld ) );
            break;
            }
}

void CFieldList::EliminateDuplicates( BOOL bTimeLimit ) // Eliminate duplicates for disambiguation
{
	if ( !pfldNext( pfldFirst() ) ) // 1.5.1ng If only one field, don't remove spaces
		return; // 1.5.1ng 
	CField* pfld1 = pfldFirst();
    for ( ; pfld1; pfld1 = pfldNext( pfld1 ) ) // For each field // 1.3br Remove extra spaces before checking for duplicates
		{
		Str8 s = pfld1->sContents();
		RemoveExtraSpaces( s );
		pfld1->SetContent( s );
		}
    for ( pfld1 = pfldFirst(); pfld1; pfld1 = pfldNext( pfld1 ) ) // For each field
		{
        for ( CField* pfld2 = pfldNext( pfld1 ); pfld2; ) // For each other field later in list
            if ( !strcmp( pfld1->sContents(), pfld2->sContents() ) ) // If duplicate, delete
                pfld2 = pfldDelete( pfld2 ); 
            else
                pfld2 = pfldNext( pfld2 ); 
		if ( bTimeLimit && bTimeLimitExceeded() ) // 1.3bm Do time limit during eliminate duplicates
			break;
		}
}

// --------------------------------------------------------------------------

// Construct a record consisting of a key field.
// NOTE NOTE NOTE: The record now "owns" this field and upon destruction
// will delete it.
CRecord::CRecord(CField* pfldKey) : CFieldList()
{
    ASSERT(pfldKey != NULL);
    Add(pfldKey);
}

const Str8& CRecord::sKey() const
{
    CField* pfldKey = pfldFirst();
    ASSERT(pfldKey != NULL);
    return *pfldKey;
}

        /* Doc and comments by AB 6-8-96
        Finds the first field within the hierarchy that is under or over or level with the key field pfldPriKey.
        Clarifying examples:
        Sample record:
            \w ball
                \s 1
                    \p n
                        \g spherical toy
                            \i The pitcher threw the ball.
                \s 2
                    \p n
                        \g party
                            \i The princess went to the ball.
                        \g fun
                            \i She had a ball.
                            \t Se gustaba mucho.
        Examples:
        mkr     pfldPriKey  Return
         g          none            \g spherical toy        If no key, just return the first in the record  
         g          \g party        \g party                    If the desired marker is the same as the key, return it
         g          \s 2              \g party                     If the key is over the marker, search forward within markers under the key 
         g          \i She had a ball.  \g fun              If the marker is over the key, search backward within markers at same level or over the key
         i           \t Se gustaba mucho.   \i She had a ball.      Else (marker at same level as key) search backward for lowest field over both and then forward within markers under that field
        */
CField* CRecord::pfldFirstInSubRecord(const CMarker* pmkr,
        CField* pfldPriKey) const
{
    ASSERT( pmkr );
    if ( !pfldPriKey ) // If no key, just return the first in the record
        return pfldFirstWithMarker(pmkr);
        
    const CMarker* pmkrPriKey = pfldPriKey->pmkr();
    if ( pmkrPriKey == pmkr ) // If the desired marker is the same as the key, return it
        {
        // 03/01/2000 TLB : If the primary sort field has been deleted from the record, return NULL and let the
        // caller deal with it.
        if ( bIsMember(pfldPriKey) )
            return pfldPriKey;
        else
            return NULL;
        }

    if ( !pmkr->pmkrOverThis() || !pmkrPriKey->pmkrOverThis() ) // If no marker above the desired one or above the key (no hierarchy, or one of them is the record marker) just return first in record
        return pfldFirstWithMarker(pmkr);
        
    if ( pmkrPriKey->bOver(pmkr) ) // If the key is over the marker, search forward within markers under the key
        {
        CField* pfld = pfldNext(pfldPriKey);        
        for ( ; pfld && pmkrPriKey->bOver(pfld->pmkr()); pfld = pfldNext(pfld) )
            if ( pfld->pmkr() == pmkr )
                return pfld;
        }
    else if ( pmkrPriKey->bUnder(pmkr) ) // If the marker is over the key, search backward within markers at same level or over the key 
        {
        CField* pfld = pfldPrev(pfldPriKey);        
        for ( ; pfld && !pfld->pmkr()->bAtHigherLevelThan(pmkr); pfld = pfldPrev(pfld) )
            if ( pfld->pmkr() == pmkr )
                return pfld;
        }
    else // Else (marker at same level as key) search backward for lowest field over both and then forward within markers under that field 
        {
        const CMarker* pmkrOver = pmkrPriKey->pmkrLowestOverBoth(pmkr); // Find lowest marker over both
        if ( !pmkrOver )  // 1996-07-26 MRP
            return pfldFirstWithMarker(pmkr);
            
        CField* pfld = pfldPrev(pfldPriKey);        
        for ( ; pfld; pfld = pfldPrev(pfld) ) // Search back to lowest field over both
            if ( pfld->pmkr() == pmkrOver || pfld->pmkr()->bOver(pmkrOver) )
                break;

        // 03/01/2000 TLB : Replaced ASSERT with the following. If there was no field found in subrecord, just
        //                  return NULL and let the caller handle it.
        if ( !pfld )
            return NULL;

        pmkrOver = pfld->pmkr();
        pfld = pfldNext(pfld); // Search forward within markers under the lowest over both
        for ( ; pfld && !pfld->pmkr()->bAtHigherLevelThan(pmkrOver); pfld = pfldNext(pfld) )
            if ( pfld->pmkr() == pmkr )
                return pfld;
        }
        
    return NULL;
}

// insert template or clipboard contents into record at specified position
//------------------------------------------------------
void CRecord::Paste( CRecPos* prps, Str8 sStr, CMarkerSet* pmkrset )
{
    CField* pfld = prps->pfld;
    Str8 sTail = pfld->Mid( prps->iChar ); // Save tail part
    pfld->ReleaseBuffer( prps->iChar ); // Cut off tail part
    BOOL bAddedNewMkr = FALSE;

    if ( sStr.GetLength() && sStr[0] == '\\' ) // is first thing a marker?
        sStr = pNL + sStr; // insert nl into beginning of clipboard contents

    const char* p;
    for ( const char* s = p = sStr; p; s = p+1 )
        {
#ifdef _MAC
        p = strstr( s, "\n\\" );    // search for marker
#else
        p = strstr( s, pNL );    // replace \r\n with just \n
#endif
        int len;
        if ( !p )
            len = strlen( s );  // no more \r\n found, just copy rest of string
        else
            len = p - s;
        if ( !bValidFldLen( pfld->GetLength() + len ) ) // don't overstuff Str8!
            break;
        *pfld += Str8( s, len ); // add to field contents
#ifndef _MAC
        if ( p && *(p+2) == '\\' ) // found a marker!
            {
#endif
            const char* pszMkr = p+strlen(pNL)+1; // skip nl and backslash
            len = strcspn( pszMkr, " \t\r\n" ); // BJY NOTE!!! This should change to something
                                                    // a little more portable???
            if ( !len )
                continue;
            Str8 sMkr( pszMkr, len );
            CMarker* pmkr = pmkrset->pmkrSearch( sMkr );
            if (!pmkr) // if marker doesn't currently exist
                {
                pmkr = pmkrset->pmkrAdd_MarkAsNew( sMkr );
                bAddedNewMkr = TRUE;
                }
            ASSERT(pmkr);
            if ( pmkr == pfldFirst()->pmkr() ) // can't insert record marker!
                {
                if ( pfld != pfldFirst() ) // don't put in record marker as text if we're inserting into first field
                    {
                    *pfld += "\n\\" + sMkr + " "; // just put in as text???
                    MessageBeep(0);
                    }
                }
            else
                {
                CField* pfldNew = new CField( pmkr ); // create new field to insert
                InsertAfter( pfld, pfldNew );
                pfld = pfldNew;
                }
            p += len + strlen(pNL)+1;
            if (*p == *pNL || !*p)
                p--;
            }
#ifndef _MAC
        }
#endif

    if (bAddedNewMkr)
        AfxMessageBox(_("New markers were added with * as field name.")); // 1.5.0fj 

    // update rps to position following stuff just inserted
    prps->SetPos( (*pfld).GetLength(), pfld );
    if ( bValidFldLen( pfld->GetLength() + sTail.GetLength() ) ) // don't overstuff Str8!
        *pfld += sTail; // tack on stuff following caret
}


void CRecord::ApplyDateStamp( CMarker* pmkrDateStamp ) // update or add datestamp field
{
    ASSERT( pmkrDateStamp );

    CField* pfld = pfldFirstWithMarker( pmkrDateStamp ); // see if there is one already
    if ( !pfld ) // if not, create one and append to record
        {
        pfld = new CField( pmkrDateStamp );
        Add( pfld );
        }
    
    // 1998-04-07 MRP: At the year 2000, Shoebox 3 will start
    // producing mangled date stamps like 01/Jan/100@#$...
    // Shoebox 4 extends the existing format with a four-digit year.
    Str8 sDateStamp;
    CDateCon::s_DateStamp(sDateStamp);  // Format: 07/Apr/1998

    Str8 sBlankLines; // If blank lines or other content after date, keep it and restore it
    int iBlankLines = pfld->Find( '\n' );
    if ( iBlankLines >= 0 ) // If blank lines at end of date, keep them
        sBlankLines = pfld->Mid( iBlankLines );
    sDateStamp += sBlankLines;

    pfld->SetContent( sDateStamp ); // replace previous field contents with today's date
}

// CRecord::bCheckConsistency replaces CShwView::bValidateRangeSet
BOOL CRecord::bCheckConsistency( const CNumberedRecElPtr& pnrl, CShwView* pview, CCheckingState *pcks )
{
    ASSERT( pview );
    CField* pfld = pfldFirst();
    for ( ; pfld; pfld = pfldNext(pfld) )
        if ( !bCheckFieldConsistency( pfld, pnrl, pview, pcks ) )
            return FALSE;
    return TRUE;
}


// 12-07-1999 - TLB: The following three functions (formerly CField::bCheckConsistency,
//                   CField:bDataPropertiesPassed, and CField::bProcessSkipOrReplaceAll)
//                   were moved into CRecord to facilitate actions in the Inconsistency
//                   dialogs that might result in modification or destruction of the field
//                   being checked. These functions probably fit more logically in the
//                   CField class, but that's life.
BOOL CRecord::bCheckFieldConsistency(CField* pfld, const CNumberedRecElPtr& pnrl,
                                    CShwView* pview, CCheckingState *pcks)
{
    ASSERT(pfld);
    ASSERT(pnrl);
    ASSERT(pview);

    // IMPORTANT NOTE:
    // Any checks added here must be reflected in CMarker::bHasConsistencyCheckConditions
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    BOOL bCheckDataProperties = ( pcks ) ? pcks->bCheckDataDefn() : TRUE;
    BOOL bCheckRangeSets = ( pcks ) ? pcks->bCheckRangeSets() : TRUE;
    BOOL bCheckDataLinks = ( pcks ) ? pcks->bCheckRefConsistency() : TRUE;

    bCheckRangeSets &= pfld->pmkr()->bRangeSetEnabled();
    CJumpPath* pjmp = NULL;
    if ( bCheckDataLinks )
        {
        pjmp = pfld->pmkr()->pjmpPrimary();
        if ( !pjmp )
            bCheckDataLinks = FALSE;
        }

    CRecPos rpsDataBegin, rpsStart, rpsEnd;
    BOOL bMultipleItemData = pfld->pmkr()->bMultipleItemData();

    if ( bCheckDataProperties && !bFieldDataPropertiesPassed(pfld, pnrl, pview, pcks) )
        return FALSE;

    // If there's no data, we're done.
    if ( !pfld->bParseFirstItem(pnrl->prec(), rpsStart, rpsEnd, bMultipleItemData) )
        return TRUE;

    Str8 sData;
    int iChar;

    BOOL bRecheck = FALSE; // TRUE when a replacement has been made
    do
        {
        // Data properties already checked above, so we only need to recheck this when
        // a replacement is made. (Currently, only a replacement on the first item can
        // invalidate a previously passed data properties check, but we don't want to
        // count on that.)
        if ( bRecheck )
            {
            if ( bCheckDataProperties && !bFieldDataPropertiesPassed(pfld, pnrl, pview, pcks) )
                {
                return FALSE;
                }
            if ( !pfld->bParseCurItem(rpsStart, rpsEnd, bMultipleItemData) )
                {
                return TRUE;
                }
            }

        // If data is multiple items, we assume rpsStart and rpsEnd delimit a single word.
        sData = pfld->sItem(rpsStart, rpsEnd, !bMultipleItemData);

        bRecheck = FALSE;
        // Note: if another check is added prior to the first one here, need to add !bRecheck
        // as first condition in the if statement because once a replacement is made requiring
        // a recheck, we skipp all remaining checks and start over from the top.

        if ( bCheckRangeSets && !pfld->pmkr()->prngset()->bConsistent(sData, iChar) )
            {
            if ( !pcks || !bProcessSkipOrReplaceAll(sData, rpsStart, rpsEnd, pcks, bRecheck) )
                {
                // Select the inconsistency (or the item containing the inconsistent char) in
                // the appropriate view
                pview->SetCurAndCaret(pnrl, pfld, rpsStart.iChar, sData.GetLength());

                // If this is a character-based range set, display the Character Range Set
                // Consistency Check dialog; otherwise, display the Range Set Consistency
                // Check dialog.
                if ( pfld->pmkr()->prngset()->bRangeSetForCharacters() )
                    {
                    CCharRangeSetInconsistencyDlg dlg(pview, rpsStart, rpsEnd, pcks, bRecheck, iChar);
                    if (dlg.DoModal() != IDOK)
                        return FALSE;
                    }
                else
                    {
                    CRangeSetInconsistencyDlg dlg(pview, rpsStart, rpsEnd, pcks, bRecheck, _("Data item not found in range set."), FALSE);
                    if (dlg.DoModal() != IDOK)
                        return FALSE;
                    }
                }
            }

        if ( !bRecheck && bCheckDataLinks )
            {
            BOOL bMatched;
            BOOL bEnableInsert;
#ifdef _DEBUG
            ASSERT(pjmp);
#endif
            if ( !pjmp->bCheckDataLink(sData, bMatched, bEnableInsert) )
                return FALSE;
            if ( !bMatched )
                {
                if ( !pcks || !bProcessSkipOrReplaceAll(sData, rpsStart, rpsEnd, pcks, bRecheck) )
                    {
                    // Select the inconsistency (or the item containing the inconsistent char) in
                    // the appropriate view
                    pview->SetCurAndCaret(pnrl, pfld, rpsStart.iChar, sData.GetLength());

                    // Display Data Link Consistency Check dialog
                    CShwDoc* pdoc = NULL;
                    CReferentialInconsistencyDlg dlg(pview, rpsStart, rpsEnd, pcks, bRecheck,
                                                     bEnableInsert, pjmp, &pdoc);
                    switch ( dlg.DoModal() )
                        {
                        case IDOK:
                            break;
                        case IDC_INSERT:
                            {
                            ASSERT(pdoc);
                            CNumberedRecElPtr prel = NULL;
                            pview->JumpToKey(TRUE, sData, pdoc->pindset()->pindRecordOwner(), pdoc, prel);
                            // Now we need to cancel this validation operation and let the user do whatever
                            // they need to do to enter some data into the newly inserted record. This
                            // avoids an ASSERT in the case where a jump target has been set and it also
                            // helps the user avoid glosing track of the new records that need to be completed.
                            // Finally, it sidesteps some potential confusion when the operation that triggered
                            // the validation in the first place was a jump or jump-insert.
                            return FALSE;
                            }
                        default:
                            return FALSE;
                        }
                    }
                }
            }
        }
    while ( bRecheck || pfld->bParseNextItem(rpsStart, rpsEnd, bMultipleItemData) );
    return TRUE;
}

// Checks Data Properties. If they don't pass, it applies any matching skip-all or replace-all. If no
// skip/replace match is found, display the appropriate dialog box. If any kind of replacement occurs,
// cycle back through until data passes or until it is skipped by user. Returns TRUE unless user chooses
// cancel in the dialog box.
BOOL CRecord::bFieldDataPropertiesPassed(CField* pfld, const CNumberedRecElPtr& pnrl,
                                         CShwView* pview, CCheckingState *pcks)
{
    ASSERT(pfld);
    ASSERT(pnrl);
    ASSERT(pview);

//    UINT nID; // 1.4heq
	Str8 sMessage; // 1.4heq
	BOOL bExplicitNewlines; // 1.4heq
    BOOL bReplacementMade;
    CRecPos rpsStart, rpsEnd;

    do
        {
        bReplacementMade = FALSE;
        if ( pfld->bValidItemCount(pnrl->prec(), rpsStart, rpsEnd, sMessage, bExplicitNewlines) ) // 1.4heq
            return TRUE;

        Str8 sData = pfld->sItem(rpsStart, rpsEnd, TRUE);
        if ( pcks && bProcessSkipOrReplaceAll(sData, rpsStart, rpsEnd, pcks, bReplacementMade) )
            {
            if ( !bReplacementMade )
                return TRUE; // This was a Skip-All, so now we've "passed"
            }
        else
            {
            // Select the inconsistency in the appropriate view
            pview->SetCurAndCaret(pnrl, pfld, rpsStart.iChar, (rpsEnd.iChar - rpsStart.iChar));

            // If there is an item-based range set on this field, display the Range Set
            // Consistency Check dialog; otherwise, display the Data Definition Consistency
            // Check dialog.
            if ( pfld->pmkr()->bRangeSetEnabled() && !pfld->pmkr()->prngset()->bRangeSetForCharacters() )
                {
                CRangeSetInconsistencyDlg dlg(pview, rpsStart, rpsEnd, pcks, bReplacementMade, sMessage, bExplicitNewlines); // 1.4heq
                if (dlg.DoModal() != IDOK)
                    return FALSE;
                }
            else
                {
                CDataDefnInconsistencyDlg dlg(pview, rpsStart, rpsEnd, pcks, bReplacementMade, sMessage, bExplicitNewlines); // 1.4heq
                if (dlg.DoModal() != IDOK)
                    return FALSE;
                }
            }
        }
    while ( bReplacementMade );
    return TRUE;
}


BOOL CRecord::bProcessSkipOrReplaceAll(const Str8& sData, const CRecPos& rpsStart, const CRecPos& rpsEnd,
                                      CCheckingState *pcks, BOOL &bReplacementMade)
{
    ASSERT(pcks);
    CField* pfld = rpsStart.pfld;
    ASSERT(pfld);
    ASSERT(pfld == rpsEnd.pfld);
    ASSERT(rpsEnd >= rpsStart);
    if ( pcks->bSkipAllElement(pfld->pmkr(), sData) )
        {
        pcks->IncrementInconsistenciesSkipped();
        bReplacementMade = FALSE;
        return TRUE;
        }

    Str8 sReplacement;
    if ( pcks->bReplaceAllElement(pfld->pmkr(), sData, sReplacement) )
        {
        ASSERT(sData != sReplacement);
        pfld->SetContent(pfld->sContents().Left(rpsStart.iChar) + sReplacement + pfld->sContents().Mid(rpsEnd.iChar));
        bReplacementMade = TRUE;
        return TRUE;
        }
    return FALSE;
}

#ifdef _DEBUG
void CRecord::AssertValid() const
{
    ASSERT(!bIsEmpty());
}
#endif  // _DEBUG

// --------------------------------------------------------------------------

CFieldIterator::CFieldIterator(const CRecLookEl* prel, const CIndex* pind)
{
    ASSERT( prel );
    ASSERT( pind );
    m_prec = prel->prec();
    ASSERT( m_prec );
    m_pfldPriKey = prel->pfldPriKey();
    m_bSubstitutedForNullPriKey = ( m_pfldPriKey == NULL );
    if ( m_bSubstitutedForNullPriKey )
        m_pfldPriKey = pind->pfldSubstituteForNullPriKey();
    ASSERT( m_pfldPriKey );
    m_bPriKeyIsFirstField = ( m_pfldPriKey == m_prec->pfldFirst() );
    m_pfld = m_pfldPriKey;
    m_bUnderPriKey = FALSE;
    ASSERT( bIncludeMarker(m_pfldPriKey->pmkr()) );
}

CFieldIterator::CFieldIterator()
{
    m_prec = NULL;
    m_pfldPriKey = NULL;
    m_bSubstitutedForNullPriKey = FALSE;
    m_bPriKeyIsFirstField = FALSE;
    m_pfld = NULL;
    m_bUnderPriKey = TRUE;
}

CFieldIterator::CFieldIterator(const CFieldIterator& qfld)
{
    m_prec = qfld.m_prec;
    m_pfldPriKey = qfld.m_pfldPriKey;
    m_bSubstitutedForNullPriKey = qfld.m_bSubstitutedForNullPriKey;
    m_bPriKeyIsFirstField = qfld.m_bPriKeyIsFirstField;
    m_pfld = qfld.m_pfld;
    m_bUnderPriKey = qfld.m_bUnderPriKey;
}

CFieldIterator& CFieldIterator::operator++()  // Pre-increment
{
    NextField();  // Increment
    return *this;
}

const CFieldIterator CFieldIterator::operator++(int)  // Post-increment
{
    CFieldIterator qfld = *this;
    NextField();  // Increment
    return qfld;
}

void CFieldIterator::NextField()
{
    do NextFieldInSubRecord(); while ( m_pfld && !bIncludeField() );
}

BOOL CFieldIterator::bIncludeField() const
{
    ASSERT( m_pfld );
    const CMarker* pmkr = m_pfld->pmkr();
    
    return bIncludeMarker(pmkr);
}

BOOL CFieldIterator::bIncludeMarker(const CMarker* pmkr) const
{
    ASSERT( pmkr );
//  return pmkr->bIncludedIn(m_...?);

    return TRUE;
}

void CFieldIterator::NextFieldInSubRecord()
{
    do NextFieldInRecord(); while ( m_pfld && !bInSubRecord() );
}

BOOL CFieldIterator::bInSubRecord() const
{
    ASSERT( m_pfld );
    const CMarker* pmkr = m_pfld->pmkr();
    ASSERT( pmkr );
    if ( m_bSubstitutedForNullPriKey )  // If no primary key field
        return TRUE;  // Then all fields are in the NULL element's sub-record
        
    const CMarker* pmkrPriKey = m_pfldPriKey->pmkr();
    if ( pmkr == pmkrPriKey )
        // Exclude any other occurrences of the primary key marker
        return m_pfld == m_pfldPriKey;      
    else if ( !pmkr->pmkrOverThis() || !pmkrPriKey->pmkrOverThis() )
        // If either marker lacks a hierarchical relationship
        return TRUE;  // Then all fields are in the element's sub-record
    else if ( pmkr->bOver(pmkrPriKey) )
        {
        if ( m_bUnderPriKey )
            return FALSE;
            
        CField* pfld = m_prec->pfldNext(m_pfld);        
        for ( ; pfld != m_pfldPriKey; pfld = m_prec->pfldNext(pfld) )
            {
            ASSERT( pfld );
            if ( pfld->pmkr()->bEqualOrOver(pmkr) )
                return FALSE;
            }
        }
    else if ( pmkr->bUnder(pmkrPriKey) )
        {
        if ( !m_bUnderPriKey )
            return FALSE;
            
        CField* pfld = m_prec->pfldPrev(m_pfld);        
        for ( ; pfld != m_pfldPriKey; pfld = m_prec->pfldPrev(pfld) )
            {
            ASSERT( pfld );
            if ( pfld->pmkr()->bEqualOrOver(pmkrPriKey) )
                return FALSE;
            }
        }
    else
        {
        const CMarker* pmkrOverBoth = pmkr->pmkrLowestOverBoth(pmkrPriKey);
        if ( !pmkrOverBoth )  // 1996-07-26 MRP
            return TRUE;
        
        if ( m_bUnderPriKey )
            {
            CField* pfld = m_prec->pfldPrev(m_pfld);        
            for ( ; pfld != m_pfldPriKey; pfld = m_prec->pfldPrev(pfld) )
                {
                ASSERT( pfld );
                if ( pfld->pmkr()->bEqualOrOver(pmkrOverBoth) )
                    return FALSE;
                }
            }
        else
            {
            CField* pfld = m_prec->pfldNext(m_pfld);        
            for ( ; pfld != m_pfldPriKey; pfld = m_prec->pfldNext(pfld) )
                {
                ASSERT( pfld );
                if ( pfld->pmkr()->bEqualOrOver(pmkrOverBoth) )
                    return FALSE;
                }
            }
        }
        
    return TRUE;
}

void CFieldIterator::NextFieldInRecord()
{
    ASSERT( m_pfld );
    if ( m_bPriKeyIsFirstField )
        {
        m_pfld = m_prec->pfldNext(m_pfld);
        m_bUnderPriKey = TRUE;
        }
    else if ( m_pfld == m_pfldPriKey )
        m_pfld = m_prec->pfldFirst();
    else
        {
        m_pfld = m_prec->pfldNext(m_pfld);
        if ( m_pfld == m_pfldPriKey )
            {
            m_pfld = m_prec->pfldNext(m_pfld);
            m_bUnderPriKey = TRUE;
            }
        }
}

// ==========================================================================
