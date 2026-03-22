// lng.cpp Implementation of language encodings (1995-02-25)

#include "stdafx.h"
#include "toolbox.h"

#include "shw.h"  // Shw_Update
#include "project.h"
#include "lng.h"
#include "font.h"
#include "obstream.h"  // Object_istream, Object_ostream
#if UseCct
#include "ChangeTable.h"  // class ChangeTable
#endif
#include "tools.h"  // sPath
#include <fstream>  // ifstream
#include <wingdi.h> // ExtTextOutW
#include "usp10.h" // Uniscribe ScriptString functions

// 11-08-1997
#include "nlstream.h"

#include <sstream>

#include "patch.h"
#include "shwnotes.h"

#include "lng_d.h"
#include "find_d.h"

#include "dirlist.h"
#include "typ.h"
#include "malloc.h"
#include "strstream_d255.h"

// #define _Graphite // Temp for initial test
#ifdef _Graphite
#include "GrClient.h"
#endif

#ifdef _DEBUG
    #undef THIS_FILE
    static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#ifdef TEST_BUILD
    #undef ASSERT
    #define ASSERT(f) ((void)0)
#endif

// **************************************************************************

#define SQUARE_BRACKETS

#ifdef SQUARE_BRACKETS
const char Shw_chLMeta = '[';
const char Shw_chRMeta = ']';
#else
const char Shw_chLMeta = '\xAB';  // 0d171
const char Shw_chRMeta = '\xBB';  // 0d187
#endif  // ASCII_CHEVRONS

const char Shw_pszLMeta[] = { Shw_chLMeta, '\0' };
const char Shw_pszRMeta[] = { Shw_chRMeta, '\0' };


// --------------------------------------------------------------------------

CMChar::CMChar(const char* pszMChar, UIdMChar uch, CLangEnc* plngMyOwner) :
    CSetEl(pszMChar),
    m_sMCharDraw("  ")
{
    ASSERT( pszMChar != NULL );
    ASSERT( *pszMChar != '\0' );  // 1995-03-31 MRP: Valid for Undefined???
    m_bUpperCase = FALSE;
    m_pmchOtherCase = NULL;
    ASSERT( uch != 0 );
    m_uch = uch;
    m_sMCharDraw += pszMChar;
    m_plngMyOwner = plngMyOwner;
    ASSERT( m_plngMyOwner );
    m_lSubMCharsInVar = 0L;  // 1999-05-18 TLB: From set.h (CSubSetEl)
}

CMChar::~CMChar()
{
    // 1995-02-25 MRP: eventually, there may be integrity conditions to check
}


CSet* CMChar::psetMyOwner()
{
    ASSERT( FALSE );
    return m_plngMyOwner;
}


BOOL CMChar::bCaseAssociateOf(const CMChar* pmch) const
{
    // If the multichars are identical, they are obviously associates.
    if ( this == pmch )
        return TRUE;

    // If they aren't identical, then they cannot be case associates
    // unless both have an other-case associate.
    ASSERT( pmch );
    if ( !bHasOtherCase() || pmch->bHasOtherCase() )
        return FALSE;

    // 1997-06-17 MRP: At present we allow only one upper case
    // (but possibly multiple lower case). When we allow "title case"
    // for multicharacter units, this logic will need to be changed.
    const CMChar* pmchThisUpper = (m_bUpperCase ? this : m_pmchOtherCase);
    const CMChar* pmchUpper = (pmch->bUpperCase() ? pmch : pmch->pmchOtherCase());
    return pmchThisUpper == pmchUpper;
}

BOOL CMChar::bSetUpperCase(CMChar* pmchLowerCaseCounterpart)
{
    if ( m_pmchOtherCase )
        return FALSE;  // Already defined

    ASSERT( !m_bUpperCase );        
    m_bUpperCase = TRUE;
    m_pmchOtherCase = pmchLowerCaseCounterpart;
    ASSERT( m_pmchOtherCase );
    
    return TRUE;
}

BOOL CMChar::bSetLowerCase(CMChar* pmchUpperCaseCounterpart)
{
    if ( m_pmchOtherCase )
        return FALSE;
        
    ASSERT( !m_bUpperCase );
    m_pmchOtherCase = pmchUpperCaseCounterpart;
    ASSERT( m_pmchOtherCase );
    
    return TRUE;
}

void CMChar::ClearCase()
{
    m_bUpperCase = FALSE;
    m_pmchOtherCase = NULL;
}


void CMChar::WriteMetaMChar(std::ostream& ios) const
{
    BOOL bMultiByte = ( sMChar().GetLength() != 1 );
    if ( bMultiByte )
        ios << Shw_pszLMeta;
    ios << sMChar();
    if ( bMultiByte )
        ios << Shw_pszRMeta;
}


BOOL CMChar::bWordChar() const
{
    if ( Shw_bAtWhiteSpace(sMChar()) )
        return FALSE;

    // 1998-01-31 MRP: Had been global (yuck)
    CVar* pvarPunct = m_plngMyOwner->plngsetMyOwner()->pvarset()->pvarPunct();
    if ( pvarPunct->bIncludes(this) )
        return FALSE;

    return TRUE;
}


#ifdef _DEBUG
void CMChar::AssertValid() const
{
    ASSERT( !sKey().IsEmpty() );
}
#endif  // _DEBUG


// --------------------------------------------------------------------------

CLangEnc::CLangEnc(CLangEncProxy* plrxMyProxy) :
    m_plrxMyProxy(plrxMyProxy),
    m_bModified(FALSE)
{
    ASSERT( m_plrxMyProxy );
    m_uchNext = 1; // TLB 4/3/2000: Field separator character is now 0xFFFF, so we can use 1-2500
    m_pmchUndefined = new CMChar("\x1A", m_uchNext++, this);
    m_psrtset = new CSortOrderSet(this);
    m_pvinset = new CVarInstanceSet(this);
    m_bPropertiesDlgKeyboard = FALSE;
    m_pkbdPropertiesDlg = NULL;
    m_bUseInternalKeyboard = FALSE; // 1.5.1mc 
    m_bRightToLeft = FALSE;
    m_bRTLRightJustify = FALSE; // 1.2bd Make option for rtl right justify
	m_bUnicodeLang = TRUE; // 1.4qzpc Make new language encoding default to Unicode
    m_bRenDLL = FALSE;
    m_iTable = 0;
    m_bMultiChar = FALSE;
    m_bDBCS = FALSE;
#ifdef LNG_DOUBLE_BYTE
    m_bDoubleByteChar = FALSE;
#endif
    m_bSingleChar = TRUE;

    // 1996-08-09 MRP: Eliminate the internal space variable.
    // 1995-04-01 MRP: separate this out as a helper function
    // White space multichars are members of every language encoding
    // Cannot use the ordinary mechanism, since white space is a delimiter!
    // NOTE NOTE NOTE: MyProxy must be set in order to get MyOwner.
    const char* psz = Shw_pszWhiteSpace;
    for ( ; *psz != '\0'; psz += 1 )
        {
        Str8 sMChar = *psz;
        CMChar* pmch = pmchAdd(sMChar);
//      pmch->SetMemberOfVar(pvar);
        }
}


CLangEnc::~CLangEnc()
{
    if ( m_bRenDLL ) // If rendering DLL in use, free it
        FreeLibrary( m_hRenDLL );

    if ( !Shw_pProject()->bClosing() )
        Shw_pProject()->pfndset()->pftxlst()->DeletingLanguage( this ); // notify find text list of language being unloaded

    // Cannot destroy this if any marker still refers to it.
    delete m_pmchUndefined;
    delete m_psrtset;
    delete m_pvinset;
}


static const char* psz_LangEncExt = ".lng"; 
static const char* psz_LangEnc = "LanguageEncoding";
static const char* psz_lng = "lng";
static const char* psz_ver = "ver";
static const char* psz_desc = "desc";
static const char* psz_case = "case";
static const char* psz_varset = "varset";
static const char* psz_var = "var";
static const char* psz_UseInternalKeyboard = "UseInternalKeyboard"; // 1.5.1mc 
static const char* psz_RightToLeft = "RightToLeft";
static const char* psz_RTLRightJustify = "RTLRightJustify";
static const char* psz_UnicodeLang = "UnicodeLang";
static const char* psz_Punctuation = "Punctuation"; // 1.4qa Add punc to lng
static const char* psz_RenDLL = "RenDLL";
static const char* psz_RenTable = "RenTable";
static const char* psz_MultiChar = "MultiChar";
static const char* psz_DBCS = "DBCS";
static const char* psz_KeyDefs = "KeyDefs"; // 1.5.0fe


void CLangEnc::SetDefaultProperties()
{
    Str8 sDefault = "Default";
    const Str8& sSortOrderName = ( sName().IsEmpty() ? sDefault : sName() );
//  strstream ios;
    strstream_d255 ios; // 1.4vxe Make new language encoding default to Unicode
    ios <<
        "\\+LanguageEncoding " << sName() << "\n" <<
        "\\desc Language encoding with default properties\n"
        "\\case A a\n" "B b\n" "C c\n" "D d\n" "E e\n" "F f\n" "G g\n" "H h\n"
        "I i\n" "J j\n" "K k\n" "L l\n" "M m\n" "N n\n" "O o\n" "P p\n"
        "Q q\n" "R r\n" "S s\n" "T t\n" "U u\n" "V v\n" "W w\n" "X x\n"
        "Y y\n" "Z z\n"
        "\\+srtset\n"
        ;
    ios <<
        "\\+srt " << sSortOrderName << "\n" <<
        "\\desc " << CSortOrder::s_pszDescription << '\n' <<
        "\\primary " << CSortOrder::s_pszPrimary <<
        "\\SecPreceding " << CSortOrder::s_pszSecondaryBefore << '\n' <<
        "\\SecFollowing " << CSortOrder::s_pszSecondaryAfter << '\n' <<
        "\\ignore " << CSortOrder::s_pszIgnore << '\n'
        ;
    if ( CSortOrder::s_bSecondariesFollowBase )
        ios << "\\SecAfterBase";
    ios <<
        "\\-srt\n"
        "\\-srtset\n"
        "\\+varset\n"
        "\\+var !\n"
        "\\chars . , ? : ; ! < > { } [ ] ( ) \" ' `\n"
        "\\-var\n"
        "\\+var @\n"
        "\\chars A a B b C c D d E e F f G g H h I i J j K k L l M m\n"
        "N n O o P p Q q R r S s T t U u V v W w X x Y y Z z\n"
        "\\+var D\n"
        "\\chars 0 1 2 3 4 5 6 7 8 9\n"
        "\\-var\n"
        "\\+var L\n"
        "\\chars a b c d e f g h i j k l m n o p q r s t u v w x y z\n"
        "\\-var\n"
        "\\+var U\n"
        "\\chars A B C D E F G H I J K L M N O P Q R S T U V W X Y Z\n"
        "\\-var\n"
        "\\-var\n"
        "\\+var cons\n"
        "\\chars B b C c D d F f G g H h J j K k L l M m\n"
        "N n P p Q q R r S s T t V v W w X x Y y Z z\n"
        "\\-var\n"
        "\\+var nasal\n"
        "\\chars M m N n\n"
        "\\-var\n"
        "\\+var vowel\n"
        "\\chars A a E e I i O o U u\n"
        "\\-var\n"
        "\\-varset\n"
		"\\UnicodeLang\n"
        "\\-LanguageEncoding\n"
        ;
    CNoteList notlst;
    Object_istream obs(ios, notlst);
    BOOL bRead = bReadProperties(obs);
    ASSERT( bRead );
}

void CLangEnc::CleanUpKeyDefs() // 1.5.0ff Clean up the key defs for case and correct nl's
	{
	m_sKeyDefs.Replace( "dead", "Dead" ); // 1.5.0ff Standardize case
	m_sKeyDefs.Replace( "  ", " ", TRUE ); // 1.5.0hb Remove extras spaces
	m_sKeyDefs.Replace( "\n ", "\n", TRUE ); // 1.5.0ff Remove leading spaces from line
	m_sKeyDefs.Trim(); // 1.5.0hb 
	m_sKeyDefs = "\n" + m_sKeyDefs + "\n"; // 1.5.0ff Make sure there is an nl at each end
	}

BOOL CLangEnc::bReadProperties(CNoteList& notlst)
{
    const Str8& sPath = m_plrxMyProxy->sPath();
	if ( !bAllASCIIComplain( sPath ) ) // 1.4vze 
		return FALSE; // 1.4vze 
	m_sLng.bReadFile( sPath ); // 1.5.0fe
	int iStartKeyDefs = 0; // 1.5.0fe
	m_sLng.bGetSettingsTagSection( m_sKeyDefs, psz_KeyDefs, iStartKeyDefs, FALSE ); // 1.5.0fe 
	CleanUpKeyDefs(); // 1.5.0ff 
    std::ifstream ios(sPath);
    if ( ios.fail() )
        return FALSE;

     //11-08-1997
     //The class Newline_istream provieds datas from both, Mac and PC
    Newline_istream iosnl(ios);
        
    Object_istream obs(iosnl, notlst);
	if ( !bReadProperties(obs) ) // 1.5.0fe 
		return FALSE; // 1.5.0fe 
	m_sUnrecognizedSettingsInfo.bDeleteSettingsTagSection( psz_KeyDefs ); // 1.5.0fe 
    return TRUE; // 1.5.0fe 
}

