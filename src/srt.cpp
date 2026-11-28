// srt.cpp  Implementation of Shoebox sort orders (1995-04-03)

#include "stdafx.h"
#include "toolbox.h"
#include "lng.h"
#include "obstream.h"  // classes Object_istream, Object_ostream
#include "project.h"
#include "patch.h"  // Shw_bMatchWhiteSpaceAt, Shw_bAtWhiteSpace
#include "not.h"  // new CNote
#include "shwnotes.h"  // IDN_srt_...
#include "tools.h"  // pszNextLineBrk
#include "srt.h"

#include "srt_d.h"
#include "set_d.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

// Standard DLL call to make sort key
typedef void WINAPI MakeSortKeyFunction(char* pchInput, char* pchOutput);

// **************************************************************************

const Order CMCharOrder::ordSecOnly = 0;
const Order CMCharOrder::ordFirstOrdered = 1;
const Order CMCharOrder::ordLastOrdered = 65533;
const Order CMCharOrder::ordIgnore = 65534;
const Order CMCharOrder::ordUndefined = 65535;

// Multichar type classifications  (NOTE: Do not use bit value 0x10)
const MCharType CMCharOrder::mchtypPri =                    0x01;
const MCharType CMCharOrder::mchtypUndefined =              0x02;
const MCharType CMCharOrder::mchtypSecOnly =                0x04;
const MCharType CMCharOrder::mchtypIgnore =                 0x08;

// Match settings: bitwise union of multichar types to be included
const MatchSetting CMCharOrder::matsetSamePrimaryGroup =    0x03;
const MatchSetting CMCharOrder::matsetExactDisregardCase =  0x17;
const MatchSetting CMCharOrder::matsetExact =               0x07;
const MatchSetting CMCharOrder::matsetEvenIgnore =          0x0F;


CMCharOrder::CMCharOrder()
{
    m_ordPri = ordUndefined;
    m_ordSecDisregardingCase = ordUndefined;
    m_ordSec = ordUndefined;
    m_mchtyp = mchtypUndefined;
}

void CMCharOrder::Set(Order ordPri, Order ordSec, MCharType mchtyp)
{
    m_ordPri = ordPri;
    m_ordSecDisregardingCase = ordSec;
    m_ordSec = ordSec;
    m_mchtyp = mchtyp;
    ASSERT( m_mchtyp != mchtypUndefined );
}

void CMCharOrder::SetOrderDisregardingCase(Order ord)
{
    m_ordSecDisregardingCase = ord;
}

void CMCharOrder::ClearOrderDisregardingCase()
{
    m_ordSecDisregardingCase = m_ordSec;
}


Order CMCharOrder::ord(MatchSetting matset) const
{
    Order ord;
    switch ( matset )
        {
        case matsetSamePrimaryGroup:
        case 0x01:  // 1997-12-18 MRP
            ord = ordPri();
            break;
            
        case matsetExactDisregardCase:
        case 0x15:  // 1997-12-18 MRP
            ord = ordSecDisregardingCase();
            break;
            
        case matsetExact:
        case 0x05:  // 1997-12-18 MRP
        case matsetEvenIgnore:
        case 0x0D:  // 1997-12-18 MRP
            ord = ordSec();
            break;

        default:
            ASSERT( FALSE );            
        }

    return ord;
}


#ifdef _DEBUG
void CMCharOrder::AssertValid() const
{
    ASSERT( m_ordPri <= m_ordSec );
    if ( bPrimary() )
        {
        ASSERT( ordSecOnly < m_ordPri );
        ASSERT( m_ordPri < ordIgnore );
        ASSERT( m_ordSec < ordIgnore );
        }
    else if ( bSecondaryOnly() )
        {
        ASSERT( (m_ordPri == ordSecOnly) );
        ASSERT( ordSecOnly < m_ordSec );
        ASSERT( m_ordSec < ordIgnore );
        }
    else if ( bIgnore() )
        {
        ASSERT( m_ordPri == ordIgnore );
        ASSERT( m_ordSecDisregardingCase == ordIgnore );
        ASSERT( m_ordSec == ordIgnore );
        }
    else if ( bUndefined() )
        {
        ASSERT( m_ordPri == ordUndefined );
        ASSERT( m_ordSecDisregardingCase == ordUndefined );
        ASSERT( m_ordSec == ordUndefined );
        }
    else
        ASSERT( FALSE );
}
#endif  // _DEBUG


// --------------------------------------------------------------------------

static const unsigned short s_maxnumMCharOrders = 10000; // 1.4zbf Raise limit of sort order lines from 2500 to 10,000

const UIdMChar CSortOrder::uchMultiKeySeparator = 0xFFFF; // TLB 4/3/2000: Separator is no longer 1; This will make India happy.

#ifndef _MAC
// Turn off optimization because DLL calls in 32 bit change the stack pointer
#pragma optimize( "", off )
#endif
CSortOrder::CSortOrder(const char* pszName, const char* pszDescription,
        const char* pszPrimaryOrder, const char* pszSecondariesPreceding, const char* pszSecondariesFollowing,
        const char* pszIgnore, BOOL bSecAfterBase,
        const char* pszPathCCT, const char* pszPathDLL,
        BOOL bSkipSeq,
        CSortOrderSet* psrtsetMyOwner) : CSetEl(pszName)
{
    ASSERT( pszDescription );
    m_sDescription = pszDescription;
    ASSERT( pszPrimaryOrder );
    m_sPrimaryOrder = pszPrimaryOrder;
    ASSERT(pszSecondariesPreceding);
    m_sSecondariesPreceding = pszSecondariesPreceding;
    ASSERT(pszSecondariesFollowing);
    m_sSecondariesFollowing = pszSecondariesFollowing;
    ASSERT( pszIgnore );
    m_sIgnore = pszIgnore;
    m_bSecAfterBase = bSecAfterBase;
#if UseCct
    ASSERT( pszPathCCT );
    m_sPathCCT = pszPathCCT;
#endif
    ASSERT( pszPathDLL );
    m_sPathDLL = pszPathDLL;
    m_bSkipSeq = bSkipSeq;
    m_hDLL = NULL;
    m_pfMakeSortKey = NULL;
#if UseCct
    if ( !m_sPathCCT.IsEmpty() )
        m_cct.bLoadFromFile(m_sPathCCT);
    else 
#endif
		if ( !m_sPathDLL.IsEmpty() )
        {
#ifndef _MAC
        m_hDLL = LoadLibrary( swUTF16( m_sPathDLL  ) ); // 1.4quc Upgrade LoadLibrary for Unicode build
        // Note: If the DLL function isn't available, Shoebox falls back to ordinary sorting.
        if ( m_hDLL < (HINSTANCE)HINSTANCE_ERROR ) // Cast avoids compiler error in 32 bit
            {
            m_hDLL = NULL;
            }
        if ( m_hDLL )
            {
            m_pfMakeSortKey = (void *)GetProcAddress(m_hDLL, "MAKE_SORT_KEY");
			}
#endif // _MAC
        }
    
    m_bSpacePrecedes = TRUE;    // RNE 1996-03-01  Space always precedes.
    
    m_pmchord = new CMCharOrder[s_maxnumMCharOrders];
    m_ordPri = CMCharOrder::ordSecOnly;  // not yet in any group
    m_ordSecNext = CMCharOrder::ordFirstOrdered;
    m_ordFirstPri = m_ordSecNext;

    m_psrtsetMyOwner = psrtsetMyOwner;
    ASSERT( m_psrtsetMyOwner );
}
#ifndef _MAC
// Restore frame pointer optimization, temporarily off for DLL calls
#pragma optimize( "", on )
#endif


CSortOrder::~CSortOrder()
{
    delete [] m_pmchord;
    if ( m_hDLL )
        FreeLibrary( m_hDLL );
}


CSet* CSortOrder::psetMyOwner()
{
    return m_psrtsetMyOwner;
}


