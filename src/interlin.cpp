// interlin.cpp Interlinear classes Alan Buseman 25 Jan 95

#include "stdafx.h"
#include "toolbox.h"
#include "interlin.h"
#include "recset.h"
#include "shw.h"
#include "shwdoc.h" //?? Maybe restructure later
#include "shwview.h" 
#include "doclist.h"
#include "intprc_d.h"
#include "ambig_d.h"
#include "typ_d.h"
#include "progress.h"
#include "obstream.h"  // Object_istream, Object_ostream
#include "wdf.h"  // 1999-08-31 MRP: CWordFormulaSet
#include <ctype.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

const Str8 CProcNames::sProcName[ eNumProcs ] = { _("Parse"), _("Lookup"), _("Rearrange"), _("Generate"), _("Given") }; // 1.4ah Change to not use string resource id in interlinear 

//-----------------------------------------------------------
// Formula utilities
const char* pszNextForcedGloss( const char* pszLook, char cStart, char cEnd ) // Find next forced gloss
	{ // Move to close parend and then to open, if there is an open immediately following
	const char* pszEnd = strchr( pszLook, cEnd );
	ASSERT( pszEnd ); // Don't call except from inside a forced gloss
	if( *( pszEnd + 1 ) == cStart ) // If another forced gloss immediately, return start of it
		return pszEnd + 2;
	else
		return NULL;
	}

const char* pszEndForcedGlosses( const char* pszLook, char cStart, char cEnd, char cMorphBreak ) // Find next lookup position
	{ // Move to first open parend and then to last close parend in series
	const char* pszEnd = NULL;
    ASSERT( pszLook );  // 1999-10-25 MRP
	const char* pszStart = strchr( pszLook, cStart );
	if ( pszStart ) // If start parend found, look for end
		{
		pszEnd = strchr( pszStart, cEnd );
		if ( !pszEnd ) // If no end parend, go to space or end of line
			{
			pszEnd = strchr( pszStart, ' ' );
			if ( !pszEnd )
				pszEnd = pszStart + strlen( pszStart );
			return pszEnd;
			}
		while( pszEnd && *( pszEnd + 1 ) == cStart ) // If end parend found, see if another set immediately
			pszEnd = strchr( pszEnd + 1, cEnd );
		if ( pszEnd ) // If end parend found, move past it
			pszEnd++; // Move past close parend
		}
	return pszEnd;
	}

const char* pszNextLookupPos( const char* pszLook, char cStart, char cEnd, char cMorphBreak ) // Find next lookup position
	{ // Move to first open parend and then to last close parend in series
    ASSERT( pszLook );  // 1999-10-25 MRP
	const char* pszStart = strchr( pszLook, cStart );
    ASSERT( pszLook );  // 1999-10-25 MRP
	const char* pszEnd = pszEndForcedGlosses( pszStart, cStart, cEnd, cMorphBreak );
	while ( *pszEnd == ' ' ) // Move past spaces
		pszEnd++;
	if ( *pszEnd == cMorphBreak && *( pszEnd + 1 ) == ' ' ) // Move past plain hyphen between compound roots
		pszEnd += 2;
	return pszEnd;
	}

const char* pszNextEl( const char* psz ) // Move to next match element in a rule, stops at begin of next word or at end of line or end of field
{
    while ( *psz && !( *psz == ' ' ) && !( *psz == '\n' ) ) // Move to end of element
        psz++;
    while ( *psz == ' ' ) // Move to start of next element
        psz++;
    return psz;
}

BOOL bElMatch( const char* pszText, const char* pszFind ) // Match replace element in match
{
    if ( *pszText == '(' ) // If at parend, move past it
        pszText++;
    if ( *pszFind == '(' ) // If at parend, move past it
        pszFind++;
    while ( *pszText++ == *pszFind++ ) // While match, move forward
        if ( !*pszText || *pszText == ' ' || *pszText == '\n' || *pszText == ')' ) // If text space or end of line, we may have success
            {
            if ( !*pszFind || *pszFind == ' ' || *pszFind == '\n' || *pszFind == ')' ) // If find word also at end, we have success
                return TRUE;
            return FALSE; // If find word not at end, fail
            }
    return FALSE; // If mismatch, fail
}

int iElFind( const char* pszMatch, const char* pszRepl ) // Find replace element in match string, return number of matched element or -1 if no match
    {
    int iElNum = 0;
    while ( !bElMatch( pszMatch, pszRepl ) ) // While no match found, try next
        {
        pszMatch = pszNextEl( pszMatch ); // Move to next
        if ( !*pszMatch || *pszMatch == '\n' ) // If end of match, return failure
            return -1;
        iElNum++; // Count the element
        }
    return iElNum; // Return number of matched element, may be zero
    }

const char* pszPhonDefNextEl( const char* psz ) // Move to next match element in a rule, stops at begin of next word or at end of line or end of field
{
    while ( *psz && !( *psz == ' ' ) && !( *psz == '\n' ) ) // Move to end of element
        psz++;
    while ( *psz == ' ' || *psz == '\n' ) // Move to start of next element
        psz++;
    return psz;
}

#define FULL_MATCH -2
#define NO_MATCH -1
int iDefNameMatch( const char* pszDef, const char* pszText, BOOL bPhon ) // Match find word in text, if success, return length of match, else return 0
{
    const char* pszStart = pszText; // Remember text start
    while ( *pszDef++ == *pszText++ ) // While match, move forward
		{
        if ( !*pszDef || *pszDef == ' ' || *pszDef == '\n' ) // If def name at end, we may have success
			{
			if ( !*pszText || *pszText == ' ' || *pszText == '\n' || *pszText == ')'  // If text name also at end, succeed
					|| ( bPhon && ( *pszText == ']' ) || ( *pszText == '*' ) ) )
				return pszText - pszStart; // Return length of match
			else // Else (text name not at end), fail
				return NO_MATCH;
			}
		}
    return NO_MATCH; // If mismatch, fail
}

int iWdMatch( const char* pszFind, const char* pszText, CLangEnc* plng ) // Match find word in text, if success, return length of match, else return 0
{
    const char* pszStart = pszText; // Remember text start
    while ( *pszText++ == *pszFind++ ) // While match, move forward
        if ( !*pszFind || *pszFind == ' ' || *pszFind == '\n' || *pszFind == ')' ) // If find word at space or end of line, we may have success
            {
            if ( !*pszText || plng->iWdSep( pszText ) ) // If text also at end, we have success
                {
                while ( !plng->bIsWhite( pszText ) && plng->iWdSep( pszText ) ) // Move past trailing punc
                    pszText++;
                pszText = plng->pszSkipWhite( pszText ); // Move to start of next word
                return pszText - pszStart; // Return length of match
                }
            return NO_MATCH; // If find word not at end, fail
            }
    return NO_MATCH; // If mismatch, fail
}

//---------------------------------------------
// CInterlinearProc
CMarkerSet* CInterlinearProc::pmkrset() const // Owner database type marker set
    { return pintprclst()->ptyp()->pmkrset(); } 

char CInterlinearProc::cForcedGlossStartChar() const
    { return m_pintprclstMyOwner->cForcedGlossStartChar(); }

char CInterlinearProc::cForcedGlossEndChar() const
    { return m_pintprclstMyOwner->cForcedGlossEndChar(); }

char CInterlinearProc::cParseMorphBoundChar() const
    { return m_pintprclstMyOwner->cParseMorphBoundChar(); }

Str8 CInterlinearProc::sMorphBreakChars() const
    { return m_pintprclstMyOwner->sMorphBreakChars(); }

BOOL CInterlinearProc::bIsMorphBreakChar( const char c ) const
    { return c && sMorphBreakChars().Find( c ) != -1; }

char CInterlinearProc::cMorphBreakChar( int i ) const // Return requested morph break char
    {
    int iLen = sMorphBreakChars().GetLength(); // Get number of break chars
    if ( !iLen )
        return ' '; // If none (langs with no word breaks do this), return space
    if ( i >= iLen ) // If too far, return first one
        i = 0;
    return sMorphBreakChars().GetChar( i );
    }

char CInterlinearProc::cMorphBreakChar( const char cDefault, const CLangEnc* plng ) const // 1.4ywp Return first morph break char if legacy IPA93, otherwise default
    {
	if ( plng->bUnicodeLang() ) // 1.4ywp If Unicode language, return default
		return cDefault; // 1.4ywp 
	else
		{
		Str8 s = plng->sFontName(); // 1.4ywp If legacy language, check font name for 93
		int iFind = s.Find( "93" ); // 1.4ywp 
		if ( iFind < 0 ) // 1.4ywp If not IPA93, return default
			return cDefault; // 1.4ywp 
		else // 1.4ywp Else, IPA93, return first morpheme break char
		    return cMorphBreakChar( 0 ); // 1.4ywp 
		}
    }

//---------------------------------------------------------------
CLookupProc::CLookupProc( CInterlinearProcList* pintprclstMyOwner,
        BOOL bParseProc )
            : CInterlinearProc( pintprclstMyOwner )
{
    for ( int i = 0; i < NUMTRIES; i++ )
        m_pptri[ i ] = new CDbTrie( this, i ); // Initialize tries
    m_bParseProc = bParseProc; // Set parsing
    if ( bParseProc )
		SetType( CProcNames::eParse ); // 1.4ah Change to not use string resource id in interlinear
    else
		SetType( CProcNames::eLookup ); // 1.4ah Change to not use string resource id in interlinear
    m_bApplyCC = FALSE;
    m_bSH2Parse = FALSE;
    m_bLongerOverride = TRUE;
    m_bOverrideShowAll = TRUE;
    m_bKeepCap = FALSE;
    m_bKeepPunc = FALSE;
    m_bStopAtSemicolon = FALSE;
    m_sGlossSeparator = ";";
    m_bShowFailMark = TRUE;
    m_bInsertFail = FALSE;
    m_sFailMark = "***";
    m_bShowWord = FALSE;
    m_bShowRootGuess = TRUE; // 1.4yj Change quick setup parse default to show root guess
    m_bInfixBefore = FALSE;
    m_bInfixMultiple = FALSE;
    m_bPreferPrefix = FALSE;
	if ( bParseProc )
		m_bPreferSuffix = TRUE;
	else
		m_bPreferSuffix = FALSE;
    m_bNoCompRoot = FALSE;
    m_pdocWordFormulas = NULL;
    // m_precWordFormulas = NULL;  // 1999-08-30 MRP
    m_pwdfset = NULL;  // 1999-09-02 MRP
}

CLookupProc::~CLookupProc() // Destructor   
{
    for ( int i = 0; i < NUMTRIES; i++ ) // Delete all tries
        delete m_pptri[ i ];
    delete m_pwdfset;  // 1990-09-02 MRP
}

void CLookupProc::SetRuleFile( const Str8& sRuleFile ) // Set rule file name   
{
    m_drflstWordFormulas.DeleteAll(); // Empty the ref list
    if ( !sRuleFile.IsEmpty() )
        m_drflstWordFormulas.Add( sRuleFile, "" ); // Make a new ref with the rule file name  
    // m_precWordFormulas = NULL;  // 1999-08-30 MRP
}

const char* CLookupProc::pszRuleFile() // Name of rule file
{
    CDatabaseRef* pdrf = m_drflstWordFormulas.pdrfFirst(); // Get database ref, can return null if no databases in list
    if ( !pdrf ) // If no rule file, return null string
        return "";
    else return pdrf->sDatabasePath();
}

// Prefix utilities
Str8 CLookupProc::sPrefMorphChange( Str8& sOut ) // Return morph change part of prefix ptot, and cut it off sOut
{       
    Str8 sMorphChange = ""; // Init to no morphophonemic part
    int iMorphBreak = sOut.Find( pintprclst()->cParseMorphBoundChar() ); // See if morphophonemic change, morph break char in output, eg. saient re#aient
    if ( iMorphBreak >= 0 ) // If morphophonemic change
        {
        ASSERT( sOut.GetLength() - iMorphBreak - 1 >= 0 );
        sMorphChange = sOut.Right( sOut.GetLength() - iMorphBreak - 1 ); // Add morphophonemic changed material  to leftover
        sOut = sOut.Left( iMorphBreak ); // Cut off morphophonemic material
		if ( !sOut.IsEmpty() && sOut[ sOut.GetLength() - 1 ] != ' ' ) // If added thing is not a separate word, add hyphen
			sOut += cMorphBreakChar( 0 ); // Add hyphen
        }
    return sMorphChange; // Return the morphophonemic part  
}

BOOL CLookupProc::bNextSamePrefMatch( CTrieOut** pptot ) // Return true if same length with same root output and different affix output
{
    CTrieOut* ptot = *pptot; // Get current
    Str8 sPref = ptot->pfld->sContents(); // Get prefix output for morphophonemics
    Str8 sMorphChange = sPrefMorphChange( sPref ); // Get morph change part and update sPref
    for ( CTrieOut* ptotNext = ptot->ptotNext();
            ptotNext && ptot->iLen() == ptotNext->iLen();
            ptotNext = ptotNext->ptotNext() ) // For all others on the list, see if ambiguity found
        {   
        Str8 sPrefNext = ptotNext->pfld->sContents(); // Get prefix output for morphophonemics
        Str8 sMorphChangeNext = sPrefMorphChange( sPrefNext ); // Get morphophonemic change output and cut off sPrefNext
        if ( sMorphChange == sMorphChangeNext && sPref != sPrefNext ) // If same morphophonemic change and different output, change
            {
            *pptot = ptotNext; // Move forward to the one found 
            return TRUE; // Succeed 
            }
        }
    return FALSE; // If none found, fail    
}

void CLookupProc::AddPrefix( const char* pszAff, CField* pfld ) // Add affix to field
{
    Str8 sResult = " " + *pfld; // Build result, starting with result of parse of the rest, and adding a space
    int iEnd = strlen( pszAff ) - 1; // Get end of prefix
    if ( iEnd < 0 ) // If prefix is null, protect against negative number
        iEnd = 0;
//    if ( !bIsMorphBreakChar( *( pszAff + iEnd ) ) ) // If not already a hyphen on the prefix, add one
//        sResult = cMorphBreakChar( 0 ) + sResult; //??? Maybe trie can choose which one // Use first one, assumes interface insists there be at least one
    sResult = pszAff + sResult; // Add prefix result    
    pfld->SetContent( sResult ); // Add prefix result to front of result of parse of the rest
}

// Suffix utilities                                
Str8 CLookupProc::sSuffMorphChange( Str8& sOut ) // Return morph change part of suffix ptot, and cut it off sOut
{       
    Str8 sMorphChange = ""; // Init to no morphophonemic part
    int iMorphBreak = sOut.Find( pintprclst()->cParseMorphBoundChar() ); // See if morphophonemic change, morph break char in output, eg. saient re#aient
    if ( iMorphBreak >= 0 ) // If morphophonemic change
        {
        sMorphChange = sOut.Left( iMorphBreak ); // Add morphophonemic changed material  to leftover   
        ASSERT( sOut.GetLength()  - iMorphBreak - 1 >= 0 );
        sOut = sOut.Right( sOut.GetLength()  - iMorphBreak - 1 ); // Cut off morphophonemic material from the suffix
		if ( !sOut.IsEmpty() && sOut[0] != ' ' ) // If added thing is not a separate word, add hyphen
			sOut = cMorphBreakChar( 0 ) + sOut; // Add hyphen
        }
    return sMorphChange; // Return the morphophonemic part  
}

BOOL CLookupProc::bNextSameSuffMatch( CTrieOut** pptot ) // Return true if same length with same root output and different affix output
{
    CTrieOut* ptot = *pptot; // Get current
    Str8 sSuff = ptot->pfld->sContents(); // Get suffix output for morphophonemics
    Str8 sMorphChange = sSuffMorphChange( sSuff ); // Get morph change part and update sSuff
    for ( CTrieOut* ptotNext = ptot->ptotNext();
            ptotNext && ptot->iLen() == ptotNext->iLen();
            ptotNext = ptotNext->ptotNext() ) // For all others on the list, see if ambiguity found
        {   
        Str8 sSuffNext = ptotNext->pfld->sContents(); // Get suffix output for morphophonemics
        Str8 sMorphChangeNext = sSuffMorphChange( sSuffNext ); // Get morphophonemic change output and cut off sSuffNext
        if ( sMorphChange == sMorphChangeNext && sSuff != sSuffNext ) // If same morphophonemic change and different output, change
            {
            *pptot = ptotNext; // Move forward to the one found 
            return TRUE; // Succeed 
            }
        }
    return FALSE; // If none found, fail    
}

void CLookupProc::AddSuffix( const char* pszAff, CField* pfld ) // Add affix to field
{
    Str8 sResult = *pfld + " "; // Build result, starting with result of parse of the rest, and adding a space
//    if ( !bIsMorphBreakChar( *pszAff ) ) // If not already a hyphen on the suffix, add one
//        sResult += cMorphBreakChar( 0 ); //??? Maybe trie can choose which one // Use first one, assumes interface insists there be at least one
    sResult += pszAff; // Add suffix result 
    pfld->SetContent( sResult ); // Set field to new result
}

void CLookupProc::AddRoot( const char* pszAff, CField* pfld ) // Add compound root to field
{
    Str8 sResult = " " + *pfld; // Build result, starting with result of parse of the rest, and adding a space
    if ( !bIsMorphBreakChar( *pfld->psz() ) ) // If no hyphen on front, add one
		{
		if ( cMorphBreakChar( 0 ) != ' ' ) // Langs with no word breaks use none, and see space here
			{
			sResult = cMorphBreakChar( 0 ) + sResult; // Add a morpheme break char
			sResult =  " " + sResult; // Add another space
			}
		}
    sResult = pszAff + sResult; // Add root result  
    pfld->SetContent( sResult ); // Add root result to front of result of parse of the rest
}

BOOL bShowAllParses = FALSE; // Show parses that fail the formulas, triggered by user

#define SUFFMAX 50
static int s_iSuffixMax = SUFFMAX; // Maximum suffix depth value, changeable here so that we can get out of near-infinite loops when an affix can be reanalyzed as itself another layer down

BOOL CLookupProc::bParseWord(  // Parse word, upper call to start recursive calls of bParse
            const char* psz,            // String to parse
            CFieldList* pfldlst,        // Field list for result of lookup and parse, may be empty, or have previous results to add to
            unsigned int& iSmallest, // Smallest piece found so far, used to show failed root on second pass to show failure
            BOOL bShowFail,             // If TRUE, show failure when iSmallest is reached, defaults to FALSE
			BOOL bSpellCheck )			// If TRUE, this is spell check, don't give parse timeout message // 1.4qzjm
{
	if ( bShowFail && bTimeLimitExceeded() ) // 1.2rg If main parse exceeded time limit, don't do failure parse
		return FALSE;
    if ( !bShowFail ) // If showing fail, don't initialize max suffix from parse setting it down
        s_iSuffixMax = SUFFMAX; // Initialize max suffix to normal size, it can be cut down inside parse if necessary to stop a near-infinite loop
    int iSuffix = 1; // Gives depth of recursive call, says start with suffix first if an odd number, bite only one of each affix if 0
	if ( bPreferPrefix() ) // If prefer prefix, make even number to start with prefix first
		iSuffix = 2;
	InitTimeLimit( 5, !bSpellCheck ); // 1.3bh Move timer init to separate function  // 1.4qzjm Don't give parse timeout message on interlinear check
    return bParse( psz, pfldlst, iSuffix, iSmallest, bShowFail );
}

    // Parse sequence:
    // Parse is called with iSuffix = 1. That triggers search for a suffix.
    // If a suffix is found, it then tries a bite (calls parse with a zero value which prevents any other affixes from being found).
    // If the bite fails, then the same is tried with a prefix (cut one prefix and try for a bite).
    // If both bites fail, then the suffix success tries to cut a prefix off the rest by using an ISuffix of 2.
    // The result is that the parse prefers to bite as shallow as possible, and prefers longer internal
    //  combinations to cutting into them. 
