#include "Change_istream.h"

namespace
{
    const size_t kInChunk = 8192;
}

Change_istreambuf::Change_istreambuf(const char* pszChangeTablePath, std::istream& iosInput)
    : m_iosInput(iosInput),
    m_in(kInChunk),
    m_bFinished(false)
{
    setg(nullptr, nullptr, nullptr);

    if (!m_cct.bLoadFromFile(pszChangeTablePath))
        m_bFinished = true;     // could not load the table: behave as empty
}

int Change_istreambuf::underflow()
{
    typedef std::char_traits<char> traits;

    if (gptr() && gptr() < egptr())
        return traits::to_int_type(*gptr());

    m_out.clear();

    // CC may legitimately produce nothing for a chunk (it holds text back, or
    // the table deletes it), so keep feeding it until there is output or the
    // stream is finished.
    while (m_out.empty() && !m_bFinished)
    {
        m_iosInput.read(m_in.data(), static_cast<std::streamsize>(m_in.size()));
        const int len = static_cast<int>(m_iosInput.gcount());

        // A short read means end of input (or a failed source stream): this is
        // the last chunk, so CC will release anything it was holding back.
        const bool bLast = (len < static_cast<int>(m_in.size()));

        if (!m_cct.bStreamChunk(m_in.data(), len, bLast, m_out))
            m_bFinished = true;         // CC error: end the stream
        else if (bLast)
            m_bFinished = true;         // bStreamChunk drained everything
    }

    if (m_out.empty())
    {
        setg(nullptr, nullptr, nullptr);
        return traits::eof();
    }

    setg(&m_out[0], &m_out[0], &m_out[0] + m_out.size());
    return traits::to_int_type(*gptr());
}