/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <private/qqmljsengine_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsastvisitor_p.h>
#include <private/qqmljsast_p.h>

#include "../../shared/util.h"
#include "../../shared/qqmljsastdumper.h"

#include <qtest.h>
#include <QDir>
#include <QDebug>
#include <cstdlib>

class tst_qqmlparser : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlparser();

private slots:
    void initTestCase();
#if !defined(QTEST_CROSS_COMPILED) // sources not available when cross compiled
    void qmlParser_data();
    void qmlParser();
#endif
    void invalidEscapeSequence();
    void stringLiteral();
    void noSubstitutionTemplateLiteral();
    void templateLiteral();
    void leadingSemicolonInClass();
    void templatedReadonlyProperty();
    void qmlImportInJSRequiresFullVersion();
    void typeAnnotations_data();
    void typeAnnotations();
    void disallowedTypeAnnotations_data();
    void disallowedTypeAnnotations();
    void semicolonPartOfExpressionStatement();
    void typeAssertion_data();
    void typeAssertion();
    void annotations_data();
    void annotations();

private:
    QStringList excludedDirs;

    QStringList findFiles(const QDir &);
};

namespace check {

using namespace QQmlJS;

class Check: public AST::Visitor
{
    QList<AST::Node *> nodeStack;

public:
    void operator()(AST::Node *node)
    {
        AST::Node::accept(node, this);
    }

    void checkNode(AST::Node *node)
    {
        if (! nodeStack.isEmpty()) {
            AST::Node *parent = nodeStack.last();
            const quint32 parentBegin = parent->firstSourceLocation().begin();
            const quint32 parentEnd = parent->lastSourceLocation().end();

            if (node->firstSourceLocation().begin() < parentBegin)
                qDebug() << "first source loc failed: node:" << node->kind << "at" << node->firstSourceLocation().startLine << "/" << node->firstSourceLocation().startColumn
                         << "parent" << parent->kind << "at" << parent->firstSourceLocation().startLine << "/" << parent->firstSourceLocation().startColumn;
            if (node->lastSourceLocation().end() > parentEnd)
                qDebug() << "last source loc failed: node:" << node->kind << "at" << node->lastSourceLocation().startLine << "/" << node->lastSourceLocation().startColumn
                         << "parent" << parent->kind << "at" << parent->lastSourceLocation().startLine << "/" << parent->lastSourceLocation().startColumn;

            QVERIFY(node->firstSourceLocation().begin() >= parentBegin);
            QVERIFY(node->lastSourceLocation().end() <= parentEnd);
        }
    }

    virtual bool preVisit(AST::Node *node)
    {
        checkNode(node);
        nodeStack.append(node);
        return true;
    }

    virtual void postVisit(AST::Node *)
    {
        nodeStack.removeLast();
    }

    void throwRecursionDepthError() final
    {
        QFAIL("Maximum statement or expression depth exceeded");
    }
};

struct TypeAnnotationObserver: public AST::Visitor
{
    bool typeAnnotationSeen = false;

    void operator()(AST::Node *node)
    {
        AST::Node::accept(node, this);
    }

    virtual bool visit(AST::TypeAnnotation *)
    {
        typeAnnotationSeen = true;
        return true;
    }

    void throwRecursionDepthError() final
    {
        QFAIL("Maximum statement or expression depth exceeded");
    }
};

struct ExpressionStatementObserver: public AST::Visitor
{
    int expressionsSeen = 0;
    bool endsWithSemicolon = true;

    void operator()(AST::Node *node)
    {
        AST::Node::accept(node, this);
    }

    virtual bool visit(AST::ExpressionStatement *statement)
    {
        ++expressionsSeen;
        endsWithSemicolon = endsWithSemicolon
                && (statement->lastSourceLocation().end() == statement->semicolonToken.end());
        return true;
    }

    void throwRecursionDepthError() final
    {
        QFAIL("Maximum statement or expression depth exceeded");
    }
};

}

tst_qqmlparser::tst_qqmlparser()
{
}

void tst_qqmlparser::initTestCase()
{
    QQmlDataTest::initTestCase();
    // Add directories you want excluded here

    // These snippets are not expected to run on their own.
    excludedDirs << "doc/src/snippets/qml/visualdatamodel_rootindex";
    excludedDirs << "doc/src/snippets/qml/qtbinding";
    excludedDirs << "doc/src/snippets/qml/imports";
    excludedDirs << "doc/src/snippets/qtquick1/visualdatamodel_rootindex";
    excludedDirs << "doc/src/snippets/qtquick1/qtbinding";
    excludedDirs << "doc/src/snippets/qtquick1/imports";
}

