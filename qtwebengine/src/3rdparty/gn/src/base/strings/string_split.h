// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_STRINGS_STRING_SPLIT_H_
#define BASE_STRINGS_STRING_SPLIT_H_

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace base {

enum WhitespaceHandling {
  KEEP_WHITESPACE,
  TRIM_WHITESPACE,
};

enum SplitResult {
  // Strictly return all results.
  //
  // If the input is ",," and the separator is ',' this will return a
  // vector of three empty strings.
  SPLIT_WANT_ALL,

  // Only nonempty results will be added to the results. Multiple separators
  // will be coalesced. Separators at the beginning and end of the input will
  // be ignored. With TRIM_WHITESPACE, whitespace-only results will be dropped.
  //
  // If the input is ",," and the separator is ',', this will return an empty
  // vector.
  SPLIT_WANT_NONEMPTY,
};

// Split the given string on ANY of the given separators, returning copies of
// the result.
//
// To split on either commas or semicolons, keeping all whitespace:
//
//   std::vector<std::string> tokens = base::SplitString(
//       input, ",;", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
std::vector<std::string> SplitString(std::string_view input,
                                     std::string_view separators,
                                     WhitespaceHandling whitespace,
                                     SplitResult result_type);
std::vector<std::u16string> SplitString(std::u16string_view input,
                                        std::u16string_view separators,
                                        WhitespaceHandling whitespace,
                                        SplitResult result_type);

// Like SplitString above except it returns a vector of StringPieces which
// reference the original buffer without copying. Although you have to be
// careful to keep the original string unmodified, this provides an efficient
// way to iterate through tokens in a string.
//
// To iterate through all whitespace-separated tokens in an input string:
//
//   for (const auto& cur :
//        base::SplitStringPiece(input, base::kWhitespaceASCII,
//                               base::KEEP_WHITESPACE,
//                               base::SPLIT_WANT_NONEMPTY)) {
//     ...
std::vector<std::string_view> SplitStringPiece(std::string_view input,
                                               std::string_view separators,
                                               WhitespaceHandling whitespace,
                                               SplitResult result_type);
std::vector<std::u16string_view> SplitStringPiece(
    std::u16string_view input,
    std::u16string_view separators,
    WhitespaceHandling whitespace,
    SplitResult result_type);

using StringPairs = std::vector<std::pair<std::string, std::string>>;

// Splits |line| into key value pairs according to the given delimiters and
// removes whitespace leading each key and trailing each value. Returns true
// only if each pair has a non-empty key and value. |key_value_pairs| will
// include ("","") pairs for entries without |key_value_delimiter|.
bool SplitStringIntoKeyValuePairs(std::string_view input,
                                  char key_value_delimiter,
                                  char key_value_pair_delimiter,
                                  StringPairs* key_value_pairs);

// Similar to SplitString, but use a substring delimiter instead of a list of
// characters that are all possible delimiters.
std::vector<std::u16string> SplitStringUsingSubstr(
    std::u16string_view input,
    std::u16string_view delimiter,
    WhitespaceHandling whitespace,
    SplitResult result_type);
std::vector<std::string> SplitStringUsingSubstr(std::string_view input,
                                                std::string_view delimiter,
                                                WhitespaceHandling whitespace,
                                                SplitResult result_type);

// Like SplitStringUsingSubstr above except it returns a vector of StringPieces
// which reference the original buffer without copying. Although you have to be
// careful to keep the original string unmodified, this provides an efficient
// way to iterate through tokens in a string.
//
// To iterate through all newline-separated tokens in an input string:
//
//   for (const auto& cur :
//        base::SplitStringUsingSubstr(input, "\r\n",
//                                     base::KEEP_WHITESPACE,
//                                     base::SPLIT_WANT_NONEMPTY)) {
//     ...
std::vector<std::u16string_view> SplitStringPieceUsingSubstr(
    std::u16string_view input,
    std::u16string_view delimiter,
    WhitespaceHandling whitespace,
    SplitResult result_type);
std::vector<std::string_view> SplitStringPieceUsingSubstr(
    std::string_view input,
    std::string_view delimiter,
    WhitespaceHandling whitespace,
    SplitResult result_type);

}  // namespace base

#endif  // BASE_STRINGS_STRING_SPLIT_H_
