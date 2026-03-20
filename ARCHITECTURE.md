# Toolbox Architecture

## 1. System Overview

### 1.1 Ownership Hierarchy

```
CShwApp (global)
├─ CProject (current project)
│  ├─ CDatabaseTypeSet
│  │  └─ CDatabaseType[1..n]
│  │     ├─ CMarkerSet
│  │     ├─ CFilterSet
│  │     ├─ CJumpPathSet
│  │     └─ CInterlinearProcList
│  │        └─ CLookupProc[1..n]
│  ├─ CLangEncSet
│  └─ CCorpusSet
├─ CMainFrame (main window)
│  └─ CShwView[1..n] (open windows)
│     └─ CShwDoc (reference, not owned)
└─ CShwDoc[1..n] (open database files)
   └─ CIndexSet
      └─ CIndex[1..n]
         └─ CRecord[1..n]
            └─ CField[1..n]

References (not ownership):

CField  → CMarker (via CMString)
CIndex  → CMarker (sort key), CFilter
CShwView → CShwDoc (displays)
CRecPos → CRecord, CField (navigation cursor)
```

### 1.2 Ownership Graph

```mermaid
graph TB
    APP["CShwApp<br/>(global singleton)"]
    PRJ["CProject"]
    TYPSET["CDatabaseTypeSet"]
    LNGSET["CLangEncSet"]
    CORSET["CCorpusSet"]

    DBT["CDatabaseType"]
    MKR["CMarkerSet"]
    FIL["CFilterSet"]
    JMP["CJumpPathSet"]
    INT["CInterlinearProcList"]

    MF["CMainFrame"]
    VIEW["CShwView"]

    DOC["CShwDoc"]
    INDSET["CIndexSet"]
    IND["CIndex"]
    RECLE["CRecLookEl"]
    REC["CRecord"]

    APP -->|owns| PRJ
    APP -->|owns| MF
    APP -->|owns| DOC

    PRJ -->|owns| TYPSET
    PRJ -->|owns| LNGSET
    PRJ -->|owns| CORSET

    TYPSET -->|contains| DBT
    DBT -->|owns| MKR
    DBT -->|owns| FIL
    DBT -->|owns| JMP
    DBT -->|owns| INT

    MF -->|owns| VIEW
    VIEW -.->|displays| DOC

    DOC -->|owns| INDSET
    INDSET -->|contains| IND
    IND -->|contains| RECLE
    RECLE -.->|references| REC

    style APP fill:#2e7d32
    style PRJ fill:#558b2f
    style TYPSET fill:#558b2f
    style LNGSET fill:#558b2f
    style CORSET fill:#558b2f
    style DBT fill:#81c784
    style MKR fill:#a5d6a7
    style FIL fill:#a5d6a7
    style JMP fill:#a5d6a7
    style INT fill:#a5d6a7
    style MF fill:#1976d2
    style VIEW fill:#1565c0
    style DOC fill:#81c784
    style INDSET fill:#a5d6a7
    style IND fill:#a5d6a7
    style RECLE fill:#b3e5fc
    style REC fill:#4fc3f7
```

### 1.3 Project Structure Graph

```mermaid
graph TD
    APP["CShwApp<br/>(global)"]
    PRJ["CProject<br/>(current project)"]
    TYPSET["CDatabaseTypeSet"]
    DBT1["CDatabaseType"]
    DBT2["CDatabaseType"]

    LNGSET["CLangEncSet"]
    CORSET["CCorpusSet"]

    MWN["CMainFrame<br/>(main window)"]
    VIEW["CShwView<br/>(displays records)"]

    DOC["CShwDoc<br/>(open database)"]
    INDSET["CIndexSet"]
    IND["CIndex<br/>(sorted view)"]
    RECLE["CRecLookEl<br/>(indexed record)"]
    REC["CRecord"]

    APP -->|owns| PRJ
    APP -->|owns| MWN
    APP -->|owns| DOC

    PRJ -->|owns| TYPSET
    PRJ -->|owns| LNGSET
    PRJ -->|owns| CORSET

    TYPSET -->|contains| DBT1
    TYPSET -->|contains| DBT2

    MWN -->|owns| VIEW
    VIEW -.->|displays| DOC

    DOC -->|owns| INDSET
    INDSET -->|contains| IND
    IND -->|contains| RECLE
    RECLE -.->|references| REC

    style APP fill:#c8e6c9
    style PRJ fill:#a5d6a7
    style MWN fill:#66bb6a
    style VIEW fill:#43a047
    style DOC fill:#81c784
    style INDSET fill:#a5d6a7
    style IND fill:#a5d6a7
    style RECLE fill:#b3e5fc
    style REC fill:#4fc3f7
```

