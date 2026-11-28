// jmp.cpp  Implementation for Multiple Jump Paths (1999-04-29)
//
// "Yet these you may eat among all the winged insects which
// walk on all fours: those which have above their feet
// jointed legs with which to jump on the earth."
//                    Leviticus 11:21

#include "stdafx.h"
#include "toolbox.h"
#include "jmp.h"
#include "spath_d.h"
#include "shwdoc.h"
#include <fstream>
#include "recset.h"
#include "shw.h"
#include "project.h"
#include "tools.h"
#include "doclist.h"
#include "jumpdlg.h"
#include "multmat.h"
#include "seldb_d.h"
#include "nomat_d.h"
#include "obstream.h"  // Object_istream, Object_ostream
#include "crecpos.h"

#include "shwview.h"
#include "mkr_d.h"
#include "set_d.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


// **************************************************************************
// Normal constructor: jump path from all sources (default) to no targets
// Subsequent property reads will usually restrict sources, add targets, and
// possibly alter default matching criteria.
CJumpPath::CJumpPath(const char* pszName, CJumpPathSet *pjmpsetMyOwner) :
    CSetEl( pszName ),
    m_pjmpsetMyOwner( pjmpsetMyOwner ),
    m_subsetSourceFields( pjmpsetMyOwner->pmkrset(), TRUE ),
    m_bEnforceRefIntegrity( FALSE ),
    m_bMatchCriteriaSetExplicitly( FALSE ),
    m_bMatchWhole(s_bDefaultMatchWholeSetting(FALSE)),
    m_matset(s_matsetDefaultSetting(FALSE))
{
    ASSERT( pszName );
    ASSERT( pjmpsetMyOwner );
}

// Special default constructor for old-style jump path: all sources included
CJumpPath::CJumpPath(const char* pszName,
                     const CDatabaseRefList &drflstTarget,
                     CJumpPathSet *pjmpsetMyOwner) :
    CSetEl( "default" ),
    m_pjmpsetMyOwner( pjmpsetMyOwner ),
    m_subsetSourceFields( pjmpsetMyOwner->pmkrset(), TRUE ),
    m_drflstTarget(drflstTarget),
    m_bEnforceRefIntegrity( FALSE ),
    m_bMatchCriteriaSetExplicitly( FALSE ),
    m_bMatchWhole(s_bDefaultMatchWholeSetting(FALSE)),
    m_matset(s_matsetDefaultSetting(FALSE))
{
    ASSERT( pszName );
    ASSERT( pjmpsetMyOwner );
}

CJumpPath::CJumpPath(const CJumpPath& jmp) :
    CSetEl( jmp.sName() + "*" ),
    m_pjmpsetMyOwner( jmp.pjmpsetMyOwner() ),
    m_subsetSourceFields( jmp.pjmpsetMyOwner()->pmkrset(), FALSE ),
    m_drflstTarget( jmp.m_drflstTarget ),
    m_bEnforceRefIntegrity( jmp.bEnforceRefIntegrity() ),
    m_bMatchCriteriaSetExplicitly( jmp.m_bMatchCriteriaSetExplicitly ),
    m_bMatchWhole(jmp.m_bMatchWhole),
    m_matset(jmp.m_matset)
{
    ASSERT( m_pjmpsetMyOwner );
}

CJumpPath::~CJumpPath()
{
}

static const char* psz_jmp = "jmp";
static const char* psz_EnfRefIntg = "EnforceRefIntegrity";
static const char* psz_MatchWhole = "MatchWhole";
static const char* psz_MatchChar = "match_char";

#ifdef typWritefstream // 1.6.4ac
void CJumpPath::WriteProperties(Object_ofstream& obs) const 
#else
void CJumpPath::WriteProperties(Object_ostream& obs) const 
#endif
{
    obs.WriteBeginMarker(psz_jmp, sName());
    obs.WriteBool(psz_EnfRefIntg, m_bEnforceRefIntegrity);
    m_subsetSourceFields.WriteProperties(obs);
    m_drflstTarget.WriteProperties(obs);
    if ( m_bMatchCriteriaSetExplicitly )
        {
        obs.WriteBool(psz_MatchWhole, m_bMatchWhole);
        // 1999-11-30 TLB: Need to encapsulate this during revision of matching
        const char* pszProperty;
        if ( m_matset == CMCharOrder::matsetSamePrimaryGroup )
            pszProperty = "p";
        else if ( m_matset == CMCharOrder::matsetExactDisregardCase )
            pszProperty = "c";
        else if ( m_matset == CMCharOrder::matsetExact )
            pszProperty = "e";
        else if ( m_matset == CMCharOrder::matsetEvenIgnore )
            pszProperty = "i";
        else
            ASSERT( FALSE );
        obs.WriteString(psz_MatchChar, pszProperty);
        }
    obs.WriteEndMarker(psz_jmp);
}

