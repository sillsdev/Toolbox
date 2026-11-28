// jmp.h  Interface for Multiple Jump Paths (1999-04-29)
//
// "And, jumping up, he got on to his feet and went into the
// Temple with them, walking and jumping and giving praise to God."
//                            Acts 3:8

#ifndef JMP_H
#define JMP_H

#include "set.h"    // classes CSet and CSetEl
#include "mkr.h"    // CMarkerSet
#include "dbref.h"  // CDatabaseRefList
#include "srt.h"    // MatchSetting
#include "Crecpos.h" // CRecPos

class CField;  // cfield.h
class CIndex;  // ind.h
class CKey;    // recset.h
class CRecLookEl;  // crlookel.h
class Object_istream;  // obstream.h
class Object_ostream;  // obstream.h
class CCheckingState;  // cck.h

#include "lbo.h"  // CSetListBox, CDblListListBox
#include "update.h"  // CUpdate

class CJumpPathSet;

// **************************************************************************

class CJumpPath : public CSetEl  // Hungarian: jmp
{
private:
    CJumpPathSet* m_pjmpsetMyOwner;
    friend class CJumpPathDlg;

    CMarkerSubSet m_subsetSourceFields;
    CDatabaseRefList m_drflstTarget;
    BOOL m_bEnforceRefIntegrity;
    BOOL m_bMatchCriteriaSetExplicitly;
    BOOL m_bMatchWhole;
    MatchSetting m_matset;

    // Prevent use of the assignment operator by making it private and omitting its definition.
    CJumpPath& operator=(const CJumpPath& jmp);

public:
    CJumpPath(const char* pszName, CJumpPathSet *pjmpsetMyOwner);

    CJumpPath(const char* pszName,
              const CDatabaseRefList &drflstTarget,
              CJumpPathSet *pjmpsetMyOwner);

    CJumpPath(const CJumpPath& jmp);

    ~CJumpPath();  // destructor

#ifdef typWritefstream // 1.6.4ac
    void WriteProperties(Object_ofstream& obs) const;
#else
    void WriteProperties(Object_ostream& obs) const;
#endif
    void ReadProperties(Object_istream& obs);
    
    CJumpPathSet* pjmpsetMyOwner() const { return m_pjmpsetMyOwner; }
    CSet* psetMyOwner() { return (CSet*) m_pjmpsetMyOwner; }
    CDatabaseRefList* pdrflstTarget() { return &m_drflstTarget; }
    BOOL bEnforceRefIntegrity() const { return m_bEnforceRefIntegrity; }
    BOOL bKeySetExplicitlyByUser() const { return m_bMatchCriteriaSetExplicitly; }
    void SetKeyCriteria(CKey& key) const;
    BOOL bMatchWhole() const {return m_bMatchWhole; }
    MatchSetting matset() const {return m_matset; }
    void SetMatchCriteriaFromKey(CKey* pkey, BOOL bMatchCriteriaSetExplicitly = TRUE);

    static BOOL s_bDefaultMatchWholeSetting( BOOL bIsDataLink ) { return TRUE; }
    static MatchSetting s_matsetDefaultSetting( BOOL bIsDataLink )
    { return (bIsDataLink)? CMCharOrder::matsetEvenIgnore: CMCharOrder::matsetSamePrimaryGroup; }

    BOOL bHasTargets() const { return (!m_drflstTarget.bIsEmpty()); }
    BOOL bDefaultPath() const { return m_subsetSourceFields.bIncludeAllElements(); }
    BOOL bIncludes(const CMarker* pmkr) const { return m_subsetSourceFields.bIncludes(pmkr); }

    CLangEnc* plngDefault(); // Get the language encoding for an arbitrary marker

    enum EPathSearchResult { psrNoRefs, psrFound, psrNoMatches, psrCancel }; // possible returns from psrSearch()
    // Call psrSearch for db ref list
    // Values in mmtlst and bEnableInsert are defined only if search functions return psrFound or psrNoMatches
    EPathSearchResult psrSearch(const CKey* pkey, CMultipleMatchList &mmtlst, BOOL &bEnableInsert);

    void OnDocumentClose( CShwDoc* pdoc ); // handle references to document being closed
    BOOL bRefersTo( CIndexSet* pindset ); // check for any references to this index set
    BOOL bRefersTo( CShwDoc* pdoc ); // check for any references to this doc
    
    void MarkerUpdated( CMarkerUpdate& mup ); // update drfs

