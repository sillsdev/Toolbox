// mkr.cpp  Implementation for Standard Format text markup (1995-02-21)

// A description of descriptive markup appears at the bottom of markup1.h. 

// Change history:
// 1995-05-17 0.15  MRP: count marker references to language encodings
// 1995-04-07 0.11  MRP: revise loc for undefined and for marked substrings
// 1995-03-28 0.10  MRP: distinguish sort orders of a language encoding
// 1995-03-07 0.08a MRP: Marked substrings without language encoding stack
// 1995-03-02 0.02  MRP: Marked string location
// 1995-02-21 0.01  MRP: Initial design sketch


#include "stdafx.h"
#include "toolbox.h"
#include <stdlib.h> // For _fnsplit function.
#include "mkr.h"
#include "crngset.h"  // CRangeSet
#include <fstream>
#include "shwnotes.h"
#include "tools.h"
#include "font.h"
#include "interlin.h"
#include "project.h"
#include "obstream.h"  // Object_istream, Object_ostream
#include "typ.h"
#include "jmp.h" // CJumpPath, CJumpPathSet

#include "mkr_d.h"
#include "set_d.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


// **************************************************************************

// 08-18-1997 Added iRangeSetCharacter & bRangeSetSpaceAllowed to paramterlist
CMarker::CMarker(const char* pszName,
            CLangEnc* plng,
            CMarkerSet* pmkrsetMyOwner,
            const char* pszFieldName,
            const char* pszDescription,
            const char* pszMarkerOverThis,
            const char* pszMarkerFollowingThis,
            //const char* pszRangeSet, int iRangeSetCharacter, BOOL bRangeSetSpaceAllowed,
            CFontDef* pfntdef,
            BOOL bMultipleItemData,
            BOOL bMultipleWordItems,
            BOOL bMustHaveData,
            BOOL bNoWordWrap,
            BOOL bCharStyle,
            int iSect,
            BOOL bHidden,
            BOOL bSubfield) :
    CSetEl(pszName),
    m_bMultipleItemData(bMultipleItemData),
    m_bMultipleWordItems(bMultipleWordItems),
    m_bMustHaveData(bMustHaveData),
    m_prngset(NULL)
{
    ASSERT( pszName != NULL );
    ASSERT( pszFieldName != NULL );
    m_sDescription = pszDescription;
    m_sFieldName = pszFieldName;
    m_plng = plng;
    ASSERT( m_plng );
    // m_prngset->SetRangeSet(this, pszRangeSet, iRangeSetCharacter, bRangeSetSpaceAllowed);
    m_pmkrsetMyOwner = pmkrsetMyOwner;
    ASSERT( m_pmkrsetMyOwner );
    ASSERT(!(bMultipleItemData && bMultipleWordItems)); // Can't have both! TLB
    m_iInterlinear = 0;
    SetFont( pfntdef );
    ASSERT( pszMarkerOverThis );
    m_sMarkerOverThis = pszMarkerOverThis;
    // 1997-11-26 MRP: New markers added after the record marker
    // has been established get it as the default marker over them.
    CMarker* pmkrRecord = m_pmkrsetMyOwner->pmkrRecord();
    if ( m_sMarkerOverThis.IsEmpty() && pmkrRecord )
        m_sMarkerOverThis = pmkrRecord->sName();
    m_pmkrOverThis = NULL;
    ASSERT( pszMarkerFollowingThis );
    m_sMarkerFollowingThis = pszMarkerFollowingThis;
    m_pmkrFollowingThis = NULL;
    m_lev = 0;
    m_bNoWordWrap = bNoWordWrap;
    m_bCharStyle = bCharStyle;
    m_iSect = iSect;
    m_bHidden = bHidden;
    m_bSubfield = bSubfield;
}

CMarker::~CMarker()
{
    delete m_prngset;  // 1999-06-07 MRP
}

CSet* CMarker::psetMyOwner()
{
    return m_pmkrsetMyOwner;
}

 //08-18-1997 Added iRangeSetCharacter & bRangeSetSpaceAllowed to Paramterlist                
BOOL CMarker::s_bConstruct(const char* pszName,
        const char* pszDescription, const char* pszFieldName,
        CLangEnc* plng,
        const char* pszMarkerOverThis, const char* pszMarkerFollowingThis,
        //const char* pszRangeSet, int iRangeSetCharacter, BOOL bRangeSetSpaceAllowed,
        CFontDef* pfntdef,
        BOOL bMultipleItemData, BOOL bMultipleWordItems, BOOL bMustHaveData,
        BOOL bNoWordWrap, BOOL bCharStyle, int iSect, BOOL bHidden, BOOL bSubfield,
        CMarkerSet* pmkrsetMyOwner, CMarker** ppmkr, CNote** ppnot)
{
    ASSERT(ppmkr);
    ASSERT(!(*ppmkr));
//    if ( !s_bValidName(pszName, ppnot) )
    if ( !pmkrsetMyOwner->bValidNewName(pszName, ppnot) )
        {
        (*ppnot)->SetStringContainingReferent(pszName);
        return FALSE;
        }

    if ( *pszFieldName != '\0' &&  // 1995-05-05 MRP: for now, allow empty
            !s_bValidFieldName(pszFieldName, ppnot) ) 
        {
        (*ppnot)->SetStringContainingReferent(pszFieldName);
        return FALSE;
        }
    
    //08-18-1997 Added iRangeSetCharacter & bRangeSetSpaceAllowed to parameterlist
    *ppmkr = new CMarker(pszName, plng, pmkrsetMyOwner, pszFieldName, pszDescription, 
        pszMarkerOverThis, pszMarkerFollowingThis,
        // pszRangeSet, iRangeSetCharacter, bRangeSetSpaceAllowed,
        pfntdef, bMultipleItemData, bMultipleWordItems, bMustHaveData, bNoWordWrap,
        bCharStyle, iSect, bHidden, bSubfield);
    return ( (*ppmkr) != NULL );
}


const char* CMarker::s_pszInvalidNameChars =
        "\\"  // ordinary SF field marker begins with backslash, e.g. \lx
        "{}"  // braces delimit SGML text element tags, e.g. {fv}...{-fv}
        "|"   // bar separates marker from text pattern in filter, e.g. [ps|N]
              // bar (and braces) delimit CTW/SF-WORD tags, e.g. |fv{...}
        ","   // comma separates items in a .tag file, e.g. \p,Paragraph
//      "[]"  // brackets enclose marker in filters, e.g. [Field|hm]
        ;


BOOL CMarker::s_bValidName(const char* pszName, CNote** ppnot)
{
    Length lenName;
    return s_bMatchNameAt(&pszName, "", &lenName, ppnot);
}


BOOL CMarker::s_bMatchNameAt(const char** ppszName, const char* pszValidDelimiters, Str8& sName, CNote** ppnot)
{
    if ( !Shw_bMatchNameAt(ppszName, s_pszInvalidNameChars, pszValidDelimiters, sName, ppnot) )
        {
        CNote* pnot = *ppnot;
        pnot->bSubstitute(_("Expecting a name"), _("Expecting a marker")); // 1.4ek
        return FALSE;
        }
        
    return TRUE;
}


BOOL CMarker::s_bMatchNameAt(const char** ppszName, const char* pszValidDelimiters, Length* plenName, CNote** ppnot)
{
    if ( !Shw_bMatchNameAt(ppszName, s_pszInvalidNameChars, pszValidDelimiters, plenName, ppnot) )
        {
        CNote* pnot = *ppnot;
        pnot->bSubstitute(_("Expecting a name"), _("Expecting a marker")); // 1.4ek
        return FALSE;
        }
        
    return TRUE;
}
 

BOOL CMarker::s_bValidFieldName(const char* pszFieldName, CNote** ppnot)
{
    Str8 sFieldName;
    return s_bMatchFieldNameAt(&pszFieldName, "", sFieldName, ppnot);
}

// 1995-07-17 MRP: This may become a class static member variable;
// but let's can wait until it's time for a review of set element names.
static const char* s_pszInvalidFieldNameChars =
        "\\{};" // control words in RTF, so disallowed in Word field names
        ",";    // delimiter in SFC .tag file-- may become valid later

BOOL CMarker::s_bMatchFieldNameAt(const char** ppszFieldName,
        const char* pszValidDelimiters, Str8& sFieldName, CNote** ppnot)
{
    const char* pszFieldName = *ppszFieldName;
    Length lenName;
    if ( !Shw_bMatchNameAt(ppszFieldName, Shw_pszOtherWhiteSpace,
            s_pszInvalidFieldNameChars, pszValidDelimiters,
            &lenName, ppnot) )
        {
        CNote* pnot = *ppnot;
        pnot->bSubstitute(_("Expecting a name"), _("Expecting a style name")); // 1.4ek
        return FALSE;
        }
        
    // Note; in general, the matched substring is not null terminated.
    char* pch = sFieldName.GetBuffer(lenName); // 1.4qzfv GetBuffer OK because written immediately
    memcpy(pch, pszFieldName, lenName);
    sFieldName.ReleaseBuffer(lenName);
    return TRUE;
}

// TLB - 07/29/1999 : Function to get the primary jump path, if any, for this marker
//                    This allows for possible future performance enhancement if
//                    markers need to explicitly store the primary jump path.
CJumpPath* CMarker::pjmpPrimary() const
{
    CJumpPathSet* pjmpset = m_pmkrsetMyOwner->ptyp()->pjmpset();
    CJumpPath* pjmpPrimary = pjmpset->pjmpFirst();
    for ( ; pjmpPrimary; pjmpPrimary = pjmpset->pjmpNext(pjmpPrimary) )
        {
        if ( !pjmpPrimary->bDefaultPath() && pjmpPrimary->bIncludes(this) )
            break;
        }
    return pjmpPrimary;
}

CJumpPath* CMarker::pjmpDefault() const
{ 
	return m_pmkrsetMyOwner->ptyp()->pjmpset()->pjmpDefault(); 
}

void CMarker::SetLangEnc(CLangEnc* plng)
{
    ASSERT( plng != NULL );
//  m_plng->DecrementNumberOfRefs();
    m_plng = plng;
//  m_plng->IncrementNumberOfRefs();
}

// 1999-11-17 TLB: Tells whether this marker can be checked for inconsistencies.
BOOL CMarker::bHasConsistencyCheckConditions(BOOL bCheckDataDefn,
                                             BOOL bCheckRangeSets,
                                             BOOL bCheckRefConsistency) const
{
    // Single-word and single-item fields could have data property inconsistencies,
    // as well as any field that requires data.
    if ( bCheckDataDefn && (!m_bMultipleItemData || m_bMustHaveData) )
        return TRUE;
    // Fields with range sets could have range set inconsistencies
    if ( bCheckRangeSets && bRangeSetEnabled() )
        return TRUE;
    // Fields with primary jump paths that require referential integrity could have
    // data link inconsistencies.
    else if ( bCheckRefConsistency )
        {
        CJumpPath* pjmp = pjmpPrimary();
        return ( pjmp && pjmp->bEnforceRefIntegrity() );
        }
    else
        return FALSE;
}

