// shwdoc.h : interface of the CShwDoc class
//
/////////////////////////////////////////////////////////////////////////////


#ifndef _SHWDOC_H
#define _SHWDOC_H

#include "ind.h"  // class CIndexSet
#include "typ.h"  // class CDatabaseType

class Object_istream;  // obstream.h
class Object_ostream;  // obstream.h
class Field_ostream;  // sfstream.h
class SF_istream;  // sfstream.h

class CMarkerUpdate;
class CFilterUpdate;
class CLangEncUpdate;
class CSortOrderUpdate;


class CShwDoc : public CDocument
{
private:
    CIndexSet* m_pindset;
    BOOL m_bReadOnly;
    CTime m_timeLastLoaded;

public:
    // Presently each document/database owns a "search path" to other databases
    // or documents on the search list.
// removed 1-3-96 BJY  Now owned by database type
//  CSearchPathList m_lstSearchPath;

public:     // enum constants for import mode
    enum { iNormal=0, iDBTypeNotFound };


    // RE 1995-03-13
    //  The CShwDoc constructor was protected previously,
    //  but I needed to make it public so I could construct
    //  the class for testing purposes.
    
public: // create from serialization only
    CShwDoc();
    DECLARE_DYNCREATE(CShwDoc)


// Attributes
public:
// Operations
    void Update(CUpdate& up);
    void MarkerUpdated(CMarkerUpdate& mup);
    void FilterUpdated(CFilterUpdate& fup);
    void LangEncUpdated(CLangEncUpdate& lup);
    void SortOrderUpdated(CSortOrderUpdate& sup);

    void RecordModified( CRecLookEl* prel, BOOL bUpdateDateStamp=TRUE ); // record modified (not thru view), update indexes if necessary
private:    
    void MergeMarker( CMarker* pmkrTo, CMarker* pmkrFrom ); // change all occurences of pmkrFrom to pmkrTo in this database

public:
// Implementation
    CIndexSet* pindset() { return m_pindset; }
    CDatabaseType* ptyp() { return m_pindset->ptyp(); }
    CMarker* pmkrRecord() { return m_pindset->pmkrRecord(); }
    CMarkerSet* pmkrset();
    CFilterSet* pfilset();
    CInterlinearProcList* pintprclst();

    BOOL bReadOnly() const { return m_pindset && m_pindset->bReadOnly(); } // 1.2be Fix bug of crash when file fails to open, new from 1.2an
    void SetReadOnly(BOOL b) { m_bReadOnly = b; } // m_pindset->SetReadOnly(b); } // 1.1ea

    BOOL bWrite(const char* pszPath, BOOL bExport = FALSE, CIndex* pindExport = NULL, CMarkerSubSet* psubsetMarkersToExport = NULL, BOOL bCurrentRecord = FALSE, const CRecLookEl* prelCur = NULL, DWORD* pdwError = NULL);
    
#ifdef prjWritefstream // 1.6.4aa 
    void WriteProperties(Object_ofstream& obs);
#else
    void WriteProperties(Object_ostream& obs);
#endif
    static BOOL s_bReadProperties(Object_istream& obs);
    
    BOOL bExport(CIndex* pindCur, const CRecLookEl* prelCur);
    
	BOOL bNumber(CShwView* pview, CRecLookEl* prelCur, BOOL bDoBreak);

private:
    BOOL bRead(const char* pszPath, Str8& sMessage);
    BOOL bImport(const char* pszPath, const int nMode, const Str8& sMissingDBType, Str8& sMessage);
    BOOL bReadRecords(const char* pszPath, std::ifstream& ios, SF_istream& sfs);

    BOOL bSave(const char* pszPath, Str8& sMessage);

public:
    virtual ~CShwDoc();
    BOOL bNeedsRefresh();
    void OnCloseDocument();
    BOOL OnOpenDocument( const char* pszPathName );
    BOOL OnSaveDocument( const char* pszPathName );
    BOOL bSaveDocument();
	void ResetUndoForAllViews();
	void ResetUndoForAllViews( CRecord* prec );
//  virtual void Serialize(CArchive& ar);   // overridden for document i/o
    BOOL bValidateViews( BOOL bAutoSave = TRUE, CShwView* pviewDontValidate=NULL );
        // Call this function before closing or saving the document.
        // Returns whether all records currently being viewed are valid.
        // For each current record (which has been modified) validity checks
        // are done (e.g. range sets, CARLA field syntax).
        // If any one is not valid, this function makes its view active,
        // gives feedback and returns FALSE; the caller should allow
        // the user to continue to edit that record.
        // If all are valid, their keys are recomputed and, if changed
        // the record elements are positioned correctly.

    CShwView* pviewJumpTarget( CIndex* pind, CNumberedRecElPtr* pprel );
    // return a view based on pind if it's a jump target, else return NULL
    // 1998-06-18 MRP: First prefer a view whose index matches exactly;
    // otherwise return a corresponding record element in the view's index.

#ifdef _DEBUG
    void AssertValidity() const;
        // Assert validity of Shoebox-specific data.
    virtual void AssertValid() const;
#endif  // _DEBUG


protected:
#ifdef _MAC
    // Had to override this function on Mac platform to convert to and from the formatted
    // filename buffer that the common dialog wants to use.
    BOOL DoSave(const char* lpszPathName, BOOL bReplace=TRUE);
#endif
    virtual BOOL SaveModified();

public: // 1.4qzd
    virtual BOOL OnNewDocument();
    virtual void SetTitle(const char* pszDocumentPath);

// Generated message map functions
protected:
    //{{AFX_MSG(CShwDoc)
    afx_msg void OnFileSaveAs();
    afx_msg void OnFileSave();
//    afx_msg void OnFileClose();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif // _SHWDOC_H
