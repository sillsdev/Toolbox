#include "stdafx.h"
#include "toolbox.h"
#include "shw.h"
#include "shwdoc.h"
#include "recset.h"
#include "spath_d.h"
#include "multmat.h"
#include "project.h"  // Shw_pProject()->bOpenMovedFile()
#include "obstream.h"  // Object_istream, Object_ostream
#include "doclist.h" // CDocList - 03/29/99 TLB
#include "dbref.h"

CDatabaseRef::CDatabaseRef(const char* pszDatabasePath, const char* pszKey)
{
    m_sDatabasePath = pszDatabasePath;
    m_sKey = pszKey;
    m_pind = NULL;
    m_bOkayToSkipDB = FALSE;
    m_bOkayToSkipMkr = FALSE;
    m_bDontCheckForOpenDB = FALSE;
}

// Special constructor used to create temp dbref when we already have index
// and don't care about db name. Note: some member functions will ASSERT with
// this type of dbref.
CDatabaseRef::CDatabaseRef(CIndex* pind)
{
    ASSERT(pind);
    m_pind = pind;
}

CIndex* CDatabaseRef::pind(EFailureExplanation& fex) // Access to index, supplies explanation if NULL
{
    fex = fexOK;
    if ( !m_pind ) // If no index, try to get one
        {
        CShwDoc* pdocTmp = pdoc(FALSE); // FALSE = don't open if not already open
        if ( !pdocTmp ) // If database not open, return null index
            {
            fex = fexDBNotOpen;
            return NULL;
            }
        CMarker* pmkr = pdocTmp->pmkrset()->pmkrSearch(m_sKey); // get specified marker, correctly returns NULL if m_sKey is a null string
        if ( pmkr ) // If key found, return index of that key
            {
            CLangEnc* plng = pmkr->plng();
            CSortOrder* psrtPriKey = plng->psrtDefault();
            CMarkerRefList mrflstSecKeys;  // no secondary sort fields
            // NOTE!!! The hardcoded 100 below should be changed!
            m_pind = pdocTmp->pindset()->pindFind_AddIfNew(pmkr, psrtPriKey,
                &mrflstSecKeys, 100, FALSE);
            }
        else // Else (m_sKey is null string or not found)
            {
            fex = fexMarkerNotFound;
            return NULL;
            }
        }
    ASSERT(m_pind);
    return m_pind;
}

CIndex* CDatabaseRef::pind() // Access to index, returns owning index if mkr not found
{
    if ( !m_pind ) // If no index, try to get one
        {
        CShwDoc* pdocTmp = pdoc(FALSE); // FALSE = don't open if not already open
        if ( !pdocTmp ) // If database not open, return null index
            return NULL;
        CMarker* pmkr = pdocTmp->pmkrset()->pmkrSearch(m_sKey); // get specified marker, correctly returns NULL if m_sKey is a null string
        if ( pmkr ) // If key found, return index of that key
            {
            CLangEnc* plng = pmkr->plng();
            CSortOrder* psrtPriKey = plng->psrtDefault();
            CMarkerRefList mrflstSecKeys;  // no secondary sort fields
            // NOTE!!! The hardcoded 100 below should be changed!
            m_pind = pdocTmp->pindset()->pindFind_AddIfNew(pmkr, psrtPriKey,
                &mrflstSecKeys, 100, FALSE);
            }
        else // Else (m_sKey is null string or not found) return owning index
            m_pind = pdocTmp->pindset()->pindRecordOwner();
        }
    ASSERT(m_pind);
    return m_pind;
}

CShwDoc* CDatabaseRef::pdoc(BOOL bOpen) const // Access to document
{
    ASSERT(!m_sDatabasePath.IsEmpty());
    if (bOpen)
        return Shw_papp()->pdocOpen( m_sDatabasePath ); // Get doc, opening if not already open
    else
        return Shw_papp()->pdoc( m_sDatabasePath ); // Get doc, NULL if not open
}