static const char* psz_srt = "srt";
static const char* psz_desc = "desc";
static const char* psz_order = "order"; // RNE to be eliminated.
static const char* psz_primary = "primary";
static const char* psz_SecPreceding = "SecPreceding";
static const char* psz_SecFollowing = "SecFollowing";
static const char* psz_SecAfterBase = "SecAfterBase";
static const char* psz_ignore = "ignore";
static const char* psz_cct = "cct";
static const char* psz_dll = "sortingDLL"; // TLB 04/10/2000 - This used to be just "dll" but it didn't work prior to v5.0
// static const char* psz_SkipSeq = "SkipSeq"; // TLB 04/10/2000 - Didn't work prior to v5.0. Now replaced by the following:
static const char* psz_DLLOutputsCharCodes = "DLL_OutputsLangEncCharCodes"; // TLB 04/10/2000

#ifdef lngWritefstream // 1.6.4ab 
void CSortOrder::WriteProperties(Object_ofstream& obs) const
#else
void CSortOrder::WriteProperties(Object_ostream& obs) const
#endif
{
    obs.WriteBeginMarker(psz_srt, sName());
    
    obs.WriteString(psz_desc, m_sDescription);
    obs.WriteString(psz_primary, m_sPrimaryOrder);
    obs.WriteString(psz_SecPreceding, m_sSecondariesPreceding);
    obs.WriteString(psz_SecFollowing, m_sSecondariesFollowing);
    obs.WriteString(psz_ignore, m_sIgnore);
    obs.WriteBool(psz_SecAfterBase, m_bSecAfterBase);
#if UseCct
    obs.WriteProjectPath(psz_cct, m_sPathCCT);
#endif
    if (sGetDirPath(m_sPathDLL).IsEmpty())
        obs.WriteString(psz_dll, m_sPathDLL); // Preserve the system lookup for bare DLL names.
    else
        obs.WriteProjectPath(psz_dll, m_sPathDLL);
//    obs.WriteBool(psz_SkipSeq, m_bSkipSeq ); // TLB 04/10/2000 - Replaced with the following:
    obs.WriteBool(psz_DLLOutputsCharCodes, (!m_sPathDLL.IsEmpty() && !m_bSkipSeq)); // When a DLL is present, the default action is to skip the char-code sequencing 
    obs.WriteEndMarker(psz_srt);
    obs.WriteNewline();
}

BOOL CSortOrder::s_bReadProperties(Object_istream& obs,
        CSortOrderSet* psrtsetMyOwner, CSortOrder** ppsrt)
{
    Str8 sName;
    if ( !obs.bReadBeginMarker(psz_srt, sName) )
        return FALSE;

    // Default properties
    Str8 sDescription;       
    Str8 sPrimary;
    Str8 sSecPreceding;
    Str8 sSecFollowing;
    Str8 sIgnore;
    BOOL bSecAfterBase = FALSE;
    Str8 sPathCCT;
    Str8 sPathDLL;   
    BOOL bDLLOutputsCharCodes = FALSE;
    
    while ( !obs.bAtEnd() )
        {
        if ( obs.bReadString(psz_desc, sDescription) )
            ;
        else if ( obs.bReadString(psz_SecPreceding, sSecPreceding) )
            ;
        else if ( obs.bReadString(psz_primary, sPrimary) )
            ;
        else if ( obs.bReadString(psz_SecFollowing, sSecFollowing) )
            ;
        else if ( obs.bReadString(psz_ignore, sIgnore) )
            ;
        else if ( obs.bReadBool(psz_SecAfterBase, bSecAfterBase) )
            ;
        else if ( obs.bReadProjectPath(psz_cct, sPathCCT) )
            ;
        else if ( obs.bReadString(psz_dll, sPathDLL) )
            ;
//        else if ( obs.bReadBool(psz_SkipSeq, bSkipSeq) ) // TLB 04/10/2000 - Replaced with the following:
        else if ( obs.bReadBool(psz_DLLOutputsCharCodes, bDLLOutputsCharCodes) )
            ;
        else if ( obs.bEnd(psz_srt) )
            break;
        }
    
    if ( !sPathCCT.IsEmpty() && sGetDirPath(sPathCCT).IsEmpty() )
        {
        // Assume that the Change table file is in the settings directory
        const Str8& sSettingsDirPath = psrtsetMyOwner->
            plngMyOwner()->plngsetMyOwner()->sSettingsDirPath();
        sPathCCT = sPath(sSettingsDirPath, sPathCCT);
        }
    if (!sGetDirPath(sPathDLL).IsEmpty() && Shw_pProject())
        sPathDLL = Shw_pProject()->sResolvedPath(sPathDLL, FALSE);

    // When sorting or comparing, skip the Language Encoding Character code look-up if we have a Sorting
    // DLL that outputs straight numeric sorting codes rather than language encoding character codes.
    BOOL bSkipSeq = (!sPathDLL.IsEmpty() && !bDLLOutputsCharCodes);

    CNote* pnot = NULL;
    if ( !s_bConstruct(sName, sDescription,
            sPrimary, sSecPreceding, sSecFollowing, sIgnore, bSecAfterBase,
            sPathCCT, sPathDLL, bSkipSeq, psrtsetMyOwner, ppsrt, &pnot) )
        {
        ASSERT( pnot );
#ifdef JUNK // 1.5.0ft 
        pnot->Add(IDN_srt_InSortOrder1, sName);
        CLangEnc* plng = psrtsetMyOwner->plngMyOwner();
        pnot->Add(IDN_lng_InLangEnc1, plng->sName());
        CLangEncProxy* plrx = plng->plrxMyProxy();
        if ( !plrx->plngsetMyOwner()->bReadFromString() )
            pnot->Add(IDN_lng_InLangEncFile1, plrx->sPath());
        obs.notlst().Add(pnot);
#endif // JUNK // 1.5.0ft 
        }
        
    return TRUE;  // that the properties were read
}

#ifdef lngWritefstream // 1.6.4ab 
void CSortOrder::WriteRef(Object_ofstream& obs, const char* pszQualifier) const
#else
void CSortOrder::WriteRef(Object_ostream& obs, const char* pszQualifier) const
#endif
{
    obs.WriteString(psz_srt, pszQualifier, sName());
}


const char* CSortOrder::s_pszDescription =
    "Sort order with default properties.";
const char* CSortOrder::s_pszPrimary =
    "A a\n" "B b\n" "C c\n" "D d\n" "E e\n" "F f\n" "G g\n" "H h\n"
    "I i\n" "J j\n" "K k\n" "L l\n" "M m\n" "N n\n" "O o\n" "P p\n"
    "Q q\n" "R r\n" "S s\n" "T t\n" "U u\n" "V v\n" "W w\n" "X x\n"
    "Y y\n" "Z z\n" "0\n" "1\n" "2\n" "3\n" "4\n" "5\n" "6\n" "7\n" "8\n" "9\n" "_\n";
const char* CSortOrder::s_pszSecondaryBefore =  "`";
const char* CSortOrder::s_pszSecondaryAfter = "'";
const char* CSortOrder::s_pszIgnore = "-";
const BOOL CSortOrder::s_bSecondariesFollowBase = FALSE;
const BOOL CSortOrder::s_bSpaceFirst = TRUE;

void CSortOrder::s_ConstructDefault(const char* pszName,
        CSortOrderSet* psrtsetMyOwner, CSortOrder** ppsrt)
{

    CNote* pnot = NULL;
    BOOL bConstructed = s_bConstruct(pszName, s_pszDescription,
        s_pszPrimary, s_pszSecondaryBefore, s_pszSecondaryAfter, s_pszIgnore,
        s_bSecondariesFollowBase, "", "", FALSE, psrtsetMyOwner, ppsrt, &pnot);
    ASSERT( bConstructed );
    // NOTE NOTE NOTE: Should we return success in case name is invalid???
}