BOOL CLookupProc::bParse(  // Parse morphemes of string, calls itself recursively on smaller pieces
            const char* pszParse,           // String to parse
            CFieldList* pfldlst,        // Field list for result of lookup and parse, may be empty, or have previous results to add to
            int iSuffix,                        // Gives depth of recursive call, says start with suffix first if an odd number, bite only one of each affix if 0
            unsigned int& iSmallest, // Smallest piece found so far, used to show failed root on second pass to show failure
            BOOL bShowFail )            // If TRUE, show failure when iSmallest is reached, defaults to FALSE
{   
	if ( bTimeLimitExceeded() ) // 1.2rg At regular intervals, check elapsed time // 1.3bh Change to separate function
			return FALSE;
	BOOL bLongerAffixOverride = m_bLongerOverride && !( bShowAllParses && !m_bOverrideShowAll ); // 1.3bp Make show all parses show all affixes
    int iLenMatched = 0;
            // If lookup gives whole root match, return success, also save time by not trying null string
    BOOL bRootSucc = FALSE;
    if ( *pszParse && m_pptri[ROOT]->bSearch( pszParse, pfldlst, &iLenMatched, WHOLE_WORD | CASE_FOLD ) ) // Do lookup of whole root match
        {
        bRootSucc = TRUE; // Remember we saw a success on root
        if ( !( bShowAllParses && !m_bOverrideShowAll ) ) // 1.3bg Add option to not do longer override on show all parses // If longer overriding, succeed now, don't try further parse
			{
            return TRUE;
			}
        }
            // Remember smallest so as to show affixes with failure marks
    if ( !bShowFail ) // If not showing failures, remember smallest 
        iSmallest = __min( iSmallest, strlen( pszParse ) );
    else // Else (showing failures) see if smallest has been reached    
        if ( strlen( pszParse ) <= iSmallest ) // If smallest has been reached, do a failure tag
            {
            Str8 sFail; // Fail output string
            if ( m_bShowFailMark || ( m_bInsertFail && m_bShowRootGuess ) ) // If showing fail mark, put it in, also put it in for insert root guess
                sFail = m_sFailMark;
            if ( m_bShowRootGuess ) // If showing root guess, put it in
                sFail += pszParse;
            pfldlst->Add( new CField( m_pptri[ROOT]->pmkrOut(), sFail ) ); // Make a failure tag
            return TRUE; // Return true to show a usable result is here. Above will test for stars for failure.
            }
    if ( !*pszParse ) // If null string, save time by not trying further
        return FALSE;       
    if ( iSuffix > s_iSuffixMax ) // If greater than some ridiculous number, we must be in the infinite loop where morphophonemic output is the same length or longer than the input, so fail to avoid stack overflow
        { // Morphophonemic form gives cyclical pattern
        if ( s_iSuffixMax > 0 ) // If first time into here, give user a message
            {
            Str8 sMsg = _("Morphophonemic form gives cyclical pattern:"); // 1.5.0fg 
			sMsg = sMsg + " " + pszParse; // 1.5.0fg 
            AfxMessageBox( sMsg ); // 1.4gd
            }
        s_iSuffixMax = 0; // Cut down suffix max to stop the near-infinite loop
        return FALSE;       
        }

    if ( !iSuffix ) // If suffix is false, we are only testing the bite, so fail now
        return FALSE;
    BOOL bSucc = FALSE;
	BOOL bInfixSucc = FALSE; // 1.2pn If infix is found, try affixes also
    const char* pszStart = pszParse; // 1.2pn Check for infixes before affixes
	if ( !bShowFail && !m_pptri[INF]->bIsEmpty() ) // 1.2pk Don't search infixes if there are none // Don't try infixes if showing failure affixes
		{
		const char* psz = pszStart; // 1.5.1mb Set up to search forward or backward
		BOOL bMore = TRUE; // 1.5.1mb 
		if ( *psz == NULL ) // 1.5.1mb If null string, don't look for infixes
			bMore = FALSE; // 1.5.1mb 
		else
			if ( !bInfixBefore() ) // 1.5.1mb If infix after, look for last infix first
				psz = psz + strlen( psz ) - 1; // 1.5.1mb 
//		for ( const char* psz = pszStart; *psz; psz++ ) // Look for infixes: For each position in string, see if infix here // 1.5.1mb 
		while ( bMore ) // 1.5.1mb 
			{
			CTrieOut* ptotFound; // Trie out, results of trie searches
			if ( ( ptotFound = m_pptri[INF]->ptotSearch( psz ) ) ) // If infixes found
				{
				for ( CTrieOut* ptot = ptotFound; ptot; ptot = ptot->ptotNext() ) // For each infix found, try to parse rest
					{
					CFieldList fldlst; // Temporary field list for result of parse, relies on constructor to initialize and destructor to empty
					int iPos = psz - pszStart;
					Str8 sRest( pszStart ); // Make a copy to modify
					Str8 sFirst = sRest.Left( iPos ); // First part of root            
					ASSERT( sRest.GetLength() - ptot->iLen() - iPos >= 0 );
					Str8 sLast = sRest.Right( sRest.GetLength() - ptot->iLen() - iPos ); // Last part of root
							// Treat infix morphophonemics as if it were a prefix
					Str8 sOut = ptot->pfld->sContents(); // Get infix output for morphophonemics
					sLast = sPrefMorphChange( sOut ) + sLast; // Add morphophonemic changed material  to leftover and cut off morphophonemic material from sOut
					sRest = sFirst + sLast; // Form root without infix
                
					int iInfixMultiple = 0; // 1.5.1mb 
					if ( bInfixMultiple() ) // 1.5.1mb 
						iInfixMultiple = 1; // 1.5.1mb 
					if ( bParse( sRest, &fldlst, iInfixMultiple, iSmallest, bShowFail ) ) // 1.2pp Only accept infix if it is in root // If parse of rest succeeds, build results // 1.5.1mb Option to allow multiple infixes
						{ // For each result of parse, add this affix to the result
						for ( CField* pfld = fldlst.pfldFirst(); pfld; pfld = fldlst.pfldNext( pfld ) ) // For each result of parse
							{
							if ( bInfixBefore() ) // If infix before, add it as a prefix, else as a suffix
								AddPrefix( sOut, pfld );
							else
								AddSuffix( sOut, pfld );
							pfldlst->Add( new CField( *pfld ) ); // Add field to main field list
							}
						bInfixSucc = TRUE; // 1.2pn If infix is found, try affixes also // Note successful parse was found    
						CTrieOut* ptotNext = ptot->ptotNext();
						if ( !m_bSH2Parse // If not in SH2 parse, don't show all ambiguities
								&& bLongerAffixOverride // If longer override, don't show all ambigs
								&& !( ptotNext && ptot->iLen() == ptotNext->iLen() ) ) // Also, if next affix same length, show its ambiguities
							break; // If not SH2 and next is shorter, don't try any more
						}
					}
				CTrieOut::s_DeleteAll( ptotFound ); // Delete temp struct built by trie
				}
			if ( bInfixMultiple() && bInfixSucc ) // 1.5.1mb If infix found, don't look for another at this level
				break; // 1.5.1mb 
			if ( bInfixBefore() ) // 1.5.1mb If infix before, check for end and move forward
				{
				if ( !*psz )
					bMore = FALSE;
				else
					psz++;
				}
			else // 1.5.1mb Else (infix after) check for beginning and move backward
				{
				if ( psz == pszStart )
					bMore = FALSE;
				else
					psz--;
				}
			}
		}
    if ( !( m_bSH2Parse && iSuffix >= 3 ) ) // Don't go deeper if SH2 parse
      for ( int iBite = 1; iBite >= 0; iBite-- ) // First time just bite one, second time parse deeper
        {
        for ( int iTurn = 0; iTurn < 2; iTurn++ ) // First turn is suffix or prefix, depending on iSuffix passed in, next turn is the other one
            {
			if ( ( iTurn + iSuffix ) % 2 ) // If it is suffixes turn, do it, else do prefix
                {       // Look  for a suffix
                CTrieOut* ptotFound; // Trie out, results of trie searches
                if ( ( ptotFound = m_pptri[SUFF]->ptotSearch( pszParse ) ) ) // If suffixes found
                    {
                    for ( CTrieOut* ptot = ptotFound; ptot; ptot = ptot->ptotNext() ) // For each suffix found, try to parse rest
                        {
                        CFieldList fldlst; // Temporary field list for result of parse, relies on constructor to initialize and destructor to empty
                        Str8 sRest( pszParse ); // Make a copy to modify
                        sRest = sRest.Left( sRest.GetLength() - ptot->iLen() ); // Cut off suffix
                        Str8 sOut = ptot->pfld->sContents(); // Get suffix output for morphophonemics
                        sRest += sSuffMorphChange( sOut ); // Add morphophonemic changed material  to leftover and cut off morphophonemic material from sOut
                        if ( sRest.GetLength() && bIsMorphBreakChar( sRest[ sRest.GetLength() - 1 ] ) ) // If hyphen next, move past it
                            sRest = sRest.Left( sRest.GetLength() - 1 );
                        int iBiteOrSuffix = iBite ? 0 : iSuffix + 1; // Set up to try bite or to give prefix a turn
						if ( !iBite && bPreferSuffix() ) // If prefer suffix, then give suffix next turn by incrementing past prefix turn
							iBiteOrSuffix++;
                        if ( bParse( sRest, &fldlst, iBiteOrSuffix, iSmallest, bShowFail ) ) // If parse of rest succeeds, build results
                            { // For each result of parse, add this affix to the result
                            for ( CField* pfld = fldlst.pfldFirst(); pfld; pfld = fldlst.pfldNext( pfld ) ) // For each result of parse
                                {
                                AddSuffix( sOut, pfld ); // Add this affix to the field
                                pfldlst->Add( new CField( *pfld ) ); // Add field to main field list
                                }
                            bSucc = TRUE; // Note successful parse was found    
                            CTrieOut* ptotNext = ptot->ptotNext();
                            if ( !m_bSH2Parse // If not in SH2 parse, don't show all ambiguities
                                    && bLongerAffixOverride // If longer override, don't show all ambigs
                                    && !( ptotNext && ptot->iLen() == ptotNext->iLen() ) ) // Also, if next affix same length, show its ambiguities
                                break; // If not SH2 and next is shorter, don't try any more
                            }
                        }
                    CTrieOut::s_DeleteAll( ptotFound ); // Delete temp struct built by trie
                    if ( bSucc && bPreferSuffix() ) // 1.2pr Fix bug of non-override not finding all affix parses
                        return TRUE; // Return successful parse
                    }
                }   
            else // Else prefixes turn
                {   
                            // Look for a prefix    
                CTrieOut* ptotFound; // Trie out, results of trie searches
                if ( ( ptotFound = m_pptri[PREF]->ptotSearch( pszParse, CASE_FOLD ) ) ) // If prefixes found
                    {                   
                    for ( CTrieOut* ptot = ptotFound; ptot; ptot = ptot->ptotNext() ) // For each prefix found, try to parse rest
                        {
                        CFieldList fldlst; // Temporary field list for result of parse, relies on constructor to initialize and destructor to empty
                        Str8 sRest( pszParse ); // Make a copy to modify
                        ASSERT( sRest.GetLength() - ptot->iLen() >= 0 );
                        sRest = sRest.Right( sRest.GetLength() - ptot->iLen() ); // Cut off prefix
                        Str8 sOut = ptot->pfld->sContents(); // Get prefix output for morphophonemics
                        sRest = sPrefMorphChange( sOut ) + sRest; // Add morphophonemic changed material  to leftover and cut off morphophonemic material from sOut
                        if ( sRest.GetLength() && bIsMorphBreakChar( sRest[0] ) ) // If hyphen next, move past it
                            sRest = sRest.Mid( 1 );
                        int iBiteOrSuffix = iBite ? 0 : iSuffix + 1;
						if ( !iBite && bPreferPrefix() ) // If prefer prefix, then give prefix next turn by incrementing past suffix turn
							iBiteOrSuffix++;
                        if ( bParse( sRest, &fldlst, iBiteOrSuffix, iSmallest, bShowFail ) ) // If parse of rest succeeds, build results
                            { // For each result of parse, add this affix to the result
                            for ( CField* pfld = fldlst.pfldFirst(); pfld; pfld = fldlst.pfldNext( pfld ) ) // For each result of parse
                                {
                                AddPrefix( sOut, pfld ); // Add this affix to the field
                                pfldlst->Add( new CField( *pfld ) ); // Add field to main field list
                                } 
                            bSucc = TRUE; // Note successful parse was found    
                            CTrieOut* ptotNext = ptot->ptotNext();
                            if ( !m_bSH2Parse // If not in SH2 parse, don't show all ambiguities
                                    && bLongerAffixOverride // If longer override, don't show all ambigs
                                    && !( ptotNext && ptot->iLen() == ptotNext->iLen() ) ) // Also, if next affix same length, show its ambiguities
                                break; // If not SH2 and next is shorter, don't try any more
                            }
                        }
                    CTrieOut::s_DeleteAll( ptotFound ); // Delete temp struct built by trie
                    if ( bSucc && bPreferPrefix() ) // 1.2pr Fix bug of non-override not finding all affix parses
                        return TRUE; // Return successful parse
                    }
                }   
            }
        }
    if ( *pszParse && !bRootSucc && !m_bSH2Parse && bCompRoot() ) // && !bShowFail ) // If no whole root or affix parses found, try compound root 5.9a
        {
        for ( int iBite = 0; iBite <= 1; iBite++ ) // First time just bite one, second time parse deeper
            {
                        // Look for a partial root  
            CTrieOut* ptotFound; // Trie out, results of trie searches
            if ( ( ptotFound = m_pptri[ROOT]->ptotSearch( pszParse, CASE_FOLD ) ) ) // If roots found
                {                   
                for ( CTrieOut* ptot = ptotFound; ptot; ptot = ptot->ptotNext() ) // For each root found, try to parse rest
                    {
                    CFieldList fldlst; // Temporary field list for result of parse, relies on constructor to initialize and destructor to empty
                    Str8 sRest( pszParse ); // Make a copy to modify 
                    ASSERT( sRest.GetLength() - ptot->iLen() >= 0 );
                    sRest = sRest.Right( sRest.GetLength() - ptot->iLen() ); // Cut off root
                    Str8 sOut = ptot->pfld->sContents(); // Get prefix output for morphophonemics
                    if ( sRest.GetLength() && bIsMorphBreakChar( sRest[0] ) ) // If hyphen next, move past it
                        sRest = sRest.Mid( 1 );
                    if ( bParse( sRest, &fldlst, iBite, iSmallest, bShowFail ) ) // If parse of rest succeeds, build results
                        { // For each result of parse, add this root to the result
                        for ( CField* pfld = fldlst.pfldFirst(); pfld; pfld = fldlst.pfldNext( pfld ) ) // For each result of parse
                            {
                            AddRoot( sOut, pfld ); // Add this root to the field
                            pfldlst->Add( new CField( *pfld ) ); // Add field to main field list
                            } 
                        bSucc = TRUE; // Note successful parse was found    
                        CTrieOut* ptotNext = ptot->ptotNext();
                        if ( !m_bSH2Parse // If not in SH2 parse, don't show all ambiguities
                                && bLongerAffixOverride // If longer override, don't show all ambigs
                                && !( ptotNext && ptot->iLen() == ptotNext->iLen() ) ) // Also, if next affix same length, show its ambiguities
                            break; // If not SH2 and next is shorter, don't try any more
                        }
                    }
                CTrieOut::s_DeleteAll( ptotFound ); // Delete temp struct built by trie
                if ( bSucc )
                    return TRUE; // Return successful parse
                }
            }       
        }
	bSucc = ( bRootSucc || bInfixSucc || bSucc ); // 1.4vzp Try to fix bug of infix fail on show all parses, didn't work
    return bSucc; // 1.2pr Fix bug of non-override not finding all affix parses // 1.2pn Return true if infixes found // No affix parses found, return result of root search
	}

static CMarker* pmkr( CRecPos rps, const char* pszName ) // Get marker pointer from name using recpos
{
    ASSERT( rps.pfld );
    CMarkerSet* pmkrset = rps.pfld->pmkr()->pmkrsetMyOwner(); // Get marker set from marker of field in recpos
    return pmkrset->pmkrSearch_AddIfNew( pszName );
}

static void InsistNonZero( int& i, int iForce ) // Insist an integer is non-zero, if it is zero, force it to a value
{
    #ifdef _DEBUG
    ASSERT( i ); // ASSERT in the debug version, but try to recover in the release version - This can happen when the text line and parse line use different language encodings.
    #endif
    if ( !i )
        i = iForce;
}

static void InsistNonEmpty( Str8& s, const char* pszForce ) // Insist a Str8 is non-empty, if it is empty, force it to a value
{
    #ifdef _DEBUG
    ASSERT( !s.IsEmpty() ); // ASSERT in the debug version, but try to recover in the release version
    #endif
    if ( s.IsEmpty() )
        s = pszForce;
}

static void InsistNonEmpty( CMString& mks, const char* pszForce ) // Insist a CMString is non-empty, if it is empty, force it to a value
{
    #ifdef _DEBUG
//    ASSERT( !mks.IsEmpty() ); // ASSERT in the debug version, but try to recover in the release version
    #endif
    if ( mks.IsEmpty() )
        mks = pszForce;
}

BOOL CLookupProc::bStartsWithSuffix( const char* psz ) // True if string starts with suffix (but not infix)
{
    if ( !*psz ) // If empty, false
        return FALSE;
    if ( !bIsMorphBreakChar( *psz++ ) ) // If no hyphen on front, false (if yes, step to next)
        return FALSE;
    while( *psz && *psz != ' ' ) // While no space, look for infix hyphen
        if ( bIsMorphBreakChar( *psz++ ) )  // If infix hyphen found, false
            return FALSE;
    return TRUE; // If all conditions met, true     
}

BOOL CLookupProc::bEndsWithPrefix( const char* psz ) // True if string ends with prefix (but not infix)
{
    if ( !*psz ) // If empty, false
        return FALSE;
    const char* pszStart = psz; // Remember start
    psz += strlen( psz ) - 1; // Move to end
    if ( !bIsMorphBreakChar( *psz-- ) ) // If no hyphen at end, false (if yes, step to next)
        return FALSE;
    while( psz >= pszStart && *psz != ' ' ) // While no space, look for infix hyphen
        if ( bIsMorphBreakChar( *psz-- ) )  // If infix hyphen found, false
            return FALSE;
    return TRUE; // If all conditions met, true         
}

int CLookupProc::iGetLenMorph( const char* psz ) // Get the length of the actual morpheme, discounting the forced glosses
{
    char cForceStart = pintprclst()->cForcedGlossStartChar();
    const char* pszForceStart = strchr( psz, cForceStart ); // Search for force start char
    if ( pszForceStart ) // If found, return length up to it
        return pszForceStart - psz;
    else
        return strlen( psz ); // Else return full length    
}


// --------------------------------------------------------------------------

BOOL CLookupProc::bDisambigLookup( CRecPos rpsStartWd, CFieldList* pfldlstAnal )
{
    ASSERT( !m_bParseProc );
    ASSERT( pfldlstAnal );
	if ( !bTimeLimitExceeded() ) // 1.3bk Don't eliminate duplicates if time limit already exceeded
	    pfldlstAnal->EliminateDuplicates( TRUE ); // Eliminate duplicates // 1.3bm Do time limit on eliminate duplicates

    if ( pfldlstAnal->lGetCount() > 1 ) // If more than one ambiguity, have user choose
        {
        CRecPos rpsEnd = rpsStartWd.rpsEndOfBundle();
        rpsEnd.pfld->SetContent( rpsEnd.pfld->sContents() + '\n' ); // Add nl at end of bundle to make it look right while disambiguating
        // Select the thing being disambiguated, and make sure it is visible in window
        CShwView* pview = Shw_papp()->pviewActive();
        ASSERT( pview );
        pview->SelectAmbig( rpsStartWd ); // Select ambiguity containing rps, scroll it up from bottom
        CAmbigDlg dlg(pfldlstAnal);
        BOOL bOK = (dlg.DoModal() == IDOK); // Put ambiguities into dialog box and make user choose one
        pview->ClearSelection(); // Clear the selection begin
        rpsEnd.pfld->Trim(); // Trim off nl again
        if ( !bOK ) // If they canceled, stop interlinearizing
            return FALSE; // Stop interlinearizing
        }

    CField* pfldSelected = pfldlstAnal->pfldFirst();
    ASSERT( pfldSelected ); // There had better be one left  
    pfldSelected->Trim(); // Trim trailing spaces and nl's from insert field
    return TRUE;    
}

BOOL CLookupProc::bDisambigParse( CRecPos rpsStartWd, CFieldList* pfldlstAnal,
        CFieldList* pfldlstExcluded, BOOL bShowAlways )
{
    ASSERT( m_bParseProc );
    ASSERT( pfldlstAnal );
    ASSERT( pfldlstExcluded );
	if ( !bTimeLimitExceeded() )
	    pfldlstAnal->EliminateDuplicates( TRUE ); // Eliminate duplicates // 1.3bm Do time limit on eliminate duplicates); // Eliminate duplicates

    if ( pfldlstAnal->lGetCount() > 1 || bShowAlways ) // If more than one ambiguity, have user choose
        {
        CRecPos rpsEnd = rpsStartWd.rpsEndOfBundle();
        rpsEnd.pfld->SetContent( rpsEnd.pfld->sContents() + '\n' ); // Add nl at end of bundle to make it look right while disambiguating
        // Select the thing being disambiguated, and make sure it is visible in window
        CShwView* pview = Shw_papp()->pviewActive();
        ASSERT( pview );
        pview->SelectAmbig( rpsStartWd ); // Select ambiguity containing rps, scroll it up from bottom
        CAmbigDlg dlg(pfldlstAnal, pfldlstExcluded, m_pwdfset);
        BOOL bOK = (dlg.DoModal() == IDOK); // Put ambiguities into dialog box and make user choose one
        pview->ClearSelection(); // Clear the selection begin
        rpsEnd.pfld->Trim(); // Trim off nl again
        if ( !bOK ) // If they canceled, stop interlinearizing
            return FALSE; // Stop interlinearizing
        }

    CField* pfldSelected = pfldlstAnal->pfldFirst();
    ASSERT( pfldSelected ); // There had better be one left  
    pfldSelected->Trim(); // Trim trailing spaces and nl's from insert field
    return TRUE;    
}

void CLookupProc::InsertFailMarks( CFieldList* pfldlst ) const
{
    const char* pchInvalid = m_sFailMark;
    ASSERT( *pchInvalid );
    char pszInvalid[] = { *pchInvalid, ' ', '\0' };

    ASSERT( pfldlst );
    CField* pfld = pfldlst->pfldFirst();
    for ( ; pfld; pfld = pfldlst->pfldNext(pfld) )
        pfld->Insert(pszInvalid, 0);
}

void CLookupProc::DeleteFailMarks( CFieldList* pfldlst ) const
{
    const char* pchInvalid = m_sFailMark;
    ASSERT( *pchInvalid );
    char pszInvalid[] = { *pchInvalid, ' ', '\0' };

    ASSERT( pfldlst );
    CField* pfld = pfldlst->pfldFirst();
    for ( ; pfld; pfld = pfldlst->pfldNext(pfld) )
        {
		const char* pszParse = pfld->psz();
		if ( strncmp(pszParse, pszInvalid, 2) == 0 )
			pfld->Delete(0, 2); // Delete fail mark and space
        }
}

// Attempt to match an item in a word formula pattern
// with any of the items of lookup data for a morpheme in the parse.
// For example, for the pattern
//     n (pos) (p) (p)
// and the data
//     hawa(gramma)(n) -ka(2s.poss)(pos)
// this function will be called twice, once to match gramma or n with the required n
// in the pattern, and a second time to match 2s.poss or pos with the optional
// pos in the pattern. The function will return TRUE the first time because the part
// of speech in the data for the hawa morpheme does match the n in the pattern. It will
// return TRUE the second time because the part of speech in the data for the -ka
// morpheme does match the pos in the pattern.
BOOL CLookupProc::bMatchParseLookupData(const char* pszPatternItem, int lenPatternItem,
        const char* pszParse, const char** ppszUnmatched) const
{
    ASSERT( pszPatternItem );
    ASSERT( 0 < lenPatternItem );
    ASSERT( pszParse );
    ASSERT( ppszUnmatched );

    char cStart = cForcedGlossStartChar();
	char cEnd =  cForcedGlossEndChar();
	char cMorphBreak = cMorphBreakChar( 0 );

    const char* pszDataItem = strchr( pszParse, cStart );
	if ( !pszDataItem )
        return FALSE;  // No lookup data

    // 2000-09-21 MRP: This had previously been implemented using iWdMatch;
    // however, because that function is too general for this purpose,
    // it can indicate a false partial match if the lookup data
    // includes embedded word break characters (e.g., p.Cont).
    ASSERT( *pszDataItem == cStart );
    pszDataItem++;
	while ( !bMatchParseLookupData( pszPatternItem, lenPatternItem, pszDataItem ) )
        {
        pszDataItem = pszNextForcedGloss( pszDataItem, cStart, cEnd );
        if ( !pszDataItem )
            return FALSE;  // No more lookup data
        }

    // Skip to the next morpheme in the parse
	*ppszUnmatched = pszNextLookupPos( pszParse, cStart, cEnd, cMorphBreak );
    return TRUE;
}

// 2000-09-21 MRP: Attempt to match an item in a word formula pattern
// with an item of lookup data for a morpheme in the parse.
BOOL CLookupProc::bMatchParseLookupData(const char* pszPatternItem, int lenPatternItem,
        const char* pszDataItem) const
{
    ASSERT( pszPatternItem );
    ASSERT( 0 < lenPatternItem );
    ASSERT( pszDataItem );

    char cStart = cForcedGlossStartChar();
	char cEnd =  cForcedGlossEndChar();

    const char* pszPatternItemEnd = pszPatternItem + lenPatternItem;
    while ( pszPatternItem != pszPatternItemEnd && *pszDataItem != cEnd )
        {
        ASSERT( *pszPatternItem );
        // Guard against incorrect syntax in the lexical data.
        if ( !*pszDataItem || *pszDataItem == ' ' || *pszDataItem == cStart )
            return FALSE;

        if ( *pszPatternItem != *pszDataItem )
            return FALSE;  // Items don't match

        pszPatternItem++;
        pszDataItem++;
        }

    // We have to get to the end of both the pattern and the lookup data
    // item at the same time (i.e., they have to be the same length).
    if ( pszPatternItem != pszPatternItemEnd )
        return FALSE;  // Didn't match the entire pattern item

    if ( *pszDataItem != cEnd )
        return FALSE;  // Didn't match the entire lookup data item

    return TRUE;
}

int CLookupProc::nLookupDataGroupsInParse(const Str8& sParse) const
{
    char cStart = cForcedGlossStartChar();
	char cEnd =  cForcedGlossEndChar();
	char cMorphBreak = cMorphBreakChar( 0 );

    const char* pszParse = sParse;
    // 1999-10-25 MRP: Guard against invalid operation.
    // Incorrect syntax in the \a and \u fields can cause absence
    // of the "forced gloss" characters for some morphemes in the parse.
    pszParse = strchr( pszParse, cStart );

    int n = 0;
    while ( pszParse )
        {
        n++;
  	    pszParse = pszNextLookupPos( pszParse, cStart, cEnd, cMorphBreak );
        pszParse = strchr( pszParse, cStart );  // 1999-10-25 MRP
        }

    return n;
}

// --------------------------------------------------------------------------

void CLookupProc::AddGlossesToField( CField *pfldToGloss, int iStartPos, 
				CFieldList *pfldlstOut ) // Do lookup of each morpheme to add gloss and part of speech, recursive
	{
    // 2000-07-20 MRP: Added the forced gloss code that was to be done "later".
    ASSERT( pfldToGloss );
    ASSERT( pfldlstOut );
    ASSERT( pfldlstOut->bIsEmpty() );
	char cStart = cForcedGlossStartChar();
	char cEnd =  cForcedGlossEndChar();
	char cMorphBreak = cMorphBreakChar( 0 );
	const char* pszLook = pfldToGloss->psz();
	if ( iStartPos >= 0 ) // Move to next place, past gloss and ps of current
		pszLook = pszNextLookupPos( pszLook + iStartPos, cStart, cEnd, cMorphBreak );
	int iLookPos = pszLook - pfldToGloss->psz(); // Remember number of lookup position
	if ( pszLook ) // Do lookup at next place
		{
		// Remove forced gloss before lookup
        Str8 sForcedGloss;  // 2000-07-20 MRP
        BOOL bForcedGloss = FALSE;
        int iForcedGloss = 0;
		const char* pszForceStart = strchr( pszLook, cStart ); // Find a forced gloss start out ahead somewhere
		if ( pszForceStart ) // If one is found, take it out before lookup
			{
			const char* pszForceEnd = pszEndForcedGlosses( pszLook, cStart, cEnd, cMorphBreak ); // Find end of forced gloss
			if ( pszForceEnd )
				{
				int iDelStart = pszForceStart - pfldToGloss->psz(); // Calculate beginning of deletion
				int iDelNum = pszForceEnd - pszForceStart; // Calculate end of delete
                sForcedGloss = pfldToGloss->Mid( iDelStart, iDelNum );  // 2000-07-20 MRP
                bForcedGloss = TRUE;
                iForcedGloss = iDelStart;
				pfldToGloss->Delete( iDelStart, iDelNum ); // Delete the forced gloss, later save it somewhere first
				pszLook = pfldToGloss->psz() + iLookPos; // Correct lookup pointer because the field has moved
				}
			}
		int iLenMatched = 0;
		CFieldList fldlstLookOut; // Field list for result of lookup
		if ( m_pptri[LOOK]->bSearch( pszLook, &fldlstLookOut, &iLenMatched, WHOLE_WORD | CASE_FOLD | LONGEST ) )
			{
			// Later see if lookup goes up to the forced gloss start, if so use it to constrain this lookup, otherwise put back in
            // 2000-07-20 MRP
			// for ( CField* pfldLookOut = fldlstLookOut.pfldFirst(); pfldLookOut; pfldLookOut = fldlstLookOut.pfldNext( pfldLookOut ) )
            CField* pfldLookOut = fldlstLookOut.pfldFirst();
            while ( pfldLookOut )
				{ // For every lookup result, build a list entry from field with lookup inserted
				const char* pszLookOut = pfldLookOut->psz();  // Put parends around the first gloss of the lookup
				const char* pszInsertParen = strchr( pszLookOut, cStart ); // Find place to insert, just before parend
				if ( !pszInsertParen ) // If no parend, insert at end
					pszInsertParen = pszLookOut + strlen( pszLookOut );
				int iInsertParen = pszInsertParen - pszLookOut;
				pfldLookOut->Insert( cEnd, iInsertParen ); // Insert close parend
				pfldLookOut->Insert( cStart, 0 ); // Insert open parend
                // 2000-07-20 MRP: Use the forced gloss to constrain the looked up data.
                // If this morpheme had the forced gloss in an underlying form,
                // delete any other homonyms or senses from the look up list.
                // This step is necessary to prevent parses already eliminated
                // via forced glosses from being added back to the list.
                if ( bForcedGloss && iLookPos + iLenMatched == iForcedGloss )
                    {
                    if ( strstr(*pfldLookOut, sForcedGloss) == NULL )
                        {
                        CField* pfldNext = fldlstLookOut.pfldNext( pfldLookOut );
                        // 2000-09-14 MRP: CFieldList::Delete asserts at
                        // Line 46 in CRECORD.CPP if the first lookup field
                        // in the list needs to be deleted. That assertion is
                        // appropriate for a genuine database record. However,
                        // fldlistLookOut is simply list of fields. It would
                        // have been better to have two different classes.
                        // The minimal fix is to call the base class function.
                        // fldlstLookOut.Delete( pfldLookOut );
                        fldlstLookOut.CDblList::Delete( (CDblListEl*) pfldLookOut );
                        pfldLookOut = pfldNext;
                        continue;
                        }
                    }
                int iLenInserted = pfldLookOut->GetLength();  // 2000-07-20 MRP
				int iStartLen = pszLook - pfldToGloss->psz() + iLenMatched; // Find the length of the start of the field
				Str8 s( pfldToGloss->psz(), iStartLen ); // Get the start of the field
				pfldLookOut->Insert( s, 0 ); // Insert start
				pfldLookOut->Insert( pszLook + iLenMatched, pfldLookOut->GetLength() );
                // 2000-07-20 MRP: Put the forced gloss back.
                if ( bForcedGloss && iLookPos + iLenMatched < iForcedGloss )
                    pfldLookOut->Insert( sForcedGloss, iForcedGloss + iLenInserted );
                pfldLookOut = fldlstLookOut.pfldNext( pfldLookOut );
				}
			int iNextPlace = pszLook - pfldToGloss->psz();
			AddGlossesToList( &fldlstLookOut, iNextPlace ); // Add glosses to result list at next place
            pfldlstOut->MoveFieldsFrom( &fldlstLookOut ); // Move fields to output list
			}
		}
	if ( pfldlstOut->bIsEmpty() ) // If empty, (lookup fail or no more morphs to gloss) return field unchanged
		{
		CField* pfldNew = new CField( *pfldToGloss );
		pfldlstOut->Add( pfldNew );
		}
	}

