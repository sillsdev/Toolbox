// interlin.h Interlinear classes Alan Buseman 7 Feb 95

#ifndef InterLin_H
#define InterLin_H

#include "cdbllist.h" // For derivation
#include "cdblel.h" // For derivation
#include "project.h" // For update path
#include "crecpos.h"
#include "trie.h"    
#include <fstream>

class Object_istream;  // obstream.h
class Object_ostream;  // obstream.h

class CProcNames { // Provides a list of name id's of derived process classes for interface to show user a list of processes
public:
    enum { eParse, eLookup, eRearrange, eGenerate, eGiven, eNumProcs }; // Add new procs here, eNumProcs is the number of processes
//    static const unsigned int iProcID[ eNumProcs ]; // Initialized at top of interlin.cpp // Note that getting eNumProcs right requires that enum definition starts at zero for first symbol and increments by one for each symbol in the enum
    static const Str8 sProcName[ eNumProcs ]; // 1.4ah Change to not use string resource id in interlinear // Initialized at top of interlin.cpp // Note that getting eNumProcs right requires that enum definition starts at zero for first symbol and increments by one for each symbol in the enum 
};

class CInterlinearProcList; // Needed for owner of process

class CInterlinearProc : public CDblListEl { // Base class for interlinear processes, Hungarian intprc
protected:
    CInterlinearProcList* m_pintprclstMyOwner; // Owning list
    CMarkerPtr m_pmkrFrom; // From marker in interlinear text
    CMarkerPtr m_pmkrTo; // To marker in interlinear text  
    UINT m_iType; // Type of process, used to test for process type, such as Given // 1.4ah Change to not use string resource id in interlinear
    BOOL m_bAdapt; // True if adaptation process
	Str8 m_sUnrecognizedSettingsInfo; // 1.0cp Don't lose settings info from later versions

public:
    CInterlinearProc( CInterlinearProcList* pintprclstMyOwner ) // Base constructor
        {
        m_pintprclstMyOwner = pintprclstMyOwner;
        m_pmkrFrom = pmkrset()->pmkrFirst(); // Default to some marker so controls can get at language
        m_pmkrTo = m_pmkrFrom;
        m_iType = 0; // 1.4ah Change to not use string resource id in interlinear
        m_bAdapt = FALSE;
        }
        
    virtual ~CInterlinearProc() {}
    virtual BOOL bInterlinearize( CRecPos rpsStart, int* piLen, int iProcNum, int& iResult ) = 0; // Run interlinearization process   
    virtual BOOL bSpellCheckField( CRecPos rpsCur, CRecPos rpsEnd ) // Run spell check process  
        { return FALSE; }

    virtual void IndexUpdated( CIndexUpdate iup ) {} // Update all references to record in all tries

    virtual void OnDocumentClose( CShwDoc* pdoc ) {}    

    virtual void MarkerUpdated( CMarkerUpdate& mup ) {} // update mrflsts

    static BOOL s_bReadProperties( Object_istream& obs,
            CInterlinearProcList* pintprclst, CInterlinearProc** ppintprc );
#ifdef typWritefstream // 1.6.4ac
    virtual void WriteProperties( Object_ofstream& obs ) = 0;
#else
    virtual void WriteProperties( Object_ostream& obs ) = 0;
#endif

    virtual BOOL bStopAtSemicolon() { return FALSE; } // True if keep punc
    
    virtual const char* pszGlossSeparator() const { ASSERT(FALSE); return NULL; } // only in CLookupProc
    virtual void SetGlossSeparator( const char* pszSep ) { ASSERT(FALSE); }

	virtual BOOL bLookup( const char* psz, CFieldList* pfldlst, int* piLenMatched ) { return FALSE; } // External interface for search
    virtual BOOL bLookupProc() { return FALSE; } // True if this is a lookup process that is not a parse process
    virtual BOOL bParseProc() { return FALSE; } // True if this is a parse process
    virtual BOOL bInsertFail() { return FALSE; } // True if this is a lookup or parse process that is inserting failures into the lexicon

    virtual void ClearRefs() {} // clear ref ptrs in preparation for database type deletion

    virtual void MakeRefs() {} // turn marker refs into CMarkerPtrs
    
