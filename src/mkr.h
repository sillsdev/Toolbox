// mkr.h  Interface for Standard Format text markup (1995-02-21)

#ifndef MKR_H
#define MKR_H

#include "set.h"  // classes CSet and CSetEl
#include "lng.h"
#include "patch.h"
#include "not.h"   

#include "update.h"  // CUpdate
#include "lbo.h"  // class CSetListBox
#include "cbo.h"  // class CSetComboBox

class Object_istream;  // obstream.h
class Object_ostream;  // obstream.h
class CDataExchange;
class CJumpPath; // jmp.h


// **************************************************************************

// An SIL Standard Format marker corresponds to a type of text element
// which has a distinct nature or function in a document or database.
// The marker tagging a string identifies it with other text elements
// of the same kind and distinguishes it from those which are different.

typedef unsigned int Level;  // Hungarian: lev

// The marker object consists of descriptive information.
// E.g. The "d" marker represents a "Definition" text element.
// The language encoding for the text is "English".
//      CMarker* pmkr = new CMarker("d", "Definition", plngEnglish);
//      ASSERT(pmkr->sMarker() == "d");
//      ASSERT(pmkr->sName() == "Definition");
//      ASSERT(pmkr->sLangEnc() == "English");

class CDataItemSet;  // crngset.h
class CRangeSet;  // crngset.h

class CMarker : public CSetEl  // Hungarian: mkr
{
private:
    Str8 m_sFieldName;  // e.g. "Definition", like a Word style name
    Str8 m_sDescription;
    CLangEncPtr m_plng;  // reference to the language encoding, e.g. "English"    
    int m_iInterlinear; // True if an interlinear line
    CRangeSet* m_prngset;  // 1999-06-07 MRP: Remove dependency on "crngset.h"
    CFont m_fnt;    // Font used to display field contents
    COLORREF m_rgbColor; // color used to display field contents
    BOOL m_bUseDefaultFont; // If TRUE, use font from language encoding
    int m_iLineHeight; // height of a line in pixels
    int m_iAscent; // space between base line and top of the character cell
    int m_iOverhang; // for italic: amount the top is skewed past the bottom
    BOOL m_bNoWordWrap; // inhibit auto wrap / reshape in fields using this marker

    // TLB 1999-08-09
    BOOL m_bMultipleItemData;
    BOOL m_bMultipleWordItems;
    BOOL m_bMustHaveData;
    
    static int s_iInterlinear;
    static int s_iFirstInterlinear;
    
    // Markers are interrelated by a hieararchy similar to an outline.
    // Each marker except the record marker, which is at the highest level,
    // is under some particular marker (i.e. the marker "over this").
    // By default all other markers are under the record marker.
    // 1995-12-02 MRP: Temporarily all markers default to NULL over this.
    Str8 m_sMarkerOverThis;  // read from .typ file as a forward reference
    CMarker* m_pmkrOverThis;  // NULL for the record marker
    Level m_lev;  // Higher levels have lesser values: zero for record marker

    Str8 m_sMarkerFollowingThis;
    CMarker* m_pmkrFollowingThis;

    BOOL m_bCharStyle;  
    int m_iSect;  // Insert a section break
    BOOL m_bHidden;
    BOOL m_bSubfield;
    
    CRefCount m_refFields;  // number of fields referring to this

    // In order for a marked string location to search for a substring marker,
    // the string marker must refer back to its owning marker set.  
    friend class CMarkerSet;
    CMarkerSet* m_pmkrsetMyOwner;
     
    //08-18-1997 Added iRangeSetCharacter & bRangeSetSpaceAllowed to Paramterlist
    CMarker(const char* pszName,
            CLangEnc* plng,
            CMarkerSet* pmkrsetMyOwner,
            const char* pszFieldName = "",
            const char* pszDescription = "",
            const char* pszMarkerOverThis = "",
            const char* pszMarkerFollowingThis = "",
            //const char* pszRangeSet, int iRangeSetCharacter, BOOL bRangeSetSpaceAllowed,
            CFontDef* pfntdef = NULL,
            BOOL bMultipleItemData = TRUE,
            BOOL bMultipleWordItems = FALSE,
            BOOL bMustHaveData = FALSE,
            BOOL bNoWordWrap = FALSE,
            BOOL bCharStyle = FALSE,
            int iSect = 0,
            BOOL bHidden = FALSE,
            BOOL bSubfield = FALSE);
    
