#pragma once
#include "stdafx.h"
#include <string>
#include <vector>

//
// Wrapper around the statically linked CC (Consistent Changes) library.
//
// Three ways to use a loaded table:
//   1. bMakeChanges(buffer...)  one-shot: whole input in, whole output out.
//   2. bMakeChanges(path, path) file to file (binary, streamed internally).
//   3. bStreamChunk(...)        incremental: feed input in pieces, state is
//                               kept between calls (stores, switches, matches
//                               that span chunk boundaries).
//
class ChangeTable
{
public:
    ChangeTable();
    ~ChangeTable();

    // Copying would double-free the CC table handle.
    ChangeTable(const ChangeTable&) = delete;
    ChangeTable& operator=(const ChangeTable&) = delete;

    ChangeTable(ChangeTable&& other) noexcept;
    ChangeTable& operator=(ChangeTable&& other) noexcept;

    // Table lifecycle
    bool bLoadFromFile(const char* pszChangeTablePath);
    void Unload();
    bool Reinitialize();               // reset to the start-of-data state
    bool bLoaded() const { return m_hTable != nullptr; }

    // One-shot buffer transformation. On entry *plenOut is the capacity of
    // pszOutput; on success it is the number of bytes produced.
    bool bMakeChanges(const char* pszInput, int lenIn, char* pszOutput, int* plenOut);

    // File to file. Both files are handled as raw bytes.
    bool bMakeChanges(const char* pszInPath, const char* pszOutPath);

    // Incremental transformation. Appends whatever CC produces to 'out'.
    // Output normally lags input, because CC holds text back while a match
    // is still possible; the held-back text comes out on the call with
    // bLast == true. After that call bStreamDone() is true and further calls
    // fail until Reinitialize() (or a new Load) is done.
    //
    // These functions aren't currently used, but they could be called in the future
    // to better handle large amounts of data.
    bool bStreamChunk(const char* pszIn, int lenIn, bool bLast, std::string& out);
    bool bStreamDone() const { return m_bStreamDone; }

private:
    void UnloadLocked();

    void* m_hTable;        // CC table handle (HANDLE is void*)
    bool              m_bStreamDone;
    std::vector<char> m_scratch;       // output staging buffer for bStreamChunk
};