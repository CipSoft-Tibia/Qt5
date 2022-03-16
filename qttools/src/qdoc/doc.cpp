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

#include "config.h"
#include "doc.h"
#include "codemarker.h"
#include "editdistance.h"
#include "openedlist.h"
#include "quoter.h"
#include "text.h"
#include "atom.h"
#include "tokenizer.h"
#include "loggingcategory.h"
#include <qdatetime.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qhash.h>
#include <qtextstream.h>
#include <qregexp.h>
#include <ctype.h>
#include <limits.h>
#include <qdebug.h>
#include "generator.h"

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QSet<QString>, null_Set_QString)
Q_GLOBAL_STATIC(TopicList, nullTopicList)
Q_GLOBAL_STATIC(QStringList, null_QStringList)
Q_GLOBAL_STATIC(QList<Text>, null_QList_Text)
Q_GLOBAL_STATIC(QStringMultiMap, null_QStringMultiMap)

struct Macro
{
    QString defaultDef;
    Location defaultDefLocation;
    QStringMap otherDefs;
    int numParams;
};

enum {
    CMD_A,
    CMD_ANNOTATEDLIST,
    CMD_B,
    CMD_BADCODE,
    CMD_BOLD,
    CMD_BR,
    CMD_BRIEF,
    CMD_C,
    CMD_CAPTION,
    CMD_CODE,
    CMD_CODELINE,
    CMD_DIV,
    CMD_DOTS,
    CMD_E,
    CMD_ELSE,
    CMD_ENDCODE,
    CMD_ENDDIV,
    CMD_ENDFOOTNOTE,
    CMD_ENDIF,
    CMD_ENDLEGALESE,
    CMD_ENDLINK,
    CMD_ENDLIST,
    CMD_ENDMAPREF,
    CMD_ENDOMIT,
    CMD_ENDQUOTATION,
    CMD_ENDRAW,
    CMD_ENDSECTION1,
    CMD_ENDSECTION2,
    CMD_ENDSECTION3,
    CMD_ENDSECTION4,
    CMD_ENDSIDEBAR,
    CMD_ENDTABLE,
    CMD_ENDTOPICREF,
    CMD_FOOTNOTE,
    CMD_GENERATELIST,
    CMD_GRANULARITY,
    CMD_HEADER,
    CMD_HR,
    CMD_I,
    CMD_IF,
    CMD_IMAGE,
    CMD_IMPORTANT,
    CMD_INCLUDE,
    CMD_INLINEIMAGE,
    CMD_INDEX,
    CMD_INPUT,
    CMD_KEYWORD,
    CMD_L,
    CMD_LEGALESE,
    CMD_LI,
    CMD_LINK,
    CMD_LIST,
    CMD_MAPREF,
    CMD_META,
    CMD_NEWCODE,
    CMD_NOTE,
    CMD_O,
    CMD_OLDCODE,
    CMD_OMIT,
    CMD_OMITVALUE,
    CMD_OVERLOAD,
    CMD_PRINTLINE,
    CMD_PRINTTO,
    CMD_PRINTUNTIL,
    CMD_QUOTATION,
    CMD_QUOTEFILE,
    CMD_QUOTEFROMFILE,
    CMD_QUOTEFUNCTION,
    CMD_RAW,
    CMD_ROW,
    CMD_SA,
    CMD_SECTION1,
    CMD_SECTION2,
    CMD_SECTION3,
    CMD_SECTION4,
    CMD_SIDEBAR,
    CMD_SINCELIST,
    CMD_SKIPLINE,
    CMD_SKIPTO,
    CMD_SKIPUNTIL,
    CMD_SNIPPET,
    CMD_SPAN,
    CMD_SUB,
    CMD_SUP,
    CMD_TABLE,
    CMD_TABLEOFCONTENTS,
    CMD_TARGET,
    CMD_TOPICREF,
    CMD_TT,
    CMD_UICONTROL,
    CMD_UNDERLINE,
    CMD_UNICODE,
    CMD_VALUE,
    CMD_WARNING,
    CMD_QML,
    CMD_ENDQML,
    CMD_CPP,
    CMD_ENDCPP,
    CMD_QMLTEXT,
    CMD_ENDQMLTEXT,
    CMD_CPPTEXT,
    CMD_ENDCPPTEXT,
    CMD_JS,
    CMD_ENDJS,
    NOT_A_CMD
};

static struct {
    const char *english;
    int no;
    QString *alias;
} cmds[] = {
    { "a", CMD_A, 0 },
    { "annotatedlist", CMD_ANNOTATEDLIST, 0 },
    { "b", CMD_B, 0 },
    { "badcode", CMD_BADCODE, 0 },
    { "bold", CMD_BOLD, 0 },
    { "br", CMD_BR, 0 },
    { "brief", CMD_BRIEF, 0 },
    { "c", CMD_C, 0 },
    { "caption", CMD_CAPTION, 0 },
    { "code", CMD_CODE, 0 },
    { "codeline", CMD_CODELINE, 0},
    { "div", CMD_DIV, 0 },
    { "dots", CMD_DOTS, 0 },
    { "e", CMD_E, 0 },
    { "else", CMD_ELSE, 0 },
    { "endcode", CMD_ENDCODE, 0 },
    { "enddiv", CMD_ENDDIV, 0 },
    { "endfootnote", CMD_ENDFOOTNOTE, 0 },
    { "endif", CMD_ENDIF, 0 },
    { "endlegalese", CMD_ENDLEGALESE, 0 },
    { "endlink", CMD_ENDLINK, 0 },
    { "endlist", CMD_ENDLIST, 0 },
    { "endmapref", CMD_ENDMAPREF, 0 },
    { "endomit", CMD_ENDOMIT, 0 },
    { "endquotation", CMD_ENDQUOTATION, 0 },
    { "endraw", CMD_ENDRAW, 0 },
    { "endsection1", CMD_ENDSECTION1, 0 },  // ### don't document for now
    { "endsection2", CMD_ENDSECTION2, 0 },  // ### don't document for now
    { "endsection3", CMD_ENDSECTION3, 0 },  // ### don't document for now
    { "endsection4", CMD_ENDSECTION4, 0 },  // ### don't document for now
    { "endsidebar", CMD_ENDSIDEBAR, 0 },
    { "endtable", CMD_ENDTABLE, 0 },
    { "endtopicref", CMD_ENDTOPICREF, 0 },
    { "footnote", CMD_FOOTNOTE, 0 },
    { "generatelist", CMD_GENERATELIST, 0 },
    { "granularity", CMD_GRANULARITY, 0 }, // ### don't document for now
    { "header", CMD_HEADER, 0 },
    { "hr", CMD_HR, 0 },
    { "i", CMD_I, 0 },
    { "if", CMD_IF, 0 },
    { "image", CMD_IMAGE, 0 },
    { "important", CMD_IMPORTANT, 0 },
    { "include", CMD_INCLUDE, 0 },
    { "inlineimage", CMD_INLINEIMAGE, 0 },
    { "index", CMD_INDEX, 0 }, // ### don't document for now
    { "input", CMD_INPUT, 0 },
    { "keyword", CMD_KEYWORD, 0 },
    { "l", CMD_L, 0 },
    { "legalese", CMD_LEGALESE, 0 },
    { "li", CMD_LI, 0 },
    { "link", CMD_LINK, 0 },
    { "list", CMD_LIST, 0 },
    { "mapref", CMD_MAPREF, 0 },
    { "meta", CMD_META, 0 },
    { "newcode", CMD_NEWCODE, 0 },
    { "note", CMD_NOTE, 0 },
    { "o", CMD_O, 0 },
    { "oldcode", CMD_OLDCODE, 0 },
    { "omit", CMD_OMIT, 0 },
    { "omitvalue", CMD_OMITVALUE, 0 },
    { "overload", CMD_OVERLOAD, 0 },
    { "printline", CMD_PRINTLINE, 0 },
    { "printto", CMD_PRINTTO, 0 },
    { "printuntil", CMD_PRINTUNTIL, 0 },
    { "quotation", CMD_QUOTATION, 0 },
    { "quotefile", CMD_QUOTEFILE, 0 },
    { "quotefromfile", CMD_QUOTEFROMFILE, 0 },
    { "quotefunction", CMD_QUOTEFUNCTION, 0 },
    { "raw", CMD_RAW, 0 },
    { "row", CMD_ROW, 0 },
    { "sa", CMD_SA, 0 },
    { "section1", CMD_SECTION1, 0 },
    { "section2", CMD_SECTION2, 0 },
    { "section3", CMD_SECTION3, 0 },
    { "section4", CMD_SECTION4, 0 },
    { "sidebar", CMD_SIDEBAR, 0 },
    { "sincelist", CMD_SINCELIST, 0 },
    { "skipline", CMD_SKIPLINE, 0 },
    { "skipto", CMD_SKIPTO, 0 },
    { "skipuntil", CMD_SKIPUNTIL, 0 },
    { "snippet", CMD_SNIPPET, 0 },
    { "span", CMD_SPAN, 0 },
    { "sub", CMD_SUB, 0 },
    { "sup", CMD_SUP, 0 },
    { "table", CMD_TABLE, 0 },
    { "tableofcontents", CMD_TABLEOFCONTENTS, 0 },
    { "target", CMD_TARGET, 0 },
    { "topicref", CMD_TOPICREF, 0 },
    { "tt", CMD_TT, 0 },
    { "uicontrol", CMD_UICONTROL, 0 },
    { "underline", CMD_UNDERLINE, 0 },
    { "unicode", CMD_UNICODE, 0 },
    { "value", CMD_VALUE, 0 },
    { "warning", CMD_WARNING, 0 },
    { "qml", CMD_QML, 0 },
    { "endqml", CMD_ENDQML, 0 },
    { "cpp", CMD_CPP, 0 },
    { "endcpp", CMD_ENDCPP, 0 },
    { "qmltext", CMD_QMLTEXT, 0 },
    { "endqmltext", CMD_ENDQMLTEXT, 0 },
    { "cpptext", CMD_CPPTEXT, 0 },
    { "endcpptext", CMD_ENDCPPTEXT, 0 },
    { "js", CMD_JS, 0 },
    { "endjs", CMD_ENDJS, 0 },
    { 0, 0, 0 }
};

typedef QHash<QString, int> QHash_QString_int;
typedef QHash<QString, Macro> QHash_QString_Macro;

Q_GLOBAL_STATIC(QStringMap, aliasMap)
Q_GLOBAL_STATIC(QHash_QString_int, cmdHash)
Q_GLOBAL_STATIC(QHash_QString_Macro, macroHash)

class DocPrivateExtra
{
public:
    Doc::Sections       granularity_;
    Doc::Sections       section_; // ###
    QList<Atom*>        tableOfContents_;
    QVector<int>        tableOfContentsLevels_;
    QList<Atom*>        keywords_;
    QList<Atom*>        targets_;
    QStringMultiMap     metaMap_;

    DocPrivateExtra()
        : granularity_(Doc::Part)
        , section_(Doc::NoSection)
    { }
};

struct Shared // ### get rid of
{
    Shared()
        : count(1) { }
    void ref() { ++count; }
    bool deref() { return (--count == 0); }

    int count;
};

static QString cleanLink(const QString &link)
{
    int colonPos = link.indexOf(':');
    if ((colonPos == -1) ||
            (!link.startsWith("file:") && !link.startsWith("mailto:")))
        return link;
    return link.mid(colonPos + 1).simplified();
}

typedef QMap<QString, ArgList> CommandMap;

class DocPrivate : public Shared
{
public:
    DocPrivate(const Location& start = Location::null,
               const Location& end = Location::null,
               const QString& source = QString());
    ~DocPrivate();

    void addAlso(const Text& also);
    void constructExtra();
    bool isEnumDocSimplifiable() const;

    // ### move some of this in DocPrivateExtra
    Location start_loc;
    Location end_loc;
    QString src;
    Text text;
    QSet<QString> params;
    QList<Text> alsoList;
    QStringList enumItemList;
    QStringList omitEnumItemList;
    QSet<QString> metacommandsUsed;
    CommandMap metaCommandMap;
    bool hasLegalese : 1;
    bool hasSectioningUnits : 1;
    DocPrivateExtra *extra;
    TopicList topics_;
    DitaRefList ditamap_;
};

DocPrivate::DocPrivate(const Location& start,
                       const Location& end,
                       const QString& source)
    : start_loc(start),
      end_loc(end),
      src(source),
      hasLegalese(false),
      hasSectioningUnits(false),
      extra(0)
{
    // nothing.
}

/*!
  If the doc is a ditamap, the destructor deletes each element
  in the ditamap structure. These were allocated as needed.
 */
DocPrivate::~DocPrivate()
{
    delete extra;
    foreach (DitaRef* t, ditamap_) {
        delete t;
    }
}

