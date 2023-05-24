// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "utils.h"

#include <sstream>
#include <string_view>

#include <cctype>
#include <cassert>
#include <regex>

namespace {
const std::string_view asciiSpacing = " \t\n\r\f\v";
}

namespace qtprotoccommon::utils {

bool isAsciiAlpha(char c)
{
    return (unsigned char)c <= 127 && ::isalpha(c);
}

std::vector<std::string> split(std::string_view s, std::string_view c, bool keepEmpty)
{
    assert(!c.empty());
    std::vector<std::string> out;
    std::string::size_type pos = 0;
    std::string item;
    for (std::string::size_type posNext = 0; (posNext = s.find(c, pos)) != std::string::npos;
         pos = posNext + c.size()) {
        item = s.substr(pos, posNext - pos);
        if (keepEmpty || !item.empty())
            out.push_back(item);
    }
    item = s.substr(pos);
    if (keepEmpty || !item.empty())
        out.push_back(item);
    return out;
}

std::string replace(std::string_view where, std::string_view from, std::string_view to)
{
    assert(!from.empty());
    if (from == to)
        return std::string(where);

    std::string out;
    out.reserve(where.size());
    std::size_t pos = 0;
    for (std::string::size_type posNext = 0; (posNext = where.find(from, pos)) != std::string::npos;
         pos = posNext + from.size()) {
        out += where.substr(pos, posNext - pos);
        out += to;
    }
    out += where.substr(pos);

    return out;
}

void asciiToLower(std::string &str)
{
    const auto toLower = [](char c) {
        if (!isAsciiAlpha(c))
            return c;
        return char(c | char(0x20));
    };
    std::transform(std::begin(str), std::end(str), std::begin(str), toLower);
}

std::string removeFileSuffix(std::string fileName)
{
    size_t dot = fileName.rfind('.'), slash = fileName.rfind('/');
    if (dot != std::string::npos && (slash == std::string::npos || dot > slash))
        fileName.resize(dot);
    return fileName;
}

std::string extractFileBasename(std::string fileName)
{
    size_t dot = fileName.rfind('.'), slash = fileName.rfind('/');
    if (dot != std::string::npos && (slash == std::string::npos || dot > slash))
        fileName.resize(dot);
    return slash != std::string::npos ? fileName.substr(slash + 1) : fileName;
}

std::string capitalizeAsciiName(std::string name)
{
    if (name.empty() || !isAsciiAlpha(name[0]))
        return name;
    name[0] &= ~char(0x20);
    return name;
}

std::string deCapitalizeAsciiName(std::string name)
{
    if (name.empty() || !isAsciiAlpha(name[0]))
        return name;
    name[0] |= char(0x20);
    return name;
}

std::string escapedQmlUri(const std::string &uri)
{
    assert(!uri.empty());
    static std::regex uriExceptionsRegex("[^a-zA-Z0-9]");
    return std::regex_replace(uri, uriExceptionsRegex, "_");
}

std::string &rtrim(std::string &s)
{
    const size_t cut = s.find_last_not_of(asciiSpacing);
    s.erase(cut != std::string::npos ? cut + 1 : 0);
    return s;
}

std::string &ltrim(std::string &s)
{
    const size_t cut = s.find_first_not_of(asciiSpacing);
    s.erase(0, cut == std::string::npos ? s.size() - 1 : cut);
    return s;
}

std::string &trim(std::string &s)
{
    const size_t lastKept = s.find_last_not_of(asciiSpacing);
    if (lastKept == std::string::npos) { // true, in particular, if empty
        s.erase(0);
        return s;
    }
    const size_t firstKept = s.find_first_not_of(asciiSpacing);
    assert(firstKept != std::string::npos);
    assert(firstKept <= lastKept);
    s = s.substr(firstKept, lastKept + 1 - firstKept);
    return s;
}

// TODO C++20: use the std::string(_view) methods directly
template<typename T>
bool containsImpl(std::string_view s, T c)
{
    return s.find(c) != std::string::npos;
}

// TODO C++20: use the std::string(_view) methods directly
bool contains(std::string_view s, char c)
{
    return containsImpl(s, c);
}

// TODO C++20: use the std::string(_view) methods directly
bool contains(std::string_view s, std::string_view c)
{
    return containsImpl(s, c);
}

// TODO C++20: use the std::string(_view) methods directly
bool endsWith(std::string_view s, char c)
{
    return !s.empty() && s.back() == c;
}

// TODO C++20: use the std::string(_view) methods directly
bool endsWith(std::string_view s, std::string_view c)
{
    return s.size() >= c.size() && s.compare(s.size() - c.size(), c.size(), c) == 0;
}

// TODO C++20: use the std::string(_view) methods directly
bool startsWith(std::string_view s, char c)
{
    return !s.empty() && s.front() == c;
}

// TODO C++20: use the std::string(_view) methods directly
bool startsWith(std::string_view s, std::string_view c)
{
    return s.size() >= c.size() && s.compare(0, c.size(), c) == 0;
}

size_t count(std::string_view s, char c)
{
    return std::count(s.begin(), s.end(), c);
}
} // namespace qtprotoccommon::utils