BOOL CLangEnc::bReadProperties(Object_istream& obs)
{
    Str8 s;
    if ( !obs.bAtBackslash() || !obs.bReadBeginMarker(psz_LangEnc, s) )
        return FALSE;
        
    const char* psz = sName();

    Str8 sCase;
	m_bUnicodeLang = FALSE; // 1.4qzqb Fix problem (1.4qzpc) of language encodings changing to Unicode
#if defined(_MAC)
    CFontDef fntdef("Times", 12);
#elif defined(TLB_07_18_2000_FONT_HANDLING_BROKEN)
//    CFontDef fntdef("Times New Roman", 16);
#else
    CFontDef fntdef("Times New Roman", 12, TRUE /* Points */);
#endif
    
    while ( !obs.bAtEnd() ) // While more in file
        {
        if ( obs.bReadString(psz_ver, s) )
            ;
        else if ( obs.bReadString(psz_desc, m_sDescription) )
            ;
        else if ( obs.bReadString(psz_case, sCase) )
            ;
        else if ( m_psrtset->bReadProperties(obs) )
            ;
        else if ( m_pvinset->bReadProperties(obs) )
            ;
        else if ( fntdef.bReadProperties(obs) )
            ;
        else if ( CKeyboard::s_bReadKeyboardWin32(obs, m_sKeyboardWin32) )
            ;
        else if ( CKeyboard::s_bReadKeyboardWin16(obs, m_sKeyboardWin16) )
            ;
        else if ( obs.bReadBool( psz_UseInternalKeyboard, m_bUseInternalKeyboard ) ) // 1.5.1mc 
            ;
        else if ( obs.bReadBool( psz_RightToLeft, m_bRightToLeft ) )
            ;
        else if ( obs.bReadBool( psz_RTLRightJustify, m_bRTLRightJustify ) )
            ;
        else if ( obs.bReadBool( psz_UnicodeLang, m_bUnicodeLang ) )
            ;
        else if ( obs.bReadString( psz_Punctuation, m_sPunct ) )
            ;
        else if ( obs.bReadString( psz_RenDLL, m_sRenDLL ) )
            ;
        else if ( obs.bReadString( psz_RenTable, m_sRenTable ) )
            ;
        else if ( obs.bReadBool( psz_MultiChar, m_bMultiChar ) )
            ;
        else if ( obs.bReadBool( psz_DBCS, m_bDBCS ) )
            ;
		else if ( obs.bUnrecognizedSettingsInfo( psz_LangEnc, m_sUnrecognizedSettingsInfo  ) ) // 1.0cp Store any unrecognized settings info so as not to lose it
			;
        else if ( obs.bEnd( psz_LangEnc ) ) // If end marker or erroneous begin break
            break;
        }

    if ( !m_psrtset->psrtDefault() )  // If the \+srtset section was missing
        {
        ASSERT( m_psrtset->bIsEmpty() );        
        m_psrtset->AddDefaultSortOrder(sName());
        }
    ASSERT( m_psrtset->psrtDefault() );

    // NOTE: We store the case string as read from the .lng file
    // even if it has problems. We set the case here just in case (ha!)
    // the sort order set preceded the \case property field.
    CNote* pnot = NULL;
    if ( !bSetCase(sCase, TRUE, &pnot) )
        {
        ASSERT( pnot );
//        pnot->Add(IDN_lng_InLangEnc1, sName());
//        if ( !m_plrxMyProxy->plngsetMyOwner()->bReadFromString() )
//            pnot->Add(IDN_lng_InLangEncFile1, m_plrxMyProxy->sPath());
//        obs.notlst().Add(pnot);
        }

	if ( FALSE ) // !m_bUnicodeLang ) // 1.4vxv If lng has no upper Ansi chars, make it Unicode // 1.5.0db Undo auto change lng to Unicode (1.4vxv) because of bugs
		{
		CSortOrder* psrt = m_psrtset->psrtDefault(); // 1.4vxv Look in default sort order for upper Ansi chars
		Str8 s = psrt->pszPrimaryOrder(); // 1.4vxv 
		s = s + psrt->pszSecondariesPreceding() + psrt->pszSecondariesFollowing() + psrt->pszIgnore(); // 1.4vxv Put all chars in one string
		BOOL bUpperAnsi = m_bRightToLeft; // 1.4vxv  // 1.4ywb Fix bug (1.4vxv) of setting rtl legacy lng to Unicode
		for ( const char* psz = s; *psz; psz++ ) // 1.4vxv Look at each char
			if ( !( *psz >= 0 && *psz <= 127 ) )  // 1.4vxv If an upper Ansi char is found, this is a non-Unicode lng
				{
				bUpperAnsi = TRUE; // 1.4vxv 
				break;
				}
		if ( !bUpperAnsi ) // 1.4vxv If no upper Ansi char found, make lng Unicode
			m_bUnicodeLang = TRUE; // 1.4vxv 
		}

    LoadRenDLL();  // If there's a rendering DLL, load it and its table
    SetFont( &fntdef ); // create font  
    m_pkbd = plngsetMyOwner()->pkbdset()->pkbdSearch_DefaultIfNew(m_sKeyboardWin32); // 1.1hg Fix keyboard switch failures from missing or default keyboard
    return TRUE;    
}

BOOL CLangEnc::bReadSortOrderRef(Object_istream& obs, const char* pszQualifier,
        CSortOrder*& psrt)
{
    return m_psrtset->bReadSortOrderRef(obs, pszQualifier, psrt);
}

#ifdef lngWritefstream // 1.6.4ab 
void CLangEnc::WriteProperties(Object_ofstream& obs) const
#else
void CLangEnc::WriteProperties(Object_ostream& obs) const
#endif
{
	if ( bUnicodeLang() ) // 1.4qnh If Unicode lng file, write bom at top
		obs.WriteUTF8BOM(); // 1.4qnh Write bom at top of Unicode lng file
    obs.WriteBeginMarker(psz_LangEnc, sName());

    // 1998-03-12 MRP: Always write the current version
    obs.WriteString(psz_ver, plngsetMyOwner()->sSettingsVersion());

    obs.WriteString(psz_desc, m_sDescription);
    obs.WriteString(psz_case, m_sCase);
    obs.WriteString(psz_Punctuation, m_sPunct); // 1.4qa Add punc to lng
    m_psrtset->WriteProperties(obs);
    m_pvinset->WriteProperties(obs);
    CFontDef fntdef( &m_fnt, m_rgbColor );
    fntdef.WriteProperties(obs);

    // 1999-09-29 MRP
#ifdef _MAC
    // Macintosh
    CKeyboard::s_WriteKeyboardWin32(obs, m_sKeyboardWin32);
    CKeyboard::s_WriteKeyboardWin16(obs, m_sKeyboardWin16);
#else
    // Windows 95 and later, Keyman 4 and later
    if ( m_pkbd )
        m_pkbd->WriteRef(obs);
    CKeyboard::s_WriteKeyboardWin16(obs, m_sKeyboardWin16);
#endif
    obs.WriteBool(psz_UseInternalKeyboard, m_bUseInternalKeyboard ); // 1.5.1mc 
	obs.WriteBeginMarker( psz_KeyDefs ); // 1.5.0fe Add KeyDefs section to lng file
	obs.WriteUnrecognizedSettingsInfo( m_sKeyDefs ); // 1.5.0fe 
	obs.WriteNewline(); // 1.5.0fe 
	obs.WriteEndMarker( psz_KeyDefs ); // 1.5.0fe 

    obs.WriteBool(psz_RightToLeft, m_bRightToLeft);
    obs.WriteBool(psz_RTLRightJustify, m_bRTLRightJustify);
    obs.WriteBool(psz_UnicodeLang, m_bUnicodeLang);
    obs.WriteString(psz_RenDLL, m_sRenDLL);
    obs.WriteString(psz_RenTable, m_sRenTable);
    obs.WriteBool(psz_MultiChar, m_bMultiChar);
    obs.WriteBool(psz_DBCS, m_bDBCS);
	obs.WriteUnrecognizedSettingsInfo( m_sUnrecognizedSettingsInfo ); // 1.0cp Write unrecognized settings info

    obs.WriteEndMarker(psz_LangEnc);
}

void CLangEnc::WritePropertiesIfModified( BOOL bForceWrite )
{
	CProject* pprj = Shw_papp()->pProject();
	if ( pprj->bExerciseNoSave() ) // 5.99f Don't save if exercises
		return;

    if ( !m_bModified && !bForceWrite )
        return;
        
    ASSERT( !m_plrxMyProxy->plngsetMyOwner()->bReadFromString() );
    const Str8& sPath = m_plrxMyProxy->sPath();
#ifdef lngWritefstream // 1.6.4ab 
	CString swPath = swUTF16( sPath ); // 1.6.1cb 
	Str8 sMode = "w"; // Set up mode string  // 1.6.1cb 
	CString swMode = swUTF16( sMode ); // Convert mode string to CString  // 1.6.1cb 
	FILE* pf = nullptr;
	errno_t err = _wfopen_s(&pf, swPath, swMode);	// Open file // 1.6.1cb 
	if (err != 0 || pf == nullptr) {	// If file open failed, return failure // 1.6.1cb 
		// handle error
		return; // 1.6.1cb 
	}
    Object_ofstream obs(pf); // 1.6.1cb 
#else // 1.6.1cb 
    std::ofstream ios(sPath); // 1.6.1cb 
    Object_ostream obs(ios); // 1.6.1cb 
#endif // 1.6.1cb 

    WriteProperties(obs);
    
    m_bModified = FALSE;
}

#ifdef prjWritefstream // 1.6.4aa 
void CLangEnc::WriteRef(Object_ofstream& obs, const char* pszQualifier) const
#else
void CLangEnc::WriteRef(Object_ostream& obs, const char* pszQualifier) const
#endif
{
    obs.WriteString(psz_lng, pszQualifier, sName());
}


CSortOrder* CLangEnc::psrtDefault() const
{
    return m_psrtset->psrtDefault();
}   


void CLangEnc::SetFont( CFontDef* pfntdef ) // Create new font based on parameters in pfntdef
{
    ASSERT(pfntdef);
	if( bRenDLL() )
		pfntdef->plf()->lfClipPrecision |= 0x40 /*CLIP_DFA_OVERRIDE*/;
    m_iLineHeight = pfntdef->iCreateFont( &m_fnt, &m_rgbColor,
        &m_iAscent, &m_iOverhang ); // create new font

    CreateCtrlFont( pfntdef ); // create a smaller font for language controls

    ClearPropertiesDlgFont();  // 1996-02-02 MRP
}


const CFont* CLangEnc::pfntDlg() const
{
#ifdef TryLarge
    if ( m_iLineHeight > Shw_pProject()->iLangCtrlHeight() )
        return &m_fntDlg;
    else
#endif
        return &m_fnt;
}

void CLangEnc::CreateCtrlFont( CFontDef* pfntdef ) // create a trimmed font (if main font too big) for language controls
{
    if ( m_iLineHeight > Shw_pProject()->iLangCtrlHeight() ) // do we need a trimmed version for listboxes, etc?
        {
        pfntdef->plf()->lfHeight = Shw_pProject()->iLangCtrlHeight();
#ifdef _MAC
        pfntdef->iCreateFontMakeFit( &m_fntDlg ); // create a smaller font
#else
        pfntdef->iCreateFont( &m_fntDlg ); // create a smaller font
#endif
        }
}

void CLangEnc::RecalcCtrlFont() // recalculate language control font size
{
    CFontDef fntdef( &m_fnt ); // start with normal language font
    CreateCtrlFont( &fntdef );
}

const CFont* CLangEnc::pfntPropertiesDlg() const
{
    return m_fntPropertiesDlg.m_hObject ? &m_fntPropertiesDlg : pfntDlg();
}

void CLangEnc::SetPropertiesDlgFont( CFontDef* pfntdef )
{
    ASSERT(pfntdef);
    int iLineHeight = pfntdef->iCreateFont( &m_fntPropertiesDlg ); // create new font
    if ( iLineHeight > Shw_pProject()->iLangCtrlHeight() ) // do we need a trimmed version?
        {
        int ilfHeight = pfntdef->plf()->lfHeight; // leave pfntdef unchanged at exit
        pfntdef->plf()->lfHeight = Shw_pProject()->iLangCtrlHeight();
#ifdef _MAC
        pfntdef->iCreateFontMakeFit( &m_fntPropertiesDlg ); // create a smaller font
#else
        pfntdef->iCreateFont( &m_fntPropertiesDlg ); // create a smaller font
#endif
        pfntdef->plf()->lfHeight = ilfHeight; // restore parm
        }
}