BOOL CMarker::bRangeSetEnabled() const
{
    if ( !m_prngset )
        return FALSE;
    return m_prngset->bEnabled();
}

BOOL CMarker::bCopy(CSetEl** ppselNew)
{
    CMarkerPropertiesSheet dlg(this, (CMarker**)ppselNew);
    return ( dlg.DoModal() == IDOK );
}

BOOL CMarker::bModify()
{
    CMarkerPropertiesSheet dlg(this);
    return ( dlg.DoModal() == IDOK );  // show a modal view of the marker
}


BOOL CMarker::bModifyProperties(const char* pszName, const char* pszDescription, 
            const char* pszFieldName, CLangEnc* plng,
            CMarker* pmkrOverThis, CMarker* pmkrFollowingThis,
            //const char* pszRangeSet, int iRangeSetCharacter, BOOL bRangeSetSpaceAllowed,
            CFontDef* pfntdef,
            BOOL bMultipleItemData, BOOL bMultipleWordItems, BOOL bMustHaveData,
            BOOL bNoWordWrap, BOOL bCharStyle, CNote** ppnot)
// 08-18-1997 Added iRangeSetCharacters & bRangeSetSpaceAllowed to parameterlist
{
    // NOTE NOTE NOTE: We must construct the update notification
    // *before* changing any member data, but must wait to use it
    // until *after* the modifications have been committed. 
    CMarkerUpdate mup(this, pszName, plng);

    // If the name is being changed, verify that it will be valid
    BOOL bUpdatingName = !bEqual(pszName, sName());
    if ( bUpdatingName && !m_pmkrsetMyOwner->bValidNewName(pszName, ppnot) )
        {
        (*ppnot)->SetStringContainingReferent(pszName);
        return FALSE;
        }
    
    // If the field name is being changed, attempt to update it
    if ( !bEqual(pszFieldName, sFieldName()) &&
            !m_pmkrsetMyOwner->bValidNewFieldName(pszFieldName, ppnot) )
        {
        return FALSE;
        }

    m_sFieldName = pszFieldName;    
        
    // If the language encoding is being changed, update it
    if ( plng != m_plng )
        SetLangEnc(plng);
    if ( pmkrOverThis != m_pmkrOverThis )
        {
        ASSERT( !pmkrOverThis || !bOver(pmkrOverThis) );
        SetMarkerOverThis(pmkrOverThis);
        }
    m_pmkrFollowingThis = pmkrFollowingThis;

    // Update description whether it changed or not.
    m_sDescription = pszDescription;
    
    SetFont( pfntdef );

    SetDataType(bMultipleItemData, bMultipleWordItems);
    m_bMustHaveData = bMustHaveData;
    m_bNoWordWrap = bNoWordWrap;
    m_bCharStyle = bCharStyle;

    // If the name is being changed, update it
    if ( bUpdatingName )
        ChangeNameTo(pszName);

    extern void Shw_Update(CUpdate& up);        
    Shw_Update(mup);  // Notify other objects that this marker is modified
    
    return TRUE;
}

void CMarker::TakeRangeSet(CDataItemSet& datset)
{
    if ( !m_prngset )
        {
        m_prngset = new CRangeSet(this, datset.bCharacterItems());
        m_prngset->TransferDataItems(datset); // This will leave pdatset empty
        }
    else
        {
        m_prngset->Reset(datset); // This will leave datset empty
        }
}

void CMarker::DiscardRangeSet()
{
    delete m_prngset;
    m_prngset = 0;
}

BOOL CMarker::bSetFieldName(const char* pszFieldName, CNote** ppnot)
{
    // If the field name is being changed, attempt to update it
    if ( !bEqual(pszFieldName, sFieldName()) &&
            !m_pmkrsetMyOwner->bValidNewFieldName(pszFieldName, ppnot) )
        {
        return FALSE;
        }

    m_sFieldName = pszFieldName;    
    return TRUE;
}

void CMarker::SetLanguageEnc(CLangEnc* plng)
{
    if ( plng == m_plng )
        return;
    // NOTE NOTE NOTE: We must construct the update notification
    // *before* changing any member data, but must wait to use it
    // until *after* the modifications have been committed. 
    CMarkerUpdate mup(this, sName(), plng);

    SetLangEnc(plng);
    
    extern void Shw_Update(CUpdate& up);        
    Shw_Update(mup);  // Notify other objects that this marker is modified
}

void CMarker::SetDataType(BOOL bMultipleItemData, BOOL bMultipleWordItems)
{
    ASSERT(!(bMultipleItemData && bMultipleWordItems)); // Can't have both! TLB
    m_bMultipleItemData = bMultipleItemData;
    m_bMultipleWordItems = bMultipleWordItems;
}

void CMarker::DrawMarker(CDC* pDC, const CRect& rect, BOOL bViewMarkerHierarchy,
    BOOL bViewMarkers, BOOL bViewFieldNames) const
{
    const Length maxlen = 99;
    Length len = maxlen;
    char pszBuffer[maxlen + 1]; // used to build marker/field name string to be displayed
    DrawMarker(pszBuffer, &len, bViewMarkerHierarchy, TRUE, bViewMarkers, bViewFieldNames);
	Str8 sText = pszBuffer; // 1.4qta
	CString swText = swUTF16( sText ); // 1.4qta
    pDC->ExtTextOut( 0, 0, ETO_CLIPPED|ETO_OPAQUE, &rect, swText, swText.GetLength(), NULL ); // draw marker and/or fieldname // 1.4qta
}

void CMarker::DrawMarker(char* pszBuffer, Length* plen, BOOL bViewMarkerHierarchy,
    BOOL bViewBackslash, BOOL bViewMarkers, BOOL bViewFieldNames) const
{
    char* psz = pszBuffer;
    ASSERT( pszBuffer );
    ASSERT( plen );
    Length len = *plen;
    char* pszEnd = psz + len;

    if ( bViewMarkerHierarchy )
    {
        psz += m_pmkrsetMyOwner->iCopyMarkerIndent( psz, pszEnd-psz, m_lev );
        if ( pszEnd < psz )  // 1998-03-26 MRP
            psz = pszEnd;
    }

    if ( bViewMarkers || sFieldName().IsEmpty() )
    {
        if ( psz < pszEnd && bViewBackslash && !m_bSubfield )
            *psz++ = '\\';
        int iCnt = min( sMarker().GetLength(), pszEnd-psz );
		memcpy(psz, (const char*)sMarker(), iCnt);
        psz += iCnt;
    }

    if ( bViewFieldNames )
    {
        if ( bViewMarkers )
        {
            // Output two spaces to separate name from marker
            if ( psz < pszEnd )
                *psz++ = ' ';
            if ( psz < pszEnd )
                *psz++ = ' ';
        }
        int iCnt = min( sFieldName().GetLength(), pszEnd-psz );
        memcpy(psz, (const char*)sFieldName(), iCnt);
        psz += iCnt;
    }

    ASSERT( psz <= pszBuffer + len );  // 1998-03-26 MRP: Off by one, use <=
    *psz = '\0';
    *plen = psz - pszBuffer;
}

static Str8 s_sItem;

const Str8& CMarker::sItem() const
{
    s_sItem = sMarker();
    s_sItem += "  ";
    s_sItem += m_sFieldName;

    return s_sItem;
}

const Str8& CMarker::sItemTab() const
{
    s_sItem = sMarker();
    s_sItem += "\t";
    s_sItem += m_sFieldName;

    return s_sItem;
}


#ifdef _DEBUG
void CMarker::AssertValid() const
{
    CNote* pnot = NULL;
    ASSERT( s_bValidName(sName(), &pnot) );
    if ( !m_sFieldName.IsEmpty() )
        ASSERT( s_bValidFieldName(m_sFieldName, &pnot) );
    // MRP: assume that the language encoding set will be validated
    ASSERT( m_plng != NULL );
    ASSERT( m_pmkrsetMyOwner != NULL );
}
#endif  // _DEBUG


static const char* psz_mkr = "mkr";
static const char* psz_sty = "sty";  // 1995-09-05 MRP: replacing with nam
static const char* psz_nam = "nam";
static const char* psz_enc = "enc";
static const char* psz_rngsetCharacter = "rngsetCharacter";  //08-19-1997
static const char* psz_rngsetSpace = "rngsetNoSpace";          //08-19-1997
static const char* psz_rngset = "rngset";
static const char* psz_SingleWord = "SingleWord";
static const char* psz_MultipleWordItems = "MultipleWordItems";
static const char* psz_MustHaveData = "MustHaveData";
static const char* psz_mkrFollowingThis = "mkrFollowingThis";
static const char* psz_mkrOverThis = "mkrOverThis";
static const char* psz_NoWordWrap = "NoWordWrap";
static const char* psz_CharStyle = "CharStyle";
static const char* psz_sect = "sect";
static const char* psz_Hidden = "Hide";
static const char* psz_Subfield = "Subfield";
static const char* psz_desc = "desc";

#ifdef typWritefstream // 1.6.4ac
void CMarker::WriteProperties(Object_ofstream& obs) const 
#else
void CMarker::WriteProperties(Object_ostream& obs) const 
#endif
{
    obs.WriteBeginMarker(psz_mkr, sName());
    obs.WriteString(psz_nam, sFieldName());
    obs.WriteString(psz_desc, m_sDescription);
    m_plng->WriteRef(obs, "");
    // Range sets are written using old-style "unwrapped" markers unless they are the new
    // multiple-word type items or empty range sets. These are written later after the marker set is all done.
    // This prevents backwards incompatibility.
    if ( m_prngset && (!m_bMultipleWordItems || m_prngset->bCharacterItems()) && !m_prngset->bIsEmpty() )
        {
        m_prngset->WritePropertiesUnwrapped(obs);
        if ( !m_bMultipleItemData )
            obs.WriteBool(psz_rngsetSpace, FALSE); // TLB 03/30/2000 - for backwards compatibility
        }
//    obs.WriteBool(psz_rngsetCharacter, m_prngset->bRangeSetForCharacters());  //08-19-1997
//    obs.WriteBool(psz_rngsetSpace, !m_prngset->bRangeSetSpaceAllowed()); //08-19-1997
//    obs.WriteString(psz_rngset, m_prngset->sContent());
    obs.WriteBool(psz_SingleWord, (!m_bMultipleItemData && !m_bMultipleWordItems));
    obs.WriteBool(psz_MultipleWordItems, m_bMultipleWordItems);
    obs.WriteBool(psz_MustHaveData, m_bMustHaveData);

    if ( !bUsingDefaultFont() )
        {
        CFontDef fntdef( &m_fnt, m_rgbColor );
        fntdef.WriteProperties(obs);
        }
    obs.WriteBool(psz_NoWordWrap, m_bNoWordWrap);
    if ( m_pmkrOverThis )
        obs.WriteString(psz_mkrOverThis, m_pmkrOverThis->sMarker());        
    if ( m_pmkrFollowingThis )
        obs.WriteString(psz_mkrFollowingThis, m_pmkrFollowingThis->sMarker());        
    obs.WriteBool(psz_CharStyle, m_bCharStyle);
    if ( m_iSect != 0 )
        obs.WriteInteger(psz_sect, m_iSect);
    obs.WriteBool(psz_Hidden, m_bHidden);
    obs.WriteBool(psz_Subfield, m_bSubfield);
	obs.WriteUnrecognizedSettingsInfo( m_sUnrecognizedSettingsInfo ); // 1.0cp Write unrecognized settings info

    obs.WriteEndMarker(psz_mkr);
    obs.WriteNewline();
}