void DocPrivate::addAlso(const Text& also)
{
    alsoList.append(also);
}

void DocPrivate::constructExtra()
{
    if (extra == 0)
        extra = new DocPrivateExtra;
}

bool DocPrivate::isEnumDocSimplifiable() const
{
    bool justMetColon = false;
    int numValueTables = 0;

    const Atom *atom = text.firstAtom();
    while (atom) {
        if (atom->type() == Atom::AutoLink || atom->type() == Atom::String) {
            justMetColon = atom->string().endsWith(QLatin1Char(':'));
        }
        else if ((atom->type() == Atom::ListLeft) &&
                 (atom->string() == ATOM_LIST_VALUE)) {
            if (justMetColon || numValueTables > 0)
                return false;
            ++numValueTables;
        }
        atom = atom->next();
    }
    return true;
}

class DocParser
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::DocParser)

public:
    void parse(const QString &source,
               DocPrivate *docPrivate,
               const QSet<QString> &metaCommandSet,
               const QSet<QString>& possibleTopics);

    static int endCmdFor(int cmd);
    static QString cmdName(int cmd);
    static QString endCmdName(int cmd);
    static QString untabifyEtc(const QString& str);
    static int indentLevel(const QString& str);
    static QString unindent(int level, const QString& str);
    static QString slashed(const QString& str);

    static int tabSize;
    static QStringList exampleFiles;
    static QStringList exampleDirs;
    static QStringList sourceFiles;
    static QStringList sourceDirs;
    static bool quoting;

private:
    Location& location();
    QString detailsUnknownCommand(const QSet<QString>& metaCommandSet,
                                  const QString& str);
    void insertTarget(const QString& target, bool keyword);
    void include(const QString& fileName, const QString& identifier);
    void startFormat(const QString& format, int cmd);
    bool openCommand(int cmd);
    bool closeCommand(int endCmd);
    void startSection(Doc::Sections unit, int cmd);
    void endSection(int unit, int endCmd);
    void parseAlso();
    void append(const QString &string);
    void append(Atom::AtomType type, const QString& string = QString());
    void append(Atom::AtomType type, const QString& p1, const QString& p2);
    void append(const QString& p1, const QString& p2);
    void appendChar(QChar ch);
    void appendWord(const QString &word);
    void appendToCode(const QString &code);
    void appendToCode(const QString &code, Atom::AtomType defaultType);
    void startNewPara();
    void enterPara(Atom::AtomType leftType = Atom::ParaLeft,
                   Atom::AtomType rightType = Atom::ParaRight,
                   const QString& string = QString());
    void leavePara();
    void leaveValue();
    void leaveValueList();
    void leaveTableRow();
    CodeMarker *quoteFromFile();
    bool expandMacro();
    void expandMacro(const QString& name, const QString& def, int numParams);
    QString expandMacroToString(const QString &name, const QString &def, int numParams, const QString &matchExpr);
    Doc::Sections getSectioningUnit();
    QString getArgument(bool verbatim = false);
    QString getBracedArgument(bool verbatim);
    QString getBracketedArgument();
    QString getOptionalArgument();
    QString getRestOfLine();
    QString getMetaCommandArgument(const QString &cmdStr);
    QString getUntilEnd(int cmd);
    QString getCode(int cmd, CodeMarker *marker, const QString &argStr = QString());
    QString getUnmarkedCode(int cmd);

    bool isBlankLine();
    bool isLeftBraceAhead();
    bool isLeftBracketAhead();
    void skipSpacesOnLine();
    void skipSpacesOrOneEndl();
    void skipAllSpaces();
    void skipToNextPreprocessorCommand();
    static bool isCode(const Atom *atom);
    static bool isQuote(const Atom *atom);

    QStack<int> openedInputs;

    QString input_;
    int pos;
    int backslashPos;
    int endPos;
    int len;
    Location cachedLoc;
    int cachedPos;

    DocPrivate* priv;
    enum ParagraphState {
        OutsideParagraph,
        InSingleLineParagraph,
        InMultiLineParagraph
    };
    ParagraphState paraState;
    bool inTableHeader;
    bool inTableRow;
    bool inTableItem;
    bool indexStartedPara; // ### rename
    Atom::AtomType pendingParaLeftType;
    Atom::AtomType pendingParaRightType;
    QString pendingParaString;

    int braceDepth;
    Doc::Sections currentSection;
    QMap<QString, Location> targetMap_;
    QMap<int, QString> pendingFormats;
    QStack<int> openedCommands;
    QStack<OpenedList> openedLists;
    Quoter quoter;
    QStack<DitaRef*> ditarefs_;
    Atom *lastAtom;
};

int DocParser::tabSize;
QStringList DocParser::exampleFiles;
QStringList DocParser::exampleDirs;
QStringList DocParser::sourceFiles;
QStringList DocParser::sourceDirs;
bool DocParser::quoting = false;

/*!
  Parse the \a source string to build a Text data structure
  in \a docPrivate. The Text data structure is a linked list
  of Atoms.

  \a metaCommandSet is the set of metacommands that may be
  found in \a source. These metacommands are not markup text
  commands. They are topic commands and related metacommands.
 */
