// trie.h CTrie classes Alan Buseman 11 Dec 95

#ifndef Trie_H
#define Trie_H

//#include "cdbllist.h" // For derivation
//#include "cdblel.h" // For derivation
#include "crecord.h" // For references to records and fields
#include "crecpos.h" // For recpos
#include "dbref.h" // For CDatabaseRefList
#include "ptr.h" // For CDatabaseTypeRef

class Object_istream;  // obstream.h
class Object_ostream;  // obstream.h

/* A letter trie is used for high speed search for morphemes or words. The trie typically
contains a large number of morphemes, and is used to see if one or more of the
morphemes can be found in a word beginning at a particular letter in the word.
For more information see the book Computational Morphology by Richard Sproat.

A letter trie structure has an array of letters and a matching array of pointers.
It is used in searching by doing a strchr search for a letter in the array of letters.
If the letter is found, the corresponding pointer is followed to the next trie, which
is used to search for the next letter.

The AMPLE and STAMP programs also use a trie. This trie differs from theirs in two main
ways. 1) The AMPLE trie uses a linked list of pointers, but this one uses an array of pointers.
It is much faster to offset into an array than to walk a linked list. 2) The AMPLE trie is of fixed
depth, but this one is of variable depth. The AMPLE trie code knows that after a particular
number of trie searches the next pointer is to a linked list. This trie code can tell for each
trie it searches if it points to further tries or to a linked list. The AMPLE trie depth can be
set with a command line option, but once set it cannot vary. This trie can have variable
depths on different paths. This trie is also self extending as new morphemes are added.
When it detects that a linked list is longer than a certain threshold, it adds another layer
of trie to break up the list.

This trie does the transition to a linked list by polymorphism. Each pointer points to
a CTrieBase, which is a virtual class that can be either a CTrie or a CTrieList.

*/

class CTrieOut : public CRecPos // tot Output of a trie lookup, list of matches
{
private:
    int m_iLen; // Length of match
    CTrieOut* m_ptotNext; // Next result in list
public:
    CTrieOut( CRecPos rps, int iLen ); // Constructor
    ~CTrieOut(); // Destructor, deletes all the rest of the list
    int iLen() // Length of match
        { return m_iLen; }
    CTrieOut* ptotNext() // Next in list of results
        { return m_ptotNext; }
    void SetNext( CTrieOut* ptot ) // Set next in list      
        { m_ptotNext = ptot; }
    CTrieOut* ptotAdd( CRecPos rps, int iLen ); // Add result to list, longest first
    CTrieOut* ptotDelete(); // Delete element, return next
    void DeleteRest(); // Delete rest of list
    static void CTrieOut::s_DeleteAll( CTrieOut* ptot ); // delete list starting at ptot

#ifdef _DEBUG
    virtual void AssertValid() const;
#endif
};

class CTrieBase // tbs Abstract base class for TrieList and Trie for polymorphism
{
public:
    virtual ~CTrieBase() {};
    virtual int iListLen() = 0;                         // Length of list
    virtual void Add( const char* pszRest, CRecPos rps, int iType = 0 ) = 0;
    virtual void DeleteRec( CRecord* prec ) = 0; // Delete all references to record
    virtual BOOL bIsEmpty() = 0; // True if trie or trie list is empty
    virtual CTrieOut* ptotLookup( const char* pszRest, CTrieOut* ptot = NULL, int iLenMatched = 0 ) const = 0; // Needs to return a list of matches
#ifdef _DEBUG
    virtual void AssertValid() const = 0;
#endif      
};

class CTrieListEl // tle TrieList element is a single piece of a trie list
{
private:
    char* m_pszRest;                // Rest of word.
    CRecPos m_rps;              // Info for word that ends this way.
    CTrieListEl* m_ptleNext;                // Next element of list.
private:
    friend class CTrieList;
    friend class CTrie;
    friend class CDbTrie; // CDbTrie uses one of these for redup list
    CTrieListEl( const char* pszRest, CRecPos rps ); // Constructor
    ~CTrieListEl();                             // Destructor
    char* pszRest() // Rest of word
        { return m_pszRest; }
    CRecPos rps() // Record info
        { return m_rps; }
    CTrieListEl* ptleNext() // Next in list of results
        { return m_ptleNext; }
    void SetNext( CTrieListEl* ptle ); // Set next in list      
#ifdef _DEBUG
public:    
    void AssertValid() const;   // Assert valid routine
#endif      
};

