// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "fileinfothread_p.h"
#include <qdiriterator.h>
#include <qpointer.h>
#include <qtimer.h>

#include <QDebug>
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcFileInfoThread, "qt.labs.folderlistmodel.fileinfothread")

FileInfoThread::FileInfoThread(QObject *parent)
    : QThread(parent),
      abort(false),
      scanPending(false),
#if QT_CONFIG(filesystemwatcher)
      watcher(nullptr),
#endif
      sortFlags(QDir::Name),
      needUpdate(true),
      updateTypes(UpdateType::None),
      showFiles(true),
      showDirs(true),
      showDirsFirst(false),
      showDotAndDotDot(false),
      showHidden(false),
      showOnlyReadable(false),
      caseSensitive(true)
{
#if QT_CONFIG(filesystemwatcher)
    watcher = new QFileSystemWatcher(this);
    connect(watcher, SIGNAL(directoryChanged(QString)), this, SLOT(dirChanged(QString)));
    connect(watcher, SIGNAL(fileChanged(QString)), this, SLOT(updateFile(QString)));
#endif // filesystemwatcher
}

FileInfoThread::~FileInfoThread()
{
    QMutexLocker locker(&mutex);
    abort = true;
    condition.wakeOne();
    locker.unlock();
    wait();
}

void FileInfoThread::clear()
{
    QMutexLocker locker(&mutex);
#if QT_CONFIG(filesystemwatcher)
    watcher->removePaths(watcher->files());
    watcher->removePaths(watcher->directories());
#endif
}

void FileInfoThread::removePath(const QString &path)
{
    QMutexLocker locker(&mutex);
#if QT_CONFIG(filesystemwatcher)
    if (!path.startsWith(QLatin1Char(':')))
        watcher->removePath(path);
#else
    Q_UNUSED(path);
#endif
    currentPath.clear();
}

void FileInfoThread::setPath(const QString &path)
{
    qCDebug(lcFileInfoThread) << "setPath called with path" << path;
    Q_ASSERT(!path.isEmpty());

    QMutexLocker locker(&mutex);
#if QT_CONFIG(filesystemwatcher)
    if (!path.startsWith(QLatin1Char(':')))
        watcher->addPath(path);
#endif
    currentPath = path;
    needUpdate = true;
    initiateScan();
}

void FileInfoThread::setRootPath(const QString &path)
{
    qCDebug(lcFileInfoThread) << "setRootPath called with path" << path;
    Q_ASSERT(!path.isEmpty());

    QMutexLocker locker(&mutex);
    rootPath = path;
}

#if QT_CONFIG(filesystemwatcher)
void FileInfoThread::dirChanged(const QString &directoryPath)
{
    qCDebug(lcFileInfoThread) << "dirChanged called with directoryPath" << directoryPath;
    Q_UNUSED(directoryPath);
    QMutexLocker locker(&mutex);
    updateTypes |= UpdateType::Contents;
    initiateScan();
}
#endif

void FileInfoThread::setSortFlags(QDir::SortFlags flags)
{
    qCDebug(lcFileInfoThread) << "setSortFlags called with flags" << flags;
    Q_ASSERT(flags != sortFlags);
    QMutexLocker locker(&mutex);
    sortFlags = flags;
    updateTypes |= UpdateType::Sort;
    needUpdate = true;
    initiateScan();
}

void FileInfoThread::setNameFilters(const QStringList & filters)
{
    qCDebug(lcFileInfoThread) << "setNameFilters called with filters" << filters;
    QMutexLocker locker(&mutex);
    nameFilters = filters;
    updateTypes |= UpdateType::Contents;
    initiateScan();
}

void FileInfoThread::setShowFiles(bool show)
{
    qCDebug(lcFileInfoThread) << "setShowFiles called with show" << show;
    QMutexLocker locker(&mutex);
    showFiles = show;
    updateTypes |= UpdateType::Contents;
    initiateScan();
}

void FileInfoThread::setShowDirs(bool showFolders)
{
    qCDebug(lcFileInfoThread) << "setShowDirs called with showFolders" << showFolders;
    QMutexLocker locker(&mutex);
    showDirs = showFolders;
    updateTypes |= UpdateType::Contents;
    initiateScan();
}