BOOL g_bReading_MDF_HTML_pn = FALSE;  // 2000-07-20 MRP

BOOL CMarker::s_bReadProperties(Object_istream& obs,
        CMarkerSet* pmkrsetMyOwner, CMarker** ppmkr)
{
    Str8 sName;
    Str8 sDescription;
	Str8 sUnrecognizedSettingsInfo;
    if ( !obs.bReadBeginMarker(psz_mkr, sName) )
        return FALSE;
        
    Str8 sFieldName;
    ASSERT( pmkrsetMyOwner );
    CLangEncSet* plngset = pmkrsetMyOwner->plngset();
    CLangEnc* plng = NULL;
    Str8 sRangeSet;
    CDataItemSet datset;
    
    // Set to default values
#ifdef _MAC
    CFontDef fntdef("Times", 12);
#elif defined(TLB_07_18_2000_FONT_HANDLING_BROKEN)
//    CFontDef fntdef("Times New Roman", 16);
#else
    CFontDef fntdef("Times New Roman", 12, TRUE /* Points */);
#endif

    BOOL bFontFound = FALSE;
    BOOL bSingleWord = FALSE;
    BOOL bMultipleItemData = TRUE;
    BOOL bMultipleWordItems = FALSE;
    BOOL bMustHaveData = FALSE;
    BOOL bUseRangeSet = FALSE;
    // 2000-09-05 TLB
    BOOL bV43RangeSet = FALSE;
    BOOL bNoWordWrap = FALSE;
    BOOL bCharStyle = FALSE;
    int iSect = 0;
    BOOL bHidden = FALSE;
    BOOL bSubfield = FALSE;
    BOOL bRangeSetCharacter = FALSE;  //08-19-1997
    BOOL bRangeSetSpaceNotAllowed = FALSE;  //08-19-1997

    Str8 sMarkerOverThis;
    Str8 sMarkerFollowingThis;

    // 2000-07-20 MRP: Automatically correct an error in MDF-HTML.typ
    // for users of Shoebox test versions 4.1 through 4.3:
    // The \pn marker had language National instead of national.
    // This causes an unwanted language encoding to be created,
    // especially when older settings files are converted to version 5.0.
    // See CLangEncSet::bReadLangEncRef in lng.cpp.
    if ( bEqual(sName, "pn") )
        {
        CDatabaseType* ptyp = pmkrsetMyOwner->ptyp();
        ASSERT( ptyp );
        CDatabaseTypeProxy* ptrx = ptyp->ptrxMyProxy();
        ASSERT( ptrx );
        const Str8& sTypeName = ptrx->sName();
        if ( bEqual(sTypeName, "MDF SF-to-HTML") )
            g_bReading_MDF_HTML_pn = TRUE;
        }
    
    while ( !obs.bAtEnd() ) // While more in file
        {
        if ( obs.bReadString(psz_nam, sFieldName) )
            ;
        else if ( obs.bReadString(psz_desc, sDescription) )
            ;
        else if ( plngset->bReadLangEncRef(obs, "", plng) )
            ;
        else if ( datset.bReadPropertiesVer43(obs) )
            {
            // 2000-09-05 TLB
            bUseRangeSet = bV43RangeSet = TRUE;
            }
        else if ( obs.bReadBool(psz_SingleWord, bSingleWord)) //08-19-1997
            bMultipleItemData = bMultipleWordItems = !bSingleWord;
        else if ( obs.bReadBool(psz_MultipleWordItems, bMultipleWordItems)) //08-19-1997
            bMultipleItemData = !bMultipleWordItems; // Guarantee they aren't both true!
        else if ( obs.bReadBool(psz_MustHaveData, bMustHaveData)) //08-19-1997
            ;
        else if ( obs.bReadString(psz_rngset, sRangeSet) )
            ; // Old setting -- will be dealt with below
        else if ( obs.bReadBool(psz_rngsetCharacter, bRangeSetCharacter)) //08-19-1997
            ; // Old setting -- will be dealt with below
        else if ( obs.bReadBool(psz_rngsetSpace, bRangeSetSpaceNotAllowed)) //08-19-1997
            {
            // This is an old setting, now dealt with by marker data definition.
            bSingleWord = TRUE;
            bMultipleItemData = bMultipleWordItems = FALSE;
            }
        else if ( fntdef.bReadProperties(obs) )
            bFontFound = TRUE;
        else if ( obs.bReadBool(psz_NoWordWrap, bNoWordWrap) )
            ;
        else if ( obs.bReadString(psz_mkrOverThis, sMarkerOverThis) )
            ;
        else if ( obs.bReadString(psz_mkrFollowingThis, sMarkerFollowingThis) )
            ;
        else if ( obs.bReadBool(psz_CharStyle, bCharStyle) )
            ;
        else if ( obs.bReadInteger(psz_sect, iSect) )
            ;
        else if ( obs.bReadBool(psz_Hidden, bHidden) )
            ;
        else if ( obs.bReadBool(psz_Subfield, bSubfield) )
            ;
		else if ( obs.bUnrecognizedSettingsInfo( psz_mkr, sUnrecognizedSettingsInfo  ) ) // 1.0cp Store any unrecognized settings info so as not to lose it
			;
        else if ( obs.bEnd( psz_mkr ) ) // If end marker or erroneous begin break
            break;
        }

    if ( !plng )
        plng = pmkrsetMyOwner->plngDefault();
    CNote* pnot = NULL;

    // 2000-09-05 TLB
    // Handle the case of a 4.3-style range set that has multi-word range set
    // items even though the data properties are set to require single word items.
    // Might be nice to warn the user, but for now just change the data properties
    // to match the range set elements. For similar code to handle other versions
    // of Shoebox, see the CRangeSet::s_bReadProperties method.
    if ( bV43RangeSet && !bMultipleWordItems && datset.bHasMultiwordElements() )
        {
        bMultipleItemData = FALSE;
        bMultipleWordItems = TRUE;
        }


    //08-18-1997 Added iRangeSetCharacter & bRangeSetSpaceAllowed to Paramterlist
    if ( !s_bConstruct(sName, sDescription, sFieldName, plng,
            sMarkerOverThis, sMarkerFollowingThis,
            // sRangeSet, iRangeSetCharacter, !bRangeSetSpaceNotAllowed,
            bFontFound ? &fntdef : NULL, bMultipleItemData,
            bMultipleWordItems, bMustHaveData, bNoWordWrap,
            bCharStyle, iSect, bHidden, bSubfield,
            pmkrsetMyOwner, ppmkr, &pnot) )
        {
//      pnotlst->Add(pnot);  // Add to obs's notelist
        delete pnot;  // MRP: TEMPORARY (i.e., permanent (TLB))
        ASSERT(!(*ppmkr));
        return TRUE;
        }

    ASSERT(*ppmkr);
    CMarker* pmkr = *ppmkr;
	pmkr->m_sUnrecognizedSettingsInfo = sUnrecognizedSettingsInfo; // 1.0cp Store any unrecognized settings info so as not to lose it

    if ( bUseRangeSet )
        {
        ASSERT(!pmkr->m_prngset);
        pmkr->m_prngset = new CRangeSet(pmkr, datset.bCharacterItems());
        pmkr->m_prngset->TransferDataItems(datset);
        }
    else if ( !sRangeSet.IsEmpty() ) // Found old-style range set
        {
        CMarker* pmkr = *ppmkr;
        ASSERT(!pmkr->m_prngset);
        pmkr->m_prngset = new CRangeSet(pmkr, bRangeSetCharacter);
        pmkr->m_prngset->AddItems(sRangeSet);
        }

    g_bReading_MDF_HTML_pn = FALSE;  // 2000-07-20 MRP

    return TRUE;            
}

#ifdef typWritefstream // 1.6.4ac
void CMarker::WriteRef(Object_ofstream& obs, const char* pszQualifier) const
#else
void CMarker::WriteRef(Object_ostream& obs, const char* pszQualifier) const
#endif
{
    obs.WriteString(psz_mkr, pszQualifier, sName());
}

#ifdef typWritefstream // 1.6.4ac
void CMarker::s_WriteRef( Object_ofstream& obs, const char* pszName )
#else
void CMarker::s_WriteRef( Object_ostream& obs, const char* pszName )
#endif
{
    obs.WriteString( psz_mkr, pszName );
}

#ifdef typWritefstream // 1.6.4ac
void CMarker::s_WriteRef( Object_ofstream& obs, const char* pszQualifier, const char* pszName )
#else
void CMarker::s_WriteRef( Object_ostream& obs, const char* pszQualifier, const char* pszName )
#endif
{
    obs.WriteString( psz_mkr, pszQualifier, pszName );
}

BOOL CMarker::s_bReadRef( Object_istream& obs, Str8& sName )
{
    return obs.bReadString( psz_mkr, sName );
}

BOOL CMarker::s_bReadRef( Object_istream& obs, const char* pszQualifier, Str8& sName )
{
    return obs.bReadString( psz_mkr, pszQualifier, sName );
}

void CMarker::SetFont( CFontDef* pfntdef ) // Create new font
{
    if ( !pfntdef )
        {
        m_bUseDefaultFont = TRUE;
        return;
        }
    else
        m_bUseDefaultFont = FALSE;

    m_iLineHeight = pfntdef->iCreateFont( &m_fnt, &m_rgbColor,
        &m_iAscent, &m_iOverhang );        
}