void DocParser::parse(const QString& source,
                      DocPrivate *docPrivate,
                      const QSet<QString>& metaCommandSet,
                      const QSet<QString>& possibleTopics)
{
    input_ = source;
    pos = 0;
    len = input_.length();
    cachedLoc = docPrivate->start_loc;
    cachedPos = 0;
    priv = docPrivate;
    priv->text << Atom::Nop;
    priv->topics_.clear();

    paraState = OutsideParagraph;
    inTableHeader = false;
    inTableRow = false;
    inTableItem = false;
    indexStartedPara = false;
    pendingParaLeftType = Atom::Nop;
    pendingParaRightType = Atom::Nop;

    braceDepth = 0;
    currentSection = Doc::NoSection;
    openedCommands.push(CMD_OMIT);
    quoter.reset();

    CodeMarker *marker = 0;
    Atom *currentLinkAtom = 0;
    QString p1, p2;
    QStack<bool> preprocessorSkipping;
    int numPreprocessorSkipping = 0;

    while (pos < len) {
        QChar ch = input_.at(pos);

        switch (ch.unicode()) {
        case '\\': {
            QString cmdStr;
            backslashPos = pos;
            pos++;
            while (pos < len) {
                ch = input_.at(pos);
                if (ch.isLetterOrNumber()) {
                    cmdStr += ch;
                    pos++;
                }
                else {
                    break;
                }
            }
            endPos = pos;
            if (cmdStr.isEmpty()) {
                if (pos < len) {
                    enterPara();
                    if (input_.at(pos).isSpace()) {
                        skipAllSpaces();
                        appendChar(QLatin1Char(' '));
                    }
                    else {
                        appendChar(input_.at(pos++));
                    }
                }
            }
            else {
                // Ignore quoting atoms to make appendToCode()
                // append to the correct atom.
                if (!quoting || !isQuote(priv->text.lastAtom()))
                    lastAtom = priv->text.lastAtom();

                int cmd = cmdHash()->value(cmdStr,NOT_A_CMD);
                switch (cmd) {
                case CMD_A:
                    enterPara();
                    p1 = getArgument();
                    append(Atom::FormattingLeft,ATOM_FORMATTING_PARAMETER);
                    append(Atom::String, p1);
                    append(Atom::FormattingRight,ATOM_FORMATTING_PARAMETER);
                    priv->params.insert(p1);
                    break;
                case CMD_BADCODE:
                    leavePara();
                    append(Atom::CodeBad, getCode(CMD_BADCODE, marker, getMetaCommandArgument(cmdStr)));
                    break;
                case CMD_BR:
                    enterPara();
                    append(Atom::BR);
                    break;
                case CMD_BOLD:
                    location().warning(tr("'\\bold' is deprecated. Use '\\b'"));
                    Q_FALLTHROUGH();
                case CMD_B:
                    startFormat(ATOM_FORMATTING_BOLD, cmd);
                    break;
                case CMD_BRIEF:
                    leavePara();
                    enterPara(Atom::BriefLeft, Atom::BriefRight);
                    break;
                case CMD_C:
                    enterPara();
                    p1 = untabifyEtc(getArgument(true));
                    marker = CodeMarker::markerForCode(p1);
                    append(Atom::C, marker->markedUpCode(p1, 0, location()));
                    break;
                case CMD_CAPTION:
                    leavePara();
                    enterPara(Atom::CaptionLeft, Atom::CaptionRight);
                    break;
                case CMD_CODE:
                    leavePara();
                    append(Atom::Code, getCode(CMD_CODE, 0, getMetaCommandArgument(cmdStr)));
                    break;
                case CMD_QML:
                    leavePara();
                    append(Atom::Qml, getCode(CMD_QML,
                                              CodeMarker::markerForLanguage(QLatin1String("QML")),
                                              getMetaCommandArgument(cmdStr)));
                    break;
                case CMD_QMLTEXT:
                    append(Atom::QmlText);
                    break;
                case CMD_JS:
                    leavePara();
                    append(Atom::JavaScript, getCode(CMD_JS,
                                                     CodeMarker::markerForLanguage(QLatin1String("JavaScript")),
                                                     getMetaCommandArgument(cmdStr)));
                    break;
                case CMD_DIV:
                    leavePara();
                    p1 = getArgument(true);
                    append(Atom::DivLeft, p1);
                    openedCommands.push(cmd);
                    break;
                case CMD_ENDDIV:
                    leavePara();
                    append(Atom::DivRight);
                    closeCommand(cmd);
                    break;
                case CMD_CODELINE:
                    if (quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, " ");
                    }
                    if (isCode(lastAtom) && lastAtom->string().endsWith("\n\n"))
                        lastAtom->chopString();
                    appendToCode("\n");
                    break;
                case CMD_DOTS: {
                    QString arg = getOptionalArgument();
                    if (arg.isEmpty())
                        arg = "4";
                    if (quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, arg);
                    }
                    if (isCode(lastAtom) && lastAtom->string().endsWith("\n\n"))
                        lastAtom->chopString();

                    int indent = arg.toInt();
                    for (int i = 0; i < indent; ++i)
                        appendToCode(" ");
                    appendToCode("...\n");
                    break;
                }
                case CMD_ELSE:
                    if (preprocessorSkipping.size() > 0) {
                        if (preprocessorSkipping.top()) {
                            --numPreprocessorSkipping;
                        } else {
                            ++numPreprocessorSkipping;
                        }
                        preprocessorSkipping.top() = !preprocessorSkipping.top();
                        (void)getRestOfLine(); // ### should ensure that it's empty
                        if (numPreprocessorSkipping)
                            skipToNextPreprocessorCommand();
                    } else {
                        location().warning(tr("Unexpected '\\%1'").arg(cmdName(CMD_ELSE)));
                    }
                    break;
                case CMD_ENDCODE:
                    closeCommand(cmd);
                    break;
                case CMD_ENDQML:
                    closeCommand(cmd);
                    break;
                case CMD_ENDQMLTEXT:
                    append(Atom::EndQmlText);
                    break;
                case CMD_ENDJS:
                    closeCommand(cmd);
                    break;
                case CMD_ENDFOOTNOTE:
                    if (closeCommand(cmd)) {
                        leavePara();
                        append(Atom::FootnoteRight);
                        paraState = InMultiLineParagraph; // ###
                    }
                    break;
                case CMD_ENDIF:
                    if (preprocessorSkipping.count() > 0) {
                        if (preprocessorSkipping.pop())
                            --numPreprocessorSkipping;
                        (void)getRestOfLine(); // ### should ensure that it's empty
                        if (numPreprocessorSkipping)
                            skipToNextPreprocessorCommand();
                    } else {
                        location().warning(tr("Unexpected '\\%1'").arg(cmdName(CMD_ENDIF)));
                    }
                    break;
                case CMD_ENDLEGALESE:
                    if (closeCommand(cmd)) {
                        leavePara();
                        append(Atom::LegaleseRight);
                    }
                    break;
                case CMD_ENDLINK:
                    if (closeCommand(cmd)) {
                        if (priv->text.lastAtom()->type() == Atom::String
                                && priv->text.lastAtom()->string().endsWith(QLatin1Char(' ')))
                            priv->text.lastAtom()->chopString();
                        append(Atom::FormattingRight, ATOM_FORMATTING_LINK);
                    }
                    break;
                case CMD_ENDLIST:
                    if (closeCommand(cmd)) {
                        leavePara();
                        if (openedLists.top().isStarted()) {
                            append(Atom::ListItemRight,
                                   openedLists.top().styleString());
                            append(Atom::ListRight,
                                   openedLists.top().styleString());
                        }
                        openedLists.pop();
                    }
                    break;
                case CMD_ENDMAPREF:
                case CMD_ENDTOPICREF:
                    if (closeCommand(cmd))
                        ditarefs_.pop();
                    break;
                case CMD_ENDOMIT:
                    closeCommand(cmd);
                    break;
                case CMD_ENDQUOTATION:
                    if (closeCommand(cmd)) {
                        leavePara();
                        append(Atom::QuotationRight);
                    }
                    break;
                case CMD_ENDRAW:
                    location().warning(tr("Unexpected '\\%1'").arg(cmdName(CMD_ENDRAW)));
                    break;
                case CMD_ENDSECTION1:
                    endSection(Doc::Section1, cmd);
                    break;
                case CMD_ENDSECTION2:
                    endSection(Doc::Section2, cmd);
                    break;
                case CMD_ENDSECTION3:
                    endSection(Doc::Section3, cmd);
                    break;
                case CMD_ENDSECTION4:
                    endSection(Doc::Section4, cmd);
                    break;
                case CMD_ENDSIDEBAR:
                    if (closeCommand(cmd)) {
                        leavePara();
                        append(Atom::SidebarRight);
                    }
                    break;
                case CMD_ENDTABLE:
                    if (closeCommand(cmd)) {
                        leaveTableRow();
                        append(Atom::TableRight);
                    }
                    break;
                case CMD_FOOTNOTE:
                    if (openCommand(cmd)) {
                        enterPara();
                        append(Atom::FootnoteLeft);
                        paraState = OutsideParagraph;
                    }
                    break;
                case CMD_ANNOTATEDLIST:
                    append(Atom::AnnotatedList, getArgument());
                    break;
                case CMD_SINCELIST:
                    append(Atom::SinceList, getRestOfLine().simplified());
                    break;
                case CMD_GENERATELIST: {
                        QString arg1 = getArgument();
                        QString arg2 = getOptionalArgument();
                        if (!arg2.isEmpty())
                            arg1 += " " + arg2;
                        append(Atom::GeneratedList, arg1);
                    }
                    break;
                case CMD_GRANULARITY:
                    priv->constructExtra();
                    priv->extra->granularity_ = getSectioningUnit();
                    break;
                case CMD_HEADER:
                    if (openedCommands.top() == CMD_TABLE) {
                        leaveTableRow();
                        append(Atom::TableHeaderLeft);
                        inTableHeader = true;
                    } else {
                        if (openedCommands.contains(CMD_TABLE))
                            location().warning(tr("Cannot use '\\%1' within '\\%2'")
                                               .arg(cmdName(CMD_HEADER))
                                               .arg(cmdName(openedCommands.top())));
                        else
                            location().warning(tr("Cannot use '\\%1' outside of '\\%2'")
                                               .arg(cmdName(CMD_HEADER))
                                               .arg(cmdName(CMD_TABLE)));
                    }
                    break;
                case CMD_I:
                    location().warning(tr("'\\i' is deprecated. Use '\\e' for italic or '\\li' for list item"));
                    Q_FALLTHROUGH();
                case CMD_E:
                    startFormat(ATOM_FORMATTING_ITALIC, cmd);
                    break;
                case CMD_HR:
                    leavePara();
                    append(Atom::HR);
                    break;
                case CMD_IF:
                    preprocessorSkipping.push(!Tokenizer::isTrue(getRestOfLine()));
                    if (preprocessorSkipping.top())
                        ++numPreprocessorSkipping;
                    if (numPreprocessorSkipping)
                        skipToNextPreprocessorCommand();
                    break;
                case CMD_IMAGE:
                    leaveValueList();
                    append(Atom::Image, getArgument());
                    append(Atom::ImageText, getRestOfLine());
                    break;
                case CMD_IMPORTANT:
                    leavePara();
                    enterPara(Atom::ImportantLeft, Atom::ImportantRight);
                    break;
                case CMD_INCLUDE:
                case CMD_INPUT: {
                    QString fileName = getArgument();
                    QString identifier = getRestOfLine();
                    include(fileName, identifier);
                    break;
                }
                case CMD_INLINEIMAGE:
                    enterPara();
                    append(Atom::InlineImage, getArgument());
                    append(Atom::ImageText, getRestOfLine());
                    append(Atom::String, " ");
                    break;
                case CMD_INDEX:
                    if (paraState == OutsideParagraph) {
                        enterPara();
                        indexStartedPara = true;
                    } else {
                        const Atom *last = priv->text.lastAtom();
                        if (indexStartedPara &&
                                (last->type() != Atom::FormattingRight ||
                                 last->string() != ATOM_FORMATTING_INDEX))
                            indexStartedPara = false;
                    }
                    startFormat(ATOM_FORMATTING_INDEX, cmd);
                    break;
                case CMD_KEYWORD:
                    insertTarget(getRestOfLine(),true);
                    break;
                case CMD_L:
                    enterPara();
                    if (isLeftBracketAhead())
                        p2 = getBracketedArgument();
                    if (isLeftBraceAhead()) {
                        p1 = getArgument();
                        append(p1, p2);
                        if (!p2.isEmpty() && !(priv->text.lastAtom()->error().isEmpty()))
                            location().warning(tr("Check parameter in '[ ]' of '\\l' command: '%1', "
                                                  "possible misspelling, or unrecognized module name")
                                               .arg(priv->text.lastAtom()->error()));
                        if (isLeftBraceAhead()) {
                            currentLinkAtom = priv->text.lastAtom();
                            startFormat(ATOM_FORMATTING_LINK, cmd);
                        }
                        else {
                            append(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
                            append(Atom::String, cleanLink(p1));
                            append(Atom::FormattingRight, ATOM_FORMATTING_LINK);
                        }
                    } else {
                        p1 = getArgument();
                        append(p1, p2);
                        if (!p2.isEmpty() && !(priv->text.lastAtom()->error().isEmpty()))
                            location().warning(tr("Check parameter in '[ ]' of '\\l' command: '%1', "
                                                  "possible misspelling, or unrecognized module name")
                                               .arg(priv->text.lastAtom()->error()));
                        append(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
                        append(Atom::String, cleanLink(p1));
                        append(Atom::FormattingRight, ATOM_FORMATTING_LINK);
                    }
                    p2.clear();
                    break;
                case CMD_LEGALESE:
                    leavePara();
                    if (openCommand(cmd))
                        append(Atom::LegaleseLeft);
                    docPrivate->hasLegalese = true;
                    break;
                case CMD_LINK:
                    if (openCommand(cmd)) {
                        enterPara();
                        p1 = getArgument();
                        append(p1);
                        append(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
                        skipSpacesOrOneEndl();
                    }
                    break;
                case CMD_LIST:
                    if (openCommand(cmd)) {
                        leavePara();
                        openedLists.push(OpenedList(location(),
                                                    getOptionalArgument()));
                    }
                    break;
                case CMD_TOPICREF:
                case CMD_MAPREF:
                    if (openCommand(cmd)) {
                        DitaRef* t = 0;
                        if (cmd == CMD_MAPREF)
                            t = new MapRef();
                        else
                            t = new TopicRef();
                        t->setNavtitle(getArgument(true));
                        if (cmd == CMD_MAPREF)
                            t->setHref(getArgument());
                        else
                            t->setHref(getOptionalArgument());
                        if (ditarefs_.isEmpty())
                            priv->ditamap_.append(t);
                        else
                            ditarefs_.top()->appendSubref(t);
                        ditarefs_.push(t);
                    }
                    break;
                case CMD_META:
                    priv->constructExtra();
                    p1 = getArgument();
                    priv->extra->metaMap_.insert(p1, getArgument());
                    break;
                case CMD_NEWCODE:
                    location().warning(tr("Unexpected '\\%1'").arg(cmdName(CMD_NEWCODE)));
                    break;
                case CMD_NOTE:
                    leavePara();
                    enterPara(Atom::NoteLeft, Atom::NoteRight);
                    break;
                case CMD_O:
                    location().warning(tr("'\\o' is deprecated. Use '\\li'"));
                    Q_FALLTHROUGH();
                case CMD_LI:
                    leavePara();
                    if (openedCommands.top() == CMD_LIST) {
                        if (openedLists.top().isStarted())
                            append(Atom::ListItemRight, openedLists.top().styleString());
                        else
                            append(Atom::ListLeft, openedLists.top().styleString());
                        openedLists.top().next();
                        append(Atom::ListItemNumber, openedLists.top().numberString());
                        append(Atom::ListItemLeft, openedLists.top().styleString());
                        enterPara();
                    } else if (openedCommands.top() == CMD_TABLE) {
                        p1 = "1,1";
                        p2.clear();
                        if (isLeftBraceAhead()) {
                            p1 = getArgument();
                            if (isLeftBraceAhead())
                                p2 = getArgument();
                        }

                        if (!inTableHeader && !inTableRow) {
                            location().warning(tr("Missing '\\%1' or '\\%2' before '\\%3'")
                                               .arg(cmdName(CMD_HEADER))
                                               .arg(cmdName(CMD_ROW))
                                               .arg(cmdName(CMD_LI)));
                            append(Atom::TableRowLeft);
                            inTableRow = true;
                        } else if (inTableItem) {
                            append(Atom::TableItemRight);
                            inTableItem = false;
                        }

                        append(Atom::TableItemLeft, p1, p2);
                        inTableItem = true;
                    } else
                        location().warning(tr("Command '\\%1' outside of '\\%2' and '\\%3'")
                                           .arg(cmdName(cmd))
                                           .arg(cmdName(CMD_LIST))
                                           .arg(cmdName(CMD_TABLE)));
                    break;
                case CMD_OLDCODE:
                    leavePara();
                    append(Atom::CodeOld, getCode(CMD_OLDCODE, marker));
                    append(Atom::CodeNew, getCode(CMD_NEWCODE, marker));
                    break;
                case CMD_OMIT:
                    getUntilEnd(cmd);
                    break;
                case CMD_OMITVALUE:
                    p1 = getArgument();
                    if (!priv->enumItemList.contains(p1))
                        priv->enumItemList.append(p1);
                    if (!priv->omitEnumItemList.contains(p1))
                        priv->omitEnumItemList.append(p1);
                    break;
                case CMD_PRINTLINE: {
                    leavePara();
                    QString rest = getRestOfLine();
                    if (quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, rest);
                    }
                    appendToCode(quoter.quoteLine(location(), cmdStr, rest));
                    break;
                }
                case CMD_PRINTTO: {
                    leavePara();
                    QString rest = getRestOfLine();
                    if (quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, rest);
                    }
                    appendToCode(quoter.quoteTo(location(), cmdStr, rest));
                    break;
                }
                case CMD_PRINTUNTIL: {
                    leavePara();
                    QString rest = getRestOfLine();
                    if (quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, rest);
                    }
                    appendToCode(quoter.quoteUntil(location(), cmdStr, rest));
                    break;
                }
                case CMD_QUOTATION:
                    if (openCommand(cmd)) {
                        leavePara();
                        append(Atom::QuotationLeft);
                    }
                    break;
                case CMD_QUOTEFILE: {
                    leavePara();
                    QString fileName = getArgument();
                    Doc::quoteFromFile(location(), quoter, fileName);
                    if (quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, fileName);
                    }
                    append(Atom::Code, quoter.quoteTo(location(), cmdStr, QString()));
                    quoter.reset();
                    break;
                }
                case CMD_QUOTEFROMFILE: {
                    leavePara();
                    QString arg = getArgument();
                    if (quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, arg);
                    }
                    Doc::quoteFromFile(location(), quoter, arg);
                    break;
                }
                case CMD_QUOTEFUNCTION: {
                    leavePara();
                    marker = quoteFromFile();
                    p1 = getRestOfLine();
                    if (quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, slashed(marker->functionEndRegExp(p1)));
                    }
                    quoter.quoteTo(location(), cmdStr, slashed(marker->functionBeginRegExp(p1)));
                    append(Atom::Code, quoter.quoteUntil(location(), cmdStr, slashed(marker->functionEndRegExp(p1))));
                    quoter.reset();
                    break;
                }
                case CMD_RAW:
                    leavePara();
                    p1 = getRestOfLine();
                    if (p1.isEmpty())
                        location().warning(tr("Missing format name after '\\%1'")
                                           .arg(cmdName(CMD_RAW)));
                    append(Atom::FormatIf, p1);
                    append(Atom::RawString, untabifyEtc(getUntilEnd(cmd)));
                    append(Atom::FormatElse);
                    append(Atom::FormatEndif);
                    break;
                case CMD_ROW:
                    if (openedCommands.top() == CMD_TABLE) {
                        p1.clear();
                        if (isLeftBraceAhead())
                            p1 = getArgument(true);
                        leaveTableRow();
                        append(Atom::TableRowLeft,p1);
                        inTableRow = true;
                    } else {
                        if (openedCommands.contains(CMD_TABLE))
                            location().warning(tr("Cannot use '\\%1' within '\\%2'")
                                               .arg(cmdName(CMD_ROW))
                                               .arg(cmdName(openedCommands.top())));
                        else
                            location().warning(tr("Cannot use '\\%1' outside of '\\%2'")
                                               .arg(cmdName(CMD_ROW))
                                               .arg(cmdName(CMD_TABLE)));
                    }
                    break;
                case CMD_SA:
                    parseAlso();
                    break;
                case CMD_SECTION1:
                    startSection(Doc::Section1, cmd);
                    break;
                case CMD_SECTION2:
                    startSection(Doc::Section2, cmd);
                    break;
                case CMD_SECTION3:
                    startSection(Doc::Section3, cmd);
                    break;
                case CMD_SECTION4:
                    startSection(Doc::Section4, cmd);
                    break;
                case CMD_SIDEBAR:
                    if (openCommand(cmd)) {
                        leavePara();
                        append(Atom::SidebarLeft);
                    }
                    break;
                case CMD_SKIPLINE: {
                    leavePara();
                    QString rest = getRestOfLine();
                    if (quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, rest);
                    }
                    quoter.quoteLine(location(), cmdStr, rest);
                    break;
                }
                case CMD_SKIPTO: {
                    leavePara();
                    QString rest = getRestOfLine();
                    if (quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, rest);
                    }
                    quoter.quoteTo(location(), cmdStr, rest);
                    break;
                }
                case CMD_SKIPUNTIL: {
                    leavePara();
                    QString rest = getRestOfLine();
                    if (quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, rest);
                    }
                    quoter.quoteUntil(location(), cmdStr, rest);
                    break;
                }
                case CMD_SPAN:
                    p1 = ATOM_FORMATTING_SPAN + getArgument(true);
                    startFormat(p1, cmd);
                    break;
                case CMD_SNIPPET: {
                    leavePara();
                    QString snippet = getArgument();
                    QString identifier = getRestOfLine();
                    if (quoting) {
                        append(Atom::SnippetCommand, cmdStr);
                        append(Atom::SnippetLocation, snippet);
                        append(Atom::SnippetIdentifier, identifier);
                    }
                    marker = Doc::quoteFromFile(location(), quoter, snippet);
                    appendToCode(quoter.quoteSnippet(location(), identifier), marker->atomType());
                    break;
                }
                case CMD_SUB:
                    startFormat(ATOM_FORMATTING_SUBSCRIPT, cmd);
                    break;
                case CMD_SUP:
                    startFormat(ATOM_FORMATTING_SUPERSCRIPT, cmd);
                    break;
                case CMD_TABLE:
                    p1 = getOptionalArgument();
                    p2 = getOptionalArgument();
                    if (openCommand(cmd)) {
                        leavePara();
                        append(Atom::TableLeft, p1, p2);
                        inTableHeader = false;
                        inTableRow = false;
                        inTableItem = false;
                    }
                    break;
                case CMD_TABLEOFCONTENTS:
                    p1 = "1";
                    if (isLeftBraceAhead())
                        p1 = getArgument();
                    p1 += QLatin1Char(',');
                    p1 += QString::number((int)getSectioningUnit());
                    append(Atom::TableOfContents, p1);
                    break;
                case CMD_TARGET:
                    insertTarget(getRestOfLine(),false);
                    break;
                case CMD_TT:
                    startFormat(ATOM_FORMATTING_TELETYPE, cmd);
                    break;
                case CMD_UICONTROL:
                    startFormat(ATOM_FORMATTING_UICONTROL, cmd);
                    break;
                case CMD_UNDERLINE:
                    startFormat(ATOM_FORMATTING_UNDERLINE, cmd);
                    break;
                case CMD_UNICODE: {
                    enterPara();
                    p1 = getArgument();
                    bool ok;
                    uint unicodeChar = p1.toUInt(&ok, 0);
                    if (!ok || (unicodeChar == 0x0000) || (unicodeChar > 0xFFFE))
                        location().warning(tr("Invalid Unicode character '%1' specified with '%2'")
                                           .arg(p1, cmdName(CMD_UNICODE)));
                    else
                        append(Atom::String, QChar(unicodeChar));
                    break;
                }
                case CMD_VALUE:
                    leaveValue();
                    if (openedLists.top().style() == OpenedList::Value) {
                        QString p2;
                        p1 = getArgument();
                        if (p1.startsWith(QLatin1String("[since ")) && p1.endsWith(QLatin1String("]"))) {
                            p2 = p1.mid(7, p1.length() - 8);
                            p1 = getArgument();
                        }
                        if (!priv->enumItemList.contains(p1))
                            priv->enumItemList.append(p1);

                        openedLists.top().next();
                        append(Atom::ListTagLeft, ATOM_LIST_VALUE);
                        append(Atom::String, p1);
                        append(Atom::ListTagRight, ATOM_LIST_VALUE);
                        if (!p2.isEmpty()) {
                            append(Atom::SinceTagLeft, ATOM_LIST_VALUE);
                            append(Atom::String, p2);
                            append(Atom::SinceTagRight, ATOM_LIST_VALUE);
                        }
                        append(Atom::ListItemLeft, ATOM_LIST_VALUE);

                        skipSpacesOrOneEndl();
                        if (isBlankLine())
                            append(Atom::Nop);
                    } else {
                        // ### unknown problems
                    }
                    break;
                case CMD_WARNING:
                    leavePara();
                    enterPara();
                    append(Atom::FormattingLeft, ATOM_FORMATTING_BOLD);
                    append(Atom::String, "Warning:");
                    append(Atom::FormattingRight, ATOM_FORMATTING_BOLD);
                    append(Atom::String, " ");
                    break;
                case CMD_OVERLOAD:
                    priv->metacommandsUsed.insert(cmdStr);
                    p1.clear();
                    if (!isBlankLine())
                        p1 = getRestOfLine();
                    if (!p1.isEmpty()) {
                        append(Atom::ParaLeft);
                        append(Atom::String, "This function overloads ");
                        append(Atom::AutoLink,p1);
                        append(Atom::String, ".");
                        append(Atom::ParaRight);
                    } else {
                        append(Atom::ParaLeft);
                        append(Atom::String,"This is an overloaded function.");
                        append(Atom::ParaRight);
                        p1 = getMetaCommandArgument(cmdStr);
                    }
                    priv->metaCommandMap[cmdStr].append(ArgLocPair(p1,location()));
                    break;
                case NOT_A_CMD:
                    if (metaCommandSet.contains(cmdStr)) {
                        priv->metacommandsUsed.insert(cmdStr);
                        QString arg = getMetaCommandArgument(cmdStr);
                        priv->metaCommandMap[cmdStr].append(ArgLocPair(arg,location()));
                        if (possibleTopics.contains(cmdStr)) {
                            priv->topics_.append(Topic(cmdStr,arg));
                        }
                    } else if (macroHash()->contains(cmdStr)) {
                        const Macro &macro = macroHash()->value(cmdStr);
                        int numPendingFi = 0;
                        QStringMap::ConstIterator d;
                        int numFormatDefs = 0;
                        QString matchExpr;
                        d = macro.otherDefs.constBegin();
                        while (d != macro.otherDefs.constEnd()) {
                            if (d.key() == "match") {
                                matchExpr = d.value();
                                ++d;
                            } else {
                                append(Atom::FormatIf, d.key());
                                expandMacro(cmdStr, *d, macro.numParams);
                                ++d;
                                ++numFormatDefs;
                                if (d == macro.otherDefs.constEnd()) {
                                    append(Atom::FormatEndif);
                                } else {
                                    append(Atom::FormatElse);
                                    numPendingFi++;
                                }
                            }
                        }
                        while (numPendingFi-- > 0)
                            append(Atom::FormatEndif);

                        if (!macro.defaultDef.isEmpty()) {
                            if (numFormatDefs > 0) {
                                macro.defaultDefLocation.warning(
                                            tr("Macro cannot have both "
                                               "format-specific and qdoc-"
                                               "syntax definitions"));
                            } else {
                                QString expanded = expandMacroToString(cmdStr,
                                                                       macro.defaultDef,
                                                                       macro.numParams,
                                                                       matchExpr);
                                input_.replace(backslashPos, endPos - backslashPos, expanded);
                                len = input_.length();
                                pos = backslashPos;
                            }
                        }
                    } else {
                        int curPos = 0;
                        int numUppercase = 0;
                        int numLowercase = 0;
                        int numStrangeSymbols = 0;

                        while (curPos < cmdStr.size()) {
                            unsigned char latin1Ch = cmdStr.at(curPos).toLatin1();
                            if (islower(latin1Ch)) {
                                ++numLowercase;
                                ++curPos;
                            } else if (isupper(latin1Ch)) {
                                ++numUppercase;
                                ++curPos;
                            } else if (isdigit(latin1Ch)) {
                                if (curPos > 0)
                                    ++curPos;
                                else
                                    break;
                            } else if (latin1Ch == '_' || latin1Ch == '@') {
                                ++numStrangeSymbols;
                                ++curPos;
                            } else if ((latin1Ch == ':') &&
                                       (curPos < cmdStr.size() - 1) &&
                                       (cmdStr.at(curPos + 1) == QLatin1Char(':'))) {
                                ++numStrangeSymbols;
                                curPos += 2;
                            } else if (latin1Ch == '(') {
                                if (curPos > 0) {
                                    if ((curPos < cmdStr.size() - 1) &&
                                        (cmdStr.at(curPos + 1) == QLatin1Char(')'))) {
                                        ++numStrangeSymbols;
                                        pos += 2;
                                        break;
                                    } else {
                                        // ### handle functions with signatures
                                        // and function calls
                                        break;
                                    }
                                } else {
                                    break;
                                }
                            } else {
                                break;
                            }
                        }
                        if ((numUppercase >= 1 && numLowercase >= 2) || numStrangeSymbols > 0) {
                            qDebug() << "APPENDING: " << cmdStr;
                            appendWord(cmdStr);
                        } else {
                            location().warning(tr("Unknown command '\\%1'").arg(cmdStr),
                                               detailsUnknownCommand(metaCommandSet,cmdStr));
                            enterPara();
                            append(Atom::UnknownCommand, cmdStr);
                        }
                    }
                }
            } // case '\\' (qdoc markup command)
            break;
        }
        case '{':
            enterPara();
            appendChar('{');
            braceDepth++;
            pos++;
            break;
        case '}': {
            braceDepth--;
            pos++;

            QMap<int, QString>::Iterator f = pendingFormats.find(braceDepth);
            if (f == pendingFormats.end()) {
                enterPara();
                appendChar('}');
            } else {
                append(Atom::FormattingRight, *f);
                if (*f == ATOM_FORMATTING_INDEX) {
                    if (indexStartedPara)
                        skipAllSpaces();
                } else if (*f == ATOM_FORMATTING_LINK) {
                    // hack for C++ to support links like
                    // \l{QString::}{count()}
                    if (currentLinkAtom &&
                            currentLinkAtom->string().endsWith("::")) {
                        QString suffix = Text::subText(currentLinkAtom,
                                                       priv->text.lastAtom()).toString();
                        currentLinkAtom->appendString(suffix);
                    }
                    currentLinkAtom = 0;
                }
                pendingFormats.erase(f);
            }
            break;
        }
        // Do not parse content after '//!' comments
        case '/': {
            if (pos + 2 < len)
                if (input_.at(pos + 1) == '/')
                    if (input_.at(pos + 2) == '!') {
                        pos += 2;
                        getRestOfLine();
                        break;
                    }
            Q_FALLTHROUGH(); // fall through
        }
        default: {
            bool newWord;
            switch (priv->text.lastAtom()->type()) {
            case Atom::ParaLeft:
                newWord = true;
                break;
            default:
                newWord = false;
            }

            if (paraState == OutsideParagraph) {
                if (ch.isSpace()) {
                    ++pos;
                    newWord = false;
                } else {
                    enterPara();
                    newWord = true;
                }
            } else {
                if (ch.isSpace()) {
                    ++pos;
                    if ((ch == '\n') &&
                            (paraState == InSingleLineParagraph ||
                             isBlankLine())) {
                        leavePara();
                        newWord = false;
                    } else {
                        appendChar(' ');
                        newWord = true;
                    }
                } else {
                    newWord = true;
                }
            }

            if (newWord) {
                int startPos = pos;
                int numInternalUppercase = 0;
                int numLowercase = 0;
                int numStrangeSymbols = 0;

                while (pos < len) {
                    unsigned char latin1Ch = input_.at(pos).toLatin1();
                    if (islower(latin1Ch)) {
                        ++numLowercase;
                        ++pos;
                    } else if (isupper(latin1Ch)) {
                        if (pos > startPos)
                            ++numInternalUppercase;
                        ++pos;
                    } else if (isdigit(latin1Ch)) {
                        if (pos > startPos)
                            ++pos;
                        else
                            break;
                    } else if (latin1Ch == '_' || latin1Ch == '@') {
                        ++numStrangeSymbols;
                        ++pos;
                    } else if (latin1Ch == ':' && pos < len - 1 && input_.at(pos + 1) == QLatin1Char(':')) {
                        ++numStrangeSymbols;
                        pos += 2;
                    } else if (latin1Ch == '(') {
                        if (pos > startPos) {
                            if (pos < len - 1 && input_.at(pos + 1) == QLatin1Char(')')) {
                                ++numStrangeSymbols;
                                pos += 2;
                                break;
                            } else {
                                // ### handle functions with signatures
                                // and function calls
                                break;
                            }
                        } else {
                            break;
                        }
                    } else {
                        break;
                    }
                }

                if (pos == startPos) {
                    if (!ch.isSpace()) {
                        appendChar(ch);
                        ++pos;
                    }
                } else {
                    QString word = input_.mid(startPos, pos - startPos);
                    // is word a C++ symbol or an English word?
                    if ((numInternalUppercase >= 1 && numLowercase >= 2) || numStrangeSymbols > 0) {
                        if (word.startsWith(QString("__")))
                            appendWord(word);
                        else
                            append(Atom::AutoLink, word);
                    }
                    else {
                        appendWord(word);
                    }
                }
            }
        } // default:
        } // switch (ch.unicode())
    }
    leaveValueList();

    // for compatibility
    if (openedCommands.top() == CMD_LEGALESE) {
        append(Atom::LegaleseRight);
        openedCommands.pop();
    }

    if (openedCommands.top() != CMD_OMIT) {
        location().warning(tr("Missing '\\%1'").arg(endCmdName(openedCommands.top())));
    }
    else if (preprocessorSkipping.count() > 0) {
        location().warning(tr("Missing '\\%1'").arg(cmdName(CMD_ENDIF)));
    }

    if (currentSection > Doc::NoSection) {
        append(Atom::SectionRight, QString::number(currentSection));
        currentSection = Doc::NoSection;
    }

    if (priv->extra && priv->extra->granularity_ < priv->extra->section_)
        priv->extra->granularity_ = priv->extra->section_;
    priv->text.stripFirstAtom();
}

