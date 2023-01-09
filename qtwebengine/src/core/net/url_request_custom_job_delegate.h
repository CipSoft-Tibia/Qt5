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

#ifndef URL_REQUEST_CUSTOM_JOB_DELEGATE_H_
#define URL_REQUEST_CUSTOM_JOB_DELEGATE_H_

#include "base/memory/ref_counted.h"
#include "qtwebenginecoreglobal_p.h"

#include <QMap>
#include <QObject>
#include <QUrl>

QT_FORWARD_DECLARE_CLASS(QIODevice)

namespace QtWebEngineCore {

class URLRequestCustomJobProxy;

class Q_WEBENGINECORE_PRIVATE_EXPORT URLRequestCustomJobDelegate : public QObject
{
    Q_OBJECT
public:
    ~URLRequestCustomJobDelegate();

    enum Error {
        NoError = 0,
        UrlNotFound,
        UrlInvalid,
        RequestAborted,
        RequestDenied,
        RequestFailed
    };

    QUrl url() const;
    QByteArray method() const;
    QUrl initiator() const;
    QMap<QByteArray, QByteArray> requestHeaders() const;

    void reply(const QByteArray &contentType, QIODevice *device);
    void redirect(const QUrl &url);
    void abort();
    void fail(Error);

private Q_SLOTS:
    void slotReadyRead();

private:
    URLRequestCustomJobDelegate(URLRequestCustomJobProxy *proxy,
                                const QUrl &url,
                                const QByteArray &method,
                                const QUrl &initiatorOrigin,
                                const QMap<QByteArray, QByteArray> &requestHeaders);

    friend class URLRequestCustomJobProxy;
    scoped_refptr<URLRequestCustomJobProxy> m_proxy;
    QUrl m_request;
    QByteArray m_method;
    QUrl m_initiatorOrigin;
    const QMap<QByteArray, QByteArray> m_requestHeaders;
};

} // namespace

#endif // URL_REQUEST_CUSTOM_JOB_DELEGATE_H_
