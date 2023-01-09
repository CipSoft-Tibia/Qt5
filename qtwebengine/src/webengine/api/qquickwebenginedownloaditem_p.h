/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
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

#ifndef QQUICKWEBENGINEDOWNLOADITEM_P_H
#define QQUICKWEBENGINEDOWNLOADITEM_P_H

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

#include <QtWebEngine/private/qtwebengineglobal_p.h>
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QUrl>

QT_BEGIN_NAMESPACE

class QQuickWebEngineDownloadItemPrivate;
class QQuickWebEngineProfilePrivate;
class QQuickWebEngineView;

class Q_WEBENGINE_PRIVATE_EXPORT QQuickWebEngineDownloadItem: public QObject {
    Q_OBJECT
public:
    ~QQuickWebEngineDownloadItem();
    enum DownloadState {
        DownloadRequested,
        DownloadInProgress,
        DownloadCompleted,
        DownloadCancelled,
        DownloadInterrupted
    };
    Q_ENUM(DownloadState)

    enum SavePageFormat {
        UnknownSaveFormat = -1,
        SingleHtmlSaveFormat,
        CompleteHtmlSaveFormat,
        MimeHtmlSaveFormat
    };
    Q_ENUM(SavePageFormat)

    enum DownloadInterruptReason {
        NoReason = 0,
        FileFailed = 1,
        FileAccessDenied = 2,
        FileNoSpace = 3,
        FileNameTooLong = 5,
        FileTooLarge = 6,
        FileVirusInfected = 7,
        FileTransientError = 10,
        FileBlocked = 11,
        FileSecurityCheckFailed = 12,
        FileTooShort = 13,
        FileHashMismatch = 14,
        NetworkFailed = 20,
        NetworkTimeout = 21,
        NetworkDisconnected = 22,
        NetworkServerDown = 23,
        NetworkInvalidRequest = 24,
        ServerFailed = 30,
        //ServerNoRange = 31,
        ServerBadContent = 33,
        ServerUnauthorized = 34,
        ServerCertProblem = 35,
        ServerForbidden = 36,
        ServerUnreachable = 37,
        UserCanceled = 40,
        //UserShutdown = 41,
        //Crash = 50
    };
    Q_ENUM(DownloadInterruptReason)

    enum DownloadType {
        Attachment = 0,
        DownloadAttribute,
        UserRequested,
        SavePage
    };
    Q_ENUM(DownloadType)

    Q_PROPERTY(quint32 id READ id CONSTANT FINAL)
    Q_PROPERTY(DownloadState state READ state NOTIFY stateChanged FINAL)
    Q_PROPERTY(SavePageFormat savePageFormat READ savePageFormat WRITE setSavePageFormat NOTIFY savePageFormatChanged REVISION 2 FINAL)
    Q_PROPERTY(qint64 totalBytes READ totalBytes NOTIFY totalBytesChanged FINAL)
    Q_PROPERTY(qint64 receivedBytes READ receivedBytes NOTIFY receivedBytesChanged FINAL)
    Q_PROPERTY(QString mimeType READ mimeType NOTIFY mimeTypeChanged REVISION 1 FINAL)
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged FINAL)
    Q_PROPERTY(DownloadType type READ type NOTIFY typeChanged REVISION 3 FINAL)
    Q_PROPERTY(DownloadInterruptReason interruptReason READ interruptReason NOTIFY interruptReasonChanged REVISION 4 FINAL)
    Q_PROPERTY(QString interruptReasonString READ interruptReasonString NOTIFY interruptReasonChanged REVISION 4 FINAL)
    Q_PROPERTY(bool isFinished READ isFinished NOTIFY isFinishedChanged REVISION 5 FINAL)
    Q_PROPERTY(bool isPaused READ isPaused NOTIFY isPausedChanged REVISION 5 FINAL)
    Q_PROPERTY(bool isSavePageDownload READ isSavePageDownload CONSTANT REVISION 6 FINAL)
    Q_PROPERTY(QQuickWebEngineView *view READ view CONSTANT REVISION 7 FINAL)
    Q_PROPERTY(QUrl url READ url CONSTANT REVISION 10 FINAL)
    Q_PROPERTY(QString suggestedFileName READ suggestedFileName CONSTANT REVISION 10 FINAL)
    Q_PROPERTY(QString downloadDirectory READ downloadDirectory WRITE setDownloadDirectory NOTIFY downloadDirectoryChanged REVISION 10 FINAL)
    Q_PROPERTY(QString downloadFileName READ downloadFileName WRITE setDownloadFileName NOTIFY downloadFileNameChanged REVISION 10 FINAL)

    Q_INVOKABLE void accept();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();

    quint32 id() const;
    DownloadState state() const;
    qint64 totalBytes() const;
    qint64 receivedBytes() const;
    QString mimeType() const;
    QString path() const;
    void setPath(QString path);
    SavePageFormat savePageFormat() const;
    void setSavePageFormat(SavePageFormat format);
    DownloadType Q_DECL_DEPRECATED type() const;
    DownloadInterruptReason interruptReason() const;
    QString interruptReasonString() const;
    bool isFinished() const;
    bool isPaused() const;
    bool isSavePageDownload() const;
    QQuickWebEngineView *view() const;
    QUrl url() const;
    QString suggestedFileName() const;
    QString downloadDirectory() const;
    void setDownloadDirectory(const QString &directory);
    QString downloadFileName() const;
    void setDownloadFileName(const QString &fileName);
    bool isCustomFileName;

Q_SIGNALS:
    void stateChanged();
    Q_REVISION(2) void savePageFormatChanged();
    void receivedBytesChanged();
    void totalBytesChanged();
    Q_REVISION(1) void mimeTypeChanged();
    void pathChanged();
    Q_REVISION(3) void typeChanged();
    Q_REVISION(4) void interruptReasonChanged();
    Q_REVISION(5) void isFinishedChanged();
    Q_REVISION(5) void isPausedChanged();
    Q_REVISION(10) void downloadDirectoryChanged();
    Q_REVISION(10) void downloadFileNameChanged();

private:
    QQuickWebEngineDownloadItem(QQuickWebEngineDownloadItemPrivate*, QObject *parent = 0);
    Q_DISABLE_COPY(QQuickWebEngineDownloadItem)
    Q_DECLARE_PRIVATE(QQuickWebEngineDownloadItem)
    friend class QQuickWebEngineProfilePrivate;

    QScopedPointer<QQuickWebEngineDownloadItemPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QQUICKWEBENGINEDOWNLOADITEM_P_H