BOOL CSortOrder::s_bConstruct(const char* pszName, const char* pszDescription,
        const char* pszPrimaryOrder, 
        const char* pszSecondariesPreceding, const char* pszSecondariesFollowing,
        const char* pszIgnore, BOOL bSecAfterBase,
        const char* pszPathCCT, const char* pszPathDLL,
        BOOL bSkipSeq,
        CSortOrderSet* psrtsetMyOwner,
        CSortOrder** ppsrt, CNote** ppnot)
{
    if ( !psrtsetMyOwner->bValidNewName(pszName, ppnot) )
        {
        (*ppnot)->SetStringContainingReferent(pszName);
        return FALSE;
        }

    CSortOrder* psrt = new CSortOrder(pszName, pszDescription,
        pszPrimaryOrder, pszSecondariesPreceding, pszSecondariesFollowing,
        pszIgnore, bSecAfterBase, pszPathCCT, pszPathDLL, bSkipSeq, psrtsetMyOwner);
    // Even if the sort order's properties have problems,
    // we return as much of the order as possible...
    *ppsrt = psrt;
    // ...and a note telling where the problem was detected.
    // Notice: The order in which these functions are called matters: Secondaries before, then
    // primary groups, then 2ndary after, then ignores.
    BOOL bNoProblems = psrt->bSetSecondaries(pszSecondariesPreceding, ppnot) &&
        psrt->bSetPrimaries(pszPrimaryOrder, ppnot) &&
        psrt->bSetSecondaries(pszSecondariesFollowing, ppnot) &&
        psrt->bSetIgnore(pszIgnore, ppnot);
    return bNoProblems; 
}


// RNE 1996-03-04  NOTE NOTE.  Secondaries before/after are no longer included with
// the primaries.
BOOL CSortOrder::bSetPrimaries(const char* pszPrimaryOrder, CNote** ppnot)
{
    m_ordFirstPri = m_ordSecNext;  // 1996-06-10 MRP
    if ( m_bSpacePrecedes )
        {
        CMChar* pmchSpace = plngMyOwner()->pmchSearch_AddIfNew(" ");
        BOOL bSetSpace = bSetPri(pmchSpace, TRUE);
        ASSERT( bSetSpace );
        }

    const char* psz = pszPrimaryOrder;
    ASSERT( psz );
    (void) Shw_bMatchWhiteSpaceAt(&psz);
    while ( *psz )
        {
        BOOL bSet = bSetPri(&psz, ppnot);
        if ( !bSet )
            {
            (*ppnot)->SetStringContainingReferent(pszPrimaryOrder);
            return FALSE;
            }
        }
        
    return TRUE;
}


BOOL CSortOrder::bSetSecondaries(const char* pszSecondaries, CNote** ppnot)
{
    const char* psz = pszSecondaries;
    ASSERT( psz );
    (void) Shw_bMatchWhiteSpaceAt(&psz);
    while ( *psz )
        {
        BOOL bSet = bSetSec(&psz, ppnot);
        if ( !bSet )
            {
            (*ppnot)->SetStringContainingReferent(pszSecondaries);
            return FALSE;
            }
        }
    return TRUE;
}


BOOL CSortOrder::bSetPri(const char** ppszPri, CNote** ppnot)
{
    BOOL bFirstInGroup = TRUE;
    CLangEnc* plng = plngMyOwner();
    
    ASSERT( ppszPri );
    const char* psz = *ppszPri;
    (void) Shw_bMatchWhiteSpaceAt(&psz);
    ASSERT( psz );
    ASSERT( !Shw_bAtWhiteSpace(psz) );
    while ( *psz && !Shw_bMatchNewLineAt(&psz) )
        {
        const char* pszChar = psz;
        CMChar* pmch = NULL;
        if ( !plng->bMatchMetaMCharAt_AddIfNew(&psz, &pmch, ppnot) )
            return FALSE;
            
        if ( !bSetPri(pmch, bFirstInGroup) )
            {
            ASSERT( pszChar < psz );
            Length lenChar = psz - pszChar;
            Str8 sChar(pszChar, lenChar);
            *ppnot = new CNote(_("This character is already defined:"), sChar, pszChar, lenChar); // 1.5.0ft 
            return FALSE;
            }
            
        bFirstInGroup = FALSE;
        (void) Shw_bMatchWhiteSpaceNoNLAt(&psz);
        }

    *ppszPri = psz;
    return TRUE;    
}
                
// RNE 1996-03-04  Secondaries no longer in braces.
BOOL CSortOrder::bSetSec(const char** ppszSec, CNote** ppnot)
{
    CLangEnc* plng = plngMyOwner();
    
    ASSERT( ppszSec );
    const char* psz = *ppszSec;
    ASSERT( psz );
    while (*psz)
        {
        const char* pszChar = psz;
        CMChar* pmch = NULL;
        if ( !plng->bMatchMetaMCharAt_AddIfNew(&psz, &pmch, ppnot) )
            return FALSE;
            
        if ( !bSetSec(pmch) )
            {
            ASSERT( pszChar < psz );
            Length lenChar = psz - pszChar;
            Str8 sChar(pszChar, lenChar);
            *ppnot = new CNote(_("This character is already defined:"), sChar, pszChar, lenChar); // 1.5.0ft 
            return FALSE;
            }
        // Eliminate trailing spaces or newlines.
        (void) Shw_bMatchWhiteSpaceAt(&psz);        
        }
        
    *ppszSec = psz;
    return TRUE;    
}


BOOL CSortOrder::bSetIgnore(const char* pszIgnore, CNote** ppnot)
{
    CLangEnc* plng = plngMyOwner();
    const char* psz = pszIgnore;
    ASSERT( psz );
    (void) Shw_bMatchWhiteSpaceAt(&psz);
    while ( *psz )
        {
        const char* pszChar = psz;
        CMChar* pmch = NULL;
        if ( !plng->bMatchMetaMCharAt_AddIfNew(&psz, &pmch, ppnot) )
            return FALSE;
            
        if ( !bSetIgnore(pmch) )
            {
            ASSERT( pszChar < psz );
            Length lenChar = psz - pszChar;
            Str8 sChar(pszChar, lenChar);
            // This character is already defined: %1
            *ppnot = new CNote(_("This character is already defined:"), sChar, // 1.5.0ft 
                pszChar, lenChar, pszIgnore);
            return FALSE;
            }

        // We are permitting arbitrary white space between characters.
        (void) Shw_bMatchWhiteSpaceAt(&psz);
        }
        
    return TRUE;
}


BOOL CSortOrder::bSetPri(CMChar* pmch, BOOL bFirstInGroup)
{
    if ( bFirstInGroup )
        m_ordPri = m_ordSecNext;
    ASSERT( m_ordPri != CMCharOrder::ordSecOnly );
    return bSetMCharOrder(pmch, m_ordPri, m_ordSecNext++,
        CMCharOrder::mchtypPri);
}


BOOL CSortOrder::bSetSec(CMChar* pmch)
{
    m_ordPri = CMCharOrder::ordSecOnly;
    return bSetMCharOrder(pmch, m_ordPri, m_ordSecNext++,
        CMCharOrder::mchtypSecOnly);
}


BOOL CSortOrder::bSetIgnore(CMChar* pmch)
{
    return bSetMCharOrder(pmch, CMCharOrder::ordIgnore, CMCharOrder::ordIgnore,
        CMCharOrder::mchtypIgnore);
}


BOOL CSortOrder::bSetMCharOrder(CMChar* pmch,
        Order ordPri, Order ordSec, MCharType mchtyp)
{
    UIdMChar uch = pmch->uch();
    if ( !m_pmchord[uch].bUndefined() )
        return FALSE;
        
    m_pmchord[uch].Set(ordPri, ordSec, mchtyp);
    return TRUE;
}


void CSortOrder::SetOrderDisregardingCase()
{
    ClearOrderDisregardingCase();
    
    CLangEnc* plng = plngMyOwner();
    CMChar* pmch = plng->pmchFirst();
    for ( ; pmch; pmch = plng->pmchNext(pmch) )
        if ( !pmch->bUpperCase() )
            {
            CMChar* pmchUpperCase = pmch->pmchOtherCase();
            if ( pmchUpperCase )
                // Set the "order disregarding case" of characters having
                // an upper case counterpart in this language encoding.
                SetOrderDisregardingCase(pmchUpperCase, pmch);
            }
}

void CSortOrder::ClearOrderDisregardingCase()
{
    CMCharOrder* pmchord = m_pmchord;
    int i = 0;
    for ( ; i != s_maxnumMCharOrders; i += 1, pmchord += 1 )
        pmchord->ClearOrderDisregardingCase();
    
}

