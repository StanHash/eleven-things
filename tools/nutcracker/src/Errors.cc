#include "Errors.hh"

#include <cstdarg>
#include <cstdio>

// ************************************************************************************************************************************
Error::Error(const Error & r) : m_what(r.m_what)
{
}

// ************************************************************************************************************************************
Error::Error(const char * format, ...)
{
    char buffer[800];

    va_list args;
    va_start(args, format);

    vsprintf(buffer, format, args);

    va_end(args);

    m_what = buffer;
}

// ************************************************************************************************************************************
const char * Error::what() const noexcept
{
    return m_what.c_str();
}