class CTrieList : public CTrieBase // tls TrieList is the lower end of every trie
{
private:
    CTrieListEl* m_ptle;                // First element of list.
    int m_iListLen;                     // Length of list
private:
    friend class CTrie; // CTrie uses these for the bottoms of tries
    friend class CDbTrie; // CDbTrie uses one of these for redup list
    CTrieList(); // Constructor
    CTrieList( const char* pszRest, CRecPos rps ); // Constructor
    ~CTrieList();                           // Destructor
    CTrieListEl* ptle()         // First list element
        { return m_ptle; }
    int iListLen()                          // Length of list
        { return m_iListLen; }
    void Add( const char* pszRest, CRecPos rps, int iType = 0 ); // Add a new info to a list
    CTrieListEl* ptleDelete( CTrieListEl* ptle ); // Delete an element, return next
    void DeleteRec( CRecord* prec ); // Delete all references to record
    BOOL bIsEmpty() // True if trie or trie list is empty
        { return !m_ptle; }
    CTrieOut* ptotLookup( const char* pszRest, CTrieOut* ptot = NULL, int iLenMatched = 0 ) const; // Find a string in a list
#ifdef _DEBUG
public:    
    void AssertValid() const;   // Assert valid routine
#endif      
};

class CTrie : public CTrieBase // tri Trie is a high speed lookup engine for parsing
{
private:
    char* m_pszLetters;         // Letters at this level.
    CTrieBase** m_pptbsSubtries;        // Pointers to subtries or lists, one for each letter.
    CTrieList* m_ptls;              // Info for word that terminates at this level.
private:
    friend class CDbTrie; // CLTrie is the only place these can be called
    CTrie()                     // Constructor
         { Init(); }
    ~CTrie()                        // Destructor
        { DeleteAll(); }
    void Unload()                   // Unload and reinitialize trie
        { DeleteAll(); Init(); }
    void Init();                            // Initiailization helper for constructor and unload
    void DeleteAll();               // Delete all helper for unload
    
    int iListLen()       // Return list length of 0 for trie
        { return 0; };
    CTrieOut* ptotLookup( const char* pszRest, CTrieOut* ptot = NULL, int iLenMatched = 0 ) const; // Get info for a string from a trie
public:
    void Add( const char* pszRest, CRecPos rps, int iType = 0 );        // Add a new piece of information to a trie
    void DeleteRec( CRecord* prec ); // Delete all references to record
    BOOL bIsEmpty() // True if trie is empty
        { return !m_ptls && !*m_pszLetters; }
#ifdef _DEBUG
public:
    void AssertValid() const;
#endif      
};

class CInterlinearProc; // Forward reference to avoid circularity
class CDatabaseTypeSet; // For ptypset()

//-----------------------------------------------------------------------------
class CDbTrie : public CTrie // tri Database trie, public interface of trie
{
#ifdef TEST_BUILD
    friend class CDbTrie_Test;
#endif
private:
    CTrieList m_tlsRedup; // Redup list
    CInterlinearProc* m_pintprcMyOwner; // Owning process
    CDatabaseTypeRef m_trf; // Database type of documents in list, included here mostly to be sure type is not removed from memory when the last database of the type is closed. also used in interface to assure all db's are of same type
    CDatabaseRefList m_drflst; // List of documents, for update
    CMarkerRefList m_mrflst; // List of markers to look up in document
    Str8 m_sMkrOut; // String form of output marker
    CMarkerPtr m_pmkrOut; // Pointer to output marker
    int m_iType; // Type of trie, values below
        #define LOOK 0 // General lookup trie , parse database if parsing
        #define PREF 1 // Prefix trie
        #define ROOT 2 // Root trie
        #define SUFF 3 // Suffix trie, loads and searches in reverse
        #define INF 4 // Infix trie
        #define NUMTRIES 5 // Total number of tries in process
    BOOL m_bLoaded; // False if trie has not been loaded yet, used for automatic loading
public:
    CDbTrie( CInterlinearProc* pintprc, int iType ) // Constructor for new trie
        { m_pintprcMyOwner = pintprc;
        m_pmkrOut = NULL;
        m_iType = iType; 
        m_bLoaded = FALSE; }    
        
