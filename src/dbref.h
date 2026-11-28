// dbref.h
// Brian Yoder  12-4-95

#ifndef Dbref_h
#define Dbref_h

#include "cdbllist.h" // For derivation
#include "cdblel.h" // For derivation

#include "ind.h" // For members

#include <iostream>
using namespace std;

class CMultipleMatchList;

class CDatabaseRef : public CDblListEl  // Hungarian: drf
{
private:
    Str8 m_sDatabasePath; // Path to database
    CIndexPtr m_pind; // Index of database
    Str8 m_sKey; // Name of marker index is sorted on

    BOOL m_bOkayToSkipDB;
    BOOL m_bOkayToSkipMkr;
    BOOL m_bDontCheckForOpenDB;

    friend class CDatabaseRefList;
        
#ifdef typWritefstream // 1.6.4ac
    void WriteProperties(Object_ofstream& obs) const;
#else
    void WriteProperties(Object_ostream& obs) const;
#endif
    static BOOL s_bReadProperties(Object_istream& obs, CDatabaseRef** ppdrf);

    CDatabaseRef( const CDatabaseRef& drf ) // Copy constructor
        { m_sDatabasePath = drf.m_sDatabasePath;
        m_pind = drf.m_pind;
        m_sKey = drf.m_sKey;
        m_bOkayToSkipDB = drf.m_bOkayToSkipDB;
        m_bOkayToSkipMkr = drf.m_bOkayToSkipMkr;
        m_bDontCheckForOpenDB = drf.m_bDontCheckForOpenDB; }
    BOOL bDocIsOpen() const { return pdoc(FALSE) != NULL; } // find out if database this refers to is open

public:
    CDatabaseRef(const char* pszDatabasePath, const char* pszMarker);
    CDatabaseRef(CIndex* pind); // used when we already have index and don't care about db name
    CIndex* pind(); // Access to index, returns owning index if mkr not found
    enum EFailureExplanation { fexOK, fexDBNotOpen, fexMarkerNotFound }; // Explanations for null pind
    CIndex* pind(EFailureExplanation& fex); // Access to index, supplies explanation if NULL
    CShwDoc* pdoc(BOOL bOpen=TRUE) const; // Access to document, opens if not already open if bOpen is TRUE
    CMarkerSet* pmkrset() const; // Access to marker set
    const Str8& sDatabasePath() const // Access to database path string
        { return m_sDatabasePath; }
    const Str8& sKey() const // Access to key string
        { return m_sKey; }
    
// 1999-11-01 TLB : bInsertable replaces bReadOnly and ties together three issues: DB open, DB is writable, and
// marker (if any) refers to record marker.
    BOOL bInsertable() const;  // Database is open and is not read-only, and marker (if any) is the record marker.
    
    BOOL bSearch(const CKey* pkey, CMultipleMatchList* pmmtlst); // search path for match to pkey (if doc is open)
    enum EDBRefSearchResult { dsrFound, dsrNoMatches, dsrDBNotOpen, dsrMarkerNotFound }; // possible returns from dsrSearch()
    EDBRefSearchResult dsrSearch(const CKey* pkey, CMultipleMatchList* pmmtlst); // search path for match to pkey
    BOOL bOkayToSkipDB() const { return m_bOkayToSkipDB; }
    BOOL bOkayToSkipMkr() const { return m_bOkayToSkipMkr; }
    BOOL bDontCheckForOpenDB() const { return m_bDontCheckForOpenDB; }

    void MarkerUpdated( CMarkerUpdate& mup ); // update marker text if name changed

	void WritePaths( std::ofstream& ostr ); // Write paths that are in active use

    void UpdatePath(const Str8 sNewPath); // Use if DB moved and path is to old location (but project hasn't moved)
};  // class CDatabaseRef


// --------------------------------------------------------------------------

class CDatabaseRefList : public CDblList  // Hungarian: drflst
{
public:
    CDatabaseRefList() {}; // Constructor
    CDatabaseRefList(CShwDoc *pdocCur, BOOL bIncludeCur = FALSE); // Constructor for list of like documents
    CDatabaseRefList( const CDatabaseRefList& drflst ); // Copy constructor
    ~CDatabaseRefList() { DeleteAll(); }

#ifdef typWritefstream // 1.6.4ac
    void WriteProperties(Object_ofstream& obs) const;
#else
    void WriteProperties(Object_ostream& obs) const;
#endif
    BOOL bReadProperties(Object_istream& obs);

    CDatabaseRef* pdrfFirst() const { return (CDatabaseRef*)pelFirst(); }
    CDatabaseRef* pdrfNext( const CDatabaseRef* pdrf) const { return (CDatabaseRef*)pelNext(pdrf); }
    CDatabaseRef* pdrfPrev( const CDatabaseRef* pdrf) const { return (CDatabaseRef*)pelPrev(pdrf); }
    CDatabaseRef* pdrfLast() const  { return (CDatabaseRef*)pelLast(); }    
    
    void Add(const char* pszPath, const char* pszKey) // Add another db to search path
        { Add( new CDatabaseRef(pszPath, pszKey) ); }
    void Add(const CDatabaseRef* pdrf) // Add another db to search path
        { CDblList::Add((CDblListEl*)pdrf); }

    // Delete element, pel is changed to Next if it exists, else Prev if it exists, else NULL
    void Delete( CDatabaseRef** ppdrf ) // Delete a db
        { CDblList::Delete( (CDblListEl**)ppdrf ); }

    void DeleteAll() { CDblList::DeleteAll(); } // Delete all elements       
    
    CDatabaseRef* pdrfRemove( CDatabaseRef** ppdrf ) // remove without deleting
        { return (CDatabaseRef*)CDblList::pelRemove( (CDblListEl**)ppdrf ); }

    void OnDocumentClose( CShwDoc* pdoc ); // handle references to document being closed
    BOOL bRefersTo( CIndexSet* pindset ); // check for any references to this index set
    BOOL bRefersTo( CShwDoc* pdoc ); // check for any references to this doc
    
    void MarkerUpdated( CMarkerUpdate& mup ); // update drfs

	void WritePaths( std::ofstream& ostr ); // Write paths that are in active use

    void DontCheckDiffPathsForDBRefs(const Str8& sDatabasePath); // All matching dbrefs should be skipped unconditionally if Doc is not open
    void OkayToSkipDBRefs(const Str8& sDatabasePath, const Str8& sKey = ""); // Mark all matching dbrefs as okay to skip
    void FixDBRefs(const Str8& sOldDatabasePath, const Str8& sNewDatabasePath); // Change all matching dbrefs to new pathname

    //------------------------------------------    
    // Interface elements.
    BOOL bModalView_Elements() { return FALSE; }
};  // class CDatabaseRefList

#endif  // Dbref_h