void FileInfoThread::setShowDirsFirst(bool show)
{
    qCDebug(lcFileInfoThread) << "setShowDirsFirst called with show" << show;
    QMutexLocker locker(&mutex);
    showDirsFirst = show;
    updateTypes |= UpdateType::Contents;
    initiateScan();
}

void FileInfoThread::setShowDotAndDotDot(bool on)
{
    qCDebug(lcFileInfoThread) << "setShowDotAndDotDot called with on" << on;
    QMutexLocker locker(&mutex);
    showDotAndDotDot = on;
    updateTypes |= UpdateType::Contents;
    needUpdate = true;
    initiateScan();
}

void FileInfoThread::setShowHidden(bool on)
{
    qCDebug(lcFileInfoThread) << "setShowHidden called with on" << on;
    QMutexLocker locker(&mutex);
    showHidden = on;
    updateTypes |= UpdateType::Contents;
    needUpdate = true;
    initiateScan();
}

void FileInfoThread::setShowOnlyReadable(bool on)
{
    qCDebug(lcFileInfoThread) << "setShowOnlyReadable called with on" << on;
    QMutexLocker locker(&mutex);
    showOnlyReadable = on;
    updateTypes |= UpdateType::Contents;
    initiateScan();
}

void FileInfoThread::setCaseSensitive(bool on)
{
    qCDebug(lcFileInfoThread) << "setCaseSensitive called with on" << on;
    QMutexLocker locker(&mutex);
    caseSensitive = on;
    updateTypes |= UpdateType::Contents;
    initiateScan();
}

#if QT_CONFIG(filesystemwatcher)
void FileInfoThread::updateFile(const QString &path)
{
    qCDebug(lcFileInfoThread) << "updateFile called with path" << path;
    Q_UNUSED(path);
    QMutexLocker locker(&mutex);
    updateTypes |= UpdateType::Contents;
    initiateScan();
}
#endif

void FileInfoThread::run()
{
    forever {
        bool updateFiles = false;
        QMutexLocker locker(&mutex);
        if (abort) {
            return;
        }
        if (currentPath.isEmpty() || !needUpdate) {
            emit statusChanged(currentPath.isEmpty() ? QQuickFolderListModel::Null : QQuickFolderListModel::Ready);
            condition.wait(&mutex);
        }

        if (abort) {
            return;
        }

        if (!currentPath.isEmpty()) {
            updateFiles = true;
            emit statusChanged(QQuickFolderListModel::Loading);
        }
        if (updateFiles)
            getFileInfos(currentPath);
        locker.unlock();
    }
}

void FileInfoThread::runOnce()
{
    if (scanPending)
        return;
    scanPending = true;
    QPointer<FileInfoThread> guardedThis(this);

    auto getFileInfosAsync = [guardedThis](){
        if (!guardedThis)
            return;
        guardedThis->scanPending = false;
        if (guardedThis->currentPath.isEmpty()) {
            emit guardedThis->statusChanged(QQuickFolderListModel::Null);
            return;
        }
        emit guardedThis->statusChanged(QQuickFolderListModel::Loading);
        guardedThis->getFileInfos(guardedThis->currentPath);
        emit guardedThis->statusChanged(QQuickFolderListModel::Ready);
    };

    QTimer::singleShot(0, getFileInfosAsync);
}

void FileInfoThread::initiateScan()
{
#if QT_CONFIG(thread)
    qCDebug(lcFileInfoThread) << "initiateScan is about to call condition.wakeAll()";
    condition.wakeAll();
#else
    qCDebug(lcFileInfoThread) << "initiateScan is about to call runOnce()";
    runOnce();
#endif
}

QString fileInfoListToString(const QFileInfoList &fileInfoList)
{
    return fileInfoList.size() <= 10
        ? QDebug::toString(fileInfoList)
        : QString::fromLatin1("%1 files").arg(fileInfoList.size());
}