void CJumpPath::ReadProperties(Object_istream& obs)
{
    Str8 sMatchChar;
    BOOL bMatchWhole = FALSE;
    while ( !obs.bAtEnd() )
        {
        if ( obs.bReadBool(psz_EnfRefIntg, m_bEnforceRefIntegrity) )
            ;
        else if ( m_subsetSourceFields.bReadProperties(obs) )
            ;
        else if ( m_drflstTarget.bReadProperties(obs) )
            ;
        else if ( obs.bReadBool(psz_MatchWhole, bMatchWhole) )
            m_bMatchCriteriaSetExplicitly = TRUE;
        else if ( obs.bReadString("match_char", sMatchChar) )
            {
            m_bMatchCriteriaSetExplicitly = TRUE;
            if ( bEqual(sMatchChar, "p") )
                m_matset = CMCharOrder::matsetSamePrimaryGroup;
            else if ( bEqual(sMatchChar, "c") )
                m_matset = CMCharOrder::matsetExactDisregardCase;
            else if ( bEqual(sMatchChar, "e") )
                m_matset = CMCharOrder::matsetExact;
            else if ( bEqual(sMatchChar, "i") )
                m_matset = CMCharOrder::matsetEvenIgnore;
            }
        else if ( obs.bEnd(psz_jmp) )
            break;
        }
    // Default path or path with no destinations (targets) can't be used for ref. integrity
    // Note that through the interface, it is no longer possible to create a jump path with
    // no destinations, but this will catch any holdovers from v 4.2 or any settings-file
    // tampering outside Shoebox.
    if ( bDefaultPath() || m_drflstTarget.bIsEmpty() )
        m_bEnforceRefIntegrity = FALSE;
    if ( m_bMatchCriteriaSetExplicitly )
        {
        m_bMatchWhole = bMatchWhole;
        }
    else if ( m_bEnforceRefIntegrity )
        {
        m_bMatchWhole = s_bDefaultMatchWholeSetting(TRUE);
        m_matset = s_matsetDefaultSetting(TRUE);
        }
}

void CJumpPath::SetKeyCriteria(CKey& key) const
{
    key.SetKeyCriteria(m_bMatchWhole, m_matset);
}

void CJumpPath::SetMatchCriteriaFromKey(CKey* pkey, BOOL bMatchCriteriaSetExplicitly)
{
    m_bMatchCriteriaSetExplicitly = bMatchCriteriaSetExplicitly;
    m_bMatchWhole = pkey->bMatchWhole();
    m_matset = pkey->matset();
}

// Get the language encoding for an arbitrary marker
CLangEnc* CJumpPath::plngDefault()
{
    if ( bDefaultPath() )
        return m_pjmpsetMyOwner->pmkrset()->plngDefault();

    CMarker* pmkr = m_pjmpsetMyOwner->pmkrset()->pmkrFirst();
    for (; pmkr; pmkr = m_pjmpsetMyOwner->pmkrset()->pmkrNext(pmkr))
        {
        if ( bIncludes(pmkr) )
            return pmkr->plng();
        }
    return m_pjmpsetMyOwner->pmkrset()->plngDefault();
}

