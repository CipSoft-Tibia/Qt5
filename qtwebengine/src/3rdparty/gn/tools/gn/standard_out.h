// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_STANDARD_OUT_H_
#define TOOLS_GN_STANDARD_OUT_H_

#include <string>

enum TextDecoration {
  DECORATION_NONE = 0,
  DECORATION_DIM,
  DECORATION_RED,
  DECORATION_GREEN,
  DECORATION_BLUE,
  DECORATION_YELLOW
};

enum HtmlEscaping {
  NO_ESCAPING,

  // Convert < and > to &lt; and &gt; when writing markdown output in non-code
  // sections.
  DEFAULT_ESCAPING,
};

void OutputString(const std::string& output,
                  TextDecoration dec = DECORATION_NONE,
                  HtmlEscaping = DEFAULT_ESCAPING);

// If printing markdown, this generates table-of-contents entries with
// links to the actual help; otherwise, prints a one-line description.
void PrintSectionHelp(const std::string& line,
                      const std::string& topic,
                      const std::string& tag);

// Prints a line for a command, assuming there is a colon. Everything before
// the colon is the command (and is highlighted). After the colon if there is
// a square bracket, the contents of the bracket is dimmed.
//
// The link_tag is set, it will be used for markdown output links. This is
// used when generating the markdown for all help topics. If empty, no link tag
// will be emitted. In non-markdown mode, this parameter will be ignored.
//
// The line is indented 2 spaces.
void PrintShortHelp(const std::string& line,
                    const std::string& link_tag = std::string());

// Prints a longer help section.
//
// Rules:
// - Lines beginning with non-whitespace are highlighted up to the first
//   colon (or the whole line if not).
// - Lines whose first non-whitespace character is a # are dimmed.
//
// The tag will be used as a link target for the first header. This is used
// when generating the markdown for all help topics. If empty, no link tag will
// be emitted. Used only in markdown mode.
void PrintLongHelp(const std::string& text, const std::string& tag = "");

#endif  // TOOLS_GN_STANDARD_OUT_H_