void CSortOrder::SetOrderDisregardingCase(CMChar* pmchUpperCase, CMChar* pmchLowerCase)
{
    ASSERT( pmchUpperCase );
    ASSERT( pmchUpperCase->bUpperCase() );
    ASSERT( pmchLowerCase );
    ASSERT( !pmchLowerCase->bUpperCase() );
    ASSERT( pmchLowerCase->pmchOtherCase() == pmchUpperCase );

    // Case associates share the upper's order. 
    Order ordUpper = pmchord(pmchUpperCase)->ordSecDisregardingCase();
    CMCharOrder* pmchordLower = m_pmchord + pmchLowerCase->uch();
    pmchordLower->SetOrderDisregardingCase(ordUpper);
}


CLangEnc* CSortOrder::plngMyOwner() const
{
    return m_psrtsetMyOwner->plngMyOwner();
}

BOOL CSortOrder::bUnicodeSequenceSort() const // True if this sort order is a Unicode sequence sort 1.4eg
{ 
	return ( m_sPrimaryOrder.IsEmpty() && plngMyOwner()->bUnicodeLang() ); 
}

LONG lNextCharUTF8ToUTF32( const char* &psz ) // Get next Unicode char, move char pointer 1.4eh
	{
	int iNumBytes = 0;
	LONG lUTF32 = lUTF8ToUTF32( psz, iNumBytes ); // Get next Unicode char
	psz += iNumBytes;
	return lUTF32;
	}

BOOL CSortOrder::bUnicodeSequenceIgnore( LONG lUTF32Char ) const
	{
	const char* pszIg = pszIgnore();
	while ( *pszIg ) // Compare current char to each ignore char
		{
		if ( *pszIg == ' ' ) // Treat space as separator between ignore chars
			pszIg++;
		else
			{
			LONG lIg = lNextCharUTF8ToUTF32( pszIg );
			if ( lUTF32Char == lIg ) // If ignore char matches, return true
				return TRUE;
			}
		}
	return FALSE; // If no ignore char matches, return flase
	}

void CSortOrder::SkipUnicodeSequenceIgnores( const char* &psz ) const
	{
	while ( *psz ) // While there is a char to check for ignore char
		{
		int iNumBytes = 0;
		LONG lUTF32Char = lUTF8ToUTF32( psz, iNumBytes ); // Get next Unicode main char
		if ( bUnicodeSequenceIgnore( lUTF32Char ) ) // If it is an ignore char, move past it
			psz += iNumBytes;
		else
			return; // If no more ignore chars, return
		}
	}

int CSortOrder::iCompareUnicodeSequence( const char* pszA, const char* pszB, MatchSetting matset, BOOL bMatchWhole ) const // Compare using Unicode sequence sort 1.4eg // 1.4gy Add match criteria for Search
	{
	if ( !bUnicodeSequenceSort() )
		return 0;
	ASSERT( bUnicodeSequenceSort() );
	if ( !pszA && !pszB ) // If no field in either record, return equal
		return 0;
	if ( pszA && !pszB ) // If A but not B, A is larger
		return 1;
	if ( !pszA && pszB ) // If B but not A, B is larger
		return -1;
	while ( TRUE )
		{
		if ( matset != CMCharOrder::matsetEvenIgnore ) // 1.4gy Make Unicode sequence compare obey ignore match settings
			{
			SkipUnicodeSequenceIgnores( pszA ); // 1.4eh Handle ignores in Unicode sequence sort
			SkipUnicodeSequenceIgnores( pszB );
			}
		if ( !*pszA && !*pszB ) // If both at end, compare is done, they are equal
			return 0;
		if ( *pszA && !*pszB ) // If not end of A, but end of B, A is larger
			return 1;
		if ( !*pszA && *pszB ) // If end of A, but not end of B, B is larger
			{
			if ( bMatchWhole ) // 1.4gy Make Unicode sequence compare obey match whole setting
				return -1;
			else
				return 0; // 1.4gy If not matching whole, and A at end, but B not at end, return match
			}
		LONG lUTF32A = lNextCharUTF8ToUTF32( pszA ); // Get next Unicode char from A
		LONG lUTF32B = lNextCharUTF8ToUTF32( pszB ); // Get next Unicode char from B
		if ( lUTF32A > lUTF32B ) // If A is larger, return plus
			return 1;
		if ( lUTF32A < lUTF32B ) // If B is larger, return minus
			return -1;
		}
	}

void CSortOrder::s_SortableSubString(const char** ppsz, Length* plen)
{
    ASSERT( ppsz );
    const char* psz = *ppsz;
    ASSERT( psz );
    // 1. Skip over leading white space;
    while ( *psz && Shw_bAtWhiteSpace(psz) )
        psz += 1;
    // 2. If there are any embedded newlines, then stop at the first;
    const char* pszEnd = pszNextLineBrk(psz);
    // 3. Back up over trailing white space.
    while ( psz < pszEnd && Shw_bAtWhiteSpace(pszEnd-1) )
        pszEnd -= 1;
        
    // The substring [psz,pszEnd) can be used to make a sort key.
    *ppsz = psz;
    ASSERT( plen );
    *plen = pszEnd - psz;
}

void CSortOrder::s_SortableSubString(Str8* ps)
{
    ASSERT( ps );
    Str8& s = *ps;
    if ( s.IsEmpty() )
        return;
        
    const char* pszString = s;
    const char* pszSubString = pszString;
    Length len = s.GetLength();
    s_SortableSubString(&pszSubString, &len);
    if ( pszSubString != pszString || len != (Length)s.GetLength() )
        s = s.Mid(pszSubString - pszString, len);
}

static void ReverseSortKey(UIdMChar* puchSortKey, Length lenSortKey)
{
    UIdMChar* puchBegin = puchSortKey;
    UIdMChar* puchEnd = puchBegin + lenSortKey - 1;
    while ( puchBegin < puchEnd )
        {
        // Exchange char ids at beginning and end of the key
        UIdMChar uchBegin = *puchBegin;
        *puchBegin++ = *puchEnd;
        *puchEnd-- = uchBegin;
        }
}

BOOL CSortOrder::bChangeKeyCCT(const char* pchKey, int lenKey,
        char** ppszChangedKey, int* plenChangedKey)
{
#if UseCct
    ASSERT( m_cct.bLoaded() );  // Call only if there is a change table
#endif
    ASSERT( pchKey );
    ASSERT( lenKey >= 0 );
    ASSERT( ppszChangedKey );
    ASSERT( plenChangedKey );

    static const int s_maxlenChangedKeyCCT = 5000; // 1.4vnh Fix bug of CC buffer not big enough for Thai sort
    static char s_pchChangedKeyCCT[s_maxlenChangedKeyCCT + 1];
    int lenChangedKeyCCT = s_maxlenChangedKeyCCT;
    BOOL bTruncated = FALSE;
#if UseCct
    if ( !m_cct.bMakeChanges(pchKey, lenKey, s_pchChangedKeyCCT, &lenChangedKeyCCT) )
        {
        bTruncated = TRUE;
        // lenChangedKey = 0;  // 1998-03-23 MRP
        }

    ASSERT( 0 <= lenChangedKeyCCT );
    ASSERT( lenChangedKeyCCT <= s_maxlenChangedKeyCCT );
    s_pchChangedKeyCCT[lenChangedKeyCCT] = '\0';
#endif

	*ppszChangedKey = s_pchChangedKeyCCT;
    *plenChangedKey = lenChangedKeyCCT;
    return !bTruncated;
}