    //08-18-1997 Added iRangeSetCharacter & bRangeSetSpaceAllowed to Paramterlist            
    static BOOL s_bConstruct(const char* pszName,
        const char* pszDescription, const char* pszFieldName,
        CLangEnc* plng,
        const char* pszMarkerOverThis, const char* pszMarkerFollowingThis,
        //const char* pszRangeSet, int iRangeSetCharacter, BOOL bRangeSetSpaceAllowed,
        CFontDef* pfntdef,
        BOOL bMultipleItemData, BOOL bMultipleWordItems, BOOL bMustHaveData,
        BOOL bNoWordWrap, BOOL bCharStyle, int iSect, BOOL bHidden, BOOL bSubfield,
        CMarkerSet* pmkrsetMyOwner, CMarker** ppmkr, CNote** ppnot);
    static BOOL s_bReadProperties(Object_istream& obs,
        CMarkerSet* pmkrsetMyOwner, CMarker** ppmkr);
public:
	Str8 m_sUnrecognizedSettingsInfo; // 1.0cp Don't lose settings info from later versions

public:
    ~CMarker();  // destructor
    
    const Str8& sMarker() const { return sName(); }
    const Str8& sFieldName() const { return m_sFieldName; }
    const Str8& sDescription() const { return m_sDescription; }

    // references by settings and references by fields are maintained separately by
    // CMarkerPtr (for settings) and CFieldMarkerPtr for fields
    NumberOfRefs numRefs() const { return CSetEl::numRefs() + m_refFields.numRefs(); }
    BOOL bHasFieldRefs() const { return m_refFields.bHasRefs(); }
    BOOL bHasSettingsRefs() const { return CSetEl::bHasRefs(); } // is marker used in any settings?
    virtual BOOL bHasRefs() const { return bHasSettingsRefs() || bHasFieldRefs(); }
    virtual BOOL bDeletable() { return !bHasRefs(); } // Return whether this marker is eligible to be deleted,
    void IncrementNumberOfFieldRefs() { m_refFields.IncrementNumberOfRefs(); }
    void DecrementNumberOfFieldRefs() { m_refFields.DecrementNumberOfRefs(); }

    CLangEnc* plng() const { return m_plng; }
    const Str8& sLangEnc() const { return m_plng->sName(); }

    // 1999-11-17 TLB: Tells whether anything can be checked for this marker
    BOOL bHasConsistencyCheckConditions(BOOL bCheckDataDefn = TRUE,
                                        BOOL bCheckRangeSets = TRUE,
                                        BOOL bCheckRefConsistency = TRUE) const;
    BOOL bUseRangeSet() const { return (m_prngset != NULL); }  // 1999-09-08 TLB
    BOOL bRangeSetEnabled() const;  // 1999-06-07 MRP
    CRangeSet* prngset() const { return m_prngset; }
    
    BOOL bNoWordWrap() const { return m_bNoWordWrap; } // if TRUE, don't auto wrap fields using this marker
    
    // font functions
    const CFont* pfnt() const { return( m_bUseDefaultFont ? plng()->pfnt() : &m_fnt ); }
    int iLineHeight() const { return( m_bUseDefaultFont ? plng()->iLineHeight() : m_iLineHeight ); }
    int iAscent() const { return( m_bUseDefaultFont ? plng()->iAscent() : m_iAscent ); }
    int iOverhang() const { return( m_bUseDefaultFont ? plng()->iOverhang() : m_iOverhang ); }
    BOOL bUsingDefaultFont() const { return m_bUseDefaultFont; }
    void SetFont( CFontDef* pfntdef ); // Change font
    const CFont* pfntDlg() const;
    COLORREF rgbColor() const { return( m_bUseDefaultFont ? plng()->rgbColor() : m_rgbColor ); }

	BOOL bRightToLeft() { return( plng()->bRightToLeft() ); } // 1.2bd Make marker shorthand for rtl
	BOOL bRTLRightJustify() { return( plng()->bRTLRightJustify() ); } // 1.2bd Make option for rtl right justify

    // Replacements for standard device context display functions.
    // If this marker has explicitly chosen font and color properties,
    // use them; otherwise use its language encoding's properties.
    CFont* SelectFontIntoDC(CDC* pDC) const;
        // Select this marker's font into the device context.
        // Returns the font being replaced.
    CFont* SelectDlgFontIntoDC(CDC* pDC) const;
        // Select this marker's dialog box font into the
        // device context. Returns the font being replaced.
        // Use this font when the display height is restricted.
    void SetTextAndBkColorInDC(CDC* pDC, BOOL bHighlight) const;
        // Set the device context's text and background colors
        // to those chosen for this marker.
        // If bHighlight, use the appropriate system-defined colors.
    void ExtTextOut(CDC* pDC, int ixBegin, int iyBegin,
            int nOptions, const CRect* prect,
            const char* pszUnderlying, int lenUnderlying,
            int lenContext) const;
        // Display a substring rendered in its surface form
        // using the device context's current font and color.

