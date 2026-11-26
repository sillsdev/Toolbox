#include "Change_ostream.h"
#include <string>

namespace
{
    const size_t kOutBuf = 4096;
}

Change_ostreambuf::Change_ostreambuf(const char* pszChangeTablePath, std::ostream& iosOutput)
    : m_iosOutput(iosOutput),
    m_buf(kOutBuf + 1),
    m_bFinished(false),
    m_bFailed(false)
{
    // The extra byte lets overflow() store the character it was handed.
    setp(m_buf.data(), m_buf.data() + kOutBuf);

    m_cct.bLoadFromFile(pszChangeTablePath);    // if it fails: pass through
}

Change_ostreambuf::~Change_ostreambuf()
{
    finish();
}

void Change_ostreambuf::finish()
{
    if (m_bFinished)
        return;
    try
    {
        bPushPending(true);
        m_iosOutput.flush();
    }
    catch (...)
    {
        // never throw from a destructor path
    }
}

// Hand whatever is in the put area to CC and write what comes back.
bool Change_ostreambuf::bPushPending(bool bLast)
{
    const char* p = pbase();
    const int len = static_cast<int>(pptr() - pbase());
    bool ok = true;

    if (len == 0 && !bLast)
        return !m_iosOutput.fail();

    if (m_bFinished)
    {
        ok = (len == 0);                // data after finish() is an error
    }
    else if (!m_cct.bLoaded() || m_bFailed)
    {
        if (len > 0)
            m_iosOutput.write(p, len);
        if (bLast)
            m_bFinished = true;
    }
    else
    {
        std::string out;
        if (m_cct.bStreamChunk(p, len, bLast, out))
        {
            if (!out.empty())
                m_iosOutput.write(out.data(), static_cast<std::streamsize>(out.size()));
            if (bLast)
                m_bFinished = true;
        }
        else
        {
            // CC failed: do not lose the data, pass the rest through
            // untransformed and report the error to the stream.
            m_bFailed = true;
            ok = false;
            if (len > 0)
                m_iosOutput.write(p, len);
        }
    }

    pbump(-len);
    return ok && !m_iosOutput.fail();
}

int Change_ostreambuf::sync()
{
    if (!bPushPending(false))
        return -1;
    m_iosOutput.flush();
    return m_iosOutput.fail() ? -1 : 0;
}

int Change_ostreambuf::overflow(int ch)
{
    typedef std::char_traits<char> traits;

    if (ch != traits::eof())
    {
        *pptr() = static_cast<char>(ch);    // uses the spare byte
        pbump(1);
    }

    if (!bPushPending(false))
        return traits::eof();

    return traits::not_eof(ch);
}