CJumpPath::EPathSearchResult CJumpPath::psrSearch(const CKey* pkey,
                                                  CMultipleMatchList &mmtlst,
                                                  BOOL &bEnableInsert)
{
    if ( m_drflstTarget.bIsEmpty() )
        return psrNoRefs;

    EPathSearchResult psrReturn = psrNoMatches;
    bEnableInsert = FALSE;  // 1996-10-15 MRP
    BOOL bTryAgain;
    CDatabaseRef* pdrf = m_drflstTarget.pdrfFirst();
    for ( ; pdrf; pdrf = m_drflstTarget.pdrfNext(pdrf) )
        {
        do {
        bTryAgain = FALSE;
        switch ( pdrf->dsrSearch(pkey, &mmtlst) ) // search entire jump path for match
            {
            case CDatabaseRef::dsrFound:
                psrReturn = psrFound;
                // Don't break here...
            case CDatabaseRef::dsrNoMatches:
                bEnableInsert |= ( pdrf->bInsertable() );
                break;
            case CDatabaseRef::dsrDBNotOpen:
                if ( !pdrf->bDontCheckForOpenDB() )
                    {
                    Str8 sDBName = sGetFileName(pdrf->sDatabasePath(), TRUE);
                    Str8 sDocPathName;
                    CDocList doclst; // list of currently open docs
                    CShwDoc* pdoc = doclst.pdocFirst();
                    for (; pdoc; pdoc = doclst.pdocNext() )
                        {
                        // Strip off path -- all we want is a matching name
                        sDocPathName =  sUTF8( pdoc->GetPathName() ); // 1.4qra
                        if ( !_stricmp(sGetFileName(sDocPathName, TRUE), sDBName) )
                            break;
                        }
                    int iResult;
                    // If another DB with same name is open, display CDBDiffPathDlg
                    if ( pdoc )
                        {
                        CDBDiffPathDlg dlg(pdrf->sDatabasePath(), sDocPathName, sKey());
                        iResult = dlg.DoModal();
                        if ( iResult == ID_FIX_IT )
                            {
                            m_pjmpsetMyOwner->FixDBRefs(pdrf->sDatabasePath(), sDocPathName);
                            bTryAgain = TRUE;
                            }
                        else if ( iResult == ID_SKIP )
                            m_pjmpsetMyOwner->DontCheckDiffPathsForDBRefs(pdrf->sDatabasePath());
                        }
                    else if ( pdrf->bOkayToSkipDB() )
                        break;
                    else // Otherwise, display CDBNotOpenDlg dialog
                        {
                        CDBNotOpenDlg dlg(pdrf->sDatabasePath(), sKey());
                        iResult = dlg.DoModal();
                        if ( iResult == ID_SKIP )
                            m_pjmpsetMyOwner->SkipDBRefs(pdrf->sDatabasePath());
                        }
                    if ( iResult == IDCANCEL )
                        return psrCancel;
                    }
                break;
            case CDatabaseRef::dsrMarkerNotFound:
                if ( !pdrf->bOkayToSkipMkr() )
                    {
                    CFieldNotDefinedDlg dlg(pdrf->sDatabasePath(), pdrf->sKey(), sKey());
                    int iResult = dlg.DoModal();
                    if ( iResult == ID_SKIP )
                        m_pjmpsetMyOwner->SkipDBRefs(pdrf->sDatabasePath(), pdrf->sKey());
                    else if ( iResult == IDCANCEL )
                        return psrCancel;
                    }
                break;
            default:
                ASSERT(FALSE); // Unexpected return code
                return psrCancel;
            }
        } while ( bTryAgain );
        }
    return psrReturn;
}

void CJumpPath::OnDocumentClose( CShwDoc* pdoc ) // handle references to document being closed
{
    m_drflstTarget.OnDocumentClose(pdoc);
}

BOOL CJumpPath::bRefersTo( CIndexSet* pindset ) // check for any references to this index set
{
    return m_drflstTarget.bRefersTo(pindset);
}

BOOL CJumpPath::bRefersTo( CShwDoc* pdoc ) // check for any references to this doc
{
    return m_drflstTarget.bRefersTo(pdoc);
}

void CJumpPath::MarkerUpdated( CMarkerUpdate& mup ) // update drflsts
{
    m_drflstTarget.MarkerUpdated(mup);
}

// All matching dbrefs should be skipped unconditionally if Doc is not open
void CJumpPath::DontCheckDiffPathsForDBRefs(const Str8& sDatabasePath)
{
    m_drflstTarget.DontCheckDiffPathsForDBRefs(sDatabasePath);
}

// Mark all matching dbrefs as okay to skip
void CJumpPath::SkipDBRefs(const Str8& sDatabasePath, const Str8& sKey)
{
    m_drflstTarget.OkayToSkipDBRefs(sDatabasePath, sKey);
}