/*!
  Returns the current location.
 */
Location &DocParser::location()
{
    while (!openedInputs.isEmpty() && openedInputs.top() <= pos) {
        cachedLoc.pop();
        cachedPos = openedInputs.pop();
    }
    while (cachedPos < pos)
        cachedLoc.advance(input_.at(cachedPos++));
    return cachedLoc;
}

QString DocParser::detailsUnknownCommand(const QSet<QString> &metaCommandSet,
                                         const QString &str)
{
    QSet<QString> commandSet = metaCommandSet;
    int i = 0;
    while (cmds[i].english != 0) {
        commandSet.insert(*cmds[i].alias);
        i++;
    }

    if (aliasMap()->contains(str))
        return tr("The command '\\%1' was renamed '\\%2' by the configuration"
                  " file. Use the new name.")
                .arg(str).arg((*aliasMap())[str]);

    QString best = nearestName(str, commandSet);
    if (best.isEmpty())
        return QString();
    return tr("Maybe you meant '\\%1'?").arg(best);
}

void DocParser::insertTarget(const QString &target, bool keyword)
{
    if (targetMap_.contains(target)) {
        location().warning(tr("Duplicate target name '%1'").arg(target));
        targetMap_[target].warning(tr("(The previous occurrence is here)"));
    }
    else {
        targetMap_.insert(target, location());
        priv->constructExtra();
        if (keyword) {
            append(Atom::Keyword, target);
            priv->extra->keywords_.append(priv->text.lastAtom());
        }
        else {
            append(Atom::Target, target);
            priv->extra->targets_.append(priv->text.lastAtom());
        }
    }
}