CFont* CMarker::SelectFontIntoDC(CDC* pDC) const
{
    ASSERT( pDC );
    return pDC->SelectObject( (CFont*)pfnt() );  // Cast away const-ness
}

CFont* CMarker::SelectDlgFontIntoDC(CDC* pDC) const
{
    ASSERT( pDC );
    return pDC->SelectObject( (CFont*)pfntDlg() );  // Cast away const-ness
}

void CMarker::SetTextAndBkColorInDC(CDC* pDC, BOOL bHighlight) const
{
    ASSERT( pDC );
    if ( bHighlight )
        {
        pDC->SetTextColor(::GetSysColor( COLOR_HIGHLIGHTTEXT));
        pDC->SetBkColor(::GetSysColor( COLOR_HIGHLIGHT));
        }
    else
        {
        pDC->SetTextColor(rgbColor());        
        pDC->SetBkColor(::GetSysColor(COLOR_WINDOW));
        }
}

void CMarker::ExtTextOut(CDC* pDC, int ixBegin, int iyBegin,
        int nOptions, const CRect* prect,
        const char* psz, int len, int lenContext) const
{
    m_plng->ExtTextOutLng(pDC, ixBegin, iyBegin, nOptions, prect,
        psz, len, lenContext);
}


const CFont* CMarker::pfntDlg() const
{
    if ( m_bUseDefaultFont || m_iLineHeight > Shw_pProject()->iLangCtrlHeight() )
        return plng()->pfntDlg(); // get trimmed font from language encoding
    else
        return &m_fnt; // return marker's font if it's not too big
}
 
int CMarker::s_iInterlinear =      0x0001;
int CMarker::s_iFirstInterlinear = 0x0002;

void CMarker::SetInterlinear(BOOL bFirst)
{
    m_iInterlinear |= s_iInterlinear;
    if ( bFirst )
        m_iInterlinear |= s_iFirstInterlinear;
}

BOOL CMarker::bFirstInterlinear() const
{
    return ( (m_iInterlinear & s_iFirstInterlinear) != 0 );
}


void CMarker::SetMarkerFollowingThis(CMarker* pmkrFollowingThis)
{
    m_pmkrFollowingThis = pmkrFollowingThis;
}

void CMarker::SetMarkerFollowingThis()
{
    ASSERT( !m_pmkrFollowingThis );
    m_pmkrFollowingThis = m_pmkrsetMyOwner->pmkrSearch(m_sMarkerFollowingThis);
    // 1997-11-26 MRP: Enabled the following condition
    if ( m_pmkrFollowingThis == m_pmkrsetMyOwner->pmkrRecord() )
        m_pmkrFollowingThis = NULL;
    m_sMarkerFollowingThis.Empty();  // No longer need the string
}

void CMarker::SetMarkerOverThis(CMarker* pmkrOverThis)
{
    ASSERT( !pmkrOverThis || !bOver(pmkrOverThis) );
    m_pmkrOverThis = pmkrOverThis;
    // 1997-11-16 MRP: Can this ever be NULL anymore?
//  if ( !m_pmkrOverThis )
//      m_pmkrOverThis = m_pmkrset->pmkrRecord();
    m_pmkrsetMyOwner->SetAllLevels();
}

void CMarker::SetMarkerOverThis()
{
    ASSERT( !m_pmkrOverThis );
    m_pmkrOverThis = m_pmkrsetMyOwner->pmkrSearch(m_sMarkerOverThis);
    // 1997-11-26 MRP: Corrected and enabled the following condition
    if ( this == m_pmkrsetMyOwner->pmkrRecord() )
        m_pmkrOverThis = NULL;
    else
        {
        m_pmkrOverThis = m_pmkrsetMyOwner->pmkrSearch(m_sMarkerOverThis);
        if ( !m_pmkrOverThis )
            m_pmkrOverThis = m_pmkrsetMyOwner->pmkrRecord();
        }
    m_sMarkerOverThis.Empty();  // No longer need the string
}

void CMarker::SetLevel()
{
    m_lev = 0;
    const CMarker* pmkrOver = pmkrOverThis();
    for ( ; pmkrOver; pmkrOver = pmkrOver->pmkrOverThis() )
        {
		if ( pmkrOver == this ) // 1.6.1bh Fix assert that stayed in infinite loop if mkr over same as mkr
			{
//			ASSERT( pmkrOver != this ); // 1.6.1bh 
			break; // 1.6.1bh 
			}
        m_lev += 1;
        }
}

    
BOOL CMarker::bUnder(const CMarker* pmkr) const
{
    ASSERT( pmkr );
    if ( lev() <= pmkr->lev() )
        return FALSE;
        
    const CMarker* pmkrOver = pmkrOverThis();
    for ( ; pmkrOver && pmkrOver != pmkr; pmkrOver = pmkrOver->pmkrOverThis() )
        ASSERT( pmkrOver != this );

    return pmkrOver == pmkr;
}
    
BOOL CMarker::bOver(const CMarker* pmkr) const
{
    ASSERT( pmkr );
    if ( pmkr->lev() <= lev() )
        return FALSE;
        
    const CMarker* pmkrOver = pmkr->pmkrOverThis();
    for ( ; pmkrOver && pmkrOver != this; pmkrOver = pmkrOver->pmkrOverThis() )
        ASSERT( pmkrOver != pmkr );

    return pmkrOver == this;
}
    
BOOL CMarker::bEqualOrOver(const CMarker* pmkr) const
{
    ASSERT( pmkr );
    return pmkr == this || bOver(pmkr);
}

const CMarker* CMarker::pmkrLowestOverBoth(const CMarker* pmkr) const
{
    ASSERT( pmkr );
    const CMarker* pmkrOver = pmkrOverThis();
    for ( ; pmkrOver && !pmkrOver->bOver(pmkr); pmkrOver = pmkrOver->pmkrOverThis() )
        ASSERT( pmkrOver != this );
            
    return pmkrOver;  // 1996-07-30 MRP: Can return NULL
}

BOOL CMarker::bAtHigherLevelThan(const CMarker* pmkr) const
{
    ASSERT( pmkr );
    return lev() < pmkr->lev();  // Higher levels have lesser values
}


// --------------------------------------------------------------------------

void CMarkerPtr::IncrementNumberOfRefs(CMarker* pmkr)
{
    m_pmkr = pmkr;
    if ( m_pmkr )
        m_pmkr->IncrementNumberOfRefs();
}

void CMarkerPtr::DecrementNumberOfRefs()
{
    if ( m_pmkr )
        m_pmkr->DecrementNumberOfRefs();
}


const CMarkerPtr& CMarkerPtr::operator=(CMarker* pmkr)
{
    if ( m_pmkr != pmkr )
        {
        DecrementNumberOfRefs();
        IncrementNumberOfRefs(pmkr);
        }
        
    return *this;
}


// --------------------------------------------------------------------------

void CFieldMarkerPtr::IncrementNumberOfRefs(CMarker* pmkr)
{
    m_pmkr = pmkr;
    if ( m_pmkr )
        m_pmkr->IncrementNumberOfFieldRefs();
}

void CFieldMarkerPtr::DecrementNumberOfRefs()
{
    if ( m_pmkr )
        m_pmkr->DecrementNumberOfFieldRefs();
}


const CFieldMarkerPtr& CFieldMarkerPtr::operator=(CMarker* pmkr)
{
    if ( m_pmkr != pmkr )
        {
        DecrementNumberOfRefs();
        IncrementNumberOfRefs(pmkr);
        }
        
    return *this;
}


// --------------------------------------------------------------------------

CMarkerUpdate::CMarkerUpdate(CMarker* pmkr, BOOL bToBeDeleted)
{
    m_pmkr = pmkr;
    ASSERT( m_pmkr );
    m_pmkrMerge = NULL;
    m_bModified = FALSE;
    m_bNameModified = FALSE;
    m_bLangEncModified = FALSE;
    m_bToBeDeleted = bToBeDeleted;
    m_pszOldName = NULL;
}

CMarkerUpdate::CMarkerUpdate(CMarker* pmkr, const char* pszName,
        CLangEnc* plng)
{
    m_pmkr = pmkr;
    ASSERT( m_pmkr );
    m_pmkrMerge = NULL;
    m_bModified = TRUE;
    m_bNameModified = !bEqual(pszName, m_pmkr->sName());
    if ( m_bNameModified )
        m_pszOldName = m_pmkr->sName(); // name hasn't changed yet, hang onto it
    else
        m_pszOldName = NULL;
    ASSERT( plng );
    m_bLangEncModified = ( plng != m_pmkr->plng() );
	m_bToBeDeleted = FALSE;
}

CMarkerUpdate::CMarkerUpdate(CMarker* pmkr, CMarker* pmkrMerge)
{
    m_pmkr = pmkr;
    m_pmkrMerge = pmkrMerge;
    ASSERT( m_pmkr && m_pmkrMerge );
    m_bModified = TRUE;
    m_bNameModified = FALSE;
    m_bLangEncModified = FALSE;
    m_pszOldName = NULL;
	m_bToBeDeleted = FALSE;
}


// --------------------------------------------------------------------------

CMarkerSet::CMarkerSet(CLangEncSet* plngset)
{
    m_plngset = plngset;
    ASSERT( m_plngset );
    m_plngDefault = m_plngset->plngDefault();
    ASSERT( m_plngDefault );
    m_pmkrRecord = NULL;  // 1997-11-26 MRP
    m_bSubfields = FALSE;
    m_ptypMyOwner = NULL;  // 2000-07-20 MRP

    m_maxlev = 0;
}

CMarkerSet::~CMarkerSet()
{
}


CMarker* CMarkerSet::pmkrSearch_AddIfNew( const char* pszMarker )
{
    CMarker* pmkr = pmkrSearch(pszMarker);
    if ( pmkr == NULL )
        {
        //08-18-1997 Added 0 (=iRangeSetCharacter) and
        // TRUE (=bRangeSetSpaceAllowed) to Paramterlist
        pmkr = new CMarker(pszMarker, m_plngDefault, this);
        ASSERT(pmkr);
        Add(pmkr);
        }
        
    return pmkr;
}

// 04/19/2000 - TLB: New version of function to support quick Interlinear setup with hierarchy
CMarker* CMarkerSet::pmkrSearch_AddIfNew( const char* pszMarker, CMarker* pmkrOverNew, Str8 sFieldName )
{
    CMarker* pmkr = pmkrSearch(pszMarker);
    if ( pmkr == NULL )
        {
        pmkr = new CMarker(pszMarker, m_plngDefault, this, sFieldName);
        ASSERT(pmkr);
        Add(pmkr);
        pmkr->SetMarkerOverThis(pmkrOverNew);
        }
        
    return pmkr;
}

