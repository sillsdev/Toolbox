// typ.h  Interface for Standard Format text markup type (1995-08-10)



#ifndef TYP_H
#define TYP_H

#include "set.h"  // classes CSet and CSetEl
#include "mkr.h"
#include "interlin.h"
#include "fil.h"
#include "jmp.h"
#include "printinf.h"
#include "number.h"
#include "export.h"
#include <fstream>

class CMarkerUpdate;
class CFilterUpdate;
class CLangEncUpdate;
class CSortOrderUpdate;

class CDatabaseTypeSet; // Forward reference for type set from database type
class RtfPageSetup;  // pgs.h
class Object_istream;  // obstream.h
class Object_ostream;  // obstream.h
// class CExportProcessSet;  // export.h
// class CExportProcess;  // export.h

// **************************************************************************

class CDatabaseType  // Hungarian: typ
{
private:
    Str8 m_sDescription;
    CMarkerSet m_mkrset;
    CFilterSet m_filset;
    CMarkerPtr m_pmkrRecord;
    CMarkerRefList m_mrflstSecKeys;
    Str8 m_sTemplate;
    CMarkerPtr m_pmkrDateStamp;
    CMarkerPtr m_pmkrTextRef; // 1.1ma Add text ref marker to database type
    CJumpPathSet m_jmpset; // search paths - TLB 1999-05-05
    // 2000-06-22 MRP: Use a Str8 instead of a CMarkerPtr,
    // so that users are not prevented from deleting the marker.
    Str8 m_sMarkerMultipleMatchShow; // additional field to view in multiple match dialog
    BOOL m_bModified;

    CPrintProperties m_prtp;    // Properties for printing records of this database.

    friend class CDatabaseTypeProxy;
    CDatabaseTypeProxy* m_ptrxMyProxy;
    CInterlinearProcList m_intprclst;
    int m_iInterlinCharWd; // Char width to use for interlinear display
    CRefCount m_refReadOnlyIndexSets;

    CExportProcessSet* m_pexpset;  // 1997-11-15 MRP

	BOOL m_bPreventNumbering; // 1999-02-16 TLB
	BOOL m_bNumberingDisabled; // 1999-02-16 TLB
    CNumberProcess* m_pnbrproc;  // 1999-02-03 TLB
	BOOL m_bHideFields; // 1.0be Turn hide fields on and off, so export process can be remembered when off
	Str8 m_sPlaySoundPath; // 1.5.0hc Remember play sound path

	Str8 m_sUnrecognizedSettingsInfo; // 1.0cm Don't lose settings info from later versions

private:    
    CDatabaseType(CLangEncSet* plngset, CDatabaseTypeProxy* ptrxMyProxy,
        BOOL bNew = FALSE);
public:
    ~CDatabaseType();
    
    const Str8& sName() const;
    const Str8& sDescription() const { return m_sDescription; }
    void SetDescription( const char* psz ) { m_sDescription = psz; }
    CMarkerSet* pmkrset() { return &m_mkrset; }
    CMarker* pmkrRecord() { return m_pmkrRecord; }
    // void SetRecordMkr( const char* pszMkr ) // set type's record marker
    //    { m_pmkrRecord = m_mkrset.pmkrSearch_AddIfNew( pszMkr ); }
    // Use these member functions INSTEAD OF assigning to the member variable
    void SetRecordMarker( const char* pszMarker );
    void ChangeRecordMarker( const char* pszMarker );
    const CMarkerRefList& mrflstSecKeys() const { return m_mrflstSecKeys; }
    CInterlinearProcList* pintprclst()
        { return &m_intprclst; }
    void DeleteInterlinearProcList() // Delete all interlinear processes, needed for correct destructor operation on database type list 
        { m_intprclst.DeleteAll(); }
    void ClearProcListRefs() // clear marker and dbtype pointers in interlin proc list
        { m_intprclst.ClearRefs(); }
    void SetInterlinearMkrs() // Update interlinear settings of markers from intprclst
        { m_mkrset.SetInterlinear( &m_intprclst ); }
    int iInterlinCharWd()
        { return m_iInterlinCharWd; }
    void SetInterlinCharWd( int i )
        { m_iInterlinCharWd = i; }
    CFilterSet* pfilset() { return &m_filset; }
    Str8 sTemplate() { return m_sTemplate; }
    void SetTemplate( Str8& sTemplate ) { m_sTemplate = sTemplate; }
    CMarker* pmkrDateStamp() { return m_pmkrDateStamp; }
    void SetDateStampMkr( CMarker* pmkr ) { m_pmkrDateStamp = pmkr; }
    CMarker* pmkrTextRef() { return m_pmkrTextRef; }
    void SetTextRefMkr( CMarker* pmkr ) { m_pmkrTextRef = pmkr; }
    BOOL bTextFile() const { return m_pmkrTextRef != NULL; } // 1.1ma Use existence of text reference marker instead of boolean
    const Str8& sPlaySoundPath() const { return m_sPlaySoundPath; }
    void SetPlaySoundPath( const char* psz ) { m_sPlaySoundPath = psz; }