---

## 2. High-Level Summary

1. **Application Layer** (`CShwApp` → `CMainFrame` → `CShwView`): UI and global state
2. **Project Layer** (`CProject`): All settings, schemas, and corpuses for a project
3. **Document Layer** (`CShwDoc`): One open database file and its indexes
4. **Processing Layer** (`CInterlinearProcList` → `CLookupProc`): How data is analyzed and transformed
5. **Indexing Layer** (`CIndex` → `CIndexSet`): How records are sorted and searched
6. **Organization Layer** (`CFieldList` → `CMarkerSet` → `CDatabaseType`): How fields are organized and what they mean
7. **Bottom Layer** (`Str8` → `CMString` → `CField` → `CRecord`): Raw text representation and storage

**Key Insight**: A `CRecord` is the atomic unit—it contains all fields for one entry. Fields are marked with `CMarker` to give them meaning. Multiple `CIndex` views sort the same records differently. `CShwDoc` holds the file; `CProject` holds the schema. `CShwView` displays it to the user.

---

## 3. Visual Data Flow

```
CShwApp (global application)
└─ CProject (current project settings)
   └─ CDatabaseTypeSet
      └─ CDatabaseType "Dictionary"
         ├─ CMarkerSet
         │   ├─ CMarker "tx" (text field)
         │   ├─ CMarker "ps" (part of speech)
         │   └─ CMarker "dt" (definition)
         ├─ CInterlinearProcList
         │   └─ CLookupProc (morpheme parser)
         │       └─ CDbTrie (lexicon)
         │           ├─ Entry: "jump" → gloss
         │           └─ Entry: "-ed" → gloss
         └─ CIndexSet
            └─ CIndex (sorted by headword)
               └─ CRecLookEl[1]
                  └─ CRecord (one dictionary entry)
                     ├─ CField[1]: marker="tx", content="jumped"
                     │   └─ CMString
                     │       ├─ Str8: "jumped"
                     │       └─ CMarker: "tx"
                     ├─ CField[2]: marker="ps", content="verb"
                     │   └─ CMString
                     │       ├─ Str8: "verb"
                     │       └─ CMarker: "ps"
                     └─ CField[3]: marker="dt", content="past tense of jump"

CShwView (UI window)
└─ displays CRecord via CRecPos navigation
   └─ CRecPos: prec=CRecord, pfld=CField[1], iChar=0
```

### 3.1 Text Entry to Display Diagram

```mermaid
sequenceDiagram
    participant User as User
    participant View as CShwView
    participant Doc as CShwDoc
    participant Index as CIndex
    participant RecLE as CRecLookEl
    participant Rec as CRecord
    participant Field as CField
    participant RecPos as CRecPos
    User->>View: Navigate to record
    activate View
    View->>Doc: Request record
    activate Doc
    Doc->>Index: Get record element
    activate Index
    Index->>RecLE: Lookup record
    activate RecLE
    RecLE-->>Index: Return record ref
    deactivate RecLE
    Index-->>Doc: Record element found
    deactivate Index
    Doc->>Rec: Display fields
    activate Rec
    Rec->>Field: Get field content
    activate Field
    Field-->>Rec: Return content
    deactivate Field
    Rec-->>Doc: All fields
    deactivate Rec
    Doc-->>View: Display data
    deactivate Doc
    View->>RecPos: Create cursor
    activate RecPos
    RecPos->>Rec: Navigate
    RecPos-->>View: Cursor ready
    deactivate RecPos
    View-->>User: Show record on screen
    deactivate View
```

---

## 4. Core Data Model

### 4.1 Core Text Representation