CMarker* CMarkerSet::pmkrSearchForWholeSubString( const char* pszName,
        Length len ) const
{
    return (CMarker*)pselSearchForWholeSubString(pszName, len);
}

CMarker* CMarkerSet::pmkrAdd_MarkAsNew( const char* pszMarker )
{
     //08-18-1997 Added 0 (=iRangeSetCharacter) and
    // TRUE (=bRangeSetSpaceAllowed) to Paramterlist
    CMarker* pmkr = new CMarker(pszMarker, m_plngDefault, this, "*");
    Add(pmkr);
    pmkr->SetMarkerOverThis();  // 1997-11-26 MRP
    SetAllLevels();  // 2000-06-21 MRP
    return pmkr;
}

void CMarkerSet::Add( CMarker* pmkrNew )
{
    ASSERT( pmkrNew != NULL );
    ASSERT( pmkrSearch(pmkrNew->sMarker()) == NULL );
    CSet::Add( pmkrNew );
}


// This is the full version (used by the marker properties sheet).
BOOL CMarkerSet::bAddMarker(const char* pszName, const char* pszDescription, 
            const char* pszFieldName, CLangEnc* plng,
            CMarker* pmkrOverThis, CMarker* pmkrFollowingThis,
            CFontDef* pfntdef,
            BOOL bMultipleItemData, BOOL bMultipleWordItems, BOOL bMustHaveData,
            BOOL bNoWordWrap, BOOL bCharStyle,
            CMarker** ppmkr, CNote** ppnot)
{
    if ( pmkrSearch(pszName) != NULL )
        {
        *ppnot = new CNote(_("This marker already exists:"), pszName, // 1.5.0ft 
            pszName, strlen(pszName), pszName);
        return FALSE;
        }

    if ( !bValidNewFieldName(pszFieldName, ppnot) )
        return FALSE;
    const char* pszMarkerOverThis = ""; // 1.4tge Fix possible U bug in marker over this on add marker
	if ( pmkrOverThis ) // 1.4tge 
		pszMarkerOverThis = pmkrOverThis->sName(); // 1.4tge 
    const char* pszMarkerFollowingThis = ""; // 1.4tge 
	if ( pmkrFollowingThis ) // 1.4tge 
		pszMarkerFollowingThis = pmkrFollowingThis->sName(); // 1.4tge Fix U bug of add marker losing none for marker following
    CMarker* pmkr = NULL;
     //08-18-1997 Added iRangeSetCharacter & bRangeSetSpaceAllowed to Paramterlist
    if ( !CMarker::s_bConstruct(pszName, pszDescription, pszFieldName, plng,
            pszMarkerOverThis, pszMarkerFollowingThis,
            //pszRangeSet, iRangeSetCharacter, bRangeSetSpaceAllowed,
            pfntdef, bMultipleItemData, bMultipleWordItems, bMustHaveData,
            bNoWordWrap, bCharStyle, 0, FALSE, FALSE,
            this, &pmkr, ppnot) )
        return FALSE;
        
    ASSERT(pmkr);

    Add(pmkr);
    pmkr->SetMarkerOverThis();
    pmkr->pmkrsetMyOwner()->SetAllLevels();
    pmkr->SetMarkerFollowingThis(); // 1.4qrb Fix bug of not keeping following field marker on add marker
    
    CMarkerUpdate mup(pmkr);
    extern void Shw_Update(CUpdate& up);        
    Shw_Update(mup);  // Notify other objects that this marker is added
    
    *ppmkr = pmkr;
    return TRUE;
}

// This is the skimpy version (used by Wordlist feature)
BOOL CMarkerSet::bAddMarker(const char* pszName, const char* pszDescription, const char* pszFieldName,
            CLangEnc* plng, CNote** ppnot, BOOL bSingleMultiWordDataItem)
{
    if ( pmkrSearch(pszName) != NULL )
        return FALSE;

    if ( !bValidNewFieldName(pszFieldName, ppnot) )
        return FALSE;

    CMarker* pmkr = new CMarker(pszName, plng, this, pszFieldName, pszDescription);
    ASSERT(pmkr);
    Add(pmkr);

    if ( bSingleMultiWordDataItem )
        pmkr->SetDataType(FALSE, TRUE);

    pmkr->SetMarkerOverThis();
    SetAllLevels();
    
    CMarkerUpdate mup(pmkr);
    extern void Shw_Update(CUpdate& up);        
    Shw_Update(mup);  // Notify other objects that this marker is added
    
    return TRUE;
}

BOOL CMarkerSet::bUnicodeLang() // Return true if any marker in set uses a Unicode language
	{
    for ( CMarker* pmkr = pmkrFirst(); pmkr; pmkr = pmkrNext(pmkr) )
        if ( pmkr->plng()->bUnicodeLang() )
            return TRUE;
	return FALSE;
	}

#ifdef TLB_2000_06_15_NOT_USED
BOOL CMarkerSet::bValidNewMarkerName(const char* pszNewName, CNote** ppnot)
{
    if ( !CMarker::s_bValidName(pszNewName, ppnot) )
        return FALSE;
        
    if ( pmkrSearch(pszNewName) != NULL )
        {
        *ppnot = new CNote(_("This marker already exists:"), pszNewName, // 1.5.0ft 
            pszNewName, strlen(pszNewName));
        return FALSE;
        }

    return TRUE;    
}
#endif

BOOL CMarkerSet::bValidName(const char* pszName, CNote** ppnot)
{
    if ( !CSet::bValidName(pszName, ppnot) ) // Explicitly call the base class implementation
        return FALSE;
    
    // 1996-03-13 MRP: KEEP SIMPLIFYING MARKER NAME MATCHING AND VALIDITY!!!
    if ( !CMarker::s_bValidName(pszName, ppnot) )
        return FALSE;
        
    return TRUE;
}

BOOL CMarkerSet::bValidNewFieldName(const char* pszFieldName, CNote** ppnot) const
{
    ASSERT( pszFieldName );
    if ( *pszFieldName != '\0' &&  // 1995-07017 MRP: for now, allow empty
            !CMarker::s_bValidFieldName(pszFieldName, ppnot) )
        {
        (*ppnot)->SetStringContainingReferent(pszFieldName);
        return FALSE;
        }

    if ( *pszFieldName && *pszFieldName != '*' )
        {
        const CMarker* pmkr = pmkrSearchForFieldName(pszFieldName);
        if ( pmkr )
            {
            *ppnot = new CNote(_("Another marker already has this Field Name:"), pszFieldName, pszFieldName, strlen(pszFieldName), pszFieldName); // 1.5.0ft 
            return FALSE;
            }
        }
        
    return TRUE;
}

CMarker* CMarkerSet::pmkrSearchForFieldName(const char* pszFieldName) const
{
    ASSERT( pszFieldName );
    CMarker* pmkr = pmkrFirst();
    for ( ; pmkr; pmkr = pmkrNext(pmkr) )
        if ( bEqual(pmkr->sFieldName(), pszFieldName) )
            break;

    return pmkr;
}


void CMarkerSetListBox::InitLabels()
{
    m_xMarker = xSubItem_Show(IDC_mkrset_lblMarker);
    m_xFieldName = xSubItem_Show(IDC_mkrset_lblFieldName);
    m_xLangEnc = xSubItem_Show(IDC_mkrset_lblLanguage);
    m_xUnder = xSubItem_Show(IDC_mkrset_lblUnder);
    m_xMisc = xSubItem_Show(IDC_mkrset_lblMisc);

    m_chParagraph = 'P'; // 1.3aj Bring in marker set list box labels from resources
    m_chCharacter = 'C';
    m_chFont = 'F';
    m_chRangeSet = 'R';
}

void CMarkerSetListBox::DrawElement(CDC& cDC, const RECT& rcItem,
            const CDblListEl *pel)
{
    CMarker* pmkr = (CMarker*)pel;
    const Length maxlen = 20;
    char pszMarkerBuffer[maxlen + 1];
    Length len = maxlen;
    pmkr->DrawMarker(pszMarkerBuffer, &len, FALSE, TRUE, TRUE, FALSE);
    DrawSubItem(cDC, rcItem, m_xMarker, m_xFieldName, pszMarkerBuffer);
    DrawSubItem(cDC, rcItem, m_xFieldName, m_xLangEnc, pmkr->sFieldName());
    DrawSubItem(cDC, rcItem, m_xLangEnc, m_xUnder, pmkr->sLangEnc());
    const CMarker* pmkrOverThis = pmkr->pmkrOverThis();
    const char* pszMarkerOverThis = ( pmkrOverThis ? pmkrOverThis->sName() : "" );
    DrawSubItem(cDC, rcItem, m_xUnder, m_xMisc, pszMarkerOverThis);

    char pszMisc[4];
    pszMisc[0] = ( pmkr->bCharStyle() ? m_chCharacter : m_chParagraph );
    pszMisc[1] = ( pmkr->bUsingDefaultFont() ? ' ' : m_chFont );
    // Putting the range set second improves the alignment,
    // since few markers have them.
    pszMisc[2] = ( pmkr->bRangeSetEnabled() ? m_chRangeSet : ' ' );
    pszMisc[3] = '\0';
    DrawSubItem(cDC, rcItem, m_xMisc, 0, pszMisc);
}


BOOL CMarkerSet::bModalView_Elements(CMarker** ppmkrSelected)
{
    return FALSE;
}

void CMarkerSet::Delete(CSetEl** ppsel)
{
    ASSERT( ppsel );
    CMarker* pmkrToBeDeleted = (CMarker*)*ppsel;
    ASSERT( pmkrToBeDeleted );
    CMarker* pmkrOverOneToBeDeleted = pmkrToBeDeleted->pmkrOverThis();
    CMarker* pmkr = pmkrFirst();
    for ( ; pmkr; pmkr = pmkrNext(pmkr) )
        if ( pmkr->pmkrOverThis() == pmkrToBeDeleted )
            {
            ASSERT( pmkr != pmkrToBeDeleted );
            pmkr->SetMarkerOverThis(pmkrOverOneToBeDeleted);
            }
    CMarkerUpdate mup(pmkrToBeDeleted, TRUE);
    extern void Shw_Update(CUpdate& up);        
    Shw_Update(mup);  // Notify other objects that this marker is deleted
            
    CSet::Delete(ppsel);
}

void CMarkerSet::RemoveFromHierarchy( CMarker* pmkrRemove ) // get rid of any hierarchical dependencies on pmkr
{
    CMarker* pmkr = pmkrFirst();
    for ( ; pmkr; pmkr = pmkrNext(pmkr) )
        {
        if ( pmkr->pmkrOverThis() == pmkrRemove )
            pmkr->SetMarkerOverThis(NULL); // just put under record marker
        if ( pmkr->pmkrFollowingThis() == pmkrRemove )
            pmkr->SetMarkerFollowingThis(NULL);
        }
}