#ifndef _MAC
// Turn off optimization because DLL calls in 32 bit change the stack pointer
#pragma optimize( "", off )
#endif
BOOL CSortOrder::bChangeKeyDLL(const char* pchKey, int lenKey,
        char** ppszChangedKey, int* plenChangedKey)
{
    ASSERT( m_pfMakeSortKey );  // Call only if there is a DLL function
    ASSERT( m_hDLL );  // If the function is linked, the DLL should be loaded
    ASSERT( pchKey );
    ASSERT( lenKey >= 0 );
    ASSERT( ppszChangedKey );
    ASSERT( plenChangedKey );

    static const int s_maxlenBufferDLL = 255; 
    static char s_pszInputDLL[s_maxlenBufferDLL + 1];
    static char s_pszOutputDLL[s_maxlenBufferDLL + 1];

    // Copy the input so that the DLL can use the buffer for "scratch work".
    int lenInputDLL = lenKey;
    if ( lenInputDLL > s_maxlenBufferDLL )
        lenInputDLL = s_maxlenBufferDLL;
    memcpy(s_pszInputDLL, pchKey, lenInputDLL);
    s_pszInputDLL[lenInputDLL] = '\0';
    s_pszOutputDLL[0] = '\0';

    (*((MakeSortKeyFunction*)m_pfMakeSortKey))(s_pszInputDLL, s_pszOutputDLL);

    s_pszOutputDLL[s_maxlenBufferDLL] = '\0';
    int lenOutputDLL = strlen(s_pszOutputDLL);

    *ppszChangedKey = s_pszOutputDLL;
    *plenChangedKey = lenOutputDLL;
    return TRUE;
}
#ifndef _MAC
// Restore frame pointer optimization, temporarily off for DLL calls
#pragma optimize( "", on )
#endif

BOOL CSortOrder::bSortKey(const char* pszKey, UIdMChar** ppuchSortKeyBuf,
        Length maxlenSortKey, BOOL bSortFromEnd)
{
    ASSERT( pszKey );
    ASSERT( ppuchSortKeyBuf );
    UIdMChar* puch = *ppuchSortKeyBuf;
    ASSERT( puch );

    // 1. Determine the part of the string to be considered for sorting.
    // a. Skip leading and trailing spaces.
    // b. If there are any embedded newlines, then stop at the first.
    const char* pchKey = pszKey;
    Length lenKey = 0;
    s_SortableSubString(&pchKey, &lenKey);
    BOOL bTruncated = FALSE;  

    // 2. Make changes to the sort key (optional step for non-Roman scripts).
#if UseCct
    if ( m_cct.bLoaded() && lenKey != 0 )
        {
        char* pchChangedKey = NULL;
        int lenChangedKey = 0;
        if ( !bChangeKeyCCT(pchKey, lenKey, &pchChangedKey, &lenChangedKey) )
            bTruncated = TRUE;
        pchKey = pchChangedKey;
        ASSERT( pchKey );
        lenKey = lenChangedKey;
        }
    else 
#endif
		if ( m_pfMakeSortKey )
        {
        char* pchChangedKey = NULL;
        int lenChangedKey = 0;
        if ( !bChangeKeyDLL(pchKey, lenKey, &pchChangedKey, &lenChangedKey) )
            bTruncated = TRUE;
        pchKey = pchChangedKey;
        ASSERT( pchKey );
        lenKey = lenChangedKey;
        }

    if ( bSortFromEnd && bTruncated )
        {
        **ppuchSortKeyBuf = 0;  // Return empty sort key
        return FALSE;
        }

    // 3. Fill the sort key buffer
    CLangEnc* plng = plngMyOwner();
    const char* pch = pchKey;
    const char* pchEnd = pchKey + lenKey;
    Length lenSortKey = 0;
    while ( pch < pchEnd && lenSortKey != maxlenSortKey )
        {
        // Note: If the DLL function isn't available, Shoebox falls back to ordinary sorting.
        if ( m_pfMakeSortKey && m_bSkipSeq )
            {
            unsigned char ch = *pch; // avoid interpreting 0xFF as the key field separator (0xFFFF)
            *puch++ = ch;
            }
        else
            {
            const CMChar* pmch = NULL;
            (void) plng->bMChar(&pch, &pmch);
            *puch++ = pmch->uch();
            }
        ++lenSortKey;
        }

    if ( pch < pchEnd ) // If there's still more sort key that we couldn't fit into the output buffer
        bTruncated = TRUE;

    // 4. Reverse the sort key, if sorting from the end
    if ( bSortFromEnd )
        {
        if ( bTruncated )
            {
            **ppuchSortKeyBuf = 0;  // Return empty sort key
            return FALSE;
            }

        ReverseSortKey(*ppuchSortKeyBuf, lenSortKey);
        }
    
    *puch = 0;  // Null terminate the sort key
    *ppuchSortKeyBuf = puch;  // Return the sort key's end
    return !bTruncated;
}


BOOL CSortOrder::bAppendToSortKey(const char* pszKey, UIdMChar** ppuchSortKeyBuf,
        Length maxlenSortKey)
{
    ASSERT( ppuchSortKeyBuf );
    ASSERT( *ppuchSortKeyBuf );
    **ppuchSortKeyBuf = uchMultiKeySeparator;
    *ppuchSortKeyBuf += 1;

    return bSortKey(pszKey, ppuchSortKeyBuf, maxlenSortKey, FALSE);
}


int CSortOrder::iCompare(const UIdMChar* puchKeyA, const UIdMChar* puchKeyB,
                            MatchSetting matset, BOOL bMatchWhole) const
{
    // 2000-04-17 MRP: Rewrite to account for changes made to the sort key
    // using a DLL (optional step for non-Roman scripts).
    // Note: If the DLL function isn't available, Shoebox falls back to ordinary sorting.
    if ( m_pfMakeSortKey && m_bSkipSeq )
        {
        return iCompareSkipSeq(puchKeyA, puchKeyB, bMatchWhole);
        }

    // 1997-12-18 MRP: Emergency patch to provide compatibility with MDF-DOS
    // sorting of a reversed index (finderlist).
    BOOL bNotIncludeUndefined = ((matset & CMCharOrder::mchtypUndefined) == 0);

    MatchSetting matset1 = CMCharOrder::matsetSamePrimaryGroup;
    if ( bNotIncludeUndefined)
        matset1 = 0x01;  // 1997-12-18 MRP
    int i = iCompare1(puchKeyA, puchKeyB, matset1, bMatchWhole);
    if ( (i != 0 ) || (matset == matset1) )
        return i;

    matset1 = CMCharOrder::matsetExactDisregardCase;
    if ( bNotIncludeUndefined)
        matset1 = 0x15;  // 1997-12-18 MRP
    i = iCompare1(puchKeyA, puchKeyB, matset1, bMatchWhole);
    if ( (i != 0 ) || (matset == matset1) )
        return i;

    matset1 = CMCharOrder::matsetExact;
    if ( bNotIncludeUndefined)
        matset1 = 0x05;  // 1997-12-18 MRP
    i = iCompare1(puchKeyA, puchKeyB, matset1, bMatchWhole);
    if ( (i != 0 ) || (matset == matset1) )
        return i;

    matset1 = CMCharOrder::matsetEvenIgnore;
    if ( bNotIncludeUndefined)
        matset1 = 0x0D;  //1997-12-18 MRP
    i = iCompare1(puchKeyA, puchKeyB, matset1, bMatchWhole);
    return i;
}


int CSortOrder::iCompare1(const UIdMChar* puchKeyA, const UIdMChar* puchKeyB,
                            MatchSetting matset, BOOL bMatchWhole) const
{
    int i = 0;
    const UIdMChar* puchA = puchKeyA;
    const UIdMChar* puchB = puchKeyB;
    while ( !bIsTerminator(*puchA) && !bIsTerminator(*puchB) )
        if ( !bIncludeInMatch(*puchA, matset) )
            puchA += 1;
        else if ( !bIncludeInMatch(*puchB, matset) )
                puchB += 1;
        else
            {
            i = iCompareMChars(*puchA, *puchB, matset);
            if ( i != 0 )
                // A and B differ at this sort order element
                return i;
            puchA += 1;
            puchB += 1;
            }

    // 1995-08-09 MRP: Skip over any characters at the end
    // which are not to be included in the match (for Match Whole).
    while ( !bIsTerminator(*puchA) && !bIncludeInMatch(*puchA, matset) )
        puchA += 1;
    while ( !bIsTerminator(*puchB) && !bIncludeInMatch(*puchB, matset) )
        puchB += 1;

    // 1996-06-10 MRP: When secondaries are included in the match,
    // those ordered BEFORE unmarked primaries must be considered
    // less than a terminator (e.g. ba` < ba).
    BOOL bIncludingSecondary = ( matset != CMCharOrder::matsetSamePrimaryGroup );
    BOOL bAtEndA = bIsTerminator(*puchA);
    BOOL bAtEndB = bIsTerminator(*puchB);
    if ( bAtEndA && bAtEndB )
        i = 0;
    else if ( bAtEndA )
        {
        const CMCharOrder& mchordB = m_pmchord[*puchB];
        BOOL bSecondaryB = bIncludingSecondary && mchordB.bSecondaryOnly();
        if ( !bMatchWhole )
            {
            // Special logic for SEARCHING, not sorting
            // If secondaries follow primary base characters,
            // Then it must be included in a partial match search.
            if ( bSecondaryB && m_bSecAfterBase )
                i = ( m_ordFirstPri < mchordB.ordSec() ? -1 : 1 );  // A < B
            else
                i = 0;  // A and B equivalent for length of A, so A == B
                        // i.e. it is permitted for A to match a prefix of B            
            }
        else if ( bSecondaryB )
            i = ( m_ordFirstPri < mchordB.ordSec() ? -1 : 1 );  // A < B
        else
            i = -1;  // A < B
        }
    else if ( bAtEndB )
        {
        const CMCharOrder& mchordA = m_pmchord[*puchA];
        if ( bIncludingSecondary && mchordA.bSecondaryOnly() )
            i = ( mchordA.ordSec() < m_ordFirstPri ? -1 : 1 );  // A < B
        else
            i = 1;  // B < A
        }
    else
        ASSERT( FALSE );
        
    return i;
}