void DocParser::include(const QString& fileName, const QString& identifier)
{
    if (location().depth() > 16)
        location().fatal(tr("Too many nested '\\%1's").arg(cmdName(CMD_INCLUDE)));

    QString userFriendlyFilePath;
    QString filePath = Doc::config()->getIncludeFilePath(fileName);
#if 0
    QString filePath = Config::findFile(location(),
                                        sourceFiles,
                                        sourceDirs,
                                        fileName,
                                        userFriendlyFilePath);
#endif
    if (filePath.isEmpty()) {
        location().warning(tr("Cannot find qdoc include file '%1'").arg(fileName));
    }
    else {
        QFile inFile(filePath);
        if (!inFile.open(QFile::ReadOnly)) {
            location().warning(tr("Cannot open qdoc include file '%1'")
                               .arg(userFriendlyFilePath));
        }
        else {
            location().push(userFriendlyFilePath);

            QTextStream inStream(&inFile);
            QString includedStuff = inStream.readAll();
            inFile.close();

            if (identifier.isEmpty()) {
                input_.insert(pos, includedStuff);
                len = input_.length();
                openedInputs.push(pos + includedStuff.length());
            }
            else {
                QStringList lineBuffer = includedStuff.split(QLatin1Char('\n'));
                int i = 0;
                int startLine = -1;
                while (i < lineBuffer.size()) {
                    if (lineBuffer[i].startsWith("//!")) {
                        if (lineBuffer[i].contains(identifier)) {
                            startLine = i+1;
                            break;
                        }
                    }
                    ++i;
                }
                if (startLine < 0) {
                    location().warning(tr("Cannot find '%1' in '%2'")
                                       .arg(identifier)
                                       .arg(userFriendlyFilePath));
                    return;

                }
                QString result;
                i = startLine;
                do {
                    if (lineBuffer[i].startsWith("//!")) {
                        if (i<lineBuffer.size()) {
                            if (lineBuffer[i].contains(identifier)) {
                                break;
                            }
                        }
                    }
                    else
                        result += lineBuffer[i] + QLatin1Char('\n');
                    ++i;
                } while (i < lineBuffer.size());
                if (result.isEmpty()) {
                    location().warning(tr("Empty qdoc snippet '%1' in '%2'")
                                       .arg(identifier)
                                       .arg(userFriendlyFilePath));
                }
                else {
                    input_.insert(pos, result);
                    len = input_.length();
                    openedInputs.push(pos + result.length());
                }
            }
        }
    }
}

void DocParser::startFormat(const QString& format, int cmd)
{
    enterPara();

    QMap<int, QString>::ConstIterator f = pendingFormats.constBegin();
    while (f != pendingFormats.constEnd()) {
        if (*f == format) {
            location().warning(tr("Cannot nest '\\%1' commands")
                               .arg(cmdName(cmd)));
            return;
        }
        ++f;
    }

    append(Atom::FormattingLeft, format);

    if (isLeftBraceAhead()) {
        skipSpacesOrOneEndl();
        pendingFormats.insert(braceDepth, format);
        ++braceDepth;
        ++pos;
    }
    else {
        append(Atom::String, getArgument());
        append(Atom::FormattingRight, format);
        if (format == ATOM_FORMATTING_INDEX && indexStartedPara) {
            skipAllSpaces();
            indexStartedPara = false;
        }
    }
}

bool DocParser::openCommand(int cmd)
{
    int outer = openedCommands.top();
    bool ok = true;

    if (cmd != CMD_LINK) {
        if (outer == CMD_LIST) {
            ok = (cmd == CMD_FOOTNOTE || cmd == CMD_LIST);
        }
        else if (outer == CMD_SIDEBAR) {
            ok = (cmd == CMD_LIST ||
                  cmd == CMD_QUOTATION ||
                  cmd == CMD_SIDEBAR);
        }
        else if (outer == CMD_QUOTATION) {
            ok = (cmd == CMD_LIST);
        }
        else if (outer == CMD_TABLE) {
            ok = (cmd == CMD_LIST ||
                  cmd == CMD_FOOTNOTE ||
                  cmd == CMD_QUOTATION);
        }
        else if (outer == CMD_FOOTNOTE || outer == CMD_LINK) {
            ok = false;
        }
        else if (outer == CMD_TOPICREF)
            ok = (cmd == CMD_TOPICREF || cmd == CMD_MAPREF);
        else if (outer == CMD_MAPREF)
            ok = false;
    }

    if (ok) {
        openedCommands.push(cmd);
    }
    else {
        location().warning(tr("Can't use '\\%1' in '\\%2'").arg(cmdName(cmd)).arg(cmdName(outer)));
    }
    return ok;
}

bool DocParser::closeCommand(int endCmd)
{
    if (endCmdFor(openedCommands.top()) == endCmd && openedCommands.size() > 1) {
        openedCommands.pop();
        return true;
    }
    else {
        bool contains = false;
        QStack<int> opened2 = openedCommands;
        while (opened2.size() > 1) {
            if (endCmdFor(opened2.top()) == endCmd) {
                contains = true;
                break;
            }
            opened2.pop();
        }

        if (contains) {
            while (endCmdFor(openedCommands.top()) != endCmd && openedCommands.size() > 1) {
                location().warning(tr("Missing '\\%1' before '\\%2'")
                                   .arg(endCmdName(openedCommands.top()))
                                   .arg(cmdName(endCmd)));
                openedCommands.pop();
            }
        }
        else {
            location().warning(tr("Unexpected '\\%1'").arg(cmdName(endCmd)));
        }
        return false;
    }
}

void DocParser::startSection(Doc::Sections unit, int cmd)
{
    leaveValueList();

    if (currentSection == Doc::NoSection) {
        currentSection = (Doc::Sections) (unit);
        priv->constructExtra();
        priv->extra->section_ = currentSection;
    }
    else
        endSection(unit,cmd);

    append(Atom::SectionLeft, QString::number(unit));
    priv->constructExtra();
    priv->extra->tableOfContents_.append(priv->text.lastAtom());
    priv->extra->tableOfContentsLevels_.append(unit);
    enterPara(Atom::SectionHeadingLeft,
              Atom::SectionHeadingRight,
              QString::number(unit));
    currentSection = unit;

}

void DocParser::endSection(int , int) // (int unit, int endCmd)
{
    leavePara();
    append(Atom::SectionRight, QString::number(currentSection));
    currentSection = (Doc::NoSection);
}

void DocParser::parseAlso()
{
    leavePara();
    skipSpacesOnLine();
    while (pos < len && input_[pos] != '\n') {
        QString target;
        QString str;

        if (input_[pos] == '{') {
            target = getArgument();
            skipSpacesOnLine();
            if (input_[pos] == '{') {
                str = getArgument();

                // hack for C++ to support links like \l{QString::}{count()}
                if (target.endsWith("::"))
                    target += str;
            }
            else {
                str = target;
            }
        }
        else {
            target = getArgument();
            str = cleanLink(target);
        }

        Text also;
        also << Atom(Atom::Link, target)
             << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
             << str
             << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        priv->addAlso(also);

        skipSpacesOnLine();
        if (pos < len && input_[pos] == ',') {
            pos++;
            skipSpacesOrOneEndl();
        }
        else if (input_[pos] != '\n') {
            location().warning(tr("Missing comma in '\\%1'").arg(cmdName(CMD_SA)));
        }
    }
}

void DocParser::append(Atom::AtomType type, const QString &string)
{
    Atom::AtomType lastType = priv->text.lastAtom()->type();
    if ((lastType == Atom::Code) && priv->text.lastAtom()->string().endsWith(QLatin1String("\n\n")))
        priv->text.lastAtom()->chopString();
    priv->text << Atom(type, string);
}

void DocParser::append(const QString &string)
{
    Atom::AtomType lastType = priv->text.lastAtom()->type();
    if ((lastType == Atom::Code) && priv->text.lastAtom()->string().endsWith(QLatin1String("\n\n")))
        priv->text.lastAtom()->chopString();
    priv->text << Atom(string); // The Atom type is Link.
}

void DocParser::append(Atom::AtomType type, const QString& p1, const QString& p2)
{
    Atom::AtomType lastType = priv->text.lastAtom()->type();
    if ((lastType == Atom::Code) && priv->text.lastAtom()->string().endsWith(QLatin1String("\n\n")))
        priv->text.lastAtom()->chopString();
    priv->text << Atom(type, p1, p2);
}

void DocParser::append(const QString& p1, const QString& p2)
{
    Atom::AtomType lastType = priv->text.lastAtom()->type();
    if ((lastType == Atom::Code) && priv->text.lastAtom()->string().endsWith(QLatin1String("\n\n")))
        priv->text.lastAtom()->chopString();
    if (p2.isEmpty())
        priv->text << Atom(p1); // The Atom type is Link.
    else
        priv->text << LinkAtom(p1, p2);
}

void DocParser::appendChar(QChar ch)
{
    if (priv->text.lastAtom()->type() != Atom::String)
        append(Atom::String);
    Atom *atom = priv->text.lastAtom();
    if (ch == QLatin1Char(' ')) {
        if (!atom->string().endsWith(QLatin1Char(' ')))
            atom->appendChar(QLatin1Char(' '));
    }
    else
        atom->appendChar(ch);
}