BOOL CMarkerSet::bModalView_NewMarker(CMarker** ppmkrNew, const char* pszMarker )
{
    CMarkerPropertiesSheet dlg(this, ppmkrNew, pszMarker );
    return ( dlg.DoModal() == IDOK );  // show a modal view of a new marker
}

BOOL CMarkerSet::bAdd(CSetEl** ppselNew)
{
    return bModalView_NewMarker((CMarker**)ppselNew);
}

void CMarkerSet::SetDefaultLangEnc(CLangEnc* plng)
{
    m_plngDefault = plng;
    ASSERT( m_plngDefault );
}

int CMarkerSet::iCopyMarkerIndent(char* psz, int iMaxCopy, Level lev) const
{
    ASSERT( lev <= m_maxlev );
    int len = lev + lev + lev;  // 1996-07-30 MRP: Three characters ".  " indent per level
    int iCnt = min( iMaxCopy, len );
    memcpy(psz, m_sIndent, iCnt);
    *(psz + iCnt) = '\0'; // null terminate
    return iCnt; // return number of chars copied
}

CMarkerHierarchyComboBox::CMarkerHierarchyComboBox(CMarkerSet* pmkrset, CMarker* pmkr) :
CSetComboBox(pmkrset, (pmkr)? (pmkr == pmkr->pmkrsetMyOwner()->pmkrRecord()) : FALSE, NULL)
    // 1997-11-26 MRP: Only the record marker gets a [none] element for Modify
    // 1997-11-26 MRP: No [none] element for Add and Copy
{
    m_pmkr = pmkr;
}
    
void CMarkerHierarchyComboBox::UpdateElements(CMarker* pmkrToSelect)
{
    CSetComboBox::UpdateElements( pmkrToSelect );
}

CMarker* CMarkerHierarchyComboBox::pmkrSelected()
{
    return (CMarker*)CSetComboBox::pselSelected();
}

BOOL CMarkerHierarchyComboBox::bIncludeElement(const CSetEl* psel) const
{
    const CMarker* pmkr = (const CMarker*)psel;
    ASSERT( pmkr );
    return ( m_pmkr ?
        pmkr != m_pmkr && !pmkr->bSubfield() && !pmkr->bUnder(m_pmkr) :
        TRUE );
}


#ifdef _DEBUG
void CMarkerSet::AssertValid() const
{
    ASSERT( m_plngset );
    ASSERT( m_plngset->bIsMember(m_plngDefault) );
    
    // 1995-05-17 MRP: Can the ordering be verified in CLookup::AssertValid?
    Str8 sMarkerPrev;
    const CMarker* pmkr = pmkrFirst();  
    for ( ; pmkr; pmkr = pmkrNext( pmkr ) )
        {
        // Set's lookup key must be identical with Marker's string
        ASSERT( bEqual(pmkr->sMarker(), pmkr->pszKey()) );
        
        // Each marker in the set must be unique
        if ( !bIsFirst(pmkr) )
            ASSERT( !bEqual(pmkr->sMarker(), sMarkerPrev) );
        sMarkerPrev = pmkr->sMarker();
        
        // Marker must be valid-- a "deep" AssertValid on the set
        pmkr->AssertValid();    
        }
}
#endif  // _DEBUG


static const char* psz_mkrset = "mkrset";
static const char* psz_Default = "Default";
static const char* psz_mkrRecord = "mkrRecord";  // 1997-11-26 MRP
static const char* psz_Subfields = "Subfields";

#ifdef typWritefstream // 1.6.4ac
void CMarkerSet::WriteProperties(Object_ofstream& obs) const
#else
void CMarkerSet::WriteProperties(Object_ostream& obs) const
#endif
{
    obs.WriteBeginMarker(psz_mkrset);
    m_plngDefault->WriteRef(obs, psz_Default);
    obs.WriteString(psz_mkrRecord, m_pmkrRecord->sName());  // 1997-11-26 MRP
    obs.WriteBool(psz_Subfields, m_bSubfields);
    obs.WriteNewline();
   
	const CMarker* pmkr = pmkrFirst();
    for ( ; pmkr; pmkr = pmkrNext(pmkr) )
        pmkr->WriteProperties(obs);
       
    obs.WriteEndMarker(psz_mkrset);
    obs.WriteNewline();

    // 3/30/2000 TLB : Now go back and catch any new-style multi-word range sets or empty range sets.
    // These are written wrapped in \+rngset ... \-rngset form so that older
    // versions of the program will ignore them.
    for ( pmkr = pmkrFirst(); pmkr; pmkr = pmkrNext(pmkr) )
        if ( pmkr->bUseRangeSet() &&
             ( ( pmkr->bMultipleWordItems() && !pmkr->prngset()->bCharacterItems() ) ||
               pmkr->prngset()->bIsEmpty() ) )
            pmkr->prngset()->WriteProperties(obs);
}

BOOL CMarkerSet::bReadProperties(Object_istream& obs)
{
    if ( !obs.bReadBeginMarker(psz_mkrset) )
        return FALSE;

    CLangEnc* plng = NULL;
    while ( !obs.bAtEnd() )
        {
        if ( m_plngset->bReadLangEncRef(obs, psz_Default, plng) )
            m_plngDefault = plng;
        else if ( obs.bReadBool(psz_Subfields, m_bSubfields) )
            ;
        else
            break;
        }
    ASSERT( m_plngDefault );

    while ( !obs.bAtEnd() )
        {
        CMarker* pmkr = NULL;
        if ( CMarker::s_bReadProperties(obs, this, &pmkr) )
            {
            if ( pmkr ) // 2000-06-15 TLB: The Add function ASSERTs if pmkr is NULL
                Add(pmkr);
            }
        else if ( obs.bEnd(psz_mkrset) )
            break;
        }

// 1997-11-26 MRP: Move to SetRecordMarker and ChangeRecordMarker
//    // Resolve hierarchy forward references and compute derived attributes
//    // 1995-12-02 MRP: Should this become a member function???
//    CMarker* pmkr = pmkrFirst();
//    for ( ; pmkr; pmkr = pmkrNext(pmkr) )
//        {
//        pmkr->SetMarkerFollowingThis();
//        pmkr->SetMarkerOverThis();
//        }
//    SetAllLevels();

    return TRUE;
}

BOOL CMarkerSet::bReadMarkerRef(Object_istream& obs, const char* pszQualifier,
        CMarker*& pmkr) const
{
    Str8 sName;
    if ( !obs.bReadString(psz_mkr, pszQualifier, sName) )
        return FALSE;
        
    CMarker* pmkrFound = pmkrSearch(sName);
    if ( !pmkrFound )
        return FALSE;
        
    pmkr = pmkrFound;
    return TRUE;
}


void CMarkerSet::SetInterlinear( const CInterlinearProcList* pintprclst ) // Set interlinear properties of all markers referred to in process list
{
    // Clear the interlinear properties of all markers.
    CMarker* pmkr = pmkrFirst();
    for ( ; pmkr; pmkr = pmkrNext(pmkr) )
        pmkr->ClearInterlinear();

    // Set the interlinear properties of markers referred to in the process list.
    BOOL bFirst = TRUE;
    const CInterlinearProc* pintprc = pintprclst->pintprcFirst();
    for ( ; pintprc; pintprc = pintprclst->pintprcNext(pintprc) ) // For each process
        {
        if ( pintprc->pmkrFrom() ) // Some processes such as hand entry or free translation don't have an input marker
			{
			if ( !pintprc->pmkrFrom()->bInterlinear() ) // 1.6.4zn If not already interlinear set it, fix bug of return from jump going up too far
			  pintprc->pmkrFrom()->SetInterlinear(bFirst); // Set input marker as interlinear, set first if first process.
			}
        bFirst = FALSE;
        pintprc->pmkrTo()->SetInterlinear(); // Set output marker as interlinear
        }
}


void CMarkerSet::SetRecordMarker(CMarker* pmkrRecord)
{
    ASSERT( pmkrRecord );
    m_pmkrRecord = pmkrRecord;  // 1997-11-26 MRP

    // 1997-11-26 MRP: Moved from bReadProperties
    CMarker* pmkr = pmkrFirst();
    for ( ; pmkr; pmkr = pmkrNext(pmkr) )
        {
        pmkr->SetMarkerFollowingThis();
        pmkr->SetMarkerOverThis();
        }
    SetAllLevels();

// 1997-11-16 MRP: Do in SetMarkerFollowingThis
//    CMarker* pmkr = pmkrFirst();
//    for ( ; pmkr; pmkr = pmkrNext(pmkr) )
//        if ( pmkr->pmkrFollowingThis() == pmkrRecord )
//            pmkr->m_pmkrFollowingThis = NULL;
}

void CMarkerSet::ChangeRecordMarker(CMarker* pmkrFrom, CMarker* pmkrTo)
{
    ASSERT( pmkrFrom );
    ASSERT( pmkrTo );
    if ( pmkrFrom == pmkrTo )
        return;

    m_pmkrRecord = pmkrTo;  // 1997-11-26 MRP
    pmkrTo->m_pmkrOverThis = NULL;
    pmkrFrom->m_pmkrOverThis = pmkrTo;
    SetAllLevels();
}

#ifdef _MAC
//static const char s_chHierarchyLevel = '\xE1';  // d225: light centered dot
//static const char s_chHierarchyLevel = '\xA5';  // d165: heavy centered dot
// 1996-06-04 MRP: Apparently some Mac fonts have the 'blotch' at these codes
#else
//static const char s_chHierarchyLevel = '\xB7';  // d183: light centered dot
//static const char s_chHierarchyLevel = '\x95';  // d149: heavy centered dot
// 1996-05-27 MRP: A blotch in MS Sans Serif, ugh yuch!
#endif
static const char s_chHierarchyLevel = '.';  // 1996-06-04 MRP: period

void CMarkerSet::SetAllLevels() // Set hierarchical nesting levels of all markers and set max level
{
    m_maxlev = 0;
    CMarker* pmkr = pmkrFirst();
    for ( ; pmkr; pmkr = pmkrNext(pmkr) )
        {
        pmkr->SetLevel();
        if ( m_maxlev < pmkr->lev() )
            m_maxlev = pmkr->lev();
        }
        
    int len = m_maxlev + m_maxlev + m_maxlev;
    if ( m_sIndent.GetLength() != len )
        {
        m_sIndent.Empty();
        for ( Level i = 0; i != m_maxlev; i++ )
            m_sIndent += ".  ";
        }
}


// ==========================================================================