int CSortOrder::iCompareSkipSeq(const UIdMChar* puchKeyA, const UIdMChar* puchKeyB,
                               BOOL bMatchWhole) const
{
    const UIdMChar* puchA = puchKeyA;
    const UIdMChar* puchB = puchKeyB;
    while ( !bIsTerminator(*puchA) && !bIsTerminator(*puchB) )
        {
        int i = *puchA - *puchB;
        if ( i != 0 )
            // A and B differ at this sort order element
            return i;
        puchA += 1;
        puchB += 1;
        }

    // 1996-06-10 MRP: When secondaries are included in the match,
    // those ordered BEFORE unmarked primaries must be considered
    // less than a terminator (e.g. ba` < ba).
    BOOL bAtEndA = bIsTerminator(*puchA);
    BOOL bAtEndB = bIsTerminator(*puchB);
    if ( bAtEndA && bAtEndB )
        return 0;
    if ( bAtEndA )
        {
        if ( !bMatchWhole )
            return 0;  // A and B equivalent for length of A, so A == B
                    // i.e. it is permitted for A to match a prefix of B            
        else
            return -1;  // A < B
        }
    if ( bAtEndB )
        {
        return 1;  // B < A
        }

    ASSERT( FALSE );    
    return 0;
}


BOOL CSortOrder::bIncludeInMatch(const CMChar* pmch, MatchSetting matset) const
{
    return bIncludeInMatch(pmch->uch(), matset);
}


int CSortOrder::iCompareMChars(const CMChar* pmchA, const CMChar* pmchB,
                                                MatchSetting matset) const
{
    return iCompareMChars(pmchA->uch(), pmchB->uch(), matset);
}


const CMCharOrder* CSortOrder::pmchord(const CMChar* pmch) const
{
    return m_pmchord + pmch->uch();
}


BOOL CSortOrder::bFirstPrimary(const Str8& s, const CMChar** ppmch)
{
    const char* pch = s;
    Length len = s.GetLength();

    // Determine the part of the string to be considered for sorting.
    // a. Skip leading and trailing spaces.
    // b. If there are any embedded newlines, then stop at the first.
    s_SortableSubString(&pch, &len);

    return bFirstPrimary(pch, len, ppmch);
}

BOOL CSortOrder::bFirstPrimary(const char* pch, int len, const CMChar** ppmch)
{
    ASSERT( pch );
    ASSERT( len >= 0 );
    ASSERT( ppmch );

    // Make changes to the sort key (optional step for non-Roman scripts).
#if UseCct
    if ( m_cct.bLoaded() && len != 0 )
        {
        char* pchChanged = NULL;
        int lenChanged = 0;
        (void) bChangeKeyCCT(pch, len, &pchChanged, &lenChanged);
        pch = pchChanged;
        ASSERT( pch );
        len = lenChanged;
        ASSERT( len >= 0 );
        }
    else 
#endif
		if ( m_pfMakeSortKey && !m_bSkipSeq )
        {
        char* pchChanged = NULL;
        int lenChanged = 0;
        (void) bChangeKeyDLL(pch, len, &pchChanged, &lenChanged);
        pch = pchChanged;
        ASSERT( pch );
        len = lenChanged;
        ASSERT( len >= 0 );
        }

	// Find the first primary sorting character
	const CLangEnc* plng = plngMyOwner();
	const CMChar* pmch = NULL;
    const char* pchEnd = pch + len;
	while ( pch < pchEnd )
		{
		if ( *pch == '\n' )
			break;  // Limited to the first line of a multi-line field

        // For characters that are not defined in the sort order,
        // a CMChar object is returned from the first if condition,
        // but the second if condition fails.
		if ( plng->bMChar(&pch, &pmch) )
			{
            Order ord = pmchord(pmch)->ordPri();
			if ( CMCharOrder::ordFirstOrdered <= ord &&
					ord <= CMCharOrder::ordLastOrdered )
				break;
			}

        pmch = NULL;
		}

	if ( !pmch )
        return FALSE;

    // Find the first character in its primary group
	const CMChar* pmchFirst = pmch;
	Order ordPri = pmchord(pmch)->ordPri();
	for ( pmch = plng->pmchFirst(); pmch; pmch = plng->pmchNext(pmch) )
		{
		if ( pmchord(pmch)->ordPri() == ordPri &&
				pmchord(pmch)->ordSec() < pmchord(pmchFirst)->ordSec() )
			pmchFirst = pmch;
		}							
	pmch = pmchFirst;

    *ppmch = pmch;
    return TRUE;
}

BOOL CSortOrder::bFirstPrimaryLowerCase(const char* pch, int len, Str8& sMChar)
{
    const CMChar* pmch = NULL;
    if ( !bFirstPrimary(pch, len, &pmch) )
        return FALSE;

	// Use its lower case associate, if it has one
    ASSERT( pmch );
	if ( pmch->bUpperCase() && pmch->bHasOtherCase() )
		pmch = pmch->pmchOtherCase();
    ASSERT( pmch );

    sMChar = pmch->sMChar();
    return TRUE;
}
    

// ----------------------------------------------------------------------------
#ifdef ESCES
// ESCES: External Scripture Checking Exchange Standard

static const TLength s_maxlenKey = 255;
static LngSortCode s_pcodA[s_maxlenKey + 1];
static LngSortCode s_pcodB[s_maxlenKey + 1];

int CSortOrder::iCompareStrings(const char* pchrA,
        const char* pchrB) const
{
    ASSERT( pchrA );
    TLength lenA = strlen(pchrA); // 1.4ds remove t form
    ASSERT( pchrB );
    TLength lenB = strlen(pchrB); // 1.4ds remove t form

    return iCompareSubstrings(pchrA, lenA, pchrB, lenB);
}

BOOL CSortOrder::bMatchStrings(const char* pchrA,
        const char* pchrB,
        BOOL bDistinguishCase, BOOL bWhole) const
{
    ASSERT( pchrA );
    TLength lenA = strlen(pchrA); // 1.4ds remove t form
    ASSERT( pchrB );
    TLength lenB = strlen(pchrB); // 1.4ds remove t form

    return bMatchSubstrings(pchrA, lenA, pchrB, lenB,
        bDistinguishCase, bWhole);
}

int CSortOrder::iCompareSubstrings(
        const char* pchrA, TLength lenA,
        const char* pchrB, TLength lenB) const
{
    TLength lenKey = lenMakeSortKey(pchrA, lenA, s_pcodA, s_maxlenKey);
    lenKey = lenMakeSortKey(pchrB, lenB, s_pcodB, s_maxlenKey);

    return iCompareSortKeys(s_pcodA, s_pcodB);
}