| Class        | What it holds                     | Relationships                                                   | Role                                                            |
| ------------ | --------------------------------- | --------------------------------------------------------------- | --------------------------------------------------------------- |
| **Str8**     | UTF-8 string content              | Base class for all string objects                               | Raw string storage with UTF-8 support                           |
| **CMString** | String text + marker reference    | Inherits from `Str8`; references `CMarker`                      | Marked string: text tagged with metadata
| **CMarker**  | Descriptor metadata               | Referenced by `CField` (via `CMString`); member of `CMarkerSet` | Defines field type: name, language encoding, display properties |
| **CField**   | String content + marker reference | Inherits from `CMString` and `CDblListEl`; references `CMarker` | One field/data element in a record (e.g., `\dt definition`)     |

### 4.2 Field Collection Layer

| Class          | What it holds                              | Relationships                                         | Role                                                                 |
| -------------- | ------------------------------------------ | ----------------------------------------------------- | -------------------------------------------------------------------- |
| **CDblListEl** | List linkage (prev/next pointers)          | Base class for all list elements                      | Provides doubly-linked list support                                  |
| **CDblList**   | Ordered collection of `CDblListEl` objects | Generic doubly-linked list container                  | Generic list structure for any element type                          |
| **CFieldList** | Ordered list of `CField` objects           | Inherits from `CDblList`; contains `CField` objects   | Container for fields in a record                                     |
| **CRecord**    | Multiple `CField` objects with a key field | Inherits from `CFieldList`; contains `CField` objects | One complete entry/record in a database (e.g., one dictionary entry) |

### 4.3 Inheritance Graph

```mermaid
graph TD
    Str8["Str8<br/>(UTF-8 string)"]
    CMString["CMString<br/>(Str8 + CMarker)"]
    CField["CField<br/>(CMString + CDblListEl)"]
    CDblListEl["CDblListEl<br/>(List node)"]
    CDblList["CDblList<br/>(Generic linked list)"]
    CFieldList["CFieldList<br/>(CDblList)"]
    CRecord["CRecord<br/>(CFieldList)"]

    Str8 -->|inherits| CMString
    CDblListEl -->|inherits| CField
    CMString -->|inherits| CField
    CDblList -->|inherits| CFieldList
    CFieldList -->|inherits| CRecord

    CMarker["CMarker<br/>(Descriptor)"]
    CMString -.->|references| CMarker
    CField -.->|via CMString| CMarker

    style Str8 fill:#e1f5ff
    style CMString fill:#b3e5fc
    style CField fill:#81d4fa
    style CRecord fill:#4fc3f7
    style CMarker fill:#ffeb3b
```

### 4.4 Data Model Example: One Record Structure

```mermaid
graph TD
    Rec["CRecord<br/>(one entry)"]
    FL["CFieldList<br/>(collection)"]
    F1["CField 1<br/>marker: tx"]
    F2["CField 2<br/>marker: ps"]
    F3["CField 3<br/>marker: dt"]

    MS1["CMString<br/>marker + content"]
    S1["Str8: 'jumped'"]
    M1["CMarker: tx"]

    MS2["CMString"]
    S2["Str8: 'verb'"]
    M2["CMarker: ps"]

    MS3["CMString"]
    S3["Str8: 'past tense'"]
    M3["CMarker: dt"]

    Rec -->|contains| FL
    FL -->|contains| F1
    FL -->|contains| F2
    FL -->|contains| F3

    F1 -->|is-a| MS1
    MS1 -->|holds| S1
    MS1 -->|references| M1

    F2 -->|is-a| MS2
    MS2 -->|holds| S2
    MS2 -->|references| M2

    F3 -->|is-a| MS3
    MS3 -->|holds| S3
    MS3 -->|references| M3

    style Rec fill:#4fc3f7
    style FL fill:#81d4fa
    style F1 fill:#b3e5fc
    style F2 fill:#b3e5fc
    style F3 fill:#b3e5fc
    style M1 fill:#ffeb3b
    style M2 fill:#ffeb3b
    style M3 fill:#ffeb3b
```

## 5. Database Structure Layer

### 5.1 Schema Definitions

| Class                | What it holds                       | Relationships                                                                                             | Role                                                                     |
| -------------------- | ----------------------------------- | --------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------ |
| **CMarkerSet**       | All markers for one database type   | Member of `CDatabaseType`; contains `CMarker` objects                                                     | Defines all field types possible in a database                           |
| **CFilterSet**       | All filters for one database type   | Member of `CDatabaseType`                                                                                 | Search/display filters                                                   |
| **CJumpPathSet**     | Jump paths between records          | Member of `CDatabaseType`                                                                                 | Cross-reference definitions                                              |
| **CDatabaseType**    | Complete database schema + settings | Member of `CDatabaseTypeSet`; contains `CMarkerSet`, `CFilterSet`, `CJumpPathSet`, `CInterlinearProcList` | Defines structure of one type of database (e.g., "Dictionary", "Corpus") |
| **CDatabaseTypeSet** | All database type definitions       | Member of `CProject`                                                                                      | Collection of all database schemas in a project                          |