void DocParser::appendWord(const QString &word)
{
    if (priv->text.lastAtom()->type() != Atom::String) {
        append(Atom::String, word);
    }
    else
        priv->text.lastAtom()->appendString(word);
}

void DocParser::appendToCode(const QString& markedCode)
{
    if (!isCode(lastAtom)) {
        append(Atom::Code);
        lastAtom = priv->text.lastAtom();
    }
    lastAtom->appendString(markedCode);
}

void DocParser::appendToCode(const QString &markedCode, Atom::AtomType defaultType)
{
    if (!isCode(lastAtom)) {
        append(defaultType, markedCode);
        lastAtom = priv->text.lastAtom();
    } else {
        lastAtom->appendString(markedCode);
    }
}

void DocParser::startNewPara()
{
    leavePara();
    enterPara();
}

void DocParser::enterPara(Atom::AtomType leftType,
                          Atom::AtomType rightType,
                          const QString& string)
{
    if (paraState == OutsideParagraph) {

        if ((priv->text.lastAtom()->type() != Atom::ListItemLeft) &&
                (priv->text.lastAtom()->type() != Atom::DivLeft)) {
            leaveValueList();
        }

        append(leftType, string);
        indexStartedPara = false;
        pendingParaLeftType = leftType;
        pendingParaRightType = rightType;
        pendingParaString = string;
        if (leftType == Atom::SectionHeadingLeft) {
            paraState = InSingleLineParagraph;
        }
        else {
            paraState = InMultiLineParagraph;
        }
        skipSpacesOrOneEndl();
    }
}

void DocParser::leavePara()
{
    if (paraState != OutsideParagraph) {
        if (!pendingFormats.isEmpty()) {
            location().warning(tr("Missing '}'"));
            pendingFormats.clear();
        }

        if (priv->text.lastAtom()->type() == pendingParaLeftType) {
            priv->text.stripLastAtom();
        }
        else {
            if (priv->text.lastAtom()->type() == Atom::String &&
                    priv->text.lastAtom()->string().endsWith(QLatin1Char(' '))) {
                priv->text.lastAtom()->chopString();
            }
            append(pendingParaRightType, pendingParaString);
        }
        paraState = OutsideParagraph;
        indexStartedPara = false;
        pendingParaRightType = Atom::Nop;
        pendingParaString.clear();
    }
}

void DocParser::leaveValue()
{
    leavePara();
    if (openedLists.isEmpty()) {
        openedLists.push(OpenedList(OpenedList::Value));
        append(Atom::ListLeft, ATOM_LIST_VALUE);
    }
    else {
        if (priv->text.lastAtom()->type() == Atom::Nop)
            priv->text.stripLastAtom();
        append(Atom::ListItemRight, ATOM_LIST_VALUE);
    }
}

void DocParser::leaveValueList()
{
    leavePara();
    if (!openedLists.isEmpty() &&
            (openedLists.top().style() == OpenedList::Value)) {
        if (priv->text.lastAtom()->type() == Atom::Nop)
            priv->text.stripLastAtom();
        append(Atom::ListItemRight, ATOM_LIST_VALUE);
        append(Atom::ListRight, ATOM_LIST_VALUE);
        openedLists.pop();
    }
}

void DocParser::leaveTableRow()
{
    if (inTableItem) {
        leavePara();
        append(Atom::TableItemRight);
        inTableItem = false;
    }
    if (inTableHeader) {
        append(Atom::TableHeaderRight);
        inTableHeader = false;
    }
    if (inTableRow) {
        append(Atom::TableRowRight);
        inTableRow = false;
    }
}

CodeMarker *DocParser::quoteFromFile()
{
    return Doc::quoteFromFile(location(), quoter, getArgument());
}

/*!
  Expands a macro in-place in input.

  Expects the current \e pos in the input to point to a backslash, and the macro to have a
  default definition. Format-specific macros are currently not expanded.

  \note In addition to macros, a valid use for a backslash in an argument include
  escaping non-alnum characters, and splitting a single argument across multiple
  lines by escaping newlines. Escaping is also handled here.

  Returns \c true on successful macro expansion.
 */
bool DocParser::expandMacro()
{
    Q_ASSERT(input_[pos].unicode() == '\\');

    QString cmdStr;
    int backslashPos = pos++;
    while (pos < (int) input_.length() && input_[pos].isLetterOrNumber())
        cmdStr += input_[pos++];

    endPos = pos;
    if (!cmdStr.isEmpty()) {
        if (macroHash()->contains(cmdStr)) {
            const Macro &macro = macroHash()->value(cmdStr);
            if (!macro.defaultDef.isEmpty()) {
                QString expanded = expandMacroToString(cmdStr,
                                                       macro.defaultDef,
                                                       macro.numParams,
                                                       macro.otherDefs.value("match"));
                input_.replace(backslashPos, pos - backslashPos, expanded);
                len = input_.length();
                pos = backslashPos;
                return true;
            } else {
                location().warning(tr("Macro '%1' does not have a default definition").arg(cmdStr));
            }
        } else {
            location().warning(tr("Unknown macro '%1'").arg(cmdStr));
            pos = ++backslashPos;
        }
    } else if (input_[pos].isSpace()) {
        skipAllSpaces();
    } else if (input_[pos].unicode() == '\\') {
        // allow escaping a backslash
        input_.remove(pos--, 1);
        --len;
    }
    return false;
}

void DocParser::expandMacro(const QString &name,
                            const QString &def,
                            int numParams)
{
    if (numParams == 0) {
        append(Atom::RawString, def);
    }
    else {
        QStringList args;
        QString rawString;

        for (int i = 0; i < numParams; i++) {
            if (numParams == 1 || isLeftBraceAhead()) {
                args << getArgument(true);
            }
            else {
                location().warning(tr("Macro '\\%1' invoked with too few"
                                      " arguments (expected %2, got %3)")
                                   .arg(name).arg(numParams).arg(i));
                numParams = i;
                break;
            }
        }

        int j = 0;
        while (j < def.size()) {
            int paramNo;
            if (((paramNo = def[j].unicode()) >= 1) &&
                    (paramNo <= numParams)) {
                if (!rawString.isEmpty()) {
                    append(Atom::RawString, rawString);
                    rawString.clear();
                }
                append(Atom::String, args[paramNo - 1]);
                j += 1;
            }
            else {
                rawString += def[j++];
            }
        }
        if (!rawString.isEmpty())
            append(Atom::RawString, rawString);
    }
}

QString DocParser::expandMacroToString(const QString &name, const QString &def, int numParams, const QString &matchExpr)
{
    QString rawString;

    if (numParams == 0) {
        rawString = def;
    } else {
        QStringList args;
        for (int i = 0; i < numParams; i++) {
            if (numParams == 1 || isLeftBraceAhead()) {
                args << getArgument(true);
            }
            else {
                location().warning(tr("Macro '\\%1' invoked with too few"
                                      " arguments (expected %2, got %3)")
                                   .arg(name).arg(numParams).arg(i));
                numParams = i;
                break;
            }
        }

        int j = 0;
        while (j < def.size()) {
            int paramNo;
            if (((paramNo = def[j].unicode()) >= 1) &&
                    (paramNo <= numParams)) {
                rawString += args[paramNo - 1];
                j += 1;
            }
            else {
                rawString += def[j++];
            }
        }
    }
    if (matchExpr.isEmpty())
        return rawString;

    QString result;
    QRegExp re(matchExpr);
    int capStart = (re.captureCount() > 0) ? 1 : 0;
    int i = 0;
    while ((i = re.indexIn(rawString, i)) != -1) {
        for (int c = capStart; c <= re.captureCount(); ++c)
            result += re.cap(c);
        i += re.matchedLength();
    }

    return result;
}

Doc::Sections DocParser::getSectioningUnit()
{
    QString name = getOptionalArgument();

    if (name == "section1") {
        return Doc::Section1;
    }
    else if (name == "section2") {
        return Doc::Section2;
    }
    else if (name == "section3") {
        return Doc::Section3;
    }
    else if (name == "section4") {
        return Doc::Section4;
    }
    else if (name.isEmpty()) {
        return Doc::NoSection;
    }
    else {
        location().warning(tr("Invalid section '%1'").arg(name));
        return Doc::NoSection;
    }
}

/*!
  Gets an argument that is enclosed in braces and returns it
  without the enclosing braces. On entry, the current character
  is the left brace. On exit, the current character is the one
  that comes after the right brace.

  If \a verbatim is true, extra whitespace is retained in the
  returned string. Otherwise, extra whitespace is removed.
 */
QString DocParser::getBracedArgument(bool verbatim)
{
    QString arg;
    int delimDepth = 0;
    if (pos < (int) input_.length() && input_[pos] == '{') {
        pos++;
        while (pos < (int) input_.length() && delimDepth >= 0) {
            switch (input_[pos].unicode()) {
            case '{':
                delimDepth++;
                arg += QLatin1Char('{');
                pos++;
                break;
            case '}':
                delimDepth--;
                if (delimDepth >= 0)
                    arg += QLatin1Char('}');
                pos++;
                break;
            case '\\':
                if (verbatim || !expandMacro())
                    arg += input_[pos++];
                break;
            default:
                if (input_[pos].isSpace() && !verbatim)
                    arg += QChar(' ');
                else
                    arg += input_[pos];
                pos++;
            }
        }
        if (delimDepth > 0)
            location().warning(tr("Missing '}'"));
    }
    endPos = pos;
    return arg;
}

/*!
  Typically, an argument ends at the next white-space. However,
  braces can be used to group words:

  {a few words}

  Also, opening and closing parentheses have to match. Thus,

  printf("%d\n", x)

  is an argument too, although it contains spaces. Finally,
  trailing punctuation is not included in an argument, nor is 's.
*/
QString DocParser::getArgument(bool verbatim)
{
    skipSpacesOrOneEndl();

    int delimDepth = 0;
    int startPos = pos;
    QString arg = getBracedArgument(verbatim);
    if (arg.isEmpty()) {
        while ((pos < input_.length()) &&
               ((delimDepth > 0) || ((delimDepth == 0) && !input_[pos].isSpace()))) {
            switch (input_[pos].unicode()) {
            case '(':
            case '[':
            case '{':
                delimDepth++;
                arg += input_[pos];
                pos++;
                break;
            case ')':
            case ']':
            case '}':
                delimDepth--;
                if (pos == startPos || delimDepth >= 0) {
                    arg += input_[pos];
                    pos++;
                }
                break;
            case '\\':
                if (verbatim || !expandMacro())
                    arg += input_[pos++];
                break;
            default:
                arg += input_[pos];
                pos++;
            }
        }
        endPos = pos;
        if ((arg.length() > 1) &&
                (QString(".,:;!?").indexOf(input_[pos - 1]) != -1) &&
                !arg.endsWith("...")) {
            arg.truncate(arg.length() - 1);
            pos--;
        }
        if (arg.length() > 2 && input_.mid(pos - 2, 2) == "'s") {
            arg.truncate(arg.length() - 2);
            pos -= 2;
        }
    }
    return arg.simplified();
}

/*!
  Gets an argument that is enclosed in brackets and returns it
  without the enclosing brackets. On entry, the current character
  is the left bracket. On exit, the current character is the one
  that comes after the right bracket.
 */
QString DocParser::getBracketedArgument()
{
    QString arg;
    int delimDepth = 0;
    skipSpacesOrOneEndl();
    if (pos < input_.length() && input_[pos] == '[') {
        pos++;
        while (pos < input_.length() && delimDepth >= 0) {
            switch (input_[pos].unicode()) {
            case '[':
                delimDepth++;
                arg += QLatin1Char('[');
                pos++;
                break;
            case ']':
                delimDepth--;
                if (delimDepth >= 0)
                    arg += QLatin1Char(']');
                pos++;
                break;
            case '\\':
                arg += input_[pos];
                pos++;
                break;
            default:
                arg += input_[pos];
                pos++;
            }
        }
        if (delimDepth > 0)
            location().warning(tr("Missing ']'"));
    }
    return arg;
}

QString DocParser::getOptionalArgument()
{
    skipSpacesOrOneEndl();
    if (pos + 1 < (int) input_.length() && input_[pos] == '\\' &&
            input_[pos + 1].isLetterOrNumber()) {
        return QString();
    }
    else {
        return getArgument();
    }
}