BOOL CSortOrder::bMatchSubstrings(
        const char* pchrA, TLength lenA,
        const char* pchrB, TLength lenB,
        BOOL bDistinguishCase, BOOL bWhole) const
{
    TLength lenKey = lenMakeSortKey(pchrA, lenA, s_pcodA, s_maxlenKey);
    lenKey = lenMakeSortKey(pchrB, lenB, s_pcodB, s_maxlenKey);

    return bMatchSortKeys(s_pcodA, s_pcodB, bDistinguishCase, bWhole);
}

int CSortOrder::iCompareSortKeys(const LngSortCode* pcodA,
        const LngSortCode* pcodB) const
{
    // 1997-06-27 MRP: TEMPORARILY convert 16-bit codes to 8-bit codes
    UIdMChar puchA[s_maxlenKey + 1];
    UIdMChar puchB[s_maxlenKey + 1];
    UIdMChar* puch = puchA;
    const LngSortCode* pcod = pcodA;
    ASSERT( pcod );
    TLength lenKey = 0;
    for ( ; *pcod; pcod++, lenKey++ )
        if ( lenKey < s_maxlenKey )
            *puch++ = (UIdMChar)*pcod;
    *puch = '\0';
    puch = puchB;
    pcod = pcodB;
    ASSERT( pcod );
    lenKey = 0;
    for ( ; *pcod; pcod++, lenKey++ )
        if ( lenKey < s_maxlenKey )
            *puch++ = (UIdMChar)*pcod;
    *puch = '\0';

    return iCompare(puchA, puchB, CMCharOrder::matsetEvenIgnore, TRUE);
}

BOOL CSortOrder::bMatchSortKeys(const LngSortCode* pcodA,
        const LngSortCode* pcodB,
        BOOL bDistinguishCase, BOOL bWhole) const
{
    // 1997-06-27 MRP: TEMPORARILY convert 16-bit codes to 8-bit codes
    UIdMChar puchA[s_maxlenKey + 1];
    UIdMChar puchB[s_maxlenKey + 1];
    UIdMChar* puch = puchA;
    const LngSortCode* pcod = pcodA;
    ASSERT( pcod );
    TLength lenKey = 0;
    for ( ; *pcod; pcod++, lenKey++ )
        if ( lenKey < s_maxlenKey )
            *puch++ = (UIdMChar)*pcod;
    *puch = '\0';
    puch = puchB;
    pcod = pcodB;
    ASSERT( pcod );
    lenKey = 0;
    for ( ; *pcod; pcod++, lenKey++ )
        if ( lenKey < s_maxlenKey )
            *puch++ = (UIdMChar)*pcod;
    *puch = '\0';

    MatchSetting matset = (bDistinguishCase ?
        CMCharOrder::matsetExact :
        CMCharOrder::matsetExactDisregardCase
        );
    int i = iCompare(puchA, puchB, matset, bWhole);

    return i == 0;
}

TLength CSortOrder::lenMakeSortKey(const char* pchr, TLength len,
        LngSortCode* pcod, TLength maxlenKey) const
{
    ASSERT( pchr );
    ASSERT( 0 <= len );
    const char* pchrEnd = pchr + len;
    ASSERT( pcod );
    ASSERT( 0 <= maxlenKey );
    const CLangEnc* plng = plngMyOwner();
    TLength lenKey = 0;
    for ( ; pchr < pchrEnd; lenKey++ )
        {
        const CMChar* pmch = plng->pmchNextSortingUnit(&pchr);
        if ( lenKey < maxlenKey )
            *pcod++ = pmch->uch();  // pmch->cod()
        }
    *pcod = 0;

    return lenKey;
}

TLength CSortOrder::s_lenCopySortKey(const LngSortCode* pcodFrom,
        LngSortCode* pcodTo, TLength maxlenKeyTo)
{
    ASSERT( pcodFrom );
    ASSERT( pcodTo );
    ASSERT( 0 <= maxlenKeyTo );
    TLength lenKey = 0;
    for ( ; *pcodFrom; pcodFrom++, lenKey++ )
        if ( lenKey < maxlenKeyTo )
            *pcodTo++ = *pcodFrom;
    *pcodTo = 0;

    return lenKey;
}

TLength CSortOrder::s_lenSortKey(const LngSortCode* pcod)
{
    ASSERT( pcod );
    TLength lenKey = 0;
    for ( ; *pcod; pcod++, lenKey++ )
        ;

    return lenKey;
}

#endif
// ----------------------------------------------------------------------------

BOOL CSortOrder::bModifyProperties(const char* pszName,
        const char* pszDescription,
        const char* pszPrimaryOrder, const char* pszSecondariesPreceding, const char* pszSecondariesFollowing,
        const char* pszIgnore, BOOL bSecAfterBase,
        CNote** ppnot)
{
    // NOTE NOTE NOTE: We must construct the update notification
    // *before* changing any member data, but must wait to use it
    // until *after* the modifications have been committed. 
    CSortOrderUpdate sup(this, pszName, 
        pszPrimaryOrder, pszSecondariesPreceding, pszSecondariesFollowing, 
        pszIgnore, bSecAfterBase);

    // If the name is being changed, verify that it will be valid
    BOOL bUpdatingName = !bEqual(pszName, sName());
    if ( bUpdatingName && !m_psrtsetMyOwner->bValidNewName(pszName, ppnot) )
        {
        (*ppnot)->SetStringContainingReferent(pszName);
        return FALSE;
        }

    // NOTE NOTE NOTE: One of these must be deleted before returning.
    CMCharOrder* pmchordUnModified = m_pmchord;     
    m_pmchord = new CMCharOrder[s_maxnumMCharOrders];
    
    m_ordPri = CMCharOrder::ordSecOnly;  // not yet in any group
    m_ordSecNext = CMCharOrder::ordFirstOrdered;
    
    // Notice: The order in which these functions are called matters: Secondaries before, then
    // primary groups, then 2ndary after, then ignores.
    if (!bSetSecondaries(pszSecondariesPreceding, ppnot) ||
            !bSetPrimaries(pszPrimaryOrder, ppnot) ||
            !bSetSecondaries(pszSecondariesFollowing, ppnot) ||
            !bSetIgnore(pszIgnore, ppnot) )
        {
        delete m_pmchord;
        m_pmchord = pmchordUnModified;  // abandon the modifications
        return FALSE;
        }
        
    // Use case associations defined in the language encoding
    // 1996-03-18 MRP: Can apply to both primaries and secondaries
    SetOrderDisregardingCase();

    if ( bUpdatingName )
        ChangeNameTo(pszName);
//      m_psrtsetMyOwner->ModifyName(this, pszName);
        
    ASSERT( pszDescription );
    m_sDescription = pszDescription;
    m_sPrimaryOrder = pszPrimaryOrder;
    m_sSecondariesPreceding = pszSecondariesPreceding;
    m_sSecondariesFollowing = pszSecondariesFollowing;
    m_sIgnore = pszIgnore;
    m_bSecAfterBase = bSecAfterBase;
    delete pmchordUnModified;

    extern void Shw_Update(CUpdate& up);        
    Shw_Update(sup);  // Notify other objects that this marker is modified

    return TRUE;
}

BOOL CSortOrder::bCopy(CSetEl** ppselNew)
{
    CSortEditDlg dlg(this, (CSortOrder**)ppselNew);
    return ( dlg.DoModal() == IDOK );
}

BOOL CSortOrder::bModify()
{
    CSortEditDlg dlg(this);
    return ( dlg.DoModal() == IDOK );  // let user view or modify properties
}

    
#ifdef _DEBUG
void CSortOrder::AssertValid() const
{
    ASSERT( m_pmchord );
    CMCharOrder* pmchord = m_pmchord;
    int num = s_maxnumMCharOrders;
    for ( ; num != 0; num -= 1, pmchord += 1 )
        pmchord->AssertValid();
        
    ASSERT( m_ordPri < m_ordSecNext );  // IS AN OVERFLOW POSSIBLE???
}
#endif  // _DEBUG


// --------------------------------------------------------------------------

void CSortOrderPtr::IncrementNumberOfRefs(CSortOrder* psrt)
{
    m_psrt = psrt;
    if ( m_psrt )
        {
        m_psrt->IncrementNumberOfRefs();
        m_psrt->plngMyOwner()->IncrementNumberOfRefs();
        }
}

