// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldom_utils_p.h"
#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qstring.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qcbormap.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QQmlJSDomImporting, "qt.qqmljsdom.importing")

namespace QQmlJS {
namespace Dom {

using namespace Qt::StringLiterals;

QStringList resourceFilesFromBuildFolders(const QStringList &buildFolders)
{
    QStringList result;
    for (const QString &path : buildFolders) {
        QDir dir(path);
        if (!dir.cd(u".rcc"_s))
            continue;

        QDirIterator it(dir.canonicalPath(), QStringList{ u"*.qrc"_s }, QDir::Files,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            result.append(it.next());
        }
    }
    return result;
}

static QMetaEnum regionEnum = QMetaEnum::fromType<FileLocationRegion>();

QString fileLocationRegionName(FileLocationRegion region)
{
    return QString::fromLatin1(regionEnum.key(region));
}

FileLocationRegion fileLocationRegionValue(QStringView region)
{
    return static_cast<FileLocationRegion>(regionEnum.keyToValue(region.toLatin1()));
}

QCborValue sourceLocationToQCborValue(QQmlJS::SourceLocation loc)
{
    QCborMap res({
        {QStringLiteral(u"offset"), loc.offset},
        {QStringLiteral(u"length"), loc.length},
        {QStringLiteral(u"startLine"), loc.startLine},
        {QStringLiteral(u"startColumn"), loc.startColumn}
    });
    return res;
}

} // namespace Dom
}; // namespace QQmlJS

QT_END_NAMESPACE