// 1999-11-01 TLB : bInsertable replaces bReadOnly and ties together three issues: DB open,
// DB is writable, and marker (if any) refers to record marker.
// This final condition prevents user from attempting to do an automatic insertion of failed
// jump data using a dbref that doesn't reference the record marker. This is because "insert"
// would be ambiguous in this case: it might mean to insert a new record (with an unknown
// record name) and insert the target field with the failed data (what about the template???);
// Or it could mean to seek a specific record in the target database and either add or
// modify the the target field with the failed data. The interface would be complex and
// unweildly, so better to force user to cancel and take the needed action manually.
BOOL CDatabaseRef::bInsertable() const
{
    // Database is open and is not read-only, and marker (if any) is the record marker.

    // 1) Is the database open?
    if ( !bDocIsOpen() )
        return FALSE;

    // 2) Is the database writable?
    ASSERT(pdoc());
    if ( pdoc()->bReadOnly() || (Shw_papp()->pProject()->iLockProject() >= 10) ) // 1.2gx Fix bug (1.2ca) of project lock level 10 allowing insert char
        return FALSE;

    // 3) If this ref refers to a specific marker, is it the record marker?
    // Return FALSE if this dbref refers to record marker or does not refer to any record marker.
    CMarker* pmkrRef = pmkrset()->pmkrSearch(m_sKey); // get specified marker, correctly returns NULL if m_sKey is a null string
    if ( !pmkrRef )
        return TRUE;
    CMarker* pmkrRec = pmkrset()->pmkrRecord();
    ASSERT(pmkrRec);
    return ( pmkrRec == pmkrRef );
}

CMarkerSet* CDatabaseRef::pmkrset() const // Access to marker set
{
    ASSERT( pdoc() );
    return pdoc()->pmkrset();
}   

BOOL CDatabaseRef::bSearch(const CKey* pkey, CMultipleMatchList* pmmtlst)
{
    if (!pind()) // make sure index exists
        {
        ASSERT(FALSE); // TLB:06-07-99
        return FALSE;
        }
    CSearchRecordSet recset( pind(), (CKey*)pkey ); // Do lookup (cast away constness rather than fix base class
    return recset.bSearch(pmmtlst, this); // add any matches to multiple match list
}

CDatabaseRef::EDBRefSearchResult CDatabaseRef::dsrSearch(const CKey* pkey, CMultipleMatchList* pmmtlst)
{
    EFailureExplanation fex;
    if (!pind(fex))
        {
        switch ( fex )
            {
            case fexDBNotOpen:
                return dsrDBNotOpen;
            case fexMarkerNotFound:
                return dsrMarkerNotFound;
            default:
                ASSERT(FALSE); // Unknown error
                return dsrNoMatches;
            }
        }
    CSearchRecordSet recset( pind(), (CKey*)pkey ); // Do lookup (cast away constness rather than fix base class
    // add any matches to multiple match list
    return ( recset.bSearch(pmmtlst, this) ) ? dsrFound : dsrNoMatches;
}

void CDatabaseRef::MarkerUpdated( CMarkerUpdate& mup ) // handle marker name change
{
    if ( !mup.bNameModified() ) // only care about rename
        return;

    CShwDoc* pdocT = pdoc(FALSE);
    if ( pdocT && pdocT->ptyp() == mup.pmkr()->pmkrsetMyOwner()->ptyp()
            && !strcmp( m_sKey, mup.pszOldName() ) ) // was this marker changed?
        m_sKey = mup.pmkr()->sName();
}

void CDatabaseRef::WritePaths( std::ofstream& ostr ) // Write paths that are in active use
{
	ostr << m_sDatabasePath << "\n";
}

void CDatabaseRef::UpdatePath(const Str8 sNewPath) // Use if DB moved and path is to old location (but project hasn't moved)
{
    ASSERT(!m_pind); // TLB - 1999-06-11: Prevent use of this function once index has been established.
    ASSERT(!_stricmp(sGetFileName(sNewPath, TRUE), sGetFileName(m_sDatabasePath, TRUE)));

	m_sDatabasePath = sNewPath;
}

static const char* psz_File = "File";
static const char* psz_mkr = "mkr";
static const char* psz_drf = "drf";