    Str8 sName() const // Name of process for interface
		{ return CProcNames::sProcName[ m_iType ]; } // 1.4ah Change to not use string resource id in interlinear
    void SetType( UINT iType ) // Set process type // 1.4ah Change to not use string resource id in interlinear
        { m_iType = iType; }
    UINT iType() // Return process type, used to test for process type, such as Given // 1.4ah Change to not use string resource id in interlinear
        { return m_iType; }

    void SetAdapt( BOOL bAdapt ) // Set process as adapt if true
        { m_bAdapt = bAdapt; }
    CInterlinearProcList* pintprclst() const // Owner process list
        { return m_pintprclstMyOwner; }
    CMarkerSet* pmkrset() const; // Owner database type marker set

    void SetMarkers( CMarker* pmkrFrom, CMarker* pmkrTo )
        { m_pmkrFrom = pmkrFrom; m_pmkrTo = pmkrTo; }
    CMarker* pmkrFrom() const // Interlinear From marker
        { return m_pmkrFrom; }
    CMarker* pmkrTo() const // Interlinear To marker
        { return m_pmkrTo; }
    BOOL bAdapt() const // True if adapt process
        { return m_bAdapt; }

    char cForcedGlossStartChar() const;
    char cForcedGlossEndChar() const;
    char cParseMorphBoundChar() const;
    Str8 sMorphBreakChars() const;
    BOOL bIsMorphBreakChar( const char c ) const;
    char cMorphBreakChar( int i ) const; // Return requested morph break char
    char cMorphBreakChar( const char cDefault, const CLangEnc* plng ) const; // 1.4ywp Return first morph break char if legacy IPA93, otherwise default

	virtual BOOL bApplyCC() { return FALSE; } // True if applying CC to lookup failures

	virtual void AddGlossesToList( CFieldList *pfldlstAnal, int iStartPos ) {} // Do lookup of each morpheme to add gloss and part of speech

	virtual void UpdatePaths() {}; // Update paths if project moved
	virtual void WritePaths( std::ofstream& ostr ) {}; // Write paths in use

    virtual BOOL bModalView_Properties() = 0;
}; // End class CInterlinearProc

class CWordFormulaSet;  // 1999-08-30 MRP: wdf.h