    ~CDbTrie() {} // Destructor

    void CDbTrie::ClearAll(); // Unload trie and clear all settings

    void CopySettings( CDbTrie* ptri ); // Copy settings from another trie, used for interface to set all affix tries from one dialog box     

    void Unload()                   // Unload and reinitialize trie
        { CTrie::Unload(); m_bLoaded = FALSE; }
        
    BOOL bLoaded() // True if loaded, needed to see if references need to be created
        { return m_bLoaded; } 
        
    void MakeRefs(); // Make marker pointers from names
private:
    BOOL bAddToFieldList( CTrieOut* ptot, CFieldList* pfldlst, int* piLenMatched ); // Add ptot list to field list, set iLenMatched to longest
public:
    BOOL bRedup( const char* pszWord, const char* pszPrevWord, 
            CFieldList* pfldlst, int* piLenMatched ); // If the root is reduped, add it to analyses

    BOOL bSearch( const char* pszRest,
            CFieldList* pfldlst, int* piLenMatched, const int iSettings = 0 ); // External interface for trie lookup
        #define WHOLE_WORD 1 // iSetting to do whole word match
        #define CASE_FOLD 2     // iSetting to do case fold
        #define LONGEST 4       // iSetting to keep only longest matches

    CTrieOut* ptotSearch( const char* pszRest, const int iSettings = 0 ); // External interface for trie lookup

    void IndexUpdated( CIndexUpdate iup ); // Keep trie in step with databases when an index is updated

    CDatabaseType* ptypDrfType() // Database type of databases in the drflst of trie, may be null if no databases
        { return m_trf.ptyp(); }

    void SetDrfType( const char* psz ) // Set the string of the database type
        { m_trf.SetType( psz); }
    
    Str8 sMkrOut() // String form of output marker
        { return m_sMkrOut; }
        
    CMarker* pmkrOut() // Get marker pointer from output marker string  
        { return m_pmkrOut; }
    
    void SetMkrOut( const char* psz ) // Set the output marker string, don't make ref yet because this is used on loading, MakeRefs makes the ref
        { m_sMkrOut = psz; m_pmkrOut = NULL; }

private:
    void AddField( CRecord* prec, CField* pfld, CField* pfldOut ); // Add a field to the trie       
public: 
    void AddRec( CRecord* prec ); // Add a record to the trie

    void AddAll(); // Add all records in all databases to trie

    CDatabaseRefList* pdrflst() // List of documents, for update
        { return &m_drflst; }
    CMarkerRefList* pmrflst() // List of markers to look up in document
        { return &m_mrflst; }
                            
    BOOL bIsEmpty()
        { return m_drflst.bIsEmpty(); }
    
    void OnDocumentClose( CShwDoc* pdoc ); // Check for references to document being closed 
    
    void MarkerUpdated( CMarkerUpdate& mup ); // update mrflsts in case of marker name change

	void UpdatePaths() { m_drflst.UpdatePaths(); } // Update paths if project moved
    
    void ClearRefs() // Clear mrflst and database type pointer, for clearing up before closing database types
        { m_pmkrOut = NULL, m_mrflst.ClearRefs(), m_trf.ClearPtr(); }

#ifdef typWritefstream // 1.6.4ac
    void WriteProperties( Object_ofstream& obs, const char* pszMarker ); // Write the properties of the class
#else
    void WriteProperties( Object_ostream& obs, const char* pszMarker ); // Write the properties of the class
#endif
    static BOOL s_bReadProperties( Object_istream& obs,
        CDbTrie* ptri, const char* pszMarker, int iType = FALSE );

    BOOL bModalView_Properties();
#ifdef _DEBUG
    void AssertValid() const;
#endif      
};
#endif // Trie_H
