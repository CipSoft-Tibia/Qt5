/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qregexp.h>
#include "atom.h"
#include "location.h"
#include "qdocdatabase.h"
#include <stdio.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

/*! \class Atom
    \brief The Atom class is the fundamental unit for representing
    documents internally.

  Atoms have a \i type and are completed by a \i string whose
  meaning depends on the \i type. For example, the string
  \quotation
      \i italic text looks nicer than \bold bold text
  \endquotation
  is represented by the following atoms:
  \quotation
      (FormattingLeft, ATOM_FORMATTING_ITALIC)
      (String, "italic")
      (FormattingRight, ATOM_FORMATTING_ITALIC)
      (String, " text is more attractive than ")
      (FormattingLeft, ATOM_FORMATTING_BOLD)
      (String, "bold")
      (FormattingRight, ATOM_FORMATTING_BOLD)
      (String, " text")
  \endquotation

  \also Text
*/

/*! \enum Atom::AtomType

  \value AnnotatedList
  \value AutoLink
  \value BaseName
  \value BriefLeft
  \value BriefRight
  \value C
  \value CaptionLeft
  \value CaptionRight
  \value Code
  \value CodeBad
  \value CodeNew
  \value CodeOld
  \value CodeQuoteArgument
  \value CodeQuoteCommand
  \value DivLeft
  \value DivRight
  \value EndQmlText
  \value FormatElse
  \value FormatEndif
  \value FormatIf
  \value FootnoteLeft
  \value FootnoteRight
  \value FormattingLeft
  \value FormattingRight
  \value GeneratedList
  \value Image
  \value ImageText
  \value ImportantNote
  \value InlineImage
  \value JavaScript
  \value EndJavaScript
  \value Keyword
  \value LineBreak
  \value Link
  \value LinkNode
  \value ListLeft
  \value ListItemNumber
  \value ListTagLeft
  \value ListTagRight
  \value ListItemLeft
  \value ListItemRight
  \value ListRight
  \value NavAutoLink
  \value NavLink
  \value Nop
  \value Note
  \value ParaLeft
  \value ParaRight
  \value Qml
  \value QmlText
  \value QuotationLeft
  \value QuotationRight
  \value RawString
  \value SectionLeft
  \value SectionRight
  \value SectionHeadingLeft
  \value SectionHeadingRight
  \value SidebarLeft
  \value SidebarRight
  \value SinceList
  \value SinceTagLeft
  \value SinceTagRight
  \value String
  \value TableLeft
  \value TableRight
  \value TableHeaderLeft
  \value TableHeaderRight
  \value TableRowLeft
  \value TableRowRight
  \value TableItemLeft
  \value TableItemRight
  \value TableOfContents
  \value Target
  \value UnhandledFormat
  \value UnknownCommand
*/

QString Atom::noError_ = QString();