#ifdef typWritefstream // 1.6.4ac
void CDatabaseRef::WriteProperties(Object_ofstream& obs) const 
#else
void CDatabaseRef::WriteProperties(Object_ostream& obs) const 
#endif
{
    obs.WriteBeginMarker(psz_drf);
    ASSERT(!m_sDatabasePath.IsEmpty());
    obs.WriteProjectPath(psz_File, m_sDatabasePath);
    if (m_sKey.GetLength()) // m_sKey not used in some cases
        CMarker::s_WriteRef( obs, m_sKey );
    obs.WriteEndMarker(psz_drf);
}

BOOL CDatabaseRef::s_bReadProperties(Object_istream& obs, CDatabaseRef** ppdrf)
{
    if ( !obs.bReadBeginMarker(psz_drf) )
        return FALSE;
    Str8 sDatabasePath;
    Str8 sKey;
    while ( !obs.bAtEnd() )
        {
        if ( obs.bReadProjectPath(psz_File, sDatabasePath) )
            ;
        else if ( CMarker::s_bReadRef( obs, sKey ) )
            ;
        else if ( obs.bEnd(psz_drf) )
            break;
        }

    if ( sDatabasePath.IsEmpty() )
        return TRUE;
        
    if ( obs.bLocateMissingReferences() && !bFileExists(sDatabasePath) )
        {
        Str8 locatedPath = sDatabasePath;
        if (Shw_pProject()->bOpenMovedFile(&locatedPath))
            sDatabasePath = locatedPath;
        }
        
    *ppdrf = new CDatabaseRef(sDatabasePath, sKey);
    return TRUE;            
}

////////////////////////////////////////////////////////////////////////////////
// Constructor for list of like documents. The new drflst contains the
// full path names of all open databases of the same type as the passed
// document. If bIncludeCur is TRUE, the passed doc is also listed in drflst.
CDatabaseRefList::CDatabaseRefList(CShwDoc *pdocCur, BOOL bIncludeCur)
{
    ASSERT( pdocCur );
    CTypedDocList doclst( pdocCur->ptyp()); // get list of currently open documents
    CShwDoc *pdocExclude = ( bIncludeCur ) ? NULL : pdocCur;
    for ( CShwDoc *pdoc = doclst.pdocFirst( pdocExclude ); pdoc; pdoc = doclst.pdocNext() )
        {
            ASSERT(pdoc->GetPathName().GetLength());
            Add( sUTF8( pdoc->GetPathName() ), NULL); // 1.4qxu Upgrade Add for Unicode build
            // The key doesn't matter for these purposes.  Also note that
            // GetPathName never returns empty because Shoebox forces new
            // databases to have a file name.
        }
}

CDatabaseRefList::CDatabaseRefList( const CDatabaseRefList& drflst ) // Copy constructor
{
    for ( const CDatabaseRef* pdrf = drflst.pdrfFirst(); pdrf; pdrf = drflst.pdrfNext( pdrf ) )
        Add( new CDatabaseRef( *pdrf ) );
}

// --------------------------------------------------------------------------

void CDatabaseRefList::MarkerUpdated( CMarkerUpdate& mup ) // update drfs
{
    for ( CDatabaseRef* pdrf = pdrfFirst(); pdrf; pdrf = pdrfNext(pdrf) )
        pdrf->MarkerUpdated(mup);
}

void CDatabaseRefList::WritePaths( std::ofstream& ostr ) // Write active paths to ostr
{
    for ( CDatabaseRef* pdrf = pdrfFirst(); pdrf; pdrf = pdrfNext(pdrf) )
		pdrf->WritePaths( ostr );
}

static const char* psz_drflst = "drflst";

#ifdef typWritefstream // 1.6.4ac
void CDatabaseRefList::WriteProperties(Object_ofstream& obs) const
#else
void CDatabaseRefList::WriteProperties(Object_ostream& obs) const
#endif
{
    obs.WriteBeginMarker(psz_drflst);
    
    for ( CDatabaseRef* pdrf = pdrfFirst(); pdrf; pdrf = pdrfNext(pdrf) )
        pdrf->WriteProperties(obs);
        
    obs.WriteEndMarker(psz_drflst);
}

