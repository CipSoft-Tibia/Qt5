// Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdownloadhelperservice_p.h"
#include "qdownloadnetworkworker_p.h"
#include <QtCore/QThread>
#include <Qt3DCore/QAspectEngine>
#include <Qt3DCore/private/qabstractserviceprovider_p.h>
#include <Qt3DCore/private/qaspectengine_p.h>
#include <Qt3DCore/private/qaspectmanager_p.h>
#include <Qt3DCore/private/qservicelocator_p.h>

#include <QFile>

QT_BEGIN_NAMESPACE

namespace Qt3DCore {

QDownloadRequest::QDownloadRequest(const QUrl &url)
    : m_url(url)
    , m_succeeded(false)
    , m_cancelled(false)
{

}

QDownloadRequest::~QDownloadRequest()
{

}

void QDownloadRequest::onDownloaded()
{
    // this is called in dl thread. It's an opportunity to do long running tasks
    // like loading the data into a QImage
}


class Q_AUTOTEST_EXPORT QDownloadHelperServicePrivate : public QAbstractServiceProviderPrivate
{
public:
    explicit QDownloadHelperServicePrivate(const QString &description);
    ~QDownloadHelperServicePrivate();

    void init();
    void shutdown();
    void _q_onRequestCompleted(const QDownloadRequestPtr &request);

    Q_DECLARE_PUBLIC(QDownloadHelperService)

    QThread *m_downloadThread;
    QDownloadNetworkWorker *m_downloadWorker;
};


QDownloadHelperServicePrivate::QDownloadHelperServicePrivate(const QString &description)
    : QAbstractServiceProviderPrivate(QServiceLocator::DownloadHelperService, description)
    , m_downloadThread(nullptr)
    , m_downloadWorker(nullptr)
{
}

QDownloadHelperServicePrivate::~QDownloadHelperServicePrivate()
{
}

void QDownloadHelperServicePrivate::init()
{
    Q_Q(QDownloadHelperService);
    m_downloadThread = new QThread(q);
    m_downloadWorker = new QDownloadNetworkWorker;
    m_downloadWorker->moveToThread(m_downloadThread);
    // QueuedConnection
    QObject::connect(m_downloadWorker, SIGNAL(requestDownloaded(const Qt3DCore::QDownloadRequestPtr &)),
                     q, SLOT(_q_onRequestCompleted(const Qt3DCore::QDownloadRequestPtr &)));
    m_downloadThread->start();
}

void QDownloadHelperServicePrivate::shutdown()
{
    emit m_downloadWorker->cancelAllRequests();
    m_downloadThread->exit();
    m_downloadThread->wait();
    m_downloadWorker->deleteLater();
}

// Executed in AspectThread (queued signal connected to download thread)
void QDownloadHelperServicePrivate::_q_onRequestCompleted(const Qt3DCore::QDownloadRequestPtr &request)
{
    request->onCompleted();
}


QDownloadHelperService::QDownloadHelperService(const QString &description)
    : QAbstractServiceProvider(*new QDownloadHelperServicePrivate(description))
{
    Q_D(QDownloadHelperService);
    d->init();
    qRegisterMetaType<Qt3DCore::QDownloadRequestPtr>();
}

QDownloadHelperService::~QDownloadHelperService()
{
    Q_D(QDownloadHelperService);
    d->shutdown();
}

void QDownloadHelperService::submitRequest(const Qt3DCore::QDownloadRequestPtr &request)
{
    Q_D(QDownloadHelperService);

    if (isLocal(request->url())) {
        QFile file(urlToLocalFileOrQrc(request->url()));
        if (file.open(QIODevice::ReadOnly)) {
            request->m_data = file.readAll();
            file.close();
            request->m_succeeded = true;
        } else {
            request->m_succeeded = false;
        }
        request->onCompleted();
    } else {
        emit d->m_downloadWorker->submitRequest(request);
    }
}

void QDownloadHelperService::cancelRequest(const Qt3DCore::QDownloadRequestPtr &request)
{
    Q_D(QDownloadHelperService);
    request->m_cancelled = true;
    emit d->m_downloadWorker->cancelRequest(request);
}

void QDownloadHelperService::cancelAllRequests()
{
    Q_D(QDownloadHelperService);
    emit d->m_downloadWorker->cancelAllRequests();
}

QString QDownloadHelperService::urlToLocalFileOrQrc(const QUrl &url)
{
    const QString scheme(url.scheme().toLower());
    if (scheme == QLatin1String("qrc")) {
        if (url.authority().isEmpty())
            return QLatin1Char(':') + url.path();
        return QString();
    }

#if defined(Q_OS_ANDROID)
    if (scheme == QLatin1String("assets")) {
        if (url.authority().isEmpty())
            return url.toString();
        return QString();
    }
#endif

    return url.toLocalFile();
}

QDownloadHelperService *QDownloadHelperService::getService(QAspectEngine *engine)
{
    auto enginePrivate = Qt3DCore::QAspectEnginePrivate::get(engine);
    return enginePrivate->m_aspectManager->serviceLocator()->downloadHelperService();
}

bool QDownloadHelperService::isLocal(const QUrl &url)
{
    const QString scheme(url.scheme().toLower());
    if (scheme == QLatin1String("file") || scheme == QLatin1String("qrc"))
        return true;
#if defined(Q_OS_ANDROID)
    if (scheme == QLatin1String("assets"))
        return true;
#endif
    return false;
}

} // namespace Qt3DCore

QT_END_NAMESPACE

#include "moc_qdownloadhelperservice_p.cpp"