QStringList tst_qqmlparser::findFiles(const QDir &d)
{
    for (int ii = 0; ii < excludedDirs.count(); ++ii) {
        QString s = excludedDirs.at(ii);
        if (d.absolutePath().endsWith(s))
            return QStringList();
    }

    QStringList rv;

    QStringList files = d.entryList(QStringList() << QLatin1String("*.qml") << QLatin1String("*.js"),
                                    QDir::Files);
    foreach (const QString &file, files) {
        rv << d.absoluteFilePath(file);
    }

    QStringList dirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot |
                                   QDir::NoSymLinks);
    foreach (const QString &dir, dirs) {
        QDir sub = d;
        sub.cd(dir);
        rv << findFiles(sub);
    }

    return rv;
}

/*
This test checks all the qml and js files in the QtQml UI source tree
and ensures that the subnode's source locations are inside parent node's source locations
*/

#if !defined(QTEST_CROSS_COMPILED) // sources not available when cross compiled
void tst_qqmlparser::qmlParser_data()
{
    QTest::addColumn<QString>("file");

    QString examples = QLatin1String(SRCDIR) + "/../../../../examples/";
    QString tests = QLatin1String(SRCDIR) + "/../../../../tests/";

    QStringList files;
    files << findFiles(QDir(examples));
    files << findFiles(QDir(tests));

    foreach (const QString &file, files)
        QTest::newRow(qPrintable(file)) << file;
}
#endif

#if !defined(QTEST_CROSS_COMPILED) // sources not available when cross compiled
void tst_qqmlparser::qmlParser()
{
    QFETCH(QString, file);

    using namespace QQmlJS;

    QString code;

    QFile f(file);
    if (f.open(QFile::ReadOnly))
        code = QString::fromUtf8(f.readAll());

    const bool qmlMode = file.endsWith(QLatin1String(".qml"));

    Engine engine;
    Lexer lexer(&engine);
    lexer.setCode(code, 1, qmlMode);
    Parser parser(&engine);
    bool ok = qmlMode ? parser.parse() : parser.parseProgram();

    if (ok) {
        check::Check chk;
        chk(parser.rootNode());
    }
}
#endif

void tst_qqmlparser::invalidEscapeSequence()
{
    using namespace QQmlJS;

    Engine engine;
    Lexer lexer(&engine);
    lexer.setCode(QLatin1String("\"\\"), 1);
    Parser parser(&engine);
    parser.parse();
}

void tst_qqmlparser::stringLiteral()
{
    using namespace QQmlJS;

    Engine engine;
    Lexer lexer(&engine);
    QLatin1String code("'hello string'");
    lexer.setCode(code , 1);
    Parser parser(&engine);
    QVERIFY(parser.parseExpression());
    AST::ExpressionNode *expression = parser.expression();
    QVERIFY(expression);
    auto *literal = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(expression);
    QVERIFY(literal);
    QCOMPARE(literal->value, "hello string");
    QCOMPARE(literal->firstSourceLocation().begin(), 0u);
    QCOMPARE(literal->lastSourceLocation().end(), quint32(code.size()));
}

void tst_qqmlparser::noSubstitutionTemplateLiteral()
{
    using namespace QQmlJS;

    Engine engine;
    Lexer lexer(&engine);
    QLatin1String code("`hello template`");
    lexer.setCode(code, 1);
    Parser parser(&engine);
    QVERIFY(parser.parseExpression());
    AST::ExpressionNode *expression = parser.expression();
    QVERIFY(expression);

    auto *literal = QQmlJS::AST::cast<QQmlJS::AST::TemplateLiteral *>(expression);
    QVERIFY(literal);

    QCOMPARE(literal->value, "hello template");
    QCOMPARE(literal->firstSourceLocation().begin(), 0u);
    QCOMPARE(literal->lastSourceLocation().end(), quint32(code.size()));
}