QString DocParser::getRestOfLine()
{
    QString t;

    skipSpacesOnLine();

    bool trailingSlash = false;

    do {
        int begin = pos;

        while (pos < input_.size() && input_[pos] != '\n') {
            if (input_[pos] == '\\' && !trailingSlash) {
                trailingSlash = true;
                ++pos;
                while ((pos < input_.size()) &&
                       input_[pos].isSpace() &&
                       (input_[pos] != '\n'))
                    ++pos;
            }
            else {
                trailingSlash = false;
                ++pos;
            }
        }

        if (!t.isEmpty())
            t += QLatin1Char(' ');
        t += input_.mid(begin, pos - begin).simplified();

        if (trailingSlash) {
            t.chop(1);
            t = t.simplified();
        }
        if (pos < input_.size())
            ++pos;
    } while (pos < input_.size() && trailingSlash);

    return t;
}

/*!
  The metacommand argument is normally the remaining text to
  the right of the metacommand itself. The extra blanks are
  stripped and the argument string is returned.
 */
QString DocParser::getMetaCommandArgument(const QString &cmdStr)
{
    skipSpacesOnLine();

    int begin = pos;
    int parenDepth = 0;

    while (pos < input_.size() && (input_[pos] != '\n' || parenDepth > 0)) {
        if (input_.at(pos) == '(')
            ++parenDepth;
        else if (input_.at(pos) == ')')
            --parenDepth;
        else if (input_.at(pos) == '\\' && expandMacro())
            continue;
        ++pos;
    }
    if (pos == input_.size() && parenDepth > 0) {
        pos = begin;
        location().warning(tr("Unbalanced parentheses in '%1'").arg(cmdStr));
    }

    QString t = input_.mid(begin, pos - begin).simplified();
    skipSpacesOnLine();
    return t;
}

QString DocParser::getUntilEnd(int cmd)
{
    int endCmd = endCmdFor(cmd);
    QRegExp rx("\\\\" + cmdName(endCmd) + "\\b");
    QString t;
    int end = rx.indexIn(input_, pos);

    if (end == -1) {
        location().warning(tr("Missing '\\%1'").arg(cmdName(endCmd)));
        pos = input_.length();
    }
    else {
        t = input_.mid(pos, end - pos);
        pos = end + rx.matchedLength();
    }
    return t;
}

QString DocParser::getCode(int cmd, CodeMarker *marker, const QString &argStr)
{
    QString code = untabifyEtc(getUntilEnd(cmd));

    if (!argStr.isEmpty()) {
        QStringList args = argStr.split(" ", QString::SkipEmptyParts);
        int paramNo, j = 0;
        while (j < code.size()) {
            if (code[j] == '\\'
                 && j < code.size() - 1
                 && (paramNo = code[j + 1].digitValue()) >= 1
                 && paramNo <= args.size()) {
                QString p = args[paramNo - 1];
                code.replace(j, 2, p);
                j += qMin(1, p.size());
            } else {
                ++j;
            }
        }
    }

    int indent = indentLevel(code);
    code = unindent(indent, code);
    if (!marker)
        marker = CodeMarker::markerForCode(code);
    return marker->markedUpCode(code, 0, location());
}

/*!
  Was used only for generating doxygen output.
 */
QString DocParser::getUnmarkedCode(int cmd)
{
    QString code = getUntilEnd(cmd);
    return code;
}

bool DocParser::isBlankLine()
{
    int i = pos;

    while (i < len && input_[i].isSpace()) {
        if (input_[i] == '\n')
            return true;
        i++;
    }
    return false;
}

bool DocParser::isLeftBraceAhead()
{
    int numEndl = 0;
    int i = pos;

    while (i < len && input_[i].isSpace() && numEndl < 2) {
        // ### bug with '\\'
        if (input_[i] == '\n')
            numEndl++;
        i++;
    }
    return numEndl < 2 && i < len && input_[i] == '{';
}

bool DocParser::isLeftBracketAhead()
{
    int numEndl = 0;
    int i = pos;

    while (i < len && input_[i].isSpace() && numEndl < 2) {
        // ### bug with '\\'
        if (input_[i] == '\n')
            numEndl++;
        i++;
    }
    return numEndl < 2 && i < len && input_[i] == '[';
}

/*!
  Skips to the next non-space character or EOL.
 */
void DocParser::skipSpacesOnLine()
{
    while ((pos < input_.length()) &&
           input_[pos].isSpace() &&
           (input_[pos].unicode() != '\n'))
        ++pos;
}

/*!
  Skips spaces and on EOL.
 */
void DocParser::skipSpacesOrOneEndl()
{
    int firstEndl = -1;
    while (pos < (int) input_.length() && input_[pos].isSpace()) {
        QChar ch = input_[pos];
        if (ch == '\n') {
            if (firstEndl == -1) {
                firstEndl = pos;
            }
            else {
                pos = firstEndl;
                break;
            }
        }
        pos++;
    }
}

void DocParser::skipAllSpaces()
{
    while (pos < len && input_[pos].isSpace())
        pos++;
}

void DocParser::skipToNextPreprocessorCommand()
{
    QRegExp rx("\\\\(?:" + cmdName(CMD_IF) + QLatin1Char('|') +
               cmdName(CMD_ELSE) + QLatin1Char('|') +
               cmdName(CMD_ENDIF) + ")\\b");
    int end = rx.indexIn(input_, pos + 1); // ### + 1 necessary?

    if (end == -1)
        pos = input_.length();
    else
        pos = end;
}

int DocParser::endCmdFor(int cmd)
{
    switch (cmd) {
    case CMD_BADCODE:
        return CMD_ENDCODE;
    case CMD_CODE:
        return CMD_ENDCODE;
    case CMD_DIV:
        return CMD_ENDDIV;
    case CMD_QML:
        return CMD_ENDQML;
    case CMD_QMLTEXT:
        return CMD_ENDQMLTEXT;
    case CMD_JS:
        return CMD_ENDJS;
    case CMD_FOOTNOTE:
        return CMD_ENDFOOTNOTE;
    case CMD_LEGALESE:
        return CMD_ENDLEGALESE;
    case CMD_LINK:
        return CMD_ENDLINK;
    case CMD_LIST:
        return CMD_ENDLIST;
    case CMD_NEWCODE:
        return CMD_ENDCODE;
    case CMD_OLDCODE:
        return CMD_NEWCODE;
    case CMD_OMIT:
        return CMD_ENDOMIT;
    case CMD_QUOTATION:
        return CMD_ENDQUOTATION;
    case CMD_RAW:
        return CMD_ENDRAW;
    case CMD_SECTION1:
        return CMD_ENDSECTION1;
    case CMD_SECTION2:
        return CMD_ENDSECTION2;
    case CMD_SECTION3:
        return CMD_ENDSECTION3;
    case CMD_SECTION4:
        return CMD_ENDSECTION4;
    case CMD_SIDEBAR:
        return CMD_ENDSIDEBAR;
    case CMD_TABLE:
        return CMD_ENDTABLE;
    case CMD_TOPICREF:
        return CMD_ENDTOPICREF;
    case CMD_MAPREF:
        return CMD_ENDMAPREF;
    default:
        return cmd;
    }
}

QString DocParser::cmdName(int cmd)
{
    return *cmds[cmd].alias;
}

QString DocParser::endCmdName(int cmd)
{
    return cmdName(endCmdFor(cmd));
}

QString DocParser::untabifyEtc(const QString& str)
{
    QString result;
    result.reserve(str.length());
    int column = 0;

    for (int i = 0; i < str.length(); i++) {
        const QChar c = str.at(i);
        if (c == QLatin1Char('\r'))
            continue;
        if (c == QLatin1Char('\t')) {
            result += &"        "[column % tabSize];
            column = ((column / tabSize) + 1) * tabSize;
            continue;
        }
        if (c == QLatin1Char('\n')) {
            while (result.endsWith(QLatin1Char(' ')))
                result.chop(1);
            result += c;
            column = 0;
            continue;
        }
        result += c;
        column++;
    }

    while (result.endsWith("\n\n"))
        result.truncate(result.length() - 1);
    while (result.startsWith(QLatin1Char('\n')))
        result = result.mid(1);

    return result;
}

int DocParser::indentLevel(const QString& str)
{
    int minIndent = INT_MAX;
    int column = 0;

    for (int i = 0; i < (int) str.length(); i++) {
        if (str[i] == '\n') {
            column = 0;
        }
        else {
            if (str[i] != ' ' && column < minIndent)
                minIndent = column;
            column++;
        }
    }
    return minIndent;
}

QString DocParser::unindent(int level, const QString& str)
{
    if (level == 0)
        return str;

    QString t;
    int column = 0;

    for (int i = 0; i < (int) str.length(); i++) {
        if (str[i] == QLatin1Char('\n')) {
            t += '\n';
            column = 0;
        }
        else {
            if (column >= level)
                t += str[i];
            column++;
        }
    }
    return t;
}

QString DocParser::slashed(const QString& str)
{
    QString result = str;
    result.replace(QLatin1Char('/'), "\\/");
    return QLatin1Char('/') + result + QLatin1Char('/');
}

/*!
 Returns \c true if \a atom represents a code snippet.
 */
bool DocParser::isCode(const Atom *atom)
{
    Atom::AtomType type = atom->type();
    return (type == Atom::Code
            || type == Atom::Qml
            || type == Atom::JavaScript);
}

/*!
 Returns \c true if \a atom represents quoting information.
 */
bool DocParser::isQuote(const Atom *atom)
{
    Atom::AtomType type = atom->type();
    return (type == Atom::CodeQuoteArgument
            || type == Atom::CodeQuoteCommand
            || type == Atom::SnippetCommand
            || type == Atom::SnippetIdentifier
            || type == Atom::SnippetLocation);
}

/*!
  Parse the qdoc comment \a source. Build up a list of all the topic
  commands found including their arguments.  This constructor is used
  when there can be more than one topic command in theqdoc comment.
  Normally, there is only one topic command in a qdoc comment, but in
  QML documentation, there is the case where the qdoc \e{qmlproperty}
  command can appear multiple times in a qdoc comment.
 */
Doc::Doc(const Location& start_loc,
         const Location& end_loc,
         const QString& source,
         const QSet<QString>& metaCommandSet,
         const QSet<QString>& topics)
{
    priv = new DocPrivate(start_loc,end_loc,source);
    DocParser parser;
    parser.parse(source,priv,metaCommandSet,topics);
}

Doc::Doc(const Doc& doc)
    : priv(0)
{
    operator=(doc);
}

Doc::~Doc()
{
    if (priv && priv->deref())
        delete priv;
}

Doc &Doc::operator=(const Doc& doc)
{
    if (doc.priv)
        doc.priv->ref();
    if (priv && priv->deref())
        delete priv;
    priv = doc.priv;
    return *this;
}

void Doc::renameParameters(const QStringList &oldNames,
                           const QStringList &newNames)
{
    if (priv && oldNames != newNames) {
        detach();

        priv->params = newNames.toSet();

        Atom *atom = priv->text.firstAtom();
        while (atom) {
            if (atom->type() == Atom::FormattingLeft
                    && atom->string() == ATOM_FORMATTING_PARAMETER) {
                atom = atom->next();
                if (!atom)
                    return;
                int index = oldNames.indexOf(atom->string());
                if (index != -1 && index < newNames.count())
                    atom->setString(newNames.at(index));
            }
            atom = atom->next();
        }
    }
}

void Doc::simplifyEnumDoc()
{
    if (priv) {
        if (priv->isEnumDocSimplifiable()) {
            detach();

            Text newText;

            Atom *atom = priv->text.firstAtom();
            while (atom) {
                if ((atom->type() == Atom::ListLeft) &&
                        (atom->string() == ATOM_LIST_VALUE)) {
                    while (atom && ((atom->type() != Atom::ListRight) ||
                                    (atom->string() != ATOM_LIST_VALUE)))
                        atom = atom->next();
                    if (atom)
                        atom = atom->next();
                }
                else {
                    newText << *atom;
                    atom = atom->next();
                }
            }
            priv->text = newText;
        }
    }
}

void Doc::setBody(const Text &text)
{
    detach();
    priv->text = text;
}

/*!
  Returns the starting location of a qdoc comment.
 */
const Location &Doc::location() const
{
    static const Location dummy;
    return priv == 0 ? dummy : priv->start_loc;
}

/*!
  Returns the starting location of a qdoc comment.
 */
const Location& Doc::startLocation() const
{
    return location();
}

/*!
  Returns the ending location of a qdoc comment.
 */
const Location& Doc::endLocation() const
{
    static const Location dummy;
    return priv == 0 ? dummy : priv->end_loc;
}

const QString &Doc::source() const
{
    static QString null;
    return priv == 0 ? null : priv->src;
}

bool Doc::isEmpty() const
{
    return priv == 0 || priv->src.isEmpty();
}

const Text& Doc::body() const
{
    static const Text dummy;
    return priv == 0 ? dummy : priv->text;
}

Text Doc::briefText(bool inclusive) const
{
    return body().subText(Atom::BriefLeft, Atom::BriefRight, 0, inclusive);
}