class CLookupProc : public CInterlinearProc { // Lookup process, look up in database, return content of a field marker
#ifdef TEST_BUILD
    friend class CLookupProc_Test;
#endif
private:
    CDbTrie* m_pptri[ NUMTRIES ]; // See Trie.h for defines that give meanings to numbers
    BOOL m_bParseProc; // True if parsing
    // 1999-08-30 MRP: Change from word formula database to properties
    CDatabaseRefList m_drflstWordFormulas; // Word disambig rule file
    CShwDoc* m_pdocWordFormulas; // Document pointer, saved to know if closed
    CWordFormulaSet* m_pwdfset;  // 1999-08-30 MRP
    BOOL m_bSH2Parse; // True if SH2 style parse    
    BOOL m_bLongerOverride; // True if longest affix string overrides on parse
    BOOL m_bOverrideShowAll; // 1.3bg True if longest root overrides on show all parses, same as not show all
    BOOL m_bKeepCap; // True if keep capitalization
    BOOL m_bKeepPunc; // True if keep punctuation
    BOOL m_bStopAtSemicolon; // True if stop at first gloss separator
    Str8 m_sGlossSeparator; // typically ';'
    BOOL m_bInsertFail; // On fail, jump insert
    BOOL m_bShowFailMark; // On fail, show failure mark
    Str8 m_sFailMark; // Mark to use for failure
    BOOL m_bShowWord; // On fail, show word
    BOOL m_bShowRootGuess; // On parse fail, show root guess
    BOOL m_bApplyCC; // On lookup fail, apply CC table
#if UseCct
    ChangeTable m_cct; // Change table to use if lookup fails, or if this is a CC process
    Str8 m_sCCT; // File name of CC table
#endif
    BOOL m_bInfixBefore; // True if infixes placed before root
    BOOL m_bInfixMultiple; // True if multiple infixes allowed // 1.5.1mb 
	BOOL m_bPreferPrefix; // True if prefixes preferred over suffixes
	BOOL m_bPreferSuffix; // True if suffixes preferred over prefixes
	BOOL m_bNoCompRoot; // True if compound roots allowed
//    CDatabaseTypePtr m_ptypMyOwner; // used only to keep our database type from getting purged, thereby keeping tries in memory AB disabled 10-29 3.0.8r
private:
    BOOL bParseWord(  // Parse word, upper call to start recursive calls of bParse
            const char* psz,            // String to parse
            CFieldList* pfldlst,        // Field list for result of lookup and parse, may be empty, or have previous results to add to
            unsigned int& iSmallest, // Smallest piece found so far, used to show failed root on second pass to show failure
            BOOL bShowFail = 0,           // If TRUE, show failure when iSmallest is reached, defaults to FALSE
			BOOL bSpellCheck = FALSE ); // 1.4qzjm Add spell check arg to prevent parse timeout message on interlinear check
    BOOL bParse( const char* psz, CFieldList* pfldlst,
            int iSuffix, unsigned int& iSmallest,
            BOOL bShowFail = 0 ); // Parse morphemes of string, calls itself recursively on smaller pieces, bShowFail says it is a failure, and should show how far it got
    Str8 sPrefMorphChange( Str8& sOut ); // Return morph change part of prefix ptot, and cut it off sOut
    BOOL bNextSamePrefMatch( CTrieOut** pptot ); // Return true if same length with same root output and different affix output
    void AddPrefix( const char* pszOut, CField* pfld ); // Add affix to field
    Str8 sSuffMorphChange( Str8& sOut ); // Return morph change part of suffix ptot, and cut it off sOut
    BOOL bNextSameSuffMatch( CTrieOut** pptot ); // Return true if same length with same root output and different affix output
    void AddSuffix( const char* pszOut, CField* pfld ); // Add affix to field
    void AddRoot( const char* pszAff, CField* pfld ); // Add compound root to field
    BOOL bStartsWithSuffix( const char* psz ); // True if string starts with suffix (but not infix)
    BOOL bEndsWithPrefix( const char* psz ); // True if string ends with prefix (but not infix)
    int iGetLenMorph( const char* psz ); // Get the length of the actual morpheme, discounting the forced glosses
public:
    CLookupProc( CInterlinearProcList* pintprclstMyOwner,
            BOOL bParseProc = FALSE ); // Constructor

    ~CLookupProc(); // Destructor
    
    BOOL bLookupProc() { return !m_bParseProc; } // True if this is a lookup process that is not a parse process
    BOOL bParseProc() { return m_bParseProc; } // True if this is a parse process

    BOOL bSH2Parse() // True if SH2 style parse
        { return m_bSH2Parse; }     
    void SetSH2Parse( BOOL b ) // Set or clear SH2 style parse
        { m_bSH2Parse = b; }

    BOOL bKeepCap() // True if keep cap
        { return m_bKeepCap; }     
    void SetKeepCap( BOOL b ) // Set or clear keep cap
        { m_bKeepCap = b; }

    BOOL bKeepPunc() // True if keep punc
        { return m_bKeepPunc; }     
    void SetKeepPunc( BOOL b ) // Set or clear keep punc
        { m_bKeepPunc = b; }

    BOOL bStopAtSemicolon() // True if keep punc
        { return m_bStopAtSemicolon; }     
    void SetStopAtSemicolon( BOOL b ) // Set or clear keep punc
        { m_bStopAtSemicolon = b; }

    const char* pszGlossSeparator() const { return m_sGlossSeparator; } // typically ';'
    void SetGlossSeparator( const char* pszSep ) { m_sGlossSeparator = pszSep; }

    BOOL bShowFailMark()
        { return m_bShowFailMark; }     
    void SetShowFailMark( BOOL b )
        { m_bShowFailMark = b; }

    BOOL bInsertFail()
        { return m_bInsertFail; }     
    void SetInsertFail( BOOL b )
        { m_bInsertFail = b; }

    const char* pszFailMark() // fail mark
        { return m_sFailMark; }
    void SetFailMark( const char* psz ) // Set fail mark
        { m_sFailMark = psz; }  

    BOOL bShowWord()
        { return m_bShowWord; }     
    void SetShowWord( BOOL b )
        { m_bShowWord = b; }

    BOOL bShowRootGuess()
        { return m_bShowRootGuess; }     
    void SetShowRootGuess( BOOL b )
        { m_bShowRootGuess = b; }

