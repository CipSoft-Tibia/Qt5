/****************************************************************************
**
** Copyright (C) 2016 Alexander Volkov <a.volkov@rusbitech.ru>
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSensors module of the Qt Toolkit.
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

#include "iiosensorproxyorientationsensor.h"
#include "iiosensorproxylightsensor.h"
#include "iiosensorproxycompass.h"

#include <qsensorplugin.h>
#include <qsensorbackend.h>
#include <qsensormanager.h>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>

#include <QtCore/QFile>
#include <QtCore/QDebug>

class IIOSensorProxySensorPlugin : public QObject, public QSensorPluginInterface, public QSensorBackendFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.qt-project.Qt.QSensorPluginInterface/1.0" FILE "plugin.json")
    Q_INTERFACES(QSensorPluginInterface)
public:
    void registerSensors() override
    {
        if (QDBusConnection::systemBus().interface()->isServiceRegistered("net.hadess.SensorProxy")) {
            if (!QSensorManager::isBackendRegistered(QOrientationSensor::type, IIOSensorProxyOrientationSensor::id))
                QSensorManager::registerBackend(QOrientationSensor::type, IIOSensorProxyOrientationSensor::id, this);
            if (!QSensorManager::isBackendRegistered(QLightSensor::type, IIOSensorProxyLightSensor::id))
                QSensorManager::registerBackend(QLightSensor::type, IIOSensorProxyLightSensor::id, this);
            if (!QSensorManager::isBackendRegistered(QCompass::type, IIOSensorProxyCompass::id))
                QSensorManager::registerBackend(QCompass::type, IIOSensorProxyCompass::id, this);
        }
    }

    QSensorBackend *createBackend(QSensor *sensor) override
    {
        if (sensor->identifier() == IIOSensorProxyOrientationSensor::id)
            return new IIOSensorProxyOrientationSensor(sensor);
        else if (sensor->identifier() == IIOSensorProxyLightSensor::id)
            return new IIOSensorProxyLightSensor(sensor);
        else if (sensor->identifier() == IIOSensorProxyCompass::id)
            return new IIOSensorProxyCompass(sensor);

        return 0;
    }
};

#include "main.moc"