// Change all matching dbrefs to new pathname
void CJumpPath::FixDBRefs(const Str8& sOldDatabasePath, const Str8& sNewDatabasePath)
{
    m_drflstTarget.FixDBRefs(sOldDatabasePath, sNewDatabasePath);
}

/**********************************************************************************
 * Name:      CJumpPath::bDataLinkConsistent
 * Written:   07/29/1999
 * Developer: Tom Bogle
 * Purpose:   Validates a data item for referential integrity of data link.
 *
 * Parameters:
 *   Name           Type              Description             In/Out  Default Value
 *   =============  ================  ======================  ======  =============
 *   sData          const Str8&    Data to be checked      Input
 *   bMatched       BOOL&             Referenced data found?  Output
 *   key            CKey&             search key (needed for  Output
 *                                    caller to do insert)
 *   bEnableInsert  BOOL&             Is there a valid dest-  Output
 *                                    ination for insertion?
 *
 * Return type:    Boolean
 * Return value:   TRUE  a) if jump path is not a data link (bMatched = TRUE)
 *                       b) if referenced data is found (bMatched = TRUE)
 *                       c) if referenced data is not found (bMatched = FALSE)
 *                 FALSE a) if data link is invalid (produces ASSERT)
 *                       b) if user cancels from one of the warning dialogs.
 * Preconditions:  Parameter sData is not empty.
 * Postconditions: If function returns FALSE, none of the output parameters are
 *                 defined. Otherwise, all are defined as per their descriptions.
 * Description:
 * Issues/Notes:
 *
 * Modifications:
 *   Date        Developer    Description
 *   ==========  ===========  =====================================================
 **********************************************************************************/
BOOL CJumpPath::bCheckDataLink(const Str8& sData, BOOL& bMatched, BOOL& bEnableInsert)
{
    if ( !m_bEnforceRefIntegrity )
    {
        bMatched = TRUE;
        return TRUE;
    }

    ASSERT(!sData.IsEmpty());

	Str8 sTempData = sData;  // 1.5.9e Fix bug of data link check not handling number at end of word
	char cLastChar = sData.GetChar( sTempData.GetLength() - 1 ); // 1.5.9e Get last character of data
	if ( cLastChar >= '1' && cLastChar <= '9' ) // 1.5.9e If last character of data is digit, remove it
		sTempData.Delete( sTempData.GetLength() - 1, 1 ); // 1.5.9e Remove last character of data

    CKey key(sTempData, m_bMatchWhole, m_matset);
    CMultipleMatchList mmtlst;
    bEnableInsert = FALSE;
    switch ( psrSearch(&key, mmtlst, bEnableInsert) )
        {
        case psrFound:
            bMatched = TRUE;
            return TRUE;
        case psrNoMatches:
            bMatched = FALSE;
            return TRUE;
        case psrCancel:
            break;
        case psrNoRefs:
            ASSERT(FALSE); // Jump paths set up to enforce referential integrity cannot have zero destinations
            break;
        default:
            ASSERT(FALSE); // Unexpected return code
        }
    return FALSE;
}

BOOL CJumpPath::bCopy(CSetEl** ppselNew)
{
    CJumpPathDlg dlg(this, (CJumpPath**)ppselNew);
    return ( dlg.DoModal() == IDOK );
}

BOOL CJumpPath::bModify()
{
    CJumpPathDlg dlg(this);
    if ( dlg.DoModal() == IDOK )  // show a modal view of the jump path
        {
        m_pjmpsetMyOwner->UpdateDefaultJumpPathSetting(this);
        return TRUE;
        }
    else
        return FALSE;
}

// --------------------------------------------------------------------------
CJumpPathSet::CJumpPathSet(CMarkerSet* pmkrset) :
    m_pmkrset(pmkrset)
{
    ASSERT( m_pmkrset );
    m_pjmpDefault = NULL;
}

static const char* psz_jmpset = "jmpset";

#ifdef typWritefstream // 1.6.4ac
void CJumpPathSet::WriteProperties(Object_ofstream& obs) const
#else
void CJumpPathSet::WriteProperties(Object_ostream& obs) const
#endif
{
    obs.WriteNewline();
    obs.WriteBeginMarker(psz_jmpset);
    
    const CJumpPath* pjmp = pjmpFirst();
    for ( ; pjmp; pjmp = pjmpNext(pjmp) )
        pjmp->WriteProperties(obs);

    obs.WriteEndMarker(psz_jmpset);
    obs.WriteNewline();
}

