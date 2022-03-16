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

#ifndef CODEPARSER_H
#define CODEPARSER_H

#include <qset.h>
#include "node.h"

QT_BEGIN_NAMESPACE

class Config;
class QString;
class QDocDatabase;

class CodeParser
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::CodeParser)

public:
    CodeParser();
    virtual ~CodeParser();

    virtual void initializeParser(const Config& config);
    virtual void terminateParser();
    virtual QString language() = 0;
    virtual QStringList headerFileNameFilter();
    virtual QStringList sourceFileNameFilter() = 0;
    virtual void parseHeaderFile(const Location& location, const QString& filePath);
    virtual void parseSourceFile(const Location& location, const QString& filePath) = 0;
    virtual void precompileHeaders() { }
    virtual Node *parseFnArg(const Location &, const QString &) { return 0; }
    virtual Node *parseMacroArg(const Location &, const QString &) { return 0; }
    virtual Node *parseOtherFuncArg(const QString &, const Location &, const QString &) { return 0; }

    bool isParsingH() const;
    bool isParsingCpp() const;
    bool isParsingQdoc() const;
    const QString& currentFile() const { return currentFile_; }
    const QString& moduleHeader() const { return moduleHeader_; }
    void setModuleHeader(const QString& t) { moduleHeader_ = t; }
    void checkModuleInclusion(Node* n);

    static void initialize(const Config& config);
    static void terminate();
    static CodeParser *parserForLanguage(const QString& language);
    static CodeParser *parserForHeaderFile(const QString &filePath);
    static CodeParser *parserForSourceFile(const QString &filePath);
    static void setLink(Node* node, Node::LinkType linkType, const QString& arg);
    static bool isWorthWarningAbout(const Doc &doc);

protected:
    const QSet<QString>& commonMetaCommands();
    void processCommonMetaCommand(const Location& location,
                                  const QString& command,
                                  const ArgLocPair& arg,
                                  Node *node);
    static void extractPageLinkAndDesc(const QString& arg,
                                       QString* link,
                                       QString* desc);
    QString moduleHeader_;
    QString currentFile_;
    QDocDatabase* qdb_;

private:
    static QList<CodeParser *> parsers;
    static bool showInternal_;
    static bool singleExec_;
};

QT_END_NAMESPACE

#endif