void CLookupProc::AddGlossesToList( CFieldList *pfldlstAnal, int iStartPos ) // Do lookup of each morpheme to add gloss and part of speech
	{
	int iNumDone = 0; // 1.3bj Allow some parses to be shown even if time limit is exceeded
	CFieldList fldlstResult; // Temp result list
	for ( CField* pfld = pfldlstAnal->pfldFirst(); pfld; pfld = pfldlstAnal->pfldNext( pfld ) ) // For each field
		{
		CFieldList fldlstResultFromField;
		AddGlossesToField( pfld, iStartPos, &fldlstResultFromField ); // Look up line, ambiguities may produce multiple results
		fldlstResult.MoveFieldsFrom( &fldlstResultFromField ); 
		if ( bTimeLimitExceeded() && iNumDone++ > 100 ) // 1.3bh Check elapsed time during word formulas // 1.3bj Allow some parses to be shown even if time limit is exceeded
			break;
		}
	pfldlstAnal->DeleteAll(); // Clear the original list
	pfldlstAnal->MoveFieldsFrom( &fldlstResult ); // Copy the temp result list to the original list
	}

// Functions below are encapsulizations of parts of bInterlinearize so that it and verify will be short
BOOL CLookupProc::bInitWordFormulas() // If first time, and word formulas not set up yet, do them now
	{
	return TRUE;
	}

BOOL CLookupProc::bInitStart( CRecPos &rpsStart, CField* pfldFrom ) // Init rpsStart
	{
    ASSERT( rpsStart.iChar <= rpsStart.pfld->GetLength() );
    if ( !pfldFrom ) // If no earlier process generated the desired field, don't go further
        return FALSE;
    rpsStart.SetPosEx( rpsStart.iChar, pfldFrom ); // Set starting position to desired column in from field
	return TRUE;
	}

Str8 sGetPrevWord( CRecPos rpsStart ) // Set up previous word to check for whole word reduplication
	{
    Str8 sPrevWord; // Previous word, for possible redup
    CRecPos rpsPrevWd = rpsStart; // Temp for prev word start
    if ( rpsPrevWd.bMoveToPrevWdWfc() )
        sPrevWord = Str8( rpsPrevWd.psz(), rpsPrevWd.iWdLen() ); // Remember previous word for possible redup
	return sPrevWord;
	}

#define cForceStart pintprclst()->cForcedGlossStartChar() // Define shorthands for forced gloss start and end chars
#define cForceEnd pintprclst()->cForcedGlossEndChar()

Str8 CLookupProc::sGetLeadPunc( CRecPos rpsStartCol, CRecPos &rpsStartWd ) // Set up leading punc of word
	{
    if ( !rpsStartWd.bMatch( m_sFailMark ) ) // If not fail mark
        rpsStartWd.bSkipPunc(); // Move past leading punc to start of next word
    if ( !rpsStartWd.bEndOfField() && 
            rpsStartWd.iChar > 0 && bIsMorphBreakChar( *(rpsStartWd.psz() - 1) ) ) // If we went past a suffix hyphen, back over it
        rpsStartWd.iChar -= 1;
	if ( rpsStartWd.iChar < rpsStartCol.iChar ) // Prevent assert if top word sits one space to right of hyphen below
		rpsStartWd.iChar = rpsStartCol.iChar;
    Str8 sLeadPunc( rpsStartCol.psz(), rpsStartWd.psz() - rpsStartCol.psz() ); // Collect leading punc
	return sLeadPunc;
	}

Str8 CLookupProc::sGetCurWord( CRecPos rpsStartCol, CRecPos &rpsStartWd, CRecPos &rpsEndWd, int &iLenMatched ) // Get current word
	{
	if ( rpsStartWd.bIsWhite() || rpsStartWd.bEndOfField() ) // If word is pure punctuation, move start back to beginning of word
		rpsStartWd = rpsStartCol;
    if ( bIsMorphBreakChar( *rpsEndWd.psz() ) ) // If morph break char, include it 
        rpsEndWd.iChar++;
    if ( rpsEndWd.bMatch( m_sFailMark ) ) // If a fail mark, go past it
        rpsEndWd.iChar += m_sFailMark.GetLength();
    rpsEndWd.bSkipWdChar(); // Move to end of word
    if ( bIsMorphBreakChar( *rpsEndWd.psz() ) ) // If morph break char, include it 
        rpsEndWd.iChar++;
    iLenMatched = rpsEndWd.iChar - rpsStartWd.iChar; // Length of match for insert
    InsistNonZero( iLenMatched, 1 ); // Should always be something matched 
    Str8 sCurWord( rpsStartWd.psz(), iLenMatched ); // Save current word, to be parsed or used as copy if no parse
	return sCurWord;
	}

BOOL CLookupProc::bLookup( const char* psz, CFieldList* pfldlst, int* piLenMatched ) // External interface for search
{
	return m_pptri[LOOK]->bSearch( psz, pfldlst, piLenMatched, WHOLE_WORD | CASE_FOLD ); // | LONGEST ); // Longest caused false fail of verify interlinear
}

/* How interlinearization works:
It always starts in the top line of a bundle, at the start of a word.
    If there is punctuation at the beginning of the word, it starts before the punctuation. 
    That is done by ShwView::Interlinearize as part of its setup.
This first thing it does is to move to the nearest starting place.
    If it is inside a word, or at the end, it moves to the beginning of the word.
It finds out how much it is processing by looking at how far it is to the next word,
    or if the first match is of a phrase, by looking for the next word after the phrase.
Once it knows how far it is to the next word, it clears the space below.
It then places the result below, loosening or tightening the later words as necessary.
*/
BOOL CLookupProc::bInterlinearize( CRecPos rpsStart, int* piLen, int iProcNum, int& iResult ) // Run interlinearization process   
{
    CField* pfldFrom = rpsStart.pfldFindInBundle( pmkrFrom() ); // Find From field
    CField* pfldTo = rpsStart.pfldFindInBundleAdd( pmkrTo() ); // Find To field, if not present, insert it
	if ( !bInitStart( rpsStart, pfldFrom ) || !bInitWordFormulas() ) // Set up start and word formulas
		return FALSE; // If one of these fails, don't continue
    CMString mksAnal( pfldTo->pmkr(), "" ); // Analysis output
    CFieldList fldlstAnal; // Field list for result of lookup and parse
    Str8 sPrevWord = sGetPrevWord( rpsStart ); // Set up previous word to check for whole word reduplication

    for ( CRecPos rpsStartCol = rpsStart; !rpsStartCol.bEndOfField(); ) // Walk through words or morphemes
        { // Position vars: rpsStartCol before begin punc, rpsStartWd after begin punc, rpsEndWd before end punc, rpsEndPunc after end punc, rpsEndCol start of next word
        CRecPos rpsStartWd = rpsStartCol;
		Str8 sLeadPunc = sGetLeadPunc( rpsStartCol, rpsStartWd ); // Set up leading punc of word, set start of word    
		CRecPos rpsEndWd = rpsStartWd;
		int iLenMatched = 0; // Length of matched word, used to move forward after parse
        Str8 sCurWord = sGetCurWord( rpsStartCol, rpsStartWd, rpsEndWd, iLenMatched ); // Get current word, set start, end, and length
		InitTimeLimit( 5 ); // 1.3bn Initialize parse time limit at every word to prevent false message // 1.4qzjm Add time limit arg because all other calls use it
        BOOL bSucc = FALSE; // General success variable
        if ( !m_bParseProc ) // If not parse, do lookup
            {
            const char* pszDebugStartWd = rpsStartWd.psz();
            bSucc = m_pptri[LOOK]->bSearch( rpsStartWd.psz(), &fldlstAnal, &iLenMatched, WHOLE_WORD | CASE_FOLD | LONGEST );
            if ( bSucc ) // If found
                {
                // It is a forced gloss if there is a parend right after iLenMatched
                const char* pszForceStart = rpsStartWd.psz() + iLenMatched;
                if ( *pszForceStart == cForceStart ) // If forced gloss start, find end
                    {
                    // Take the first forced gloss off to use it to disambig, leave the others on
                    const char* pszForceEnd = pszForceStart + 1; // Find matching force end
                    while ( *pszForceEnd && *pszForceEnd != cForceEnd ) // Move to force end
                        if ( *pszForceEnd == cForceStart ) // If we hit a force begin before an end, don't go further
                            break;
                        else
                            pszForceEnd++;
                    // This is a forced gloss if it stops at a force end char, and the force is not empty
                    int iForceLen = pszForceEnd - pszForceStart - 1;
                    if ( *pszForceEnd == cForceEnd && iForceLen > 0 ) // If forced gloss
                        {
                        Str8 sForce( pszForceStart + 1, iForceLen ); // Collect forced gloss
						// RemoveExtraSpaces( sForce ); // 1.6.4n Fix bug of false forced gloss not found message // 1.6.4p Don't change sForce
                        // Now I have to delete the forced gloss I used up, but leave any others alone
                        CRecPos rpsDel = rpsStartWd;
                        rpsDel.iChar += pszForceStart - rpsStartWd.psz(); // Get start of part to delete
                        rpsDel.pfld->Delete( rpsDel.iChar, iForceLen + 2 ); // Delete
                        // I have to add compensating spaces for the deleted portion
                        // These must be added after the end of all later forced glosses, some of which may contain spaces
                        if ( *pszForceStart == cForceStart ) // If more forced glosses coming, move past them
                            {
                            for ( pszForceEnd = pszForceStart + 1; *pszForceEnd; pszForceEnd++ ) // Move till we find a force end not immediately followed by a force begin
                                {
                                if ( *pszForceEnd == cForceEnd ) // If at end, break if next is not another start
                                    {
                                    if ( *( pszForceEnd + 1 ) == cForceStart ) // If a force end followed by a begin, move past both
                                        pszForceEnd++;
                                    else
                                        break;  
                                    }
                                else if ( *pszForceEnd == cForceStart ) // If we hit a force begin before an end, don't go further
                                    break;
                                }
                            int iMoreForceLen = pszForceEnd - pszForceStart + 1;
                            rpsDel.iChar += iMoreForceLen;  
                            // Next I need to move the matched length past the more forced glosses
                            iLenMatched += iMoreForceLen;
                            }
                        rpsDel.bSkipNonWhite(); // Move past any trailing punc
                        rpsDel.pfld->Insert( ' ', rpsDel.iChar, iForceLen + 2 ); // Insert spaces to compensate for deleted forced gloss
                        rpsDel.iChar--; // Step back to tighten right place
						if ( !pintprclst()->bWordParse() ) // 1.6.4w Fix bug of deleting aligning spaces from word parse
	                        rpsDel.Tighten( piLen, TRUE ); // Tighten to make up for removed forced gloss
                        // Next I need to use the forced gloss to choose among ambiguities
                        for ( CField* pfld = fldlstAnal.pfldFirst(); pfld; )
                            {
                            Str8 sField = pfld->psz();
							// RemoveExtraSpaces( sField ); // 1.6.4k Fix bug of false forced gloss not found message // 1.6.4p Don't change sField
                            int iForce = sField.Find( cForceStart );
                            if ( iForce > 0 )
                                sField = sField.Left( iForce );
                            CField* pfldNext = fldlstAnal.pfldNext( pfld ); // Get next before delete
							Str8 sForceCompare = sForce; // 1.6.4p Make temp for caompare
							RemoveExtraSpaces( sForceCompare ); // 1.6.4p Remove spaces to match comparison
							Str8 sFieldCompare = sField; // 1.6.4p Make temp for caompare
							RemoveExtraSpaces( sFieldCompare ); // 1.6.4p Remove spaces to match comparison
                            if ( strcmp( sForceCompare, sFieldCompare ) ) // If it didn't match, delete it and move to next, else just move to next // 1.6.4p 
                                fldlstAnal.Delete( &pfld );
                            pfld = pfldNext;    
                            }
                        if ( fldlstAnal.lGetCount() == 0 ) // If no matches, complain and fail
                            {
							Str8 sMsg = _("Forced value not in lexicon:"); // 1.5.0fg 
							sMsg = sMsg + " " + sForce; // 1.5.0fg 
							AfxMessageBox( sMsg ); // 1.4gd
                            mksAnal = m_sFailMark;
                            }
						if ( fldlstAnal.lGetCount() > 1 ) // 1.4ywn Fix bug of extra ambiguity box from duplicate box, if more than one result, delete the unneeded ones
							{
							CField* pfldT = fldlstAnal.pfldFirst(); // 1.4ywn Get first
							pfldT = fldlstAnal.pfldNext( pfldT ); // 1.4ywn Get second
							fldlstAnal.DeleteRest( pfldT ); // 1.4ywn Delete second and all after it
							}
                        }       
                    }
                if ( fldlstAnal.lGetCount() > 0 ) // If there is an analysis, disambig and put it in analysis
					{
					if ( iResult != 1 ) // 5.99y If this is lookup below a failed parse, don't ask user to disambig, just use first one
						{
						if ( !bDisambigLookup( rpsStartWd, &fldlstAnal ) ) // Have user disambig
							return FALSE; // If cancel, stop interlinearizing
						}
					// See if there is forced gloss on the output    
					Str8 sOutput = fldlstAnal.pfldFirst()->psz(); // Get output
					int iForceStart = sOutput.Find( cForceStart );
					if ( iForceStart > 0 ) // If there is a force start
						{
						Str8 sForce = sOutput.Mid( iForceStart );
						sOutput = sOutput.Left( iForceStart );
						const char* pszForceDebug = sForce;
						const char* pszOutputDebug = sOutput;
						// Next I need to insert the force up in the input line at iLenMatched, if there isn't already a forced gloss there
						int iLenForce = sForce.GetLength(); // Get length of force
						CRecPos rpsInsert = rpsStartWd; // Set up insert place
						rpsInsert.iChar += iLenMatched;                                          
						const char* pszInsert = rpsInsert.psz();
						if ( !( *( pszInsert - 1 ) == cForceEnd ) )
							{
							if ( pintprclst()->bWordParse() ) // 1.6.4s Use different Loosen for WordParse
								{
								rpsInsert.LoosenWordParse( iLenForce, TRUE ); // Loosen as needed // 1.6.4s 
								}
							else
								{
								rpsInsert.Loosen( iLenForce, TRUE ); // Loosen as needed
								}
							*piLen += iLenForce; // Compensate length
							iLenMatched += iLenForce; // Compensate length matched
							CRecPos rpsTrailPunc = rpsInsert; // See if trailing punc
							if ( rpsTrailPunc.bSkipNonWhite() ) // If trailing punc, move it up out of the way
								{
								int iLenTrailPunc = rpsTrailPunc.iChar - rpsInsert.iChar; // Get length of punc
								char* psz = (char*) rpsInsert.psz() + iLenForce; // Get writable pointer to place to move to
                                memmove(psz, rpsInsert.psz(), iLenTrailPunc);
								}
                            rpsInsert.pfld->Overlay( sForce, rpsInsert.iChar, 0 ); // Overlay on the spaces
							}
						int iBreakPlace = 0;
						}
					mksAnal = sOutput; // Put output in analysis
					}
                }           
            else // If not found, show failure marks or apply CC table
                {
                mksAnal = ""; // Init empty so we can append     
                if ( rpsStartWd.bIsDigit() // If this is a number or fail mark or plain hyphen, pass it on down 5.99r use bIsDigit
 						|| rpsStartWd.bMatch( m_sFailMark )
						|| ( bIsMorphBreakChar( sCurWord[0] ) && sCurWord.GetLength() == 1) )
					mksAnal = sCurWord;
                else
                    {
                    if ( m_bInsertFail && !rpsStartWd.bMatch( m_sFailMark ) ) // If inserting in lexicon, ask about jump insert
                        {
                        CShwView* pview = Shw_papp()->pviewActive();
                        pview->SelectAmbig( rpsStartWd ); // Select ambiguity containing rps, scroll it up from bottom
						if ( m_bShowWord ) // 1.4qzmb If insert whole word, don't show dialog box that asks for that same information
							{
							Str8 sInsert = sCurWord; // 1.4qzmd Get string of current word for case adjustment
							const char* pszInsert = sInsert; // 1.4qzmd
					        const CMChar* pmch = pfldFrom->plng()->pmchNextSortingUnit( &pszInsert ); // 1.4qzmd Get first character of string
			                const CMChar* pmchLower = pmch; // 1.4qzmd
							if ( pmch->bUpperCase() ) // 1.4qzmd If it is upper case, replace with lower case equivalent
								{
								pmchLower = pmch->pmchOtherCase(); // 1.4qzmd Get lower case equivalent
								Str8 sUpper = pmch->sMChar(); // 1.4qzmd Get string of upper case letter
								Str8 sLower = pmchLower->sMChar(); // 1.4qzmd Get string of lower case letter
								sInsert = sInsert.Mid( sUpper.GetLength() ); // 1.4qzmd Delete upper case letter from front
								sInsert = sLower + sInsert; // 1.4qzmd Add lower case letter to front
								}
//							pview->EditJumpTo( TRUE, FALSE, sInsert, pfldFrom->pmkr() ); // Do jump insert of current word // 1.4qzmb // 1.4ywa 
							pview->EditJumpTo( FALSE, FALSE, sInsert, pfldFrom->pmkr() ); // Do jump insert of current word // 1.4qzmb // 1.4ywa Fix U bug of lookup inserting into lexicon without asking
							pview->ClearSelection(); // Clear the selection begin // 1.4qzmb Clear selection for now, as it doesn't stay correct
							*piLen = 0; // Don't move cursor forward // 1.4qzme Fix bug of skipping word on interlinearize return from jump
							return FALSE; // 1.4qzmb Return false to say that interlinearization should stop
							}
						else
							{
							int iAns = IDNO; // 1.6.4t Make iAns available for auto insert
							Str8 sInsert; // 1.6.4t Make sInsert available for auto insert
							BOOL bWordParse = pintprclst()->bWordParse(); // 1.6.4t Word parse bool for interlinearize after auto insert
							Str8 sFileName = sUTF8( pview->pdoc()->GetTitle() ); // 1.6.4u Get file name
							if ( sFileName.Find( "WordParse" ) >= 0 ) // 1.6.4u If from WordParse, default to insert
								bWordParse = TRUE; // 1.6.4u 
							if ( bWordParse ) // 1.6.4t If WordParse insert without asking
								{
								iAns = IDYES; // 1.6.4t Set to yes for WordParse
								sInsert = sCurWord; // 1.6.4t Default insert to current word
								if ( rpsStartCol.iChar == 0 ) // 1.6.4zm If first word in field, lower case of word
									{
									const char* pszInsert = sInsert; // 1.6.4zf Lower case of word inserted into WordParse file
									const CMChar* pmch = pfldFrom->plng()->pmchNextSortingUnit( &pszInsert ); // 1.4qzmd Get first character of string
									const CMChar* pmchLower = pmch; // 1.4qzmd
									if ( pmch->bUpperCase() ) // 1.4qzmd If it is upper case, replace with lower case equivalent
										{
										pmchLower = pmch->pmchOtherCase(); // 1.4qzmd Get lower case equivalent
										Str8 sUpper = pmch->sMChar(); // 1.4qzmd Get string of upper case letter
										Str8 sLower = pmchLower->sMChar(); // 1.4qzmd Get string of lower case letter
										sInsert = sInsert.Mid( sUpper.GetLength() ); // 1.4qzmd Delete upper case letter from front
										sInsert = sLower + sInsert; // 1.4qzmd Add lower case letter to front
										}
									}
								}
							else
								{
								CLookFailDlg dlg( sCurWord, pfldFrom->plng() );
								iAns = dlg.DoModal(); // 1.6.4t 
								if ( iAns == IDYES ) // If yes, set insert to dlg result // 1.6.4t 
									sInsert = dlg.pszKey(); // 1.6.4t 
								}
							if ( iAns == IDYES ) // If yes, do jump insert and stop so they can edit inserted entry
								{
								pview->EditJumpTo( TRUE, FALSE, sInsert, pfldFrom->pmkr() ); // Do jump insert of current word
								}
							pview->ClearSelection(); // Clear the selection begin
							if ( iAns != IDNO ) // If cancel or insert, stop interlinearizing, otherwise (no) go on down to insert fail mark
								{
								*piLen = 0; // Don't move cursor forward
								return FALSE;
								}
							}
                        }
                    if ( m_bShowFailMark ) // If showing fail mark, show it
                        mksAnal = m_sFailMark;
                    if ( m_bShowWord ) // If showing word, attach word
                        mksAnal += sCurWord;
                    }
#if UseCct
                if ( bApplyCC() && bCCLoaded() ) // If applying a CC table to failures, do it now
                    {
                    char pszCCOutBuffer[ 1000 ]; // Output buffer for CC
                    int iCCOutLen = 1000;
					mksAnal = " " + mksAnal + " "; // Add spaces before and after the word
                    if ( m_cct.bMakeChanges( mksAnal, mksAnal.GetLength() + 1,
                            pszCCOutBuffer, &iCCOutLen ) )
                        {   
                        pszCCOutBuffer[ iCCOutLen ] = '\0';
                        mksAnal = pszCCOutBuffer;   
                        }
					mksAnal.TrimBothEnds(); // Trim off spaces
                    }
#endif
				}
            }
        else // Else (parsing) do parse
            {
			BOOL bParseSucc = TRUE; // Remember if parse failed for disambig
            // Look up whole word in parse database for SH2 style parse
            if ( m_bSH2Parse && !m_pptri[LOOK]->bIsEmpty() )
                bSucc = m_pptri[LOOK]->bSearch( rpsStartWd.psz(), &fldlstAnal, &iLenMatched,
                        WHOLE_WORD | CASE_FOLD | LONGEST );
            // If not in parse database, parse with conjoined affixes       
            if ( !bSucc && !m_pptri[ROOT]->bIsEmpty() ) // If no match and there is a lexicon, try parsing a word
                {
                // Check for full word redup of previous word
                bSucc = m_pptri[ROOT]->bRedup( sCurWord, sPrevWord, &fldlstAnal, &iLenMatched ); // If the root is reduped, add it to analyses          
                // Look up phrase or whole word in root database
                if ( !bSucc && !m_bSH2Parse )
                    bSucc = m_pptri[ROOT]->bSearch( rpsStartWd.psz(), &fldlstAnal, &iLenMatched, WHOLE_WORD | CASE_FOLD | LONGEST );
				BOOL bRootSucc = bSucc; // 1.3bc Remember that root succeeded
				BOOL bPhrase = ( iLenMatched > sCurWord.GetLength() );
                if ( !bRootSucc || ( bShowAllParses && !m_bOverrideShowAll && !bPhrase ) ) // 1.3bg Add option to not do longer override on show all parses // 1.3bc Don't try to parse if longer root is a phrase // 1.3bc Add option for longer root to not override // If no whole root success, parse a single word
                    {           
                    unsigned int iSmallest = sCurWord.GetLength(); // Start with full length as smallest seen
                    bSucc = bParseWord( sCurWord, &fldlstAnal, iSmallest ); // Parse the word
					bSucc = ( bSucc || bRootSucc ); // 1.3bc If parse faile, but root had succeeded, don't do fail parse
                    if ( !bSucc ) // If failure, do second pass to show how far it got
                        {
                        if ( m_bShowWord ) // If showing the word, don't do fail parse
                            {
                            mksAnal = sCurWord;
                            if ( m_bShowFailMark ) // If also showing fail mark, attach it to word
                                mksAnal = m_sFailMark + mksAnal;
                            }
                        else
                            {
                            mksAnal = m_sFailMark;
                            if ( bIsMorphBreakChar( sCurWord[0] ) // Don't try again if it is already an affix
                                        || bIsMorphBreakChar( sCurWord[ sCurWord.GetLength() - 1 ] )
                                        || ( __isascii( sCurWord[0] ) && isdigit( sCurWord[0] ) ) // Also don't try again if it is a number or fail mark
										|| rpsStartWd.bMatch( m_sFailMark ) )
								;
                            else
                                {
								bParseSucc = FALSE; // Remember that parse failed for disambig
                                bSucc = bParseWord( sCurWord, &fldlstAnal, iSmallest, TRUE ); // Should succed
                                }
                            }
                        if ( m_bInsertFail ) // If inserting in lexicon, ask about jump insert
                            {
                            CShwView* pview = Shw_papp()->pviewActive();
                            pview->SelectAmbig( rpsStartWd ); // Select ambiguity containing rps, scroll it up from bottom
                            mksAnal = sCurWord;
				            if ( m_bShowRootGuess ) // If showing root guess, offer it for insertion
								{
								if ( fldlstAnal.lGetCount() > 0 ) // There should be a root guess analysis
									{
									mksAnal = fldlstAnal.pfldFirst()->psz(); // Put field in analysis
									int iStartWd = mksAnal.Find( m_sFailMark ) + 1; // Find fail mark
									if ( iStartWd > 0 ) // It should be found
										{
										mksAnal = mksAnal.Mid( iStartWd ); // Cut off before fail mark
										mksAnal.Append( " " ); // Add space to be sure there is one to find // 1.4qzkb
										int iEndWd = mksAnal.Find( ' ' );
										mksAnal = mksAnal.Left( iEndWd ); // Extract up to space
										}
									}
								}
                            CLookFailDlg dlg( mksAnal, pfldFrom->plng() );
                            int iAns = dlg.DoModal();
							if ( iAns == IDC_WholeWord ) // If whole word, do jum insert of whole word
                                pview->EditJumpTo( TRUE, FALSE, sCurWord, pfldFrom->pmkr() ); // Do jump insert of current word
                            else if ( iAns == IDYES ) // If yes, do jump insert and stop so they can edit inserted entry
								{
								mksAnal = dlg.pszKey();
                                pview->EditJumpTo( TRUE, FALSE, mksAnal, pfldFrom->pmkr() ); // Do jump insert of current word
								}
                            pview->ClearSelection(); // Clear the selection begin
                            if ( iAns != IDNO ) // If cancel or insert, stop interlinearizing, otherwise (no) go on down to insert fail mark
                                {
                                *piLen = 0; // Don't move cursor forward
                                return FALSE;
                                }
                            }
                        }
                    }       
                }
            CFieldList fldlstExcluded;
			long lfldlstAnalCount = fldlstAnal.lGetCount(); // 1.3bf Limit parsing time
            if ( bSucc && lfldlstAnalCount > 0 ) // If success, disambig with word formulas
                {
				// If word formulas, do lookup of each morpheme to add gloss and part of speech
				if ( m_pwdfset && m_pwdfset->bEnabled() )
					{
					if ( !bTimeLimitExceeded() ) // 1.3bk Don't eliminate duplicates if time limit already exceeded
					    fldlstAnal.EliminateDuplicates( TRUE ); // Eliminate duplicates // 1.3bm Do time limit on eliminate duplicates); // Eliminate duplicates
					lfldlstAnalCount = fldlstAnal.lGetCount(); // 1.3bf Limit parsing time
					CInterlinearProc* pintprcNext = pintprclst()->pintprcNext( this );
					if ( pintprcNext && pintprcNext->bLookupProc() ) // If a lookup next, do it on the field list
						pintprcNext->AddGlossesToList( &fldlstAnal, -1 );
					lfldlstAnalCount = fldlstAnal.lGetCount(); // 1.3bf Limit parsing time
                    m_pwdfset->MatchParses( &fldlstAnal, &fldlstExcluded, bShowAllParses ); // Move fails to excluded list
					}
				}
			lfldlstAnalCount = fldlstAnal.lGetCount(); // 1.3bf Limit parsing time
            if ( bSucc && lfldlstAnalCount > 0 ) // If success, disambig and put in analysis
				{
                if ( bParseSucc && !bDisambigParse( rpsStartWd, &fldlstAnal, &fldlstExcluded, bShowAllParses ) ) // Have user disambig
                    return FALSE; // If cancel, stop interlinearizing
                mksAnal = fldlstAnal.pfldFirst()->psz(); // Put field in analysis
				const char* pszAnal = fldlstAnal.pfldFirst()->psz();
				if ( *pszAnal == *(const char*)m_sFailMark // If the selected one failed word formulas, remove fail mark
						&& *(pszAnal + 1) == ' ' 
						&& !bIsMorphBreakChar( *(pszAnal + 2) ) ) // 5.97c Fix bug of not showing fail if all suffixes
					mksAnal.Delete( 0, 2 ); // Delete star
                }   
            else if ( rpsStartWd.bIsDigit() // If this is a number or fail mark or plain hyphen, pass it on down
						|| rpsStartWd.bMatch( m_sFailMark )
						|| ( bIsMorphBreakChar( sCurWord[0] ) && sCurWord.GetLength() == 1) )
				mksAnal = sCurWord;
			if ( !bParseSucc ) // 5.99y If parse failed, set result to show that
				iResult = 1;
            } // End parse block
            
        InsistNonEmpty( mksAnal, sCurWord );  // Should be filled in by now, but just in case, copy input  word as it used to do

        // Reset word end for length matched in lookup or parse
        rpsEndWd.SetPosEx( rpsStartWd.iChar + iLenMatched ); // Set word end correct for actual match length    

		// if ( !m_bParseProc ) // 11-6-97 AB Don't add hyphens in parsing. This allows underlying of a joined form to be a separate word.
			{
			// If hyphen needed on front or back, insert one
			char cStartChar = *rpsStartWd.psz(); // 1.4ywp 
			int iLenMorph = iGetLenMorph( Str8( rpsStartWd.psz(), iLenMatched ) );
			char cEndChar = *rpsStartWd.psz( rpsStartWd.iChar + iLenMorph - 1 );
			CLangEnc* plngFrom = pfldFrom->pmkr()->plng(); // 1.5.8e 
			CLangEnc* plngTo = pfldTo->pmkr()->plng(); // 1.5.8e 
			BOOL bFromRTL = plngFrom->bRightToLeft(); // 1.5.8e 
			BOOL bToRTL = plngTo->bRightToLeft(); // 1.5.8e 
			if ( plngFrom->bRightToLeft() == plngTo->bRightToLeft() ) // 1.5.8e If interlinear opposite rtl direction, don't add hyphens
				{
				if ( bIsMorphBreakChar( cStartChar ) && !bIsMorphBreakChar( mksAnal.GetChar( 0 ) ) )
					mksAnal = cMorphBreakChar( cStartChar, rpsStartWd.pfld->plng() ) + mksAnal; // ALB 5.96f Change to insert first morph break char instead of copy, to fix IPA font problem // 1.4ywp Change interlinear to copy down same morph break char in Unicode languages
				if ( bIsMorphBreakChar( cEndChar ) && !bIsMorphBreakChar( mksAnal.GetChar( mksAnal.GetLength() - 1 ) ) ) // If hyphen needed on back, insert one
					mksAnal = mksAnal + cMorphBreakChar( cEndChar, rpsStartWd.pfld->plng() ); // 1.4ywp 
				}
			}

		if ( m_bSH2Parse ) // If Sh2 mode, add spaces like Sh2 did
			mksAnal.AddSuffHyphSpaces( sMorphBreakChars(), cForceStart, cForceEnd ); // Add a space before each suffix hyphen that doesn't have one

        // If keep capitalization, recapitalize if necessary
        if ( m_bKeepCap )
            {
            const char* psz = rpsStartWd.psz();
            const CMChar* pmch = NULL;
            if ( pfldFrom->plng()->bMChar(&psz, &pmch) && pmch->bUpperCase() && pmch->bHasOtherCase() ) // If upper case multichar in input and it has other case
                {
                psz = mksAnal; // Get start of output
                if ( pfldTo->plng()->bMChar(&psz, &pmch) && !pmch->bUpperCase() && pmch->bHasOtherCase() ) // If lower case multichar in output and it has other case
                    { // Recapitalize output
                    pmch = pmch->pmchOtherCase(); // Change to upper case
                    mksAnal = pmch->sMChar() + psz; // Make upper case version of anal output string by adding upper case letter to rest of string
                    }
                }
            }

        // Find end of punc and next column, and save end punc          
        CRecPos rpsEndPunc = rpsEndWd; // End of end punc
		rpsEndPunc.bSkipPunc(); // Move past punc
        int iPuncLen = rpsEndPunc.psz() - rpsEndWd.psz(); // Get length of trailing punc
        Str8 sTrailPunc( rpsEndWd.psz(), iPuncLen ); // Get trailing punc
        CRecPos rpsEndCol = rpsEndPunc; // Ending column, start of next word
		if ( rpsEndCol.bIsNonWhite() && m_bParseProc ) // If no white space before next word during parse, insert a space to allow alignment
			rpsEndCol.pfld->Insert( ' ', rpsEndCol.iChar );
        rpsEndCol.bSkipWhite(); // Move past spaces between words

        // If keep punctuation, add punc to output
        if ( m_bKeepPunc )
			{
#if UseCct
            if ( bApplyCC() && bCCLoaded() ) // If applying a CC table to failures, apply it to punc as well
                {
                char pszCCOutBuffer[ 1000 ]; // Output buffer for CC
                int iCCOutLen = 1000;
                if ( m_cct.bMakeChanges( sLeadPunc, sLeadPunc.GetLength() + 1,
                        pszCCOutBuffer, &iCCOutLen ) )
                    {   
                    pszCCOutBuffer[ iCCOutLen ] = '\0';
                    sLeadPunc = pszCCOutBuffer;   
                    }
				iCCOutLen = 1000; // Restore buffer length after it was changed above
                if ( m_cct.bMakeChanges( sTrailPunc, sTrailPunc.GetLength() + 1,
                        pszCCOutBuffer, &iCCOutLen ) )
                    {   
                    pszCCOutBuffer[ iCCOutLen ] = '\0';
                    sTrailPunc = pszCCOutBuffer;   
                    }
                }    
#endif
            mksAnal = sLeadPunc + (const char*)mksAnal + sTrailPunc; // Add punc // 1.2gs
			}
            
        // Add punctuation length to length matched 
        iLenMatched += sLeadPunc.GetLength(); // Add leading punc len to matched length
        iLenMatched += sTrailPunc.GetLength(); // Add punc to matched length
		// Clear space below word before inserting first time
		BOOL bGivenPrev = FALSE; // 1.5.1nf 
		CInterlinearProc* pintprcPrev = pintprclst()->pintprcPrev( this ); // 1.5.1nf 
		if ( pintprcPrev && pintprcPrev->iType() == CProcNames::eGiven ) // If a given previous, note that // 1.5.1nf 
			bGivenPrev = TRUE; // 1.5.1nf 
		if ( iProcNum == 0 || ( iProcNum == 1 && bGivenPrev ) ) // If first process, clear space below
			{                                                                            
			CRecPos rps = rpsStartCol; 
			rps.pfld = rps.prec->pfldPrev( pfldTo );
			rps.ClearBundleBelow( rpsEndCol ); // Clear space below up to end column
			}
		// Insert result into To field of record at current position
		int iLenInsert = mksAnal.GetLength() + 1;
		int iLenSpace = rpsEndCol.iChar - rpsStartCol.iChar;
		int iLenNeeded = iLenInsert - iLenSpace;
		if ( iLenNeeded > 0 )
			{
			if ( pintprclst()->bWordParse() ) // 1.6.4s Use different Loosen for WordParse
				rpsEndWd.LoosenWordParse( iLenNeeded, TRUE ); // Loosen as needed // 1.6.4s 
			else
				rpsEndWd.Loosen( iLenNeeded, TRUE ); // Loosen as needed
			rpsEndCol.SetPos( rpsEndCol.iChar + iLenNeeded ); // Compensate end column
			*piLen += iLenNeeded; // Compensate length
			}
		pfldTo->Overlay( mksAnal, rpsStartCol.iChar, 0 ); // Overlay on the spaces
            
        // If current word is done, return
        if ( iProcNum == 0 ) // If first process, set end for other processes to go on down
            {
            if ( !rpsEndCol.bEndOfField() ) // If not at end of field, use actual length
                {
                *piLen = rpsEndCol.iChar - rpsStart.iChar; // Set length covered by process
                CField* pfldNext = rpsEndCol.prec->pfldNext( rpsEndCol.pfld );
                if ( !bAdapt() ) // || pfldNext->GetLength() > rpsEndCol.iChar ) // If adapting first time, don't stop, so rearrange process can see multiple words
                    return TRUE; // Stop first process after each word or phrase
                }
            else // Else (at end of field), use something large so lower processes will finish line
                {
                *piLen = 10000; // Set length to something large
                return TRUE; // Stop first process at its end
                }
            }
        else if ( rpsEndCol.bEndOfField() // 1.1ah Watch for end reached to fix bug of not continuing after first word if first process is a lookup
				|| rpsEndCol.iChar >= rpsStart.iChar + *piLen ) // Stop lower process when current word has been done
            return TRUE;
        // Else (more morphemes in lower process), move to next morpheme
        rpsStartCol = rpsEndCol;
        }
    *piLen = 10000; // Set length to something large so lower processes will finish line
    return TRUE;    
}