    BOOL bInfixBefore() // True if infixes placed before root
            { return m_bInfixBefore; }     
    void SetInfixBefore( BOOL b ) // Set or clear
        { m_bInfixBefore = b; }

    BOOL bInfixMultiple() // True if multiple infixes allowed // 1.5.1mb 
            { return m_bInfixMultiple; }     
    void SetInfixMultiple( BOOL b ) // Set or clear
        { m_bInfixMultiple = b; }

    BOOL bLongerOverride() // True if longer affixes override
            { return m_bLongerOverride; }     
    void SetLongerOverride( BOOL b ) // Set or clear
        { m_bLongerOverride = b; }

    BOOL bOverrideShowAll() // True if longer root overrides
            { return m_bOverrideShowAll; }     
    void SetOverrideShowAll( BOOL b ) // Set or clear
        { m_bOverrideShowAll = b; }

    BOOL bPreferPrefix() // True if prefixes preferred
            { return m_bPreferPrefix; }     
    void SetPreferPrefix( BOOL b ) // Set or clear
        { m_bPreferPrefix = b; }

    BOOL bPreferSuffix() // True if suffixes preferred
            { return m_bPreferSuffix; }     
    void SetPreferSuffix( BOOL b ) // Set or clear
        { m_bPreferSuffix = b; }

    BOOL bCompRoot() // True if compound roots allowed
            { return !m_bNoCompRoot; }     
    void SetCompRoot( BOOL b ) // Set or clear
        { m_bNoCompRoot = !b; }

    BOOL bApplyCC() // True if applying CC to lookup failures
        { return m_bApplyCC; }     
    void SetApplyCC( BOOL b ) // Set or clear flag to apply CC to lookup failures
        { m_bApplyCC = b; }

#if UseCct
    BOOL bLoadCCFromFile( const char* pszChangeTablePath )
        { 
        m_sCCT = pszChangeTablePath;
        return m_cct.bLoadFromFile( m_sCCT ); // This must be passed a permanent pointer
        }
        
    BOOL bCCLoaded()
        { return m_cct.bLoaded(); } 
        
    const char* pszCCT() // Name of CC table file
        { return m_sCCT; }
#endif

    CDatabaseRefList* pdrflstWordFormulas() // Rule file database ref, for loading from settings
        { return &m_drflstWordFormulas; }                           
    void SetRuleFile( const Str8& sRuleFile ); // Set rule file name  
    const char* pszRuleFile(); // Name of rule file

    // 1999-08-30 MRP: Change from word formula database to properties
    BOOL bDisambigLookup( CRecPos rpsStartWd, CFieldList* pfldlstAnal );
    BOOL bDisambigParse( CRecPos rpsStartWd, CFieldList* pfldlstAnal,
            CFieldList* pfldlstFail, BOOL bShowAlways );
    void InsertFailMarks( CFieldList* pfldlst ) const;
    void DeleteFailMarks( CFieldList* pfldlst ) const;

    // 2000-09-21 MRP: Attempt to match an item in a word formula pattern
    // with any of the items of lookup data for a morpheme in the parse.
    BOOL bMatchParseLookupData(const char* pszPatternItem, int lenPatternItem,
            const char* pszParse, const char** ppszUnmatched) const;

    // 2000-09-21 MRP: Attempt to match an item in a word formula pattern
    // with an item of lookup data for a morpheme in the parse.
    BOOL bMatchParseLookupData(const char* pszPatternItem, int lenPatternItem,
            const char* pszDataItem) const;

    int nLookupDataGroupsInParse(const Str8& sParse) const;
    CWordFormulaSet* pwdfset() { return m_pwdfset; }
    void SetWordFormulas(const CWordFormulaSet* pwdfset);

	BOOL bInitWordFormulas(); // If first time, and word formulas not set up yet, do them now

	BOOL bInitStart( CRecPos &rpsStart, CField* pfldFrom ); // Init rpsStart

	Str8 sGetLeadPunc( CRecPos rpsStartCol, CRecPos &rpsStartWd ); // Set up leading punc of word

	Str8 CLookupProc::sGetCurWord( CRecPos rpsStartCol, CRecPos &rpsStartWd, CRecPos &rpsEndWd, int &iLenMatched ); // Get current word

