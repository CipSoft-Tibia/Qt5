// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpreviewfilesystemwatcher.h"

#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qfilesystemwatcher.h>

QmlPreviewFileSystemWatcher::QmlPreviewFileSystemWatcher(QObject *parent) :
    QObject(parent), m_watcher(new QFileSystemWatcher(this))
{
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &QmlPreviewFileSystemWatcher::fileChanged);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &QmlPreviewFileSystemWatcher::onDirectoryChanged);
}

bool QmlPreviewFileSystemWatcher::watchesFile(const QString &file) const
{
    return m_files.contains(file);
}

void QmlPreviewFileSystemWatcher::addFile(const QString &file)
{
    if (watchesFile(file)) {
        qWarning() << "FileSystemWatcher: File" << file << "is already being watched.";
        return;
    }

    QStringList toAdd(file);
    m_files.insert(file);

    const QString directory = QFileInfo(file).path();
    const int dirCount = ++m_directoryCount[directory];
    Q_ASSERT(dirCount > 0);

    if (dirCount == 1)
        toAdd.append(directory);

    m_watcher->addPaths(toAdd);
}

void QmlPreviewFileSystemWatcher::removeFile(const QString &file)
{
    WatchEntrySetIterator it = m_files.find(file);
    if (it == m_files.end()) {
        qWarning() << "FileSystemWatcher: File" << file <<  "is not watched.";
        return;
    }

    QStringList toRemove(file);
    m_files.erase(it);
    m_watcher->removePath(file);

    const QString directory = QFileInfo(file).path();
    const int dirCount = --m_directoryCount[directory];
    Q_ASSERT(dirCount >= 0);

    if (!dirCount)
        toRemove.append(directory);

    m_watcher->removePaths(toRemove);
}

bool QmlPreviewFileSystemWatcher::watchesDirectory(const QString &directory) const
{
    return m_directories.contains(directory);
}

void QmlPreviewFileSystemWatcher::addDirectory(const QString &directory)
{
    if (watchesDirectory(directory)) {
        qWarning() << "FileSystemWatcher: Directory" << directory << "is already being watched.";
        return;
    }

    m_directories.insert(directory);
    const int count = ++m_directoryCount[directory];
    Q_ASSERT(count > 0);

    if (count == 1)
        m_watcher->addPath(directory);
}

void QmlPreviewFileSystemWatcher::removeDirectory(const QString &directory)
{
    WatchEntrySetIterator it = m_directories.find(directory);
    if (it == m_directories.end()) {
        qWarning() << "FileSystemWatcher: Directory"  << directory << "is not watched.";
        return;
    }

    m_directories.erase(it);

    const int count = --m_directoryCount[directory];
    Q_ASSERT(count >= 0);

    if (!count)
        m_watcher->removePath(directory);
}

void QmlPreviewFileSystemWatcher::onDirectoryChanged(const QString &path)
{
    if (m_directories.contains(path))
        emit directoryChanged(path);

    QStringList toReadd;
    const QDir dir(path);
    for (const QFileInfo &entry : dir.entryInfoList(QDir::Files)) {
        const QString file = entry.filePath();
        if (m_files.contains(file))
            toReadd.append(file);
    }

    if (!toReadd.isEmpty()) {
        const QStringList remove = m_watcher->addPaths(toReadd);
        for (const QString &rejected : remove)
            toReadd.removeOne(rejected);

        // If we've successfully added the file, that means it was deleted and replaced.
        for (const QString &reAdded : std::as_const(toReadd))
            emit fileChanged(reAdded);
    }
}
