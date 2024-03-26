// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qservicelocator_p.h"

#include <QtCore/QHash>

#include <Qt3DCore/private/nullservices_p.h>
#include <Qt3DCore/private/qabstractserviceprovider_p.h>
#include <Qt3DCore/private/qdownloadhelperservice_p.h>
#include <Qt3DCore/private/qeventfilterservice_p.h>
#include <Qt3DCore/private/qtickclockservice_p.h>
#include <Qt3DCore/private/qsysteminformationservice_p.h>


QT_BEGIN_NAMESPACE

namespace Qt3DCore {

/* !\internal
    \class Qt3DCore::QAbstractServiceProvider
    \inmodule Qt3DCore
*/

QAbstractServiceProvider::QAbstractServiceProvider(int type, const QString &description, QObject *parent)
    : QObject(*new QAbstractServiceProviderPrivate(type, description), parent)
{
}

/* \internal */
QAbstractServiceProvider::QAbstractServiceProvider(QAbstractServiceProviderPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}

QAbstractServiceProvider::~QAbstractServiceProvider()
{
}

int QAbstractServiceProvider::type() const
{
    Q_D(const QAbstractServiceProvider);
    return d->m_type;
}

QString QAbstractServiceProvider::description() const
{
    Q_D(const QAbstractServiceProvider);
    return d->m_description;
}


class QAspectEngine;

class QServiceLocatorPrivate
{
public:
    QServiceLocatorPrivate(QAspectEngine *aspectEngine)
        : m_systemInfo(aspectEngine)
        , m_nonNullDefaultServices(0)
    {}

    QHash<int, QAbstractServiceProvider *> m_services;

    QSystemInformationService m_systemInfo;
    NullOpenGLInformationService m_nullOpenGLInfo;
    QTickClockService m_defaultFrameAdvanceService;
    QEventFilterService m_eventFilterService;
    QDownloadHelperService m_downloadHelperService;
    int m_nonNullDefaultServices;
};


/* !\internal
    \class Qt3DCore::QServiceLocator
    \inmodule Qt3DCore
    \brief Service locator used by aspects to retrieve pointers to concrete service objects

    The Qt3DCore::QServiceLocator class can be used by aspects to obtain pointers to concrete
    providers of abstract service interfaces. A subclass of Qt3DCore::QAbstractServiceProvider
    encapsulates a service that can be provided by an aspect for other parts of the system.
    For example, an aspect may wish to know the current frame number, or how many CPU cores
    are available in the Qt3D tasking threadpool.

    Aspects or the Qt3DCore::QAspectEngine are able to register objects as providers of services.
    The service locator itself can be accessed via the Qt3DCore::QAbstractAspect::services()
    function.

    As a convenience, the service locator provides methods to access services provided by
    built in Qt3D aspects. Currently these are Qt3DCore::QSystemInformationService and
    Qt3DCore::QOpenGLInformationService. For such services, the service provider will never
    return a null pointer. The default implementations of these services are simple null or
    do nothing implementations.
*/

/*
    Creates an instance of QServiceLocator.
*/
QServiceLocator::QServiceLocator(QAspectEngine *aspectEngine)
    : d_ptr(new QServiceLocatorPrivate(aspectEngine))
{
}

/*
   Destroys a QServiceLocator object
*/
QServiceLocator::~QServiceLocator()
{
}

/*
    Registers \a provider service provider for the service \a serviceType. This replaces any
    existing provider for this service. The service provider does not take ownership
    of the provider.

    \sa unregisterServiceProvider(), serviceCount(), service()
*/
void QServiceLocator::registerServiceProvider(int serviceType, QAbstractServiceProvider *provider)
{
    Q_D(QServiceLocator);
    d->m_services.insert(serviceType, provider);
    if (serviceType < DefaultServiceCount)
        ++(d->m_nonNullDefaultServices);
}

/*
    Unregisters any existing provider for the \a serviceType.

    \sa registerServiceProvider()
 */
void QServiceLocator::unregisterServiceProvider(int serviceType)
{
    Q_D(QServiceLocator);
    int removedCount = d->m_services.remove(serviceType);
    if (serviceType < DefaultServiceCount)
        d->m_nonNullDefaultServices -= removedCount;
}

/*
    Returns the number of registered services.
 */
int QServiceLocator::serviceCount() const
{
    Q_D(const QServiceLocator);
    return DefaultServiceCount + d->m_services.size() - d->m_nonNullDefaultServices;
}

/*
    \fn T *Qt3DCore::QServiceLocator::service(int serviceType)

    Returns a pointer to the service provider for \a serviceType. If no provider
    has been explicitly registered, this returns a null pointer for non-Qt3D provided
    default services and a null pointer for non-default services.

    \sa registerServiceProvider()

*/

/*
    Returns a pointer to a provider for the system information service. If no provider
    has been explicitly registered for this service type, then a pointer to a null, do-
    nothing service is returned.
*/
QSystemInformationService *QServiceLocator::systemInformation()
{
    Q_D(QServiceLocator);
    return static_cast<QSystemInformationService *>(d->m_services.value(SystemInformation, &d->m_systemInfo));
}

/*
    Returns a pointer to a provider for the OpenGL information service. If no provider
    has been explicitly registered for this service type, then a pointer to a null, do-
    nothing service is returned.
*/
QOpenGLInformationService *QServiceLocator::openGLInformation()
{
    Q_D(QServiceLocator);
    return static_cast<QOpenGLInformationService *>(d->m_services.value(OpenGLInformation, &d->m_nullOpenGLInfo));
}

/*
    Returns a pointer to a provider for the frame advance service. If no provider
    has been explicitly registered for this service type, then a pointer to a simple timer-based
    service is returned.
*/
QAbstractFrameAdvanceService *QServiceLocator::frameAdvanceService()
{
    Q_D(QServiceLocator);
    return static_cast<QAbstractFrameAdvanceService *>(d->m_services.value(FrameAdvanceService, &d->m_defaultFrameAdvanceService));
}

/*
    Returns a pointer to a provider for the event filter service. If no
    provider has been explicitly registered for this service type, then a
    pointer to the default event filter service is returned.
 */
QEventFilterService *QServiceLocator::eventFilterService()
{
    Q_D(QServiceLocator);
    return static_cast<QEventFilterService *>(d->m_services.value(EventFilterService, &d->m_eventFilterService));
}

QDownloadHelperService *QServiceLocator::downloadHelperService()
{
    Q_D(QServiceLocator);
    return static_cast<QDownloadHelperService *>(d->m_services.value(DownloadHelperService, &d->m_downloadHelperService));
}

/*
    \internal
*/
QAbstractServiceProvider *QServiceLocator::_q_getServiceHelper(int type)
{
    Q_D(QServiceLocator);
    switch (type) {
    case SystemInformation:
        return systemInformation();
    case OpenGLInformation:
        return openGLInformation();
    case FrameAdvanceService:
        return frameAdvanceService();
    case EventFilterService:
        return eventFilterService();
    case DownloadHelperService:
        return downloadHelperService();
    default:
        return d->m_services.value(type, nullptr);
    }
}

}

QT_END_NAMESPACE

#include "moc_qservicelocator_p.cpp"
