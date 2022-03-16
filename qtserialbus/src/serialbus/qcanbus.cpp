/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtSerialBus module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcanbus.h"
#include "qcanbusfactory.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qglobalstatic.h>
#include <QtCore/qlist.h>
#include <QtCore/qobject.h>
#include <QtCore/qpluginloader.h>

#include <private/qfactoryloader_p.h>

#define QCanBusFactory_iid "org.qt-project.Qt.QCanBusFactory"

QT_BEGIN_NAMESPACE

class QCanBusPrivate
{
public:
    QCanBusPrivate() { }
    QCanBusPrivate(int index, const QJsonObject &meta) : meta(meta), index(index) {}

    QJsonObject meta;
    QObject *factory = nullptr;
    int index = -1;
};

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, qFactoryLoader,
    (QCanBusFactory_iid, QLatin1String("/canbus")))

typedef QMap<QString, QCanBusPrivate> QCanBusPluginStore;
Q_GLOBAL_STATIC(QCanBusPluginStore, qCanBusPlugins)

static QCanBus *globalInstance = nullptr;

static void loadPlugins()
{
    const QList<QJsonObject> meta = qFactoryLoader()->metaData();
    for (int i = 0; i < meta.count(); i++) {
        const QJsonObject obj = meta.at(i).value(QLatin1String("MetaData")).toObject();
        if (obj.isEmpty())
            continue;

        qCanBusPlugins()->insert(obj.value(QLatin1String("Key")).toString(), {i, obj});
    }
}

/*!
    \class QCanBus
    \inmodule QtSerialBus
    \since 5.8

    \brief The QCanBus class handles registration and creation of bus plugins.

    QCanBus loads Qt CAN Bus plugins at runtime. The ownership of serial bus plugins is
    transferred to the loader.
*/

/*!
    Returns a pointer to the QCanBus class. The object is loaded if necessary. QCanBus
    uses the singleton design pattern.
*/
QCanBus *QCanBus::instance()
{
    if (!globalInstance)
        globalInstance = new QCanBus();
    return globalInstance;
}

/*!
    Returns a list of identifiers for all loaded plugins.
*/
QStringList QCanBus::plugins() const
{
    return qCanBusPlugins()->keys();
}

static void setErrorMessage(QString *result, const QString &message)
{
    if (!result)
        return;

    *result = message;
}

static QObject *canBusFactory(const QString &plugin, QString *errorMessage)
{
    if (Q_UNLIKELY(!qCanBusPlugins()->contains(plugin))) {
        setErrorMessage(errorMessage, QCanBus::tr("No such plugin: '%1'").arg(plugin));
        return nullptr;
    }

    QCanBusPrivate d = qCanBusPlugins()->value(plugin);
    if (!d.factory) {
        d.factory = qFactoryLoader->instance(d.index);

        if (d.factory)
            qCanBusPlugins()->insert(plugin, d);
    }

    if (Q_UNLIKELY(!d.factory))
        setErrorMessage(errorMessage, QCanBus::tr("No factory for plugin: '%1'").arg(plugin));

    return d.factory;
}

/*!
    \since 5.9

    Returns the available interfaces for \a plugin. In case of failure, the optional
    parameter \a errorMessage returns a textual error description.

    \note Some plugins might not or only partially support this function.

    For example, the following call returns a list of all available SocketCAN
    interfaces (which can be used for \l createDevice()):

    \code
        QString errorString;
        const QList<QCanBusDeviceInfo> devices = QCanBus::instance()->availableDevices(
            QStringLiteral("socketcan"), &errorString);
        if (!errorString.isEmpty())
            qDebug() << errorString;
    \endcode

    \sa createDevice()
*/
QList<QCanBusDeviceInfo> QCanBus::availableDevices(const QString &plugin, QString *errorMessage) const
{
    const QObject *obj = canBusFactory(plugin, errorMessage);
    if (Q_UNLIKELY(!obj))
        return QList<QCanBusDeviceInfo>();

    const QCanBusFactoryV2 *factoryV2 = qobject_cast<QCanBusFactoryV2 *>(obj);
    if (Q_UNLIKELY(!factoryV2)) {
        setErrorMessage(errorMessage,
                        tr("The plugin '%1' does not provide this function.").arg(plugin));
        return QList<QCanBusDeviceInfo>();
    }

    QString errorString;
    QList<QCanBusDeviceInfo> result = factoryV2->availableDevices(&errorString);

    setErrorMessage(errorMessage, errorString);
    return result;
}

/*!
    Creates a CAN bus device. \a plugin is the name of the plugin as returned by the \l plugins()
    method. \a interfaceName is the CAN bus interface name. In case of failure, the optional
    parameter \a errorMessage returns a textual error description.

    Ownership of the returned plugin is transferred to the caller.
    Returns \c nullptr if no suitable device can be found.

    For example, the following call would connect to the SocketCAN interface vcan0:

    \code
        QString errorString;
        QCanBusDevice *device = QCanBus::instance()->createDevice(
            QStringLiteral("socketcan"), QStringLiteral("vcan0"), &errorString);
        if (!device)
            qDebug() << errorString;
        else
            device->connectDevice();
    \endcode

    \note The \a interfaceName is plugin-dependent. See the corresponding plugin documentation
    for more information: \l {CAN Bus Plugins}. To get a list of available interfaces,
    \l availableDevices() can be used.

    \sa availableDevices()
*/
QCanBusDevice *QCanBus::createDevice(const QString &plugin, const QString &interfaceName,
                                     QString *errorMessage) const
{
    const QObject *obj = canBusFactory(plugin, errorMessage);
    if (Q_UNLIKELY(!obj))
        return nullptr;

    const QCanBusFactoryV2 *factoryV2 = qobject_cast<QCanBusFactoryV2 *>(obj);
    if (Q_LIKELY(factoryV2))
        return factoryV2->createDevice(interfaceName, errorMessage);

    const QCanBusFactory *factory = qobject_cast<QCanBusFactory *>(obj);
    if (factory)
        return factory->createDevice(interfaceName, errorMessage);

    setErrorMessage(errorMessage,
                    tr("The plugin '%1' does not provide this function.").arg(plugin));
    return nullptr;
}

QCanBus::QCanBus(QObject *parent) :
    QObject(parent)
{
    loadPlugins();
}

QT_END_NAMESPACE