void tst_qqmlparser::templateLiteral()
{
    using namespace QQmlJS;

    Engine engine;
    Lexer lexer(&engine);
    QLatin1String code("`one plus one equals ${1+1}!`");
    lexer.setCode(code, 1);
    Parser parser(&engine);
    QVERIFY(parser.parseExpression());
    AST::ExpressionNode *expression = parser.expression();
    QVERIFY(expression);

    auto *templateLiteral = QQmlJS::AST::cast<QQmlJS::AST::TemplateLiteral *>(expression);
    QVERIFY(templateLiteral);

    QCOMPARE(templateLiteral->firstSourceLocation().begin(), 0u);
    auto *e = templateLiteral->expression;
    QVERIFY(e);
}

void tst_qqmlparser::leadingSemicolonInClass()
{
    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);
    lexer.setCode(QLatin1String("class X{;n(){}}"), 1);
    QQmlJS::Parser parser(&engine);
    QVERIFY(parser.parseProgram());
}

void tst_qqmlparser::templatedReadonlyProperty()
{
    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);
    lexer.setCode(QLatin1String("A { readonly property list<B> listfoo: [ C{} ] }"), 1);
    QQmlJS::Parser parser(&engine);
    QVERIFY(parser.parse());
}

void tst_qqmlparser::qmlImportInJSRequiresFullVersion()
{
    {
        QQmlJS::Engine engine;
        QQmlJS::Lexer lexer(&engine);
        lexer.setCode(QLatin1String(".import Test 1.0 as T"), 0, false);
        QQmlJS::Parser parser(&engine);
        bool b = parser.parseProgram();
        qDebug() << parser.errorMessage();
        QVERIFY(b);
    }
    {
        QQmlJS::Engine engine;
        QQmlJS::Lexer lexer(&engine);
        lexer.setCode(QLatin1String(".import Test 1 as T"), 0, false);
        QQmlJS::Parser parser(&engine);
        QVERIFY(!parser.parseProgram());
    }
    {
        QQmlJS::Engine engine;
        QQmlJS::Lexer lexer(&engine);
        lexer.setCode(QLatin1String(".import Test 1 as T"), 0, false);
        QQmlJS::Parser parser(&engine);
        QVERIFY(!parser.parseProgram());
    }
    {
        QQmlJS::Engine engine;
        QQmlJS::Lexer lexer(&engine);
        lexer.setCode(QLatin1String(".import Test as T"), 0, false);
        QQmlJS::Parser parser(&engine);
        QVERIFY(!parser.parseProgram());
    }
}

void tst_qqmlparser::typeAnnotations_data()
{
    QTest::addColumn<QString>("file");

    QString tests = dataDirectory() + "/typeannotations/";

    QStringList files;
    files << findFiles(QDir(tests));

    for (const QString &file: qAsConst(files))
        QTest::newRow(qPrintable(file)) << file;
}

void tst_qqmlparser::typeAnnotations()
{
    using namespace QQmlJS;

    QFETCH(QString, file);

    QString code;

    QFile f(file);
    if (f.open(QFile::ReadOnly))
        code = QString::fromUtf8(f.readAll());

    const bool qmlMode = file.endsWith(QLatin1String(".qml"));

    Engine engine;
    Lexer lexer(&engine);
    lexer.setCode(code, 1, qmlMode);
    Parser parser(&engine);
    bool ok = qmlMode ? parser.parse() : parser.parseProgram();
    QVERIFY(ok);

    check::TypeAnnotationObserver observer;
    observer(parser.rootNode());

    QVERIFY(observer.typeAnnotationSeen);
}

void tst_qqmlparser::disallowedTypeAnnotations_data()
{
    QTest::addColumn<QString>("file");

    QString tests = dataDirectory() + "/disallowedtypeannotations/";

    QStringList files;
    files << findFiles(QDir(tests));

    for (const QString &file: qAsConst(files))
        QTest::newRow(qPrintable(file)) << file;
}

void tst_qqmlparser::disallowedTypeAnnotations()
{
    using namespace QQmlJS;

    QFETCH(QString, file);

    QString code;

    QFile f(file);
    if (f.open(QFile::ReadOnly))
        code = QString::fromUtf8(f.readAll());

    const bool qmlMode = file.endsWith(QLatin1String(".qml"));

    Engine engine;
    Lexer lexer(&engine);
    lexer.setCode(code, 1, qmlMode);
    Parser parser(&engine);
    bool ok = qmlMode ? parser.parse() : parser.parseProgram();
    QVERIFY(!ok);
    QVERIFY2(parser.errorMessage().startsWith("Type annotations are not permitted "), qPrintable(parser.errorMessage()));
}