    BOOL bInterlinear() const { return ( m_iInterlinear != 0 ); }
    BOOL bFirstInterlinear() const;

    CMarker* pmkrOverThis() const { return m_pmkrOverThis; }
    Level lev() const { return m_lev; }
    
    BOOL bUnder(const CMarker* pmkr) const;
        // Return whether this marker is under pmkr, either directly or
        // indirectly in the outline hierarchy.
    BOOL bOver(const CMarker* pmkr) const;
        // Return whether this marker is over pmkr (i.e. pmkr is under
        // this marker), either directly or indirectly.
    BOOL bEqualOrOver(const CMarker* pmkr) const;
        // Return whether this marker is or is over pmkr (i.e. pmkr is under
        // this marker), either directly or indirectly.
    const CMarker* pmkrLowestOverBoth(const CMarker* pmkr) const;
        // Return the marker (at the lowest level) which is over both
        // this marker and pmkr (i.e. they are both under it).
        // NOTE: DO NOT CALL THIS FUNCTION IF this marker and pmkr
        // are either under or over one another, i.e. if either of
        // the above functions returns TRUE.
    BOOL bAtHigherLevelThan(const CMarker* pmkr) const;
        // Return whether this marker is at a higher level than pmkr
        // in the outline hierarchy. This is more general than "over".

    CMarker* pmkrFollowingThis() const { return m_pmkrFollowingThis; }
        
    BOOL bCharStyle() const { return m_bCharStyle; }
    BOOL bParStyle() const { return !m_bCharStyle; }
    int iSect() const { return m_iSect; }
    BOOL bHidden() const { return m_bHidden; }
    void SetHidden( BOOL b ) { m_bHidden = b; }
    BOOL bSubfield() const { return m_bSubfield; }

    // 1999-08-13 MRP
    BOOL bMultipleItemData() const { return m_bMultipleItemData; }
    BOOL bMultipleWordItems() const { return m_bMultipleWordItems; }
    BOOL bAllowEmptyContents() const { return !m_bMustHaveData; }

    CMarkerSet* pmkrsetMyOwner() const { return m_pmkrsetMyOwner; }
    virtual CSet* psetMyOwner();

    // Validate or match a marker [name]
    static const char* s_pszInvalidNameChars;
    static BOOL s_bValidName(const char* pszName, CNote** ppnot);
//      { return Shw_bValidName(pszName, s_pszInvalidNameChars, ppnot); }
    static BOOL s_bMatchNameAt(const char** ppszName,
            const char* pszValidDelimiters, Str8& sName, CNote** ppnot);
//      { return Shw_bMatchNameAt(ppszName, s_pszInvalidNameChars,
//          pszValidDelimiters, sName, ppnot); }
    static BOOL s_bMatchNameAt(const char** ppszName,
            const char* pszValidDelimiters, Length* plenName, CNote** ppnot);
//      { return Shw_bMatchNameAt(ppszName, s_pszInvalidNameChars,
//          pszValidDelimiters, plenName, ppnot); }
            
    // Validate or match a style name
    static BOOL s_bValidFieldName(const char* pszFieldName, CNote** ppnot); 
    static BOOL s_bMatchFieldNameAt(const char** ppszFieldName,
            const char* pszValidDelimiters, Str8& sFieldName, CNote** ppnot);

    // TLB 07-29-1999
    CJumpPath* pjmpPrimary() const;
    CJumpPath* pjmpDefault() const;

#ifdef typWritefstream // 1.6.4ac
    void WriteProperties(Object_ofstream& obs) const;
#else
    void WriteProperties(Object_ostream& obs) const;
#endif
#ifdef typWritefstream // 1.6.4ac
    void WriteRef(Object_ofstream& obs, const char* pszQualifier) const;
    static void s_WriteRef( Object_ofstream& obs, const char* pszName ); // Write a marker from a string
    static void s_WriteRef( Object_ofstream& obs, const char* pszQualifier, const char* pszName ); // Write a marker from a string
#else
    void WriteRef(Object_ostream& obs, const char* pszQualifier) const;
    static void s_WriteRef( Object_ostream& obs, const char* pszName ); // Write a marker from a string
    static void s_WriteRef( Object_ostream& obs, const char* pszQualifier, const char* pszName ); // Write a marker from a string
#endif

    static BOOL s_bReadRef( Object_istream& obs, Str8& sName ); // Read a marker into a string without looking it up in a marker set
    static BOOL s_bReadRef( Object_istream& obs, const char* pszQualifier, Str8& sName ); // Read a marker into a string without looking it up in a marker set
    
    virtual BOOL bCopy(CSetEl** ppselNew);
    virtual BOOL bModify();
    
