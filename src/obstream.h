// obstream.h  Interface for textual iostreams of objects (1995-07-22)

#ifndef OBSTREAM_H
#define OBSTREAM_H

#undef Object_ostream // 1.6.1cb 
#undef Object_istream // 1.6.1cm 

//class istream;  // iostream.h
//class ostream;  // iostream.h
#ifdef OBSTREAM_NOTES  // 1.6.4aa Not defined
class CNoteList;  // not.h
#endif  // 1.6.4aa 

/* Documentation and comments by Alan B 1-18-96
These routines are used for persistence, for reading and writing settings files.
They are designed to make it easy to load and save the variables of an object.
An object provides a WriteProperties function to write its member variables and 
bReadProperties routine to read its member variables.
See bottom of file for samples.
*/
// **************************************************************************
class Object_ofstream // Hungarian: obs // 1.6.1bg File object ostream
{
private:
	FILE* m_pf; // FILE to which output is written in new approach // 1.6.1bg 
public:
    Object_ofstream( FILE* pf ) { m_pf = pf; } // Constructor for FILE // 1.6.1bg 
    ~Object_ofstream() { if( m_pf ) fclose( m_pf ); } // Destructor, close FILE if it exists // 1.6.1bg 

	void WriteUTF8BOM(); // 1.4qnh Write UTF8 bom Byte Order Mark
    void WriteBeginMarker(const char* pszMarker, const char* pszName = ""); // Write a begin marker with an optional name
    void WriteEndMarker(const char* pszMarker); // Write an end marker
    void WriteBeginMarkerWithQualifier(const char* pszMarker, // Write a begin marker with qualifier
                                        const char* pszQualifier, const char* pszName = "");
    void WriteEndMarkerWithQualifier(const char* pszMarker, const char* pszQualifier); // Write an end marker with qualifier
    
    void WriteString(const char* pszMarker, const char* psz); // Write string
    void WriteProjectPath(const char* marker, const Str8& path);
    void WriteString(const char* pszMarker, const char* pszQualifier, const char* psz); // Write marker with qualifier appended and string
    void WriteObligatoryString(const char* pszMarker, const Str8& s); // Write marker and string, even if empty.
    void WriteObligatoryString(const char* pszMarker, const char* pszQualifier, const char* psz); // Write marker with qualifier appended and string, even if empty.
    void WriteChar(const char* pszMarker, char c); // Write marker and char
    void WriteBool(const char* pszMarker, BOOL b); // Write marker if boolean is true
    void WriteCOLORREF(const char* pszMarker, COLORREF rgb); // Write a set of three rgb color numbers
    void WriteInteger(const char* pszMarker, int i); // Write an integer
	void WriteUnrecognizedSettingsInfo( const char* psz ); // 1.0cm Write unrecognized settings info
    
    void WriteNewline(); // Write a blank line, used to separate sections of the file for readability

    void WriteWindowPlacement(const char* pszMarker, const CWnd* pwnd);

private:
    void WriteContents(const char* pszContents);
        // Write the field's contents NOT including a newline.
        // "Protect" backslashes at the beginning of lines
        // by writing a space before them.
}; // class Object_ofstream


// ==========================================================================

class Object_ifstream // Hungarian: obs // 1.6.1bg File object ostream
{
private:
	FILE* m_pf; // FILE to which output is written in new approach // 1.6.1bg 
    CNoteList& m_notlst;  // notes about problems reading objects // 1.6.1bg  // 1.6.1cm 
    
    char* m_pszMStringBuf; // input buffer for a marked string
    char* m_pszEnd; // end of string (i.e. its null) where next line is read
    BOOL m_bUnRead; // buffer contains a marked string which has been "unread"
    char* m_pszMarker; // beginning of unread marker
    char* m_pszString; // beginning of unread string
    char m_chEndOfLine;  // delimiter for the get-one-line function
    BOOL m_bStringTooLong;  // 07-26-1997 - TRUE if size of String bigger than 32001 Bytes
    
    BOOL bReadMarker(char cFirstChar, const char* pszMarker); // low level read begin or end marker

public:
    Object_ifstream( FILE* pf, CNoteList& notlst); // 1.6.1cm  // 1.6.1da 
    ~Object_ifstream(); // Destructor

