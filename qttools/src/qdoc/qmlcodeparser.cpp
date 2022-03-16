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

/*
  qmlcodeparser.cpp
*/

#include "qmlcodeparser.h"

#include "node.h"
#include "config.h"
#include "qmlvisitor.h"

#ifndef QT_NO_DECLARATIVE
#include <private/qqmljsast_p.h>
#include <private/qqmljsastvisitor_p.h>
#endif
#include <qdebug.h>

QT_BEGIN_NAMESPACE

#define COMMAND_STARTPAGE               Doc::alias("startpage")
#define COMMAND_VARIABLE                Doc::alias("variable")

#define COMMAND_DEPRECATED              Doc::alias("deprecated")
#define COMMAND_INGROUP                 Doc::alias("ingroup")
#define COMMAND_INTERNAL                Doc::alias("internal")
#define COMMAND_OBSOLETE                Doc::alias("obsolete")
#define COMMAND_PAGEKEYWORDS            Doc::alias("pagekeywords")
#define COMMAND_PRELIMINARY             Doc::alias("preliminary")
#define COMMAND_SINCE                   Doc::alias("since")
#define COMMAND_WRAPPER                 Doc::alias("wrapper")
#define COMMAND_NOAUTOLIST              Doc::alias("noautolist")

#define COMMAND_ABSTRACT                Doc::alias("abstract")
#define COMMAND_QMLABSTRACT             Doc::alias("qmlabstract")
#define COMMAND_QMLCLASS                Doc::alias("qmlclass")
#define COMMAND_QMLTYPE                 Doc::alias("qmltype")
#define COMMAND_QMLMODULE               Doc::alias("qmlmodule")
#define COMMAND_QMLPROPERTY             Doc::alias("qmlproperty")
#define COMMAND_QMLPROPERTYGROUP        Doc::alias("qmlpropertygroup")
#define COMMAND_QMLATTACHEDPROPERTY     Doc::alias("qmlattachedproperty")
#define COMMAND_QMLINHERITS             Doc::alias("inherits")
#define COMMAND_QMLINSTANTIATES         Doc::alias("instantiates")
#define COMMAND_INQMLMODULE             Doc::alias("inqmlmodule")
#define COMMAND_QMLSIGNAL               Doc::alias("qmlsignal")
#define COMMAND_QMLATTACHEDSIGNAL       Doc::alias("qmlattachedsignal")
#define COMMAND_QMLMETHOD               Doc::alias("qmlmethod")
#define COMMAND_QMLATTACHEDMETHOD       Doc::alias("qmlattachedmethod")
#define COMMAND_QMLDEFAULT              Doc::alias("default")
#define COMMAND_QMLREADONLY             Doc::alias("readonly")
#define COMMAND_QMLBASICTYPE            Doc::alias("qmlbasictype")
#define COMMAND_QMLMODULE               Doc::alias("qmlmodule")

#define COMMAND_JSTYPE                 Doc::alias("jstype")
#define COMMAND_JSMODULE               Doc::alias("jsmodule")
#define COMMAND_JSPROPERTY             Doc::alias("jsproperty")
#define COMMAND_JSPROPERTYGROUP        Doc::alias("jspropertygroup")
#define COMMAND_JSATTACHEDPROPERTY     Doc::alias("jsattachedproperty")
#define COMMAND_INJSMODULE             Doc::alias("injsmodule")
#define COMMAND_JSSIGNAL               Doc::alias("jssignal")
#define COMMAND_JSATTACHEDSIGNAL       Doc::alias("jsattachedsignal")
#define COMMAND_JSMETHOD               Doc::alias("jsmethod")
#define COMMAND_JSATTACHEDMETHOD       Doc::alias("jsattachedmethod")
#define COMMAND_JSBASICTYPE            Doc::alias("jsbasictype")
#define COMMAND_JSMODULE               Doc::alias("jsmodule")

/*!
  Constructs the QML code parser.
 */
QmlCodeParser::QmlCodeParser()
#ifndef QT_NO_DECLARATIVE
    : lexer( 0 ),
      parser( 0 )
#endif
{
}

/*!
  Destroys the QML code parser.
 */
QmlCodeParser::~QmlCodeParser()
{
}

/*!
  Initializes the code parser base class. The \a config argument
  is passed to the initialization functions in the base class.

  Also creates a lexer and parser from QQmlJS.
 */
void QmlCodeParser::initializeParser(const Config &config)
{
    CodeParser::initializeParser(config);

#ifndef QT_NO_DECLARATIVE
    lexer = new QQmlJS::Lexer(&engine);
    parser = new QQmlJS::Parser(&engine);
#endif
}

/*!
  Terminates the QML code parser. Deletes the lexer and parser
  created by the constructor.
 */
void QmlCodeParser::terminateParser()
{
#ifndef QT_NO_DECLARATIVE
    delete lexer;
    delete parser;
#endif
}

/*!
  Returns "QML".
 */
QString QmlCodeParser::language()
{
    return "QML";
}

/*!
  Returns a string list containing "*.qml". This is the only
  file type parsed by the QMLN parser.
 */
QStringList QmlCodeParser::sourceFileNameFilter()
{
    return QStringList() << "*.qml";
}

/*!
  Parses the source file at \a filePath and inserts the contents
  into the database. The \a location is used for error reporting.

  If it can't open the file at \a filePath, it reports an error
  and returns without doing anything.
 */
