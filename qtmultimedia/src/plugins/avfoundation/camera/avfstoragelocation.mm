/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "avfstoragelocation.h"
#include "QtCore/qstandardpaths.h"


QT_BEGIN_NAMESPACE


AVFStorageLocation::AVFStorageLocation()
{
}

AVFStorageLocation::~AVFStorageLocation()
{
}

/*!
 * Generate the actual file name from user requested one.
 * requestedName may be either empty (the default dir and naming theme is used),
 * points to existing dir (the default name used)
 * or specify the full actual path.
 */
QString AVFStorageLocation::generateFileName(const QString &requestedName,
                                             QCamera::CaptureMode mode,
                                             const QString &prefix,
                                             const QString &ext) const
{
    if (requestedName.isEmpty())
        return generateFileName(prefix, defaultDir(mode), ext);

    if (QFileInfo(requestedName).isDir())
        return generateFileName(prefix, QDir(requestedName), ext);

    return requestedName;
}

QDir AVFStorageLocation::defaultDir(QCamera::CaptureMode mode) const
{
    QStringList dirCandidates;

    if (mode == QCamera::CaptureVideo) {
        dirCandidates << QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    } else {
        dirCandidates << QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    }

    dirCandidates << QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    dirCandidates << QDir::homePath();
    dirCandidates << QDir::currentPath();
    dirCandidates << QDir::tempPath();

    for (const QString &path : qAsConst(dirCandidates)) {
        if (QFileInfo(path).isWritable())
            return QDir(path);
    }

    return QDir();
}

QString AVFStorageLocation::generateFileName(const QString &prefix, const QDir &dir, const QString &ext) const
{
    QString lastClipKey = dir.absolutePath()+QLatin1Char(' ')+prefix+QLatin1Char(' ')+ext;
    int lastClip = m_lastUsedIndex.value(lastClipKey, 0);

    if (lastClip == 0) {
        //first run, find the maximum clip number during the fist capture
        const auto list = dir.entryList(QStringList() << QString("%1*.%2").arg(prefix).arg(ext));
        for (const QString &fileName : list) {
            int imgNumber = fileName.midRef(prefix.length(), fileName.size()-prefix.length()-ext.length()-1).toInt();
            lastClip = qMax(lastClip, imgNumber);
        }
    }


    //don't just rely on cached lastClip value,
    //someone else may create a file after camera started
    while (true) {
        QString name = QString("%1%2.%3").arg(prefix)
                                         .arg(lastClip+1,
                                         4, //fieldWidth
                                         10,
                                         QLatin1Char('0'))
                                         .arg(ext);

        QString path = dir.absoluteFilePath(name);
        if (!QFileInfo(path).exists()) {
            m_lastUsedIndex[lastClipKey] = lastClip+1;
            return path;
        }

        lastClip++;
    }

    return QString();
}


QT_END_NAMESPACE