void CLangEnc::ClearPropertiesDlgFont()
{
    m_fntPropertiesDlg.DeleteObject();
}


// --------------------------------------------------------------------------
// Turn off optimization because DLL calls in 32 bit change the stack pointer
#pragma optimize( "", off )

// Notes from header of render.dll by Timm Erickson
// DllExport int iInit( const char* pszFileName );
//
// pszFileName is the name of the Naskh changes file to be loaded.
//
// returns 0 on failure
//         positive integer on success (index of rendering table created)
//

void CLangEnc::LoadRenDLL()
{
    ASSERT( !m_bRenDLL );  // Don't ever reload!
    ASSERT( !m_iTable );
#ifdef _MAC
    return;  // Rendering isn't supported on the Mac
#else
    if ( m_sRenDLL.IsEmpty() || m_sRenTable.IsEmpty() )
        return;  // This language encoding doesn't do rendering
        
    Str8 sPathDLL( m_sRenDLL );
    m_hRenDLL = LoadLibrary( swUTF16( sPathDLL ) ); // Find the DLL  // 1.4qth Upgrade LoadLibrary for Unicode build
    if ( m_hRenDLL < (HINSTANCE)HINSTANCE_ERROR ) // If not found, fail (Cast avoids compiler error in 32 bit)
        return;

	// Find the functions we need
    m_iInit = (iInitFunc*) GetProcAddress( m_hRenDLL, "iInit" );
    m_iEncode = (iEncodeFunc*) GetProcAddress( m_hRenDLL, "iEncode" );
    m_iUnderPos = (iUnderPosFunc*) GetProcAddress( m_hRenDLL, "iUnderPos" );
    m_iSurfacePos = (iSurfacePosFunc*) GetProcAddress( m_hRenDLL, "iSurfacePos" );
    // 1998-09-28 MRP: Extended functions are for rendering substrings
    m_iEncodeEx = (iEncodeFuncEx*) GetProcAddress( m_hRenDLL, "iEncodeEx" );
    m_iUnderPosEx = (iUnderPosFuncEx*) GetProcAddress( m_hRenDLL, "iUnderPosEx" );
    m_iSurfacePosEx = (iSurfacePosFuncEx*) GetProcAddress( m_hRenDLL, "iSurfacePosEx" );
    if ( !(m_iInit && m_iEncode && m_iUnderPos && m_iSurfacePos &&
    		m_iEncodeEx && m_iUnderPosEx && m_iSurfacePosEx) )
        {
        // If any functions are not found, fail
        FreeLibrary( m_hRenDLL );
        return;
        }

    Str8 sTablePath;
    const Str8& sSettingsDirPath = plngsetMyOwner()->sSettingsDirPath();
    sTablePath = sPath(sSettingsDirPath, m_sRenTable); // Put settings dir path on table
    m_iTable = m_iInit( sTablePath ); // Initialize with table
    if ( !m_iTable ) // If table not found or invalid, fail
        {
        FreeLibrary( m_hRenDLL );
        return;
        }

    m_bRenDLL = TRUE;  // Rendering DLL and table sucessfully loaded
#endif
}

// DllExport int iEncode(int iTable, const char* pszInput, int iMax, char* pszOutpu
//
// Applies a set of Naskh-style changes to a string
//
// iTable is the index of the rendering table to use (returned by iInit() )
// pszInput is the string (in underlying form) to be processed
// iMax is the maximum number of characters to return
// pszOutput is a buffer of iMax characters to store output
//
// returns 0 on fail
//         positive integer on success (undefined)
//
// DllExport int iUnderPos ( int iTable, const char* pszInput, int iSurfacePos );
//
// Gives a position in the underlying string corresponding to
// a given surface string position
//
// iTable is the index of the rendering table to use (returned by iInit() )
// pszInput is the string in its ORIGINAL/underlying form
// iSurfacePos is an index to a position in the ENCODED/surface form
//
// returns -1 on failure
//         >=0 on success (corresponding underlying position)
//
// DllExport int iSurfacePos( int iTable, const char* pszInput, int iUnderPos );
//
// Gives a position in the surface string corresponding to
// a given underlying string position
//
// iTable is the index of the rendering table to use (returned by iInit() )
// pszInput is the string in its ORIGINAL/underlying form
// iUnderPos is an index to a position in the ORIGINAL/underlying form
//
// returns -1 on failure
//         >=0 on success (corresponding surface position

int CLangEnc::iEncode( const char* pszUnderlying,
        int maxlen1, char* pszSurface ) const
{
    ASSERT( m_iTable );
    ASSERT( pszUnderlying );
    ASSERT( 0 < maxlen1 );
    ASSERT( pszSurface );
    int iSucc = m_iEncode( m_iTable, pszUnderlying, maxlen1, pszSurface );
    ASSERT( iSucc );
    return iSucc;
}

int CLangEnc::iUnderPos( const char* pszUnderlying, int iSurfacePos ) const
{
    ASSERT( m_iTable );
    ASSERT( pszUnderlying );
    ASSERT( 0 <= iSurfacePos );
    return m_iUnderPos( m_iTable, pszUnderlying, iSurfacePos );
}

int CLangEnc::iSurfacePos( const char* pszUnderlying, int iUnderPos ) const
{
    ASSERT( m_iTable );
    ASSERT( pszUnderlying );
    ASSERT( 0 <= iUnderPos );
    return m_iSurfacePos( m_iTable, pszUnderlying, iUnderPos );
}

int CLangEnc::iEncodeEx( int lenUnderlying, const char* pszUnderlying,
        int maxlen1, char* pszSurface ) const
{
    ASSERT( m_iTable );
    ASSERT( 0 <= lenUnderlying );
    ASSERT( pszUnderlying );
    ASSERT( 0 < maxlen1 );
    ASSERT( pszSurface );
    int iSucc = m_iEncodeEx( m_iTable, lenUnderlying, pszUnderlying, maxlen1, pszSurface );
    ASSERT( iSucc );
    return iSucc;
}

int CLangEnc::iUnderPosEx( int lenUnderlying, const char* pszUnderlying,
		int iSurfacePos ) const
{
    ASSERT( m_iTable );
    ASSERT( 0 <= lenUnderlying );
    ASSERT( pszUnderlying );
    ASSERT( 0 <= iSurfacePos );
    return m_iUnderPosEx( m_iTable, lenUnderlying, pszUnderlying, iSurfacePos );
}

int CLangEnc::iSurfacePosEx( int lenUnderlying, const char* pszUnderlying,
		int iUnderPos ) const
{
    ASSERT( m_iTable );
    ASSERT( 0 <= lenUnderlying );
    ASSERT( pszUnderlying );
    ASSERT( 0 <= iUnderPos );
    return m_iSurfacePosEx( m_iTable, lenUnderlying, pszUnderlying, iUnderPos );
}

// Return number of Unicode chars in UTF8 string up to iCurPos
int iNumUnicodeChars( const CLangEnc* plng, const char* pszLine, int iCurPos )
	{
	int iNumChars = 0;
	for ( int iPos = 0; 
			iPos < iCurPos;
			iPos += plng->iCharNumBytes( pszLine + iPos ) )
		iNumChars++;
	return iNumChars;
	}

static const int s_maxlenRenBuf = 1000;
static char s_pszRenBuf[s_maxlenRenBuf + 1];

CSize CLangEnc::GetTextExtent( const CDC* pDC,
        const char* pszUnderlying, int lenUnderlying,
        int lenContext /* = 0 */ ) const
{
    ASSERT( pDC );
    ASSERT( pszUnderlying );
    ASSERT( 0 <= lenUnderlying );
	CSize siz( 0, 0 );
	Str8 sUnderlying( pszUnderlying, lenUnderlying ); // 1.1gd Fix bug of spacing wrong on rtl annotations
	pszUnderlying = (const char*)sUnderlying; // 1.1gd
	if ( m_bUnicodeLang )
		{
#ifndef _MAC
		int iPixel = 0;
//		const int iMAXBUF = 10000; // 1.4rab
//		LPWSTR pwszUTF16 = (LPWSTR)alloca( iMAXBUF+1 ); // 1.4rab
		int iNumChars = MultiByteToWideChar( CP_UTF8, 0, pszUnderlying, -1, pswzBigBuffer, BIGBUFMAX ); // 1.4rab
		if ( iNumChars > 0 )
			{
			HRESULT  hr;
			SCRIPT_STRING_ANALYSIS ssa;
			DWORD dwFlags = SSA_GLYPHS;
			hr = ScriptStringAnalyse( pDC->m_hDC, (LPWSTR)pswzBigBuffer, iNumChars, 
				0, -1, dwFlags, 32767, NULL, NULL, NULL, NULL, NULL, &ssa );
			if ( hr == S_OK )
				{
				int iNumUChars = iNumUnicodeChars( this, pszUnderlying, lenUnderlying );
				if ( iNumUChars > iNumChars - 1 ) // Protect against partial UTF8 char at end of line during insert
					iNumUChars = iNumChars - 1;
				hr = ScriptStringCPtoX( ssa, iNumUChars, FALSE, &iPixel );
				ASSERT( hr == S_OK ); // or E_INVALIDARG
				ScriptStringFree( &ssa );
				}
			}
		siz.cx = iPixel;
#endif // _MAC
		}
    else if ( m_bRenDLL )
		{
		const char* pszSurface;
		int lenSurface;
		lenRenSubstring( pszUnderlying, lenUnderlying, lenContext, FALSE, s_pszRenBuf, s_maxlenRenBuf, &pszSurface, &lenSurface );
		Str8 sText = pszSurface; // 1.4qtg Upgrade some GetTextExtent for Unicode build
		CString swText = swUTF16( sText ); // 1.4qtg
		siz = pDC->GetTextExtent( swText, swText.GetLength() ); // 1.4qtg
		}
	else
		{
		Str8 sText = pszUnderlying; // 1.4qtg Upgrade some GetTextExtent for Unicode build // 1.4sw
//		CString swText = swUTF16( sText ); // 1.4qtg // 1.4sw
//		siz = pDC->GetTextExtent( swText, swText.GetLength() ); // 1.4qtg // 1.4sw
		GetTextExtentPoint32A( pDC->m_hDC, sText, sText.GetLength(), &siz ); // 1.4sw Fix U bug of incorrect count of width of upper ANSI chars
		}
	return siz;
}

int CLangEnc::GetTextWidth(const CDC* pDC,
        const char* pszUnderlying, int lenUnderlying,
        int lenContext /* = 0 */ ) const
{
    return GetTextExtent(pDC, pszUnderlying, lenUnderlying, lenContext).cx;
}

int CLangEnc::lenRenSubstring(const char* pchUnderlying, int lenUnderlying,
        int lenContext, BOOL bReverseIfRightToLeft,
        char* pszRenBuf, int maxlenRenBuf,
        const char** ppchSurface, int* plenSurface) const
{
    ASSERT( pchUnderlying );
    ASSERT( 0 <= lenUnderlying );
    ASSERT( ppchSurface );
    ASSERT( plenSurface );

    if ( !m_bRenDLL )
        {
        *ppchSurface = pchUnderlying;
        *plenSurface = lenUnderlying;
        return lenUnderlying;
        }

    ASSERT( 0 <= lenContext );
    ASSERT( lenContext == 0 || lenUnderlying <= lenContext );
    ASSERT( pszRenBuf );
    ASSERT( 0 <= maxlenRenBuf );

	int lenActual = 0;
	if ( lenContext == 0 )
		{
		// For strings terminated by zero or white space
	    int i = iEncode(pchUnderlying, maxlenRenBuf + 1, pszRenBuf);
	    lenActual = iSurfacePos(pchUnderlying, lenUnderlying);
	    }
	else
		{
	    // 1998-09-28 MRP: Extended functions are for rendering substrings
	    int i = iEncodeEx(lenContext, pchUnderlying, maxlenRenBuf + 1, pszRenBuf);
	    lenActual = iSurfacePosEx(lenContext, pchUnderlying, lenUnderlying);
		}
    ASSERT( 0 <= lenActual );
    int lenSurface = lenActual;
    if ( maxlenRenBuf < lenSurface )
        lenSurface = maxlenRenBuf;  // Result got truncated
    pszRenBuf[lenSurface] = '\0';
        
    if ( m_bRightToLeft && bReverseIfRightToLeft )
        _strrev(pszRenBuf);

    *ppchSurface = pszRenBuf;
    *plenSurface = lenSurface;
    return lenActual;
}  // CLangEnc::lenRenSubstring