BOOL CJumpPathSet::bReadProperties(Object_istream& obs)
{
    if ( !obs.bReadBeginMarker(psz_jmpset) )
        return FALSE;

    Str8 sName;
    while ( !obs.bAtEnd() )
        {
        if ( obs.bReadBeginMarker(psz_jmp, sName) )
            {
            CJumpPath* pjmp = new CJumpPath( sName, this );
            pjmp->ReadProperties( obs );
            Add( pjmp );
            }
        else if ( obs.bEnd(psz_jmpset) )
            break;
        }

    return TRUE;
}

void CJumpPathSet::Add( CJumpPath* pjmpNew )
{
    if ( pjmpNew->bDefaultPath() )
        {
        ASSERT(!m_pjmpDefault);
        m_pjmpDefault = pjmpNew;
        }

    CSet::Add(pjmpNew);
}

BOOL CJumpPathSet::bAddDefaultJumpPath( const CDatabaseRefList &drflstTarget )
{
    if ( m_pjmpDefault )
        return FALSE;

    CJumpPath* pjmpNew = new CJumpPath( "default", drflstTarget, this );
    if ( !pjmpNew )
        {
        ASSERT( FALSE );
        return FALSE;
        }
    if ( !pjmpNew->bDefaultPath() )
        {
        ASSERT( FALSE );
        return FALSE;
        }

    Add(pjmpNew);

    return TRUE;
}

void CJumpPathSet::UpdateDefaultJumpPathSetting(CJumpPath* pjmpModified)
{
    if ( pjmpModified == m_pjmpDefault )
        {
        if ( !pjmpModified->bDefaultPath() ) // User made the default jump path into a specific path
            m_pjmpDefault = NULL;
        }
    else if ( pjmpModified->bDefaultPath() ) // User turned a specific jump path into the default
        {
        ASSERT( m_pjmpDefault == NULL); // Rules in Jump Path dialog should have prevented multiple defaults
        m_pjmpDefault = pjmpModified;
        }
        
}


CSetEl* CJumpPathSet::pselRemove( CSetEl* psel )
{
    if ( (CJumpPath*)psel == m_pjmpDefault )
        m_pjmpDefault = NULL;
    return CSet::pselRemove( psel );
}

void CJumpPathSet::Delete( CSetEl** ppsel ) // Delete jump path, set pointer to NULL
{
    if ( (CJumpPath*)*ppsel == m_pjmpDefault )
    m_pjmpDefault = NULL;
    CSet::Delete( ppsel );
}

BOOL CJumpPathSet::bAdd(CSetEl** ppselNew)
{
    CJumpPathDlg dlg(this, (CJumpPath**)ppselNew);
    return ( dlg.DoModal() == IDOK );  // show a modal view of a new jump path
}

// --------------------------------------------------------------------------