---

### 5.2 Database Relationship Graph

```mermaid
graph TD
    DBT["CDatabaseType<br/>(Schema)"]
    MKR["CMarkerSet<br/>(all marker<br/>definitions)"]
    FIL["CFilterSet<br/>(search/display<br/>filters)"]
    JMP["CJumpPathSet<br/>(cross-refs)"]
    INTERLIN["CInterlinearProcList<br/>(morpheme<br/>processing)"]
    M1["CMarker: tx"]
    M2["CMarker: ps"]
    M3["CMarker: dt"]

    LP["CLookupProc<br/>(lexicon lookup)"]
    RP["CRearrangeProc<br/>(rearrange)"]
    TRIE["CDbTrie<br/>(lexicon)"]

    DBT -->|owns| MKR
    DBT -->|owns| FIL
    DBT -->|owns| JMP
    DBT -->|owns| INTERLIN

    MKR -->|contains| M1
    MKR -->|contains| M2
    MKR -->|contains| M3

    INTERLIN -->|contains| LP
    INTERLIN -->|contains| RP

    LP -->|uses| TRIE

    style DBT fill:#81c784
    style MKR fill:#a5d6a7
    style M1 fill:#ffeb3b
    style M2 fill:#ffeb3b
    style M3 fill:#ffeb3b
    style LP fill:#ffccbc
    style RP fill:#ffccbc
    style TRIE fill:#ff8a65
```

## 6. Index & Record Storage Layer

### 6.1 Indexing Logic

| Class          | What it holds                     | Relationships                                          | Role                                                                       |
| -------------- | --------------------------------- | ------------------------------------------------------ | -------------------------------------------------------------------------- |
| **CRecLookEl** | One record lookup element         | Member of `CIndex`; references `CRecord`               | Indexed record entry with sort key                                         |
| **CIndex**     | Sorted collection of `CRecLookEl` | Member of `CIndexSet`; owns sort keys and filter state | One view/index of records (same records, different sort order or filtered) |
| **CIndexSet**  | Multiple sorted `CIndex` views    | Member of `CShwDoc`                                    | All open indexes for one database document                                 |
| **CShwDoc**    | One open database file            | Contains `CIndexSet`; represents one open `.db` file   | Document/file containing all records of one database                       |

---

### 6.2 Index & Record Lookup Flow Graph

```mermaid
graph LR
    CDB["CShwDoc<br/>(database file)"]
    INDSET["CIndexSet<br/>(all indexes)"]
    IND1["CIndex 1<br/>(sorted by<br/>headword)"]
    IND2["CIndex 2<br/>(sorted by<br/>frequency)"]
    RECLE1["CRecLookEl 1"]
    RECLE2["CRecLookEl 2"]
    RECLE3["CRecLookEl 3"]

    REC1["CRecord 1"]
    REC2["CRecord 2"]
    REC3["CRecord 3"]

    CDB -->|owns| INDSET
    INDSET -->|contains| IND1
    INDSET -->|contains| IND2

    IND1 -->|contains| RECLE1
    IND1 -->|contains| RECLE2
    IND1 -->|contains| RECLE3

    IND2 -->|different order| RECLE2
    IND2 -->|different order| RECLE1
    IND2 -->|different order| RECLE3

    RECLE1 -.->|references| REC1
    RECLE2 -.->|references| REC2
    RECLE3 -.->|references| REC3

    style CDB fill:#81c784
    style INDSET fill:#a5d6a7
    style IND1 fill:#66bb6a
    style IND2 fill:#66bb6a
    style RECLE1 fill:#b3e5fc
    style RECLE2 fill:#b3e5fc
    style RECLE3 fill:#b3e5fc
    style REC1 fill:#4fc3f7
    style REC2 fill:#4fc3f7
    style REC3 fill:#4fc3f7
```

## 7. Functional Subsystems