BOOL CLangEnc::bClipAlignedSubstring(const CDC* pDC, BOOL bRightAligned,
        const char** ppszUnderlying, int* plenUnderlying,
        int* pixTextWidth) const
{
    ASSERT( pDC );
    ASSERT( ppszUnderlying );
    const char* psz = *ppszUnderlying;
    ASSERT( psz );
    ASSERT( plenUnderlying );
    int len = *plenUnderlying;
    ASSERT( 0 <= len );
    ASSERT( pixTextWidth );
    int ixTextWidth = *pixTextWidth;
    ASSERT( 0 <= ixTextWidth );

    int ix = GetTextWidth(pDC, psz, len);
    if ( ix <= ixTextWidth )
        {
        // If the text fits, no clipping is needed.
        *pixTextWidth = ix;  // ALWAYS return the actual display width.
        return FALSE;
        }
    
    if ( ixTextWidth == 0 )
        {
        // If the text can't possibly fit, don't waste time trying.
        *plenUnderlying = 0;
        return TRUE;
        }

    BOOL bClipFromEnd;
    if ( bRightAligned )
        bClipFromEnd = bRightToLeft();
    else
        bClipFromEnd = !bRightToLeft();

    while ( len != 0 && ixTextWidth < ix )
        {
        if ( bClipFromEnd )
            PrevEditingUnit(psz, &len);
        else
            {
            int lenUnit = 0;
            NextEditingUnit(psz, &lenUnit);
            psz += lenUnit;
            len -= lenUnit;
            }
        ASSERT( 0 <= len );
        ix = GetTextWidth(pDC, psz, len);
        }

    ASSERT( *ppszUnderlying <= psz );
    ASSERT( psz <= *ppszUnderlying + *plenUnderlying );
    *ppszUnderlying = psz;    
    *plenUnderlying = len;
    *pixTextWidth = ix;
    return TRUE;  // Needed to clip the text to fit.
}  // CLangEnc::bClipAlignedSubstring

// Restore frame pointer optimization, temporarily off for DLL calls
#pragma optimize( "", on )

CFont* CLangEnc::SelectFontIntoDC(CDC* pDC) const
{
    ASSERT( pDC );
    return pDC->SelectObject( (CFont*)pfnt() );  // Cast away const-ness
}

CFont* CLangEnc::SelectDlgFontIntoDC(CDC* pDC) const
{
    ASSERT( pDC );
    return pDC->SelectObject( (CFont*)pfntDlg() );  // Cast away const-ness
}

void CLangEnc::SetTextAndBkColorInDC(CDC* pDC, BOOL bHighlight) const
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


void CLangEnc::ExtTextOutLng(CDC* pDC, int ixBegin, int iyBegin,
        int nOptions, const CRect* prect,
        const char* psz, int len, int lenContext) const
{
    ASSERT( pDC );
    ASSERT( psz );
    ASSERT( 0 <= len );

 // 1.5.9md Make ssa non-local for Wine display bug test // 1.5.9me Undo 1.5.9md, make ssa local again
	HRESULT  hr;
	SCRIPT_STRING_ANALYSIS ssa;
	DWORD dwFlags = SSA_GLYPHS;

	if ( m_bUnicodeLang && len > 0 )
		{
		int iNumChars = MultiByteToWideChar( CP_UTF8, 0, psz, len, pswzBigBuffer, BIGBUFMAX );
#ifdef WineFix // 1.5.9mb Disable rendering for Wine display problem fix // 1.5.9na
		ExtTextOut(pDC->m_hDC, ixBegin, iyBegin, nOptions, prect, pswzBigBuffer, iNumChars, NULL);
#else // 1.5.9ma // 1.5.9mb
        if( m_bRightToLeft )
	        {
				// regarding the SCRIPT_STATE member of SSA below, MSDN says the following:
				//  The uBidiLevel member of SCRIPT_STATE is ignored. The value used is derived 
				//  from the SSA_RTL flag in combination with the layout of the hdc.
				// Therefore, tell the DC that we're RTL and use the SSA_RTL flag....
				// except that we do want it to be 'left-aligned' even if it's RTL or it'll show 
				//  way off to the right..., so just do the SSA_RTL bit.
				/*
				UINT nFlags = pDC->GetTextAlign();
				nFlags &= ~TA_LEFT;
				nFlags |= TA_RIGHT;
				pDC->SetTextAlign(nFlags);
				*/
            dwFlags |= SSA_RTL; // rde 533 tell SSA about right-to-left
	        }
		hr = ScriptStringAnalyse( pDC->m_hDC, pswzBigBuffer, iNumChars, 
			0, -1, dwFlags, 32767, NULL, NULL, NULL, NULL, NULL, &ssa );
		if ( hr == S_OK	)
			{
			hr = ScriptStringOut( ssa, ixBegin, iyBegin, nOptions, prect, 0, 0, FALSE );
			ScriptStringFree(&ssa); // 1.5.9mc Disable ScriptStringFree to test for Wine rendering // 1.5.9me Undo 1.5.9mc enable ScriptStringFree
			}
#endif // WineFix // 1.5.9ma // 1.5.9mb
		}
	else
		{
		// 1998-03-04 MRP: Use static buffer, increase its length,
		// and call lenRenSubstring instead of iEncode and iSurfacePos.
	    int lenActual = lenRenSubstring(psz, len, 0, TRUE, s_pszRenBuf, s_maxlenRenBuf, &psz, &len);
		
		// NOTE: ixBegin and iyBegin are ignored when using TA_UPDATECP mode
		// 1996-12-09 MRP: the ETO_CLIPPED attribute with a NULL prect leaves
		// the left and right edges undrawn in Windows 3.1
//		Str8 sText = psz; // 1.4qta // 1.4qzfd Don't do UTF8 convert on data display
//		CString swText = swUTF16( sText ); // 1.4qta // 1.4qzfd
		if ( prect )
			ExtTextOutA(pDC->m_hDC, ixBegin, iyBegin, nOptions, prect, psz, len, NULL); // 1.4qta // 1.4qzfd Fix bug of showing garbage in blank line
		else
			TextOutA( pDC->m_hDC, ixBegin, iyBegin, psz, len); // 1.4qta // 1.4qzfd
		}
}  // CLangEnc::ExtTextOut
/* Sample from CSSamp
		hr = ScriptStringAnalyse(
                hdc,
                g_wcBuf + iFirst,
                iLen, 0, -1,
                dwFlags,
                iClipWidth,
                g_fNullState ? NULL : &ScriptControl,
                g_fNullState ? NULL : &ScriptState,
                NULL,
                NULL, NULL, &ssa);

HRESULT WINAPI ScriptStringAnalyse(  
 HDC  hdc, //In Device context (required)  
 const void * pString, //In String in 8 or 16 bit characters  
 int  cString, //In Length in characters (Must be at least 1)  
 int  cGlyphs, //In Required glyph buffer size (default cString*3/2 + 1)  
 int  iCharset, //In Charset if an ANSI string, -1 for a Unicode string  
 DWORD  dwFlags, //In Analysis required  
 int  iReqWidth, //In Required width for fit and/or clip  
 SCRIPT_CONTROL * psControl, //In Analysis control (optional)  
 SCRIPT_STATE * psState, //In Analysis initial state (optional)  
 const int * piDx, //In Requested logical dx array  
 SCRIPT_TABDEF * pTabdef, //In Tab positions (optional)  
 const BYTE * pbInClass, //In Legacy GetCharacterPlacement character classifications (deprecated)  
 SCRIPT_STRING_ANALYSIS * pssa); //Out Analysis of string 

 				hr = ScriptStringOut(
                ssa,
                prc->left,
               *piY,
                ETO_CLIPPED,
               &rc,
                iFrom,   // ScriptStringOut will ignore From/To that are outside this line
                iTo,
                FALSE);

HRESULT WINAPI ScriptStringOut(  
 SCRIPT_STRING_ANALYSIS  ssa, //In Analysis with glyphs  
 int  iX, //In   
 int  iY, //In   
 UINT  uOptions, //In ExtTextOut options  
 const RECT * prc, //In Clipping rectangle (iff ETO_CLIPPED)  
 int  iMinSel, //In Logical selection. Set iMinSel>=iMaxSel for no selection  
 int  iMaxSel, //In   
 BOOL  fDisabled); //In If disabled, only the background is highlighted. 

ScriptStringCPtoX(ssa, g_iCurChar - iFirst, FALSE, &g_iCaretX);

HRESULT WINAPI ScriptStringCPtoX(  
 SCRIPT_STRING_ANALYSIS  ssa, //In String analysis  
 int  icp, //In Caret character position  
 BOOL  fTrailing, //In Which edge of icp  
 int * pX); //Out Corresponding x offset 
*/

// --------------------------------------------------------------------------

BOOL bNextKeyDef( Str8& sKeyDefs, int& iKeyDef, Str8& sMatch, Str8& sOutput ) // 1.5.0ff Find next key def
	{
	sMatch = "";
	sOutput = "";
	iKeyDef = sKeyDefs.Find( "\n", iKeyDef ); // 1.5.0ff Assumes nl at front, this is done by cleanup
	if ( iKeyDef < 0 ) // 1.5.0ff If no more nl found, no more key defs, return false
		return FALSE;
	iKeyDef++; // 1.5.0ff Move past nl
	int iKeyDefEnd = sKeyDefs.Find( "\n", iKeyDef ); // 1.5.0ff Find end of current def, assumes nl at end, this is done by cleanup
	Str8 sKeyDef = sKeyDefs.Mid( iKeyDef, iKeyDefEnd - iKeyDef );
	int iMatchEnd = sKeyDef.Find( " " );
	if ( iMatchEnd <= 0 ) // 1.5.0ff If no space found, or space at front, not a good def, but not the end of the list of defs
		return TRUE;
	sMatch = sKeyDef.Left( iMatchEnd ); // 1.5.0ff Collect match
	if ( sMatch.GetLength() == 0 ) // 1.5.0ff If nothing left in match, not a good def
		return TRUE;
	int iOutputStart = iMatchEnd + 1;
	int iOutputEnd = sKeyDef.Find( " ", iOutputStart );
	if ( iOutputEnd < 0 ) // 1.5.0ff If no space found to end output, end it at end of keydef
		iOutputEnd = sKeyDef.GetLength();
	sOutput = sKeyDef.Mid( iOutputStart, iOutputEnd ); // 1.5.0ff Collect output
	if ( sOutput.GetLength() == 0 ) // 1.5.0ff If no output, return empty match as well
		sMatch = "";
	sMatch.Replace( "space", " " ); // 1.5.1ha Allow space to be defined in keydefs
	sOutput.Replace( "space", " " ); // 1.5.1ha Allow space to be defined in keydefs
	return TRUE;
	}

Str8 sDead; // 1.5.0ff Remember dead key

BOOL bKeyDefSubstitute( Str8& sKeyDefs, Str8& strChar ) // 1.5md Encapsulate key def substitution
	{
	int iKeyDef = 0; // 1.5.0ff 
	Str8 sMatch; // 1.5.0ff 
	Str8 sOutput; // 1.5.0ff 
	while ( bNextKeyDef( sKeyDefs, iKeyDef, sMatch, sOutput ) ) // 1.5.0ff 
		{
		if ( sMatch.GetLength() == 0 ) // 1.5.0ff Don't try to compare empty string
			continue;
		if ( strChar == sMatch ) // 1.5.0ff If key matches // 1.5md Include dead key in match
			{
			if ( sOutput == "Dead" ) // 1.5.0ff If dead key, remember it
				{
				sDead = strChar; // 1.5.0ff Return dead key
				strChar = ""; // 1.5.0ff Return empty char to make key act as dead 
				}
			else
				strChar = sOutput; // 1.5.0ff Return output chars
			return TRUE; // 1.5md 
			}
		}
	return FALSE; // 1.5md 
	}

void CLangEnc::ApplyKeyDefs( Str8 &strChar ) // 1.5.0ff Implement key defs
	{
	if ( !m_bUseInternalKeyboard ) // 1.5.1mc Don't apply keydefs if turned off
		return;
	if ( strChar.GetLength() > 1 || strChar.GetLength() == 0 ) // 1.5.0ff Don't try if it is multiple bytes (UTF8)
		return; // 1.5.0ff 
	Str8 strCharOrig = strChar; // 1.5me 
	BOOL bDead = ( strChar.GetLength() > 0 ); // 1.5me 
	strChar = sDead + strChar; // 1.5md Prepend possible dead key to char
	sDead = ""; // 1.5md Clear dead
	if ( bKeyDefSubstitute( m_sKeyDefs, strChar ) || !bDead ) // 1.5md 
		return; // 1.5md 
	strChar = strCharOrig; // 1.5me Restore original without dead
	bKeyDefSubstitute( m_sKeyDefs, strChar ); // 1.5me If dead combo no match, do plain char
	}

void CLangEnc::SetKeyboard(CKeyboard* pkbd)
{
    m_pkbd = pkbd;
    ClearPropertiesDlgKeyboard();
}

CKeyboard* CLangEnc::pkbdPropertiesDlg() const
{
    return m_bPropertiesDlgKeyboard ? m_pkbdPropertiesDlg : m_pkbd;
}