BOOL CDatabaseRefList::bReadProperties(Object_istream& obs)
{
    if ( !obs.bReadBeginMarker(psz_drflst) )
        return FALSE;

    while ( !obs.bAtEnd() )
        {
        CDatabaseRef* pdrf = NULL;
        if ( CDatabaseRef::s_bReadProperties(obs, &pdrf) )
            {
            if ( pdrf )
                Add(pdrf);
            }
        else if ( obs.bEnd(psz_drflst) ) // Look for end marker; skip unexpected field
            break;
        }
        
    return TRUE;
}

// NULL out pointers to indexes of document about to close
void CDatabaseRefList::OnDocumentClose( CShwDoc* pdoc )
{
    CDatabaseRef* pdrf = pdrfFirst();
    for ( ; pdrf; pdrf = pdrfNext(pdrf) )
        if ( pdrf->m_pind && pdrf->m_pind->pindsetMyOwner() == pdoc->pindset() ) // If this index is part of the index set of the deleted document, clear index pointer
            {
            pdrf->m_pind = NULL; // trash pointer to index, it's about to be deleted
            pdrf->m_bOkayToSkipDB = FALSE;
            pdrf->m_bOkayToSkipMkr = FALSE;
            pdrf->m_bDontCheckForOpenDB = FALSE;
            }
}

BOOL CDatabaseRefList::bRefersTo( CIndexSet* pindset ) // check for any references to this index set
{
    CDatabaseRef* pdrf = pdrfFirst();
    for ( ; pdrf; pdrf = pdrfNext(pdrf) ) // For each database reference, see if its index is in the index set
        if ( pdrf->m_pind && pdrf->m_pind->pindsetMyOwner() == pindset ) // If this index is part of the index set, return true
            return TRUE;
    return FALSE;
}

BOOL CDatabaseRefList::bRefersTo( CShwDoc* pdoc ) // check for any references to this doc
{
    CDatabaseRef* pdrf = pdrfFirst();
    for ( ; pdrf; pdrf = pdrfNext(pdrf) )
        {
        if ( !_stricmp( pdrf->sDatabasePath(),  sUTF8( pdoc->GetPathName() ) ) ) // compare doc names, index may not exist // 1.4qxv Upgrade _stricmp for Unicode build
            return TRUE;
        }
    return FALSE;
}

// All matching dbrefs should be skipped unconditionally if Doc is not open
void CDatabaseRefList::DontCheckDiffPathsForDBRefs(const Str8& sDatabasePath)
{
    CDatabaseRef* pdrf = pdrfFirst();
    for ( ; pdrf; pdrf = pdrfNext(pdrf) )
        if ( !_stricmp(pdrf->m_sDatabasePath, sDatabasePath) )
            pdrf->m_bDontCheckForOpenDB = TRUE;
}

// Mark all matching dbrefs paths as okay to skip
void CDatabaseRefList::OkayToSkipDBRefs(const Str8& sDatabasePath, const Str8& sKey)
{
    CDatabaseRef* pdrf = pdrfFirst();
    for ( ; pdrf; pdrf = pdrfNext(pdrf) )
        if ( !_stricmp(pdrf->m_sDatabasePath, sDatabasePath) )
            {
            if ( sKey == "" )
                {
                pdrf->m_bOkayToSkipDB = TRUE;
                }
            else if ( sKey == pdrf->m_sKey )
                {
                pdrf->m_bOkayToSkipMkr = TRUE;
                }
            }
}

// CDatabaseRefList all matching dbrefs to new pathname
void CDatabaseRefList::FixDBRefs(const Str8& sOldDatabasePath, const Str8& sNewDatabasePath)
{
    CDatabaseRef* pdrf = pdrfFirst();
    for ( ; pdrf; pdrf = pdrfNext(pdrf) )
        if ( !_stricmp(pdrf->m_sDatabasePath, sOldDatabasePath) )
            {
            ASSERT(!pdrf->m_pind);
            pdrf->m_sDatabasePath = sNewDatabasePath;
            }
}