    //08-18-1997 Added iRangeSetCondition & bRangeSetSpaceAllowed to parameterlist
    BOOL bModifyProperties(const char* pszName,
            const char* pszDescription, 
            const char* pszFieldName,
            CLangEnc* plng,
            CMarker* pmkrOverThis,
            CMarker* pmkrFollowingThis,
            //const char* pszRangeSet, int iRangeSetCharacter, BOOL bRangeSetSpaceAllowed,
            CFontDef* pfntdef,
            BOOL bMultipleItemData,
            BOOL bMultipleWordItems,
            BOOL bMustHaveData,
            BOOL bNoWordWrap,
            BOOL bCharStyle,
            CNote** ppnot);
    void TakeRangeSet(CDataItemSet& datset);
    void DiscardRangeSet();
    //08-18-1997 Added iRangeSetCondition & bRangeSetSpaceAllowed to parameterlist
    BOOL bSetFieldName(const char* pszFieldName, CNote** ppnot);
    void SetLanguageEnc(CLangEnc* plng);
    void SetDataType(BOOL bMultipleItemData, BOOL bMultipleWordItems);

    void DrawMarker(CDC* pDC, const CRect& rect, BOOL bViewMarkerHierarchy,
        BOOL bViewMarkers, BOOL bViewFieldNames) const;
    void DrawMarker(char* pszBuffer, Length* plen, BOOL bViewMarkerHierarchy,
        BOOL bViewBackslash, BOOL bViewMarkers, BOOL bViewFieldNames) const;
    virtual const Str8& sItem() const;  // For combo (drop-down list) box
    const Str8& sItemTab() const;  // For list box
        // The string which represents the marker when it is a list item:
        // the backslash marker and its field name.

#ifdef _DEBUG
    void AssertValid() const;
#endif  // _DEBUG

private:
    void ClearInterlinear() { m_iInterlinear = 0; }
    void SetInterlinear( BOOL bFirst = 0 );

    void SetMarkerOverThis(CMarker* pmkrOverThis);
    // Resolve forward references and compute derived attributes
    // after marker hierarchy properties are read or modified.
    void SetMarkerOverThis();
    void SetLevel(); // Set nesting level

    void SetMarkerFollowingThis(CMarker* pmkrFollowingThis);
    void SetMarkerFollowingThis();
    void SetLangEnc(CLangEnc* plng);
};  // class CMarker


// --------------------------------------------------------------------------
// NOTE: CMarkerPtr is used for settings, CFieldMarkerPtr is used in CField
class CMarkerPtr  // Hungarian: pmkr
{
private:
    CMarker* m_pmkr;
    
    void IncrementNumberOfRefs(CMarker* pmkr);
    void DecrementNumberOfRefs();
    
public:
    CMarkerPtr(CMarker* pmkr = NULL)
        { IncrementNumberOfRefs(pmkr); }
    CMarkerPtr(const CMarkerPtr& pmkr)  // copy constructor
        { IncrementNumberOfRefs(pmkr.m_pmkr); }
    
    // Decrement the reference count of the index to which
    // this currently refers and increment pmkr's reference count
    const CMarkerPtr& operator=(CMarker* pmkr);
    const CMarkerPtr& operator=(const CMarkerPtr& pmkr)
        { return operator=(pmkr.m_pmkr); }

    ~CMarkerPtr() { DecrementNumberOfRefs(); }
    
    CMarker* operator->() const { return m_pmkr; }
    operator CMarker*() const { return m_pmkr; } 
};  // class CMarkerPtr


// --------------------------------------------------------------------------
// To be used by fields only!
class CFieldMarkerPtr  // Hungarian: pmkr
{
private:
    CMarker* m_pmkr;
    
    void IncrementNumberOfRefs(CMarker* pmkr);
    void DecrementNumberOfRefs();
    
public:
    CFieldMarkerPtr(CMarker* pmkr = NULL)
        { IncrementNumberOfRefs(pmkr); }
    CFieldMarkerPtr(const CFieldMarkerPtr& pmkr)  // copy constructor
        { IncrementNumberOfRefs(pmkr.m_pmkr); }
    
    // Decrement the reference count of the index to which
    // this currently refers and increment pmkr's reference count
    const CFieldMarkerPtr& operator=(CMarker* pmkr);
    const CFieldMarkerPtr& operator=(const CFieldMarkerPtr& pmkr)
        { return operator=(pmkr.m_pmkr); }

    ~CFieldMarkerPtr() { DecrementNumberOfRefs(); }
    
    CMarker* operator->() const { return m_pmkr; }
    operator CMarker*() const { return m_pmkr; } 
};  // class CFieldMarkerPtr


