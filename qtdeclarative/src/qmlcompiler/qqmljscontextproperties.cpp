// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmljscontextproperties_p.h"
#include <QtCore/qtconfigmacros.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qdirlisting.h>
#include <QtCore/qfile.h>

#if QT_CONFIG(process)
#  include <QtCore/qprocess.h>
#endif

QT_BEGIN_NAMESPACE

namespace QQmlJS {

using namespace Qt::StringLiterals;

// There are many ways to set context properties without triggering the regexp in s_pattern,
// but its supposed to catch most context properties set via "setContextProperty".
static constexpr QLatin1StringView s_pattern =
        R"x((\.|->)setContextProperty\s*\(\s*(QStringLiteral\s*\(|QString\s*\(|QLatin1String(View)?\s*\(|u)?\s*"([^"]*)")x"_L1;
static constexpr int s_contextPropertyNameIdxInPattern = 4;

// TODO: use a central list of file extensions that can also be used by qmetatypesjsonprocessor.cpp
// (that needs header file extensions) and Qt6QmlMacros.cmake.
static constexpr std::array s_fileFilters = {
    "*.cpp"_L1, "*.cxx"_L1, "*.cc"_L1, "*.c"_L1, "*.c++"_L1,
    "*.hpp"_L1, "*.hxx"_L1, "*.hh"_L1, "*.h"_L1, "*.h++"_L1,
};

static const QRegularExpression s_matchSetContextProperty{ s_pattern,
                                                           QRegularExpression::MultilineOption };

static void collectAllFromFile(const QString &filePath, ContextProperties *output)
{
    Q_ASSERT(output);

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    const QString fileContent = QString::fromUtf8(file.readAll());
    for (const auto &match : s_matchSetContextProperty.globalMatch(fileContent)) {
        const quint32 offset = match.capturedStart(s_contextPropertyNameIdxInPattern);
        const quint32 length = match.capturedLength(s_contextPropertyNameIdxInPattern);
        const auto [row, column] = QQmlJS::SourceLocation::rowAndColumnFrom(fileContent, offset);
        const QQmlJS::SourceLocation sourceLocation{ offset, length, row, column };

        (*output)[match.captured(s_contextPropertyNameIdxInPattern)].append(
                ContextProperty{ filePath, sourceLocation });
    }
}

static ContextProperties grepFallback(const QList<QString> &rootUrls)
{
    ContextProperties result;

    const QStringList fileFilters{ s_fileFilters.begin(), s_fileFilters.end() };

    for (const QString &url : rootUrls) {
        for (const auto &dirEntry : QDirListing{ url, fileFilters,
                                                 QDirListing::IteratorFlag::Recursive
                                                         | QDirListing::IteratorFlag::FilesOnly }) {

            const QString filePath = dirEntry.filePath();
            collectAllFromFile(filePath, &result);
        }
    }

    return result;
}

#if QT_CONFIG(process) && !defined(Q_OS_WINDOWS)
static ContextProperties parseGrepOutput(const QString &output)
{
    ContextProperties result;

    for (const auto line : QStringTokenizer{ output, "\n"_L1, Qt::SkipEmptyParts })
        collectAllFromFile(line.toString(), &result);

    return result;
}
#endif

/*!
   \internal
    Uses grep to find files that have setContextProperty()-calls, and then search matching files
   with QRegularExpression to extract the location and name of the found context properties.
*/
ContextProperties ContextProperty::collectAllFrom(const QList<QString> &rootUrls)
{
#if QT_CONFIG(process) && !defined(Q_OS_WINDOWS)
    if (qEnvironmentVariableIsSet("QT_QML_NO_GREP"))
        return grepFallback(rootUrls);

    QProcess grep;
    QStringList arguments{ "--recursive"_L1,
                           "--null-data"_L1, // match multiline patterns
                           "--files-with-matches"_L1,
                           "--extended-regexp"_L1, // the pattern is "extended"
                           "-e"_L1,
                           s_pattern };

    // don't search non-cpp files
    for (const auto fileFilter : s_fileFilters)
        arguments << "--include"_L1 << fileFilter;

    arguments.append(rootUrls);
    grep.start("grep"_L1, arguments);
    grep.waitForFinished();
    if (grep.exitStatus() == QProcess::NormalExit && grep.exitCode() == 0) {
        const QString output = QString::fromUtf8(grep.readAllStandardOutput());
        return parseGrepOutput(output);
    }
#endif
    return grepFallback(rootUrls);
}

bool ContextProperty::isWarningEnabled(const QList<QQmlJS::LoggerCategory> &categories)
{
    auto it = std::find_if(
            categories.cbegin(), categories.cend(),
            [](const QQmlJS::LoggerCategory &c) { return c.id() == qmlContextProperties.name(); });

    return it != categories.cend() && !it->isIgnored();
}

} // namespace QQmlJS

QT_END_NAMESPACE