    BOOL bAtEnd(); // True if at eof
    BOOL bAtBackslash(); // True if at a backslash
    BOOL bAtBeginMarker(); // True if at any begin marker
    /* 1999-11-29 TLB: No longer needed
    BOOL bAtBeginOrEndMarker(); // True if at any begin or end marker
    */
    BOOL bAtBeginMarker(const char* pszMarker, const char* pszName = NULL ); // True if at the desired begin marker followed by the requested name
    BOOL bAtBeginMarker(const char* pszMarker, Str8& sName); // True if at the desired begin marker followed by the requested name
    BOOL bAtEndMarker(const char* pszMarker); // True if at the desired end marker

    BOOL bReadBeginMarker(const char* pszMarker, Str8& sName); // Read the desired begin marker followed by the requested name
    BOOL bReadBeginMarker(const char* pszMarker) // Read the desired begin marker (pszMarker with a plus sign)
        { return bReadMarker('+', pszMarker); }
    BOOL bReadEndMarker(const char* pszMarker) // Read the desired end marker (pszMarker with a minus sign)
        { return bReadMarker('-', pszMarker); }
    BOOL bReadBeginMarkerWithQualifier(const char* pszMarker, const char* pszQualifier); // Read the desired begin marker with a qualifier
    BOOL bReadEndMarkerWithQualifier(const char* pszMarker, const char* pszQualifier); // Read the desired end marker with a qualifier

    BOOL bReadString(const char* pszMarker, const char* pszQualifier, Str8& s); // Read a string with both a marker and a qualifier before it
    BOOL bReadString(const char* pszMarker, Str8& s); // Read a string
    BOOL bReadBool(const char* pszMarker, BOOL& b); // Read a boolean. The presence of the marker means TRUE
    BOOL bReadCOLORREF(const char* pszMarker, COLORREF& rgb); // Read a set of three rgb color numbers
    BOOL bReadInteger(const char* pszMarker, int& i); // Read an integer
    BOOL bReadDecimal(const char* pszMarker, double& d);
    
	BOOL bUnrecognizedSettingsInfo( const char* pszMarker, Str8& s ); // 1.0cm Store any unrecognized settings info so as not to lose it
    BOOL bEnd( const char* pszMarker ); // Read given end marker, stop any begin marker, for error recovery on objects with unrecognized names AB 1-18-95
    BOOL bEndWithQualifier( const char* pszMarker, const char* pszQualifier ); // same as bEnd() but with additional qualifier
    
    BOOL bReadNewObjectProperties(const char* pszMarker, Str8& sProperties);
        // 1997-12-08 MRP: Experiment with reading properties written
        // by a newer version of Shoebox. This function assumes
        // that we know the object's tag, but not its internal structure.

    BOOL bStringTooLong()   //07-26-1997: Returns the Value m_bStringTooLong
        { return m_bStringTooLong; }  
    
    BOOL bReadMainWindowPlacement(const char* pszMarker, CWnd* pwnd); // If the window placement is present, apply it to the window.
    BOOL bReadWindowPlacement(const char* pszMarker, CWnd* pwnd, BOOL& eShowCmd); // Open the window to normal size, but return the actual window state.
    BOOL bReadWindowPlacement(const char* pszMarker, WINDOWPLACEMENT& wpl);

    CNoteList& notlst() { return m_notlst; }

protected:  
    void ReadMarkedString(const char** ppszMarker, const char** ppszString,
            BOOL bTrimWhiteSpace = TRUE); // Read any marked string
    void UnReadMarkedString(); // Store most recently read marked string for next read
    
private:
    void ReadMarkedLine(const char** ppszMarker, const char** ppszString); // Read any marked line
    void ReadLine(); // Read one line
    void AppendFieldTo(Str8& sProperties);  // 1997-12-08 MRP
}; // class Object_ifstream


// **************************************************************************
class Object_ostream // Hungarian: obs
{
private:
    std::ostream& m_ios;  // iostream to which output is written
public:
    Object_ostream(std::ostream& ios) : m_ios(ios) {} // Constructor for ostream
    ~Object_ostream() {} // Destructor

	void WriteUTF8BOM(); // 1.4qnh Write UTF8 bom Byte Order Mark
    void WriteBeginMarker(const char* pszMarker, const char* pszName = ""); // Write a begin marker with an optional name
    void WriteEndMarker(const char* pszMarker); // Write an end marker
    void WriteBeginMarkerWithQualifier(const char* pszMarker, // Write a begin marker with qualifier
                                        const char* pszQualifier, const char* pszName = "");
    void WriteEndMarkerWithQualifier(const char* pszMarker, const char* pszQualifier); // Write an end marker with qualifier
    
