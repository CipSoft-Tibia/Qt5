// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file defines utility functions for working with strings.

#ifndef BASE_STRINGS_STRING_UTIL_H_
#define BASE_STRINGS_STRING_UTIL_H_

#include <ctype.h>
#include <stdarg.h>  // va_list
#include <stddef.h>
#include <stdint.h>

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include "base/compiler_specific.h"
#include "util/build_config.h"

namespace base {

// C standard-library functions that aren't cross-platform are provided as
// "base::...", and their prototypes are listed below. These functions are
// then implemented as inline calls to the platform-specific equivalents in the
// platform-specific headers.

// Wrapper for vsnprintf that always null-terminates and always returns the
// number of characters that would be in an untruncated formatted
// string, even when truncation occurs.
int vsnprintf(char* buffer, size_t size, const char* format, va_list arguments)
    PRINTF_FORMAT(3, 0);

// Some of these implementations need to be inlined.

// We separate the declaration from the implementation of this inline
// function just so the PRINTF_FORMAT works.
inline int snprintf(char* buffer,
                    size_t size,
                    _Printf_format_string_ const char* format,
                    ...) PRINTF_FORMAT(3, 4);
inline int snprintf(char* buffer,
                    size_t size,
                    _Printf_format_string_ const char* format,
                    ...) {
  va_list arguments;
  va_start(arguments, format);
  int result = vsnprintf(buffer, size, format, arguments);
  va_end(arguments);
  return result;
}

// ASCII-specific tolower.  The standard library's tolower is locale sensitive,
// so we don't want to use it here.
inline char ToLowerASCII(char c) {
  return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}
inline char16_t ToLowerASCII(char16_t c) {
  return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}

// ASCII-specific toupper.  The standard library's toupper is locale sensitive,
// so we don't want to use it here.
inline char ToUpperASCII(char c) {
  return (c >= 'a' && c <= 'z') ? (c + ('A' - 'a')) : c;
}
inline char16_t ToUpperASCII(char16_t c) {
  return (c >= 'a' && c <= 'z') ? (c + ('A' - 'a')) : c;
}

// Converts the given string to it's ASCII-lowercase equivalent.
std::string ToLowerASCII(std::string_view str);
std::u16string ToLowerASCII(std::u16string_view str);

// Converts the given string to it's ASCII-uppercase equivalent.
std::string ToUpperASCII(std::string_view str);
std::u16string ToUpperASCII(std::u16string_view str);

// Functor for case-insensitive ASCII comparisons for STL algorithms like
// std::search.
//
// Note that a full Unicode version of this functor is not possible to write
// because case mappings might change the number of characters, depend on
// context (combining accents), and require handling UTF-16. If you need
// proper Unicode support, use base::i18n::ToLower/FoldCase and then just
// use a normal operator== on the result.
template <typename Char>
struct CaseInsensitiveCompareASCII {
 public:
  bool operator()(Char x, Char y) const {
    return ToLowerASCII(x) == ToLowerASCII(y);
  }
};

// Like strcasecmp for case-insensitive ASCII characters only. Returns:
//   -1  (a < b)
//    0  (a == b)
//    1  (a > b)
// (unlike strcasecmp which can return values greater or less than 1/-1). For
// full Unicode support, use base::i18n::ToLower or base::i18h::FoldCase
// and then just call the normal string operators on the result.
int CompareCaseInsensitiveASCII(std::string_view a, std::string_view b);
int CompareCaseInsensitiveASCII(std::u16string_view a, std::u16string_view b);

// Equality for ASCII case-insensitive comparisons. For full Unicode support,
// use base::i18n::ToLower or base::i18h::FoldCase and then compare with either
// == or !=.
bool EqualsCaseInsensitiveASCII(std::string_view a, std::string_view b);
bool EqualsCaseInsensitiveASCII(std::u16string_view a, std::u16string_view b);

// Contains the set of characters representing whitespace in the corresponding
// encoding. Null-terminated. The ASCII versions are the whitespaces as defined
// by HTML5, and don't include control characters.
extern const char16_t kWhitespaceUTF16[];  // Includes Unicode.
extern const char kWhitespaceASCII[];
extern const char16_t kWhitespaceASCIIAs16[];  // No unicode.

// Null-terminated string representing the UTF-8 byte order mark.
extern const char kUtf8ByteOrderMark[];

// Removes characters in |remove_chars| from anywhere in |input|.  Returns true
// if any characters were removed.  |remove_chars| must be null-terminated.
// NOTE: Safe to use the same variable for both |input| and |output|.
bool RemoveChars(const std::u16string& input,
                 std::u16string_view remove_chars,
                 std::u16string* output);
bool RemoveChars(const std::string& input,
                 std::string_view remove_chars,
                 std::string* output);

// Replaces characters in |replace_chars| from anywhere in |input| with
// |replace_with|.  Each character in |replace_chars| will be replaced with
// the |replace_with| string.  Returns true if any characters were replaced.
// |replace_chars| must be null-terminated.
// NOTE: Safe to use the same variable for both |input| and |output|.
bool ReplaceChars(const std::u16string& input,
                  std::u16string_view replace_chars,
                  const std::u16string& replace_with,
                  std::u16string* output);
bool ReplaceChars(const std::string& input,
                  std::string_view replace_chars,
                  const std::string& replace_with,
                  std::string* output);

enum TrimPositions {
  TRIM_NONE = 0,
  TRIM_LEADING = 1 << 0,
  TRIM_TRAILING = 1 << 1,
  TRIM_ALL = TRIM_LEADING | TRIM_TRAILING,
};

// Removes characters in |trim_chars| from the beginning and end of |input|.
// The 8-bit version only works on 8-bit characters, not UTF-8. Returns true if
// any characters were removed.
//
// It is safe to use the same variable for both |input| and |output| (this is
// the normal usage to trim in-place).
bool TrimString(const std::u16string& input,
                std::u16string_view trim_chars,
                std::u16string* output);
bool TrimString(const std::string& input,
                std::string_view trim_chars,
                std::string* output);

// std::string_view versions of the above. The returned pieces refer to the
// original buffer.
std::u16string_view TrimString(std::u16string_view input,
                               std::u16string_view trim_chars,
                               TrimPositions positions);
std::string_view TrimString(std::string_view input,
                            std::string_view trim_chars,
                            TrimPositions positions);

// Truncates a string to the nearest UTF-8 character that will leave
// the string less than or equal to the specified byte size.
void TruncateUTF8ToByteSize(const std::string& input,
                            const size_t byte_size,
                            std::string* output);

// Trims any whitespace from either end of the input string.
//
// The std::string_view versions return a substring referencing the input
// buffer. The ASCII versions look only for ASCII whitespace.
//
// The std::string versions return where whitespace was found.
// NOTE: Safe to use the same variable for both input and output.
TrimPositions TrimWhitespace(const std::u16string& input,
                             TrimPositions positions,
                             std::u16string* output);
std::u16string_view TrimWhitespace(std::u16string_view input,
                                   TrimPositions positions);
TrimPositions TrimWhitespaceASCII(const std::string& input,
                                  TrimPositions positions,
                                  std::string* output);
std::string_view TrimWhitespaceASCII(std::string_view input,
                                     TrimPositions positions);

// Searches for CR or LF characters.  Removes all contiguous whitespace
// strings that contain them.  This is useful when trying to deal with text
// copied from terminals.
// Returns |text|, with the following three transformations:
// (1) Leading and trailing whitespace is trimmed.
// (2) If |trim_sequences_with_line_breaks| is true, any other whitespace
//     sequences containing a CR or LF are trimmed.
// (3) All other whitespace sequences are converted to single spaces.
std::u16string CollapseWhitespace(const std::u16string& text,
                                  bool trim_sequences_with_line_breaks);
std::string CollapseWhitespaceASCII(const std::string& text,
                                    bool trim_sequences_with_line_breaks);

// Returns true if |input| is empty or contains only characters found in
// |characters|.
bool ContainsOnlyChars(std::string_view input, std::string_view characters);
bool ContainsOnlyChars(std::u16string_view input,
                       std::u16string_view characters);

// Returns true if the specified string matches the criteria. How can a wide
// string be 8-bit or UTF8? It contains only characters that are < 256 (in the
// first case) or characters that use only 8-bits and whose 8-bit
// representation looks like a UTF-8 string (the second case).
//
// Note that IsStringUTF8 checks not only if the input is structurally
// valid but also if it doesn't contain any non-character codepoint
// (e.g. U+FFFE). It's done on purpose because all the existing callers want
// to have the maximum 'discriminating' power from other encodings. If
// there's a use case for just checking the structural validity, we have to
// add a new function for that.
//
// IsStringASCII assumes the input is likely all ASCII, and does not leave early
// if it is not the case.
bool IsStringUTF8(std::string_view str);
bool IsStringASCII(std::string_view str);
bool IsStringASCII(std::u16string_view str);

// Compare the lower-case form of the given string against the given
// previously-lower-cased ASCII string (typically a constant).
bool LowerCaseEqualsASCII(std::string_view str,
                          std::string_view lowecase_ascii);
bool LowerCaseEqualsASCII(std::u16string_view str,
                          std::string_view lowecase_ascii);

// Performs a case-sensitive string compare of the given 16-bit string against
// the given 8-bit ASCII string (typically a constant). The behavior is
// undefined if the |ascii| string is not ASCII.
bool EqualsASCII(std::u16string_view str, std::string_view ascii);

// Indicates case sensitivity of comparisons. Only ASCII case insensitivity
// is supported. Full Unicode case-insensitive conversions would need to go in
// base/i18n so it can use ICU.
//
// If you need to do Unicode-aware case-insensitive StartsWith/EndsWith, it's
// best to call base::i18n::ToLower() or base::i18n::FoldCase() (see
// base/i18n/case_conversion.h for usage advice) on the arguments, and then use
// the results to a case-sensitive comparison.
enum class CompareCase {
  SENSITIVE,
  INSENSITIVE_ASCII,
};

bool StartsWith(std::string_view str,
                std::string_view search_for,
                CompareCase case_sensitivity);
bool StartsWith(std::u16string_view str,
                std::u16string_view search_for,
                CompareCase case_sensitivity);
bool EndsWith(std::string_view str,
              std::string_view search_for,
              CompareCase case_sensitivity);
bool EndsWith(std::u16string_view str,
              std::u16string_view search_for,
              CompareCase case_sensitivity);

// Determines the type of ASCII character, independent of locale (the C
// library versions will change based on locale).
template <typename Char>
inline bool IsAsciiWhitespace(Char c) {
  return c == ' ' || c == '\r' || c == '\n' || c == '\t';
}
template <typename Char>
inline bool IsAsciiAlpha(Char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
template <typename Char>
inline bool IsAsciiUpper(Char c) {
  return c >= 'A' && c <= 'Z';
}
template <typename Char>
inline bool IsAsciiLower(Char c) {
  return c >= 'a' && c <= 'z';
}
template <typename Char>
inline bool IsAsciiDigit(Char c) {
  return c >= '0' && c <= '9';
}

template <typename Char>
inline bool IsHexDigit(Char c) {
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
         (c >= 'a' && c <= 'f');
}

// Returns the integer corresponding to the given hex character. For example:
//    '4' -> 4
//    'a' -> 10
//    'B' -> 11
// Assumes the input is a valid hex character. DCHECKs in debug builds if not.
char HexDigitToInt(char16_t c);

// Returns true if it's a Unicode whitespace character.
bool IsUnicodeWhitespace(char16_t c);

// Return a byte string in human-readable format with a unit suffix. Not
// appropriate for use in any UI; use of FormatBytes and friends in ui/base is
// highly recommended instead. TODO(avi): Figure out how to get callers to use
// FormatBytes instead; remove this.
std::u16string FormatBytesUnlocalized(int64_t bytes);

// Starting at |start_offset| (usually 0), replace the first instance of
// |find_this| with |replace_with|.
void ReplaceFirstSubstringAfterOffset(std::u16string* str,
                                      size_t start_offset,
                                      std::u16string_view find_this,
                                      std::u16string_view replace_with);
void ReplaceFirstSubstringAfterOffset(std::string* str,
                                      size_t start_offset,
                                      std::string_view find_this,
                                      std::string_view replace_with);

// Starting at |start_offset| (usually 0), look through |str| and replace all
// instances of |find_this| with |replace_with|.
//
// This does entire substrings; use std::replace in <algorithm> for single
// characters, for example:
//   std::replace(str.begin(), str.end(), 'a', 'b');
void ReplaceSubstringsAfterOffset(std::u16string* str,
                                  size_t start_offset,
                                  std::u16string_view find_this,
                                  std::u16string_view replace_with);
void ReplaceSubstringsAfterOffset(std::string* str,
                                  size_t start_offset,
                                  std::string_view find_this,
                                  std::string_view replace_with);

// Reserves enough memory in |str| to accommodate |length_with_null| characters,
// sets the size of |str| to |length_with_null - 1| characters, and returns a
// pointer to the underlying contiguous array of characters.  This is typically
// used when calling a function that writes results into a character array, but
// the caller wants the data to be managed by a string-like object.  It is
// convenient in that is can be used inline in the call, and fast in that it
// avoids copying the results of the call from a char* into a string.
//
// |length_with_null| must be at least 2, since otherwise the underlying string
// would have size 0, and trying to access &((*str)[0]) in that case can result
// in a number of problems.
//
// Internally, this takes linear time because the resize() call 0-fills the
// underlying array for potentially all
// (|length_with_null - 1| * sizeof(string_type::value_type)) bytes.  Ideally we
// could avoid this aspect of the resize() call, as we expect the caller to
// immediately write over this memory, but there is no other way to set the size
// of the string, and not doing that will mean people who access |str| rather
// than str.c_str() will get back a string of whatever size |str| had on entry
// to this function (probably 0).
char* WriteInto(std::string* str, size_t length_with_null);
char16_t* WriteInto(std::u16string* str, size_t length_with_null);

// Does the opposite of SplitString()/SplitStringPiece(). Joins a vector or list
// of strings into a single string, inserting |separator| (which may be empty)
// in between all elements.
//
// If possible, callers should build a vector of std::string_views and use the
// std::string_view variant, so that they do not create unnecessary copies of
// strings. For example, instead of using SplitString, modifying the vector,
// then using JoinString, use SplitStringPiece followed by JoinString so that no
// copies of those strings are created until the final join operation.
//
// Use StrCat (in base/strings/strcat.h) if you don't need a separator.
std::string JoinString(const std::vector<std::string>& parts,
                       std::string_view separator);
std::u16string JoinString(const std::vector<std::u16string>& parts,
                          std::u16string_view separator);
std::string JoinString(const std::vector<std::string_view>& parts,
                       std::string_view separator);
std::u16string JoinString(const std::vector<std::u16string_view>& parts,
                          std::u16string_view separator);
// Explicit initializer_list overloads are required to break ambiguity when used
// with a literal initializer list (otherwise the compiler would not be able to
// decide between the string and std::string_view overloads).
std::string JoinString(std::initializer_list<std::string_view> parts,
                       std::string_view separator);
std::u16string JoinString(std::initializer_list<std::u16string_view> parts,
                          std::u16string_view separator);

// Replace $1-$2-$3..$9 in the format string with values from |subst|.
// Additionally, any number of consecutive '$' characters is replaced by that
// number less one. Eg $$->$, $$$->$$, etc. The offsets parameter here can be
// NULL. This only allows you to use up to nine replacements.
std::u16string ReplaceStringPlaceholders(
    const std::u16string& format_string,
    const std::vector<std::u16string>& subst,
    std::vector<size_t>* offsets);

std::string ReplaceStringPlaceholders(std::string_view format_string,
                                      const std::vector<std::string>& subst,
                                      std::vector<size_t>* offsets);

// Single-string shortcut for ReplaceStringHolders. |offset| may be NULL.
std::u16string ReplaceStringPlaceholders(const std::u16string& format_string,
                                         const std::u16string& a,
                                         size_t* offset);

}  // namespace base

#if defined(OS_WIN)
#include "base/strings/string_util_win.h"
#elif defined(OS_POSIX) || defined(OS_FUCHSIA)
#include "base/strings/string_util_posix.h"
#else
#error Define string operations appropriately for your platform
#endif

#endif  // BASE_STRINGS_STRING_UTIL_H_