	CPrintProperties* pprtp() { return &m_prtp; }   // Print-properties.
    CJumpPathSet* pjmpset() { return &m_jmpset; }
    CDatabaseTypeSet* ptypsetMyOwner() const;
    CDatabaseTypeProxy* ptrxMyProxy() const { return m_ptrxMyProxy; }

    void IncrementNumberOfRefs();
    void DecrementNumberOfRefs();

    void IncrementNumberOfRefsFromReadOnlyIndexSets()
        { m_refReadOnlyIndexSets.IncrementNumberOfRefs(); }
    void DecrementNumberOfRefsFromReadOnlyIndexSets()
        { m_refReadOnlyIndexSets.DecrementNumberOfRefs(); }
    BOOL bHasRefsFromReadOnlyIndexSets() const
        { return m_refReadOnlyIndexSets.bHasRefs(); }
	void WritePaths( std::ofstream& ostr ); // Write paths that are in active use
    
	BOOL bWriteProperties(const char* path, BOOL bForceWrite = FALSE) const;
    void WriteProperties(const char* path) const { bWriteProperties(path); }
#ifdef typWritefstream // 1.6.4ac
    void WriteProperties(Object_ofstream& obs) const;
#else
    void WriteProperties(Object_ostream& obs) const;
#endif
    BOOL bReadProperties(CNoteList& notlst, BOOL bLocateMissing = TRUE);
    BOOL bReadProperties(Object_istream& obs);

#ifdef typWritefstream // 1.6.4ac
    void TemplateWriteProperties(Object_ofstream& obs) const;
#else
    void TemplateWriteProperties(Object_ostream& obs) const;
#endif
    BOOL bTemplateReadProperties(Object_istream& obs);

    BOOL bMultiDictionaryFormatter() const;
    void SetMultiDictionaryFormatter(BOOL b);
    CExportProcessSet* pexpset() { return m_pexpset; }
    BOOL bExport(CIndex* pindCur, const CRecLookEl* prelCur,
            const Str8& sPathDatabase, Str8& sMessage);
	
    void SetMultipleMatchMkr(CMarker* pmkr);
    CMarker* pmkrMultipleMatchShow();
    // return marker of additional field to display in multiple matches 
    // User selected [None] in the list

    CNumberProcess* pnbrproc() { return m_pnbrproc; }
	void SetupNumbering(CMarker* pmkrRef, CMarker* pmkrTxt);
	void EnableNumbering() { m_bNumberingDisabled = m_bPreventNumbering; }
	void DisableNumbering();
	BOOL bAllowNumberingOpt() { return !m_bPreventNumbering; }
	BOOL bNumberingEnabled() { return !m_bNumberingDisabled; }
    BOOL bNumber(CShwView* pview, CRecLookEl* prelCur, BOOL bDoBreak);
	CMarker* pmkrRef() { return ( m_pnbrproc == NULL ) ? NULL: m_pnbrproc->pmkrRef(); }
	CMarker* pmkrTxt() { return ( m_pnbrproc == NULL ) ? NULL: m_pnbrproc->pmkrTxt(); }
	BOOL bHideFields() { return m_bHideFields; } // 1.0be Return hide fields on or off
	void SetHideFields( BOOL b ) { m_bHideFields = b; } // 1.0be Set hide fields on or off
    CExportProcess* pexpDefault() { return m_pexpset->pexpDefault(); } // 1.0bm Access to default export process

	BOOL bUnicode(); // Return true if db contains Unicode langs

    void WritePropertiesIfModified( BOOL bForceWrite=FALSE );
    void Update(CUpdate& up);
    void MarkerUpdated(CMarkerUpdate& mup);
    void FilterUpdated(CFilterUpdate& fup);
    void LangEncUpdated(CLangEncUpdate& lup);
    void SortOrderUpdated(CSortOrderUpdate& sup);
    void IndexUpdated(CIndexUpdate& iup);

    void OnDocumentClose( CShwDoc* pdoc ); // NULL out indexes of jump paths referring to sDoc

    BOOL bViewProperties(int iActivePage = 0);
    BOOL bViewProperties(int iActivePage, CMarker* pmkr, CFilter* pfil);
	BOOL bCCTable() { return ( sName().Find( "CCTable" ) == 0 ); } // 1.4qrg Add func to see if db type is CC Table


#ifdef _DEBUG
    void AssertValid() const;
#endif  // _DEBUG
};  // class CDatabaseType

// --------------------------------------------------------------------------
class CDatabaseTypeProxy : public CSetEl  // Hungarian: trx
{
private:
    Str8 m_sFileName;
    Str8 m_sVersion;  // 1998-03-12 MRP
    CDatabaseType* m_ptyp;
    
    friend class CDatabaseTypeSet;
    CDatabaseTypeSet* m_ptypsetMyOwner;
    
    CDatabaseTypeProxy(const char* pszName, const char* pszFileName,
            CDatabaseTypeSet* ptypsetMyOwner);
    
public:
    ~CDatabaseTypeProxy();
    