    void WriteString(const char* pszMarker, const char* psz); // Write string
    void WriteProjectPath(const char* marker, const Str8& path);
    void WriteString(const char* pszMarker, const char* pszQualifier, const char* psz); // Write marker with qualifier appended and string
    void WriteObligatoryString(const char* pszMarker, const Str8& s); // Write marker and string, even if empty.
    void WriteObligatoryString(const char* pszMarker, const char* pszQualifier, const char* psz); // Write marker with qualifier appended and string, even if empty.
    void WriteChar(const char* pszMarker, char c); // Write marker and char
    void WriteBool(const char* pszMarker, BOOL b); // Write marker if boolean is true
    void WriteCOLORREF(const char* pszMarker, COLORREF rgb); // Write a set of three rgb color numbers
    void WriteInteger(const char* pszMarker, int i); // Write an integer
	void WriteUnrecognizedSettingsInfo( const char* psz ); // 1.0cm Write unrecognized settings info
    
    void WriteNewline(); // Write a blank line, used to separate sections of the file for readability

    void WriteWindowPlacement(const char* pszMarker, const CWnd* pwnd);

private:
    void WriteContents(const char* pszContents);
        // Write the field's contents NOT including a newline.
        // "Protect" backslashes at the beginning of lines
        // by writing a space before them.
}; // class Object_ostream


// ==========================================================================

class Object_istream // Hungarian: obs
{
private:
    BOOL m_bProjectPaths = FALSE;
    BOOL m_bRelocateLegacy = FALSE;
    BOOL m_bLocateMissingReferences = TRUE;
    std::istream& m_ios;  // iostream from which input is read
    CNoteList& m_notlst;  // notes about problems reading objects
    
    char* m_pszMStringBuf; // input buffer for a marked string
    char* m_pszEnd; // end of string (i.e. its null) where next line is read
    BOOL m_bUnRead; // buffer contains a marked string which has been "unread"
    char* m_pszMarker; // beginning of unread marker
    char* m_pszString; // beginning of unread string
    char m_chEndOfLine;  // delimiter for the get-one-line function
    BOOL m_bStringTooLong;  // 07-26-1997 - TRUE if size of String bigger than 32001 Bytes
    
    BOOL bReadMarker(char cFirstChar, const char* pszMarker); // low level read begin or end marker

public:
#ifdef OBSTREAM_NOTES
    Object_istream(std::istream& ios, CNoteList& notlst);
#else
    Object_istream(std::istream& ios);
#endif
    ~Object_istream(); // Destructor

    void SetProjectPathContext(BOOL bRelocateLegacy)
        { m_bProjectPaths = TRUE; m_bRelocateLegacy = bRelocateLegacy; }
    BOOL bProjectPaths() const { return m_bProjectPaths; }
    BOOL bRelocateLegacyPaths() const { return m_bRelocateLegacy; }
    void SetLocateMissingReferences(BOOL locate) { m_bLocateMissingReferences = locate; }
    BOOL bLocateMissingReferences() const { return m_bLocateMissingReferences; }

    BOOL bAtEnd(); // True if at eof
    BOOL bAtBackslash(); // True if at a backslash
    BOOL bAtBeginMarker(); // True if at any begin marker
    /* 1999-11-29 TLB: No longer needed
    BOOL bAtBeginOrEndMarker(); // True if at any begin or end marker
    */
    BOOL bAtBeginMarker(const char* pszMarker, const char* pszName = NULL ); // True if at the desired begin marker followed by the requested name
    BOOL bAtBeginMarker(const char* pszMarker, Str8& sName); // True if at the desired begin marker followed by the requested name
    BOOL bAtEndMarker(const char* pszMarker); // True if at the desired end marker

    BOOL bReadBeginMarker(const char* pszMarker, Str8& sName); // Read the desired begin marker followed by the requested name
    BOOL bReadBeginMarker(const char* pszMarker) // Read the desired begin marker (pszMarker with a plus sign)
        { return bReadMarker('+', pszMarker); }
    BOOL bReadEndMarker(const char* pszMarker) // Read the desired end marker (pszMarker with a minus sign)
        { return bReadMarker('-', pszMarker); }
    BOOL bReadBeginMarkerWithQualifier(const char* pszMarker, const char* pszQualifier); // Read the desired begin marker with a qualifier
    BOOL bReadEndMarkerWithQualifier(const char* pszMarker, const char* pszQualifier); // Read the desired end marker with a qualifier