BOOL bIsAllDigits( LPCSTR psz ) // Check first word in string for all digits
	{
	for ( ; *psz && *psz != ' ' && *psz != '\n'; psz++ )
		if ( !( __isascii( *psz ) && isdigit( *psz ) ) )
			return FALSE;
	return TRUE;
	}

// Check if this field is from a lookup or parse process, if so, return process
CInterlinearProc* pintprcLookupOrParseToThis( CRecPos rpsTo, CShwView* pview ) 
{
	if ( !rpsTo.bInterlinear() ) // If not in interlinear, fail
		return NULL;
    CInterlinearProcList* pintprclst = pview->GetDocument()->pintprclst(); // Get interlinear proc list
	for ( CInterlinearProc* pintprc = pintprclst->pintprcFirst(); pintprc; 
			pintprc = pintprclst->pintprcNext( pintprc ) )
		if ( pintprc->pmkrTo() == rpsTo.pfld->pmkr() 
				&& ( pintprc->bLookupProc() || pintprc->bParseProc() ) )
			return pintprc;
	return NULL;
}

BOOL CLookupProc::bSpellCheckField( CRecPos rpsCur, CRecPos rpsEnd ) // Spell check current field using current process
{
    ASSERT( !rpsEnd.pfld || rpsCur.prec == rpsEnd.prec );
    CShwView* pview = Shw_papp()->pviewActive();
    ASSERT( pview );
	CRecPos rpsCurTo = rpsCur; // Remember where cur started
	if ( pintprcLookupOrParseToThis( rpsCur, pview ) ) // If lookup or parse to this, move cur to source
		{
		CField* pfldFrom = rpsCur.pfldFindInBundle( pmkrFrom() );
		if ( !pfldFrom ) // 5.99zb Protect against odd lookup field with no source
			return TRUE;
		rpsCur.pfld = pfldFrom;
		}
    while ( !rpsEnd.pfld || rpsCur < rpsEnd ) // While more words to check, check a word and move to next
        {
		CMString mksAnal( pmkrFrom(), "" ); // Analysis output
		CFieldList fldlstAnal; // Field list for result of lookup and parse
		Str8 sCurWord; // Current word, used to remember word for possible redup
		Str8 sPrevWord; // Remember previous word for possible redup
		CRecPos rpsPrevWd = rpsCur; // Temp for prev word start
		if ( rpsPrevWd.bMoveToPrevWdWfc() )
			{
			CRecPos rpsPrevEnd = rpsPrevWd; // Temp for word end
			rpsPrevEnd.bSkipWdChar(); // Skip to end of word
			sPrevWord = Str8( rpsPrevWd.psz(), rpsPrevEnd.iChar - rpsPrevWd.iChar ); // Remember previous word for possible redup
			}
        // Check current word
		if ( !( ( ( *(rpsCur.psz() ) == '*') && !rpsCur.bFirstInterlinear() ) // 5.99q Fix bug of cursor wrong position after fail mark // 5.99r Fix bug of stopping at *2*
				|| rpsCur.bIsDigit() ) ) // If fail mark or digit, don't skip over it // 5.99r use bIsDigit
			rpsCur.bSkipWdSep(); // Skip to actual stuff
        CRecPos rpsEndWd = rpsCur; // Temp for word end
		if ( rpsEndWd.bEndOfField() ) // In case this gets to end, return success
			return TRUE;
		if ( *(rpsEndWd.psz()) == '*' // 5.99q Fix bug of cursor wrong position after fail mark
				|| rpsEndWd.bIsDigit() ) // 5.99r use bIsDigit
			rpsEndWd.bSkipNonWhite(); // Skip over fail mark or digits
		else
			rpsEndWd.bSkipWdChar(); // Skip to end of word
        int iLenMatched = rpsEndWd.iChar - rpsCur.iChar; // Length of match for insert
		if ( iLenMatched == 0 )
			iLenMatched = 1; // Should always be something matched
        mksAnal = Str8( rpsCur.psz(), iLenMatched ); // Put one word into anal string, to be parsed or used as copy if no parse
        sCurWord = mksAnal; // Remember current word for possible redup
		InitTimeLimit( 5, FALSE ); // Initialize parse time limit at every word to prevent false message // 1.4qzjm Prevent parse timeout message on interlinear check
		BOOL bSucc = FALSE;
		const char* pszCur = rpsCur.psz();
		CRecPos rpsNextChar = rpsCur;
		rpsNextChar.iChar++;
		if ( bIsMorphBreakChar( *pszCur ) && rpsNextChar.bIsWdEnd() // If hyphen alone, accept it
				|| ( __isascii( *pszCur ) && isdigit( *pszCur ) ) ) // If digits, accept it // 5.99j Fix to allow anything that starts with number, like interlinear, this does number in parends and pairs of numbers with hyphen between
			bSucc = TRUE;
		else
			{
			if ( bLookupProc() ) // If lookup to this, verify lookup
				{
				iLenMatched = iInterlinearLookupCheck( rpsCurTo );
				bSucc = iLenMatched;
				}
			else // Else verify parse
				{
				CRecPos rpsFrom = rpsCur; // Get string that was parsed to produce this result
				if ( rpsCur.bInterlinear() ) // 5.99b Fix bug of crash if first line of record is in parsed language
					rpsFrom.pfld = rpsCur.pfldFindInBundle( pmkrFrom() );
					 // Look up whole word in parse database
				bSucc = m_pptri[LOOK]->bSearch( rpsFrom.psz(), &fldlstAnal, &iLenMatched,
							WHOLE_WORD | CASE_FOLD | LONGEST );
						// If not in parse database, parse with conjoined affixes       
				if ( !bSucc && !m_pptri[ROOT]->bIsEmpty() ) // If no match and there is a lexicon, try parsing a word
					{          
					bSucc = m_pptri[ROOT]->bRedup( sCurWord, sPrevWord, &fldlstAnal, &iLenMatched ); // If the root is reduped, add it to analyses          
							 // Look up phrase or whole word in root database
					if ( !bSucc && !m_bSH2Parse )
						bSucc = m_pptri[ROOT]->bSearch( rpsFrom.psz(), &fldlstAnal, &iLenMatched,
								WHOLE_WORD | CASE_FOLD | LONGEST );
					if ( !bSucc ) // If no whole root success, parse a single word
						{           
						unsigned int iSmallest = mksAnal.GetLength(); // Start with full length as smallest seen
						bSucc = bParseWord( mksAnal.psz(), &fldlstAnal, iSmallest, FALSE, TRUE ); // Parse the word // 1.4qzjm
						CFieldList fldlstExcluded;
						if ( bSucc && fldlstAnal.lGetCount() > 0 ) // If success, disambig with word formulas
							{
							// If word formulas, do lookup of each morpheme to add gloss and part of speech
							if ( m_pwdfset && m_pwdfset->bEnabled() )
								{
								if ( !bTimeLimitExceeded() ) // 1.3bk Don't eliminate duplicates if time limit already exceeded
									fldlstAnal.EliminateDuplicates( TRUE ); // Eliminate duplicates // 1.3bm Do time limit on eliminate duplicates); // Eliminate duplicates
								CInterlinearProc* pintprcNext = pintprclst()->pintprcNext( this );
								if ( pintprcNext && pintprcNext->bLookupProc() ) // If a lookup next, do it on the field list
									pintprcNext->AddGlossesToList( &fldlstAnal, -1 );
								long lNumAnal = fldlstAnal.lGetCount();
								m_pwdfset->MatchParses( &fldlstAnal, &fldlstExcluded, bShowAllParses ); // Move fails to excluded list
								lNumAnal = fldlstAnal.lGetCount();
								bSucc = fldlstAnal.lGetCount() > 0; // Success if at least one left
								}
							}
						else
							bParseWord( sCurWord, &fldlstAnal, iSmallest, TRUE, TRUE ); // Generate the fail parse // 1.4qzjm
						}
					}
				if ( rpsCurTo.bInterlinear() ) // If this is interlinear and parse succeeded, verify the output of parse
					{
					if ( !bSucc ) // 5.97f Set up for verify or check
						iLenMatched = sCurWord.GetLength();
					CRecPos rpsEndParse = rpsFrom; // Get full extent of what was parsed
					rpsEndParse.iChar += iLenMatched;
					rpsEndParse.bSkipPunc();
					rpsEndParse.bSkipWhite();
					iLenMatched = rpsEndParse.iChar - rpsFrom.iChar; // Adjust length to include spaces up to next word
					// Get the existing parse into a string
					CRecPos rpsExistingParse = rpsFrom; // Start at cur pos
					rpsExistingParse.bSkipNonWhiteLeft(); // 1.1tk Fix bug of interlinear check fail on two open punc marks // Move back to cover any leading punc
					rpsExistingParse.pfld = rpsCurTo.pfld; // Move to same pos in existing parse field
					Str8 sExistingParse = rpsExistingParse.psz(); // Collect existing parse
					int iLenRest = rpsFrom.iFldLenWithoutNL() - rpsFrom.iChar; // Get length of rest of parsed line
					if ( iLenMatched < iLenRest ) // If parse went less than all the way to the end, cut existing parse off after matched length
						{
						sExistingParse = sExistingParse.Left( iLenMatched );
						}
					sExistingParse.TrimRight();
					int iSpace = 0;
					while ( ( iSpace = sExistingParse.Find( "  " ) ) >= 0 ) // Eat out all extra spaces
						{
						Str8 sLeft = sExistingParse.Left( iSpace );
						Str8 sRight = sExistingParse.Mid( iSpace + 1 );
						sExistingParse = sLeft + sRight;
						}
					if ( bSucc ) // If parse succeeded, verify it
						{ 
						// If one of the parse results matches the desired output, succeed
						bSucc = FALSE; // Default to false unless a match is found
						long lNumAnals = fldlstAnal.lGetCount();
						for ( CField* pfld = fldlstAnal.pfldFirst(); pfld; pfld = fldlstAnal.pfldNext( pfld ) )
							{
							Str8 sParseOut = pfld->psz();
							RemoveExtraSpaces( sParseOut ); // 1.1tv Fix bug of extra spaces causing interlinear verify to fail
							int iForceStart = sParseOut.Find( cForcedGlossStartChar() ); // Remove all forced glosses
							while ( iForceStart > 0 ) 
								{
								int iForceEnd = sParseOut.Find( cForcedGlossEndChar() ); 
								if ( iForceEnd < iForceStart ) // If for some reason end isn't found, don't try to remove it
									break;
								Str8 sBeforeForce = sParseOut.Left( iForceStart );
								Str8 sAfterForce = sParseOut.Mid( iForceEnd + 1 );
								sParseOut = sBeforeForce + sAfterForce;
								iForceStart = sParseOut.Find( cForcedGlossStartChar() ); // See if another forced gloss
								}
							if ( sParseOut.Find( "* " ) == 0 ) // If parse has star, remove it because word formulas have kept it in but put in the star
								sParseOut = sParseOut.Mid( 2 );
							RemoveExtraSpaces( sParseOut );
							sParseOut = Recapitalize( rpsFrom, rpsCurTo, sParseOut ); // Recapitalize if necessary // 1.6.4zg 
							sExistingParse = RemovePunc( rpsFrom, rpsCurTo, sExistingParse ); // Remove punc if necessary // 1.6.4zg 
							if ( sExistingParse == sParseOut ) // If parse is found, break out as a success
								{
								bSucc = TRUE;
								break;
								}
							}
						}
					else // Else, parse failed, if skipping failures, accept this one
						{
						if ( pview->iVerifyPassNumber() == 1 ) // If pass 1, skip all fails that still fail 5.97x
							{
							if ( sExistingParse.Find( "*" ) >= 0 ) // 5.97f Fix bug of missing new fails
								{
								pview->SetVerifySkippedFail( TRUE ); // 5.96zd Remember that we skipped a fail so we need a second pass
								bSucc = TRUE;
								}
							}
						else if ( pview->iVerifyPassNumber() == 2 ) // If pass 2, skip all fails that give same result 5.97x
							{
							if ( fldlstAnal.lGetCount() > 0 ) // If there is a fail result
								{
								Str8 sParseOut = fldlstAnal.pfldFirst()->psz(); // If it is the same as before, skip it
								RemoveExtraSpaces( sParseOut ); // 1.1tv Fix bug of extra spaces causing interlinear verify to fail
								int iForcedGlossStart = -1; // If forced gloss found, remove it
								do {
									iForcedGlossStart = sParseOut.Find( cForcedGlossStartChar() );
									if ( iForcedGlossStart >= 0 )
										{
//										int iForcedGlossEnd = sParseOut.Find( cForcedGlossEndChar(), iForcedGlossStart ); // 1.4hen Change Find with start pos, not in Str8
										int iForcedGlossEnd = FindAfter( sParseOut, cForcedGlossEndChar(), iForcedGlossStart ); // 1.4hen Change Find with start pos, not in Str8
										if ( iForcedGlossEnd < 0 )
											break;
//										sParseOut.Delete( iForcedGlossStart, iForcedGlossEnd - iForcedGlossStart + 1 ); // 1.4hep Change Delete, Str8 doesn't have it
										sParseOut = sParseOut.Left( iForcedGlossStart ) + sParseOut.Mid( iForcedGlossEnd + 1 ); // 1.4hep
										}
									} while ( iForcedGlossStart >= 0 );
								if ( sExistingParse == sParseOut )
									{
									pview->SetVerifySkippedFail( TRUE ); // Remember that we skipped a fail so we need a third pass
									bSucc = TRUE;
									}
								}
							}
						}
					}
				if ( !bSucc ) // 5.97g If parse fails, highlight word that failed to parse // 5.99k Do this only on parse, let lookup highlight bad annotation
					rpsCurTo = rpsCur;
				}
			}
        if ( !bSucc ) // If spell check fails, select word and return
            {
                    // Select the failed word, and make sure it is visible in window
            rpsCurTo.iChar = __min( rpsCur.iChar, rpsCurTo.pfld->GetLength() ); // Move output line forward to same place as input
			if ( rpsCurTo.bInterlinear() ) // 1.1tm On interlinear verify fail, scroll annotations into view
				{
				rpsCur = rpsCurTo;
				while ( rpsCur.bInterlinear() && !rpsCur.bLastInBundle() )
					rpsCur.NextField();
				pview->EditSetCaret( rpsCur.pfld, rpsCur.iChar ); // 1.1tm On interlinear verify fail, scroll annotations into view
				}
            pview->SelectWd( rpsCurTo ); // Select word containing rpsCur
			pview->ParallelJump(); // 6.0zg Do parallel jump to bring any related windows into step with this one
            return FALSE; // Return indication that a spell check failure has been found
            }        
        else // Else (success), move to next word
            { 
//			rpsCur.bSkipNonWhiteLeft(); // 1.6.4y Fix bug of fail on ps n after << on first word // 1.6.4za Remove 1.6.4y causes hang
			CRecPos rpsCurWordStart = rpsCur; // 1.6.4zc Temp to show word start in from line, moves left to start of word below
			rpsCurWordStart.bSkipNonWhiteLeft(); // 1.6.4zc Fix bug of interlinear check fail on ps n after << on first word
//			const char* pszCurWordStart = rpsCurWordStart.psz(); // 1.6.4zc Debug to see where iChar is
//			const char* pszCur = rpsCur.psz(); // 1.6.4zc Debug to see where iChar is
//          rpsCur.iChar = __min( rpsCur.iChar + iLenMatched, rpsCur.pfld->GetLength() ); // 1.6.4zc Fix bug of moving too far on leading punc
            rpsCur.iChar = __min( rpsCurWordStart.iChar + iLenMatched, rpsCur.pfld->GetLength() ); // 1.6.4zc Fix bug of moving too far on leading punc
//			const char* pszCur2 = rpsCur.psz(); // 1.6.4zc Debug to see where iChar is
			rpsCur.bSkipWhite(); // Move to start of word
			rpsCurWordStart = rpsCur; // 1.6.4zd Set WordStart here, use it below
//			const char* pszCur3 = rpsCur.psz(); // 1.6.4zc Debug to see where iChar is
			if ( !( rpsCur.bInterlinear() && *rpsCur.psz() == *pszFailMark() ) ) // If anything except an interlinear fail mark, move past all word separators
				rpsCur.bSkipWdSep(); // Move to start of next word  
//			const char* pszCur4 = rpsCur.psz(); // 1.6.4zc Debug to see where iChar is
			// 1.6.4zd Use WordStart from above here instead of rpsCur.iChar
            rpsCurTo.iChar = __min( rpsCurWordStart.iChar, rpsCurTo.pfld->GetLength() ); // Move output line forward to same place as input // 1.6.4zd 
//			const char* pszCurTo2 = rpsCurTo.psz(); // 1.6.4zc Debug to see where iChar is
			if ( rpsCur.bEndOfField() || rpsCurTo.bEndOfField() ) // If an input field is at end, we are done
				return TRUE;
            }   
        }
    return TRUE; // Should never get here, but return something so compiler won't complain
}