	BOOL bLookup( const char* psz, CFieldList* pfldlst, int* piLenMatched ); // External interface for search

    BOOL bInterlinearize( CRecPos rpsStart, int* piLen, int iProcNum, int& iResult ); // Run interlinearization process   

    BOOL bSpellCheckField( CRecPos rpsCur, CRecPos rpsEnd ); // Run spell check process 
	unsigned int iInterlinearLookupCheck( CRecPos rpsTo ); // Check if current lookup answer is still valid
	Str8 AddLookupHyphens( CRecPos rpsFrom, CRecPos rpsTo, const char* pszOutput, int iLenMatched ); // Add hyphens to lookup output if needed
	Str8 RemovePunc( CRecPos rpsFrom, CRecPos rpsTo, const char* pszOutput ); // Remove punc if necessary // 1.6.4zg 
	Str8 Recapitalize( CRecPos rpsFrom, CRecPos rpsTo, const char* pszOutput ); // Recapitalize output of lookup if necessary // 1.6.4zg 
	int ReplaceInInterlinear( CRecPos rpsFrom, int iLenMatched, CRecPos rpsTo, const char* pszReplace ); // Replace a given word in interlinear with another word
    
    CDbTrie* ptri( int i ) // Access for interface
        { return m_pptri[ i ]; }
    
    void IndexUpdated( CIndexUpdate iup ); // Update all references to record in all tries

    void OnDocumentClose( CShwDoc* pdoc ); // Check for references to document being closed 

    void MarkerUpdated( CMarkerUpdate& mup ); // update mrflsts

	void UpdatePaths() // Update paths if project moved
		{
		for ( int i = 0; i < NUMTRIES; i++ ) // For all tries that are in use
			m_pptri[ i ]->UpdatePaths(); // Update trie paths
#if UseCct
		Shw_pProject()->UpdatePath( m_sCCT ); // Update CC table path
#endif
		m_drflstWordFormulas.UpdatePaths(); // Update Word Formula Path
		}

#if UseCct
	void WritePaths( std::ofstream& ostr ) // Write paths in use
		{
		if ( m_sCCT.GetLength() > 0 )
			ostr << m_sCCT << "\n"; // Write any cc table in use
		}
#endif

	void ClearRefs(); // clear ref ptrs in tries in preparation for database type deletion

    void MakeRefs(); // turn marker refs into CMarkerPtrs
    
    void SetParseProc() // Set this as a parse process
        { m_bParseProc = TRUE; SetType( CProcNames::eParse ); } // 1.4ah Change to not use string resource id in interlinear
        
	void AddGlossesToField( CField *pfld, int iStartPos, CFieldList *pfldlstOut ); // Do lookup of each morpheme to add gloss and part of speech, recursive
	
	void AddGlossesToList( CFieldList *pfldlstAnal, int iStartPos ); // Do lookup of each morpheme to add gloss and part of speech

    static BOOL s_bReadProperties( Object_istream& obs,
        CInterlinearProcList* pintprclst, CInterlinearProc** ppintprc );
#ifdef typWritefstream // 1.6.4ac
    void WriteProperties( Object_ofstream& obs ); 
#else
    void WriteProperties( Object_ostream& obs ); 
#endif
    
    BOOL bModalView_Properties();
    }; // End class CLookupProc

class CRearrangeProc : public CInterlinearProc { // Rearrange process, rearrange morphemes for adaptation
private:
    BOOL m_bGenerateProc; // True if Generate
    CMarkerPtr m_pmkrPSFrom; // Part of speech marker in interlinear text
    CMarkerPtr m_pmkrPunc; // Punc marker in interlinear text
    CDatabaseRefList m_drflst; // Rule file
    CShwDoc* m_pdoc; // Document pointer, saved to know if closed
	CIndex* m_pind; // Owning index, saved to avoid looking up
	CRecLookEl* m_prel; // First record element, saved to avoid looking up
    CMarkerPtr m_pmkrRule; // Rule marker
    CMarkerPtr m_pmkrDef; // Definition marker
    Str8 m_sFailMark; // Mark to use for failure
	Str8 m_sWordBound; // Mark to use for word boundary
	BOOL m_bKeepMorphemeBreaks; // True if keep morpheme breaks in generate 3.0.9e
public:
    CRearrangeProc( CInterlinearProcList* pintprclstMyOwner,
		BOOL bGenerateProc = FALSE ); // Constructor