### 7.1 Navigation & Cursor Management

| Class       | What it holds                       | Relationships                     | Role                                                     |
| ----------- | ----------------------------------- | --------------------------------- | -------------------------------------------------------- |
| **CRecPos** | Record + Field + Character position | References `CRecord` and `CField` | Cursor/caret position for navigating through record data |

```mermaid
graph TD
    REC["CRecord<br/>(entry)"]
    FL["CFieldList"]
    F1["CField<br/>marker: tx"]
    F2["CField<br/>marker: ps"]
    F3["CField<br/>marker: dt"]
    CONTENT1["jumped"]
    CONTENT2["verb"]
    CONTENT3["past tense"]

    RECPOS["CRecPos<br/>current position"]
    PREC["prec →<br/>CRecord"]
    PFLD["pfld →<br/>CField"]
    ICHAR["iChar →<br/>position in text"]

    REC -->|contains| FL
    FL -->|contains| F1
    FL -->|contains| F2
    FL -->|contains| F3

    F1 -->|holds| CONTENT1
    F2 -->|holds| CONTENT2
    F3 -->|holds| CONTENT3

    RECPOS -->|points to| PREC
    RECPOS -->|points to| PFLD
    RECPOS -->|marks| ICHAR

    PREC -.->|is| REC
    PFLD -.->|is| F1
    ICHAR -.->|at char 0 of| CONTENT1

    style REC fill:#4fc3f7
    style FL fill:#81d4fa
    style F1 fill:#b3e5fc
    style F2 fill:#b3e5fc
    style F3 fill:#b3e5fc
    style RECPOS fill:#fff9c4
    style PREC fill:#fff9c4
    style PFLD fill:#fff9c4
    style ICHAR fill:#fff9c4
```

### 7.2 Interlinearization Pipeline

| Class                    | What it holds                        | Relationships                                                                              | Role                                              |
| ------------------------ | ------------------------------------ | ------------------------------------------------------------------------------------------ | ------------------------------------------------- |
| **CInterlinearProc**     | Base class for processing operations | Abstract base; owned by `CInterlinearProcList`                                             | Parent class for all interlinear processes        |
| **CLookupProc**          | Lexicon lookup and morpheme parsing  | Inherits from `CInterlinearProc`; contains `CDbTrie` (lexicon)                             | Performs morpheme lookup: breaks words into parts |
| **CRearrangeProc**       | Word rearrangement rules             | Inherits from `CInterlinearProc`                                                           | Rearranges morphemes (e.g., for adaptation)       |
| **CGivenProc**           | Mark fields as already analyzed      | Inherits from `CInterlinearProc`                                                           | Marks fields to skip processing                   |
| **CInterlinearProcList** | Ordered list of processes to run     | Inherits from `CDblList`; contains `CInterlinearProc` objects; owns `CMarkerSet` reference | Sequence of morpheme processing steps             |

---

### 7.3 Processing Graph

```mermaid
graph LR
    REC["CRecord<br/>(text: 'jumped')"]
    INTERLIN["CInterlinearProcList"]
    LP["CLookupProc<br/>(lookup)"]
    RP["CRearrangeProc<br/>(rearrange)"]
    GP["CGivenProc<br/>(mark given)"]
    TRIE["CDbTrie<br/>(lexicon)<br/>entries: jump, -ed"]

    REC -->|pass through| INTERLIN
    INTERLIN -->|step 1| LP
    LP -->|lookup in| TRIE
    LP -->|break into| MORPH["Morphemes<br/>jump, -ed"]
    MORPH -->|insert fields| REC2["CRecord<br/>(updated)"]
    REC2 -->|step 2| RP
    RP -->|rearrange| REC3["CRecord<br/>(rearranged)"]
    REC3 -->|step 3| GP
    GP -->|mark fields| REC4["CRecord<br/>(final)"]

    style REC fill:#4fc3f7
    style INTERLIN fill:#ffccbc
    style LP fill:#ffccbc
    style RP fill:#ffccbc
    style GP fill:#ffccbc
    style TRIE fill:#ff8a65
    style MORPH fill:#fff9c4
    style REC2 fill:#4fc3f7
    style REC3 fill:#4fc3f7
    style REC4 fill:#4fc3f7
```

### 7.4 Workflow Diagram

User selects "jumped" record in text:

