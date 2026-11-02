#include "stdafx.h"
#include "ChangeTable.h"
#include <windows.h>
#include "ccdll.h"
#include <fstream>
#include <mutex>
#include <utility>

namespace
{
    // CC swaps each table's state in and out of process-wide globals
    // (SaveState/RestoreState) with no locking of its own, so all calls
    // into CC must be serialized across the whole process.
    std::mutex g_ccMutex;

    const size_t kScratchSize = 65536;   // output staging buffer
    const size_t kFileChunk = 16384;   // input chunk for file processing
}

ChangeTable::ChangeTable()
    : m_hTable(nullptr), m_bStreamDone(false)
{
}

ChangeTable::~ChangeTable()
{
    Unload();
}

ChangeTable::ChangeTable(ChangeTable&& other) noexcept
    : m_hTable(other.m_hTable),
    m_bStreamDone(other.m_bStreamDone),
    m_scratch(std::move(other.m_scratch))
{
    other.m_hTable = nullptr;
    other.m_bStreamDone = false;
}

ChangeTable& ChangeTable::operator=(ChangeTable&& other) noexcept
{
    if (this != &other)
    {
        Unload();
        m_hTable = other.m_hTable;
        m_bStreamDone = other.m_bStreamDone;
        m_scratch = std::move(other.m_scratch);
        other.m_hTable = nullptr;
        other.m_bStreamDone = false;
    }
    return *this;
}

void ChangeTable::UnloadLocked()
{
    if (m_hTable != nullptr)
    {
        CCUnloadTable(static_cast<HANDLE>(m_hTable));
        m_hTable = nullptr;
    }
    m_bStreamDone = false;
}

void ChangeTable::Unload()
{
    std::lock_guard<std::mutex> lock(g_ccMutex);
    UnloadLocked();
}

bool ChangeTable::Reinitialize()
{
    if (!bLoaded())
        return false;

    std::lock_guard<std::mutex> lock(g_ccMutex);
    const bool ok = (CCReinitializeTable(static_cast<HANDLE>(m_hTable)) == CC_SUCCESS);
    m_bStreamDone = false;
    return ok;
}

bool ChangeTable::bLoadFromFile(const char* pszChangeTablePath)
{
    if (!pszChangeTablePath || *pszChangeTablePath == '\0')
        return false;

    std::lock_guard<std::mutex> lock(g_ccMutex);
    UnloadLocked();

    HANDLE h = nullptr;
    // CCLoadTable2 needs no parent HINSTANCE.
    const int rc = CCLoadTable2(const_cast<char*>(pszChangeTablePath), &h);

    // CC_SUCCESS (text) and CC_SUCCESS_BINARY are both successful loads.
    if ((rc == CC_SUCCESS || rc == CC_SUCCESS_BINARY) && h != nullptr)
    {
        m_hTable = h;
        return true;
    }
    return false;
}

bool ChangeTable::bMakeChanges(const char* pszInput, int lenIn, char* pszOutput, int* plenOut)
{
    if (!bLoaded() || !pszInput || !pszOutput || !plenOut || *plenOut <= 0 || lenIn < 0)
        return false;

    if (lenIn == 0)                 // CCProcessBuffer rejects empty input
    {
        *plenOut = 0;
        return true;
    }

    std::lock_guard<std::mutex> lock(g_ccMutex);

    // CCProcessBuffer calls CCReinitializeTable itself, so every call starts
    // from a clean state. It also fails (rather than truncating) if the
    // output does not fit in *plenOut.
    const int rc = CCProcessBuffer(static_cast<HANDLE>(m_hTable),
        const_cast<char*>(pszInput), lenIn,
        pszOutput, plenOut);
    m_bStreamDone = false;          // CC state was reset by the call
    return rc == CC_SUCCESS;
}

bool ChangeTable::bStreamChunk(const char* pszIn, int lenIn, bool bLast, std::string& out)
{
    if (!bLoaded() || m_bStreamDone || lenIn < 0 || (lenIn > 0 && !pszIn))
        return false;

    std::lock_guard<std::mutex> lock(g_ccMutex);

    if (m_scratch.empty())
        m_scratch.resize(kScratchSize);
    const int cap = static_cast<int>(m_scratch.size());

    const char* p = pszIn;
    int n = lenIn;

    for (;;)
    {
        int outLen = cap;
        const int rc = CCMultiProcessBuffer(static_cast<HANDLE>(m_hTable),
            const_cast<char*>(p), n,
            bLast ? TRUE : FALSE,
            m_scratch.data(), &outLen);

        if ((rc != CC_SUCCESS && rc != CC_GOT_END_OF_DATA && rc != CC_CALL_AGAIN_FOR_MORE_DATA)
            || outLen < 0 || outLen > cap)
        {
            m_bStreamDone = true;   // CC state is unreliable after an error
            return false;
        }

        if (outLen > 0)
            out.append(m_scratch.data(), static_cast<size_t>(outLen));

        if (rc == CC_GOT_END_OF_DATA)
        {
            m_bStreamDone = true;
            return true;
        }

        // More output may be waiting if CC says so (only reported on the last
        // call), or if a non-final call filled the output buffer. The "- 1"
        // allows for a double-byte character split across the buffer end.
        // Drain by calling again with no input.
        const bool bMore = (rc == CC_CALL_AGAIN_FOR_MORE_DATA)
            || (!bLast && outLen + 1 >= cap);
        if (!bMore)
            return true;

        p = nullptr;
        n = 0;
    }
}

bool ChangeTable::bMakeChanges(const char* pszInPath, const char* pszOutPath)
{
    if (!bLoaded() || !pszInPath || !pszOutPath)
        return false;

    // Raw bytes both ways. (CCProcessFile opens the files in text mode unless
    // the table asks for binary, which would translate CRLF and stop at
    // Ctrl-Z, and so behave differently from the buffer and stream paths.)
    std::ifstream ifs(pszInPath, std::ios::binary);
    if (!ifs)
        return false;
    std::ofstream ofs(pszOutPath, std::ios::binary | std::ios::trunc);
    if (!ofs)
        return false;

    if (!Reinitialize())
        return false;

    std::vector<char> in(kFileChunk);
    std::string out;
    bool bLast = false;

    while (!bLast)
    {
        ifs.read(in.data(), static_cast<std::streamsize>(in.size()));
        if (ifs.bad())
            return false;
        const int n = static_cast<int>(ifs.gcount());
        bLast = (n < static_cast<int>(in.size()));   // short read means EOF

        out.clear();
        if (!bStreamChunk(in.data(), n, bLast, out))
            return false;

        if (!out.empty())
            ofs.write(out.data(), static_cast<std::streamsize>(out.size()));
        if (!ofs)
            return false;
    }

    ofs.close();
    return !ofs.fail();
}