    ~CRearrangeProc(); // Destructor

    void SetPSMarker( CMarker* pmkrPSFrom ) // Set the part of speech marker
        { m_pmkrPSFrom = pmkrPSFrom; }
    CMarker* pmkrPSFrom() const // Interlinear PSFrom marker
        { return m_pmkrPSFrom; }
    void SetPuncMarker( CMarker* pmkrPunc ) // Set the punc marker
        { m_pmkrPunc = pmkrPunc; }
    CMarker* pmkrPunc() const // Interlinear punc marker
        { return m_pmkrPunc; }

    BOOL bKeepMorphemeBreaks() // True if keep morpheme breaks
        { return m_bKeepMorphemeBreaks; }     
    void SetKeepMorphemeBreaks( BOOL b ) // Set or clear keep morpheme breaks
        { m_bKeepMorphemeBreaks = b; }

    CDatabaseRefList* pdrflst() // Rule file database ref, for loading from settings
        { return &m_drflst; }                           
    void SetRuleFile( const Str8& sRuleFile ); // Set rule file name  
    const char* pszRuleFile(); // Name of rule file
    
    BOOL bInterlinearize( CRecPos rpsStart, int* piLen, int iProcNum, int& iResult ); // Run interlinearization process   

	void UpdatePaths() // Update paths if project moved
		{ m_drflst.UpdatePaths(); }

	void WritePaths( std::ofstream& ostr ) // Write paths in use
		{ m_drflst.WritePaths( ostr ); }

    void OnDocumentClose( CShwDoc* pdoc ); // Check for references to document being closed 

    void SetGenerateProc() // Set this as a Generate process
        { m_bGenerateProc = TRUE; SetType( CProcNames::eGenerate ); } // 1.4hfj Fix bug of Generate showing as Parse in proc list // 1.4ah Change to not use string resource id in interlinear

    static BOOL s_bReadProperties( Object_istream& obs,
        CInterlinearProcList* pintprclst, CInterlinearProc** ppintprc );
#ifdef typWritefstream // 1.6.4ac
    void WriteProperties( Object_ofstream& obs ); 
#else
    void WriteProperties( Object_ostream& obs ); 
#endif
    
    BOOL bModalView_Properties();

private:
	int iPhonMatch( const char* pszFind, const char* pszText ); // Match find word in text, if success, return length of match, FULL_MATCH if all the way to end , else return 0
	int iPhonDefMatch( const char* pszFind, const char* pszText, int* piMatchElNum ); // Match Phonological element or class
    int iWdPSMatch( const char* pszFind, const char* pszText, const char* pszPS, int iRecursLevel ); // Match Find pattern at text and PS position
    const char* pszDefFind( const char* pszFind, BOOL bPhon ); // See if this find is a definition
}; // End class CRearrangeProc

class CGivenProc : public CInterlinearProc { // Given process, leave alone given interlinear lines
private:
public:
    CGivenProc( CInterlinearProcList* pintprclstMyOwner ); // Constructor

    ~CGivenProc(); // Destructor

    BOOL bInterlinearize( CRecPos rpsStart, int* piLen, int iProcNum, int& iResult ); // Run interlinearization process   

    static BOOL s_bReadProperties( Object_istream& obs,
        CInterlinearProcList* pintprclst, CInterlinearProc** ppintprc );
#ifdef typWritefstream // 1.6.4ac
    void WriteProperties( Object_ofstream& obs ); 
#else
    void WriteProperties( Object_ostream& obs ); 
#endif
    
    BOOL bModalView_Properties();
}; // End class CGivenProc

class CInterlinearProcList : public CDblList { // intprclst List of interlinear processes to be performed
private:
    CDatabaseType* m_ptypMyOwner;
    char m_cForcedGlossStartChar;
    char m_cForcedGlossEndChar;
    char m_cParseMorphBoundChar;
    Str8 m_sMorphBreakChars;  
public:
    CInterlinearProcList()
        {
        SetForcedGlossStartChar( '{' ); // Set default values of chars
        SetForcedGlossEndChar( '}' );
        SetParseMorphBoundChar( '+' );
        SetMorphBreakChars( "-" );
        }
    ~CInterlinearProcList();
    