CMarkerSubSet::CMarkerSubSet(CMarkerSet* pmkrset, BOOL bIncludeAll) :
    CSubSet(pmkrset, bIncludeAll, FALSE)
{
}

CMarkerSubSet::CMarkerSubSet(const CMarkerSubSet& subset) :
    CSubSet(subset)
{
}

CMarkerSubSet::~CMarkerSubSet()
{
}

void CMarkerSubSet::operator=(const CMarkerSubSet& subset)
{
    if ( this == &subset )
        return;

    this->CSubSet::operator=(subset);  // Base class assignment operator
}

static const char* psz_mkrsubsetIncluded = "mkrsubsetIncluded";
static const char* psz_mkrsubsetExcluded = "mkrsubsetExcluded";

#ifdef prjWritefstream // 1.6.4aa 
void CMarkerSubSet::WriteProperties(Object_ofstream& obs) const
#else
void CMarkerSubSet::WriteProperties(Object_ostream& obs) const
#endif
{
    if ( m_bIncludeAllElements )
        return;

    if ( m_bAutoIncludeNewElements )
        {
        obs.WriteBeginMarker(psz_mkrsubsetExcluded);
        const CMarker* pmkr = pmkrset()->pmkrFirst();
        for ( ; pmkr; pmkr = pmkrset()->pmkrNext(pmkr) )
            if ( !bIncludes(pmkr) )
                pmkr->WriteRef(obs, "");
        obs.WriteEndMarker(psz_mkrsubsetExcluded);
        }
    else
        {
        obs.WriteBeginMarker(psz_mkrsubsetIncluded);
        const CMarker* pmkr = pmkrset()->pmkrFirst();
        for ( ; pmkr; pmkr =pmkrset()->pmkrNext(pmkr) )
            if ( bIncludes(pmkr) )
                pmkr->WriteRef(obs, "");
        obs.WriteEndMarker(psz_mkrsubsetIncluded);
        }
}

BOOL CMarkerSubSet::bReadProperties(Object_istream& obs)
{
    if ( obs.bReadBeginMarker(psz_mkrsubsetExcluded) )
        {
        m_bIncludeAllElements = FALSE;
        m_bAutoIncludeNewElements = TRUE;
        CMarker* pmkr = pmkrset()->pmkrFirst();
        for ( ; pmkr; pmkr = pmkrset()->pmkrNext(pmkr) )
            Include(pmkr, TRUE);
        
        while ( !obs.bAtEnd() )
            {
            pmkr = NULL;
            if ( pmkrset()->bReadMarkerRef(obs, "", pmkr) )
                {
                // 1997-11-13 MRP: Check whether pmkr == NULL,
                // if there's a reference to an undefined marker.
                if ( pmkr )
                    Include(pmkr, FALSE);
                }
            else if ( obs.bEnd( psz_mkrsubsetExcluded ) )
                // If end marker or unexpected begin break
                break;
            }
            
        return TRUE;
        }

    if ( obs.bReadBeginMarker(psz_mkrsubsetIncluded) )
        {
        m_bIncludeAllElements = FALSE;
        m_bAutoIncludeNewElements = FALSE;
        CMarker* pmkr = pmkrset()->pmkrFirst();
        for ( ; pmkr; pmkr = pmkrset()->pmkrNext(pmkr) )
            Include(pmkr, FALSE);
        
        while ( !obs.bAtEnd() )
            {
            pmkr = NULL;
            if ( pmkrset()->bReadMarkerRef(obs, "", pmkr) )
                {
                // 1997-11-13 MRP: Check whether pmkr == NULL,
                // if there's a reference to an undefined marker.
                if ( pmkr )
                    Include(pmkr, TRUE);
                }
            else if ( obs.bEnd( psz_mkrsubsetIncluded ) )
                // If end marker or unexpected begin break
                break;
            }
            
        return TRUE;
        }

    return FALSE;
}

#ifdef typWritefstream // 1.6.4ac
void CMarkerSubSet::WriteOldProperties(Object_ofstream& obs,
        const char* pszMarker) const
#else
void CMarkerSubSet::WriteOldProperties(Object_ostream& obs,
        const char* pszMarker) const
#endif
{
    if ( m_bIncludeAllElements )
        return;

    ASSERT( m_bAutoIncludeNewElements );
    obs.WriteBeginMarker(pszMarker);
    const CMarker* pmkr = pmkrset()->pmkrFirst();
    for ( ; pmkr; pmkr = pmkrset()->pmkrNext(pmkr) )
        if ( !bIncludes(pmkr) )
            pmkr->WriteRef(obs, "");
    obs.WriteEndMarker(pszMarker);
}

BOOL CMarkerSubSet::bReadOldProperties(Object_istream& obs,
        const char* pszMarker)
{
    ASSERT( pszMarker );
    if ( !obs.bReadBeginMarker(pszMarker) )
        return FALSE;

    m_bIncludeAllElements = FALSE;
    m_bAutoIncludeNewElements = TRUE;
    CMarker* pmkr = pmkrset()->pmkrFirst();
    for ( ; pmkr; pmkr = pmkrset()->pmkrNext(pmkr) )
        Include(pmkr, TRUE);
        
    while ( !obs.bAtEnd() )
        {
        pmkr = NULL;
        if ( pmkrset()->bReadMarkerRef(obs, "", pmkr) )
            {
            // 1997-11-13 MRP: Check whether pmkr == NULL,
            // if there's a reference to an undefined marker.
            if ( pmkr )
                Include(pmkr, FALSE);
            }
        else if ( obs.bEnd(pszMarker) )
            // If end marker or unexpected begin break
            break;
        }
            
    return TRUE;
}

BOOL CMarkerSubSet::bIncludes(const CMarker* pmkr) const
{
    return bIncludesElement(pmkr);
}

void CMarkerSubSet::Include(CMarker* pmkr, BOOL bInclude)
{
    IncludeElement(pmkr, bInclude);
}

 // ==========================================================================
BOOL CMarkerRef::bMakeRef( CMarkerSet* pmkrset ) // Make the marker reference to a member of the marker set from the marker string
{ 
    ASSERT( pmkrset );
    if ( !m_pmkr )
        m_pmkr = pmkrset->pmkrSearch_AddIfNew( m_sMarker ); // 8-1-96 BJY Make it succeed
    return ( m_pmkr != NULL );
}

void CMarkerRef::NameModified( const char* pszNewName, const char* pszOldName ) // handle marker name change
{
    if ( !strcmp( m_sMarker, pszOldName ) ) // are we referencing the modified marker?
        m_sMarker = pszNewName;
}

CMarkerRefList::CMarkerRefList( const CMarkerRefList& mrflst ) // Copy constructor
{
    for ( const CMarkerRef* pmrf = mrflst.pmrfFirst(); pmrf; pmrf = mrflst.pmrfNext( pmrf ) )
        Add( new CMarkerRef( *pmrf ) );
}

CMarkerRefList::~CMarkerRefList()
{
    CMarkerRef* pmrf = pmrfFirst();
    while (pmrf)
        {
        Delete(&pmrf);
        pmrf = pmrfFirst();
        }
    // DeleteAll();
}


#ifdef typWritefstream // 1.6.4ac
void CMarkerRefList::WriteProperties(Object_ofstream& obs, const char* pszMarker) const
#else
void CMarkerRefList::WriteProperties(Object_ostream& obs, const char* pszMarker) const
#endif
{
    if ( bIsEmpty() )
        return;
        
    obs.WriteBeginMarker(pszMarker);
    for ( const CMarkerRef* pmrf = pmrfFirst(); pmrf; pmrf = pmrfNext(pmrf) )
        {
        if ( pmrf->pmkr() ) // If marker defined, have it write itseld
            pmrf->pmkr()->WriteRef(obs, "");
        else // Else (marker not defined yet, write its string
            obs.WriteString( psz_mkr, pmrf->m_sMarker );    
        }
    obs.WriteEndMarker(pszMarker);
}

BOOL CMarkerRefList::bReadProperties(Object_istream& obs, const char* pszMarker,
        const CMarkerSet* pmkrset)
{
    if ( !obs.bReadBeginMarker( pszMarker ) )
        return FALSE;

    m_pmkrset = pmkrset; // Set marker set as given, may be null 
    if ( pmkrset ) // If marker set available use it
        {       
        CMarker* pmkr = NULL;
        while ( pmkrset->bReadMarkerRef( obs, "", pmkr ) ) // While a marker in the marker set is found, add it to list
            AddMarkerRef( pmkr );
        }
    else // Else (no marker set) read markers as strings
        {
        Str8 sName;
        while ( obs.bReadString( psz_mkr, sName ) ) // While an mkr line, add the marker name to the list
            AddMarkerRef( NULL, sName );
        }   
        
    if ( !obs.bReadEndMarker( pszMarker ) )
        return FALSE;
        
    return TRUE;
}

void CMarkerRefList::operator=( const CMarkerRefList &mrflst ) // Copy
{
    if (&mrflst == this)  // self-assignment
        return;
    DeleteAll(); // Make sure we are clear
    for ( const CMarkerRef* pmrf = mrflst.pmrfFirst(); pmrf; pmrf = mrflst.pmrfNext( pmrf ) )
        AddMarkerRef( pmrf->m_pmkr, pmrf->m_sMarker );
}

BOOL CMarkerRefList::operator==( const CMarkerRefList &mrflst ) const // Comparison
{
    if ( this->lGetCount() != mrflst.lGetCount() ) // If different length, fail
        return FALSE;
    const CMarkerRef* pmrf1 = this->pmrfFirst();    
    for ( const CMarkerRef* pmrf = mrflst.pmrfFirst(); 
            pmrf; 
            pmrf1 = this->pmrfNext( pmrf1 ), pmrf = mrflst.pmrfNext( pmrf ) )
        if ( pmrf1->pmkr() != pmrf->pmkr() )    
            return FALSE; // If any mismatch, fail
    return TRUE; // If all the same, succeed
}
 
BOOL CMarkerRefList::bMakeRefs( CMarkerSet* pmkrset ) // Make references from strings
{
    BOOL bSucc = TRUE; // Default return to successful
    m_pmkrset = pmkrset;
    for ( CMarkerRef* pmrf = pmrfFirst(); pmrf; ) // For each marker in list
        {
        if ( pmrf->bMakeRef( pmkrset ) ) // If make reference is successful, move to next
            pmrf = pmrfNext( pmrf ); 
        else // Else (can't make reference) delete this one and move to next
            {
            Delete( &pmrf );    
            bSucc = FALSE; // Note for return that there was at least one failure
            }
        }
    return bSucc;   
}

void CMarkerRefList::AddMarkerRef( CMarker* pmkr, const char* pszName )
{
    CMarkerRef* pmrf = new CMarkerRef( pmkr, pszName );
    Add( pmrf );
}

