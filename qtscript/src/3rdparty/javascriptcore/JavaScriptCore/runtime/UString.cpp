/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Cameron Zwarich (cwzwarich@uwaterloo.ca)
 *  Copyright (C) 2009 Google Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "UString.h"

#include "JSGlobalObjectFunctions.h"
#include "Collector.h"
#include "dtoa.h"
#include "Identifier.h"
#include "Operations.h"
#include <ctype.h>
#include <limits.h>
#include <limits>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wtf/ASCIICType.h>
#include <wtf/Assertions.h>
#include <wtf/MathExtras.h>
#include <wtf/StringExtras.h>
#include <wtf/Vector.h>
#include <wtf/unicode/UTF8.h>
#include <wtf/StringExtras.h>

#if HAVE(STRINGS_H)
#include <strings.h>
#endif

using namespace WTF;
using namespace WTF::Unicode;
using namespace std;

namespace JSC {
 
extern const double NaN;
extern const double Inf;

CString::CString(const char* c)
    : m_length(strlen(c))
    , m_data(new char[m_length + 1])
{
    memcpy(m_data, c, m_length + 1);
}

CString::CString(const char* c, size_t length)
    : m_length(length)
    , m_data(new char[length + 1])
{
    memcpy(m_data, c, m_length);
    m_data[m_length] = 0;
}

CString::CString(const CString& b)
{
    m_length = b.m_length;
    if (b.m_data) {
        m_data = new char[m_length + 1];
        memcpy(m_data, b.m_data, m_length + 1);
    } else
        m_data = 0;
}

CString::~CString()
{
    delete [] m_data;
}

CString CString::adopt(char* c, size_t length)
{
    CString s;
    s.m_data = c;
    s.m_length = length;
    return s;
}

CString& CString::append(const CString& t)
{
    char* n;
    n = new char[m_length + t.m_length + 1];
    if (m_length)
        memcpy(n, m_data, m_length);
    if (t.m_length)
        memcpy(n + m_length, t.m_data, t.m_length);
    m_length += t.m_length;
    n[m_length] = 0;

    delete [] m_data;
    m_data = n;

    return *this;
}

CString& CString::operator=(const char* c)
{
    if (m_data)
        delete [] m_data;
    m_length = strlen(c);
    m_data = new char[m_length + 1];
    memcpy(m_data, c, m_length + 1);

    return *this;
}

CString& CString::operator=(const CString& str)
{
    if (this == &str)
        return *this;

    if (m_data)
        delete [] m_data;
    m_length = str.m_length;
    if (str.m_data) {
        m_data = new char[m_length + 1];
        memcpy(m_data, str.m_data, m_length + 1);
    } else
        m_data = 0;

    return *this;
}

bool operator==(const CString& c1, const CString& c2)
{
    size_t len = c1.size();
    return len == c2.size() && (len == 0 || memcmp(c1.c_str(), c2.c_str(), len) == 0);
}

// These static strings are immutable, except for rc, whose initial value is chosen to 
// reduce the possibility of it becoming zero due to ref/deref not being thread-safe.
static UChar sharedEmptyChar;
UStringImpl* UStringImpl::s_null;
UStringImpl* UStringImpl::s_empty;
UString* UString::nullUString;

void initializeUString()
{
    UStringImpl::s_null = new UStringImpl(0, 0, UStringImpl::ConstructStaticString);
    UStringImpl::s_empty = new UStringImpl(&sharedEmptyChar, 0, UStringImpl::ConstructStaticString);
    UString::nullUString = new UString;
}

static PassRefPtr<UString::Rep> createRep(const char* c)
{
    if (!c)
        return &UString::Rep::null();

    if (!c[0])
        return &UString::Rep::empty();

    size_t length = strlen(c);
    UChar* d;
    PassRefPtr<UStringImpl> result = UStringImpl::tryCreateUninitialized(length, d);
    if (!result)
        return &UString::Rep::null();

    for (size_t i = 0; i < length; i++)
        d[i] = static_cast<unsigned char>(c[i]); // use unsigned char to zero-extend instead of sign-extend
    return result;
}

static inline PassRefPtr<UString::Rep> createRep(const char* c, int length)
{
    if (!c)
        return &UString::Rep::null();

    if (!length)
        return &UString::Rep::empty();

    UChar* d;
    PassRefPtr<UStringImpl> result = UStringImpl::tryCreateUninitialized(length, d);
    if (!result)
        return &UString::Rep::null();

    for (int i = 0; i < length; i++)
        d[i] = static_cast<unsigned char>(c[i]); // use unsigned char to zero-extend instead of sign-extend
    return result;
}

UString::UString(const char* c)
    : m_rep(createRep(c))
{
}

UString::UString(const char* c, int length)
    : m_rep(createRep(c, length))
{
}

UString::UString(const UChar* c, int length)
{
    if (length == 0) 
        m_rep = &Rep::empty();
    else
        m_rep = Rep::create(c, length);
}

UString UString::createFromUTF8(const char* string)
{
    if (!string)
        return null();

    size_t length = strlen(string);
    Vector<UChar, 1024> buffer(length);
    UChar* p = buffer.data();
    if (conversionOK != convertUTF8ToUTF16(&string, string + length, &p, p + length))
        return null();

    return UString(buffer.data(), p - buffer.data());
}

UString UString::from(int i)
{
    UChar buf[1 + sizeof(i) * 3];
    UChar* end = buf + sizeof(buf) / sizeof(UChar);
    UChar* p = end;
  
    if (i == 0)
        *--p = '0';
    else if (i == INT_MIN) {
        char minBuf[1 + sizeof(i) * 3];
        sprintf(minBuf, "%d", INT_MIN);
        return UString(minBuf);
    } else {
        bool negative = false;
        if (i < 0) {
            negative = true;
            i = -i;
        }
        while (i) {
            *--p = static_cast<unsigned short>((i % 10) + '0');
            i /= 10;
        }
        if (negative)
            *--p = '-';
    }

    return UString(p, static_cast<int>(end - p));
}

UString UString::from(long long i)
{
    UChar buf[1 + sizeof(i) * 3];
    UChar* end = buf + sizeof(buf) / sizeof(UChar);
    UChar* p = end;

    if (i == 0)
        *--p = '0';
    else if (i == std::numeric_limits<long long>::min()) {
        char minBuf[1 + sizeof(i) * 3];
#if OS(WINDOWS)
        snprintf(minBuf, sizeof(minBuf) - 1, "%I64d", std::numeric_limits<long long>::min());
#else
        snprintf(minBuf, sizeof(minBuf) - 1, "%lld", std::numeric_limits<long long>::min());
#endif
        return UString(minBuf);
    } else {
        bool negative = false;
        if (i < 0) {
            negative = true;
            i = -i;
        }
        while (i) {
            *--p = static_cast<unsigned short>((i % 10) + '0');
            i /= 10;
        }
        if (negative)
            *--p = '-';
    }

    return UString(p, static_cast<int>(end - p));
}

UString UString::from(unsigned int u)
{
    UChar buf[sizeof(u) * 3];
    UChar* end = buf + sizeof(buf) / sizeof(UChar);
    UChar* p = end;
    
    if (u == 0)
        *--p = '0';
    else {
        while (u) {
            *--p = static_cast<unsigned short>((u % 10) + '0');
            u /= 10;
        }
    }
    
    return UString(p, static_cast<int>(end - p));
}

UString UString::from(long l)
{
    UChar buf[1 + sizeof(l) * 3];
    UChar* end = buf + sizeof(buf) / sizeof(UChar);
    UChar* p = end;

    if (l == 0)
        *--p = '0';
    else if (l == LONG_MIN) {
        char minBuf[1 + sizeof(l) * 3];
        sprintf(minBuf, "%ld", LONG_MIN);
        return UString(minBuf);
    } else {
        bool negative = false;
        if (l < 0) {
            negative = true;
            l = -l;
        }
        while (l) {
            *--p = static_cast<unsigned short>((l % 10) + '0');
            l /= 10;
        }
        if (negative)
            *--p = '-';
    }

    return UString(p, static_cast<int>(end - p));
}

UString UString::from(double d)
{
    DtoaBuffer buffer;
    unsigned length;
    doubleToStringInJavaScriptFormat(d, buffer, &length);
    return UString(buffer, length);
}

UString UString::spliceSubstringsWithSeparators(const Range* substringRanges, int rangeCount, const UString* separators, int separatorCount) const
{
    m_rep->checkConsistency();

    if (rangeCount == 1 && separatorCount == 0) {
        int thisSize = size();
        int position = substringRanges[0].position;
        int length = substringRanges[0].length;
        if (position <= 0 && length >= thisSize)
            return *this;
        return UString::Rep::create(m_rep, max(0, position), min(thisSize, length));
    }

    int totalLength = 0;
    for (int i = 0; i < rangeCount; i++)
        totalLength += substringRanges[i].length;
    for (int i = 0; i < separatorCount; i++)
        totalLength += separators[i].size();

    if (totalLength == 0)
        return "";

    UChar* buffer;
    PassRefPtr<Rep> rep = Rep::tryCreateUninitialized(totalLength, buffer);
    if (!rep)
        return null();

    int maxCount = max(rangeCount, separatorCount);
    int bufferPos = 0;
    for (int i = 0; i < maxCount; i++) {
        if (i < rangeCount) {
            UStringImpl::copyChars(buffer + bufferPos, data() + substringRanges[i].position, substringRanges[i].length);
            bufferPos += substringRanges[i].length;
        }
        if (i < separatorCount) {
            UStringImpl::copyChars(buffer + bufferPos, separators[i].data(), separators[i].size());
            bufferPos += separators[i].size();
        }
    }

    return rep;
}

UString UString::replaceRange(int rangeStart, int rangeLength, const UString& replacement) const
{
    m_rep->checkConsistency();

    int replacementLength = replacement.size();
    int totalLength = size() - rangeLength + replacementLength;
    if (totalLength == 0)
        return "";

    UChar* buffer;
    PassRefPtr<Rep> rep = Rep::tryCreateUninitialized(totalLength, buffer);
    if (!rep)
        return null();

    UStringImpl::copyChars(buffer, data(), rangeStart);
    UStringImpl::copyChars(buffer + rangeStart, replacement.data(), replacementLength);
    int rangeEnd = rangeStart + rangeLength;
    UStringImpl::copyChars(buffer + rangeStart + replacementLength, data() + rangeEnd, size() - rangeEnd);

    return rep;
}

bool UString::getCString(CStringBuffer& buffer) const
{
    int length = size();
    int neededSize = length + 1;
    buffer.resize(neededSize);
    char* buf = buffer.data();

    UChar ored = 0;
    const UChar* p = data();
    char* q = buf;
    const UChar* limit = p + length;
    while (p != limit) {
        UChar c = p[0];
        ored |= c;
        *q = static_cast<char>(c);
        ++p;
        ++q;
    }
    *q = '\0';

    return !(ored & 0xFF00);
}

char* UString::ascii() const
{
    static char* asciiBuffer = 0;

    int length = size();
    int neededSize = length + 1;
    delete[] asciiBuffer;
    asciiBuffer = new char[neededSize];

    const UChar* p = data();
    char* q = asciiBuffer;
    const UChar* limit = p + length;
    while (p != limit) {
        *q = static_cast<char>(p[0]);
        ++p;
        ++q;
    }
    *q = '\0';

    return asciiBuffer;
}

UString& UString::operator=(const char* c)
{
    if (!c) {
        m_rep = &Rep::null();
        return *this;
    }

    if (!c[0]) {
        m_rep = &Rep::empty();
        return *this;
    }

    int l = static_cast<int>(strlen(c));
    UChar* d = 0;
    m_rep = Rep::tryCreateUninitialized(l, d);
    if (m_rep) {
        for (int i = 0; i < l; i++)
            d[i] = static_cast<unsigned char>(c[i]); // use unsigned char to zero-extend instead of sign-extend
    } else
        makeNull();

    return *this;
}

bool UString::is8Bit() const
{
    const UChar* u = data();
    const UChar* limit = u + size();
    while (u < limit) {
        if (u[0] > 0xFF)
            return false;
        ++u;
    }

    return true;
}

UChar UString::operator[](int pos) const
{
    if (pos >= size())
        return '\0';
    return data()[pos];
}

double UString::toDouble(bool tolerateTrailingJunk, bool tolerateEmptyString) const
{
    if (size() == 1) {
        UChar c = data()[0];
        if (isASCIIDigit(c))
            return c - '0';
        if (isASCIISpace(c) && tolerateEmptyString)
            return 0;
        return NaN;
    }

    // FIXME: If tolerateTrailingJunk is true, then we want to tolerate non-8-bit junk
    // after the number, so this is too strict a check.
    CStringBuffer s;
    if (!getCString(s))
        return NaN;
    const char* c = s.data();

    // skip leading white space
    while (isASCIISpace(*c))
        c++;

    // empty string ?
    if (*c == '\0')
        return tolerateEmptyString ? 0.0 : NaN;

    double d;

    // hex number ?
    if (*c == '0' && (*(c + 1) == 'x' || *(c + 1) == 'X')) {
        const char* firstDigitPosition = c + 2;
        c++;
        d = 0.0;
        while (*(++c)) {
            if (*c >= '0' && *c <= '9')
                d = d * 16.0 + *c - '0';
            else if ((*c >= 'A' && *c <= 'F') || (*c >= 'a' && *c <= 'f'))
                d = d * 16.0 + (*c & 0xdf) - 'A' + 10.0;
            else
                break;
        }

        if (d >= mantissaOverflowLowerBound)
            d = parseIntOverflow(firstDigitPosition, c - firstDigitPosition, 16);
    } else {
        // regular number ?
        char* end;
        d = WTF::strtod(c, &end);
        if ((d != 0.0 || end != c) && d != Inf && d != -Inf) {
            c = end;
        } else {
            double sign = 1.0;

            if (*c == '+')
                c++;
            else if (*c == '-') {
                sign = -1.0;
                c++;
            }

            // We used strtod() to do the conversion. However, strtod() handles
            // infinite values slightly differently than JavaScript in that it
            // converts the string "inf" with any capitalization to infinity,
            // whereas the ECMA spec requires that it be converted to NaN.

            if (c[0] == 'I' && c[1] == 'n' && c[2] == 'f' && c[3] == 'i' && c[4] == 'n' && c[5] == 'i' && c[6] == 't' && c[7] == 'y') {
                d = sign * Inf;
                c += 8;
            } else if ((d == Inf || d == -Inf) && *c != 'I' && *c != 'i')
                c = end;
            else
                return NaN;
        }
    }

    // allow trailing white space
    while (isASCIISpace(*c))
        c++;
    // don't allow anything after - unless tolerant=true
    if (!tolerateTrailingJunk && *c != '\0')
        d = NaN;

    return d;
}

double UString::toDouble(bool tolerateTrailingJunk) const
{
    return toDouble(tolerateTrailingJunk, true);
}

double UString::toDouble() const
{
    return toDouble(false, true);
}

uint32_t UString::toUInt32(bool* ok) const
{
    double d = toDouble();
    bool b = true;

    if (d != static_cast<uint32_t>(d)) {
        b = false;
        d = 0;
    }

    if (ok)
        *ok = b;

    return static_cast<uint32_t>(d);
}

uint32_t UString::toUInt32(bool* ok, bool tolerateEmptyString) const
{
    double d = toDouble(false, tolerateEmptyString);
    bool b = true;

    if (d != static_cast<uint32_t>(d)) {
        b = false;
        d = 0;
    }

    if (ok)
        *ok = b;

    return static_cast<uint32_t>(d);
}

uint32_t UString::toStrictUInt32(bool* ok) const
{
    if (ok)
        *ok = false;

    // Empty string is not OK.
    int len = m_rep->size();
    if (len == 0)
        return 0;
    const UChar* p = m_rep->data();
    unsigned short c = p[0];

    // If the first digit is 0, only 0 itself is OK.
    if (c == '0') {
        if (len == 1 && ok)
            *ok = true;
        return 0;
    }

    // Convert to UInt32, checking for overflow.
    uint32_t i = 0;
    while (1) {
        // Process character, turning it into a digit.
        if (c < '0' || c > '9')
            return 0;
        const unsigned d = c - '0';

        // Multiply by 10, checking for overflow out of 32 bits.
        if (i > 0xFFFFFFFFU / 10)
            return 0;
        i *= 10;

        // Add in the digit, checking for overflow out of 32 bits.
        const unsigned max = 0xFFFFFFFFU - d;
        if (i > max)
            return 0;
        i += d;

        // Handle end of string.
        if (--len == 0) {
            if (ok)
                *ok = true;
            return i;
        }

        // Get next character.
        c = *(++p);
    }
}

int UString::find(const UString& f, int pos) const
{
    int fsz = f.size();

    if (pos < 0)
        pos = 0;

    if (fsz == 1) {
        UChar ch = f[0];
        const UChar* end = data() + size();
        for (const UChar* c = data() + pos; c < end; c++) {
            if (*c == ch)
                return static_cast<int>(c - data());
        }
        return -1;
    }

    int sz = size();
    if (sz < fsz)
        return -1;
    if (fsz == 0)
        return pos;
    const UChar* end = data() + sz - fsz;
    int fsizeminusone = (fsz - 1) * sizeof(UChar);
    const UChar* fdata = f.data();
    unsigned short fchar = fdata[0];
    ++fdata;
    for (const UChar* c = data() + pos; c <= end; c++) {
        if (c[0] == fchar && !memcmp(c + 1, fdata, fsizeminusone))
            return static_cast<int>(c - data());
    }

    return -1;
}

int UString::find(UChar ch, int pos) const
{
    if (pos < 0)
        pos = 0;
    const UChar* end = data() + size();
    for (const UChar* c = data() + pos; c < end; c++) {
        if (*c == ch)
            return static_cast<int>(c - data());
    }
    
    return -1;
}

int UString::rfind(const UString& f, int pos) const
{
    int sz = size();
    int fsz = f.size();
    if (sz < fsz)
        return -1;
    if (pos < 0)
        pos = 0;
    if (pos > sz - fsz)
        pos = sz - fsz;
    if (fsz == 0)
        return pos;
    int fsizeminusone = (fsz - 1) * sizeof(UChar);
    const UChar* fdata = f.data();
    for (const UChar* c = data() + pos; c >= data(); c--) {
        if (*c == *fdata && !memcmp(c + 1, fdata + 1, fsizeminusone))
            return static_cast<int>(c - data());
    }

    return -1;
}

int UString::rfind(UChar ch, int pos) const
{
    if (isEmpty())
        return -1;
    if (pos + 1 >= size())
        pos = size() - 1;
    for (const UChar* c = data() + pos; c >= data(); c--) {
        if (*c == ch)
            return static_cast<int>(c - data());
    }

    return -1;
}

UString UString::substr(int pos, int len) const
{
    int s = size();

    if (pos < 0)
        pos = 0;
    else if (pos >= s)
        pos = s;
    if (len < 0)
        len = s;
    if (pos + len >= s)
        len = s - pos;

    if (pos == 0 && len == s)
        return *this;

    return UString(Rep::create(m_rep, pos, len));
}

bool operator==(const UString& s1, const char *s2)
{
    if (s2 == 0)
        return s1.isEmpty();

    const UChar* u = s1.data();
    const UChar* uend = u + s1.size();
    while (u != uend && *s2) {
        if (u[0] != (unsigned char)*s2)
            return false;
        s2++;
        u++;
    }

    return u == uend && *s2 == 0;
}

bool operator<(const UString& s1, const UString& s2)
{
    const int l1 = s1.size();
    const int l2 = s2.size();
    const int lmin = l1 < l2 ? l1 : l2;
    const UChar* c1 = s1.data();
    const UChar* c2 = s2.data();
    int l = 0;
    while (l < lmin && *c1 == *c2) {
        c1++;
        c2++;
        l++;
    }
    if (l < lmin)
        return (c1[0] < c2[0]);

    return (l1 < l2);
}

bool operator>(const UString& s1, const UString& s2)
{
    const int l1 = s1.size();
    const int l2 = s2.size();
    const int lmin = l1 < l2 ? l1 : l2;
    const UChar* c1 = s1.data();
    const UChar* c2 = s2.data();
    int l = 0;
    while (l < lmin && *c1 == *c2) {
        c1++;
        c2++;
        l++;
    }
    if (l < lmin)
        return (c1[0] > c2[0]);

    return (l1 > l2);
}

int compare(const UString& s1, const UString& s2)
{
    const int l1 = s1.size();
    const int l2 = s2.size();
    const int lmin = l1 < l2 ? l1 : l2;
    const UChar* c1 = s1.data();
    const UChar* c2 = s2.data();
    int l = 0;
    while (l < lmin && *c1 == *c2) {
        c1++;
        c2++;
        l++;
    }

    if (l < lmin)
        return (c1[0] > c2[0]) ? 1 : -1;

    if (l1 == l2)
        return 0;

    return (l1 > l2) ? 1 : -1;
}

#if OS(SOLARIS) && COMPILER(SUNCC)
// Signature must match that of UStringImpl.h, otherwise the linker complains about undefined symbol.
bool equal(const UStringImpl* r, const UStringImpl* b)
#else
bool equal(const UString::Rep* r, const UString::Rep* b)
#endif
{
    int length = r->size();
    if (length != b->size())
        return false;
    const UChar* d = r->data();
    const UChar* s = b->data();
    for (int i = 0; i != length; ++i) {
        if (d[i] != s[i])
            return false;
    }
    return true;
}

CString UString::UTF8String(bool strict) const
{
    // Allocate a buffer big enough to hold all the characters.
    const int length = size();
    Vector<char, 1024> buffer(length * 3);

    // Convert to runs of 8-bit characters.
    char* p = buffer.data();
    const UChar* d = reinterpret_cast<const UChar*>(&data()[0]);
    ConversionResult result = convertUTF16ToUTF8(&d, d + length, &p, p + buffer.size(), strict);
    if (result != conversionOK)
        return CString();

    return CString(buffer.data(), p - buffer.data());
}

// For use in error handling code paths -- having this not be inlined helps avoid PIC branches to fetch the global on Mac OS X.
NEVER_INLINE void UString::makeNull()
{
    m_rep = &Rep::null();
}

// For use in error handling code paths -- having this not be inlined helps avoid PIC branches to fetch the global on Mac OS X.
NEVER_INLINE UString::Rep* UString::nullRep()
{
    return &Rep::null();
}

} // namespace JSC
