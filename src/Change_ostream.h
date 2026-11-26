#pragma once
#include "stdafx.h"
#include <streambuf>
#include <ostream>
#include <vector>
#include "ChangeTable.h"

class Change_ostreambuf : public std::streambuf
{
public:
    Change_ostreambuf(const char* pszChangeTablePath, std::ostream& iosOutput);
    ~Change_ostreambuf() override;

    // Pass the remaining buffered data to CC with the "last input" flag and
    // write out everything CC was holding back. Called automatically by the
    // destructor; call it yourself if you need the complete output while the
    // stream object is still alive. Nothing more can be written afterwards.
    void finish();

protected:
    int sync() override;
    int overflow(int ch = std::char_traits<char>::eof()) override;

private:
    bool bPushPending(bool bLast);

    ChangeTable       m_cct;
    std::ostream& m_iosOutput;
    std::vector<char> m_buf;         // one spare byte at the end for overflow()
    bool              m_bFinished;
    bool              m_bFailed;     // CC error seen: data is passed through
};

// Writes to iosOutput with the change table applied.
// NOTE: CC holds text back while a match is still possible, so flush() does
// not guarantee that everything written so far has reached iosOutput. The
// tail is released by finish() or by destroying the stream.
// If the table cannot be loaded, data is passed through unchanged (as before).
class Change_ostream : public std::ostream
{
public:
    Change_ostream(const char* pszChangeTablePath, std::ostream& iosOutput)
        : std::ostream(nullptr),
        m_buf(pszChangeTablePath, iosOutput)
    {
        init(&m_buf);
    }

    ~Change_ostream() override = default;

    void finish()
    {
        flush();
        m_buf.finish();
    }

private:
    Change_ostreambuf m_buf;
};