// set pprel, ppind, ppdoc to record and document match found in
// NOTE: current document should be passed in ppdoc, to be used if jump path is empty
CJumpPathSet::EJumpSearchResult CJumpPathSet::jsrSearch(CMarker* pmkrSource, Str8& sWord,
                                          CNumberedRecElPtr* pprel, CIndex** ppind,
                                          CShwDoc** ppdoc, CDatabaseType* ptyp,
                                          CLangEnc* plng, BOOL bIsJumpInsert,
                                          BOOL bShowDlg) const
{
    CJumpPath* pjmp = NULL;
    CJumpPath* pjmpPrimary = NULL;
    int iResult = IDC_TRYAGAIN;
    CKey key(sWord, CJumpPath::s_bDefaultMatchWholeSetting(FALSE), CJumpPath::s_matsetDefaultSetting(FALSE));

    // Find the appropriate primary jump path for current marker, if any.
    if ( pmkrSource )
        pjmpPrimary = pmkrSource->pjmpPrimary();
    if ( !pjmpPrimary )
        pjmpPrimary = m_pjmpDefault; // If no default jump path, this will still be NULL
    
    if ( pjmpPrimary )
        pjmpPrimary->SetKeyCriteria(key);

    do  {
    if ( bShowDlg ) // Put up dialog first time around if caller requests it
        {
        // Allow user to set key text and search parameters
        CJumpDlg dlg( &key, ptyp, &plng, &pjmpPrimary, bIsJumpInsert );
        if ( dlg.DoModal() != IDOK )
             return jsrCancel;
        // 2000-09-06 TLB: Need to set the value of sWord to reflect possible change in
        // the key. Otherwise, jump insert will insert a blank record.
        sWord = key.sKey();
        }
    else
        bShowDlg = TRUE; // always show it second time around!

    BOOL bEnableInsert = FALSE;

    // Show the results if more than one record found (or any for Jump Insert)
    CMultipleMatchList mmtlst;
    CMultipleMatch* pmmt=NULL;

    // Try the primary path, if any.
    pjmp = pjmpPrimary;
    if ( pjmp )
        {
        switch ( pjmp->psrSearch(&key, mmtlst, bEnableInsert) )
            {
            case CJumpPath::psrFound:
                break;
            case CJumpPath::psrCancel:
                return jsrCancel;
            case CJumpPath::psrNoRefs:
                pjmp = NULL; // don't break here! continue...
            case CJumpPath::psrNoMatches:
                // Try default path, if different from primary
                if ( pjmpPrimary != m_pjmpDefault && m_pjmpDefault )
                    {
                    BOOL bIgnoreEnableInsert; // Can only insert to primary jump path
                    // Use key from previous attempt
                    // key.SetKeyCriteria(*m_pjmpDefault->pkey());
                    switch ( m_pjmpDefault->psrSearch(&key, mmtlst, bIgnoreEnableInsert) )
                        {
                        case CJumpPath::psrFound:
                        case CJumpPath::psrNoMatches:
                            if ( !pjmp ) // i.e., We had psrNoRefs from primary attempt
                                {
                                pjmp = m_pjmpDefault;
                                ASSERT(pjmp);
                                }
                            break;
                        case CJumpPath::psrCancel:
                            return jsrCancel;
                        case CJumpPath::psrNoRefs:
                            break; // pjmp is either already NULL or is set to pjmpPrimary
                        default:
                            ASSERT(FALSE); // Unexpected return code
                            return jsrCancel;
                        }
                    }
                break;
            default:
                ASSERT(FALSE); // Unexpected return code
                return jsrCancel;
            }
        }

    // At this point, pjmp is either the primary jump path or the default jump
    // path (if no non-empty primary jump path defined), or NULL (if there is no
    // non-empty primary or default jump path).

    // Try current document if no non-empty jump path(s) available for this marker
    if ( !pjmp )
        {
        CDatabaseRef drf( (*ppdoc)->pindset()->pindRecordOwner() );
        drf.bSearch( &key, &mmtlst ); // search current doc
        bEnableInsert = ( !(*ppdoc)->pindset()->bReadOnly() );
        }

    long lMatches = mmtlst.lGetCount();
    if ( lMatches )
        {
        if (lMatches > 1 || bIsJumpInsert) // multiple matches?
            iResult = mmtlst.iModalView_Elements(&pmmt, FALSE, bEnableInsert); // let user choose from list of matches
        else // return first (and only) match if it's a plain Jump
            {
            iResult = IDOK;
            pmmt = mmtlst.pmmtFirst();
            }
    
        if ( iResult == IDOK )
            {
            ASSERT( pmmt );
            pprel->Set(pmmt->prel()); // set prel for caller
            if ( pjmp ) // If jump path was used, return document selected match was found in
                {
                CShwDoc* pdoc = pmmt->pdbref()->pdoc(FALSE);
                ASSERT( pdoc );
                *ppdoc = pdoc;
                CIndex* pind = pmmt->pdbref()->pind();
                ASSERT( pind );
                *ppind = pind;
                }
            else // no jump path, return owning index
                *ppind = (*ppdoc)->pindset()->pindRecordOwner();
            return jsrJump; // we're just doing a jump even if Jump Insert was selected
            }
        }
    else
        { // no matches
        if (bIsJumpInsert)
            iResult = bEnableInsert ? IDC_INSERT : IDCANCEL; // skip no matches dialog and go right to database selection dialog
        else
            {
            CNoMatchesDlg dlg( key.sKey(), plng, bEnableInsert );
            iResult = dlg.DoModal();
			key.SetKey( dlg.pszKey() );
			sWord = dlg.pszKey();
            }
        }
    } while ( iResult == IDC_TRYAGAIN ); // does user want to try jump again?

    if ( iResult == IDC_INSERT ) // user opted to insert new record
        {
        if ( !pjmp ) // just use current document if jump path is empty
            {
            *ppind = (*ppdoc)->pindset()->pindRecordOwner(); // return owning index for jump insert
            return jsrJumpInsert;
            }
        CShwDoc* pdoc = NULL;
         // Have user select database to insert new record into (if there's more than one).
        CSelectDbDlg dlg( pjmp->pdrflstTarget(), &pdoc, CSelectDbDlg::actInsert, TRUE );
        if (dlg.DoModal() != IDOK || !pdoc )
            return jsrCancel;
        *ppind = pdoc->pindset()->pindRecordOwner(); // return owning index for jump insert
        *ppdoc = pdoc; // return selected document to caller
        return jsrJumpInsert;
        }

    return jsrCancel;
}