void CLangEnc::SetPropertiesDlgKeyboard(CKeyboard* pkbd)
{
    m_pkbdPropertiesDlg = pkbd;
    m_bPropertiesDlgKeyboard = TRUE;
}

void CLangEnc::ClearPropertiesDlgKeyboard()
{
    m_bPropertiesDlgKeyboard = FALSE;
    m_pkbdPropertiesDlg = NULL;
}

// Cannot inline these because they contain forward references to member data
const Str8& CLangEnc::sName() const
{
    return m_plrxMyProxy->sName();
}

CLangEncSet* CLangEnc::plngsetMyOwner() const
{
    return m_plrxMyProxy->plngsetMyOwner();
}

Str8 CLangEnc::sFontName() const // 1.4ywp 
	{
    CFontDef fntdef( &m_fnt, m_rgbColor ); // 1.4ywp 
	LOGFONT* plf = fntdef.plf(); // 1.4ywp 
	CString sw = (*plf).lfFaceName; // 1.4ywp 
	Str8 sName = sUTF8( sw ); // 1.4ywp 
	return sName; // 1.4ywp 
	}

void CLangEnc::IncrementNumberOfRefs()
{
    m_plrxMyProxy->IncrementNumberOfRefs();
}

void CLangEnc::DecrementNumberOfRefs()
{
    m_plrxMyProxy->DecrementNumberOfRefs();
}


BOOL CLangEnc::bSetCase(const char* pszCase, BOOL bReading, CNote** ppnot)
{
    BOOL bSet = bSetCase(pszCase, ppnot);
    if ( bSet || bReading )
        // Note: If this case setting has been read from the .lng file,
        // it must be remembered even if it contains problems.
        m_sCase = pszCase;  
    else
        {
        // If this case setting has been entered via the user interface,
        // then restore the characters' original case associations
        // so that the user can Cancel, if desired.
        // Note: If the .lng file had been changed outside Shoebox,
        // it is possible to have problems even with the original setting.
        CNote* pnot = NULL;
        if ( !bSetCase(m_sCase, &pnot) )
            delete pnot;
        }
    
    return bSet;
}

BOOL CLangEnc::bSetCase(const char* pszCase, CNote** ppnot)
{
    ClearCase();

    const char* psz = pszCase;
    ASSERT( psz );
    (void) Shw_bMatchWhiteSpaceAt(&psz);
    while ( *psz )
        {
        ASSERT( !Shw_bAtWhiteSpace(psz) );
        const char* pszUpper = psz;
        CMChar* pmchUpper = NULL;
        if ( !bMatchMetaMCharAt_AddIfNew(&psz, &pmchUpper, ppnot) )
            {
            (*ppnot)->SetStringContainingReferent(pszCase);
            // ADD: In case associations of language encoding:
            return FALSE;
            }
        ASSERT( pszUpper < psz );
        Length lenUpper = psz - pszUpper;

        CMChar* pmchLower = NULL;
        (void) Shw_bMatchWhiteSpaceNoNLAt(&psz);
        while ( *psz && !Shw_bMatchNewLineAt(&psz) )
            {
            BOOL bFirstLower = !pmchLower;
            const char* pszLower = psz;         
            if ( !bMatchMetaMCharAt_AddIfNew(&psz, &pmchLower, ppnot) )
                {
                (*ppnot)->SetStringContainingReferent(pszCase);
                // ADD: In case associations of language encoding:
                return FALSE;
                }
            ASSERT( pszLower < psz );
            Length lenLower = psz - pszLower;
                
            if ( bFirstLower && !pmchUpper->bSetUpperCase(pmchLower) )
                {
                *ppnot = new CNote(_("This character's case is already defined:"), pmchUpper->sMChar(), pszUpper, lenUpper, pszCase);
                return FALSE;
                }
                
            if ( !pmchLower->bSetLowerCase(pmchUpper) )
                {
                *ppnot = new CNote(_("This character's case is already defined:"), // 1.5.0ft 
                    pmchLower->sMChar(), pszLower, lenLower, pszCase);
                return FALSE;
                }
            (void) Shw_bMatchWhiteSpaceNoNLAt(&psz);
            }
            
        if ( !pmchLower )
            {
            *ppnot = new CNote(_("Expecting a lower case association for this character:"), pmchUpper->sMChar(), pszUpper+lenUpper, 0, pszCase);
            return FALSE;
            }
        (void) Shw_bMatchWhiteSpaceAt(&psz);  // 2000-07-12 MRP: newlines too
        // The assertion at the beginning of the body of this main loop
        // failed if there was a line consisting only of space(s).
        // The correct logic is to match (i.e., skip) all white space.
        }
        
    // Set the "Exact disregarding case" of every sort order
    CSortOrder* psrt = m_psrtset->psrtFirst();
    for ( ; psrt; psrt = m_psrtset->psrtNext(psrt) )
        psrt->SetOrderDisregardingCase();

    return TRUE;
}

void CLangEnc::ClearCase()
{
    CMChar* pmch = pmchFirst();
    for ( ; pmch; pmch = pmchNext(pmch) )
        pmch->ClearCase();
}




const CMCharOrder* CLangEnc::pmchordDefault(const CMChar* pmch) const
{
    return psrtDefault()->pmchord(pmch);
}




CMChar* CLangEnc::pmchSearch_AddIfNew( const char* pszMChar )
{
    CMChar* pmch = pmchSearch(pszMChar);
    if ( !pmch )
        pmch = pmchAdd(pszMChar);

    return pmch;
}


CMChar* CLangEnc::pmchAdd( const char* pszMChar )
{
    ASSERT( !pmchSearch(pszMChar) );
    CMChar* pmch = new CMChar(pszMChar, m_uchNext++, this);
    Add(pmch);
    
    return pmch;
}


BOOL CLangEnc::bMChar(const char** ppsz, const CMChar** ppmch) const
{
    ASSERT( **ppsz != '\0' );
//  if ( **ppsz == '\0' )
//      return FALSE;
        
    const CMChar* pmch = (CMChar*) pselSearchLongest(ppsz);
    BOOL bDefined = ( pmch != NULL );
    if ( !bDefined )
        {
		if ( m_bUnicodeLang ) // 1.4qmf If Unicode lang, move past unicode char on undefined
			*ppsz += iCharNumBytes( *ppsz ); // 1.4qmf
		else
	        *ppsz += 1;  // consider current char to be "undefined"
        pmch = m_pmchUndefined;  // 1995-03-31 MRP: Double-check attributes!!!
        }
        
    *ppmch = pmch;
    return bDefined;  // 1995-04-26 MRP
}

const CMChar* CLangEnc::pmchNextSortingUnit(const char** ppchr) const
{
    ASSERT( ppchr );
    ASSERT( **ppchr );
    const CMChar* pmch = (CMChar*)pselSearchLongest(ppchr);
    if ( !pmch )
        {
        *ppchr += 1;  // consider current char to be "undefined"
        pmch = m_pmchUndefined;  // 1995-03-31 MRP: Double-check its attributes!!!
        }
        
    return pmch;
}

void CLangEnc::NextEditingUnit(const Str8& s, int* poff) const
{
    const char* pszString = (const char*)s;
    const char* psz = pszString;
    ASSERT( poff );
    psz += *poff;
    ASSERT( *psz );  // Don't call this function when at the end
	if ( m_bUnicodeLang )
		*poff += iCharNumBytes( psz );
    else if ( m_bSingleChar )
        *poff += 1;
    else
        {
        const CMChar* pmch = NULL;
#ifdef LNG_DOUBLE_BYTE
        if ( m_bDoubleByteChar )
            psz = AnsiNext(psz);  // \DBCS and actually running Far East Windows
        else
#endif
            bMChar(&psz, &pmch);  // \MultiChar
        *poff = psz - pszString;
        }
}

void CLangEnc::PrevEditingUnit(const Str8& s, int* poff) const
{
    ASSERT( poff );
    ASSERT( *poff != 0 );  // Don't call this function when at the beginning
    if ( m_bSingleChar & !m_bUnicodeLang ) // 1.4vyu Fix U bug of not conc start to limiting to start
        *poff -= 1;
    else
        {
        const char* pszString = (const char*)s;
        const char* psz = pszString;
        psz += *poff;
        const CMChar* pmch = NULL;
#ifdef LNG_DOUBLE_BYTE
        if ( m_bDoubleByteChar )
            psz = AnsiPrev(pszString, psz);  // \DBCS and running Far East Windows
        else
#endif
            {
            const char* pszPrev = pszString;
            const char* pszNext = pszPrev;
            bMChar(&pszNext, &pmch);  // \MultiChar
            while ( pszNext < psz )
                {
                pszPrev = pszNext;
                bMChar(&pszNext, &pmch);
                }
            psz = pszPrev;
            }
        *poff = psz - pszString;
        }
}

bool CLangEnc::bNextWord( Str8 sLine, Str8& sWord, int& iPos ) const // Find next space delimited word, return in sWord, return end of word in iPos
	{
	if ( iPos >= sLine.GetLength() ) // If iPos past end, return false
		return false;
	while ( iPos < sLine.GetLength() )
		{
		const char* psz = (const char*)sLine + iPos; // 1.4teb 
		int iLen = iWdSep( psz ); // // 1.4teb See if word separator
		if ( iLen > 0 )
			iPos += iLen;
		else
			break;
		}
	if ( iPos == sLine.GetLength() ) // If no more non-spaces, return false
		return false;
	int iStart = iPos;
	while ( iPos < sLine.GetLength() )
		{
		const char* psz = (const char*)sLine + iPos; // 1.4teb 
		int iLen = iWdChar( psz ); // // 1.4teb See if word separator
		if ( iLen > 0 )
			iPos += iLen;
		else
			break;
		}
	sWord = sLine.Mid( iStart, iPos - iStart ); // Collect word in return value
	return true;
	}

Str8 CLangEnc::sLowerCase( Str8 s ) // 1.4tgk Return lower case equivalent of string
	{
	Str8 sLower;
	const char* pszCase = s;
	const CMChar* pmch = NULL;
	while ( *pszCase ) // For every multichar in string // 1.4vzb 
		{
		const char* pszBefore = pszCase; // 1.4wk Fix U bug of conc not always showing correct word
		if ( bMChar(&pszCase, &pmch) ) // 1.4vzb Move into loop to fix bug of stopping at non word separator // 1.4wg Fix U bug of conc wrong if accented char before in field
			{
			if ( pmch->bUpperCase() && pmch->bHasOtherCase() ) // If upper case and has other case
				pmch = pmch->pmchOtherCase(); // Change to lower case
			sLower += pmch->sMChar(); // Add resulting multichar to converted string
			}
		else // 1.4wg 
			{
			if ( pszCase == pszBefore ) // 1.4wk 
				pszCase++; // 1.4wk 
			while ( pszBefore < pszCase ) // 1.4wk 
				{
				sLower += *pszBefore; // 1.4wg  // 1.4wk 
				pszBefore++; // 1.4wg  // 1.4wk 
				}
			}
		}
	return sLower; // 1.4tgk Return lower case equivalent
	}

// ----------------------------------------------------------------------------
#ifdef ESCES
// ESCES: External Scripture Checking Exchange Standard

BOOL CLangEnc::bNextWord(const char** ppchr, TLength* plenWord) const
{
    ASSERT( ppchr );
    const char* pchr = *ppchr;
    ASSERT( pchr );
    const char* pchrWordB = pchr;

    // Move past non-word characters to the next word's beginning
    while ( *pchr && !pmchNextSortingUnit(&pchr)->bWordChar() )
        pchrWordB = pchr;

    if ( !*pchrWordB )
        return FALSE;

    // Move to the end of the word (actually just past it)
    pchr = pchrWordB;
    const char* pchrWordE = pchr;
    while( *pchr && pmchNextSortingUnit(&pchr)->bWordChar() )
        pchrWordE = pchr;

    *ppchr = pchrWordB;
    ASSERT( plenWord );
    *plenWord = pchrWordE - pchrWordB;
    return TRUE;
}

int CLangEnc::iCompareStrings(const char* pchrA,
        const char* pchrB) const
{
    return psrtDefault()->iCompareStrings(pchrA, pchrB);
}

BOOL CLangEnc::bMatchStrings(const char* pchrA,
        const char* pchrB,
        BOOL bDistinguishCase, BOOL bWhole) const
{
    return psrtDefault()->bMatchStrings(pchrA, pchrB,
        bDistinguishCase, bWhole);
}

int CLangEnc::iCompareSubstrings(
        const char* pchrA, TLength lenA,
        const char* pchrB, TLength lenB) const
{
    return psrtDefault()->iCompareSubstrings(pchrA, lenA, pchrB, lenB);
}