// Add hyphens to output of lookup if input has hyphen and output does not
Str8 CLookupProc::AddLookupHyphens( CRecPos rpsFrom, CRecPos rpsTo, const char* pszOutput, int iLenMatched )
{
	Str8 sOutput = pszOutput;
	char cStartChar = *rpsFrom.psz(); // 1.4ywp 
	int iLenMorph = iGetLenMorph( Str8( rpsFrom.psz(), iLenMatched ) );
	char cEndChar = *rpsFrom.psz( rpsFrom.iChar + iLenMorph - 1 );
	CLangEnc* plngFrom = rpsFrom.pfld->pmkr()->plng(); // 1.5.8e 
	CLangEnc* plngTo = rpsTo.pfld->pmkr()->plng(); // 1.5.8e 
	BOOL bFromRTL = plngFrom->bRightToLeft(); // 1.5.8e 
	BOOL bToRTL = plngTo->bRightToLeft(); // 1.5.8e 
	if ( plngFrom->bRightToLeft() == plngTo->bRightToLeft() ) // 1.5.8e If interlinear opposite rtl direction, don't add hyphens
		{
		if ( bIsMorphBreakChar( cStartChar ) && !bIsMorphBreakChar( sOutput[ 0 ] ) ) // 1.4ywp 
			sOutput = cMorphBreakChar( cStartChar, rpsFrom.pfld->plng() ) + sOutput; // ALB 5.96f Change to insert first morph break char instead of copy, to fix IPA font problem
		if ( bIsMorphBreakChar( cEndChar ) && !bIsMorphBreakChar( sOutput[ sOutput.GetLength() - 1 ] ) ) // If hyphen needed on back, insert one
			sOutput = sOutput + cMorphBreakChar( cEndChar, rpsFrom.pfld->plng() ); // 1.4ywp 
		}
	return sOutput;
}

 // If keep punc, remove punc if necessary // 1.6.4zg 
Str8 CLookupProc::RemovePunc( CRecPos rpsFrom, CRecPos rpsTo, const char* pszOutput ) // Remove punc if necessary // 1.6.4zg 
{
	Str8 sOutput = pszOutput;
    if ( m_bKeepPunc )
        {
		while ( rpsTo.pfld->plng()->iWdSep( sOutput ) > 0 ) // 1.6.4zg Delete leading punc
			sOutput.Delete( 0 );
		while ( TRUE )
			{
			Str8 sRight = sOutput.Right( 1 ); // 1.6.4zg Get last char
			if ( rpsTo.pfld->plng()->iWdSep( sRight ) == 0  ) // 1.6.4zg If not punc, break
				break;
			sOutput.Delete( sOutput.GetLength() - 1 ); // 1.6.4zg If punc at end, delete it
			}
		}
	return sOutput;
}

// If keep capitalization, recapitalize output of lookup if necessary // 1.6.4zg 
Str8 CLookupProc::Recapitalize( CRecPos rpsFrom, CRecPos rpsTo, const char* pszOutput ) // 1.6.4zg 
{
	Str8 sOutput = pszOutput;
    if ( m_bKeepCap )
        {
		const char* psz = rpsFrom.psz();
		const CMChar* pmch = NULL;
		if ( rpsFrom.pfld->plng()->bMChar(&psz, &pmch) && pmch->bUpperCase() && pmch->bHasOtherCase() ) // If upper case multichar in input and it has other case
			{
			psz = sOutput; // Get start of output
			if ( rpsTo.pfld->plng()->bMChar(&psz, &pmch) && !pmch->bUpperCase() && pmch->bHasOtherCase() ) // If lower case multichar in output and it has other case
				{ // Recapitalize output
				pmch = pmch->pmchOtherCase(); // Change to upper case
				sOutput = pmch->sMChar() + psz; // Make upper case version of anal output string by adding upper case letter to rest of string
				}
			}
		}
	return sOutput;
}

// Replace a given word in interlinear with another word, returns any adjustment done to tighten or loosen interlinear
int CLookupProc::ReplaceInInterlinear( CRecPos rpsFrom, int iLenMatched, CRecPos rpsTo, const char* pszReplace )
{
	Str8 sReplace = pszReplace;
	CRecPos rpsEndCol = rpsFrom; // Find end of part to replace
	rpsEndCol.iChar += iLenMatched; // Move past matched
	rpsEndCol.bSkipNonWhite(); // Skip any punc
	rpsEndCol.bSkipWhite(); // Skip to start of next word
	int iLenInsert = sReplace.GetLength() + 1;
	int iLenSpace = rpsEndCol.iChar - rpsFrom.iChar;
	int iLenNeeded = iLenInsert - iLenSpace;
	if ( !rpsEndCol.bEndOfField() ) // If not at end of field, loosen following as needed
		{
		if ( iLenNeeded > 0 )
			rpsEndCol.Loosen( iLenNeeded, TRUE ); // Loosen as needed
		}
	else // Else, at end of field
		{
		CRecPos rpsEndToCol = rpsTo; // If from field is at end of column, find end of to field
		rpsEndToCol.bMoveEnd();
		iLenSpace = rpsEndToCol.iChar - rpsTo.iChar;
		iLenNeeded = 0; // Clear len needed for return
		}
	rpsTo.pfld->Overlay( sReplace, rpsTo.iChar, iLenSpace ); // Overlay on the spaces
	if ( !rpsEndCol.bEndOfField() && iLenNeeded < 0 )
		{
		iLenNeeded = 0; // Clear len needed so Tighten can set it
		rpsTo.Tighten( &iLenNeeded );
		}
	return iLenNeeded;
}

// Check if current lookup answer is still valid, return length of output if valid, 0 if not valid
unsigned int CLookupProc::iInterlinearLookupCheck( CRecPos rpsTo )
{
    CShwView* pview = Shw_papp()->pviewActive();
	rpsTo.bSkipNonWhiteLeft(); // Go back past any punc that was passed
	CRecPos rpsFrom = rpsTo;
	for ( ; ; rpsFrom.pfld = rpsFrom.prec->pfldPrev( rpsFrom.pfld ) ) // Find from field for this lookup process
		{
		if ( !rpsFrom.pfld ) // 5.99zb Fix bug of crash on interlinear verify of lookup with no source line
			return 0;
		if ( rpsFrom.pfld->pmkr() == pmkrFrom() ) // Succeed if From field is found
			break;
		if ( rpsFrom.bFirstInterlinear() ) // Fail if top of interlinear is passed
			return 0;
		}
	if ( rpsFrom.bFirstInterlinear() ) // 1.0dm If lookup from first interlinear, skip leading punctuation
		rpsFrom.bSkipPunc();
	Str8 sLook = rpsFrom.psz();
	int iLenMatched = 0; // Length of matched word, used to move forward after parse
	CFieldList fldlstOut; // Field list for result of lookup and parse
	Str8 sLookOut; // Output of lookup
	if ( bLookup( sLook, &fldlstOut, &iLenMatched ) ) // If lookup succeeds, see if one of the results matches
		{
		CRecPos rpsEndTo = rpsTo;
		CRecPos rpsEndFrom = rpsFrom;
		rpsEndFrom.iChar += iLenMatched; // 5.99i Rewrite find the To 
		rpsEndFrom.bSkipPunc(); // Move to end of word 
		rpsEndFrom.bSkipWhite(); // Go to start of next word
		if ( rpsEndFrom.iChar == rpsEndFrom.pfld->GetLength() ) // 5.99i If from is all the way to end, put to at end
			rpsEndTo.iChar = rpsEndTo.pfld->GetLength();
		else
			rpsEndTo.iChar = rpsEndFrom.iChar;
		if ( rpsEndTo.iChar > rpsEndTo.pfld->GetLength() ) // 5.99i If from is past end of to, put to at end
			rpsEndTo.iChar = rpsEndTo.pfld->GetLength();
		Str8 sCur( rpsTo.psz(), rpsEndTo.iChar - rpsTo.iChar ); // Collect the thing looked up
		int iLenToMove = rpsEndFrom.iChar - rpsFrom.iChar;
		if ( sCur.GetLength() > iLenToMove ) // If at end of line and to is longer than from, extend to end of to
			iLenToMove = sCur.GetLength();
		sCur.TrimRight(); // 5.99i Trim trailing spaces
		RemoveExtraSpaces( sCur ); // 5.99s Fix bug of verify failing when extra spaces in output
		sCur = RemovePunc( rpsFrom, rpsTo, sCur ); // Remove punc if necessary // 1.6.4zh 
		for ( CField* pfld = fldlstOut.pfldFirst(); pfld; pfld = fldlstOut.pfldNext( pfld ) ) // If one of the results matches the desired output, succeed
			{
			sLookOut = pfld->psz(); // Collect the output of the lookup
			int iForce = sLookOut.Find( cForcedGlossStartChar() ); 
			if ( iForce > 0 ) // If forced gloss, remove it to leave just the lookup result
				sLookOut = sLookOut.Left( iForce );
			sLookOut = AddLookupHyphens( rpsFrom, rpsTo, sLookOut, iLenMatched ); // If hyphen needed on front or back, insert one
			sLookOut = Recapitalize( rpsFrom, rpsTo, sLookOut ); // Recapitalize if necessary // 1.6.4zg 
//			sLookOut = RemovePunc( rpsFrom, rpsTo, sLookOut ); // Remove punc if necessary // 1.6.4zg  // 1.6.4zh Wrong code here
			RemoveExtraSpaces( sLookOut ); // 1.0-ag Fix bug of verify failing when extra spaces in multi-word output

			if ( sCur == sLookOut ) // If this output matches the current thing in the interlinear, accept it and move on
				return iLenToMove;

			if ( sCur.Find( sLookOut ) == 0 ) // 5.99zd Fix bug of interlinear check fail where parse didn't find phrase with suffix
				{
				int iLenCur = sCur.GetLength();
				int iLenLookOut = sLookOut.GetLength();
				if ( iLenCur > iLenLookOut && sCur[iLenLookOut] == ' ' ) // See if space next in output of lookup
					{
					rpsEndFrom = rpsFrom; // Find length of first word as distance to move
					rpsEndFrom.bSkipNonWhite();
					rpsEndFrom.bSkipWhite();
					iLenToMove = rpsEndFrom.iChar - rpsFrom.iChar;
					return iLenToMove;
					}
				}
			}
		// If here, there was at least one success, but none match what is in interlinear
		if ( fldlstOut.lGetCount() == 1 && Shw_pProject()->bAutomaticInterlinearUpdate() // 5.98w Make automatic update optional // If only one success, then replace in interlinear
				&& !pintprclst()->bWordParse() ) // 1.6.4ze Fix bug of WordParse overwriting text line if no interlinear
			{
			int iLenAdjust = ReplaceInInterlinear( rpsFrom, iLenMatched, rpsTo, sLookOut ); // Replace current at rpsTo with sLookOut, returns any tighten or loosen done to interlinear
			iLenToMove += iLenAdjust;
			pview->GetDocument()->SetModifiedFlag(); // 5.96zzb Remember something was modified
			return iLenToMove;
			}
		}
	else // If Lookup fails, check to see if it is a CC table process, if so, accept the result
		{
		if ( bApplyCC() || pview->iVerifyPassNumber() < 3 ) // If skip fails, then accept this 5.97x
			{
			if ( pview->iVerifyPassNumber() < 3 )
				pview->SetVerifySkippedFail( TRUE ); // Remember that we skipped a fail, so we need a second pass
			int iStart = rpsFrom.iChar;
			rpsFrom.bSkipNonWhite(); // 6.0n If lookup of failed root, pass by failure mark
			rpsFrom.bSkipWhite(); // 6.0n
			int iLenToMove = rpsFrom.iChar - iStart;
			if ( rpsFrom.iChar >= rpsFrom.pfld->GetLength() ) // If rpsFrom is at end, go to end of rpsTo line
				iLenToMove = rpsTo.pfld->GetLength() - iStart;
			return iLenToMove;
			}
		}
	return 0;
}

#ifdef InterlinearLookupFixFirstTry
// Find next place where lookup From-To combination occurs
CRecPos CShwView::bInterlinearLookupFind( CRecPos& rps, const char* pszFrom, const char* pszTo ) 
{
	return rps;
}

// Fix output of current lookup
BOOL CShwView::bInterlinearLookupFix( const char* pszLookupMustBeFromThis, const char* pszOldLookupResult, const char* pszNewLookupResult ) 
{
	unsigned int iMatchLen = iInterlinearLookupCheck( m_rpsCur, pszLookupMustBeFromThis );
	if ( iMatchLen  == strlen( pszOldLookupResult )) // If old is still OK, leave it alone, it came from somewhere else
		return TRUE;

	return FALSE;
}