void QmlCodeParser::parseSourceFile(const Location& location, const QString& filePath)
{
    QFile in(filePath);
    currentFile_ = filePath;
    if (!in.open(QIODevice::ReadOnly)) {
        location.error(tr("Cannot open QML file '%1'").arg(filePath));
        currentFile_.clear();
        return;
    }

#ifndef QT_NO_DECLARATIVE
    QString document = in.readAll();
    in.close();

    Location fileLocation(filePath);

    QString newCode = document;
    extractPragmas(newCode);
    lexer->setCode(newCode, 1);

    const QSet<QString>& topicCommandsAllowed = topicCommands();
    const QSet<QString>& otherMetacommandsAllowed = otherMetaCommands();
    const QSet<QString>& metacommandsAllowed = topicCommandsAllowed + otherMetacommandsAllowed;

    if (parser->parse()) {
        QQmlJS::AST::UiProgram *ast = parser->ast();
        QmlDocVisitor visitor(filePath,
                              newCode,
                              &engine,
                              metacommandsAllowed,
                              topicCommandsAllowed);
        QQmlJS::AST::Node::accept(ast, &visitor);
        if (visitor.hasError()) {
            qDebug().nospace() << qPrintable(filePath) << ": Could not analyze QML file. "
                               << "The output is incomplete.";
        }
    }
    foreach (const  QQmlJS::DiagnosticMessage &msg, parser->diagnosticMessages()) {
        qDebug().nospace() << qPrintable(filePath) << ':' << msg.loc.startLine
                           << ": QML syntax error at col " << msg.loc.startColumn
                           << ": " << qPrintable(msg.message);
    }
    currentFile_.clear();
#else
    location.warning("QtDeclarative not installed; cannot parse QML or JS.");
#endif
}

static QSet<QString> topicCommands_;
/*!
  Returns the set of strings representing the topic commands.
 */
const QSet<QString>& QmlCodeParser::topicCommands()
{
    if (topicCommands_.isEmpty()) {
        topicCommands_ << COMMAND_VARIABLE
                       << COMMAND_QMLCLASS
                       << COMMAND_QMLTYPE
                       << COMMAND_QMLPROPERTY
                       << COMMAND_QMLPROPERTYGROUP
                       << COMMAND_QMLATTACHEDPROPERTY
                       << COMMAND_QMLSIGNAL
                       << COMMAND_QMLATTACHEDSIGNAL
                       << COMMAND_QMLMETHOD
                       << COMMAND_QMLATTACHEDMETHOD
                       << COMMAND_QMLBASICTYPE
                       << COMMAND_JSTYPE
                       << COMMAND_JSPROPERTY
                       << COMMAND_JSPROPERTYGROUP
                       << COMMAND_JSATTACHEDPROPERTY
                       << COMMAND_JSSIGNAL
                       << COMMAND_JSATTACHEDSIGNAL
                       << COMMAND_JSMETHOD
                       << COMMAND_JSATTACHEDMETHOD
                       << COMMAND_JSBASICTYPE;
    }
    return topicCommands_;
}

static QSet<QString> otherMetaCommands_;
/*!
  Returns the set of strings representing the common metacommands
  plus some other metacommands.
 */
const QSet<QString>& QmlCodeParser::otherMetaCommands()
{
    if (otherMetaCommands_.isEmpty()) {
        otherMetaCommands_ = commonMetaCommands();
        otherMetaCommands_ << COMMAND_STARTPAGE
                           << COMMAND_QMLINHERITS
                           << COMMAND_QMLDEFAULT
                           << COMMAND_QMLREADONLY
                           << COMMAND_DEPRECATED
                           << COMMAND_INGROUP
                           << COMMAND_INTERNAL
                           << COMMAND_OBSOLETE
                           << COMMAND_PRELIMINARY
                           << COMMAND_SINCE
                           << COMMAND_ABSTRACT
                           << COMMAND_QMLABSTRACT
                           << COMMAND_INQMLMODULE
                           << COMMAND_INJSMODULE
                           << COMMAND_WRAPPER
                           << COMMAND_NOAUTOLIST;
    }
    return otherMetaCommands_;
}

#ifndef QT_NO_DECLARATIVE
/*!
  Copy and paste from src/declarative/qml/qdeclarativescriptparser.cpp.
  This function blanks out the section of the \a str beginning at \a idx
  and running for \a n characters.
*/
static void replaceWithSpace(QString &str, int idx, int n)
{
    QChar *data = str.data() + idx;
    const QChar space(QLatin1Char(' '));
    for (int ii = 0; ii < n; ++ii)
        *data++ = space;
}

/*!
  Copy & paste from src/declarative/qml/qdeclarativescriptparser.cpp,
  then modified to return no values.

  Searches for ".pragma <value>" declarations within \a script.
  Currently supported pragmas are: library
*/
void QmlCodeParser::extractPragmas(QString &script)
{
    const QString pragma(QLatin1String("pragma"));
    const QString library(QLatin1String("library"));

    QQmlJS::Lexer l(0);
    l.setCode(script, 0);

    int token = l.lex();

    while (true) {
        if (token != QQmlJSGrammar::T_DOT)
            return;

        int startOffset = l.tokenOffset();
        int startLine = l.tokenStartLine();

        token = l.lex();

        if (token != QQmlJSGrammar::T_IDENTIFIER ||
                l.tokenStartLine() != startLine ||
                script.mid(l.tokenOffset(), l.tokenLength()) != pragma)
            return;

        token = l.lex();

        if (token != QQmlJSGrammar::T_IDENTIFIER ||
                l.tokenStartLine() != startLine)
            return;

        QString pragmaValue = script.mid(l.tokenOffset(), l.tokenLength());
        int endOffset = l.tokenLength() + l.tokenOffset();

        token = l.lex();
        if (l.tokenStartLine() == startLine)
            return;

        if (pragmaValue == QLatin1String("library"))
            replaceWithSpace(script, startOffset, endOffset - startOffset);
        else
            return;
    }
    return;
}
#endif

QT_END_NAMESPACE