    void DontCheckDiffPathsForDBRefs(const Str8& sDatabasePath); // All matching dbrefs should be skipped unconditionally if Doc is not open
    void SkipDBRefs(const Str8& sDatabasePath, const Str8& sKey = ""); // Mark all matching dbrefs as okay to skip
    void FixDBRefs(const Str8& sOldDatabasePath, const Str8& sNewDatabasePath); // Change all matching dbrefs to new pathname

    BOOL bCheckDataLink(const Str8& sData, BOOL& bMatched, BOOL& bEnableInsert);

    BOOL bCopy(CSetEl** ppselNew);
    BOOL bModify();

};  // class CJumpPath

// --------------------------------------------------------------------------

class CJumpPathSet : public CSet  // Hungarian: filset
{
private:
    CMarkerSet* m_pmkrset;
    CJumpPath* m_pjmpDefault;
    
public:
    CJumpPathSet(CMarkerSet* pmkrset);
    ~CJumpPathSet() {}

    CMarkerSet* pmkrset() { return m_pmkrset; }

#ifdef typWritefstream // 1.6.4ac
    void WriteProperties(Object_ofstream& obs) const;
#else
    void WriteProperties(Object_ostream& obs) const;
#endif
    BOOL bReadProperties(Object_istream& obs);

    // Overrides of First, Next, etc. needed for type safe return value
    CJumpPath* pjmpSearch( const char* psz ) const // Perform a search
        { return (CJumpPath*) pselSearch( psz ); }
    
    CJumpPath* pjmpFirst() const // First jump path
        { return (CJumpPath*) pselFirst(); }
    CJumpPath* pjmpLast() const // Last jump path
        { return (CJumpPath*) pselLast(); }
    CJumpPath* pjmpNext( const CJumpPath* pjmpCur ) const // after current
        { return (CJumpPath*) pselNext( pjmpCur ); }
    CJumpPath* pjmpPrev( const CJumpPath* pjmpCur ) const // before current
        { return (CJumpPath*) pselPrev( pjmpCur ); }
        
    virtual void Add( CJumpPath* pjmpNew );
    BOOL bAddDefaultJumpPath( const CDatabaseRefList &drflstTarget );
    void UpdateDefaultJumpPathSetting(CJumpPath* pjmpModified);
    CJumpPath* pjmpDefault() const // Default jump path, NULL if none
        { return m_pjmpDefault; }

    // Remove the jump path from the set without deleting it.
    virtual CSetEl* pselRemove(CSetEl* psel);
    // Delete jump path, set pointer to NULL
    virtual void Delete( CSetEl** ppsel );

    BOOL bAdd(CSetEl** ppselNew);

    enum EJumpSearchResult { jsrCancel, jsrJump, jsrJumpInsert }; // possible returns from jsrSearch()
    EJumpSearchResult jsrSearch(CMarker* pmkrSource, Str8& sWord, CNumberedRecElPtr* pprel, CIndex** ppind, CShwDoc** ppdoc,
                CDatabaseType* ptyp, CLangEnc* plng, BOOL bIsJumpInsert, BOOL bShowDlg) const; // Call iSearch for all paths
    
    void OnDocumentClose( CShwDoc* pdoc ); // handle references to document being closed
    BOOL bRefersTo( CIndexSet* pindset ); // check for any references to this index set
    BOOL bRefersTo( CShwDoc* pdoc ); // check for any references to this doc
    
    void MarkerUpdated( CMarkerUpdate& mup ); // update drfs


    void DontCheckDiffPathsForDBRefs(const Str8& sDatabasePath); // All matching dbrefs in all jump paths should be skipped unconditionally if Doc is not open
    void SkipDBRefs(const Str8& sDatabasePath, const Str8& sKey = ""); // Mark all matching dbrefs in all jump paths as okay to skip
    void FixDBRefs(const Str8& sOldDatabasePath, const Str8& sNewDatabasePath); // Change all matching dbrefs in all jump paths to new pathname
};  // class CJumpPathSet

#ifdef JumpStackPartiallyImplemented
class CViewPos : public CObject // Used for stack of jumps for return from jump
{
public:
	CShwView* pview;
	CRecPos rps;
};

class CJumpStack // Stack of jumps for return from jump
{
public:
	void AddJump( CViewPos* vpos ); // Add a jump to the stack
	CViewPos PopJump(); // Pop most recent jump off stack
};
#endif // JumpStackPartiallyImplemented


#endif  // JMP_H