// --------------------------------------------------------------------------

class CMarkerRef : public CDblListEl  // Hungarian: mrf
{
private:
    Str8 m_sMarker; // Marker string for deferred loading
    CMarkerPtr m_pmkr; // Pointer to actual marker
    
    friend class CMarkerRefList;
    CMarkerRef( CMarker* pmkr, const char* pszMarker = "" )
        { m_pmkr = pmkr; m_sMarker = pszMarker; }
    CMarkerRef( const CMarkerRef& mrf ) // Copy constructor
        { m_sMarker = mrf.m_sMarker; m_pmkr = mrf.m_pmkr; }

    BOOL bMakeRef( CMarkerSet* pmkrset ); // Make the marker reference to a member of the marker set from the marker string
public:
    ~CMarkerRef() {};

    CMarker* pmkr() const { return m_pmkr; }
    CSortOrder* psrtDefault() { return pmkr()->plng()->psrtDefault(); }
    void ClearRef() // Clear the pointer, for cleanup before deletion of database types
        { m_pmkr = NULL; }
    void NameModified( const char* pszNewName, const char* pszOldName ); // handle marker name change
};  // class CMarkerRef


// --------------------------------------------------------------------------

class CMarkerRefList : public CDblList  // Hungarian: mrflst
{
private:
    const CMarkerSet* m_pmkrset; // Marker set from which references come
public:
    CMarkerRefList() { m_pmkrset = NULL; }
    CMarkerRefList( const CMarkerRefList& mrflst ); // Copy constructor
    ~CMarkerRefList();

#ifdef typWritefstream // 1.6.4ac
    void WriteProperties( Object_ofstream& obs, const char* pszMarker = "mrflst" ) const;
#else
    void WriteProperties( Object_ostream& obs, const char* pszMarker = "mrflst" ) const;
#endif
    BOOL bReadProperties( Object_istream& obs, const char* pszMarker = "mrflst",
            const CMarkerSet* pmkrset = NULL );

    void operator=( const CMarkerRefList &mrflst ); // Copy
    
    BOOL operator==( const CMarkerRefList &mrflst ) const; // Comparison

    BOOL bMakeRefs( CMarkerSet* pmkrset ); // Make references from strings

    CMarkerRef* pmrfFirst() const { return (CMarkerRef*)pelFirst(); }
    CMarkerRef* pmrfNext( const CMarkerRef* pmrf) const {return (CMarkerRef*)pelNext(pmrf); }
    CMarkerRef* pmrfPrev( const CMarkerRef* pmrf) const {return (CMarkerRef*)pelPrev(pmrf); }
    CMarkerRef* pmrfLast() const { return (CMarkerRef*)pelLast(); } 
    
    void AddMarkerRef( CMarker* pmkr, const char* pszName = "" );
    void Delete( CMarkerRef** ppmrf )   
        { CDblList::Delete( (CDblListEl**)ppmrf ); }

    BOOL bSearch( const CMarker* pmkr ) const; // Search for given marker       
    BOOL bEqual( const CMarkerRefList& mrflst ) const;
    void ClearRefs(); // Clear the mrf pointers, for cleanup before deletion of database types
    void NameModified( const char* pszNewName, const char* pszOldName ); // handle marker name change
};  // class CMarkerRefList


// --------------------------------------------------------------------------

class CLangEncType;
class CDatabaseType;
class CShwDoc;
class CShwView;

class CMarkerUpdate : public CUpdate  // Hungarian: mup
{
private:
    CMarker* m_pmkr;
    BOOL m_bModified;
    BOOL m_bNameModified;
    BOOL m_bLangEncModified;
	BOOL m_bToBeDeleted;
    CMarker* m_pmkrMerge;
    const char* m_pszOldName;
public:
    CMarkerUpdate(CMarker* pmkr, BOOL bToBeDeleted = FALSE);  // Add and Delete
    CMarkerUpdate(CMarker* pmkr, const char* pszName, CLangEnc* plng);
    CMarkerUpdate(CMarker* pmkr, CMarker* pmkrMerge);
    
    CMarker* pmkr() const { return m_pmkr; }
    CMarker* pmkrMerge() const { return m_pmkrMerge; } // if not NULL, marker to be merged into m_pmkr
    const char* pszOldName() const { return m_pszOldName; }

    BOOL bModified() const { return m_bModified; }  
    BOOL bNameModified() const { return m_bNameModified; }
    BOOL bLangEncModified() const { return m_bLangEncModified; }
    BOOL bToBeDeleted() const { return m_bToBeDeleted; }