    void SetForcedGlossStartChar( const char c )
        { m_cForcedGlossStartChar = c; }
    void SetForcedGlossEndChar( const char c )
        { m_cForcedGlossEndChar = c; }
    void SetParseMorphBoundChar( const char c )
        { m_cParseMorphBoundChar = c; }
    void SetMorphBreakChars( const char* psz )
        {
		if ( *psz )
			m_sMorphBreakChars = psz;
		else
			m_sMorphBreakChars = " ";
        }
    BOOL bAdapt() // True if there are adapt processes
        {
        for ( CInterlinearProc* pintprc = pintprcFirst(); pintprc; pintprc = pintprcNext( pintprc ) )
            if ( pintprc->bAdapt() )
                return TRUE;
        return FALSE;
        }
    BOOL bNonAdaptInterlin() // True if there are non-adapt interlinear processes
        {
        for ( CInterlinearProc* pintprc = pintprcFirst(); pintprc; pintprc = pintprcNext( pintprc ) )
            if ( !( pintprc->bAdapt() || pintprc->iType() == CProcNames::eGiven ) ) // 1.4ka On interlin, do adapt if only adapt and given procs
                return TRUE;
        return FALSE;
        }

	BOOL bWordParse() // True if it is a WordParse setup // 1.6.4s 
        {
        for ( CInterlinearProc* pintprc = pintprcFirst(); pintprc; pintprc = pintprcNext( pintprc ) ) // 1.6.4s 
			{
            if ( pintprc->bParseProc() ) // If parse, it is not WordParse 1.6.4s 
                return FALSE; // 1.6.4s 
			}
        return TRUE; // 1.6.4s 
        }

    void SetOwner( CDatabaseType* ptyp ) // Use in constructor only
        { m_ptypMyOwner = ptyp; }
    CDatabaseType* ptyp()
        { return m_ptypMyOwner; }
        
#ifdef typWritefstream // 1.6.4ac
    void WriteProperties(Object_ofstream& obs) const;
#else
    void WriteProperties(Object_ostream& obs) const;
#endif
    BOOL bReadProperties(Object_istream& obs);

    CInterlinearProc* pintprcFirst() const  { return (CInterlinearProc*)pelFirst(); }
    CInterlinearProc* pintprcNext(const CInterlinearProc* pintprc) const {return (CInterlinearProc*)pelNext(pintprc); }
    CInterlinearProc* pintprcPrev(const CInterlinearProc* pintprc) const {return (CInterlinearProc*)pelPrev(pintprc); }
    CInterlinearProc* pintprcLast() const   { return (CInterlinearProc*)pelLast(); }    
    
    char cForcedGlossStartChar() const
        { return m_cForcedGlossStartChar; }
    char cForcedGlossEndChar() const
        { return m_cForcedGlossEndChar; }
    char cParseMorphBoundChar() const
        { return m_cParseMorphBoundChar; }
    Str8 sMorphBreakChars() const
        { return m_sMorphBreakChars; }

    void Add(CInterlinearProc* pintprc)
        {CDblList::Add((CDblListEl*)pintprc); }
    
    void Delete(CInterlinearProc** ppintprc)    
        {CDblList::Delete((CDblListEl**)ppintprc); }

    void ClearRefs() // clear marker and dbtyp ref pointers in preparation for deleting a database type
        { 
        for ( CInterlinearProc* pintprc = pintprcFirst(); pintprc; pintprc = pintprcNext( pintprc ) )
            pintprc->ClearRefs();
        }
    void IndexUpdated( CIndexUpdate iup ); // Update all references to record in all tries

    BOOL bInterlinearize( CRecPos& rpsCur, BOOL bAdapt, BOOL bWord ); // Run interlinearization processes    

    void OnDocumentClose( CShwDoc* pdoc ); // Check for references to document being closed

    void MarkerUpdated( CMarkerUpdate& mup ); // update mrflsts

	void UpdatePaths(); // Update paths if project moved
	void WritePaths( std::ofstream& ostr ); // Update paths if project moved

    BOOL bModalView_Elements(); // Interface
}; // End class CInterlinearProcList
#endif // InterLin_H