void CSortOrderPtr::DecrementNumberOfRefs()
{
    if ( m_psrt )
        {
        m_psrt->DecrementNumberOfRefs();
        m_psrt->plngMyOwner()->DecrementNumberOfRefs();
        }
}


const CSortOrderPtr& CSortOrderPtr::operator=(CSortOrder* psrt)
{
    if ( m_psrt != psrt )
        {
        DecrementNumberOfRefs();
        IncrementNumberOfRefs(psrt);
        }
        
    return *this;
}                                                 


// --------------------------------------------------------------------------

void CSortOrderInternalPtr::IncrementNumberOfRefs(CSortOrder* psrt)
{
    m_psrt = psrt;
    if ( m_psrt )
        m_psrt->IncrementNumberOfRefs();
}

void CSortOrderInternalPtr::DecrementNumberOfRefs()
{
    if ( m_psrt )
        m_psrt->DecrementNumberOfRefs();
}


const CSortOrderInternalPtr& CSortOrderInternalPtr::operator=(CSortOrder* psrt)
{
    if ( m_psrt != psrt )
        {
        DecrementNumberOfRefs();
        IncrementNumberOfRefs(psrt);
        }
        
    return *this;
}                                                 


// --------------------------------------------------------------------------

CSortOrderUpdate::CSortOrderUpdate(CSortOrder* psrt)
{
    m_psrt = psrt;
    ASSERT( m_psrt );
    m_bModified = FALSE;
    m_bNameModified = FALSE;
    m_bOrderModified = FALSE;
}

CSortOrderUpdate::CSortOrderUpdate(CSortOrder* psrt, const char* pszName,
        const char* pszPrimaryOrder, const char* pszSecondariesPreceding,
        const char* pszSecondariesFollowing,
        const char* pszIgnore, BOOL bSecAfterBase)
{
    m_psrt = psrt;
    ASSERT( m_psrt );
    m_bModified = TRUE;
    m_bNameModified = ( pszName != m_psrt->sName() );
    ASSERT( pszPrimaryOrder );
    ASSERT( pszIgnore );
    m_bOrderModified = ( !bEqual(pszPrimaryOrder, m_psrt->pszPrimaryOrder()) ||
        !bEqual(pszSecondariesPreceding, m_psrt->pszSecondariesPreceding()) ||
        !bEqual(pszSecondariesFollowing, m_psrt->pszSecondariesFollowing()) ||
        !bEqual(pszIgnore, m_psrt->pszIgnore()) ||
        bSecAfterBase != m_psrt->bSecAfterBase() );
}


// --------------------------------------------------------------------------

CSortOrderSet::CSortOrderSet(CLangEnc* plngMyOwner)
{
    m_psrtDefault = NULL;
    m_plngMyOwner = plngMyOwner;
    ASSERT( m_plngMyOwner );
}


static const char* psz_srtset = "srtset";
static const char* psz_Default = "Default";

#ifdef lngWritefstream // 1.6.4ab 
void CSortOrderSet::WriteProperties(Object_ofstream& obs) const
#else
void CSortOrderSet::WriteProperties(Object_ostream& obs) const
#endif
{
    obs.WriteBeginMarker(psz_srtset);
    obs.WriteNewline();
    
    const CSortOrder* psrt = psrtFirst();
    for ( ; psrt; psrt = psrtNext(psrt) )
        psrt->WriteProperties(obs); // 1.6.4ab 
        
    m_psrtDefault->WriteRef(obs, psz_Default);
    
    obs.WriteEndMarker(psz_srtset);
}

BOOL CSortOrderSet::bReadProperties(Object_istream& obs)
{
    if ( !obs.bReadBeginMarker(psz_srtset) )
        return FALSE;
        
    CSortOrder* psrtDefault = NULL;

    while ( !obs.bAtEnd() )
        {
        CSortOrder* psrt = NULL;
        if ( CSortOrder::s_bReadProperties(obs, this, &psrt) )
            {
            if ( psrt )  // 2000-06-12 MRP
                CSet::Add(psrt);
            }
        else if ( bReadSortOrderRef(obs, psz_Default, psrtDefault) )
            ;
        else if ( obs.bEnd(psz_srtset) )
            break;
        }

    m_psrtDefault = ( psrtDefault ? psrtDefault : psrtFirst() );
    if ( !m_psrtDefault )  // This set contained no sort orders
        AddDefaultSortOrder(m_plngMyOwner->sName());
            
    return TRUE;
}

BOOL CSortOrderSet::bReadSortOrderRef(Object_istream& obs, const char* pszQualifier,
        CSortOrder*& psrt)
{
    Str8 sName;
    if ( !obs.bReadString(psz_srt, pszQualifier, sName) )
        return FALSE;
        
    CSortOrder* psrtFound = psrtSearch(sName);
    if ( !psrtFound )
        return FALSE;
        
    psrt = psrtFound;
    return TRUE;
}


void CSortOrderSet::AddDefaultSortOrder(const char* pszName)
{
    CSortOrder* psrtDefault = NULL;
    CSortOrder::s_ConstructDefault(pszName, this, &psrtDefault);
    m_psrtDefault = psrtDefault;
    Add(m_psrtDefault);
}

BOOL CSortOrderSet::bAddSortOrder(const char* pszName,
        const char* pszDescription,
        const char* pszPrimaryOrder, const char* pszSecondariesPreceding, const char* pszSecondariesFollowing,
        const char* pszIgnore, BOOL bSecAfterBase,
        CSortOrder** ppsrt, CNote** ppnot)
{
    CSortOrder* psrt = NULL;
    if ( !CSortOrder::s_bConstruct(pszName, pszDescription,
            pszPrimaryOrder, pszSecondariesPreceding, pszSecondariesFollowing,
            pszIgnore, bSecAfterBase, "", "", FALSE, this, &psrt, ppnot) )
        return FALSE;
        
    if ( psrtSearch(pszName) )
        {
        *ppnot = new CNote(_("A sort order already exists with this name:"), pszName, // 1.5.0ft 
            pszName, strlen(pszName), pszName);
        delete psrt;
        return FALSE;
        }
        
    Add(psrt);
    *ppsrt = psrt;  
    return TRUE;
}


void CSortOrderSet::SetDefaultSortOrder(CSortOrder* psrt)
{
    ASSERT( bIsMember(psrt) );
    m_psrtDefault = psrt;
}


BOOL CSortOrderSet::bViewProperties(CSortOrder** ppsrtSelected)
{
    return FALSE;
}


BOOL CSortOrderSet::bViewPropertiesOfNewSortOrder(CSortOrder** ppsrtNew,
        const char* pszName )
{
    DlgNotYetImplemented();
    return FALSE;
}

BOOL CSortOrderSet::bAdd(CSetEl** ppselNew)
{
    CSortEditDlg dlg(this, (CSortOrder**)ppselNew);
    return ( dlg.DoModal() == IDOK );  // show a modal view of a new variable
}

void CSortOrderSet::Delete(CSetEl** ppsel)
{
    ASSERT( ppsel );
    const Str8& sName = (*ppsel)->sName();
    Str8 sMessage = _("Delete sort order?"); // 1.5.0fg 
    if ( AfxMessageBox(sMessage, MB_YESNO | MB_ICONQUESTION) != IDYES )
        return;
    CSet::Delete(ppsel);
}
    

#ifdef _DEBUG
void CSortOrderSet::AssertValid() const
{
    CSortOrder* psrt = psrtFirst();
    for ( ; psrt; psrt = psrtNext(psrt) )
        psrt->AssertValid();
    
    ASSERT( bIsMember(m_psrtDefault) ); 
}
#endif  // _DEBUG


void CSortOrderSetListBox::InitLabels()
{
    m_xName = xSubItem_Show(IDC_SET_NAME);
    m_xDescription = xSubItem_Show(IDC_SET_DESCRIPTION);
}

void CSortOrderSetListBox::DrawElement(CDC& cDC, const RECT& rcItem,
            const CDblListEl *pel)
{
    CSortOrder* psrt = (CSortOrder*)pel;
    DrawSubItem(cDC, rcItem, m_xName, m_xDescription, psrt->sName());
    DrawSubItem(cDC, rcItem, m_xDescription, 0, psrt->sDescription());
}

// **************************************************************************