    // NOTE: Implementated in update.cpp rather than in mkr.cpp
    virtual void UpdateLangEnc(CLangEnc* plng);
    virtual void UpdateDatabaseType(CDatabaseType* ptyp);
    virtual void UpdateShwDoc(CShwDoc* pdoc);
    virtual void UpdateShwView(CShwView* pview);
};  // class CMarkerUpdate


// --------------------------------------------------------------------------

// Just as a marker corresponds to a type of text element, a set of markers
// corresponds to a type of document or database.

// E.g. The set of markers, "d", "p", "w", corresponds to a minimal glossary
// containing definitions and parts of speech for words.
//      CMarkerSet mkrsetGlossary;
//      mkrsetGlossary.Add(new CMarker("w", "Word", plngNawtulaikli));
//      mkrsetGlossary.Add(new CMarker("p", "Part of speech", plngEnglish));
//      mkrsetGlossary.Add(new CMarker("d", "Definition", plngEnglish));
//      CMarker* pmkrGlossaryRecord = mkrsetGlossary.pmkrSearch("w");

class CInterlinearProcList; // forward reference - interlin.h
class CDatabaseType; // forward reference - typ.h

class CMarkerSet : public CSet  // Hungarian: mkrset
{
private:
    CLangEncSet* m_plngset;  // set of language encodings referenced by markers
    CLangEncPtr m_plngDefault;
    CMarker* m_pmkrRecord;  // 1997-11-26 MRP
    BOOL m_bSubfields;
    CDatabaseType* m_ptypMyOwner;

    Level m_maxlev;
    Str8 m_sIndent;
    
public:
    CMarkerSet(CLangEncSet* plngset);
    ~CMarkerSet();

    // This is the full version (used by the marker properties sheet).
    BOOL bAddMarker(const char* pszName, const char* pszDescription, 
            const char* pszFieldName, CLangEnc* plng,
            CMarker* pmkrOverThis, CMarker* pmkrFollowingThis,
            CFontDef* pfntdef,
            BOOL bMultipleItemData,
            BOOL bMultipleWordItems,
            BOOL bMustHaveData,
            BOOL bNoWordWrap, BOOL bCharStyle,
            CMarker** ppmkr, CNote** ppnot);

    // This is the skimpy version (used by Wordlist feature)
    BOOL bAddMarker(const char* pszName, const char* pszDescription, const char* pszFieldName,
            CLangEnc* plng, CNote** ppnot, BOOL bSingleMultiWordDataItem = FALSE);

    CLangEncSet* plngset() const { return m_plngset; }  
    CLangEnc* plngDefault() const { return m_plngDefault; }
    CMarker* pmkrRecord() { return m_pmkrRecord; }
    BOOL bSubfields() const { return m_bSubfields; }
    CDatabaseType* ptyp() const { return m_ptypMyOwner; }
    void SetOwner( CDatabaseType* ptyp ) { m_ptypMyOwner = ptyp; }
	BOOL bUnicodeLang();  // Return true if any marker in set uses a Unicode language
        
    // Search for marker (omit backslash; case is significant), e.g. "d"
    CMarker* pmkrSearch( const char* pszMarker ) const
        { return (CMarker*) CSet::pselSearch( pszMarker ); }
    CMarker* pmkrSearch_AddIfNew( const char* pszMarker );
    CMarker* pmkrSearch_AddIfNew( const char* pszMarker, CMarker* pmkrOverNew, Str8 sFieldName );

    CMarker* pmkrSearchForWholeSubString( const char* pszName, Length len ) const;
        // Returns the marker whose entire name matches the substring
        // beginning at psz of length len (not necessarily null-terminated);
        // otherwise, NULL if none match.
    CMarker* pmkrAdd_MarkAsNew( const char* pszMarker ); // add marker and set field name to '*'
        
    // Markers are sorted by plain ASCII/ANSI collating (case is significant).
    CMarker* pmkrFirst() const  // First marker
        { return (CMarker*) pelFirst(); }
    CMarker* pmkrLast() const  // Last marker
        { return (CMarker*) pelLast(); }
    CMarker* pmkrNext( const CMarker* pmkrCur ) const  // Marker after current
        { return (CMarker*) pelNext( pmkrCur ); }
    CMarker* pmkrPrev( const CMarker* pmkrCur ) const  // Marker before current
        { return (CMarker*) pelPrev( pmkrCur ); }