Text Doc::trimmedBriefText(const QString &className) const
{
    QString classNameOnly = className;
    if (className.contains("::"))
        classNameOnly = className.split("::").last();

    Text originalText = briefText();
    Text resultText;
    const Atom *atom = originalText.firstAtom();
    if (atom) {
        QString briefStr;
        QString whats;
        /*
          This code is really ugly. The entire \brief business
          should be rethought.
        */
        while (atom) {
            if (atom->type() == Atom::AutoLink
                || atom->type() == Atom::String) {
                briefStr += atom->string();
            } else if (atom->type() == Atom::C) {
                briefStr += Generator::plainCode(atom->string());
            }
            atom = atom->next();
        }

        QStringList w = briefStr.split(QLatin1Char(' '));
        if (!w.isEmpty() && w.first() == "Returns") {
        }
        else {
            if (!w.isEmpty() && w.first() == "The")
                w.removeFirst();

            if (!w.isEmpty() && (w.first() == className || w.first() == classNameOnly))
                w.removeFirst();

            if (!w.isEmpty() && ((w.first() == "class") ||
                                 (w.first() == "function") ||
                                 (w.first() == "macro") ||
                                 (w.first() == "widget") ||
                                 (w.first() == "namespace") ||
                                 (w.first() == "header")))
                w.removeFirst();

            if (!w.isEmpty() && (w.first() == "is" || w.first() == "provides"))
                w.removeFirst();

            if (!w.isEmpty() && (w.first() == "a" || w.first() == "an"))
                w.removeFirst();
        }

        whats = w.join(' ');

        if (whats.endsWith(QLatin1Char('.')))
            whats.truncate(whats.length() - 1);

        if (!whats.isEmpty())
            whats[0] = whats[0].toUpper();

        // ### move this once \brief is abolished for properties
        resultText << whats;
    }
    return resultText;
}

Text Doc::legaleseText() const
{
    if (priv == 0 || !priv->hasLegalese)
        return Text();
    else
        return body().subText(Atom::LegaleseLeft, Atom::LegaleseRight);
}

Doc::Sections Doc::granularity() const
{
    if (priv == 0 || priv->extra == 0) {
        return DocPrivateExtra().granularity_;
    }
    else {
        return priv->extra->granularity_;
    }
}

const QSet<QString> &Doc::parameterNames() const
{
    return priv == 0 ? *null_Set_QString() : priv->params;
}

const QStringList &Doc::enumItemNames() const
{
    return priv == 0 ? *null_QStringList() : priv->enumItemList;
}

const QStringList &Doc::omitEnumItemNames() const
{
    return priv == 0 ? *null_QStringList() : priv->omitEnumItemList;
}

const QSet<QString> &Doc::metaCommandsUsed() const
{
    return priv == 0 ? *null_Set_QString() : priv->metacommandsUsed;
}

/*!
  Returns a reference to the list of topic commands used in the
  current qdoc comment. Normally there is only one, but there
  can be multiple \e{qmlproperty} commands, for example.
 */
const TopicList& Doc::topicsUsed() const
{
    return priv == 0 ? *nullTopicList() : priv->topics_;
}

ArgList Doc::metaCommandArgs(const QString& metacommand) const
{
    return priv == 0 ? ArgList() : priv->metaCommandMap.value(metacommand);
}

const QList<Text> &Doc::alsoList() const
{
    return priv == 0 ? *null_QList_Text() : priv->alsoList;
}

bool Doc::hasTableOfContents() const
{
    return priv && priv->extra && !priv->extra->tableOfContents_.isEmpty();
}

bool Doc::hasKeywords() const
{
    return priv && priv->extra && !priv->extra->keywords_.isEmpty();
}

bool Doc::hasTargets() const
{
    return priv && priv->extra && !priv->extra->targets_.isEmpty();
}

const QList<Atom *> &Doc::tableOfContents() const
{
    priv->constructExtra();
    return priv->extra->tableOfContents_;
}

const QVector<int> &Doc::tableOfContentsLevels() const
{
    priv->constructExtra();
    return priv->extra->tableOfContentsLevels_;
}

const QList<Atom *> &Doc::keywords() const
{
    priv->constructExtra();
    return priv->extra->keywords_;
}

const QList<Atom *> &Doc::targets() const
{
    priv->constructExtra();
    return priv->extra->targets_;
}

const QStringMultiMap &Doc::metaTagMap() const
{
    return priv && priv->extra ? priv->extra->metaMap_ : *null_QStringMultiMap();
}

const Config* Doc::config_ = 0;

void Doc::initialize(const Config& config)
{
    DocParser::tabSize = config.getInt(CONFIG_TABSIZE);
    DocParser::exampleFiles = config.getCanonicalPathList(CONFIG_EXAMPLES);
    DocParser::exampleDirs = config.getCanonicalPathList(CONFIG_EXAMPLEDIRS);
    DocParser::sourceFiles = config.getCanonicalPathList(CONFIG_SOURCES);
    DocParser::sourceDirs = config.getCanonicalPathList(CONFIG_SOURCEDIRS);

    QmlTypeNode::qmlOnly = config.getBool(CONFIG_QMLONLY);
    QStringMap reverseAliasMap;
    config_ = &config;

    for (const auto &a : config.subVars(CONFIG_ALIAS)) {
        QString alias = config.getString(CONFIG_ALIAS + Config::dot + a);
        if (reverseAliasMap.contains(alias)) {
            config.lastLocation().warning(tr("Command name '\\%1' cannot stand"
                                             " for both '\\%2' and '\\%3'")
                                          .arg(alias)
                                          .arg(reverseAliasMap[alias])
                                          .arg(a));
        } else {
            reverseAliasMap.insert(alias, a);
        }
        aliasMap()->insert(a, alias);
    }

    int i = 0;
    while (cmds[i].english) {
        cmds[i].alias = new QString(alias(cmds[i].english));
        cmdHash()->insert(*cmds[i].alias, cmds[i].no);

        if (cmds[i].no != i)
            Location::internalError(tr("command %1 missing").arg(i));
        i++;
    }

    for (const auto &macroName : config.subVars(CONFIG_MACRO)) {
        QString macroDotName = CONFIG_MACRO + Config::dot + macroName;
        Macro macro;
        macro.numParams = -1;
        macro.defaultDef = config.getString(macroDotName);
        if (!macro.defaultDef.isEmpty()) {
            macro.defaultDefLocation = config.lastLocation();
            macro.numParams = Config::numParams(macro.defaultDef);
        }
        bool silent = false;

        for (const auto &f : config.subVars(macroDotName)) {
            QString def = config.getString(macroDotName + Config::dot + f);
            if (!def.isEmpty()) {
                macro.otherDefs.insert(f, def);
                int m = Config::numParams(def);
                if (macro.numParams == -1)
                    macro.numParams = m;
                else if (macro.numParams != m) {
                    if (!silent) {
                        QString other = tr("default");
                        if (macro.defaultDef.isEmpty())
                            other = macro.otherDefs.constBegin().key();
                        config.lastLocation().warning(tr("Macro '\\%1' takes"
                                                         " inconsistent number"
                                                         " of arguments (%2"
                                                         " %3, %4 %5)")
                                                      .arg(macroName)
                                                      .arg(f)
                                                      .arg(m)
                                                      .arg(other)
                                                      .arg(macro.numParams));
                        silent = true;
                    }
                    if (macro.numParams < m)
                        macro.numParams = m;
                }
            }
        }
        if (macro.numParams != -1)
            macroHash()->insert(macroName, macro);
    }
    // If any of the formats define quotinginformation, activate quoting
    DocParser::quoting = config.getBool(CONFIG_QUOTINGINFORMATION);
    for (const auto &format : config.getOutputFormats())
        DocParser::quoting = DocParser::quoting || config.getBool(format
                                                                  + Config::dot
                                                                  + CONFIG_QUOTINGINFORMATION);
}

/*!
  All the heap allocated variables are deleted.
 */
void Doc::terminate()
{
    DocParser::exampleFiles.clear();
    DocParser::exampleDirs.clear();
    DocParser::sourceFiles.clear();
    DocParser::sourceDirs.clear();
    aliasMap()->clear();
    cmdHash()->clear();
    macroHash()->clear();

    int i = 0;
    while (cmds[i].english) {
        delete cmds[i].alias;
        cmds[i].alias = 0;
        ++i;
    }
}

QString Doc::alias(const QString &english)
{
    return aliasMap()->value(english, english);
}

/*!
  Trims the deadwood out of \a str. i.e., this function
  cleans up \a str.
 */
void Doc::trimCStyleComment(Location& location, QString& str)
{
    QString cleaned;
    Location m = location;
    bool metAsterColumn = true;
    int asterColumn = location.columnNo() + 1;
    int i;

    for (i = 0; i < (int) str.length(); i++) {
        if (m.columnNo() == asterColumn) {
            if (str[i] != '*')
                break;
            cleaned += ' ';
            metAsterColumn = true;
        }
        else {
            if (str[i] == '\n') {
                if (!metAsterColumn)
                    break;
                metAsterColumn = false;
            }
            cleaned += str[i];
        }
        m.advance(str[i]);
    }
    if (cleaned.length() == str.length())
        str = cleaned;

    for (int i = 0; i < 3; i++)
        location.advance(str[i]);
    str = str.mid(3, str.length() - 5);
}

QString Doc::resolveFile(const Location &location,
                         const QString &fileName,
                         QString *userFriendlyFilePath)
{
    const QString result = Config::findFile(location,
                                            DocParser::exampleFiles,
                                            DocParser::exampleDirs,
                                            fileName, userFriendlyFilePath);
    qCDebug(lcQdoc).noquote().nospace() << __FUNCTION__ << "(location="
          << location.fileName() << ':' << location.lineNo()
          << ", fileName=\"" << fileName << "\"), resolved to \""
          << result;
    return result;
}

CodeMarker *Doc::quoteFromFile(const Location &location,
                               Quoter &quoter,
                               const QString &fileName)
{
    quoter.reset();

    QString code;

    QString userFriendlyFilePath;
    const QString filePath = resolveFile(location, fileName, &userFriendlyFilePath);
    if (filePath.isEmpty()) {
        QString details = QLatin1String("Example directories: ") + DocParser::exampleDirs.join(QLatin1Char(' '));
        if (!DocParser::exampleFiles.isEmpty())
            details += QLatin1String(", example files: ") + DocParser::exampleFiles.join(QLatin1Char(' '));
        location.warning(tr("Cannot find file to quote from: '%1'").arg(fileName), details);
    }
    else {
        QFile inFile(filePath);
        if (!inFile.open(QFile::ReadOnly)) {
            location.warning(tr("Cannot open file to quote from: '%1'").arg(userFriendlyFilePath));
        }
        else {
            QTextStream inStream(&inFile);
            code = DocParser::untabifyEtc(inStream.readAll());
        }
    }

    QString dirPath = QFileInfo(filePath).path();
    CodeMarker *marker = CodeMarker::markerForFileName(fileName);
    quoter.quoteFromFile(userFriendlyFilePath, code, marker->markedUpCode(code, 0, location));
    return marker;
}

QString Doc::canonicalTitle(const QString &title)
{
    // The code below is equivalent to the following chunk, but _much_
    // faster (accounts for ~10% of total running time)
    //
    //  QRegExp attributeExpr("[^A-Za-z0-9]+");
    //  QString result = title.toLower();
    //  result.replace(attributeExpr, " ");
    //  result = result.simplified();
    //  result.replace(QLatin1Char(' '), QLatin1Char('-'));

    QString result;
    result.reserve(title.size());

    bool dashAppended = false;
    bool begun = false;
    int lastAlnum = 0;
    for (int i = 0; i != title.size(); ++i) {
        uint c = title.at(i).unicode();
        if (c >= 'A' && c <= 'Z')
            c += 'a' - 'A';
        bool alnum = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
        if (alnum) {
            result += QLatin1Char(c);
            begun = true;
            dashAppended = false;
            lastAlnum = result.size();
        }
        else if (!dashAppended) {
            if (begun)
                result += QLatin1Char('-');
            dashAppended = true;
        }
    }
    result.truncate(lastAlnum);
    return result;
}

void Doc::detach()
{
    if (!priv) {
        priv = new DocPrivate;
        return;
    }
    if (priv->count == 1)
        return;

    --priv->count;

    DocPrivate *newPriv = new DocPrivate(*priv);
    newPriv->count = 1;
    if (priv->extra)
        newPriv->extra = new DocPrivateExtra(*priv->extra);

    priv = newPriv;
}

/*!
  The destructor deletes all the sub-TopicRefs.
 */
TopicRef::~TopicRef()
{
    foreach (DitaRef* t, subrefs_) {
        delete t;
    }
}

/*!
  Returns a reference to the structure that will be used
  for generating a DITA mao.
 */
const DitaRefList& Doc::ditamap() const { return priv->ditamap_; }

QT_END_NAMESPACE