BOOL CLangEnc::bMatchSubstrings(
        const char* pchrA, TLength lenA,
        const char* pchrB, TLength lenB,
        BOOL bDistinguishCase, BOOL bWhole) const
{
    return psrtDefault()->bMatchSubstrings(pchrA, lenA, pchrB, lenB,
        bDistinguishCase, bWhole);
}

int CLangEnc::iCompareSortKeys(const LngSortCode* pcodA,
        const LngSortCode* pcodB) const
{
    return psrtDefault()->iCompareSortKeys(pcodA, pcodB);
}

BOOL CLangEnc::bMatchSortKeys(const LngSortCode* pcodA,
        const LngSortCode* pcodB,
        BOOL bDistinguishCase, BOOL bWhole) const
{
    return psrtDefault()->bMatchSortKeys(pcodA, pcodB,
        bDistinguishCase, bWhole);
}

TLength CLangEnc::lenMakeSortKey(const char* pchr, TLength len,
        LngSortCode* pcod, TLength maxlenKey) const
{
    return psrtDefault()->lenMakeSortKey(pchr, len, pcod, maxlenKey);
}

TLength CLangEnc::lenCopySortKey(const LngSortCode* pcodFrom,
        LngSortCode* pcodTo, TLength maxlenKeyTo) const
{
    return CSortOrder::s_lenCopySortKey(pcodFrom, pcodTo, maxlenKeyTo);
}

#endif
// ----------------------------------------------------------------------------

BOOL CLangEnc::bInPuncList( const char* psz, int& iNumBytes ) const // 1.4zbc Return 0 if no punct list, or first char not in punc list, return num bytes if first char is in punc list
	{
	iNumBytes = 1; // Set number of bytes in first char
	if ( bUnicodeLang() ) // If Unicode lng, find out how many bytes in first char
		iNumBytes = iCharNumBytes( psz );
	if ( sPunct() == "" ) // If no punc list, return false
		return FALSE; 
	Str8 sChar = Str8( psz, iNumBytes ); // Make string of first char
	if ( sPunct().Find( sChar ) > -1 ) // If found in punct, return true
		return TRUE;
	return FALSE; // If not found in punct, return false
	}

#ifdef LATER // 1.4zbc 
BOOL CLangEnc::bIsWdChar(const char* psz, int& iNumBytes ) const  // Return true if word char, else return false, return number of bytes in next char in iNumBytes
	{
	}

BOOL CLangEnc::bIsWdSep(const char* psz, int& iNumBytes ) const  // Return true if word separator, else return false, return number of bytes in next char in iNumBytes
	{
	}
#endif // LATER

BOOL CLangEnc::bIsWdCharMove(const char** ppsz) const  // Return true and advance pointer if word char, else return false and don't advance pointer
{
    const char* pszStart = *ppsz; // 1.4qmf
    const char* psz = *ppsz;
    if ( !*psz || *psz == ' ' || *psz == '\n' )  // If null or space it is not word char // 1.4whc Fix U bug of double click selecting two words across nl
        return FALSE;
	if ( sPunct() != "" ) // 1.4zbc If punctuation list, and char is not in the list, it is word char
		{
		int iNumBytes = 1; // Number of bytes, to be set by bInPuncList
		BOOL bPunc = bInPuncList( psz, iNumBytes ); // 1.4zbc Find number of bytes, and if in punc list
		if ( !bPunc ) // 1.4zbc If not punc, move and return TRUE
			{
			*ppsz += iNumBytes; // 1.4zbc Move forward number of bytes
			return TRUE; // 1.4zbc Return false if in punc list, true if not
			}
		else
			return FALSE;
		}
    const CMChar* pmch = NULL;;
    BOOL bDefined = bMChar( &psz, &pmch ); // Pick up one character, may be multiple bytes, especially in Unicode // 1.4whc 
	if ( !bDefined ) // 1.4whc If not in char list, it is not word char
		{
		*ppsz++; // 1.4whc Move forward one char
		return FALSE; // 1.4whc 
		}
    if ( !pmchordDefault(pmch)->bUndefined() )  // If in default sort order, it is word char
        {
        *ppsz = psz;
        return TRUE;
        }
    return FALSE;
}

BOOL CLangEnc::bIsWdSepMove(const char** ppsz) const  // Return true and advance pointer if space, or if not in the default sort order, always advance pointer
{
    const char* pszStart = *ppsz; // 1.4qmf
    const char* psz = *ppsz;
    if ( !*psz )  // If null, don't move
        return FALSE;
    if ( *psz == ' ' || *psz == '\n' || *psz == '\r' )  // If space or nl it is word sep // 1.5.3d Fix bug of spell check fail if punc at end of line
        {
        *ppsz += 1;
        return TRUE;
        }
	if ( sPunct() != "" ) // 1.4zbc If punctuation list, and char is not in the list, it is word char
		{
		int iNumBytes = 1; // Number of bytes, to be set by bInPuncList
		BOOL bPunc = bInPuncList( psz, iNumBytes ); // 1.4zbc Find number of bytes, and if in punc list
		*ppsz += iNumBytes; // 1.4zbc Move forward number of bytes
		return bPunc; // 1.4zbc Return true if in punc list, false if not
		}
    const CMChar* pmch = NULL;;
    BOOL bWordChar = bMChar( &psz, &pmch ); // Pick up one character, may be multiple bytes, especially in Unicode // 1.4wn 
	*ppsz = psz; // 1.4wn Fix U bug of possible hang on conc request
    if ( pmchordDefault(pmch)->bUndefined() )  // If not in default sort order, it is sep
        return TRUE;
    return FALSE;
}

// Trim leading and trailing whitespace
Str8 CLangEnc::sTrimWhitespace( const char* psz ) const
{
    int iLen;
    psz = pszSkipWhite( psz );
    if ( *psz )
        {
        const char* pszLastNonWhite = psz;
        for (const char* pszTemp = psz + 1; *pszTemp; pszTemp++)
            {
            if ( bIsNonWhite(pszTemp) )
                pszLastNonWhite = pszTemp;
            }
        iLen = pszLastNonWhite - psz + 1;
        }
    else
        iLen = 0;

    Str8 s(psz, iLen);
    return s;
}

// RNE 1996-03-04  major changes.  Metacharacters are surrounded now by spaces, not chevrons.
BOOL CLangEnc::s_bMatchMetaMCharAt(const char** ppsz, Str8& sMChar,
        CNote** ppnot)
{
    ASSERT( ppsz );
    const char* psz = *ppsz;
    ASSERT( psz );
#if 1
    int idxMChar = 0;
    char caTmp[2];
    caTmp[1] = 0;
    // Metacharacters are terminated by spaces (includes tab, newline).
    
    while (*psz && !Shw_bAtWhiteSpace(psz)) // RNE NOTE NOTE: Is this the right way to match white space???
        {
        caTmp[0] = *psz;
        sMChar += caTmp;
        psz += 1;
        }
        
    if (sMChar.IsEmpty())
        return FALSE;

#else   
    if ( Shw_bAtLeftMeta(psz) )
        // A multi-byte character enclosed in chevrons, e.g. "<<Ng>>"
        {
        if ( !Shw_bMatchMetaStringAt(&psz, sMChar, ppnot) )
            return FALSE;
            
        if ( bEqual(sMChar, "space") )
            sMChar = " ";  // special in Sh2 sort orders
        }
    else
        // A single character, e.g. "n"
        {
        ASSERT( *psz );
        ASSERT( !Shw_bAtWhiteSpace(psz) );
        sMChar = *psz;
        psz += 1;
        }
#endif
    *ppsz = psz;    
    return TRUE;
}

BOOL CLangEnc::bMatchMetaMCharAt_AddIfNew(const char** ppsz,
        CMChar** ppmch, CNote** ppnot)
{
    Str8 sMChar;
    if ( !s_bMatchMetaMCharAt(ppsz, sMChar, ppnot) )
        return FALSE;
        
    *ppmch = pmchSearch_AddIfNew(sMChar);
    return TRUE;
}

BOOL CLangEnc::bModifyProperties(const char* pszName,
        const char* pszDescription, const char* pszCase, const char* pszPunct, // 1.4qb
        CSortOrder* psrtDefaultOriginal,
        CNote** ppnot)
{
    // NOTE NOTE NOTE: We must construct the update notification
    // *before* changing any member data, but must wait to use it
    // until *after* the modifications have been committed.
    CLangEncUpdate lup(this, pszName, pszCase, pszPunct, psrtDefaultOriginal);
    
    if ( !bSetProperties(pszName, pszDescription, pszCase, pszPunct, ppnot) )
        return FALSE;

    // If the name is being changed, update it
    ASSERT( pszName );
    if ( !bEqual(pszName, sName()) )
        {
        int nClosedDBTypes = 0;
        CDatabaseTypePtr* aptyp;
		CDatabaseTypeProxy* ptrx = Shw_ptypset()->ptrxFirst();
        for ( ; ptrx; ptrx = Shw_ptypset()->ptrxNext(ptrx) )
            {
            if ( !ptrx->bOpen() )
                nClosedDBTypes++;
            }
        if ( nClosedDBTypes )
            {
            aptyp = new CDatabaseTypePtr[nClosedDBTypes];
            int i = 0;
            for ( ptrx = Shw_ptypset()->ptrxFirst(); ptrx; ptrx = Shw_ptypset()->ptrxNext(ptrx) )
                if ( !ptrx->bOpen() )
                    {
                    CNoteList notlst;
                    aptyp[i++] = ptrx->ptyp(notlst); // Hold a pointer so the database type number of references won't go to zero
                    }
            }

        m_plrxMyProxy->ChangeNameTo(pszName);

        // Need to save any "unused" DBTypes in case they refer to this renamed language encoding
        if ( nClosedDBTypes )
            delete [] aptyp; // Delete all the temporary type pointers.
                             // This causes their destructors to be called, which results in their getting written.
        // Need to save all typ files that ARE in use as well. This keeps stuff from getting out of synch if Shoebox were to crash
        Shw_ptypset()->WriteAllModified(TRUE);
        // Need to write this .lng file also.
        WritePropertiesIfModified(TRUE);
        }

    extern void Shw_Update(CUpdate& up);        
    Shw_Update(lup);  // Notify other objects that this language is modified
    
#ifdef ToolbarFindCombo // 1.4yq
    Shw_pcboFind()->LangEncUpdated(lup); // update find combo directly
        // needs to happen even if no docs or types or anything open
#endif // ToolbarFindCombo // 1.4yq
    
    return TRUE;
}

BOOL CLangEnc::bSetProperties(const char* pszName,
        const char* pszDescription, const char* pszCase, const char* pszPunct, // 1.4qb
		CNote** ppnot)
{
    // If the name is being changed, verify that it will be valid
    ASSERT( pszName );
    BOOL bUpdatingName = !bEqual(pszName, sName()) || sName().IsEmpty();
    if ( bUpdatingName && !plngsetMyOwner()->bValidNewName(pszName, ppnot) )
        {
        (*ppnot)->SetStringContainingReferent(pszName);
        return FALSE;
        }
        
    if ( !bSetCase(pszCase, FALSE, ppnot) )
        return FALSE;
	SetPunct( pszPunct ); // 1.4qb

    ASSERT( pszDescription );
    m_sDescription = pszDescription;

    // If the name is changing and no sort order has the new name,
    // ask whether to change the name of the default sort order
    // to agree with that of the language.
    if ( bUpdatingName && !m_psrtset->psrtSearch(pszName) )
        {
        psrtDefault()->ChangeNameTo(pszName);
        }
    return TRUE;
}

void CLangEnc::Update(CUpdate& up)
{
    up.UpdateLangEnc(this);
}

void CLangEnc::LangEncUpdated(CLangEncUpdate& lup)
{
    m_bModified = m_bModified || lup.plng() == this;
}

BOOL CLangEnc::bViewProperties(int iActivePage)
{
    CLangEncSheet dlg(this, FALSE, iActivePage);
    return ( dlg.DoModal() == IDOK );
}

BOOL CLangEnc::bViewProperties(int iActivePage, CSortOrder* psrt, CVarInstance* pvin)
{
    CLangEncSheet dlg(this, FALSE, iActivePage, psrt, pvin);
    return ( dlg.DoModal() == IDOK );
}


#ifdef _DEBUG
void CLangEnc::AssertValid() const
{
    m_psrtset->AssertValid();
}
#endif  // _DEBUG


CCharSetListBox::CCharSetListBox(CLangEnc* plng, CMChar** ppmch) :
    CSetListBox(plng, (CSetEl**)ppmch)
{
    m_plng = plng;
    ASSERT( m_plng );
    
    // Because the font that Windows provides by default lacks many
    // standard upper ANSI glyphs, use Times New Roman instead.
#ifdef _MAC
    CFontDef fntdef("Times", 10);
#else
    CFontDef fntdef("Times New Roman", 13); // 10 point (TLB: this is 10 pt at 96 dpi, but it is actually character height in pixels)
#endif
    (void) fntdef.iCreateFont(&m_fntStandard);
}