    void Add( CMarker* pmkrNew );  // Add new marker in correct order   
    // NOTE: InsertBefore/After omitted, since each marker must be unique.  
    // NOTE: Cannot delete a marker to which any CMString instances refer.
    void Delete( CMarker** ppmkr )  // Delete marker, set pointer to next
                        { CSet::Delete( (CSetEl**) ppmkr ); }
    CMarker* pmkrRemove(CMarker** ppmkr)
            { return (CMarker*)pelRemove((CDblListEl**) ppmkr); }

#ifdef typWritefstream // 1.6.4ac
    void WriteProperties(Object_ofstream& obs) const;
#else
    void WriteProperties(Object_ostream& obs) const;
#endif
    BOOL bReadProperties(Object_istream& obs);
    BOOL bReadMarkerRef(Object_istream& obs, const char* pszQualifier,
            CMarker*& pmkr) const;
    
    void SetInterlinear( const CInterlinearProcList* pintprclst ); // Set interlinear properties of all markers referred to in process list
        
    // Use these member functions INSTEAD OF assigning to the member variable
    void SetRecordMarker(CMarker* pmkr);
    void ChangeRecordMarker(CMarker* pmkrFrom, CMarker* pmkrTo);  // Copy
    void SetAllLevels(); // Set hierarchical nesting levels of all markers and set max level
    virtual BOOL bValidName(const char* pszName, CNote** ppnot);
    BOOL bValidNewFieldName(const char* pszNewFieldName, CNote** ppnot) const;
    CMarker* pmkrSearchForFieldName(const char* pszFieldName) const;

    void RemoveFromHierarchy( CMarker* pmkr ); // get rid of any hierarchical dependencies on pmkr
            
    BOOL bModalView_Elements(CMarker** ppmkrSelected);
    virtual void Delete(CSetEl** ppsel);
    
    BOOL bModalView_NewMarker(CMarker** ppmkrNew, const char* pszMarker = "" );
    BOOL bAdd(CSetEl** ppselNew);

    void SetDefaultLangEnc(CLangEnc* plng);
    
    int iCopyMarkerIndent(char* psz, int iMaxCopy, Level lev) const;
    
#ifdef _DEBUG
    void AssertValid() const;
#endif  // _DEBUG
};  // class CMarkerSet


// ==========================================================================

// Base class for deriving different kinds of marker subsets

class CMarkerSubSet : public CSubSet  // Hungarian: subset
{
public:
    CMarkerSubSet(CMarkerSet* pmkrset, BOOL bIncludeAll = TRUE);
    CMarkerSubSet(const CMarkerSubSet& subset);
    virtual ~CMarkerSubSet();

    void operator=(const CMarkerSubSet& subset);

#ifdef prjWritefstream // 1.6.4aa 
    void WriteProperties(Object_ofstream& obs) const;
#else
    void WriteProperties(Object_ostream& obs) const;
#endif
        // If CSubSet::m_bIncludeAllElements, no properties are written
        // (i.e. this is the default value).
        // If CSubSet::m_bAutoIncludeNewElements, the group's tag is
        // \+mkrsubsetExcluded; otherwise \+mkrsubsetIncluded; the
        // appropriate markers are written with tag \mkr.
    BOOL bReadProperties(Object_istream& obs);

    // For Export properties in a pre-version-3.1 database type file.
#ifdef typWritefstream // 1.6.4ac
    void WriteOldProperties(Object_ofstream& obs, const char* pszMarker) const;
#else
    void WriteOldProperties(Object_ostream& obs, const char* pszMarker) const;
#endif
    BOOL bReadOldProperties(Object_istream& obs, const char* pszMarker);

    virtual BOOL bIncludes(const CMarker* pmkr) const;
    virtual void Include(CMarker* pmkr, BOOL bInclude);
    
    const CMarkerSet* pmkrset() const { return (const CMarkerSet*) m_pset; }
};  // class CMarkerSubSet

// ==========================================================================

class CMarkerSetListBox : public CSetListBox
{
public:
    CMarkerSetListBox(CMarkerSet* pmkrset, CMarker** ppmkr) :
        CSetListBox(pmkrset, (CSetEl**)ppmkr) {}
    void ChangeSet(CMarkerSet* pmkrset, CMarker* pmkrToSelect)
        { CSetListBox::ChangeSet( (CSet*) pmkrset, (CSetEl*) pmkrToSelect); }

protected:
    int m_xMarker;
    int m_xFieldName;
    int m_xLangEnc;
    int m_xUnder;
    int m_xMisc;

    // 1998-06-12 MRP: Instead of hard-coding P, C, F, and R,
    // get them from string resources to allow translation into French.
    char m_chParagraph;
    char m_chCharacter;
    char m_chFont;
    char m_chRangeSet;
    
    virtual void InitLabels();
    virtual void DrawElement(CDC& cDC, const RECT& rcItem,
            const CDblListEl *pel);
    virtual BOOL bIsBold(CSetEl* psel) { return psel->bHasRefs(); }
};  // class CMarkerSetListBox

