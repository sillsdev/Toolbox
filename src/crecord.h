// crecord.h Interface for Standard format record (1995-02-28)

#ifndef CRECORD_H
#define CRECORD_H

#include "mkr.h"
#include "cdbllist.h"
#include "cfield.h"
class CNumberedRecElPtr;  // ind.h
class CCheckingState; // cck.h

class CRecPos; // Forward reference needed because crecpos.h includes crecord.h

//------------------------------------------------------
class CFieldList : public CDblList // fldlst List of fields
{
public:
    CFieldList() {}
    ~CFieldList() {}
    
    CField* pfldFirst() const   // first (key) field
                        { return (CField*) pelFirst(); }
    CField* pfldLast() const    // last field
                        { return (CField*) pelLast(); }
    
    CField* pfldNext( const CField* pfldCur ) const // field after current
        { return (CField*) pelNext( pfldCur ); }
    CField* pfldPrev( const CField* pfldCur ) const // field before current
        { return (CField*) pelPrev( pfldCur ); }
        
    CField* pfldFirstWithMarker(const CMarker* pmkr) const; // Find first field with a specified marker

        // Insert new after current 
    void InsertAfter( CField* pfldCur, CField* pfldNew )
                        { CDblList::InsertAfter( pfldCur, pfldNew); }
        // Insert new before current
    void InsertBefore( CField* pfldCur, CField* pfldNew )
                        { CDblList::InsertBefore( pfldCur, pfldNew); }

    void Add( CField* pfldNew ) // Insert new at end
                        { CDblList::Add( pfldNew ); }
    
        // Delete field, return next, NULL if no more
    CField* pfldDelete( CField* pfld )
        { return (CField*) pelDelete( pfld ); }

        // Delete element, pel is changed to Next if it exists, else Prev if it exists, else NULL
    void Delete( CField** ppfld ) { CDblList::Delete( (CDblListEl**) ppfld ); }

        // Delete element
    void Delete( CField* pfld );
        
    void DeleteRest( CField* pfld ) // Delete pfld and all after it
        { while ( pfld ) 
            pfld = pfldDelete( pfld ); }

    CField* pfldRemove( CField* pfld ) // Remove field from list without deleting it
        { return (CField*)CDblList::pelRemove( (CDblListEl*)pfld ); }

	void MoveFieldsFrom(CFieldList* pfldlst);  // 1999-09-13 MRP
		// Remove fields from pfldlst and add them to the end of this list

    void EliminateShorter(); // Eliminate shorter analyses for disambiguation

    void EliminateDuplicates( BOOL bTimeLimit = FALSE ); // Eliminate duplicates for disambiguation // 1.3bm Allow time limit
    
#ifdef _DEBUG
    void AssertValid() const {}
#endif  // _DEBUG
};

//------------------------------------------------------
class CRecord : public CFieldList // rec Record, list of fields with a required key
{
public:
    CRecord(CField* pfldKey);
    ~CRecord() {}
    
    const Str8& sKey() const; // Content of key field

    CField* pfldFirstInSubRecord(const CMarker* pmkr,
            CField* pfldPriKey = NULL) const;
        // Return the first occurrence of a field with pmkr
        // in relation to the pfldPriKey (a primary sort key field).
        // according to the outline hierarchy of markers.
        // Returns NULL if the record contains no such field.
        // Simply returns the very first field with pmkr
        // if pfldPriKey is NULL or if the outline hierarchy is the default.
    
    BOOL bNormalizeInterlinearLineBreaks();
    void Paste( CRecPos* prps, Str8 sStr, CMarkerSet* pmkrset, BOOL bNormalizeInterlinear = TRUE ); // insert sStr contents into record at prps
    void ApplyDateStamp( CMarker* pmkrDateStamp ); // update or add datestamp field
    BOOL bCheckConsistency( const CNumberedRecElPtr& pnrl, CShwView* pview, CCheckingState *pcks = NULL );
    
