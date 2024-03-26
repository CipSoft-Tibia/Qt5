// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMLDOMOUTWRITER_P_H
#define QMLDOMOUTWRITER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qqmldom_global.h"
#include "qqmldom_fwd_p.h"
#include "qqmldomattachedinfo_p.h"
#include "qqmldomlinewriter_p.h"

#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

class QMLDOM_EXPORT OutWriterState
{
public:
    OutWriterState(Path itPath, DomItem &it, FileLocations::Tree fLoc);

    void closeState(OutWriter &);

    Path itemCanonicalPath;
    DomItem item;
    PendingSourceLocationId fullRegionId;
    FileLocations::Tree currentMap;
    QMap<QString, PendingSourceLocationId> pendingRegions;
    QMap<QString, CommentedElement> pendingComments;
};

class QMLDOM_EXPORT OutWriter
{
public:
    int indent = 0;
    int indenterId = -1;
    bool indentNextlines = false;
    bool skipComments = false;
    LineWriter &lineWriter;
    Path currentPath;
    FileLocations::Tree topLocation;
    QString writtenStr;
    UpdatedScriptExpression::Tree reformattedScriptExpressions;
    QList<OutWriterState> states;

    explicit OutWriter(LineWriter &lw)
        : lineWriter(lw),
          topLocation(FileLocations::createTree(Path())),
          reformattedScriptExpressions(UpdatedScriptExpression::createTree(Path()))
    {
        lineWriter.addInnerSink([this](QStringView s) { writtenStr.append(s); });
        indenterId =
                lineWriter.addTextAddCallback([this](LineWriter &, LineWriter::TextAddType tt) {
                    if (indentNextlines && tt == LineWriter::TextAddType::Normal
                        && QStringView(lineWriter.currentLine()).trimmed().isEmpty())
                        lineWriter.setLineIndent(indent);
                    return true;
                });
    }

    OutWriterState &state(int i = 0);

    int increaseIndent(int level = 1)
    {
        int oldIndent = indent;
        indent += lineWriter.options().formatOptions.indentSize * level;
        return oldIndent;
    }
    int decreaseIndent(int level = 1, int expectedIndent = -1)
    {
        indent -= lineWriter.options().formatOptions.indentSize * level;
        Q_ASSERT(expectedIndent < 0 || expectedIndent == indent);
        return indent;
    }

    void itemStart(DomItem &it);
    void itemEnd(DomItem &it);
    void regionStart(QString rName);
    void regionStart(QStringView rName) { regionStart(rName.toString()); }
    void regionEnd(QString rName);
    void regionEnd(QStringView rName) { regionEnd(rName.toString()); }

    quint32 counter() const { return lineWriter.counter(); }
    OutWriter &writeRegion(QString rName, QStringView toWrite);
    OutWriter &writeRegion(QStringView rName, QStringView toWrite)
    {
        return writeRegion(rName.toString(), toWrite);
    }
    OutWriter &writeRegion(QString t) { return writeRegion(t, t); }
    OutWriter &writeRegion(QStringView t) { return writeRegion(t.toString(), t); }
    OutWriter &ensureNewline(int nNewlines = 1)
    {
        lineWriter.ensureNewline(nNewlines);
        return *this;
    }
    OutWriter &ensureSpace()
    {
        lineWriter.ensureSpace();
        return *this;
    }
    OutWriter &ensureSpace(QStringView space)
    {
        lineWriter.ensureSpace(space);
        return *this;
    }
    OutWriter &newline()
    {
        lineWriter.newline();
        return *this;
    }
    OutWriter &space()
    {
        lineWriter.space();
        return *this;
    }
    OutWriter &write(QStringView v, LineWriter::TextAddType t = LineWriter::TextAddType::Normal)
    {
        lineWriter.write(v, t);
        return *this;
    }
    OutWriter &write(QStringView v, SourceLocation *toUpdate)
    {
        lineWriter.write(v, toUpdate);
        return *this;
    }
    void flush() { lineWriter.flush(); }
    void eof(bool ensureNewline = true) { lineWriter.eof(ensureNewline); }
    int addNewlinesAutospacerCallback(int nLines)
    {
        return lineWriter.addNewlinesAutospacerCallback(nLines);
    }
    int addTextAddCallback(std::function<bool(LineWriter &, LineWriter::TextAddType)> callback)
    {
        return lineWriter.addTextAddCallback(callback);
    }
    bool removeTextAddCallback(int i) { return lineWriter.removeTextAddCallback(i); }
    void addReformattedScriptExpression(Path p, std::shared_ptr<ScriptExpression> exp)
    {
        if (auto updExp = UpdatedScriptExpression::ensure(reformattedScriptExpressions, p,
                                                          AttachedInfo::PathType::Canonical)) {
            updExp->info().expr = exp;
        }
    }
    DomItem updatedFile(DomItem &qmlFile);
};

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
#endif // QMLDOMOUTWRITER_P_H