void CJumpPathSet::OnDocumentClose( CShwDoc* pdoc )
{
    CJumpPath* pjmp = pjmpFirst();
    for ( ; pjmp; pjmp = pjmpNext(pjmp) )
        pjmp->OnDocumentClose(pdoc);
}

BOOL CJumpPathSet::bRefersTo( CShwDoc* pdoc ) // check for any references to this doc
{
    BOOL bReturn = FALSE;
    CJumpPath* pjmp = pjmpFirst();
    for ( ; pjmp; pjmp = pjmpNext(pjmp) )
        bReturn |= pjmp->bRefersTo(pdoc);

    return bReturn;
}

BOOL CJumpPathSet::bRefersTo( CIndexSet* pindset ) // check for any references to this index set
{
    BOOL bReturn = FALSE;
    CJumpPath* pjmp = pjmpFirst();
    for ( ; pjmp; pjmp = pjmpNext(pjmp) )
        bReturn |= pjmp->bRefersTo(pindset);

    return bReturn;
}

void CJumpPathSet::MarkerUpdated( CMarkerUpdate& mup ) // update drflsts
{
    CJumpPath* pjmp = pjmpFirst();
    for ( ; pjmp; pjmp = pjmpNext(pjmp) )
        pjmp->MarkerUpdated(mup);
}

// All matching dbrefs in all jump paths should be skipped unconditionally if Doc is not open
void CJumpPathSet::DontCheckDiffPathsForDBRefs(const Str8& sDatabasePath)
{
    CJumpPath* pjmp = pjmpFirst();
    for ( ; pjmp; pjmp = pjmpNext(pjmp) )
        pjmp->DontCheckDiffPathsForDBRefs(sDatabasePath);
}

// Mark all matching dbrefs in all jump paths as okay to skip
void CJumpPathSet::SkipDBRefs(const Str8& sDatabasePath, const Str8& sKey)
{
    CJumpPath* pjmp = pjmpFirst();
    for ( ; pjmp; pjmp = pjmpNext(pjmp) )
        pjmp->SkipDBRefs(sDatabasePath, sKey);
}

// Change all matching dbrefs in all jump paths to new pathname
void CJumpPathSet::FixDBRefs(const Str8& sOldDatabasePath, const Str8& sNewDatabasePath)
{
    CJumpPath* pjmp = pjmpFirst();
    for ( ; pjmp; pjmp = pjmpNext(pjmp) )
        pjmp->FixDBRefs(sOldDatabasePath, sNewDatabasePath);
}

#ifdef JumpStackPartiallyImplemented
CObList lstJumpStack;

void CJumpStack::AddJump( CViewPos* pvpos ) // Add a jump to the stack
	{
	lstJumpStack.AddHead( pvpos );
	}

CViewPos CJumpStack::PopJump() // Pop most recent jump off stack
	{
	CViewPos* pvpos = NULL;
	while ( !lstJumpStack.IsEmpty() )
		{
		pvpos = (CViewPos*) lstJumpStack.GetHead();
		lstJumpStack.RemoveHead();
		if ( Shw_App().bViewValid( pview ) // If valid view is found, jump to it

		}
	return pvpos;
	}
#endif // JumpStackPartiallyImplemented