    BOOL bCheckFieldConsistency(CField* pfld, const CNumberedRecElPtr& pnrl,
                                CShwView* pview, CCheckingState *pcks);

public:
	Str8 m_sXMLAttributes; // 1.5.9ec Storage for possible XML attributes in record marker group tag

private:
    // Checks Data Properties. If they don't pass, it applies any matching skip-all or replace-all. If no
    // skip/replace match is found, display the appropriate dialog box. If any kind of replacement occurs,
    // cycle back through until data passes or until it is skipped by user. Returns TRUE unless user chooses
    // cancel in the dialog box.
    BOOL bFieldDataPropertiesPassed(CField* pfld, const CNumberedRecElPtr& pnrl,
                                    CShwView* pview, CCheckingState *pcks);
private:
    BOOL bProcessSkipOrReplaceAll(const Str8& sData, const CRecPos& rpsStart, const CRecPos& rpsEnd,
                                  CCheckingState *pcks, BOOL &bReplacementMade);

public:
#ifdef _DEBUG
    void AssertValid() const;
#endif  // _DEBUG
};  // class CRecord


//------------------------------------------------------
class CRecLookEl;  // crlookel.h
class CIndex;  // ind.h

// A field iterator traverses the fields in an index element's subrecord
// according to the marker hierarchy, which gives the effect of a reversal
// by the index's primary key marker. If that is simply the record marker,
// then the field iterator is equivalent to a field pointer.
// See further comments below the class declaration.

class CFieldIterator  // Hungarian: qfld
{
public:
    CFieldIterator(const CRecLookEl* prel, const CIndex* pind);
    CFieldIterator();  // Default constructor: End Iterator
    CFieldIterator(const CFieldIterator& qfld);  // Copy constructor
    
    BOOL operator==(const CFieldIterator& qfld) const
            { return m_pfld == qfld.m_pfld; }
    BOOL operator!=(const CFieldIterator& qfld) const
            { return m_pfld != qfld.m_pfld; }
    const CField* operator*() const  // Dereference  
            { ASSERT( m_pfld ); return m_pfld; }
    CFieldIterator& operator++();  // Pre-increment
    const CFieldIterator operator++(int);  // Post-increment

private:
    const CRecord* m_prec;  // The record over whose fields this iterates
    const CField* m_pfldPriKey;  // The record element's primary key field
    BOOL m_bSubstitutedForNullPriKey;  // No primary key field in record
    BOOL m_bPriKeyIsFirstField;  // m_pfldPriKey is the record key field
    const CField* m_pfld;  // The current field
    BOOL m_bUnderPriKey;  // The current field is now after m_pfldPriKey

    void NextField();
        // Move m_pfld to the next field in the sub-record to be included.
    BOOL bIncludeField() const;
        // Return whether m_pfld is a field to be included.
    BOOL bIncludeMarker(const CMarker* pmkr) const;
        // Return whether pmkr is a marker to be included.
    void NextFieldInSubRecord();
        // Move m_pfld to the next field in the sub-record.
    BOOL bInSubRecord() const;
        // Return whether m_pfld is in the sub-record.
    void NextFieldInRecord();
        // Move m_pfld to the next field in the record, taking into account
        // that m_pfldPriKey is being considered the record's first field.
};  // CFieldIterator

// An iterator provides a way to access the elements of an aggregate object
// sequentially without exposing its underlying representation. Iterators
// generalize C/C++ pointers. A container can provide via iterators
// one or more means for algorithms to step through the objects they own.

// See:  _Design_Patterns_, pp. 257-271;
//       _STL_Tutorial_and_Reference_Guide_, "Input Iterators", pp. 50-52.
// Note: In contrast to an Standard Template Library input iterator,
//       whose dereference operator return a reference-to-object,
//       CFieldIterator's returns a pointer-to-object, which seems to
//       fit better with the rest of Shoebox's classes. This detail,
//       potentially confusing at first sight, has inspired the Hungarian:
//       qfld is a field iterator, which resembled pfld, a field pointer,
//       EXCEPT that *qfld returns a pfld NOT a fld, as you might expect.
//       So since q is p backwards, qfld is like pfld, only backwards.

#endif  // CRECORD_H