    BOOL bReadString(const char* pszMarker, const char* pszQualifier, Str8& s); // Read a string with both a marker and a qualifier before it
    BOOL bReadString(const char* pszMarker, Str8& s); // Read a string
    BOOL bReadProjectPath(const char* marker, Str8& path);
    BOOL bReadBool(const char* pszMarker, BOOL& b); // Read a boolean. The presence of the marker means TRUE
    BOOL bReadCOLORREF(const char* pszMarker, COLORREF& rgb); // Read a set of three rgb color numbers
    BOOL bReadInteger(const char* pszMarker, int& i); // Read an integer
    BOOL bReadDecimal(const char* pszMarker, double& d);
    
	BOOL bUnrecognizedSettingsInfo( const char* pszMarker, Str8& s ); // 1.0cm Store any unrecognized settings info so as not to lose it
    BOOL bEnd( const char* pszMarker ); // Read given end marker, stop any begin marker, for error recovery on objects with unrecognized names AB 1-18-95
    BOOL bEndWithQualifier( const char* pszMarker, const char* pszQualifier ); // same as bEnd() but with additional qualifier
    
    BOOL bReadNewObjectProperties(const char* pszMarker, Str8& sProperties);
        // 1997-12-08 MRP: Experiment with reading properties written
        // by a newer version of Shoebox. This function assumes
        // that we know the object's tag, but not its internal structure.

    BOOL bStringTooLong()   //07-26-1997: Returns the Value m_bStringTooLong
        { return m_bStringTooLong; }  
    
    BOOL bReadMainWindowPlacement(const char* pszMarker, CWnd* pwnd); // If the window placement is present, apply it to the window.
    BOOL bReadWindowPlacement(const char* pszMarker, CWnd* pwnd, BOOL& eShowCmd); // Open the window to normal size, but return the actual window state.
    BOOL bReadWindowPlacement(const char* pszMarker, WINDOWPLACEMENT& wpl);

#ifdef OBSTREAM_NOTES
    CNoteList& notlst() { return m_notlst; }
#endif

protected:  
    void ReadMarkedString(const char** ppszMarker, const char** ppszString,
            BOOL bTrimWhiteSpace = TRUE); // Read any marked string
    void UnReadMarkedString(); // Store most recently read marked string for next read
    
private:
    void ReadMarkedLine(const char** ppszMarker, const char** ppszString); // Read any marked line
    void ReadLine(); // Read one line
    void AppendFieldTo(Str8& sProperties);  // 1997-12-08 MRP
}; // class Object_istream

// **************************************************************************
/* The boolean read routines such as bReadString each check for the requested marker
(plus a qualifier after the marker in some cases).
If the desired marker matches the current marker in the input stream, they read
the requested type of information.
In other words, if the next marked string in the input stream has marker pszMarker
they return via the attribute argument the string contents.
See CMarker and CMarkerSet in mkr.h for more detailed examples.
*/
// Examples by Alan B 1-18-96
#ifdef Samples // Ifdef instead of comment so syntax coloring will show
//-----------------------------------------------------------------------------------
class CSample // Simple class with no embedded classes Hungarian sam
{
    int m_iSample;
    BOOL m_bSample;
public:
    static const char* psz_Sample = "Sample"; // Standard way of generating strings used for markers, read and write routines add the backslash
    static const char* psz_iSample = "iSample"; // Defining strings assures consistency of read and write
    static const char* psz_bSample = "bSample";

    void WriteProperties( Object_ostream& obs ) // Write the properties of a simple class
        {
        obs.WriteNewline(); // Write a blank line separator for readability of the file
        obs.WriteBeginMarker( psz_Sample ); // Write the begin marker for the class: \+Sample
        
        obs.WriteInteger( psz_iSample, m_iSample ); // Write integer value \iSample 0
        obs.WriteBool( psz_bSample, m_bSample ); // Write marker if BOOL is TRUE \bSample
        
        obs.WriteEndMarker( psz_Sample ); // Write the end marker for the class: \-Sample
        }
        