1. CShwView displays CRecord via CRecPos
2. User triggers interlinearize action
3. CLookupProc::bInterlinearize() called
4. CLookupProc looks up "jumped" in CDbTrie lexicon
5. Trie returns entries: "jump", "-ed"
6. CLookupProc creates new CField objects for morphemes
7. Inserts into CRecord's CFieldList
8. CShwView updated to show morpheme breakdown

```mermaid
sequenceDiagram
    participant User as User
    participant Lookup as CLookupProc
    participant Trie as CDbTrie
    participant Rec as CRecord
    participant FL as CFieldList
    participant Field as CField
    User->>Lookup: Call bInterlinearize
    activate Lookup
    Lookup->>Lookup: Parse input "jumped"
    Lookup->>Trie: Lookup "jumped"
    activate Trie
    Trie-->>Lookup: Not found
    deactivate Trie
    Lookup->>Lookup: Try "jump"
    Lookup->>Trie: Lookup "jump"
    activate Trie
    Trie-->>Lookup: Found: root
    deactivate Trie
    Lookup->>Lookup: Remaining: "-ed"
    Lookup->>Trie: Lookup "-ed"
    activate Trie
    Trie-->>Lookup: Found: suffix
    deactivate Trie
    Lookup->>Rec: Insert morpheme fields
    activate Rec
    Rec->>FL: Add field
    activate FL
    FL->>Field: Create "jump"
    activate Field
    Field-->>FL: Field created
    deactivate Field
    FL-->>Rec: Field added
    deactivate FL
    Rec->>FL: Add field
    activate FL
    FL->>Field: Create "-ed"
    activate Field
    Field-->>FL: Field created
    deactivate Field
    FL-->>Rec: Field added
    deactivate FL
    Rec-->>Lookup: Record updated
    deactivate Rec
    Lookup-->>User: Interlinearization complete
    deactivate Lookup
```

## 8. Project & Application Layer

| Class           | What it holds                | Relationships                                                                     | Role                                                                  |
| --------------- | ---------------------------- | --------------------------------------------------------------------------------- | --------------------------------------------------------------------- |
| **CCorpus**     | Text corpus for analysis     | Member of `CCorpusSet`                                                            | Collection of texts for linguistic analysis                           |
| **CCorpusSet**  | All text corpuses in project | Member of `CProject`                                                              | Manages available text resources                                      |
| **CLangEncSet** | All language encodings       | Member of `CProject`                                                              | Defines all writing systems/languages in project                      |
| **CProject**    | Complete project settings    | Contains `CDatabaseTypeSet`, `CLangEncSet`, `CCorpusSet`; referenced by `CShwDoc` | Configuration for entire project (all databases, languages, settings) |
| **CShwApp**     | Global application state     | Global singleton; contains `CProject`; owns main window                           | MFC application class; manages project and UI                         |
| **CMainFrame**  | Main window UI               | Referenced by `CShwApp`; contains views                                           | Main MDI frame window                                                 |
| **CShwView**    | Display one `CIndex`         | References `CShwDoc` and displays records                                         | View/window showing one index of records                              |

---

## 9. Files and Dependencies

### 9.1 Core Modules

| Module            | Primary Classes                     | Purpose                                        |
| ----------------- | ----------------------------------- | ---------------------------------------------- |
| **str8.cpp/h**    | `Str8`                              | UTF-8 string implementation                    |
| **mkr.cpp/h**     | `CMString`, `CMarker`, `CMarkerSet` | Marked strings and marker definitions          |
| **cfield.cpp/h**  | `CField`, `CFieldList`              | Field storage and collections                  |
| **crecord.cpp/h** | `CRecord`                           | Record storage and layout                      |
| **crecpos.cpp/h** | `CRecPos`                           | Navigation cursor                              |
| **lng.cpp/h**     | `CLangEnc`, `CLangEncSet`           | Language encoding (writing system) definitions |

### 9.2 Database & Index Modules

| Module        | Primary Classes                     | Purpose                     |
| ------------- | ----------------------------------- | --------------------------- |
| **typ.cpp/h** | `CDatabaseType`, `CDatabaseTypeSet` | Database schema definitions |
| **ind.cpp/h** | `CIndex`, `CIndexSet`, `CRecLookEl` | Record sorting and indexing |
| **fil.cpp/h** | `CFilter`, `CFilterSet`             | Search/display filters      |

