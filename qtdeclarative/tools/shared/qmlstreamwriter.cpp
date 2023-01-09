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

#include "qmlstreamwriter.h"

#include <QtCore/QBuffer>
#include <QtCore/QStringList>

QmlStreamWriter::QmlStreamWriter(QByteArray *array)
    : m_indentDepth(0)
    , m_pendingLineLength(0)
    , m_maybeOneline(false)
    , m_stream(new QBuffer(array))
{
    m_stream->open(QIODevice::WriteOnly);
}

void QmlStreamWriter::writeStartDocument()
{
}

void QmlStreamWriter::writeEndDocument()
{
}

void QmlStreamWriter::writeLibraryImport(const QString &uri, int majorVersion, int minorVersion, const QString &as)
{
    m_stream->write(QString::fromLatin1("import %1 %2.%3").arg(uri, QString::number(majorVersion), QString::number(minorVersion)).toUtf8());
    if (!as.isEmpty())
        m_stream->write(QString::fromLatin1(" as %1").arg(as).toUtf8());
    m_stream->write("\n");
}

void QmlStreamWriter::writeStartObject(const QString &component)
{
    flushPotentialLinesWithNewlines();
    writeIndent();
    m_stream->write(QString::fromLatin1("%1 {").arg(component).toUtf8());
    ++m_indentDepth;
    m_maybeOneline = true;
}

void QmlStreamWriter::writeEndObject()
{
    if (m_maybeOneline && !m_pendingLines.isEmpty()) {
        --m_indentDepth;
        for (int i = 0; i < m_pendingLines.size(); ++i) {
            m_stream->write(" ");
            m_stream->write(m_pendingLines.at(i).trimmed());
            if (i != m_pendingLines.size() - 1)
                m_stream->write(";");
        }
        m_stream->write(" }\n");
        m_pendingLines.clear();
        m_pendingLineLength = 0;
        m_maybeOneline = false;
    } else {
        flushPotentialLinesWithNewlines();
        --m_indentDepth;
        writeIndent();
        m_stream->write("}\n");
    }
}

void QmlStreamWriter::writeScriptBinding(const QString &name, const QString &rhs)
{
    writePotentialLine(QString::fromLatin1("%1: %2").arg(name, rhs).toUtf8());
}

void QmlStreamWriter::writeBooleanBinding(const QString &name, bool value)
{
    writeScriptBinding(name, value ? QLatin1String("true") : QLatin1String("false"));
}

void QmlStreamWriter::writeArrayBinding(const QString &name, const QStringList &elements)
{
    flushPotentialLinesWithNewlines();
    writeIndent();

    // try to use a single line
    QString singleLine;
    singleLine += QString::fromLatin1("%1: [").arg(name);
    for (int i = 0; i < elements.size(); ++i) {
        singleLine += elements.at(i);
        if (i != elements.size() - 1)
            singleLine += QLatin1String(", ");
    }
    singleLine += QLatin1String("]\n");
    if (singleLine.size() + m_indentDepth * 4 < 80) {
        m_stream->write(singleLine.toUtf8());
        return;
    }

    // write multi-line
    m_stream->write(QString::fromLatin1("%1: [\n").arg(name).toUtf8());
    ++m_indentDepth;
    for (int i = 0; i < elements.size(); ++i) {
        writeIndent();
        m_stream->write(elements.at(i).toUtf8());
        if (i != elements.size() - 1) {
            m_stream->write(",\n");
        } else {
            m_stream->write("\n");
        }
    }
    --m_indentDepth;
    writeIndent();
    m_stream->write("]\n");
}

void QmlStreamWriter::write(const QString &data)
{
    flushPotentialLinesWithNewlines();
    m_stream->write(data.toUtf8());
}

void QmlStreamWriter::writeScriptObjectLiteralBinding(const QString &name, const QList<QPair<QString, QString> > &keyValue)
{
    flushPotentialLinesWithNewlines();
    writeIndent();
    m_stream->write(QString::fromLatin1("%1: {\n").arg(name).toUtf8());
    ++m_indentDepth;
    for (int i = 0; i < keyValue.size(); ++i) {
        const QString key = keyValue.at(i).first;
        const QString value = keyValue.at(i).second;
        writeIndent();
        m_stream->write(QString::fromLatin1("%1: %2").arg(key, value).toUtf8());
        if (i != keyValue.size() - 1) {
            m_stream->write(",\n");
        } else {
            m_stream->write("\n");
        }
    }
    --m_indentDepth;
    writeIndent();
    m_stream->write("}\n");
}

void QmlStreamWriter::writeIndent()
{
    m_stream->write(QByteArray(m_indentDepth * 4, ' '));
}

void QmlStreamWriter::writePotentialLine(const QByteArray &line)
{
    m_pendingLines.append(line);
    m_pendingLineLength += line.size();
    if (m_pendingLineLength >= 80) {
        flushPotentialLinesWithNewlines();
    }
}

void QmlStreamWriter::flushPotentialLinesWithNewlines()
{
    if (m_maybeOneline)
        m_stream->write("\n");
    for (const QByteArray &line : qAsConst(m_pendingLines)) {
        writeIndent();
        m_stream->write(line);
        m_stream->write("\n");
    }
    m_pendingLines.clear();
    m_pendingLineLength = 0;
    m_maybeOneline = false;
}