void FileInfoThread::getFileInfos(const QString &path)
{
    qCDebug(lcFileInfoThread) << "getFileInfos called with path" << path << "- updateType" << updateTypes;

    QDir::Filters filter;
    if (caseSensitive)
        filter = QDir::CaseSensitive;
    if (showFiles)
        filter = filter | QDir::Files;
    if (showDirs)
        filter = filter | QDir::AllDirs | QDir::Drives;
    if (!showDotAndDotDot)
        filter = filter | QDir::NoDot | QDir::NoDotDot;
    else if (path == rootPath)
        filter = filter | QDir::NoDotDot;
    if (showHidden)
        filter = filter | QDir::Hidden;
    if (showOnlyReadable)
        filter = filter | QDir::Readable;
    if (showDirsFirst)
        sortFlags = sortFlags | QDir::DirsFirst;

    QDir currentDir(path, QString(), sortFlags);
    QList<FileProperty> filePropertyList;

    const QFileInfoList fileInfoList = currentDir.entryInfoList(nameFilters, filter, sortFlags);

    if (!fileInfoList.isEmpty()) {
        filePropertyList.reserve(fileInfoList.size());
        for (const QFileInfo &info : fileInfoList)
            filePropertyList << FileProperty(info);

        if (updateTypes & UpdateType::Contents) {
            int fromIndex = 0;
            int toIndex = currentFileList.size()-1;
            findChangeRange(filePropertyList, fromIndex, toIndex);
            currentFileList = filePropertyList;
            qCDebug(lcFileInfoThread) << "- about to emit directoryUpdated with fromIndex" << fromIndex
                << "toIndex" << toIndex << "fileInfoList" << fileInfoListToString(fileInfoList);
            emit directoryUpdated(path, filePropertyList, fromIndex, toIndex);
        } else {
            currentFileList = filePropertyList;
            if (updateTypes & UpdateType::Sort) {
                qCDebug(lcFileInfoThread) << "- about to emit sortFinished - fileInfoList:"
                    << fileInfoListToString(fileInfoList);
                emit sortFinished(filePropertyList);
            } else {
                qCDebug(lcFileInfoThread) << "- about to emit directoryChanged - fileInfoList:"
                    << fileInfoListToString(fileInfoList);
                emit directoryChanged(path, filePropertyList);
            }
        }
    } else {
        // The directory is empty
        if (updateTypes & UpdateType::Contents) {
            int fromIndex = 0;
            int toIndex = currentFileList.size()-1;
            currentFileList.clear();
            qCDebug(lcFileInfoThread) << "- directory is empty, about to emit directoryUpdated with fromIndex"
                << fromIndex << "toIndex" << toIndex;
            emit directoryUpdated(path, filePropertyList, fromIndex, toIndex);
        } else {
            currentFileList.clear();
            qCDebug(lcFileInfoThread) << "- directory is empty, about to emit directoryChanged";
            emit directoryChanged(path, filePropertyList);
        }
    }
    updateTypes = UpdateType::None;
    needUpdate = false;
}

void FileInfoThread::findChangeRange(const QList<FileProperty> &list, int &fromIndex, int &toIndex)
{
    if (currentFileList.size() == 0) {
        fromIndex = 0;
        toIndex = list.size();
        return;
    }

    int i;
    int listSize = list.size() < currentFileList.size() ? list.size() : currentFileList.size();
    bool changeFound = false;

    for (i=0; i < listSize; i++) {
        if (list.at(i) != currentFileList.at(i)) {
            changeFound = true;
            break;
        }
    }

    if (changeFound)
        fromIndex = i;
    else
        fromIndex = i-1;

    // For now I let the rest of the list be updated..
    toIndex = list.size() > currentFileList.size() ? list.size() - 1 : currentFileList.size() - 1;
}

constexpr FileInfoThread::UpdateTypes operator|(FileInfoThread::UpdateType f1, FileInfoThread::UpdateTypes f2) noexcept
{
    return f2 | f1;
}

constexpr FileInfoThread::UpdateTypes operator&(FileInfoThread::UpdateType f1, FileInfoThread::UpdateTypes f2) noexcept
{
    return f2 & f1;
}

QT_END_NAMESPACE

#include "moc_fileinfothread_p.cpp"