### 9.3 Interlinearization Modules

| Module             | Primary Classes                                                             | Purpose                                     |
| ------------------ | --------------------------------------------------------------------------- | ------------------------------------------- |
| **interlin.cpp/h** | `CInterlinearProc`, `CLookupProc`, `CRearrangeProc`, `CInterlinearProcList` | Morpheme parsing and interlinear processing |
| **trie.cpp/h**     | `CDbTrie`, `CTrieOut`                                                       | Lexicon storage (trie data structure)       |

### 9.4 Project & Application Modules

| Module            | Primary Classes         | Purpose                                |
| ----------------- | ----------------------- | -------------------------------------- |
| **project.cpp/h** | `CProject`              | Project configuration and settings     |
| **corpus.cpp/h**  | `CCorpus`, `CCorpusSet` | Text corpora management                |
| **shw.cpp/h**     | `CShwApp`               | MFC application class and global state |
| **shwdoc.cpp/h**  | `CShwDoc`               | MFC document class (one open database) |
| **shwview.cpp/h** | `CShwView`              | MFC view class (UI window)             |
| **mainfrm.cpp/h** | `CMainFrame`            | MFC main frame window                  |

### 9.5 Utility Modules

| Module             | Primary Classes                    | Purpose                    |
| ------------------ | ---------------------------------- | -------------------------- |
| **cdbllist.cpp/h** | `CDblList`, `CDblListEl`           | Generic doubly-linked list |
| **obstream.cpp/h** | `Object_istream`, `Object_ostream` | Object serialization       |
| **update.cpp/h**   | `CUpdate` and subclasses           | Update notification system |

---

### 9.6 Compilation Dependency

1. str8.h      → (independent - core string)
2. mkr.h       → str8, set, lng
3. cfield.h    → mkr, cdbllist
4. crecord.h   → cfield, crecpos
5. crecpos.h   → crecord (circular with crecord.h)
6. ind.h       → crecord, fil, ptr
7. typ.h       → mkr, fil, interlin, ind
8. project.h   → typ, corpus, lng
9. shwdoc.h    → project, crecord, ind
10. shw.h       → project, shwdoc
11. shwview.h   → shwdoc, crecpos

#### 9.7 Compilation Graph

```mermaid
graph TD
    S["str8.cpp/h"]
    M["mkr.cpp/h<br/>(CMString, CMarker)"]
    CF["cfield.cpp/h<br/>(CField, CFieldList)"]
    CR["crecord.cpp/h<br/>(CRecord)"]
    RP["crecpos.cpp/h<br/>(CRecPos)"]
    L["lng.cpp/h<br/>(CLangEnc, CLangEncSet)"]
    T["typ.cpp/h<br/>(CDatabaseType)"]
    I["ind.cpp/h<br/>(CIndex, CIndexSet)"]
    F["fil.cpp/h<br/>(CFilter)"]
    IN["interlin.cpp/h<br/>(CLookupProc)"]
    TR["trie.cpp/h<br/>(CDbTrie)"]
    P["project.cpp/h<br/>(CProject)"]
    D["shwdoc.cpp/h<br/>(CShwDoc)"]
    V["shwview.cpp/h<br/>(CShwView)"]
    A["shw.cpp/h<br/>(CShwApp)"]

    S --> M
    S --> L
    M --> CF
    L --> T
    CF --> CR
    CR --> RP
    CR --> I
    CR --> F
    T --> IN
    T --> P
    I --> D
    F --> T
    IN --> TR
    P --> D
    D --> V
    P --> A
    V --> A

    style S fill:#e1f5ff
    style M fill:#b3e5fc
    style CF fill:#81d4fa
    style CR fill:#4fc3f7
    style RP fill:#0288d1
    style L fill:#ffeb3b
    style T fill:#a5d6a7
    style I fill:#81c784
    style F fill:#81c784
    style IN fill:#ffccbc
    style TR fill:#ff8a65
    style P fill:#81c784
    style D fill:#66bb6a
    style V fill:#43a047
    style A fill:#2e7d32
```

#### 9.8 Utility includes

- toolbox.h   → shw, lng, mkr (umbrella include for most sources)
- obstream.h  → independent (serialization)
- cdbllist.h  → independent (generic list)
- update.h    → mkr, typ, fil, ind (notification system)