void CCharSetListBox::InitLabels()
{
    m_xChar = xSubItem_Show(IDC_lng_lblChar);
    m_xOtherCase = xSubItem_Show(IDC_lng_lblOtherCase);
    m_xDec = xSubItem_Show(IDC_lng_lblDec);
    m_xHex = xSubItem_Show(IDC_lng_lblHex);
    m_xStandardChar = xSubItem_Show(IDC_lng_lblStandardChar);
}

int CCharSetListBox::iItemHeight()
{
    return Shw_pProject()->iLangCtrlHeight();
}

static const int s_maxlenChar = 8;
static char s_pszChar[2 + s_maxlenChar + 1] = { ' ', ' ', '\0' };

void CCharSetListBox::DrawElement(CDC& cDC, const RECT& rcItem,
            const CDblListEl *pel)
{
    CMChar* pmch = (CMChar*)pel;
    
    const char* pszChar = pmch->sMChar();
    const char* pszCharDraw = pmch->sMCharDraw();
    DrawSubItemJustify(cDC, rcItem, m_xChar, m_xOtherCase, pszCharDraw, m_plng);
    
    CMChar* pmchOtherCase = pmch->pmchOtherCase();
    const char* pszOtherCase = ""; // 1.4vng 
	if ( pmchOtherCase ) // 1.4vng 
		pszOtherCase = pmchOtherCase->sMCharDraw(); // 1.4vng 

    // 1998-02-13 MRP: Change from pfntPropertiesDlg to m_plng
    // to allow context-sensitive rendering and right-to-left scripts.
    // Here's a problem: When a font is chosen on the Options page
    // of Language Encoding Properties, these list boxes must use it.
    // Before, we distinguished pfntDlg from pfngPropertiesDlg,
    // but that won't work in this new approach. Instead,
    // we'll consider the font choice to be un-Cancel-able.
    DrawSubItemJustify(cDC, rcItem, m_xOtherCase, m_xDec, pszOtherCase, m_plng);

    char pszDec[4];  // Enough for a decimal unsigned char: 0-255
    sprintf_s(pszDec, "%-3d", (unsigned char)*pszChar);
    DrawSubItemJustify(cDC, rcItem, m_xDec, m_xHex, pszDec);

    char pszHex[3];  // Enough for a hexadecimal unsigned char: 00-FF
    sprintf_s(pszHex, "%02x", (unsigned char)*pszChar);
    (void) _strupr_s(pszHex, sizeof(pszHex));
    DrawSubItemJustify(cDC, rcItem, m_xHex, m_xStandardChar, pszHex);

    // Here's a slight problem: the default system font on both
    // Windows and Mac doesn't show all upper characters (who knows why not).
    // Before, we explicitly selected a font that did show them all.
    // Now that we're using languages, we'll use the default language.
    CLangEnc* plngDefault = m_plng->plngsetMyOwner()->plngDefault();
    DrawSubItemJustify(cDC, rcItem, m_xStandardChar, 0, pszChar, plngDefault);
}

// --------------------------------------------------------------------------

void CLangEncPtr::IncrementNumberOfRefs(CLangEnc* plng)
{
    m_plng = plng;
    if ( m_plng )
        m_plng->IncrementNumberOfRefs();
}

void CLangEncPtr::DecrementNumberOfRefs()
{
    if ( m_plng )
        m_plng->DecrementNumberOfRefs();
}


const CLangEncPtr& CLangEncPtr::operator=(CLangEnc* plng)
{
    if ( m_plng != plng )
        {
        DecrementNumberOfRefs();
        IncrementNumberOfRefs(plng);
        }
        
    return *this;
}


// --------------------------------------------------------------------------

CLangEncUpdate::CLangEncUpdate(CLangEnc* plng)
{
    m_plng = plng;
    ASSERT( m_plng );
    m_bModified = FALSE;
    m_bNameModified = FALSE;
    m_bDefaultSortOrderChanged = FALSE;
}

CLangEncUpdate::CLangEncUpdate(CLangEnc* plng, const char* pszName, const char* pszPunct, // 1.4qb
        const char* pszCase, CSortOrder* psrtDefaultOriginal)
{
    m_plng = plng;
    ASSERT( m_plng );
    m_bModified = TRUE;
    m_bNameModified = ( pszName != m_plng->sName() );
    ASSERT( psrtDefaultOriginal );
    m_bDefaultSortOrderChanged = ( psrtDefaultOriginal != m_plng->psrtDefault() );
}


// --------------------------------------------------------------------------

CLangEncProxy::CLangEncProxy(const char* pszName,
        const char* pszFileName, CLangEncSet* plngsetMyOwner) :
    CSetEl(pszName),
    m_plng(NULL),
    m_plngsetMyOwner(plngsetMyOwner)
{
    ASSERT( pszName );
    // NOTE: If a new language encoding is being added, it will not yet
    // have been named; however it must have a valid, unique, non-empty name
    // by the time the proxy is actually added to the language encoding set.
    ASSERT( pszFileName );
    m_sFileName = pszFileName;
    ASSERT( m_plngsetMyOwner );
    m_sVersion = m_plngsetMyOwner->sSettingsVersion();  // 1998-03-12 MRP
}

CLangEncProxy::~CLangEncProxy()
{
    delete m_plng;
    m_plng = NULL;
}

Str8 CLangEncProxy::sFullPathName() const
{
    return ::sPath(plngsetMyOwner()->sSettingsDirPath(), sFileName(),
        psz_LangEncExt);
}

CSet* CLangEncProxy::psetMyOwner()
{
    return m_plngsetMyOwner;
}

void CLangEncProxy::DecrementNumberOfRefs()
{
    ASSERT( m_plng );
    CSetEl::DecrementNumRefs();
    if ( !bHasRefs() )
        {
        m_plng->WritePropertiesIfModified();
        CLangEnc* plng = m_plng; // BJY 4-16-96 Have to use temporary and clear m_plng before
                                // deleting m_plng to avoid compiler bug
        m_plng = NULL;
        delete plng;
        }
}


CLangEnc* CLangEncProxy::plng(CNoteList& notlst)
{
    if ( !m_plng )
        plngNew()->bReadProperties(notlst);
        
    return m_plng;
}

BOOL CLangEncProxy::s_bReadProperties(Object_istream& obs, const char* pszName,
        const char* pszFileName, CLangEncSet* plngsetMyOwner,
        CLangEnc** pplng)
{
    CLangEncProxy* plrxRead = new CLangEncProxy(pszName, pszFileName,
        plngsetMyOwner);
    CLangEnc* plngRead = plrxRead->plngNew();
    
    if ( !plngRead->bReadProperties(obs) )
        {
        delete plrxRead;
        return FALSE;
        }
        
    *pplng = plngRead;
    return TRUE;
}

CLangEnc* CLangEncProxy::s_plngDefaultProperties(const char* pszName,
        const char* pszFileName, CLangEncSet* plngsetMyOwner)
{
    CLangEncProxy* plrxDefault = new CLangEncProxy(pszName, pszFileName,
        plngsetMyOwner);
    CLangEnc* plngDefault = plrxDefault->plngNew();
    plngDefault->SetDefaultProperties();
    
    return plngDefault;
}

CLangEnc* CLangEncProxy::plngNew()
{
    ASSERT( !m_plng );
    m_plng = new CLangEnc(this);
    
    return m_plng;
}


Str8 CLangEncProxy::sPath()
{
    return ::sPath(m_plngsetMyOwner->sSettingsDirPath(),
        m_sFileName, psz_LangEncExt);
}

void CLangEncProxy::WritePropertiesIfModified( BOOL bForceWrite )
{
    if ( m_plng )
        m_plng->WritePropertiesIfModified( bForceWrite );
}

BOOL CLangEncProxy::s_bReadProxy(const char* pszPath, CLangEncSet* plngset,
        CLangEncProxy** pplrx)
{
    ASSERT( pszPath );
    ASSERT( plngset );
    ASSERT( pplrx );
    ASSERT( !*pplrx );  // If it isn't NULL now, there will be a leak.

    std::ifstream ios( pszPath );
    if ( ios.fail() )
        return FALSE;

    Newline_istream iosnl(ios);  // 1997-08-11
    CNoteList notlst;
    Object_istream obs(iosnl, notlst);
    Str8 sName;
    // Make sure the file begins with the proper marker.
    if ( !obs.bAtBackslash() || !obs.bReadBeginMarker(psz_LangEnc, sName) )
        return FALSE;

    Str8 sFileName = sGetFileName(pszPath, FALSE); // don't get extension
    // Str8 sFileName = sGetFileName(pszPath, TRUE); // get extension too
    if ( sName.IsEmpty() )
        sName = sFileName;

    Str8 sVersion = "3.0";  // If no \ver field in the file, assume this
    obs.bReadString(psz_ver, sVersion);  // 1998-03-12 MRP
    
    CLangEncProxy* plrx = new CLangEncProxy(sName, sFileName, plngset);
    plrx->m_sVersion = sVersion;  // MRP: NEED TO REVISE THE CONSTRUCTOR!!!

    *pplrx = plrx;
    return TRUE;
}
    

void CLangEncProxy::Update(CUpdate& up)
{
    if ( m_plng )
        m_plng->Update(up);
}

#ifdef TryOut // 1.6.1cb 
BOOL CLangEncProxy::bCopy(CSetEl** ppselNew)
{
    CNoteList notlst;
    CLangEnc* plngToCopyFrom = plng(notlst);
    // 1996-03-13 MRP: CAN THIS EVER FAIL???

//  strstream ios;
    strstream_d255 ios;
    Object_ostream obsWrite(ios);
    ASSERT( plngToCopyFrom );
    plngToCopyFrom->WriteProperties(obsWrite);

    const Str8& sDirPath = m_plngsetMyOwner->sSettingsDirPath();
    const Str8& sName = plngToCopyFrom->sName();
    Str8 sNewName = m_plngsetMyOwner->sUniqueName(sName);
    Str8 sFileName = sUniqueFileName(sDirPath, sNewName, psz_LangEncExt);
    
    Object_istream obsRead(ios, notlst);
    CLangEnc* plngNew = NULL;
    // NOTE NOTE NOTE: We must temporarily name the new object identical to
    // the one from which we are copying in order to read its properties,
    // but then we rename it to a new unique name.
    if ( !CLangEncProxy::s_bReadProperties(obsRead, sName, sFileName,
            m_plngsetMyOwner, &plngNew) )
        return FALSE;
    ASSERT( plngNew );
    CLangEncProxy* plrxNew = plngNew->plrxMyProxy();
    plrxNew->ChangeNameTo(sNewName);
    
    {
    CLangEncSheet dlg(plngNew, TRUE, CLangEncSheet::eOptionsPage);
    if ( dlg.DoModal() == IDOK )
        {
        *ppselNew = plrxNew;  // Has already been added
        return TRUE;
        }
    }
        
    delete plrxNew;
    return FALSE;
}
#endif // TryOut // 1.6.1cb 

BOOL CLangEncProxy::bModify()
{
    CNoteList notlst;
    CLangEnc* plngView = plng(notlst);
    return plngView->bViewProperties();
}

BOOL CLangEncProxy::bModifyProperties(const char* pszName,
        const char* pszDescription, const char* pszCase, const char* pszPunct, // 1.4qb
        CSortOrder* psrtDefaultOriginal,
        CNote** ppnot)
{
    return TRUE;
}

BOOL CLangEncProxy::bSetProperties(const char* pszName,
        const char* pszDescription, const char* pszCase, const char* pszPunct, // 1.4qb
		CNote** ppnot)
{
    ASSERT( m_plng );
    if ( !m_plng->bSetProperties(pszName, pszDescription, pszCase, pszPunct, ppnot) )
        return FALSE;

    // If the name is being changed, update it
    ASSERT( pszName );
    if ( !bEqual(pszName, sName()) )
        {
        ChangeNameTo(pszName);
        const Str8& sDirPath = m_plngsetMyOwner->sSettingsDirPath();
        Str8 sFileName = sUniqueFileName(sDirPath, pszName, psz_LangEncExt);
        m_sFileName = sFileName;
        }
    
    return TRUE;
}


#ifdef _DEBUG
void CLangEncProxy::AssertValid() const
{
    ASSERT( !m_sFileName.IsEmpty() );
    if ( m_plng )
        m_plng->AssertValid();
}
#endif  // _DEBUG


// --------------------------------------------------------------------------

const char* s_pszVarLetter = "@";
const char* s_pszVarLetterUpper = "U";
const char* s_pszVarLetterLower = "L";
const char* s_pszVarDigit = "D";


CLangEncSet::CLangEncSet(const char* pszSettingsVersion)
{
    ASSERT( pszSettingsVersion );
    m_sSettingsVersion = pszSettingsVersion;  // 1998-03-12 MRP
    // Instantiate default variable names; later define for each encoding
    (void) m_varset.pvarAdd(s_pszVarLetter);
    (void) m_varset.pvarAdd(s_pszVarLetterUpper);
    (void) m_varset.pvarAdd(s_pszVarLetterLower);
    (void) m_varset.pvarAdd(s_pszVarDigit);
    m_bReadFromString = FALSE;
}

