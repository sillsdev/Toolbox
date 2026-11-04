#pragma once
#include "stdafx.h"
#include <streambuf>
#include <istream>
#include <string>
#include <vector>
#include "ChangeTable.h"

class Change_istreambuf : public std::streambuf
{
public:
    Change_istreambuf(const char* pszChangeTablePath, std::istream& iosInput);
    ~Change_istreambuf() override = default;

protected:
    int underflow() override;

private:
    ChangeTable       m_cct;
    std::istream& m_iosInput;
    std::vector<char> m_in;          // raw input chunk read from m_iosInput
    std::string       m_out;         // transformed data currently being served
    bool              m_bFinished;   // no more data will ever be produced
};

// Reads from iosInput and presents the data with the change table applied.
// If the table cannot be loaded the stream is simply empty (as before).
class Change_istream : public std::istream
{
public:
    Change_istream(const char* pszChangeTablePath, std::istream& iosInput)
        : std::istream(nullptr),
        m_buf(pszChangeTablePath, iosInput)
    {
        init(&m_buf);
    }

    ~Change_istream() override = default;

private:
    Change_istreambuf m_buf;
};