class CMarkerSetComboBox : public CSetComboBox
{
public:
    CMarkerSetComboBox(CMarkerSet* pmkrset, BOOL bUseNoneItem, CMarker* pmkrDontUse)
                        : CSetComboBox(pmkrset, bUseNoneItem, pmkrDontUse) {}
    
    void UpdateElements(CMarker* pmkrToSelect)
        { CSetComboBox::UpdateElements( pmkrToSelect ); }
    void ChangeSet(CMarkerSet* pmkrset, CMarker* pmkrToSelect, CMarker* pmkrDontUse)
        { CSetComboBox::ChangeSet( pmkrset, pmkrToSelect, pmkrDontUse ); }
    CMarker* pmkrSelected() { return (CMarker*)CSetComboBox::pselSelected(); }
};

class CMarkerHierarchyComboBox : public CSetComboBox
{
private:
    CMarker* m_pmkr;
    
public:
    CMarkerHierarchyComboBox(CMarkerSet* pmkrset, CMarker* pmkr);
    
    void UpdateElements(CMarker* pmkrToSelect);
    CMarker* pmkrSelected();
    BOOL bIncludeElement(const CSetEl* psel) const;
        // Exclude this marker and any under it to prevent cycles.
};


// ==========================================================================

// A "marked string" is a string tagged with a descriptive marker.

// E.g. "\d song" is the string "song" tagged with the marker "d".
// The marker indicates that the text element is a "Definition" and also
// that it is expressed in the language encoding "English".
//      CMString mksKatrwallinDefinition(pmkrDefinition, "song");
//      CMString* pmks = new CMString(pmkrDefinition, "singing");
//      ASSERT(pmks->sMarker() == "d");

// NOTE: A "marked string" may contain "marked substrings", or word elements.
// E.g. "\oe |fv{ka} tends to be used in nominal constructions,
//       whereas |fv{jaga} tends to be used in verbal constructions.".
// The entire string is marked with "oe", however contains embedded
// substrings "ka" and "jaga", which are tagged with marker "fv".

class CMString : public Str8  // Hungarian: mks
{
private:
    CFieldMarkerPtr m_pmkr;  // reference to the marker
    
public:
    CMString(CMarker* pmkr, const char* pszString);  // constructor
    CMString(const CMString& mks);  // copy constructor
    CMString& operator=(const CMString& mks);  // assignment operator
    CMString& operator=( const char* psz );  // assignment operator from char* or Str8, copy string part, leave marker as is
    ~CMString();  // destructor
    
    CMarker* pmkr() const { return m_pmkr; }
    const Str8& sMarker() const { return m_pmkr->sMarker(); }  // e.g. "d"
    const char* psz( int iChar = 0 ) const // Return char pointer from field and char pos
    {
        ASSERT( iChar <= (int)GetLength() );
        return (const char*)*this + iChar;
    }

    void SetMarker( CMarker* pmkr ); // Set field marker to a different value
    void SetContent( const char* psz ); // Set field content to a different value

    CLangEnc* plng() const { return pmkr()->plng(); }
    const Str8& sLangEnc() const { return pmkr()->sLangEnc(); }
    
    BOOL bIsEmptyOrWhite(); // Return true if empty or all whitespace
    void Trim(); // Trim trailing spaces and nl's
    void TrimBothEnds(); // Trim leading and trailing spaces and nl's
    void Insert( UINT nChar, int iChar, int iNum = 1 ); // Insert iNum instances of nChar at pos iChar
    void Insert( const char* pszInsert, int iChar ); // Insert string pszInsert at pos iChar
    void Delete( int iChar, int iNum = 1 ); // Delete iNum chars at pos iChar
    void DeleteAll( const char cRemove ); // Delete all occurrences of cRemove
    void Extend( int iNewLen ); // Extend as necessary with spaces to be at least length iNewLen
    void Overlay( const char* pszInsert, int iChar, int iPadLen = 0 ); // Overlay pszInsert starting at iChar, filling out to iPadLen positions with spaces
	BOOL bVerify( const char* pszInsert, int iChar, int iLen ); // Verify pszInsert starting at iChar
    void Overlay( const char cInsert, int iChar ); // Overlay cInsert at iChar
    void OverlayAll( const char cRemove, const char cInsert, int iChar = 0 ); // Overlay all occurrences of cRemove with cInsert, starting at iChar

	bool bNextWord( Str8& sWord, int& iPos ) const; // Find next space delimited word, return in sWord, return end of word in iPos

    void AddSuffHyphSpaces( const char* pszMorphBreakChars, char cForceStart, char cForceEnd ); // Add a space before each suffix hyphen that doesn't have space
};  // class CMString

// **************************************************************************

#endif  // MKR_H