#if defined(TEST_BUILD) || defined(_DEBUG)
CLangEncSet::CLangEncSet(const char* pszSettingsVersion,
        const char* pszProperties)
{
    ASSERT( pszSettingsVersion );
    m_sSettingsVersion = pszSettingsVersion;  // 1998-03-12 MRP
    // Instantiate default variable names; later define for each encoding
    (void) m_varset.pvarAdd(s_pszVarLetter);
    (void) m_varset.pvarAdd(s_pszVarLetterUpper);
    (void) m_varset.pvarAdd(s_pszVarLetterLower);
    (void) m_varset.pvarAdd(s_pszVarDigit);
    // Read the properties of zero or more language encodings
    // concatenated together in pszProperties.
    m_bReadFromString = TRUE;   
    ASSERT( pszProperties );
    std::istringstream ios((char*)pszProperties);
    CNoteList notlst;
    Object_istream obs(ios, notlst);
    while ( !obs.bAtEnd() )
        {
        Str8 sName;
        BOOL bName = obs.bAtBeginMarker(psz_LangEnc, sName);
        ASSERT( bName );
        CLangEnc* plng = NULL;
        BOOL bRead = CLangEncProxy::s_bReadProperties(obs, sName, "", this,
            &plng);
        ASSERT( bRead );
        Add(plng->plrxMyProxy());
        }
    ASSERT( obs.bAtEnd() );
    ASSERT( notlst.bIsEmpty() );
}
#endif  // TEST_BUILD or _DEBUG

CLangEncSet::~CLangEncSet()
{
    // 1996-01-30  MRP, RNE  -- This DeleteAll causes the language encodings
    // to be deleted, along with their references to CKeyboard objects
    // before the Keyboard objects get deleted.  Otherwise Keyboard objects
    // may get deleted first as the keyboard set is a member of CLangEncSet.
    // Member variables' destructors get called before the base class destructor,
    // which would have had the destructors being called in the wrong order.
    m_plngDefault = NULL;  // Decrement its reference count
    DeleteAll();
}


void CLangEncSet::ReadAllProxies(CNoteList& notlst)
{
    // Read all language encoding files in the setting directory.
    // It should be an _absolute_ directory path, e.g. "C:\SHW\SETTINGS".
    ASSERT( m_sSettingsDirPath.GetLength() > 0 );

    // Get list of .lng files in settings directory.
    CFileList filelst(m_sSettingsDirPath, psz_LangEncExt);
    CFileListEl* pfle = filelst.pfleFirst();
    for ( ; pfle; pfle = filelst.pfleNext(pfle) )
        {
        CLangEncProxy* plrx = NULL;
		if ( bFileReadOnly(pfle->pszPath()) )
			{
			Shw_papp()->SetWriteProtectedSettings( TRUE );
			UnWriteProtect( pfle->pszPath() ); // 1.2aj Always clear write protect of settings files // 1.4qzhg
			}
		Str8 sPath = pfle->pszPath(); // 1.4vze 
		if ( !bAllASCII( sPath ) ) // 1.4vze 
			continue; // 1.4vze Don't load language encoding files with non-ANSI in file name
        if ( CLangEncProxy::s_bReadProxy(pfle->pszPath(), this, &plrx) )
            Add(plrx);
        }

    m_plngDefault = plngSearch_AddIfNew("Default", notlst);
}

void CLangEncSet::WriteAllModified( BOOL bForceWrite )
{
    CLangEncProxy* plrx = plrxFirst();
    for ( ; plrx; plrx = plrxNext(plrx) )
        plrx->WritePropertiesIfModified(bForceWrite);
}

BOOL CLangEncSet::bReadLangEncRef(Object_istream& obs, const char* pszQualifier,
        CLangEnc*& plng)
{
    Str8 sName;
    if ( !obs.bReadString(psz_lng, pszQualifier, sName) )
        return FALSE;

    // 2000-07-20 MRP: Automatically correct an error in MDF-HTML.typ
    // for users of Shoebox test versions 4.1 through 4.3:
    // The \pn marker had language National instead of national.
    // This causes an unwanted language encoding to be created,
    // especially when older settings files are converted to version 5.0.
    // See CMarker::s_bReadProperties in mkr.cpp.
    extern BOOL g_bReading_MDF_HTML_pn;
    if ( g_bReading_MDF_HTML_pn )
        {
        if ( bEqual(sName, "National") )
            sName = "national";
        }

    if ( !sName.IsEmpty() )  // 1998-03-24 MRP
        plng = plngSearch_AddIfNew(sName, obs.notlst());
    return TRUE;
}


CLangEnc* CLangEncSet::plngSearch_AddIfNew(const char* pszName, CNoteList& notlst)
{
    CLangEnc* plng = NULL;
    CLangEncProxy* plrx = plrxSearch(pszName);
    if ( plrx )
        plng = plrx->plng(notlst);
    else  // Add it automatically to avoid dangling references
        {
        ASSERT( !m_bReadFromString );
        Str8 sFileName = sUniqueFileName(m_sSettingsDirPath, pszName, psz_LangEncExt);
		if ( strcmp( pszName, "Default" ) != 0 ) // 1.5.0eg Fix bug (1.4.zad) of not correctly creating Default lng
			{
			sFileName = "Temporary" + sFileName; // 1.4zac Add Temporary to file name of temporary langauge encoding
			Str8 sMessage = _("Cannot find language encoding:"); // 1.4zad 
			sMessage = sMessage + " " + pszName + "\n" + _("Making temporary one in file:") + " " + sFileName + ".lng"; // 1.5.0fj 
			AfxMessageBox( sMessage ); // 1.4zad Give message if making temporary language encoding
			}
        plng = CLangEncProxy::s_plngDefaultProperties(pszName, sFileName, this);
        AddAndWrite(plng);  // Sends update
        }

    ASSERT( plng );
    return plng;
}


CLangEnc* CLangEncSet::plngSearch(const char* pszName) 
{
	CLangEnc* plng = NULL;
	CLangEncProxy* plrx = plrxSearch(pszName);
	if ( plrx )
		{
		CNoteList notlst;
		plng = plrx->plng(notlst);	// changed to load if needed  4/29/98 BW
		// old way: returns NULL if language encoding not loaded
		//plng = plrx->m_plng; // NULL if not loaded
		}

	return plng;
}


CLangEncProxy* CLangEncSet::plrxSearch(const char* pszName)
{
    ASSERT( pszName );
    return (CLangEncProxy*)pselSearch(pszName);
}

void CLangEncSet::Update(CUpdate& up)
{
    CLangEncProxy* plrx = plrxFirst();
    for ( ; plrx; plrx = plrxNext(plrx) )
        plrx->Update(up);
}

BOOL CLangEncSet::bViewElements(CLangEncProxy* plrxToSelect)
{
    CLangEncSetListBox lboSet(this, &plrxToSelect);
    CLangEncSetDlg dlg(lboSet, this);
    return dlg.DoModal() == IDOK;
}

BOOL CLangEncSet::bViewElements(CLangEnc* plngToSelect)
{
    CLangEncProxy* plrxToSelect = ( plngToSelect ?
        plngToSelect->plrxMyProxy() :
        NULL
        );
        
    return bViewElements(plrxToSelect);
}

BOOL CLangEncSet::bAdd(CSetEl** ppselNew)
{
    Str8 sName = sUniqueName("added");
    Str8 sFileName = sUniqueFileName(m_sSettingsDirPath, sName, psz_LangEncExt);
    CLangEnc* plngNew = CLangEncProxy::s_plngDefaultProperties(sName,
        sFileName, this);
    ASSERT( plngNew );
    CLangEncProxy* plrxNew = plngNew->plrxMyProxy();
    
    {
    CLangEncSheet dlg(plngNew, TRUE, CLangEncSheet::eOptionsPage);
    if ( dlg.DoModal() == IDOK )
        {
        *ppselNew = plrxNew;  // Has already been added
        return TRUE;
        }
    }  // Let dlg be destroyed and decrement reference to plrxNew
    // Perhaps the OnCancel should do that explicitly
        
    delete plrxNew;
    return FALSE;
}

void CLangEncSet::RecalcCtrlFonts() // adjusts fonts used in language controls for all languages in use
{
    CLangEncProxy* plrx = plrxFirst();
    for ( ; plrx; plrx = plrxNext(plrx) )
        if ( plrx->m_plng )
            plrx->m_plng->RecalcCtrlFont();
}

BOOL CLangEncSet::bAddLangEnc(const char* pszName,
        const char* pszDescription, const char* pszCase, const char* pszPunct, // 1.4qb
        CSortOrder* psrtDefaultOriginal,
        CLangEnc* plngToBeAdded, CNote** ppnot)
{
    ASSERT( plngToBeAdded );
    CLangEncProxy* plrxToBeAdded = plngToBeAdded->plrxMyProxy();
    if ( !plrxToBeAdded->bSetProperties(pszName, pszDescription, pszCase, pszPunct, ppnot) ) // 1.4qb
        return FALSE;
        
    AddAndWrite(plngToBeAdded);  // Sends update
    
    return TRUE;
}

void CLangEncSet::Delete(CSetEl** ppsel)
{
    ASSERT( ppsel );
    CLangEncProxy* plrxToBeDeleted = (CLangEncProxy*)*ppsel;

    const Str8& sName = plrxToBeDeleted->sName();
    const Str8& sFileName = plrxToBeDeleted->sFileName();    
    Str8 sFileNameExt = sPath("", sFileName, psz_LangEncExt);
    Str8 sMessage = _("Delete language encoding?"); // 1.5.0fg 
    if ( AfxMessageBox(sMessage, MB_YESNO | MB_ICONQUESTION) != IDYES )
        return;

    Str8 sPathToBeDeleted = sPath(m_sSettingsDirPath,
        sFileName, psz_LangEncExt);
    int iResult = remove(sPathToBeDeleted);
    CSet::Delete(ppsel);
}

void CLangEncSet::AddAndWrite(CLangEnc* plng)
{
    ASSERT( plng );
    CLangEncProxy* plrx = plng->plrxMyProxy();
    ASSERT( plrx->plngsetMyOwner() == this );
    Add(plrx);
        
    CLangEncUpdate lup(plng);
    extern void Shw_Update(CUpdate& up);        
    Shw_Update(lup);  // This causes its modified flag to be set
        
    plrx->WritePropertiesIfModified();  // This clears the flag
}


#ifdef _DEBUG
void CLangEncSet::AssertValid() const
{
    CLangEncProxy* plrx = plrxFirst();
    for ( ; plrx; plrx = plrxNext(plrx) )
        plrx->AssertValid();    
}
#endif  // _DEBUG


// ==========================================================================

CLangEncSetComboBox::CLangEncSetComboBox(CLangEncSet* plngset,
        BOOL bUseAnyItem) :
    CSetComboBox(plngset, bUseAnyItem, NULL, "Any Lang.") // 1.4gh Change set combo box to string instead of ID
{
}
        
void CLangEncSetComboBox::UpdateElements(CLangEnc* plngToSelect)
{
    CLangEncProxy* plrxToSelect = ( plngToSelect ?
        plngToSelect->plrxMyProxy() :
        NULL
        );
    CSetComboBox::UpdateElements(plrxToSelect);
}

CLangEnc* CLangEncSetComboBox::plngSelected()
{
    CLangEncProxy* plrxSelected = (CLangEncProxy*)CSetComboBox::pselSelected();
    CNoteList notlst;
    CLangEnc* plng = ( plrxSelected ?
        plrxSelected->plng(notlst) :
        NULL
        );
    
    return plng;
}

// TLB: 06/08/99 - Added for Jump dlg
void CLangEncSetComboBox::SelectElement(CLangEnc* plngToSelect)
{
    CLangEncProxy* plrxToSelect = ( plngToSelect ? plngToSelect->plrxMyProxy() : NULL );
    CSetComboBox::SelectElement(plrxToSelect);
}

CLangEncSetListBox::CLangEncSetListBox(CLangEncSet* plngset,
        CLangEncProxy** pplrx) :
    CSetListBox(plngset, (CSetEl**)pplrx)
{
}

void CLangEncSetListBox::InitLabels()
{
    m_xName = xSubItem_Show(IDC_SET_NAME);
    m_xFileName = xSubItem_Show(IDC_SET_FILE_NAME);
}

void CLangEncSetListBox::DrawElement(CDC& cDC, const RECT& rcItem,
            const CDblListEl *pel)
{
    CLangEncProxy* plrx = (CLangEncProxy*)pel;
    DrawSubItem(cDC, rcItem, m_xName, m_xFileName, plrx->sName());
    DrawSubItem(cDC, rcItem, m_xFileName, 0, plrx->sFileName());
}

// **************************************************************************