BOOL CMarkerRefList::bSearch( const CMarker* pmkr ) const // Search for given marker        
{
    for ( CMarkerRef* pmrf = pmrfFirst(); pmrf; pmrf = pmrfNext( pmrf ) ) // For each marker in list
        if ( pmrf->pmkr() == pmkr ) // If it matches, return true
            return TRUE;
    return FALSE; // If no matches, return false
}

BOOL CMarkerRefList::bEqual(const CMarkerRefList& mrflst) const
{
    const CMarkerRef* pmrfThis = pmrfFirst();
    const CMarkerRef* pmrf = mrflst.pmrfFirst();
    while ( pmrfThis && pmrf )
        {
        if ( pmrfThis->pmkr() != pmrf->pmkr() )
            return FALSE;
            
        pmrfThis = pmrfNext(pmrfThis);
        pmrf = mrflst.pmrfNext(pmrf);
        }
        
    return ( !pmrfThis && !pmrf );
}

void CMarkerRefList::ClearRefs() // Clear the mrf pointers, for cleanup before deletion of database types
{
    for ( CMarkerRef* pmrf = pmrfFirst(); pmrf; pmrf = pmrfNext( pmrf ) ) // For each marker in list
        pmrf->ClearRef(); // clear it's pointer
}

void CMarkerRefList::NameModified( const char* pszNewName, const char* pszOldName ) // handle marker name change
{
    for ( CMarkerRef* pmrf = pmrfFirst(); pmrf; pmrf = pmrfNext( pmrf ) ) // For each marker in list
        pmrf->NameModified( pszNewName, pszOldName ); // check for name change
}

// ==========================================================================

CMString::CMString(CMarker* pmkr, const char* pszString) :
        Str8(pszString)
{
    ASSERT( pmkr != NULL );
    m_pmkr = pmkr;
//  m_pmkr->IncrementNumRefs();
}


CMString::CMString(const CMString& mks) :  // copy constructor
        Str8((Str8&)mks)
{
    m_pmkr = mks.m_pmkr;
    ASSERT( m_pmkr );
//  m_pmkr->IncrementNumRefs();
}


CMString& CMString::operator=(const CMString& mks)  // assignment operator
{
    if (&mks == this)  // self-assignment
        return *this;

    this->Str8::operator=( (Str8)mks );
    ASSERT( m_pmkr != NULL );
//  m_pmkr->DecrementNumRefs();
    m_pmkr = mks.m_pmkr;
//  m_pmkr->IncrementNumRefs();
    return *this;
}

CMString& CMString::operator=( const char* psz )  // assignment operator from char* or Str8, copy string part, leave marker as is
{
    this->Str8::operator=( psz );
    ASSERT( m_pmkr != NULL );
    return *this;
}

CMString::~CMString()
{
    ASSERT( m_pmkr != NULL );
//  m_pmkr->DecrementNumRefs();
}

void CMString::SetMarker( CMarker* pmkr ) // Set field marker to a different value
{
    ASSERT( m_pmkr != NULL );
    ASSERT( pmkr != NULL );
//  m_pmkr->DecrementNumRefs();
    m_pmkr = pmkr;
//  m_pmkr->IncrementNumRefs();
}
    
void CMString::SetContent( const char* psz ) // Set field content to a different value
{
    this->Str8::operator=( psz ); // Use Str8 equal
}

BOOL CMString::bIsEmptyOrWhite() // Return true if empty or all whitespace
{
    for ( const char* psz = *this; *psz; psz++ )
        if ( !plng()->bIsWhite( psz ) )
            return FALSE;
    return TRUE;        
}

void CMString::Trim() // Trim trailing spaces and nl's
{
    Str8::TrimRight();
}

void CMString::TrimBothEnds() // Trim leading and trailing spaces and nl's
{
    Str8::Trim();
}

void CMString::Insert( UINT nChar, int iChar, int iNum ) // Insert iNum instances of nChar at pos iChar
{   
    ASSERT( iChar >= 0 );
    ASSERT( iNum >= 0 );
    int iLen =  GetLength(); // Remember length
    if ( !bValidFldLen( iLen + iNum ) )
        return;
#ifdef _DEBUG
    ASSERT( iChar <= iLen ); // Don't allow pos out of string
#endif
    if ( iChar > iLen ) // Protect against overly high argument by appending at end
        iChar = iLen;
    Str8 sToInsert;
    char* pszBuf = sToInsert.GetBuffer(iNum);
    memset(pszBuf, (char)nChar, iNum);
    sToInsert.ReleaseBuffer(iNum);
    Str8::Insert(iChar, sToInsert);
}

void CMString::Insert( const char* pszInsert, int iChar ) // Insert string pszInsert at pos iChar
{
    ASSERT(pszInsert);
    ASSERT(iChar >= 0);
    int iLen = GetLength(); // Remember current length
    int iNum = strlen(pszInsert); // Get number of chars to insert
    if (!bValidFldLen(iLen + iNum))
        return;
    ASSERT(iChar <= iLen); // Don't allow pos out of string
    Str8::Insert(iChar, pszInsert);
}

void CMString::Delete( int iChar, int iNum ) // Delete iNum chars at pos iChar
{
    ASSERT( iChar >= 0 );
    ASSERT( iNum >= 0 );
    int iLen =  GetLength(); // Remember length
    ASSERT( iChar <= iLen ); // Don't allow pos out of string
    if ( iChar + iNum > iLen ) // If delete more than end of string, delete only to end
        iNum = iLen - iChar;
    Str8::Delete(iChar, iNum);
}

void CMString::DeleteAll( const char cRemove ) // Delete all occurrences of cRemove
{
    int iLen = GetLength();
    char* psz = GetBuffer(iLen);
    int iWritePos = 0;
    for (int iReadPos = 0; iReadPos < iLen; ++iReadPos)
        if (psz[iReadPos] != cRemove)
            psz[iWritePos++] = psz[iReadPos];
    ReleaseBuffer(iWritePos);
}

void CMString::Extend( int iNewLen ) // Extend as necessary with spaces to be at least length iNewLen
{
    ASSERT( bValidFldLen( iNewLen ) );
    int iLen = GetLength();
    if ( iLen > 0 && *psz( iLen - 1 ) == '\n' ) // If line ends with nl, extend before it
        iLen--;
    if ( iNewLen > iLen ) // If extend pos beyond end, insert spaces to reach it
        Insert( ' ', iLen, iNewLen - iLen );
}

void CMString::Overlay(const char* pszInsert, int iChar, int iPadLen)   // Overlay pszInsert starting at iChar, filling out to iPadLen positions with spaces
{
    ASSERT(pszInsert);
    ASSERT(iChar >= 0);
    ASSERT(bValidFldLen(iPadLen));

    // Make sure the buffer is big enough for either the insert or the padding
    int iLenInsert = strlen(pszInsert);
    Extend(iChar + __max(iPadLen, iLenInsert));
    char* psz = GetBuffer(GetLength()) + iChar;

    // 1. Copy the entire insert string
    for (int i = 0; i < iLenInsert; i++)
        psz[i] = pszInsert[i];

    // 2. Pad with spaces if needed
    for (int i = iLenInsert; i < iPadLen; i++)
        psz[i] = ' ';

    ReleaseBuffer(GetLength());
}

void CMString::Overlay( const char cInsert, int iChar ) // Overlay cInsert at iChar
{
    ASSERT( cInsert );
    ASSERT( iChar >= 0 );
    Str8::SetAt(iChar, cInsert);
}

void CMString::OverlayAll( const char cRemove, const char cInsert, int iChar ) // Overlay all occurrences of cRemove with cInsert, starting at iChar
{
    ASSERT( cRemove );
    ASSERT( cInsert );
    int iLen = GetLength();
    char* psz = GetBuffer(iLen);
    for (int i = iChar; i < iLen; i++)
    {
        if (psz[i] == cRemove)
        {
            psz[i] = cInsert;
        }
    }
    ReleaseBuffer(iLen);
}

BOOL CMString::bVerify(const char* pszInsert, int iChar, int iLen) // Verify pszInsert starting at iChar
{
    ASSERT(pszInsert);
    ASSERT(iChar >= 0);
    ASSERT(bValidFldLen(iLen));

    int iCurrentLen = GetLength();
    if (iChar + iLen > iCurrentLen)
        return FALSE;  // Not enough chars at position
    for (int i = 0; i < iLen; i++)
    {
        char cInsert = pszInsert[i];
        char cExisting = GetChar(iChar + i);
        if (cInsert == '\0')  // End of insert string before iLen
        {
            for (int j = i; j < iLen; j++)
            {
                if (GetChar(iChar + j) != ' ')  // Rest must be spaces
                    return FALSE;
            }
            return TRUE;
        }
        if (cInsert != cExisting)
            return FALSE;
    }
    return TRUE;  // All matched
}

void CMString::AddSuffHyphSpaces( const char* pszMorphBreakChars, char cForceStart, char cForceEnd ) // Add a space before each suffix hyphen that doesn't have space
{
	int i = 1;
    for ( ; i < GetLength() - 1; i ++ ) // Insert space before each internal suffix hyphen (starting i at 1 avoids seeing initial hyphen)
        {                            
        if ( GetChar( i - 1 ) != ' ' // If not space here (relies on i starting at 1, not 0)
                && strchr( pszMorphBreakChars, GetChar( i ) ) // And this is hyphen
                && GetChar( i + 1 ) != ' ' // And next is not space (relies on i ending at length - 1)
                && GetChar( i + 1 ) != cForceStart ) // And next is not start of forced gloss on prefix
            Insert( ' ', i++ ); // Insert space and move past place
        }   
    for ( i = 1; i < GetLength() - 1; i ++ ) // If prefix hyphen is after forced gloss, move it to before forced gloss
        {
        if ( strchr( pszMorphBreakChars, GetChar( i ) ) // If this is hyphen
                && GetChar( i - 1 ) == cForceEnd ) // And previous is forced gloss end, move hyphen back
            {
            char c = GetChar( i ); // Save the hyphen
            Delete( i-- ); // Delete the hyphen and move back to account for deletion
            for ( int iFG = i; iFG > 0; iFG-- ) // Find the start of the forced gloss
                {
                if ( GetChar( iFG ) == cForceStart ) // When found, insert the hyphen
                    {
                    if ( !strchr( pszMorphBreakChars, GetChar( iFG - 1 ) ) ) // If not already a hyphen there (can come from automatically added hyphen)
                        Insert( c, iFG ); // Insert the hyphen
                    break; // Don't look any further
                    }
                if ( GetChar( iFG ) == ' ' ) // If space found, it is a user error, so don't look any further
                    break; 
                }
            }
        }   
}