static const struct {
    const char *english;
    int no;
} atms[] = {
    { "AnnotatedList", Atom::AnnotatedList },
    { "AutoLink", Atom::AutoLink },
    { "BaseName", Atom::BaseName },
    { "br", Atom::BR},
    { "BriefLeft", Atom::BriefLeft },
    { "BriefRight", Atom::BriefRight },
    { "C", Atom::C },
    { "CaptionLeft", Atom::CaptionLeft },
    { "CaptionRight", Atom::CaptionRight },
    { "Code", Atom::Code },
    { "CodeBad", Atom::CodeBad },
    { "CodeNew", Atom::CodeNew },
    { "CodeOld", Atom::CodeOld },
    { "CodeQuoteArgument", Atom::CodeQuoteArgument },
    { "CodeQuoteCommand", Atom::CodeQuoteCommand },
    { "DivLeft", Atom::DivLeft },
    { "DivRight", Atom::DivRight },
    { "EndQmlText", Atom::EndQmlText },
    { "FootnoteLeft", Atom::FootnoteLeft },
    { "FootnoteRight", Atom::FootnoteRight },
    { "FormatElse", Atom::FormatElse },
    { "FormatEndif", Atom::FormatEndif },
    { "FormatIf", Atom::FormatIf },
    { "FormattingLeft", Atom::FormattingLeft },
    { "FormattingRight", Atom::FormattingRight },
    { "GeneratedList", Atom::GeneratedList },
    { "GuidLink", Atom::GuidLink},
    { "hr", Atom::HR},
    { "Image", Atom::Image },
    { "ImageText", Atom::ImageText },
    { "ImportantLeft", Atom::ImportantLeft },
    { "ImportantRight", Atom::ImportantRight },
    { "InlineImage", Atom::InlineImage },
    { "JavaScript", Atom::JavaScript },
    { "EndJavaScript", Atom::EndJavaScript },
    { "Keyword", Atom::Keyword },
    { "LegaleseLeft", Atom::LegaleseLeft },
    { "LegaleseRight", Atom::LegaleseRight },
    { "LineBreak", Atom::LineBreak },
    { "Link", Atom::Link },
    { "LinkNode", Atom::LinkNode },
    { "ListLeft", Atom::ListLeft },
    { "ListItemNumber", Atom::ListItemNumber },
    { "ListTagLeft", Atom::ListTagLeft },
    { "ListTagRight", Atom::ListTagRight },
    { "ListItemLeft", Atom::ListItemLeft },
    { "ListItemRight", Atom::ListItemRight },
    { "ListRight", Atom::ListRight },
    { "NavAutoLink", Atom::NavAutoLink },
    { "NavLink", Atom::NavLink },
    { "Nop", Atom::Nop },
    { "NoteLeft", Atom::NoteLeft },
    { "NoteRight", Atom::NoteRight },
    { "ParaLeft", Atom::ParaLeft },
    { "ParaRight", Atom::ParaRight },
    { "Qml", Atom::Qml},
    { "QmlText", Atom::QmlText },
    { "QuotationLeft", Atom::QuotationLeft },
    { "QuotationRight", Atom::QuotationRight },
    { "RawString", Atom::RawString },
    { "SectionLeft", Atom::SectionLeft },
    { "SectionRight", Atom::SectionRight },
    { "SectionHeadingLeft", Atom::SectionHeadingLeft },
    { "SectionHeadingRight", Atom::SectionHeadingRight },
    { "SidebarLeft", Atom::SidebarLeft },
    { "SidebarRight", Atom::SidebarRight },
    { "SinceList", Atom::SinceList },
    { "SinceTagLeft", Atom::SinceTagLeft },
    { "SinceTagRight", Atom::SinceTagRight },
    { "SnippetCommand", Atom::SnippetCommand },
    { "SnippetIdentifier", Atom::SnippetIdentifier },
    { "SnippetLocation", Atom::SnippetLocation },
    { "String", Atom::String },
    { "TableLeft", Atom::TableLeft },
    { "TableRight", Atom::TableRight },
    { "TableHeaderLeft", Atom::TableHeaderLeft },
    { "TableHeaderRight", Atom::TableHeaderRight },
    { "TableRowLeft", Atom::TableRowLeft },
    { "TableRowRight", Atom::TableRowRight },
    { "TableItemLeft", Atom::TableItemLeft },
    { "TableItemRight", Atom::TableItemRight },
    { "TableOfContents", Atom::TableOfContents },
    { "Target", Atom::Target },
    { "UnhandledFormat", Atom::UnhandledFormat },
    { "UnknownCommand", Atom::UnknownCommand },
    { 0, 0 }
};

/*! \fn Atom::Atom(AtomType type, const QString& string)

  Constructs an atom of the specified \a type with the single
  parameter \a string and does not put the new atom in a list.
*/

/*! \fn Atom::Atom(AtomType type, const QString& p1, const QString& p2)

  Constructs an atom of the specified \a type with the two
  parameters \a p1 and \a p2 and does not put the new atom
  in a list.
*/

/*! \fn Atom(Atom *previous, AtomType type, const QString& string)

  Constructs an atom of the specified \a type with the single
  parameter \a string and inserts the new atom into the list
  after the \a previous atom.
*/

/*! \fn Atom::Atom(Atom* previous, AtomType type, const QString& p1, const QString& p2)

  Constructs an atom of the specified \a type with the two
  parameters \a p1 and \a p2 and inserts the new atom into
  the list after the \a previous atom.
*/

/*! \fn void Atom::appendChar(QChar ch)

  Appends \a ch to the string parameter of this atom.

  \also string()
*/

/*! \fn void Atom::appendString(const QString& string)

  Appends \a string to the string parameter of this atom.

  \also string()
*/

/*! \fn void Atom::chopString()

  \also string()
*/

/*! \fn Atom *Atom::next()
  Return the next atom in the atom list.
  \also type(), string()
*/

/*!
  Return the next Atom in the list if it is of AtomType \a t.
  Otherwise return 0.
 */
const Atom* Atom::next(AtomType t) const
{
    return (next_ && (next_->type() == t)) ? next_ : 0;
}