    const Str8& sFileName() const { return m_sFileName; }
    const Str8& sVersion() const { return m_sVersion; }
    BOOL bOpen() const { return ( m_ptyp != NULL ); }   
    CDatabaseTypeSet* ptypsetMyOwner() const { return m_ptypsetMyOwner; }
    virtual CSet* psetMyOwner();
    
    CDatabaseType* ptyp(CNoteList& notlst);
    void DecrementNumberOfRefs();
    Str8 sPath();
    
    void Update(CUpdate& up);
    
    virtual BOOL bCopy(CSetEl** ppselNew);
    virtual BOOL bModify();

private:
    CDatabaseType* ptypNew();
    static BOOL s_bReadProperties(Object_istream& obs, const char* pszName,
            const char* pszFileName, CDatabaseTypeSet* ptypsetMyOwner,
            CDatabaseType** pptyp);
    void WritePropertiesIfModified( BOOL bForceWrite=FALSE );

    static BOOL s_bReadProxy(const char* pszPath, CDatabaseTypeSet* ptypset,
            CDatabaseTypeProxy** pptrx);

#ifdef _DEBUG
    void AssertValid() const;
#endif  // _DEBUG
};  // class CDatabaseTypeProxy


// --------------------------------------------------------------------------

class CDatabaseTypeSet : public CSet  // Hungarian: typset
{
private:
    CLangEncSet* m_plngset;
    Str8 m_sSettingsVersion;  // 1998-03-12 MRP
    Str8 m_sSettingsDirPath;
    BOOL m_bReadFromString;
    
public:
    CDatabaseTypeSet(CLangEncSet* plngset, const char* pszSettingsVersion);
#ifdef _DEBUG
    CDatabaseTypeSet(CLangEncSet* plngset, const char* pszSettingsVersion,
            const char* pszProperties);
        // Read markup types from a string rather than from .typ files.
        // To be used in internal tests so that they contain their own data.
#endif  // _DEBUG
    ~CDatabaseTypeSet();
    
    CLangEncSet* plngset() { return m_plngset; }
    const Str8& sSettingsVersion() const { return m_sSettingsVersion; }
    const Str8& sSettingsDirPath() const { return m_sSettingsDirPath; }
    void SetDirPath( const char* pszPath ) { m_sSettingsDirPath = pszPath; }
    BOOL bReadFromString() const { return m_bReadFromString; }

    void ReadAllProxies(CNoteList& notlst);
    void WriteAllModified( BOOL bForceWrite=FALSE );
    BOOL bSaveProjectPaths(BOOL bForceWrite);
        
    CDatabaseTypeProxy* ptrxAdd( const char* pszName, const char* pszFileName );

    BOOL bSearch(const char* pszName, CDatabaseType** pptyp, CNoteList& notlst);
    CDatabaseType* ptypSearch_AddIfNew(const char* pszName, CNoteList& notlst);
    CDatabaseType* ptypNew(const char* pszName, const char* pszRecordMarker,
				const char* pszFileNameBase = NULL);
        // 1998-03-27 MRP: Used by word list and concordance to add a new type,
        // BUT ONLY IF it doesn't already exist.

    CDatabaseTypeProxy* ptrxFirst() const
        { return (CDatabaseTypeProxy*)pelFirst(); }
    CDatabaseTypeProxy* ptrxLast() const
        { return (CDatabaseTypeProxy*)pelLast(); }
    CDatabaseTypeProxy* ptrxNext( const CDatabaseTypeProxy* ptrxCur ) const
        { return (CDatabaseTypeProxy*)pelNext( ptrxCur ); }
    CDatabaseTypeProxy* ptrxPrev( const CDatabaseTypeProxy* ptrxCur ) const
        { return (CDatabaseTypeProxy*)pelPrev( ptrxCur ); }
    
    void Update(CUpdate& up);

    void OnDocumentClose( CShwDoc* pdoc ); // NULL out indexes of jump paths referring to sDoc
    BOOL bOkToClose( CShwDoc* pdoc ); // check for any jump path references to this doc
    
    void ViewElements(CDatabaseTypeProxy** pptrxToSelect);
    void ViewElements(CDatabaseType* ptypToSelect = NULL);
    virtual BOOL bAdd(CSetEl** ppselNew);
    virtual void Delete(CSetEl** ppsel);

#ifdef _DEBUG
    void AssertValid() const;
#endif  // _DEBUG

    CDatabaseTypeProxy* ptrxSearch(const char* pszName);

private:
    void AddAndWrite(CDatabaseType* ptyp);
};  // class CDatabaseTypeSet


class CDatabaseTypeSetListBox : public CSetListBox
{
public:
    CDatabaseTypeSetListBox(CDatabaseTypeSet* ptypset, CDatabaseTypeProxy** pptrx) :
        CSetListBox(ptypset, (CSetEl**)pptrx) {}

protected:
    int m_xName;
    int m_xFileName;
    
    virtual void InitLabels();
    virtual void DrawElement(CDC& cDC, const RECT& rcItem,
            const CDblListEl *pel);
    virtual BOOL bIsBold(CSetEl* psel);
};  // class CDatabaseTypeSetListBox

// **************************************************************************

#endif  // TYP_H