    static BOOL s_bReadProperties( Object_istream& obs, CSample **ppsam ) // Try to read the properties of a simple class
        {
        int iSample = -1; // Assume for this example that negative is not a valid value
        BOOL bSample = FALSE; // Must initialize a boolean to false
        
        if ( !obs.bReadBeginMarker( psz_Sample ) ) // If not the begin marker for this class, return false (it is some other class)
            return FALSE;
            
        while ( !obs.bAtEnd() ) // While more in file
            {
            if ( obs.bReadInteger( psz_iSample, iSample ) ) // If \iSample found, read the integer
                ;
            else if ( obs.bReadBool( psz_bSample, bSample ) ) // If \bSample found, set the boolean
                ;
            else if ( obs.bEnd( psz_Sample ) ) // If end marker or erroneous begin break, this is the standard ending of read loop
                break;
            }
        if ( iSample < 0 && !*ppsam ) // If valid values came in, and no previous value for this object, construct a new object
            *ppsam = new CSample( iSample, bSample );
        return TRUE; // Return TRUE to say we ate something, caller must initilize the argument and  verify that object returned is not null    
        }   
}
//-----------------------------------------------------------------------------------
class CSampleEl : CDblListEl // Class with pointers to other classes, element of collection Hungarian sel
{
    CSample* m_psam;
public:
    static const char* psz_SampleEl = "SampleEl"; // Standard way of generating strings used for markers, read and write routines add the backslash

    void WriteProperties( Object_ostream& obs ) // Write the properties of a class
        {
        obs.WriteNewline(); // Write a blank line separator for readability of the file
        obs.WriteBeginMarker( psz_SampleColl ); // Write the begin marker for the class: \+SampleEl
        
        m_psam->WriteProperties( obs ); // Write the embedded class
        
        obs.WriteEndMarker( psz_Sample ); // Write the end marker for the class: \-SampleEl
        }
            
    static BOOL s_bReadProperties( Object_istream& obs, CSampleEl** ppsel ) // Try to read the properties of a class
        {
        if ( !obs.bReadBeginMarker( psz_SampleEl ) ) // If not this class, return false to indicate nothing done
            return FALSE;
            
        CSample* psam = NULL; // Initialize element object
        while ( !obs.bAtEnd() ) // While more in file
            {
            if ( CSample::s_bReadProperties( obs, &psam ) ) // If sample object marker found, build one (may be null if not valid)
                ;
            else if ( obs.bEnd( psz_SampleColl ) ) // If end marker or erroneous begin, standard ending of read loop
                break;
            }
        if ( psam && !*ppsel ) // If valid values came in, and no previous value for this object, construct a new object
            *ppsel = new CSampleEl( psam );
        return TRUE; // Return TRUE to say we ate something, caller must initilize the argument and  verify that object returned is not null    
        }
}
//-----------------------------------------------------------------------------------
class CSampleColl : public CDblList // Collection class Hungarian scl
{
public:
    static const char* psz_SampleColl = "SampleColl"; // Standard way of generating strings used for markers, read and write routines add the backslash

    void WriteProperties( Object_ostream& obs ) // Write the properties of a collection class
        {
        obs.WriteNewline(); // Write a blank line separator for readability of the file
        obs.WriteBeginMarker( psz_SampleColl ); // Write the begin marker for the class: \+SampleColl
        
        for ( const SampleEl* psel = pselFirst(); psel; psel = pselNext(psel) ) // For each member of the collection, have it write its properties
            psel->WriteProperties(obs);
        
        obs.WriteEndMarker( psz_Sample ); // Write the end marker for the class: \-SampleColl
        }
            
    BOOL bReadProperties( Object_istream& obs ) // Try to read the properties of a collection class
        {
        if ( !obs.bReadBeginMarker( psz_SampleColl ) ) // If not this collection, return false to indicate nothing done
            return FALSE;
        while ( !obs.bAtEnd() ) // While more in file
            {
            CSampleEl* psel = NULL; // Initialize element object
            if ( CSampleEl::s_bReadProperties( obs, &psel ) ) // If element object marker found, build one (may be null if not valid)
                {
                if ( psel ) // If an object returned, add it to the collection
                    Add( psel );
                }
            else if ( obs.bEnd( psz_SampleColl ) ) // If end marker or erroneous begin, standard ending of read loop
                break;
            }
        return TRUE; // Return TRUE to say we ate something, caller must initilize the argument and  verify that object returned is not null    
        }
}
//-----------------------------------------------------------------------------------
// Sample calls of collection
CSampleColl* pscl;
pscl->WriteProperties( obs ); // Write the collection

if ( pscl->bReadProperties( obs ) ) // Read the collection
    ;
#endif // Samples

#ifdef FileStream // 1.6.1cb  // 1.6.4aa Never define FileStream
#define Object_ostream Object_ofstream // 1.6.1cb  // 1.6.4aa Never do the full redefine of stream
#define Object_istream Object_ifstream // 1.6.1cm  // 1.6.4aa Never do the full redefine of stream
#endif // 1.6.1cb // 1.6.4aa Never define FileStream

#endif // OBSTREAM_H