/*!
  Return the next Atom in the list if it is of AtomType \a t
  and its string part is \a s. Otherwise return 0.
 */
const Atom* Atom::next(AtomType t, const QString& s) const
{
    return (next_ && (next_->type() == t) && (next_->string() == s)) ? next_ : 0;
}

/*! \fn const Atom *Atom::next() const
  Return the next atom in the atom list.
  \also type(), string()
*/

/*! \fn AtomType Atom::type() const
  Return the type of this atom.
  \also string(), next()
*/

/*!
  Return the type of this atom as a string. Return "Invalid" if
  type() returns an impossible value.

  This is only useful for debugging.

  \also type()
*/
QString Atom::typeString() const
{
    static bool deja = false;

    if (!deja) {
        int i = 0;
        while (atms[i].english != 0) {
            if (atms[i].no != i)
                Location::internalError(QCoreApplication::translate("QDoc::Atom", "atom %1 missing").arg(i));
            i++;
        }
        deja = true;
    }

    int i = (int) type();
    if (i < 0 || i > (int) Last)
        return QLatin1String("Invalid");
    return QLatin1String(atms[i].english);
}

/*! \fn const QString& Atom::string() const

  Returns the string parameter that together with the type
  characterizes this atom.

  \also type(), next()
*/

/*!
  Dumps this Atom to stderr in printer friendly form.
 */
void Atom::dump() const
{
    QString str = string();
    str.replace(QLatin1String("\\"), QLatin1String("\\\\"));
    str.replace(QLatin1String("\""), QLatin1String("\\\""));
    str.replace(QLatin1String("\n"), QLatin1String("\\n"));
    str.replace(QRegExp(QLatin1String("[^\x20-\x7e]")), QLatin1String("?"));
    if (!str.isEmpty())
        str = QLatin1String(" \"") + str + QLatin1Char('"');
    fprintf(stderr,
            "    %-15s%s\n",
            typeString().toLatin1().data(),
            str.toLatin1().data());
}

/*!
  The only constructor for LinkAtom. It creates an Atom of
  type Atom::Link. \a p1 being the link target. \a p2 is the
  parameters in square brackets. Normally there is just one
  word in the square brackets, but there can be up to three
  words separated by spaces. The constructor splits \a p2 on
  the space character.
 */
LinkAtom::LinkAtom(const QString& p1, const QString& p2)
    : Atom(p1),
      resolved_(false),
      genus_(Node::DontCare),
      goal_(Node::NoType),
      domain_(0),
      squareBracketParams_(p2)
{
    // nada.
}

/*!
  This function resolves the parameters that were enclosed in
  square brackets. If the parameters have already been resolved,
  it does nothing and returns immediately.
 */
void LinkAtom::resolveSquareBracketParams()
{
    if (resolved_)
        return;
    QStringList params = squareBracketParams_.toLower().split(QLatin1Char(' '));
     foreach (const QString& p, params) {
        if (!domain_) {
            domain_ = QDocDatabase::qdocDB()->findTree(p);
            if (domain_) {
                 continue;
            }
         }
        if (goal_ == Node::NoType) {
            goal_ = Node::goal(p);
            if (goal_ != Node::NoType)
                continue;
        }
        if (p == "qml") {
            genus_ = Node::QML;
            continue;
        }
        if (p == "cpp") {
            genus_ = Node::CPP;
            continue;
        }
        if (p == "doc") {
            genus_ = Node::DOC;
            continue;
        }
        error_ = squareBracketParams_;
        break;
    }
    resolved_ = true;
}

/*!
  Standard copy constructor of LinkAtom \a t.
 */
LinkAtom::LinkAtom(const LinkAtom& t)
    : Atom(Link, t.string()),
      resolved_(t.resolved_),
      genus_(t.genus_),
      goal_(t.goal_),
      domain_(t.domain_),
      error_(t.error_),
      squareBracketParams_(t.squareBracketParams_)
{
    // nothing
}

/*!
  Special copy constructor of LinkAtom \a t, where
  where the new LinkAtom will not be the first one
  in the list.
 */
LinkAtom::LinkAtom(Atom* previous, const LinkAtom& t)
    : Atom(previous, Link, t.string()),
      resolved_(t.resolved_),
      genus_(t.genus_),
      goal_(t.goal_),
      domain_(t.domain_),
      error_(t.error_),
      squareBracketParams_(t.squareBracketParams_)
{
    previous->next_ = this;
}

QT_END_NAMESPACE
