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

#ifndef ATOM_H
#define ATOM_H

#include <qstringlist.h>
#include "node.h"
#include <qdebug.h>

QT_BEGIN_NAMESPACE

class Tree;
class LinkAtom;

class Atom
{
public:
    enum AtomType {
        AnnotatedList,
        AutoLink,
        BaseName,
        BR,
        BriefLeft,
        BriefRight,
        C,
        CaptionLeft,
        CaptionRight,
        Code,
        CodeBad,
        CodeNew,
        CodeOld,
        CodeQuoteArgument,
        CodeQuoteCommand,
        DivLeft,
        DivRight,
        EndQmlText,
        FootnoteLeft,
        FootnoteRight,
        FormatElse,
        FormatEndif,
        FormatIf,
        FormattingLeft,
        FormattingRight,
        GeneratedList,
        GuidLink,
        HR,
        Image,
        ImageText,
        ImportantLeft,
        ImportantRight,
        InlineImage,
        JavaScript,
        EndJavaScript,
        Keyword,
        LegaleseLeft,
        LegaleseRight,
        LineBreak,
        Link,
        LinkNode,
        ListLeft,
        ListItemNumber,
        ListTagLeft,
        ListTagRight,
        ListItemLeft,
        ListItemRight,
        ListRight,
        NavAutoLink,
        NavLink,
        Nop,
        NoteLeft,
        NoteRight,
        ParaLeft,
        ParaRight,
        Qml,
        QmlText,
        QuotationLeft,
        QuotationRight,
        RawString,
        SectionLeft,
        SectionRight,
        SectionHeadingLeft,
        SectionHeadingRight,
        SidebarLeft,
        SidebarRight,
        SinceList,
        SinceTagLeft,
        SinceTagRight,
        SnippetCommand,
        SnippetIdentifier,
        SnippetLocation,
        String,
        TableLeft,
        TableRight,
        TableHeaderLeft,
        TableHeaderRight,
        TableRowLeft,
        TableRowRight,
        TableItemLeft,
        TableItemRight,
        TableOfContents,
        Target,
        UnhandledFormat,
        UnknownCommand,
        Last = UnknownCommand
    };

    friend class LinkAtom;

    Atom(const QString& string)
        : next_(0), type_(Link)
    {
        strs << string;
    }

    Atom(AtomType type, const QString& string = "")
        : next_(0), type_(type)
    {
        strs << string;
    }

    Atom(AtomType type, const QString& p1, const QString& p2)
        : next_(0), type_(type)
    {
        strs << p1;
        if (!p2.isEmpty())
            strs << p2;
    }

    Atom(Atom* previous, AtomType type, const QString& string = "")
        : next_(previous->next_), type_(type)
    {
        strs << string;
        previous->next_ = this;
    }

    Atom(Atom* previous, AtomType type, const QString& p1, const QString& p2)
        : next_(previous->next_), type_(type)
    {
        strs << p1;
        if (!p2.isEmpty())
            strs << p2;
        previous->next_ = this;
    }

    virtual ~Atom() { }

    void appendChar(QChar ch) { strs[0] += ch; }
    void appendString(const QString& string) { strs[0] += string; }
    void chopString() { strs[0].chop(1); }
    void setString(const QString& string) { strs[0] = string; }
    Atom* next() { return next_; }
    void setNext(Atom* newNext) { next_ = newNext; }

    const Atom* next() const { return next_; }
    const Atom* next(AtomType t) const;
    const Atom* next(AtomType t, const QString& s) const;
    AtomType type() const { return type_; }
    QString typeString() const;
    const QString& string() const { return strs[0]; }
    const QString& string(int i) const { return strs[i]; }
    int count() const { return strs.size(); }
    void dump() const;
    const QStringList& strings() const { return strs; }

    virtual bool isLinkAtom() const { return false; }
    virtual Node::Genus genus() { return Node::DontCare; }
    virtual bool specifiesDomain() { return false; }
    virtual Tree* domain() { return 0; }
    virtual Node::NodeType goal() { return Node::NoType; }
    virtual const QString& error() { return noError_; }
    virtual void resolveSquareBracketParams() { }

 protected:
    static QString noError_;
    Atom* next_;
    AtomType type_;
    QStringList strs;
};

class LinkAtom : public Atom
{
 public:
    LinkAtom(const QString& p1, const QString& p2);
    LinkAtom(const LinkAtom& t);
    LinkAtom(Atom* previous, const LinkAtom& t);
    virtual ~LinkAtom() { }

    bool isLinkAtom() const override { return true; }
    Node::Genus genus() override { resolveSquareBracketParams(); return genus_; }
    bool specifiesDomain() override { resolveSquareBracketParams(); return (domain_ != 0); }
    Tree* domain() override { resolveSquareBracketParams(); return domain_; }
    Node::NodeType goal() override { resolveSquareBracketParams(); return goal_; }
    const QString& error() override { return error_; }
    void resolveSquareBracketParams() override;

 protected:
    bool        resolved_;
    Node::Genus genus_;
    Node::NodeType  goal_;
    Tree*       domain_;
    QString     error_;
    QString     squareBracketParams_;
};

#define ATOM_FORMATTING_BOLD            "bold"
#define ATOM_FORMATTING_INDEX           "index"
#define ATOM_FORMATTING_ITALIC          "italic"
#define ATOM_FORMATTING_LINK            "link"
#define ATOM_FORMATTING_PARAMETER       "parameter"
#define ATOM_FORMATTING_SPAN            "span "
#define ATOM_FORMATTING_SUBSCRIPT       "subscript"
#define ATOM_FORMATTING_SUPERSCRIPT     "superscript"
#define ATOM_FORMATTING_TELETYPE        "teletype"
#define ATOM_FORMATTING_UICONTROL       "uicontrol"
#define ATOM_FORMATTING_UNDERLINE       "underline"

#define ATOM_LIST_BULLET                "bullet"
#define ATOM_LIST_TAG                   "tag"
#define ATOM_LIST_VALUE                 "value"
#define ATOM_LIST_LOWERALPHA            "loweralpha"
#define ATOM_LIST_LOWERROMAN            "lowerroman"
#define ATOM_LIST_NUMERIC               "numeric"
#define ATOM_LIST_UPPERALPHA            "upperalpha"
#define ATOM_LIST_UPPERROMAN            "upperroman"

QT_END_NAMESPACE

#endif