// Fix all occurences of lookup in doc
BOOL CShwView::bInterlinearLookupFixAll( CShwDoc* pdoc, 
				const char* pszLookupMustBeFromThis, const char* pszOldLookupResult, const char* pszNewLookupResult ) 
{
    CIndex* pind = pdoc->pindset()->pindRecordOwner(); // use index that owns the document's records to march thru database
    for ( CNumberedRecElPtr prel = pind->pnrlFirst(); prel; prel = pind->pnrlNext( prel ) )
		{
		CRecord* prec = prel->prec();
		CField* pfld = prec->pfldFirst();
		CRecPos rps;
		// rps.bMatch
		for ( ; pfld; pfld = prec->pfldNext( pfld ) )
			{
			if ( pfld == 
			// Find next occurence
			// bInterlinearLookupFind( rps, pszFrom, pszTo ) 
			// Fix it
			bInterlinearLookupFix( pszLookupMustBeFromThis, pszOldLookupResult, pszNewLookupResult ); 
			}
		}
}

void CShwView::UpdateInterlinear() // Update interlinear files for this change, if necessary
	{
	if ( !m_prelCur ) // 5.95f, can happen if filter has no matches
		return;
    CRecord* precDic = m_prelCur->prec();
    CField* pfldDic = precDic->pfldFirst();
	CMarker* pmkrDic = pfldDic->pmkr();
    char* pszUndo = m_pszUndoAllBuffer;
	if ( !pszUndo ) // If no undo, probably no changes were made
		return;
	while (TRUE) // Compare undo fields to current, if one changed, see if it affects interlinear
		{
		int iLen = strlen(pszUndo); // length of field contents in undo buffer
		Str8 sFieldContents = pfldDic->psz();
		if ( sFieldContents.Compare( pszUndo ) != 0 ) // If content of field has changed
			{
			// If this field affects interlinear, update the interlinear
			CDocList doclst; // list of currently open docs
			for ( CShwDoc* pdoc = doclst.pdocFirst(); pdoc; pdoc = doclst.pdocNext() )
				{
				CInterlinearProcList* pintprclst = pdoc->ptyp()->pintprclst();
				CInterlinearProc* pintprc = pintprclst->pintprcFirst();
				for ( ; pintprc; pintprc = pintprclst->pintprcNext(pintprc) ) // For each process
					{

					if ( ( pmkrDic == pintprc->pmkrTo() ) && pintprc->bLookupProc() ) // If this field is output of a lookup proc, update interlinear
						// If this process uses the modified field of the dictionary
						{
						CMarker* pmkrFrom = pintprc->pmkrFrom();
						//CRecPos rps
						// Find the field this lookup is from
						// Get its content
						// Fix all occurences in this doc
						}
					}
				}
			}
		pszUndo = pszUndo + iLen + 1; // move undo buffer past field contents to next marker

		if ( !*pszUndo ) // no more fields in undo buffer
			break;

		CMarker* pmkrUndo = GetDocument()->pmkrset()->pmkrSearch( pszUndo ); // Get marker of undo
		pfldDic = precDic->pfldNext( pfldDic ); // Move record to next field
		if ( !pfldDic ) // 5.95f If no more markers, dont' try any further
			break;
		pmkrDic = pfldDic->pmkr();
		if ( pmkrUndo != pmkrDic ) // If markers don't match, don't try any further
			break;
		pszUndo += strlen(pszUndo) + 1; // move undo buffer past marker to field contents
		}

	}
#endif // InterlinearLookupFixFirstTry

void CLookupProc::IndexUpdated( CIndexUpdate iup ) // Keep tries in step with databases when an index is updated
{
    for ( int i = 0; i < NUMTRIES; i++ ) // For all tries that are in use
        m_pptri[ i ]->IndexUpdated( iup ); // Update trie
}

void CLookupProc::OnDocumentClose( CShwDoc* pdoc ) // Check for references to document being closed 
{
    for ( int i = 0; i < NUMTRIES; i++ ) // Close all tries that are in use
        m_pptri[ i ]->OnDocumentClose( pdoc );
    if ( pdoc == m_pdocWordFormulas ) // If the rule document is being closed, clear the pointer to its record
        {
        m_drflstWordFormulas.OnDocumentClose( pdoc ); // Clear references to document being closed
        m_pdocWordFormulas = NULL; // Clear all pointers
        // 1999-08-30 MRP: Change from \def to \sym and \pat fields.
        // m_precWordFormulas = NULL;
        // m_pmkrDef = NULL;
        }
}

void CLookupProc::MarkerUpdated( CMarkerUpdate& mup ) // update mrflsts in case of marker name change
{
    for ( int i = 0; i < NUMTRIES; i++ ) // Close all tries that are in use
        m_pptri[ i ]->MarkerUpdated( mup );
}

void CLookupProc::ClearRefs() // clear ref ptrs in tries in preparation for database type deletion
{
    for ( int i = 0; i < NUMTRIES; i++ )
        m_pptri[ i ]->ClearRefs();
//    m_ptypMyOwner = NULL; // clear self-reference so owning db type can be purged from memory  // AB disabled 3.0.8r to fix bug on Database Type, Copy, Cancel
}

void CLookupProc::MakeRefs() // turn marker refs into CMarkerPtrs
{
    for ( int i = 0; i < NUMTRIES; i++ )
        m_pptri[ i ]->MakeRefs();
}

static const char* psz_intprc = "intprc"; // Interlinear process
static const char* psz_Lookup = "Lookup"; // Name of lookup process
static const char* psz_Rearrange = "Rearrange"; // Name of Rearrange process
static const char* psz_Given = "Given"; // Name of Given process
static const char* psz_From = "From"; // From marker for lookup process, without mrk for WriteRef
static const char* psz_To = "To"; // To marker for lookup process, without mrk for WriteRef
static const char* psz_PSFrom = "PSFrom"; // From marker for lookup process, without mrk for WriteRef
static const char* psz_Punc = "Punc"; // From marker for lookup process, without mrk for WriteRef
static const char* psz_triLook = "triLook"; // Lookup trie
static const char* psz_triPref = "triPref"; // Prefix trie
static const char* psz_triRoot = "triRoot"; // Root trie
static const char* psz_triSuff = "triSuff"; // Suffix trie
static const char* psz_triInf = "triInf"; // Infix trie
static const char* psz_bApplyCC = "bApplyCC"; // CC Table file path
static const char* psz_CCT = "CCT"; // CC Table file path
static const char* psz_bParseProc = "bParseProc";
static const char* psz_bGenerateProc = "bGenerateProc";
static const char* psz_bSH2Parse = "bSH2Parse";
static const char* psz_bKeepCap = "bKeepCap";
static const char* psz_bNotLongerOverride = "bNotLongerOverride";
static const char* psz_bNotOverrideShowAll= "bNotOverrideShowAll";
static const char* psz_bKeepPunc = "bKeepPunc";
static const char* psz_bStopAtSemicolon = "bStopAtSemicolon";
static const char* psz_GlossSeparator = "GlossSeparator";
static const char* psz_bShowFailMark = "bShowFailMark"; 
static const char* psz_bInsertFail = "bInsertFail"; 
static const char* psz_FailMark = "FailMark";
static const char* psz_bShowWord = "bShowWord";
static const char* psz_bShowRootGuess = "bShowRootGuess";
static const char* psz_bInfixBefore = "bInfixBefore";
static const char* psz_bInfixMultiple = "bInfixMultiple"; // 1.5.1mb 
static const char* psz_bPreferPrefix = "bPreferPrefix";
static const char* psz_bPreferSuffix = "bPreferSuffix";
static const char* psz_bNoCompRoot = "bNoCompRoot";
static const char* psz_bAdapt = "bAdapt";
static const char* psz_bKeepMorphemeBreaks = "bKeepMorphemeBreaks";       

#ifdef typWritefstream // 1.6.4ac
void CLookupProc::WriteProperties( Object_ofstream& obs )
#else
void CLookupProc::WriteProperties( Object_ostream& obs )
#endif
{
    obs.WriteNewline();
    obs.WriteBeginMarker( psz_intprc, psz_Lookup );
    obs.WriteBool( psz_bParseProc, m_bParseProc );      

    m_pmkrFrom->WriteRef( obs, psz_From );
    m_pmkrTo->WriteRef( obs, psz_To );
    m_pptri[LOOK]->WriteProperties( obs, psz_triLook );
    if ( m_bParseProc ) // Root and affix tries apply only to parse process
        {
        m_pptri[PREF]->WriteProperties( obs, psz_triPref );
        m_pptri[ROOT]->WriteProperties( obs, psz_triRoot );
        }
//  m_pptri[SUFF]->WriteProperties( obs, psz_triSuff );
//  m_pptri[INF]->WriteProperties( obs, psz_triInf );
#if UseCct
    obs.WriteBool( psz_bApplyCC, m_bApplyCC );      
    obs.WriteString( psz_CCT, m_sCCT );
#endif
    obs.WriteBool( psz_bSH2Parse, m_bSH2Parse );        
    obs.WriteBool( psz_bKeepCap, m_bKeepCap );      
    obs.WriteBool( psz_bNotLongerOverride, !m_bLongerOverride ); // 1.1hr Save longer override setting for parse     
    obs.WriteBool( psz_bNotOverrideShowAll, !m_bOverrideShowAll); 
    obs.WriteBool( psz_bKeepPunc, m_bKeepPunc );        
    obs.WriteBool( psz_bStopAtSemicolon, m_bStopAtSemicolon );      
    obs.WriteString( psz_GlossSeparator, m_sGlossSeparator );
    obs.WriteString( psz_FailMark, m_sFailMark );
    obs.WriteBool( psz_bShowFailMark, m_bShowFailMark );        
    obs.WriteBool( psz_bInsertFail, m_bInsertFail );        
    obs.WriteBool( psz_bShowWord, m_bShowWord );        
    obs.WriteBool( psz_bShowRootGuess, m_bShowRootGuess );      
    obs.WriteBool( psz_bInfixBefore, m_bInfixBefore );      
    obs.WriteBool( psz_bInfixMultiple, m_bInfixMultiple ); // 1.5.1mb 
    obs.WriteBool( psz_bPreferPrefix, m_bPreferPrefix );      
    obs.WriteBool( psz_bPreferSuffix, m_bPreferSuffix );      
    obs.WriteBool( psz_bNoCompRoot, m_bNoCompRoot );      
    if ( m_pwdfset )
        m_pwdfset->WriteProperties( obs );  // 1999-09-02 MRP
    else if ( m_bParseProc && !m_drflstWordFormulas.bIsEmpty() ) // Word formulas apply only to parse process
	    m_drflstWordFormulas.WriteProperties( obs ); // Write rule file name
    obs.WriteBool( psz_bAdapt, m_bAdapt );      
	obs.WriteUnrecognizedSettingsInfo( m_sUnrecognizedSettingsInfo ); // 1.0cp Write unrecognized settings info

    obs.WriteEndMarker( psz_intprc );
}   

BOOL CInterlinearProc::s_bReadProperties( Object_istream& obs,
        CInterlinearProcList* pintprclst, CInterlinearProc** ppintprc )
{
    if ( !obs.bAtBeginMarker( psz_intprc ) )
        return FALSE;
    if ( CLookupProc::s_bReadProperties( obs, pintprclst, ppintprc ) )
        return TRUE;
    if ( CRearrangeProc::s_bReadProperties( obs, pintprclst, ppintprc ) )
        return TRUE;
    if ( CGivenProc::s_bReadProperties( obs, pintprclst, ppintprc ) )
        return TRUE;
    return TRUE; // True means we ate something         
}

BOOL CLookupProc::s_bReadProperties( Object_istream& obs,
        CInterlinearProcList* pintprclst, CInterlinearProc** ppintprc )
{
    ASSERT( pintprclst );
    if ( !obs.bAtBeginMarker( psz_intprc, psz_Lookup ) ) // If not a lookup process, fail
        return FALSE;
    BOOL bReadBegin = obs.bReadBeginMarker( psz_intprc ); // Step over the marker
 
    CLookupProc* pintprc = new CLookupProc( pintprclst ); // Make a new lookup process to load up           
    CMarkerSet* pmkrset = pintprclst->ptyp()->pmkrset();
    CMarker* pmkrFrom = NULL;
    CMarker* pmkrTo = NULL;
    Str8 sCCT;
    BOOL bParseProc = FALSE;
	BOOL bNotLongerOverride = FALSE;
	BOOL bNotOverrideShowAll= FALSE;
    pintprc->m_bShowFailMark = FALSE;
    pintprc->m_bInsertFail = FALSE;
    pintprc->m_sGlossSeparator = "";
	pintprc->m_bShowRootGuess = FALSE; // 1.5.7a Fix bug of output root guess not staying off

    while ( !obs.bAtEnd() )
        {
        if ( pmkrset->bReadMarkerRef( obs, psz_From, pmkrFrom ) )
            ;
        else if ( pmkrset->bReadMarkerRef( obs, psz_To, pmkrTo ) )
            ;
        else if ( CDbTrie::s_bReadProperties( obs, pintprc->ptri( LOOK ), psz_triLook, LOOK ) )
            ;
        else if ( CDbTrie::s_bReadProperties( obs, pintprc->ptri( PREF ), psz_triPref, PREF ) )
            ;
        else if ( CDbTrie::s_bReadProperties( obs, pintprc->ptri( ROOT ), psz_triRoot, ROOT ) )
            ;
        else if ( CDbTrie::s_bReadProperties( obs, pintprc->ptri( SUFF ), psz_triSuff, SUFF ) ) // Unused because these are no longer written. Left here only to prevent problems when reading older settings files.
            ;
        else if ( CDbTrie::s_bReadProperties( obs, pintprc->ptri( INF ), psz_triInf, INF ) ) // Unused because these are no longer written. Left here only to prevent problems when reading older settings files.
            ;
        else if ( pintprc->pdrflstWordFormulas()->bReadProperties( obs ) )
            ;
        else if ( CWordFormulaSet::s_bReadProperties(obs, pintprc, &pintprc->m_pwdfset) )
            ;
        else if ( obs.bReadBool( psz_bApplyCC, pintprc->m_bApplyCC ) )
            ;   
        else if ( obs.bReadString( psz_CCT, sCCT ) )
            ;
        else if ( obs.bReadBool( psz_bParseProc, bParseProc ) )
            ;   
        else if ( obs.bReadBool( psz_bSH2Parse, pintprc->m_bSH2Parse ) )
            ;   
        else if ( obs.bReadBool( psz_bKeepCap, pintprc->m_bKeepCap ) )
            ;   
        else if ( obs.bReadBool( psz_bNotLongerOverride, bNotLongerOverride ) )
            ;   
        else if ( obs.bReadBool( psz_bNotOverrideShowAll, bNotOverrideShowAll) )
            ;   
        else if ( obs.bReadBool( psz_bKeepPunc, pintprc->m_bKeepPunc ) )
            ;   
        else if ( obs.bReadBool( psz_bStopAtSemicolon, pintprc->m_bStopAtSemicolon ) )
            ;   
        else if ( obs.bReadString( psz_GlossSeparator, pintprc->m_sGlossSeparator ) )
            ;
        else if ( obs.bReadBool( psz_bShowFailMark, pintprc->m_bShowFailMark ) )
            ;   
        else if ( obs.bReadBool( psz_bInsertFail, pintprc->m_bInsertFail ) )
            ;   
        else if ( obs.bReadString( psz_FailMark, pintprc->m_sFailMark ) )
            ;
        else if ( obs.bReadBool( psz_bShowWord, pintprc->m_bShowWord ) )
            ;   
        else if ( obs.bReadBool( psz_bShowRootGuess, pintprc->m_bShowRootGuess ) )
            ;   
        else if ( obs.bReadBool( psz_bInfixBefore, pintprc->m_bInfixBefore ) )
            ;   
        else if ( obs.bReadBool( psz_bInfixMultiple, pintprc->m_bInfixMultiple ) ) // 1.5.1mb 
            ;   
        else if ( obs.bReadBool( psz_bPreferPrefix, pintprc->m_bPreferPrefix ) )
            ;   
        else if ( obs.bReadBool( psz_bPreferSuffix, pintprc->m_bPreferSuffix ) )
            ;   
        else if ( obs.bReadBool( psz_bNoCompRoot, pintprc->m_bNoCompRoot ) )
            ;   
        else if ( obs.bReadBool( psz_bAdapt, pintprc->m_bAdapt ) )
            ;   
		else if ( obs.bUnrecognizedSettingsInfo( psz_intprc, pintprc->m_sUnrecognizedSettingsInfo  ) ) // 1.0cp Store any unrecognized settings info so as not to lose it
			;
        else if ( obs.bEnd( psz_intprc ) ) // If end or begin something else, break
            break;
        }
	pintprc->m_bLongerOverride = !bNotLongerOverride; // 1.1hr Save longer override setting for parse
	pintprc->m_bOverrideShowAll= !bNotOverrideShowAll; 
    if ( pmkrFrom && pmkrTo ) // If markers came in, set them in the process, and set the process pointer
        {
        pintprc->SetMarkers( pmkrFrom, pmkrTo );
        *ppintprc = pintprc;
        if ( bParseProc ) // If this is a parse process, set its flag and name appropriately
            {
            pintprc->SetParseProc(); // Set this as a parse process
            pintprc->ptri( SUFF )->CopySettings( pintprc->ptri( PREF ) ); // Copy settings from PREF to the other two affix tries 
            pintprc->ptri( INF )->CopySettings( pintprc->ptri( PREF ) ); 
            }
#if UseCct
        if ( pintprc->bApplyCC() // If applying CC and there is a CC table name, load it // 5.97t Here is bug of trying to load CC table before path has been updated to new project path
                && sCCT.GetLength() )
			{
		    Shw_pProject()->UpdatePath( sCCT ); // 5.98m Update path if project moved and it was in project folder
            pintprc->bLoadCCFromFile( sCCT ); // Load CC
			}
#endif
        pintprc->MakeRefs(); // turn marker refs into CMarkerPtrs
        }
    else // Else (incomplete process), don't make one
        {
        delete pintprc;
        *ppintprc = NULL;
        }
    return TRUE; // True means some input was eaten
}

void CLookupProc::SetWordFormulas(const CWordFormulaSet* pwdfset)
{
    delete m_pwdfset;
    if ( pwdfset )
        m_pwdfset = new CWordFormulaSet(*pwdfset);
    else
        m_pwdfset = NULL;
}

BOOL CLookupProc::bModalView_Properties()
{   
    if ( m_bParseProc )
        {
        CParseDlg dlg( this );
        return ( dlg.DoModal() == IDOK );					 
        }
    else
        {
        CLookupDlg dlg( this );
        return ( dlg.DoModal() == IDOK );
        }   
}

//---------------------------------------------------------------
CRearrangeProc::CRearrangeProc( CInterlinearProcList* pintprclstMyOwner,
		BOOL bGenerateProc )
            : CInterlinearProc( pintprclstMyOwner )
{
    m_bGenerateProc = bGenerateProc; // Set generating
	if ( bGenerateProc )
		SetType( CProcNames::eGenerate ); // 1.4ah Change to not use string resource id in interlinear
    else
		SetType( CProcNames::eRearrange ); // 1.4ah Change to not use string resource id in interlinear
    m_sFailMark = "***";
	m_sWordBound = "#";
    m_pdoc = NULL;
	m_pind = NULL;
    m_prel = NULL;
    SetAdapt( TRUE );
	m_bKeepMorphemeBreaks = FALSE;
}

CRearrangeProc::~CRearrangeProc() // Destructor 
{
}

int CRearrangeProc::iWdPSMatch( const char* pszFind, const char* pszText, const char* pszPS, int iRecursLevel = 0 ) // Match Find pattern at text and PS position
{
    while ( *pszFind == ' ' ) // Skip over leading spaces in rule
        pszFind++;
    BOOL bOptional = FALSE;
    if ( *pszFind == '(' ) // If optional, move past parend and note
        {
        pszFind++;
        bOptional = TRUE;
        }
    int iMatchLen = NO_MATCH; // Init match length
    const char* pszDef = pszDefFind( pszFind, FALSE ); // See if this find is a definition
    if ( pszDef ) // If def, try to match it
        {
        if ( iRecursLevel > 10 ) // If recursion depth reaches 10, it must be left recursive, so fail
            {
			Str8 sMsg = _("Left recursive definition cannot match:"); // 1.5.0fv 
			sMsg = sMsg + " " + pszDef; // 1.5.0fd 
			AfxMessageBox( sMsg ); // 1.4gd
            return NO_MATCH;
            }
        while ( *pszDef && *pszDef != '\n' ) // While more elements to match
            {
            int iMatchEnd = 0; // Match end position in text for current word in rule
            while ( TRUE ) // While more elements to match, try to match
                {
                iMatchLen = iWdPSMatch( pszDef, pszText + iMatchEnd, pszPS, iRecursLevel + 1 ); // Try match here
                if ( iMatchLen == NO_MATCH ) // If not match, fail
                    break;
                if ( iMatchLen == FULL_MATCH ) // If matched all the way to end, move to end and count len
                    iMatchLen = strlen( pszText ); // Get remaining length to move to end of text
                else // Else (not all the way to end) move PS the same distance as text
                    pszPS += iMatchLen;
                iMatchEnd += iMatchLen; // Calculate end of match
                pszDef = pszNextEl( pszDef ); // Move def to next word
                if ( !*pszDef || *pszDef == '\n' ) // If end of def line, we have success
                    {
                    iMatchLen = iMatchEnd; // Set match length total matched
                    break; // Note success and break search loop
                    }
                }
            if ( iMatchLen == NO_MATCH ) // If no success on this line of def, move to next line
				{
                while ( *pszDef && *pszDef++ != '\n' ) // Move to start of next def line
                    ;
				while ( *pszDef == '\n' ) // Skip blank lines
					pszDef++;
				}
            }
        }
    if ( iMatchLen == NO_MATCH ) // If no def match, see if word match
        iMatchLen = iWdMatch( pszFind, pszText, m_pmkrFrom->plng() ); // See if word match here
    if ( iMatchLen == NO_MATCH ) // If no word match, see if PS match
		{
        iMatchLen = iWdMatch( pszFind, pszPS, m_pmkrPSFrom->plng() ); // See if PS match here
		if ( iMatchLen != NO_MATCH && !*(pszPS + iMatchLen) ) // 1.4hfk Fix bug of rearrange chopping morphname at end of line
			iMatchLen = FULL_MATCH; // 1.4hfk If match all the way to end, return FULL_MATCH
		}
    if ( iMatchLen == NO_MATCH && bOptional ) // If no match, but optional, succeed with zero length
        return 0;
    return iMatchLen; // Return total length matched
}

const char* CRearrangeProc::pszDefFind( const char* pszFind, BOOL bPhon ) // See if this find is a definition, bPhon says it is phonology so stop at close bracket or star, bStar returns whether it stopped at star
{
	CRecord* prec = m_prel->prec();
    for ( CField* pfld = prec->pfldFirst(); pfld; pfld = prec->pfldNext( pfld ) ) // For each field in record, see if it is a rule
        {
        if ( (CMarker*)pfld->pmkr() == m_pmkrRule ) // If rule found, don't look further (cast prevents ambiguous operator message)
            return NULL;
        if ( (CMarker*)pfld->pmkr() == m_pmkrDef ) // If def found, see if it matches (cast prevents ambiguous operator message)
            {
            const char* pszDef = pfld->psz();
            if ( iDefNameMatch( pszDef, pszFind, bPhon ) != NO_MATCH ) // If def name matches, return success
                {
                while ( *pszDef && *pszDef++ != '\n' ) // Move to start of first def line
                    ;
				while ( *pszDef == '\n' ) // Skip blank lines
					pszDef++;
                if ( !*pszDef ) // If no content to definition, fail
                    return NULL;
                return pszDef; // Return start of next line
                }
            }
        }
    return NULL;
}

const char* pszNextPhonEl( const char* psz ) // Move to next match element in a rule, stops at begin of next word or at end of line or end of field
{ // An element is a string or a class definition
// #a- e[V][C]xyz[V][C]#
// 1       2  3 4     5  6  7
	if ( *psz == '[' ) // If class definition, pass by it
		{
		while ( *psz && !( *psz == ']' ) ) // Find end of class def
			psz++;
		psz++; // Move past end
		}
	else
		{
		while ( *psz && !( *psz == '\n' || *psz == '[' ) ) // Move to end of element
			psz++;
		}
    return psz;
}

BOOL bPhonElMatch( const char* pszText, const char* pszFind ) // Match replace element in match
{
	if ( !( *pszText == '[' ) ) // Match only on class definitions
		return FALSE;
    while ( *pszText++ == *pszFind++ ) // While match, move forward
        if ( !*pszText || *( pszText - 1 ) == ']' ) // If text at end of class, we have success
            return TRUE;
    return FALSE; // If mismatch, fail
}

int iPhonElFind( const char* pszMatch, const char* pszRepl, int iClassNum ) // Find replace element in match string, return number of matched element or -1 if no match. If iClassNum is greater than zero, search for later numbered one.
    {
    int iElNum = 0;
	int iElNumPrev = -1; // Remember previous element, default to none
	while ( TRUE ) // While no match found, try next
		{
		if ( bPhonElMatch( pszMatch, pszRepl ) ) // If match, see if it is the desired one
			{
			iElNumPrev = iElNum; // Remember this match as previous. If desired number not found, return last one found
			if ( iClassNum-- == 0 ) // If class number is zero, this is the desired one.
				return iElNum;
			}
		pszMatch = pszNextPhonEl( pszMatch ); // Move to next
		if ( !*pszMatch || *pszMatch == '\n' ) // If end of match, return fail or previous match
			return iElNumPrev;
		iElNum++; // Count the element
		}
    }

int iGetClassNum( const char* pszClass, const char* pszStart ) // Find out which number of the same class this is
	{
	const char* psz = pszStart;
	for ( int iNum = 0; ; ) // Count number found
		{
		while ( *psz && *psz != '[' ) // Move to start of class
			psz = pszNextPhonEl( psz );
		if ( !*psz || psz >= pszClass ) // If end or self is reached, return number
			return iNum;
		if ( bPhonElMatch( pszClass, psz ) ) // If same as self, count it
			iNum++;
		psz = pszNextPhonEl( psz ); // Move past class
		}
	}

int iDefMatch( const char* pszFind, const char* pszText ) // Match def word in text, if success, return length of match, else return 0
{
	const char* pszStart = pszText; // Remember text start
	while ( *pszText++ == *pszFind++ ) // While match, move forward
        if ( !*pszFind || *pszFind == '\n' // If find is at end string, or end of line, we have success
				|| ( *pszFind == ' ' ) // Also success at space
				)
			return pszText - pszStart; // Return length of match
	return NO_MATCH; // If mismatch, fail
}

int CRearrangeProc::iPhonMatch( const char* pszFind, const char* pszText ) // Match find word in text, if success, return length of match, else return 0
{
	const char* pszStart = pszText; // Remember text start
	while ( *pszText++ == *pszFind++ ) // While match, move forward
		{
		if ( *pszText == ' ' && *pszFind != ' ' ) // If space in text, but none in rule, ignore it
			pszText++;
        if ( !*pszFind || *pszFind == '\n' // If find is at end string, or end of line, we have success
				|| *pszFind == '[' ) // Also success at class start
			return pszText - pszStart; // Return length of match
		}
	return NO_MATCH; // If mismatch, fail
}

int CRearrangeProc::iPhonDefMatch( const char* pszFind, const char* pszText, int* piMatchElNum ) // Match Phonological element or class
{
    int iMatchLen = NO_MATCH; // Init match length
	*piMatchElNum = 0; // Init match element number
	if ( *pszFind == '[' ) // If class definition, look for class
		{
		const char* pszDefStart = pszDefFind( pszFind + 1, TRUE ); // See if definition is found
		if ( pszDefStart ) // If def, try to match it // *** Maybe give error if def not found
			{
			int iStarRept = 1; // Note if star for repeat
			for ( const char* psz = pszFind; *psz && *psz != ']'; psz++ )
				if ( *psz == '*' )
					iStarRept = 20;
			iMatchLen = 0; // Start match length at zero so that star repeat can add to it
			for ( ; iStarRept > 0; iStarRept-- ) // If no star repeat, do only once, otherwise until no more match
				{
				const char* pszDef = pszDefStart; // Initialize def search
				int iDefMatchLen = NO_MATCH;
				for ( int iElNum = 0; *pszDef; iElNum++ ) // While more elements to match
					{
					int iLen = iDefMatch( pszDef, pszText ); // If match, succeed and remember longest
					if ( iLen > iDefMatchLen ) 
						{
						iDefMatchLen = iLen; // Remember longest match
						*piMatchElNum = iElNum; // Remember element number of longest match
						}
					pszDef = pszPhonDefNextEl( pszDef ); // Move def to next word
					}
				if ( iDefMatchLen == NO_MATCH ) // If no match
					{
					if ( iStarRept <= 1 ) // If not star, fail, otherwise keep however much was matched, including zero as an acceptable length
						iMatchLen = NO_MATCH;
					break;
					}
				iMatchLen += iDefMatchLen; // Add length of match to total
				pszText += iDefMatchLen; // Add length of match to text position
				}
			}
		if ( iMatchLen != NO_MATCH ) // If successful def match, pass by optional spaces
			{
			pszFind = pszNextPhonEl( pszFind ); // Move find to next phonolgical element
			if ( *pszText == ' ' && *pszFind != ' ' ) // If space in text, but none in rule, ignore it
				iMatchLen++; // Note that one more char of text has been matched
			}
		}
	else
        iMatchLen = iPhonMatch( pszFind, pszText ); // See if word match here
    return iMatchLen; // Return total length matched
}

void CRearrangeProc::SetRuleFile( const Str8& sRuleFile ) // Set rule file name   
{
    m_drflst.DeleteAll(); // Empty the ref list
    if ( !sRuleFile.IsEmpty() )
        m_drflst.Add( sRuleFile, "" ); // Make a new ref with the rule file name  
    m_prel = NULL; // Clear pointer so it will look up new file
}

const char* CRearrangeProc::pszRuleFile() // Name of rule file
{
    CDatabaseRef* pdrf = m_drflst.pdrfFirst(); // Get database ref, can return null if no databases in list
    if ( !pdrf ) // If no rule file, return null string
        return "";
    else return pdrf->sDatabasePath();
}

BOOL CRearrangeProc::bInterlinearize( CRecPos rpsStart, int* piLen, int iProcNum, int& iResult ) // Run interlinearization process    
{
    if ( !m_prel ) // If first time, or no rule file, get record pointer to rules database
        {
        // Get first record of rearrangement rules database from its name in the database ref list
        CDatabaseRef* pdrf = m_drflst.pdrfFirst(); // Get database ref, can return null if no databases in list
        if ( pdrf && pdrf->sDatabasePath() != "" ) // If rule file, open it
			{
			m_pdoc = pdrf->pdoc(); // Get document, opening if not open, returns null if file not found
			m_pind = pdrf->pind(); // Get owning index of document, returns null if file not open
			if ( !m_pind )
				;
			else
				{
				m_prel = m_pind->prelFirst(); // Get record look element, returns null if file is empty
				if ( !m_prel ) // If no record element, fail
					;
				else
					{
					CMarkerSet* pmkrset = m_pdoc->ptyp()->pmkrset(); // Get database type marker set
					m_pmkrRule = pmkrset->pmkrSearch_AddIfNew( "ru" ); // Find rule marker
					m_pmkrDef = pmkrset->pmkrSearch_AddIfNew( "def" ); // Find definition marker
					}
				}
			}
        }

    ASSERT( rpsStart.iChar <= rpsStart.pfld->GetLength() );
    CField* pfldFrom = rpsStart.pfldFindInBundle( pmkrFrom() ); // Find From field
    if ( !pfldFrom ) // If no earlier process generated the desired field, don't go further
        return FALSE;
    CField* pfldPSFrom = rpsStart.pfldFindInBundle( pmkrPSFrom() ); // Find Part of Speech field
    if ( !pfldPSFrom ) // If no earlier process generated the PS field, set it to the from field. This will look twice at that field, but that should not do any harm.
        pfldPSFrom = pfldFrom;
    CField* pfldPunc = rpsStart.pfldFindInBundle( pmkrPunc() ); // Find Punc field
    
    CField* pfldTo = rpsStart.pfldFindInBundleAdd( pmkrTo() ); // Find To field, if not present, insert it
    rpsStart.SetPosEx( rpsStart.iChar, pfldFrom ); // Set starting position to desired column in from field

    // Initialize variables for lookup
    CMString mksAnal( pfldTo->pmkr(), "" ); // Analysis output

    // Walk through morphemes of current word
    for ( CRecPos rpsStartCol = rpsStart; !rpsStartCol.bEndOfField(); ) // While not at end of string or line, terminates below at end of column
        {
        // Position variables, documented here so that they can be seen together, but not declared until initialized
        // rpsStartCol is Starting column, before begin punc
        // rpsStartWd is Start of word, after begin punc
        // rpsEndWd is End of word, before end punc
        // rpsEndPunc is End of word, after end punc
        // rpsEndCol is Ending column, start of next word
        // Set up leading punc of word      
        CRecPos rpsStartWd = rpsStartCol; // Start of word, after begin punc
        rpsStartWd.bSkipWhite(); // Move past space to start of next word  
        CRecPos rpsEndWd = rpsStartWd;  
        rpsEndWd.bSkipNonWhite(); // Move to end of word
        // Find end of column
        CRecPos rpsEndCol = rpsEndWd; // Ending column, start of next word
        rpsEndCol.bSkipWhite(); // Move past spaces between words
		
		if ( m_bGenerateProc ) // If generate, collect all morphemes of word, based on hyphens
			{
			while ( !rpsEndCol.bEndOfField()
					    && ( bIsMorphBreakChar( *(rpsEndWd.psz() - 1) ) 
								||  bIsMorphBreakChar( *(rpsEndCol.psz()) ) ) )
				{
				rpsEndWd = rpsEndCol;
				rpsEndWd.bSkipNonWhite(); // Move to end of word
				rpsEndCol = rpsEndWd;
				rpsEndCol.bSkipWhite(); // Move past spaces between words
				}
			}

        Str8 sCurWord( rpsStartWd.psz(), rpsEndWd.iChar - rpsStartWd.iChar ); // Save current word, to be parsed or used as copy if no parse

        int iLenMatched = rpsEndWd.iChar - rpsStartWd.iChar; // Length of match for insert
        InsistNonZero( iLenMatched, 1 ); // Should always be something matched 

        mksAnal = sCurWord; // Default to copy one word, used if no change made
		if ( m_bGenerateProc ) // If generate, eliminate spaces and apply rules
			{
			mksAnal.TrimBothEnds(); // Trim ends
			int i = 0;
			for ( ; i < mksAnal.GetLength() - 1; ) // Delete multiple spaces
				if ( mksAnal.GetChar(i) == ' ' && mksAnal.GetChar(i+1) == ' ' ) // If two spaces, delete one
					mksAnal.Delete( i );
				else
					i++;
			mksAnal = m_sWordBound + mksAnal + m_sWordBound; // Put a boundary symbol on each end so that rules can refer to them
			if ( m_prel ) // If valid rule file, apply rules
				{
				for ( CRecLookEl* prel = m_prel; prel; prel = m_pind->prelNext( prel ) ) //AB 5.1a For each record in file (changed from only first record)
					{
					CRecord* prec = prel->prec();
					for ( CField* pfld = prec->pfldFirst(); pfld; pfld = prec->pfldNext( pfld ) ) // For each field in record, see if it is a rule
						{
						if ( (CMarker*)pfld->pmkr() == m_pmkrRule ) // For each rule found, try it here (cast prevents error of ambiguous operator)
							{
							BOOL bRuleApplied = TRUE;
							for ( int iNumRuleTries = 1; bRuleApplied; iNumRuleTries++ ) // Keep applying rule until it no longer applies
								{
								bRuleApplied = FALSE; // Initialize to no rule applied
								for ( const char* pszTextStart = mksAnal.psz(); *pszTextStart; pszTextStart++ ) // While more text, try at next position
									{
									int aiMatchEnd[100]; // Match position for each element of rule that has matched
									int aiMatchElNum[100]; // Match position for each element of rule that has matched
									int iMatchEnd = 0; // Match end position in text for current word in rule
									int iMatchLen = 0; // Match length for current word in rule
									int iRuleWdNum = 0; // Number of current element in rule, total number of elements when match is complete
									const char* pszFind = pfld->psz(); // Init rule match position
									while ( *pszFind == ' ' ) // Skip over leading spaces in rule
										pszFind++;
									const char* pszText = pszTextStart;
									for ( ; ; ) // While more elements to match, try to match
										{
										int iMatchElNum = 0;
										iMatchLen = iPhonDefMatch( pszFind, pszText, &iMatchElNum ); // Try match here
										if ( iMatchLen == NO_MATCH ) // If not match, fail
											break;
										aiMatchElNum[ iRuleWdNum ] = iMatchElNum; // Remember number of element matched
										iMatchEnd += iMatchLen; // Calculate end of match
										aiMatchEnd[ iRuleWdNum++ ] = iMatchEnd + ( pszTextStart - mksAnal.psz() ); // Remember match position
										pszFind = pszNextPhonEl( pszFind ); // Move find to next phonolgical element
										pszText += iMatchLen; // Move text position forward
										if ( !*pszFind || *pszFind == '\n' ) // If end of rule match line, we have success
											break; // Note success and break search loop
										if ( *pszText == ' ' && *pszFind != ' ' ) // Skip over text space
											pszText++;
										}
									if ( iMatchLen > 0 ) // If success, output equivalent
										{
										Str8 sHead = mksAnal.Left( pszTextStart - mksAnal.psz() ); // Extract head before changed part
										Str8 sTail = mksAnal.Mid( pszText - mksAnal.psz() ); // Extract tail after changed part
										const char* pszRepl = pszFind + 1; // Pointer to replace part of rule
										while ( *pszRepl == ' ' ) // Pass by leading spaces
											pszRepl++;
										const char* pszReplStart = pszRepl; // Remember start of replace
										pszFind = pfld->psz(); // Put match back to start of rule
										for ( ; *pszRepl && *pszRepl != '\n'; pszRepl = pszNextPhonEl( pszRepl ) ) // For each element of the replace
											{
											if ( *pszRepl == '[' ) // If class definition, look for class
												{
												int iClassNum = iGetClassNum( pszRepl, pszReplStart ); // Find out which number of the same class this is
												int iElNum = iPhonElFind( pszFind, pszRepl, iClassNum ); // Search for replace element in find string
												if ( iElNum >= 0 ) // If it is found in search, output search matched section
													{
													// Get start and end
													int iStart = iElNum ? aiMatchEnd[ iElNum - 1 ] : pszTextStart - mksAnal.psz(); // Get start position of text section
													const char* pszStart = mksAnal.psz() + iStart; // Get start pointer
													const char* pszEnd = pszStart + aiMatchEnd[ iElNum ] - iStart; // Get pointer to end
													while ( pszEnd > pszStart && *( pszEnd - 1 ) == ' ' ) // Don't add ending spaces
														pszEnd--;
													for ( const char* psz = pszStart; psz < pszEnd; psz++ ) // For each char, add it to output
															sHead += *psz;
													}
												else // Else (not found in search) output equivalent from unmatched class in search
													{
													iElNum = 0; // Init element number to count elements
													for ( const char* pszSrch = pszFind; pszSrch; pszSrch = pszNextPhonEl( pszSrch ) ) // For each class in search
														{
														if ( *pszSrch == '[' && !( iPhonElFind( pszReplStart, pszSrch, 0 ) >= 0 ) ) // If not found in replace, output search element that matches the number of the element matched in the search
															{
															int iMatchElNum = aiMatchElNum[ iElNum ]; // Get element number to find
															const char* pszDef = pszDefFind( pszRepl + 1, TRUE ); // See if definition is found
															if ( pszDef )
																{
																for ( int i = 0; i < iMatchElNum; i++ ) // Count to the right numbered element
																	if ( *pszDef ) // If not past end, move to next
																		pszDef = pszPhonDefNextEl( pszDef );
																while ( *pszDef && *pszDef != ' ' && *pszDef != '\n' ) // Add def element to output
																	sHead += *pszDef++;
																}
															break;
															}
														iElNum++; // Count element
														}
													}
												}
											else // Else (not found in search) output replace string literally
												{
												const char* pszReplEnd = pszNextPhonEl( pszRepl ); // Move to next replace element
												for ( const char* psz = pszRepl; 
														*psz && psz != pszReplEnd; psz++ ) // For each char, add it to output
													sHead += *psz;
												}
											while ( *pszRepl == ' ' ) // Pass by spaces
												pszRepl++;
											}
										int iLen = sHead.GetLength();
										if ( iLen > 0 && sHead[ iLen - 1 ] == ' ' ) // If head ends with space, cut if off
											sHead = sHead.Left( iLen - 1 );
										iLen = sTail.GetLength();
										if ( iLen > 0 && sTail[ 0 ] == ' ' ) // If tail starts with space, cut it off
											sTail = sTail.Mid( 1 );
										mksAnal = sHead + sTail; // Put tail back on
										iLenMatched = aiMatchEnd[ iRuleWdNum - 1 ]; // Set length matched to end of last rule match
										bRuleApplied = TRUE; // Remember that a rule was applied
			//							// Reset word end for length matched in lookup or parse
			//							rpsEndCol.SetPosEx( rpsStartWd.iChar + iLenMatched ); // Set word end correct for actual match length
										break; // Don't look at any more rules // *** out when loop works
										}
									}
								}
							}
						}
					}
				}
			mksAnal.DeleteAll( ' ' ); // Eliminate morpheme dividing spaces
			mksAnal.OverlayAll( *m_sWordBound, ' ' ); // Change boundary symbols to spaces, including those inserted internally by rules
			mksAnal.TrimBothEnds(); // Trim boundary spaces off ends
			if ( !m_bKeepMorphemeBreaks ) // If not keeping morpheme breaks, remove them
				for ( i = 0; *mksAnal.psz( i ); ) // Remove hyphens
					if ( bIsMorphBreakChar( *mksAnal.psz( i ) ) )
						mksAnal.Delete( i );
					else
						i++;
			}
		else
			{
			BOOL bRuleApplied = FALSE; // Remember if a rule was applied
			if ( m_prel ) // If valid rule file, apply rules
				{
				for ( CRecLookEl* prel = m_prel; prel; prel = m_pind->prelNext( prel ) ) //AB 5.1a For each record in file (changed from only first record)
					{
					CRecord* prec = prel->prec();
					for ( CField* pfld = prec->pfldFirst(); pfld; pfld = prec->pfldNext( pfld ) ) // For each field in record, see if it is a rule
						{
						if ( (CMarker*)pfld->pmkr() == m_pmkrRule ) // For each rule found, try it here (cast prevents error of ambiguous operator)
							{
							int aiMatchEnd[100]; // Match position for each element of rule that has matched
							int iMatchEnd = 0; // Match end position in text for current word in rule
							int iMatchLen = 0; // Match length for current word in rule
							int iRuleWdNum = 0; // Number of current element in rule, total number of elements when match is complete
							const char* pszText = rpsStartWd.psz(); // Init text position
							const char* pszPS = pfldPSFrom->psz() + rpsStartWd.iChar; // Init PS position
							const char* pszFind = pfld->psz(); // Init rule match position
							while ( TRUE ) // While more elements to match, try to match
								{
								iMatchLen = iWdPSMatch( pszFind, pszText, pszPS ); // Try match here
								if ( iMatchLen == NO_MATCH ) // If not match, fail
									break;
								if ( iMatchLen == FULL_MATCH ) // If matched all the way to end, move to end and count len
									iMatchLen = strlen( pszText ); // Get remaining length to move to end of text
								else // Else (not all the way to end) move PS the same distance as text
									pszPS += iMatchLen;
								pszText += iMatchLen; // Move text to end of match
								iMatchEnd += iMatchLen; // Calculate end of match
								aiMatchEnd[ iRuleWdNum++ ] = iMatchEnd; // Remember match position
								pszFind = pszNextEl( pszFind ); // Move find to next word
								if ( !*pszFind || *pszFind == '\n' ) // If end of rule match line, we have success
									break; // Note success and break search loop
								}
							if ( !*pszFind ) // If no replacement line, don't replace, but treat this as an override of lower rules
								break;
							if ( iMatchLen > 0 ) // If success, output equivalent
								{
								mksAnal = ""; // Clear output to build
								const char* pszRepl = pszFind + 1; // Pointer to replace part of rule
								pszFind = pfld->psz(); // Put match back to start of rule
								while ( *pszRepl && *pszRepl != '\n' ) // For each element of the replace
									{
									int iElNum = iElFind( pszFind, pszRepl ); // Search for replace element in find string
									if ( iElNum >= 0 ) // If it is found in search, output search matched section
										{
										// Get start and end
										int iStart = iElNum ? aiMatchEnd[ iElNum - 1 ] : 0; // Get start position of text section
										const char* pszStart = rpsStartWd.psz() + iStart; // Get start pointer
										const char* pszEnd = pszStart + aiMatchEnd[ iElNum ] - iStart; // Get pointer to end
										while ( pszEnd > pszStart && *( pszEnd - 1 ) == ' ' ) // Don't add ending spaces
											pszEnd--;
										for ( const char* psz = pszStart; psz < pszEnd; psz++ ) // For each char, add it to output
											if ( !( *psz == ' ' && *(psz + 1) == ' ' ) ) // Skip multiple spaces
												mksAnal += *psz;
										}
									else // Else (not found in search) output replace string literally
										for ( const char* psz = pszRepl; *psz && *psz != ' ' && *psz != '\n'; psz++ ) // For each char, add it to output
											mksAnal += *psz;
									pszRepl = pszNextEl( pszRepl ); // Move to next replace element
									if ( *pszRepl && *pszRepl != '\n' ) // If another one coming, put a space into output
										if ( !( mksAnal.GetLength() > 0 && mksAnal.GetChar( mksAnal.GetLength() - 1 ) == ' ' ) ) // Don't add space if already one there
											mksAnal += " ";
									}
								iLenMatched = aiMatchEnd[ iRuleWdNum - 1 ]; // Set length matched to end of last rule match
								bRuleApplied = TRUE; // Remember that a rule was applied
								// Reset word end for length matched in lookup or parse
								rpsEndCol.SetPosEx( rpsStartWd.iChar + iLenMatched ); // Set word end correct for actual match length
								break; // Don't look at any more rules
								}
							}
						}
					}
				}
			}

        InsistNonEmpty( mksAnal, sCurWord );  // Should be filled in by now, but just in case, copy input  word as it used to do
        
        // Get leading punc
        Str8 sLeadPunc; // Leading punc
        Str8 sTrailPunc; // Trailing punc
        BOOL bCap = FALSE; // True if word was capitalized
        // Extract punc from punc line
        if ( pfldPunc ) // If a punc field, collect punc and cap from it
            { // Collect leading punc
            CRecPos rpsPuncStart( rpsStartCol.iChar, pfldPunc, rpsStartCol.prec ); // Init start of punc
            CRecPos rpsPuncEnd = rpsPuncStart; // Init end of punc
            BOOL bPunc = TRUE;
            if ( rpsPuncStart.iChar > 0 && rpsPuncStart.bLeftIsNonWhite() // Don't collect punc if not at start of word
                        || rpsPuncStart.bIsWhite() )
                bPunc = FALSE;
            if ( bPunc )
                rpsPuncEnd.bSkipPunc(); // Find end of punc
            sLeadPunc = rpsPuncStart.psz(); // Collect leading punc
            sLeadPunc = sLeadPunc.Left( rpsPuncEnd.psz() - rpsPuncStart.psz() );
            // See if cap necessary
            const char* psz = rpsPuncEnd.psz();
            const CMChar* pmch = NULL;
            if ( *psz && pfldPunc->plng()->bMChar(&psz, &pmch) && pmch->bUpperCase() && pmch->bHasOtherCase() ) // If upper case multichar in input and it has other case
                bCap = TRUE; // Note that punc line was cap
            // Collect trailing punc
            bPunc = TRUE;
            if ( !*rpsEndCol.psz() || rpsEndCol.iChar >= pfldPunc->GetLength() ) // If at end, go to end of punc field
                rpsPuncEnd.iChar = rpsPuncEnd.pfld->GetLength();
            else // Else (not at end) go to end of column and back up to punc
                {
                rpsPuncEnd.iChar = rpsEndCol.iChar;
                if ( rpsPuncEnd.bLeftIsNonWhite() || rpsPuncEnd.bIsWhite() ) // Must be at beginning of column, might be coming up from phrase below
                    bPunc = FALSE;
                }
            rpsPuncEnd.bSkipWhiteLeft(); // Skip white to left
            rpsPuncStart = rpsPuncEnd; // Set begin to end
            if ( bPunc ) // If we should get punc, collect it
                while ( rpsPuncStart.bLeftIsWdSep() ) // Skip punc left
                    rpsPuncStart.iChar--;
            sTrailPunc = rpsPuncStart.psz(); // Collect trailing punc
            sTrailPunc = sTrailPunc.Left( rpsPuncEnd.psz() - rpsPuncStart.psz() );
            }

        if ( bCap ) // If cap needed, cap output
            {
            const char* psz = mksAnal; // Get start of output
            const CMChar* pmch = NULL;
            if ( pfldTo->plng()->bMChar(&psz, &pmch) && !pmch->bUpperCase() && pmch->bHasOtherCase() ) // If lower case multichar in output and it has other case
                { // Recapitalize output
                pmch = pmch->pmchOtherCase(); // Change to upper case
                mksAnal = pmch->sMChar() + psz; // Make upper case version of anal output string by adding upper case letter to rest of string
                }
            }

        // If keep punctuation, add punc to output
        mksAnal = sLeadPunc + mksAnal + sTrailPunc; // Add punc
            
        // Add punctuation length to length matched 
        iLenMatched += sLeadPunc.GetLength(); // Add leading punc len to matched length
        iLenMatched += sTrailPunc.GetLength(); // Add punc to matched length
        
        // Clear space below word before inserting first time
        CRecPos rpsClearStartCol = rpsStartCol;
        rpsClearStartCol.pfld = rpsClearStartCol.prec->pfldPrev( pfldTo );
        CRecPos rpsClearEndCol = rpsEndCol;
        rpsClearStartCol.ClearBundleBelow( rpsClearEndCol ); // Clear space below up to end column
        // Insert result into To field of record at current position
        int iLenInsert = mksAnal.GetLength() + 1;
        int iLenSpace = rpsEndCol.iChar - rpsStartCol.iChar;
        int iLenNeeded = iLenInsert - iLenSpace;
        if ( iLenNeeded > 0 )
            {
            rpsEndWd.Loosen( iLenNeeded, TRUE ); // Loosen as needed
            rpsEndCol.SetPos( rpsEndCol.iChar + iLenNeeded ); // Compensate end column
            *piLen += iLenNeeded; // Compensate length
            }
        pfldTo->Overlay( mksAnal, rpsStartCol.iChar, 0 ); // Overlay on the spaces
            
        // If current word is done, return
        if ( iProcNum == 0 ) // If first process, set end for other processes to go on down
            {
            if ( !rpsEndCol.bEndOfField() ) // If not at end of field, use actual length
                {
                *piLen = rpsEndCol.iChar - rpsStart.iChar; // Set length covered by process
                CField* pfldNext = rpsEndCol.prec->pfldNext( rpsEndCol.pfld );
                if ( FALSE ) /* !bAdapt() */ // || pfldNext->GetLength() > rpsEndCol.iChar ) // If adapting first time, don't stop, so rearrange process can see multiple words
                    return TRUE; // Stop first process after each word or phrase
                }
            else // Else (at end of field), use something large so lower processes will finish line
                {
                *piLen = 10000; // Set length to something large
                return TRUE; // Stop first process at its end
                }
            }
        else if ( rpsEndCol.iChar >= rpsStart.iChar + *piLen ) // Stop lower process when current word has been done
            {
            if ( !rpsEndCol.bEndOfField() ) // If not at end of field, use actual length
                *piLen = rpsEndCol.iChar - rpsStart.iChar; // Set length covered by process
            else
                *piLen = 10000; // Else (at end of field), use something large 
            return TRUE;
            }
        // Else (more morphemes in lower process), move to next morpheme
        rpsStartCol = rpsEndCol;
        }
    *piLen = 10000; // Set length to something large so lower processes will finish line
    return TRUE;
}                     

void CRearrangeProc::OnDocumentClose( CShwDoc* pdoc ) // Check for references to document being closed  
{
    if ( pdoc == m_pdoc ) // If the rule document is being closed, clear the pointer to its record
        {
        m_drflst.OnDocumentClose( pdoc ); // Clear references to document being closed
        m_pdoc = NULL; // Clear all pointers
		m_pind = NULL;
        m_prel = NULL;
        m_pmkrRule = NULL;
        m_pmkrDef = NULL;
        }
}

#ifdef typWritefstream // 1.6.4ac
void CRearrangeProc::WriteProperties( Object_ofstream& obs )
#else
void CRearrangeProc::WriteProperties( Object_ostream& obs )
#endif
{
    obs.WriteNewline();
    obs.WriteBeginMarker( psz_intprc, psz_Rearrange );
    obs.WriteBool( psz_bGenerateProc, m_bGenerateProc );

    m_pmkrFrom->WriteRef( obs, psz_From );
    m_pmkrTo->WriteRef( obs, psz_To );
    if ( m_pmkrPSFrom )
        m_pmkrPSFrom->WriteRef( obs, psz_PSFrom );
    if ( m_pmkrPunc )
        m_pmkrPunc->WriteRef( obs, psz_Punc );
    m_drflst.WriteProperties( obs ); // Write database
    obs.WriteBool( psz_bAdapt, m_bAdapt );      
	obs.WriteBool( psz_bKeepMorphemeBreaks, m_bKeepMorphemeBreaks );
	obs.WriteUnrecognizedSettingsInfo( m_sUnrecognizedSettingsInfo ); // 1.0cp Write unrecognized settings info

    obs.WriteEndMarker( psz_intprc );
}

BOOL CRearrangeProc::s_bReadProperties( Object_istream& obs, 
        CInterlinearProcList* pintprclst, CInterlinearProc** ppintprc )
{
    ASSERT( pintprclst );
    if ( !obs.bAtBeginMarker( psz_intprc, psz_Rearrange ) ) // If not a rearrange process, fail
        return FALSE;
    BOOL bReadBegin = obs.bReadBeginMarker( psz_intprc ); // Step over the marker
 
    CRearrangeProc* pintprc = new CRearrangeProc( pintprclst ); // Make a new rearrange process to load up          
    CMarkerSet* pmkrset = pintprclst->ptyp()->pmkrset();
    CMarker* pmkrFrom = NULL;
    CMarker* pmkrTo = NULL;
    CMarker* pmkrPSFrom = NULL;
    CMarker* pmkrPunc = NULL;
    BOOL bGenerateProc = FALSE;

    while ( !obs.bAtEnd() )
        {
        if ( pmkrset->bReadMarkerRef( obs, psz_From, pmkrFrom ) )
            ;
        else if ( pmkrset->bReadMarkerRef( obs, psz_To, pmkrTo ) )
            ;
        else if ( pmkrset->bReadMarkerRef( obs, psz_PSFrom, pmkrPSFrom ) )
            ;
        else if ( pmkrset->bReadMarkerRef( obs, psz_Punc, pmkrPunc ) )
            ;
        else if ( pintprc->pdrflst()->bReadProperties( obs ) )
            ;
        else if ( obs.bReadBool( psz_bGenerateProc, bGenerateProc ) )
            ;   
        else if ( obs.bReadBool( psz_bAdapt, pintprc->m_bAdapt ) )
            ;   
        else if ( obs.bReadBool( psz_bKeepMorphemeBreaks, pintprc->m_bKeepMorphemeBreaks ) )
            ;   
		else if ( obs.bUnrecognizedSettingsInfo( psz_intprc, pintprc->m_sUnrecognizedSettingsInfo  ) ) // 1.0cp Store any unrecognized settings info so as not to lose it
			;
        else if ( obs.bEnd( psz_intprc ) ) // If end or begin something else, break
            break;
        }
    if ( pmkrFrom && pmkrTo ) // If markers came in, set them in the process, and set the process pointer
        {
        pintprc->SetMarkers( pmkrFrom, pmkrTo );
        pintprc->SetPSMarker( pmkrPSFrom );
        pintprc->SetPuncMarker( pmkrPunc );
        *ppintprc = pintprc;
        if ( bGenerateProc ) // If this is a Generate process, set its flag and name appropriately
            pintprc->SetGenerateProc(); // Set this as a Generate process // 1.4ah Change to not use string resource id in interlinear
        }
    else // Else (incomplete process), don't make one
        {
        delete pintprc;
        *ppintprc = NULL;
        }
    return TRUE; // True means some input was eaten
}

BOOL CRearrangeProc::bModalView_Properties()
{
    if ( m_bGenerateProc )
        {
        CGenerateDlg dlg( this );
        return ( dlg.DoModal() == IDOK );
        }
    else
        {
		CRearrangeDlg dlg( this );
		return ( dlg.DoModal() == IDOK );
		}
}

//---------------------------------------------------------------
CGivenProc::CGivenProc( CInterlinearProcList* pintprclstMyOwner )
            : CInterlinearProc( pintprclstMyOwner )
{
	SetType( CProcNames::eGiven ); // 1.4ah Change to not use string resource id in interlinear
}

CGivenProc::~CGivenProc() // Destructor 
{
}

BOOL CGivenProc::bInterlinearize( CRecPos rpsStart, int* piLen, int iProcNum, int& iResult ) // Run interlinearization process    
{
    ASSERT( rpsStart.iChar <= rpsStart.pfld->GetLength() );
    CField* pfldFrom = rpsStart.pfldFindInBundle( pmkrFrom() ); // Find From field
    if ( !pfldFrom ) // If no earlier process generated the desired field, don't go further
        return FALSE;
    CField* pfldTo = rpsStart.pfldFindInBundleAdd( pmkrTo() ); // Find To field, if not present, insert it
    rpsStart.SetPosEx( rpsStart.iChar, pfldFrom ); // Set starting position to desired column in from field

    // Initialize variables for lookup
    CMString mksAnal( pfldTo->pmkr(), "" ); // Analysis output

    // Walk through morphemes of current word
    for ( CRecPos rpsStartCol = rpsStart; !rpsStartCol.bEndOfField(); ) // While not at end of string or line, terminates below at end of column
        {
        // Position variables, documented here so that they can be seen together, but not declared until initialized
        // rpsStartCol is Starting column, before begin punc
        // rpsStartWd is Start of word, after begin punc
        // rpsEndWd is End of word, before end punc
        // rpsEndPunc is End of word, after end punc
        // rpsEndCol is Ending column, start of next word
        // Set up leading punc of word      
        CRecPos rpsStartWd = rpsStartCol; // Start of word, after begin punc
        rpsStartWd.bSkipWhite(); // Move past space to start of next word
        rpsStartWd.bSkipPunc(); // Move past leading punc to start of word
        CRecPos rpsEndWd = rpsStartWd;
        if ( !rpsStartWd.bEndOfField() && 
                rpsStartWd.iChar > 0 && bIsMorphBreakChar( *(rpsStartWd.psz() - 1) ) ) // If we went past a suffix hyphen, back over it
            rpsStartWd.iChar -= 1;  
        // Find end of word
        if ( bIsMorphBreakChar( *rpsEndWd.psz() ) ) // If morph break char, include it 
            rpsEndWd.iChar++;
        rpsEndWd.bSkipWdChar(); // Move to end of word
        if ( bIsMorphBreakChar( *rpsEndWd.psz() ) ) // If morph break char, include it 
            rpsEndWd.iChar++;
        Str8 sCurWord( rpsStartWd.psz(), rpsEndWd.iChar - rpsStartWd.iChar ); // Save current word, to be parsed or used as copy if no parse
        // Find end of column
        CRecPos rpsEndCol = rpsEndWd; // Ending column, start of next word
        rpsEndCol.bSkipPunc(); // Move past trailing punc
        rpsEndCol.bSkipWhite(); // Move past spaces between words

        int iLenMatched = rpsEndWd.iChar - rpsStartWd.iChar; // Length of match for insert
        InsistNonZero( iLenMatched, 1 ); // Should always be something matched 

        // mksAnal = sCurWord; // Default to copy one word, used if no change made

        // If current word is done, return
        if ( iProcNum == 0 ) // If first process, set end for other processes to go on down
            {
            if ( !rpsEndCol.bEndOfField() ) // If not at end of field, use actual length
                {
                *piLen = rpsEndCol.iChar - rpsStart.iChar; // Set length covered by process
                return TRUE; // Stop first process after each word or phrase
                }
            else // Else (at end of field), use something large so lower processes will finish line
                {
                *piLen = 10000; // Set length to something large
                return TRUE; // Stop first process at its end
                }
            }
        else if ( rpsEndCol.iChar >= rpsStart.iChar + *piLen ) // Stop lower process when current word has been done
            {
            if ( !rpsEndCol.bEndOfField() ) // If not at end of field, use actual length
                *piLen = rpsEndCol.iChar - rpsStart.iChar; // Set length covered by process
            else
                *piLen = 10000; // Else (at end of field), use something large 
            return TRUE;
            }
        // Else (more morphemes in lower process), move to next morpheme
        rpsStartCol = rpsEndCol;
        }
    *piLen = 10000; // Set length to something large so lower processes will finish line
    return TRUE;
}                     

#ifdef typWritefstream // 1.6.4ac
void CGivenProc::WriteProperties( Object_ofstream& obs )
#else
void CGivenProc::WriteProperties( Object_ostream& obs )
#endif
{
    obs.WriteNewline();
    obs.WriteBeginMarker( psz_intprc, psz_Given );

    m_pmkrFrom->WriteRef( obs, psz_From );
    m_pmkrTo->WriteRef( obs, psz_To );
    obs.WriteBool( psz_bAdapt, m_bAdapt );      
	obs.WriteUnrecognizedSettingsInfo( m_sUnrecognizedSettingsInfo ); // 1.0cp Write unrecognized settings info

    obs.WriteEndMarker( psz_intprc );
}

BOOL CGivenProc::s_bReadProperties( Object_istream& obs, 
        CInterlinearProcList* pintprclst, CInterlinearProc** ppintprc )
{
    ASSERT( pintprclst );
    if ( !obs.bAtBeginMarker( psz_intprc, psz_Given ) ) // If not a Given process, fail
        return FALSE;
    BOOL bReadBegin = obs.bReadBeginMarker( psz_intprc ); // Step over the marker
 
    CGivenProc* pintprc = new CGivenProc( pintprclst ); // Make a new Given process to load up          
    CMarkerSet* pmkrset = pintprclst->ptyp()->pmkrset();
    CMarker* pmkrFrom = NULL;
    CMarker* pmkrTo = NULL;

    while ( !obs.bAtEnd() )
        {
        if ( pmkrset->bReadMarkerRef( obs, psz_From, pmkrFrom ) )
            ;
        else if ( pmkrset->bReadMarkerRef( obs, psz_To, pmkrTo ) )
            ;
        else if ( obs.bReadBool( psz_bAdapt, pintprc->m_bAdapt ) )
            ;   
		else if ( obs.bUnrecognizedSettingsInfo( psz_intprc, pintprc->m_sUnrecognizedSettingsInfo  ) ) // 1.0cp Store any unrecognized settings info so as not to lose it
			;
        else if ( obs.bEnd( psz_intprc ) ) // If end or begin something else, break
            break;
        }
    if ( pmkrFrom && pmkrTo ) // If markers came in, set them in the process, and set the process pointer
        {
        pintprc->SetMarkers( pmkrFrom, pmkrTo );
        *ppintprc = pintprc;
        }
    else // Else (incomplete process), don't make one
        {
        delete pintprc;
        *ppintprc = NULL;
        }
    return TRUE; // True means some input was eaten
}

BOOL CGivenProc::bModalView_Properties()
{
    CGivenDlg dlg( this );
    return ( dlg.DoModal() == IDOK );
}

//------------------------------------------------------------------------
CInterlinearProcList::~CInterlinearProcList()
{
    CInterlinearProc* pintprc = pintprcFirst();
    while (pintprc)
        {
        Delete(&pintprc);
        pintprc = pintprcFirst();
        }
}

static const char* psz_intprclst = "intprclst"; // Interlinear process list
static const char* psz_fglst = "fglst"; // Forced gloss start
static const char* psz_fglend = "fglend"; // Forced gloss end
static const char* psz_mbnd = "mbnd"; // Morpheme bound char
static const char* psz_mbrks = "mbrks"; // Morpheme break chars

#ifdef typWritefstream // 1.6.4ac
void CInterlinearProcList::WriteProperties( Object_ofstream& obs ) const
#else
void CInterlinearProcList::WriteProperties( Object_ostream& obs ) const
#endif
{
    CInterlinearProc* pintprc = pintprcFirst();
    if ( !pintprc ) // If no processes, don't write the markers
        return;
    obs.WriteNewline();
    obs.WriteBeginMarker( psz_intprclst );
    obs.WriteChar( psz_fglst, cForcedGlossStartChar() );
    obs.WriteChar( psz_fglend, cForcedGlossEndChar() );
    obs.WriteChar( psz_mbnd, cParseMorphBoundChar() );
    obs.WriteString( psz_mbrks, sMorphBreakChars() );
    for ( ; pintprc; pintprc = pintprcNext( pintprc ) )
        pintprc->WriteProperties( obs );
    obs.WriteNewline();
    obs.WriteEndMarker( psz_intprclst );
}          

BOOL CInterlinearProcList::bReadProperties( Object_istream& obs )
{
    if ( !obs.bReadBeginMarker( psz_intprclst ) )
        return FALSE;
    Str8 s;  
    while ( !obs.bAtEnd() )
        {
        CInterlinearProc* pintprc = NULL;
        if ( CInterlinearProc::s_bReadProperties( obs, this, &pintprc ) ) // If we read a process, add it to list
            {
            if ( pintprc ) // If valid process, add it to the list
                Add( pintprc );
            }
        else if ( obs.bReadString( psz_fglst, s ) ) // If read forced gloss start, set it
            SetForcedGlossStartChar( *s );
        else if ( obs.bReadString( psz_fglend, s ) ) // If read forced gloss end, set it
            SetForcedGlossEndChar( *s );
        else if ( obs.bReadString( psz_mbnd, s ) ) // If read morph bound char, set it
            SetParseMorphBoundChar( *s );
        else if ( obs.bReadString( psz_mbrks, s ) ) // If read morph break chars, set them
            SetMorphBreakChars( s );
        else if ( obs.bEnd( psz_intprclst ) ) // If end of list, stop
            break;
        }
   return TRUE;
}

void CInterlinearProcList::IndexUpdated( CIndexUpdate iup )
{
    for ( CInterlinearProc* pintprc = pintprcFirst(); pintprc; pintprc = pintprcNext( pintprc ) )
        pintprc->IndexUpdated( iup ); // For every process, update it
}

BOOL CInterlinearProcList::bInterlinearize( CRecPos& rpsCur, BOOL bAdapt, BOOL bWord ) // Run interlinearization processes
{
    CRecPos rps = rpsCur; // Use temp so that changing m_rpsCur underneath won't change this 
    CRecPos rpsStart = rpsCur; // Remember starting place
    int iLen = 0;
    BOOL bMore = TRUE;
    while ( bMore && !rps.bEndOfField() )
        { // Call all the processes at this position, first one sets iLen to show how far it got. The others work within that range.
		int iResult = 0; // 1.3dg Fix bug of not asking for ambiguity after failure // Result of interlinear process, 0 success, 1 parse failure
        int iProcNum = 0; // Tell process which number it is, first tells how far it got, others use that to limit themselves
		BOOL bAdaptProc = FALSE; // Used to tell if an adapt process has already been seen. All processes after first adapt are considered adapt
        for ( CInterlinearProc* pintprc = pintprcFirst(); pintprc && bMore; pintprc = pintprcNext( pintprc ) )
            {
			if ( pintprc->bAdapt() ) // If this is adapt, it and all after it will be adapt.
				bAdaptProc = TRUE;
            if ( bAdapt && !bAdaptProc ) // If we are adapting and this is not an adapt process, skip it
                continue;
            if ( !bAdapt && bAdaptProc ) // If we are not adapting and this is an adapt process, skip it
                continue;
            bMore = pintprc->bInterlinearize( rps, &iLen, iProcNum++, iResult ); // If user cancelled at disambig or insert, stop interlinearizing
            }
		if ( iLen == 0 ) // 1.4ke Fix bug of possible hang on interlinearize if given process first
			iLen = 1;
        rps.iChar += iLen; // Move to next column
        rps.SetPos( __min( rps.iChar, rps.pfld->GetLength() ) ); // Don't let past end
        if ( bWord ) // If single word (not first time) break
            {
            CField* pfldNext = rps.prec->pfldNext( rps.pfld );
            if ( pfldNext->GetLength() > rps.iChar ) // If below is already interlinearized, break
                break;                  
            }
        }
    if ( bWord ) // If single word, tighten if possible
        rps.TightenLeft( &rps.iChar );  
    rpsCur = rps; // Set m_rpsCur to changed value for word 
	return bMore; // Return false if user cancelled
}

void CInterlinearProcList::OnDocumentClose( CShwDoc* pdoc ) // Check for references to document being closed
{
    for ( CInterlinearProc* pintprc = pintprcFirst(); pintprc; pintprc = pintprcNext( pintprc ) )
        pintprc->OnDocumentClose( pdoc );   
}

void CInterlinearProcList::MarkerUpdated( CMarkerUpdate& mup ) // update mrflsts
{
    for ( CInterlinearProc* pintprc = pintprcFirst(); pintprc; pintprc = pintprcNext( pintprc ) )
        pintprc->MarkerUpdated( mup );  
}

void CInterlinearProcList::UpdatePaths() // Update paths if project moved
{
	for ( CInterlinearProc* pintprc = pintprcFirst(); pintprc; pintprc = pintprcNext( pintprc ) )
		pintprc->UpdatePaths();
}

void CInterlinearProcList::WritePaths( ofstream& ostr ) // Update paths if project moved
{
	for ( CInterlinearProc* pintprc = pintprcFirst(); pintprc; pintprc = pintprcNext( pintprc ) )
		pintprc->WritePaths( ostr );
}

BOOL CInterlinearProcList::bModalView_Elements() // Interface
{
    return FALSE; // not used anymore, access thru database type properties tabbed dialog
}
 
//------------------------------------------------------
CField* CRecPos::pfldFindInBundle( const CMarker* pmkr ) const  // Find first field with a specified marker
{
	if ( !bInterlinear() && !bFirstInterlinear() ) // 5.99b Protect against call from non-interlinear field // 5.99p Fix failure on first field that is going to become interlinearized
		return NULL; // 5.99e Change to return NULL if not found
    for ( CRecPos rps = rpsFirstInBundle(); ; rps.pfld = rps.pfldNext() )
        {
        if ( rps.pfld->pmkr() == pmkr )
            return rps.pfld;
        if ( rps.bLastInBundle() )
            break;
        }   
    return NULL; // If not found, fail
}

CField* CRecPos::pfldFindInBundleAdd( CMarker* pmkr, const CMarker* pmkrBefore ) const  // Find first field with a specified marker, add if not present
{
    CField* pfld = pfldFindInBundle( pmkr ); // Try to find field in bundle
    if ( pfld ) // If found, return it
        return pfld;
    CField* pfldBefore = NULL;
    if ( pmkrBefore ) // If there is a marker to insert after, try to find it
        pfldBefore = pfldFindInBundle( pmkrBefore ); // Try to find field to insert after in bundle
    if ( !pfldBefore )  // If not found, use last in bundle
        {
		CRecPos rps = rpsFirstInBundle();
        for ( ; ; rps.pfld = rps.pfldNext() )
            if ( rps.bLastInBundle() )
                break;
        pfldBefore = rps.pfld;
        }
    pfld = new CField( pmkr ); // If not found, add as a new field      
    prec->InsertAfter( pfldBefore, pfld ); // Insert after the field that should be before it
    return pfld; 
}

BOOL bGiven( CRecPos rps )
{
	CField* pfld = rps.pfld;
	CMarker* pmkr = pfld->pmkr();
	CMarkerSet* pmkrset = pmkr->pmkrsetMyOwner();
	CDatabaseType* ptyp = pmkrset->ptyp();
	CInterlinearProcList* pintprclst = ptyp->pintprclst();
	for ( CInterlinearProc* pintprc = pintprclst->pintprcFirst(); pintprc; 
			pintprc = pintprclst->pintprcNext( pintprc ) )
	{
		if ( pintprc->iType() == CProcNames::eGiven ) // 1.4ah Change to not use string resource id in interlinear
			if ( pintprc->pmkrTo() == pmkr )
				return TRUE;
	}
	return FALSE;

}

void CRecPos::ClearBundleBelow( CRecPos rpsEndCol ) // Clear space below up to end column
{
    int iWdSpaceLen = rpsEndCol.iChar - iChar;
    if ( rpsEndCol.bEndOfField() ) // If at end of line, use a large number for delete so it will clear end of line
        iWdSpaceLen = 1000;            
    for ( CRecPos rps = *this; !rps.bLastInBundle(); ) // For each line of bundle after the first, clear space underneath
    {
		rps.NextFieldExtend(); // Move rps to next field and extend it to reach next word start
		if ( !bGiven( rps ) )
		{
			rps.Clear( rpsEndCol ); // Clear space, deleting all to end if necessary
		}
    } 
}

void CRecPos::Clear( CRecPos rpsEndCol ) // Clear space up to end column or end
{
    if ( rpsEndCol.bEndOfField() ) // If at end of top line, delete to end of line
        pfld->Delete( iChar, pfld->GetLength() - iChar ); // Delete end of current line
    else
        pfld->Overlay( "", iChar, rpsEndCol.iChar - iChar ); // Else overlay spaces, extending line if necessary
}

void CRecPos::LoosenWordParse( int iNum, BOOL bEvenAtEnd ) // Loosen next words by iNum spaces // 1.6.4s 
{
    ASSERT( iNum >= 0 );
	CRecPos rpsFirst = rpsFirstInBundle( iChar ); // 1.6.4s Find first in bundle to look for next word start
	CRecPos rpsNextAlign = rpsNextWdBeginBundle(); // 1.6.4s Find next align positionn
	int iNextAlign = rpsNextAlign.iChar; // 1.6.4s Next align pos, end of first line if none found
	if ( iNextAlign >= rpsFirst.pfld->GetLength() ) // 1.6.4s If no align, loosen only first line
		{
        if ( rpsFirst.bLeftIsNonWhite() ) // If inside a word, move to end
            rpsFirst.bSkipNonWhite(); // Move to spaces
        rpsFirst.bSkipWhite(); // Move to end of spaces to see if at end of field
        if ( rpsFirst.iChar < rpsFirst.pfld->GetLength() || bEvenAtEnd ) // Don't add spaces to end of field, unless specially told to
            rpsFirst.pfld->Insert( ' ', rpsFirst.iChar, iNum ); // Loosen this field
		return; // 1.6.4s Do only first line
		}
    for ( CRecPos rps = rpsFirstInBundle( iChar ); ; rps.NextField( iChar ) ) // For each field in bundle // 1.6.4s 
		{
        rps.pfld->Insert( ' ', iNextAlign, iNum ); // Loosen this field // 1.6.4s 
		Str8 sFld = rps.pfld->sContents(); // 1.6.4s Debug show
		const char* pszFld = sFld; // 1.6.4s 
        if ( rps.bLastInBundle() ) // 1.6.4s Include last in bundle
            break; // 1.6.4s 
		}
}

void CRecPos::Loosen( int iNum, BOOL bEvenAtEnd ) // Loosen next words by iNum spaces
{
    ASSERT( iNum >= 0 );
    for ( CRecPos rps = rpsFirstInBundle( iChar ); ; rps.NextField( iChar ) ) // For each field in bundle
        {
        int iEnd = rps.pfld->GetLength(); // Get pos of end of field
        if ( *rps.psz( iEnd - 1 ) == '\n' ) // Reduce for nl
            iEnd--;
        if ( rps.bLeftIsNonWhite() ) // If inside a word, move to end
            rps.bSkipNonWhite(); // Move to spaces
        rps.bSkipWhite(); // Move to end of spaces to see if at end of field
        if ( iEnd > rps.iChar || bEvenAtEnd ) // Don't add spaces to end of field, unless specially told to
            rps.pfld->Insert( ' ', rps.iChar, iNum ); // Loosen this field
        if ( rps.bLastInBundle() )
            break;
        }   
}

void CRecPos::TightenLeft( int* piAdjust ) const // Tighten current words as much as possible
{ // iAdjust is a variable to be adjusted for position   
    CRecPos rps = *this;
    int iCharTighten = rps.iChar; // Note place to tighten
    // Find out how much to tighten
    int iLarge = 1000; // A large number to detect later if unchanged
    int iNumExtra = iLarge; // Number to tighten, start high for min
    for ( rps = rpsFirstInBundle(); ; rps.pfld = rps.pfldNext() )
        {
        rps.SetPosEx( iCharTighten ); // Set to tighten pos
        rps.bSkipWhiteLeft(); // Move left past white
        int iNum = iCharTighten - rps.iChar; // Calculate how many spaces after prev word   
        iNumExtra = __min( iNumExtra, iNum ); // Find lowest number of extra spaces
        if ( rps.bLastInBundle() )
            break;
        }   
    // Tighten each line
    if ( iNumExtra <= 1 ) // If no extra spaces, can't tighten, so don't try
        return;
    for ( rps = rpsFirstInBundle(); ; rps.pfld = rps.pfldNext() )
        {
        rps.SetPos( iCharTighten );
        rps.bSkipWhiteLeft(); // Move left past white
        rps.pfld->Delete( iCharTighten - iNumExtra, iNumExtra - 1 ); // Tighten this field (delete protects itself against delete past end)
        if ( rps.bLastInBundle() )
            break;
        }
    if ( piAdjust ) 
        *piAdjust -= iNumExtra - 1; // Adjust for number removed        
}

void CRecPos::Tighten( int* piAdjust, BOOL bExtend ) const // Tighten next words as much as possible
{ // iAdjust is a variable to be adjusted for position   
    CRecPos rps = *this;
    CRecPos rpsEnd = rps;
    rpsEnd.bSkipWhite();
    if ( rpsEnd.bEndOfField() ) // If cursor was in spaces at end of line, don't tighten
        return;
    rps.bSkipNonWhite(); // Find end of word in current field to adjust at best place
    int iCharTighten = rps.iChar; // Note place to tighten
    // Find out how much to tighten
    int iNumExtra = 1000; // Number of spaces before start of next word, start large so it will come down for minimum
    int iFirstPos = 0; // Tighten position in first line 
	BOOL bLastHasNl = FALSE; // 1.4vyk Remember nl at end of last line to not delete or add it
    for ( rps = rpsFirstInBundle(); ; rps.pfld = rps.pfldNext() )
        {
        rps.pfld->Extend( iFirstPos );
        rps.SetPosEx( iCharTighten );
        rps.bSkipNonWhite(); // Move past non-white
        int iStart = rps.iChar; // Remember end of word
        rps.bSkipWhite(); // Move past white
        int iNum = rps.iChar - iStart; // Calculate how many spaces before next word    
        if ( rps.bFirstInterlinear() ) // Remember tighten position in first line
            iFirstPos = rps.iChar;
        if ( !( rps.bFirstInterlinear() && iNum == 0 ) ) // Don't set lowest at end of first line
            iNumExtra = __min( iNumExtra, iNum ); // Find lowest number of extra spaces
        if ( rps.bLastInBundle() )
			{
			int iLen = rps.pfld->GetLength(); // 1.4vyk 
			if ( iLen > 0 && *(rps.pfld->psz( iLen - 1 )) == '\n' ) // 1.4vyk 
				bLastHasNl = TRUE; // 1.4vyk Remember nl at end of last line to not delete or add it
            break;
			}
        }   
    // Tighten each line
    if ( iNumExtra > 1 ) // If no extra spaces, can't tighten, so don't try
        {
        for ( rps = rpsFirstInBundle(); ; rps.pfld = rps.pfldNext() )
            {
            rps.SetPos( iCharTighten );
            rps.bSkipNonWhite(); // Move past non-white
            rps.pfld->Delete( rps.iChar, iNumExtra - 1 ); // Tighten this field (delete protects itself against delete past end)
            if ( rps.bLastInBundle() )
                break;
            }
        if ( piAdjust ) 
            *piAdjust -= iNumExtra - 1; // Adjust for number removed
        }           
    if ( !bExtend ) // If not extend, trim lines and put nl back on
        rps.TrimBundle( bLastHasNl ); // 1.4vyk Remember nl at end of last line to not delete or add it
}

CRecPos CRecPos::rpsBeginSentence() const // Find beginning of sentence
{
// CRecPos rps = *this;
//!! Find beginning of sentence
return rpsFirstInBundle();
}
CRecPos CRecPos::rpsEndSentence() const // Find end of sentence
{
// CRecPos rps = *this;
//!! Find end of sentence
return rpsEndOfBundle();
}

void CRecPos::TrimBundle( BOOL bAddNl ) // Trim spaces and nl's off ends of all lines in bundle 
{
	CRecPos rps = rpsFirstInBundle();
    for ( ; ; rps.pfld = rps.pfldNext() )
        {
        rps.pfld->Trim(); // Trim trailing nl's and spaces
        if ( rps == *this ) // If current line trimmed, be sure position is not beyond end 
            SetPos( __min( iChar, rps.pfld->GetLength() ) );
        if ( rps.bLastInBundle() )
            break;
        }
    if ( bAddNl ) // If add nl to clean up, add it now  
        rps.pfld->SetContent( rps.pfld->sContents() + '\n' ); // Add nl at end of bundle
}

BOOL CRecPos::bAllGiven( CInterlinearProcList* pintprclst ) // 1.4kb Return true if bundle contains only Given lines
{
	BOOL bMoreThanOne = FALSE;
    for ( CRecPos rps = rpsFirstInBundle(); ; rps.pfld = rps.pfldNext() )
        {
		CMarker* pmkr = rps.pfld->pmkr();
        for ( CInterlinearProc* pintprc = pintprclst->pintprcFirst(); pintprc; pintprc = pintprclst->pintprcNext( pintprc ) )
			if ( pmkr == pintprc->pmkrFrom() || pmkr == pintprc->pmkrTo() ) // If from this process, and this process is not a Given proc, return FALSE
				{
				if ( pintprc->iType() != CProcNames::eGiven )
					return FALSE;
				}
        if ( rps.bLastInBundle() )
            break;
		bMoreThanOne = TRUE;
        }
	return bMoreThanOne;
}

int iWordNumChars( const char* psz ) // 1.4kb Count the number of chars in a word
	{
	const char* pszStart = psz;
	while( *psz && *psz != ' ' && *psz != '\n' )
		psz++;
	return psz - pszStart;
	}

int CRecPos::iLongestWordNumChars( int iPos ) // 1.4kb Find longest word at a particular position
	{
	int iLongest = 0; // Find number of chars in longest word starting at this position
	for ( CRecPos rps = rpsFirstInBundle(); ; rps.pfld = rps.pfldNext() )
		{
		if ( rps.pfld->GetLength() > iPos ) // If more in field, measure current word
			iLongest = __max(  iWordNumChars( rps.psz( iPos ) ), iLongest ); // If longer, remember as longest
		if ( rps.bLastInBundle() )
			break;
		}
	return iLongest;
	}

void RemoveExtraSpaces( Str8& s, int iWordEnd ) // 1.4kb Remove extra spaces CString
	{
	while ( s.GetLength() > iWordEnd && s.GetChar( iWordEnd ) == ' ' )
		s.Delete( iWordEnd );
	}


void AddSpacesUpToLen( Str8& s, int iWordEnd, int iDesiredLen ) // 1.4kb Add extra spaces up to desired length
	{
	RemoveExtraSpaces( s, iWordEnd + 1 ); // Remove extra spaces
	if ( s.GetLength() <= iWordEnd + 1 ) // Don't add spaces at end of string
		return;
	for ( int i = 0; i < iDesiredLen - iWordEnd - 1; i++ ) // Insert as many spaces as are needed
		s.Insert( iWordEnd, " " );
	}

void ExpandFollowingWordSpace( Str8& s, int iPos, int iDesiredLen ) // 1.4kb Expand following word space to allow for iNum chars
	{
	int iWordLen = iWordNumChars( (const char*)s + iPos ); // See how many chars in this word
	int iWordEnd = iPos + iWordLen; // Note end of word
	AddSpacesUpToLen( s, iWordEnd, iDesiredLen ); // Add extra spaces up to desired length
	}

int CRecPos::iAlignBundleAtPos( int iPos ) // 1.4kb Correct interlinear alignment of bundle at a position
	{
	int iLongest = iLongestWordNumChars( iPos ); // Find number of chars in longest word starting at this position
	for ( CRecPos rps = *this; ; rps.pfld = rps.pfldNext() ) // For each line in bundle align at this pos // 1.4qrf Align whole bundle start at current field, for tab align on import
		{
		Str8 s = rps.pfld->sContents();
		ExpandFollowingWordSpace( s, iPos, iLongest + iPos + 1 ); // Expand following word spaces to align next words
		rps.pfld->SetContent( s );
		if ( rps.bLastInBundle() )
			break;
		}
	return iLongest;
	}

BOOL CRecPos::bAlignWholeBundle() // 1.4kb Correct interlinear alignment of whole bundle
{
	int iCurPos = 0;
	BOOL bDone = FALSE;
	while ( !bDone )
		{
		int iLongest = iAlignBundleAtPos( iCurPos ); // Align at this position
		if ( iLongest > 0 ) // If more to align, move to next position
			iCurPos += iLongest + 1; // Allow for longest word and the space after it
		else
			bDone = TRUE;
		}
	return TRUE;
}

//------------------------------------------------------
BOOL CShwView::bInterlinearize( BOOL bAdapt, BOOL bCont ) // Interlinearize text
{
	CShwView* pviewFrom = Shw_papp()->pviewJumpedFrom(); // Get last view jumped from
	if ( !pdoc()->pintprclst()->bWordParse() ) // 1.6.4zn Fix bug reinterlinearize word parse moving to wrong place
		{
	    if ( !bValidate() ) // validate current view first
		    return FALSE;
		if ( !pviewFrom ) // 1.6.4zn Fix bug of return from jump finding cursor at top of text window
			if ( !Shw_papp()->bValidateAllViews() ) // then validate everyone
				return FALSE;
		}
    if ( bSelecting( eAnyText ) ) // If selecting text, start interlinearize at beginning of selection
        m_rpsCur = rpsSelBeg();
    ClearSelection(); // Prevent odd selection results

    CInterlinearProcList* pintprclst = GetDocument()->pintprclst(); // Get interlinear proc list
    if ( bAdapt && !pintprclst->bAdapt() ) // If request adapt, but no adapt procs, interlinearize instead. This lets one button call adapt and do both jobs.
        bAdapt = FALSE;
    if ( !bAdapt && !pintprclst->bNonAdaptInterlin() ) // If request interlinearize, but only adapt procs, adapt
        bAdapt = TRUE;

	if ( !m_rpsCur.bInterlinear() && !m_rpsCur.bFirstInterlinear() ) // 5.99t If not in interlinear, freshen interlinear settings of markers to fix bug that sometimes they get lost
		ptyp()->SetInterlinearMkrs();

	if ( !m_rpsCur.bInterlinear() && Shw_pProject()->bAutoSave() ) // 6.0m If autosaving, save before interlinearize only if this is the first time this text is being interlinearized, saving on every re-interlinearize was a little too often
		{
		CShwDoc* pdoc = GetDocument(); // 1.4qzhb
		pdoc->OnSaveDocument( sUTF8( pdoc->GetPathName() ) ); // 1.4qzhb Fix problem of auto save on interlinear in U build
		}

	BOOL bMore = TRUE;
	BOOL bOK = TRUE;
    do
	{
		if ( m_rpsCur.bInterlinear()  // If in interlinear other than at end of last line, move to top of bundle
				&& ( !m_rpsCur.bEndOfBundle() || m_rpsCur.pfld->GetLength() == 0 ) ) // 1.6.4u Interlinearize if on empty last line
			m_rpsCur = m_rpsCur.rpsFirstInBundle( m_rpsCur.iChar );
		else // Else move forward to interlinear
			{
			if ( m_rpsCur.pfldNext() && !m_rpsCur.bFirstInterlinear() ) // 1.4mc Fix bug (1.4kw) of interlinearize at end of first line going to next bundle
				m_rpsCur.SetPos( 0, m_rpsCur.pfldNext() ); // 1.4kw Fix bug (1.4kb) of interlinearize not moving from last line in bundle
			while ( !( m_rpsCur.bFirstInterlinear() // Find a first interlinear field (usually top of bundle, but user editing may violate that assumption)
					|| ( m_rpsCur.pfld->pmkr()->bInterlinear() // Or two interlinears in a row to detect the first of a set of Given
						&& m_rpsCur.pfldNext() // 1.4kd Fix bug (1.4kb) of crash on interlinearize at bottom of record
						&& m_rpsCur.pfldNext()->pmkr()->bInterlinear() ) ) ) // 1.4kb Detect first of a set of Given as start of interlinear
				{
				CField* pfld = m_rpsCur.pfldNext(); // Get next field
				if ( !pfld ) // If end of record, return without doing anything
					{
					m_rpsCur.SetPos( m_rpsCur.pfld->GetLength() ); // Move to end of field
					pntPosToPnt( m_rpsCur ); // Display caret at new place
					return FALSE;         
					}
				m_rpsCur.SetPos( 0, pfld ); // Set to beginning of next field
				}
			}

#ifdef _MAC
		if ( m_rpsCur.pfld->plng()->bRightToLeft() ) // If Arabic field
				m_rpsCur.pfld->OverlayAll( '\xA0', ' ' ); // Change any Arabic space 160 to regular space
#endif
		if ( m_rpsCur.bIsWhite() && m_rpsCur.bLeftIsWhite() ) // If in space between words, move left to end of prev word
			m_rpsCur.bSkipWhiteLeft();
		m_rpsCur.bSkipNonWhiteLeft(); // Move cursor position to beginning of nearest word to left
		BOOL bLastHasNl = FALSE; // 1.4vym 
		CRecPos rpsLast = m_rpsCur; // 1.4vyq 
		if ( rpsLast.bInterlinear() ) // 1.4vyq Fix bug (1.4vyp) of assert on interlinearize new line
			rpsLast = rpsLast.rpsEndOfBundle(); // 1.4vym See if last field has nl
		int iLen = rpsLast.pfld->GetLength(); // 1.4vyk  // 1.4vym 
		if ( iLen > 0 && *(rpsLast.pfld->psz( iLen - 1 )) == '\n' ) // 1.4vyk  // 1.4vym 
			bLastHasNl = TRUE; // 1.4vyk Remember nl at end of last line to not delete or add it // 1.4vym 
		m_rpsCur.TrimBundle(); // Trim ends of fields
		BOOL bNew = m_rpsCur.bLastInBundle(); // Remember that this is being newly interlinearized, did not have interlinear fields before
		if ( bNew ) // If first interlinearize, make sure no multiple lines
			{
			m_rpsCur.pfld->OverlayAll( '\n', ' ' ); // Change any nl's to spaces
			m_rpsCur.iChar = 0; // ALB 5.96u If newly interlinearizing, start at beginning of field, to fix bug of odd space being left under first part of interlinear
			}
		else // If fields present, but next field is not filled out, treat as new so final wrap will happen
			{
			CRecPos rps = m_rpsCur;
			rps.pfld = rps.prec->pfldNext( rps.pfld );
			int iLenNext = rps.pfld->GetLength();
			if ( iLenNext < m_rpsCur.iChar )
				bNew = TRUE;
			}

		if ( m_rpsCur.bAllGiven( pintprclst ) ) // 1.4kb On interlinearze, if all given, redo alignment
			{
			m_rpsCur.bAlignWholeBundle();
			m_rpsCur.TrimBundle( bLastHasNl ); // Trim ends of fields and add nl to last line // 1.4vym Don't add nl to last line unless it already had one
			m_rpsCur = m_rpsCur.rpsEndOfBundle(); // Move cursor to end of sentence
			pntPosToPnt( m_rpsCur ); // Display caret at new place
			SetCaretPosAdj(); // Scroll the bottom of bundle into view
			pntPosToPnt( m_rpsCur ); // Display caret at new place
			SetScroll( TRUE ); // Update scroll in case fields were added
			SetModifiedFlag(); // Set modified to trigger save question on close
			return TRUE;
			}
    
		if ( bAdapt && bNew && pintprclst->bNonAdaptInterlin() ) // If request adapt, but there are interlinear procs that have not yet been run, do interlinearize first
			{
			CRecPos rps = m_rpsCur; // Remember current pos
			bMore = pintprclst->bInterlinearize( m_rpsCur, FALSE, !bNew ); // Run interlinearization with adapt false
			m_rpsCur = rps; // Restore current pos
			}

		bMore = pintprclst->bInterlinearize( m_rpsCur, bAdapt, !bNew ); // Run interlinearization processes
		BOOL bEmptyField = ( m_rpsCur.pfld->sContents() == "" ); // 1.2hg Fix bug of hang on empty field in Adapt All
		m_rpsCur.TrimBundle( TRUE ); // Trim ends of fields and add nl to last line

		if ( m_rpsCur.bEndOfField() ) // If at end of line
			{
			m_rpsCur = m_rpsCur.rpsEndOfBundle(); // Move cursor to end of sentence
			if ( bNew ) // If newly interlinearized, break lines, otherwise leave alone
				m_rpsCur = rpsWrapInterlinear( m_rpsCur ); // Break interlinear lines as necessary
			}
		else // Else (existing interlinear), make sure bottom of bundle is in view
			{
			CRecPos rpsSave =  m_rpsCur; // Remember current pos
			while ( !m_rpsCur.bLastInBundle() ) // Move to bottom of bundle so whole bundle will show
				bEditDown();
			pntPosToPnt( m_rpsCur ); // Display caret at new place
			SetCaretPosAdj(); // Scroll the bottom of bundle into view
			m_rpsCur = rpsSave;
			}
		if ( bEmptyField ) // 1.2hg If field was empty, move to next field
			{
			bEditDown(); // 1.2hg Move down to blank line that was added at end
			bMore = bEditDown(); // 1.2hg Try to move to next field, if none, stop
			bOK = bMore; // 1.2hg If empty field at end of record, return false to say end of record
			}
	} while ( bCont && bMore );

    pntPosToPnt( m_rpsCur ); // Display caret at new place
    SetScroll( TRUE ); // Update scroll in case fields were added
    SetModifiedFlag(); // Set modified to trigger save question on close
	return bOK;
}   