void tst_qqmlparser::semicolonPartOfExpressionStatement()
{
    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);
    lexer.setCode(QLatin1String("A { property int x: 1+1; property int y: 2+2 \n"
                                "tt: {'a': 5, 'b': 6}; ff: {'c': 'rrr'}}"), 1);
    QQmlJS::Parser parser(&engine);
    QVERIFY(parser.parse());

    check::ExpressionStatementObserver observer;
    observer(parser.rootNode());

    QCOMPARE(observer.expressionsSeen, 4);
    QVERIFY(observer.endsWithSemicolon);
}

void tst_qqmlparser::typeAssertion_data()
{
    QTest::addColumn<QString>("expression");
    QTest::addRow("as A")
            << QString::fromLatin1("A { onStuff: (b as A).happen() }");
    QTest::addRow("as double paren")
            << QString::fromLatin1("A { onStuff: console.log((12 as double)); }");
    QTest::addRow("as double noparen")
            << QString::fromLatin1("A { onStuff: console.log(12 as double); }");
    QTest::addRow("property as double")
            << QString::fromLatin1("A { prop: (12 as double); }");
    QTest::addRow("property noparen as double")
            << QString::fromLatin1("A { prop: 12 as double; }");

    // rabbits cannot be discerned from types on a syntactical level.
    // We could detect this on a semantical level, once we implement type assertions there.

    QTest::addRow("as rabbit")
            << QString::fromLatin1("A { onStuff: (b as rabbit).happen() }");
    QTest::addRow("as rabbit paren")
            << QString::fromLatin1("A { onStuff: console.log((12 as rabbit)); }");
    QTest::addRow("as rabbit noparen")
            << QString::fromLatin1("A { onStuff: console.log(12 as rabbit); }");
    QTest::addRow("property as rabbit")
            << QString::fromLatin1("A { prop: (12 as rabbit); }");
    QTest::addRow("property noparen as rabbit")
            << QString::fromLatin1("A { prop: 12 as rabbit; }");
}

void tst_qqmlparser::typeAssertion()
{
    QFETCH(QString, expression);

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);
    lexer.setCode(expression, 1);
    QQmlJS::Parser parser(&engine);
    QVERIFY(parser.parse());
}

void tst_qqmlparser::annotations_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("refFile");

    QString tests = dataDirectory() + "/annotations/";
    QString compare = dataDirectory() + "/noannotations/";

    QStringList files;
    files << findFiles(QDir(tests));

    QStringList refFiles;
    refFiles << findFiles(QDir(compare));

    for (const QString &file: qAsConst(files)) {
        auto fileNameStart = file.lastIndexOf(QDir::separator());
        QStringRef fileName(&file, fileNameStart, file.length()-fileNameStart);
        auto ref=std::find_if(refFiles.constBegin(),refFiles.constEnd(), [fileName](const QString &s){ return s.endsWith(fileName); });
        if (ref != refFiles.constEnd())
            QTest::newRow(qPrintable(file)) << file << *ref;
        else
            QTest::newRow(qPrintable(file)) << file << QString();
    }
}

void tst_qqmlparser::annotations()
{
    using namespace QQmlJS;

    QFETCH(QString, file);
    QFETCH(QString, refFile);

    QString code;
    QString refCode;

    QFile f(file);
    if (f.open(QFile::ReadOnly))
        code = QString::fromUtf8(f.readAll());
    QFile refF(refFile);
    if (!refFile.isEmpty() && refF.open(QFile::ReadOnly))
        refCode = QString::fromUtf8(refF.readAll());

    const bool qmlMode = true;

    Engine engine;
    Lexer lexer(&engine);
    lexer.setCode(code, 1, qmlMode);
    Parser parser(&engine);
    QVERIFY(parser.parse());

    if (!refCode.isEmpty()) {
        Engine engine2;
        Lexer lexer2(&engine2);
        lexer2.setCode(refCode, 1, qmlMode);
        Parser parser2(&engine2);
        QVERIFY(parser2.parse());

        QCOMPARE(AstDumper::diff(parser.ast(), parser2.rootNode(), 3, DumperOptions::NoAnnotations | DumperOptions::NoLocations), QString());
    }
}

QTEST_MAIN(tst_qqmlparser)

#include "tst_qqmlparser.moc"
