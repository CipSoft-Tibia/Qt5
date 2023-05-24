// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef TST_TYPEDJSON_H
#define TST_TYPEDJSON_H

#include <QtJsonRpc/private/qtypedjson_p.h>
#include <QtTest/QtTest>
#include <QCborValue>
#include <QDebug>
#include <QLibraryInfo>
#include <QByteArray>

#include <memory>

namespace TestSpec {

class Position
{
public:
    int line = {};
    int character = {};

    template<typename W>
    void walk(W &w)
    {
        field(w, "line", line);
        field(w, "character", character);
    }
};

class Range
{
public:
    Position start = {};
    Position end = {};

    template<typename W>
    void walk(W &w)
    {
        field(w, "start", start);
        field(w, "end", end);
    }
};

using ProgressToken = std::variant<int, QByteArray>;

class ReferenceContext
{
public:
    bool includeDeclaration = {};

    template<typename W>
    void walk(W &w)
    {
        field(w, "includeDeclaration", includeDeclaration);
    }
};

class WorkDoneProgressParams
{
public:
    std::optional<ProgressToken> workDoneToken = {};

    template<typename W>
    void walk(W &w)
    {
        field(w, "workDoneToken", workDoneToken);
    }
};

class TextDocumentIdentifier
{
public:
    QByteArray uri = {};

    template<typename W>
    void walk(W &w)
    {
        field(w, "uri", uri);
    }
};

class TextDocumentPositionParams
{
public:
    TextDocumentIdentifier textDocument = {};
    Position position = {};

    template<typename W>
    void walk(W &w)
    {
        field(w, "textDocument", textDocument);
        field(w, "position", position);
    }
};

class PartialResultParams
{
public:
    std::optional<ProgressToken> partialResultToken = {};

    template<typename W>
    void walk(W &w)
    {
        field(w, "partialResultToken", partialResultToken);
    }
};

class ReferenceParams : public TextDocumentPositionParams,
                        WorkDoneProgressParams,
                        PartialResultParams
{
public:
    ReferenceContext context = {};

    template<typename W>
    void walk(W &w)
    {
        TextDocumentPositionParams::walk(w);
        WorkDoneProgressParams::walk(w);
        PartialResultParams::walk(w);
        field(w, "context", context);
    }
};

} // namespace TestSpec

QT_BEGIN_NAMESPACE
namespace QTypedJson {

class TestTypedJson : public QObject
{
    Q_OBJECT
    QJsonObject loadJson(QString filePath)
    {
        QFile f(filePath);
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning() << "Failed opening" << filePath << "due to" << f.errorString();
            Q_ASSERT(false);
        }
        QByteArray data = f.readAll();
        QJsonParseError error;
        QJsonDocument json = QJsonDocument::fromJson(data, &error);
        if (json.isNull()) {
            qWarning() << "Error reading json from" << filePath << ": " << error.errorString();
            Q_ASSERT(false);
        }
        return json.object();
    }
    template<typename T>
    void testT(QString file, T &value, bool checkJson = true)
    {
        QJsonObject obj = loadJson(file);
        QTypedJson::Reader r(obj);
        QTypedJson::doWalk(r, value);
        QJsonObject obj2B = toJsonValue(value).toObject();
        QByteArray json2B = QJsonDocument(obj2B).toJson();
        if (checkJson) {
            QByteArray json1 = QJsonDocument(obj).toJson();
            QCOMPARE(json1, json2B);
        }
        QTypedJson::Reader r2B(obj2B);
        T value2B;
        QTypedJson::doWalk(r2B, value2B);
        QJsonObject obj3 = toJsonValue(value2B).toObject();
        QByteArray json3 = QJsonDocument(obj3).toJson();
        QCOMPARE(json2B, json3);
    }
private slots:

    void testJson()
    {
        QString baseDir = QLatin1String(QT_TYPEDJSON_DATADIR);
        QString f1 = baseDir + QLatin1String("/Range.json");
        TestSpec::Range value;
        testT(f1, value);
        QCOMPARE(value.start.line, 5);
        QCOMPARE(value.start.character, 23);
        QCOMPARE(value.end.line, 6);
        QCOMPARE(value.end.character, 0);
        QString f2 = baseDir + QLatin1String("/ReferenceParams.json");
        TestSpec::ReferenceParams value2;
        testT(f2, value2);
        QCOMPARE(value2.textDocument.uri, QByteArray("file:///folder/file.ts"));
        QCOMPARE(value2.position.line, 9);
        QCOMPARE(value2.position.character, 5);
    }
};

} // namespace QTypedJson
QT_END_NAMESPACE

#endif // TST_TYPEDJSON_H
