/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <dshow.h>

#include <QtCore/qstring.h>
#include <QtCore/qdebug.h>
#include <QtCore/QFile>

#include "directshowglobal.h"
#include "dsserviceplugin.h"

#include "dsvideodevicecontrol.h"
#include <dshow.h>
#include "dscameraservice.h"

#if QT_CONFIG(directshow_player)
#include "directshowplayerservice.h"
#endif

#include <qmediaserviceproviderplugin.h>

extern const CLSID CLSID_VideoInputDeviceCategory;


#ifndef _STRSAFE_H_INCLUDED_
#include <tchar.h>
#endif
#include <dshow.h>
#include <objbase.h>
#include <initguid.h>
#ifdef Q_CC_MSVC
#  pragma comment(lib, "strmiids.lib")
#  pragma comment(lib, "ole32.lib")
#endif // Q_CC_MSVC
#include <windows.h>
#include <ocidl.h>

QT_BEGIN_NAMESPACE
Q_LOGGING_CATEGORY(qtDirectShowPlugin, "qt.multimedia.plugins.directshow")

static int g_refCount = 0;
void addRefCount()
{
    if (++g_refCount == 1)
        CoInitialize(nullptr);
}

void releaseRefCount()
{
    if (--g_refCount == 0)
        CoUninitialize();
}

QMediaService* DSServicePlugin::create(QString const& key)
{
    if (key == QLatin1String(Q_MEDIASERVICE_CAMERA)) {
        addRefCount();
        return new DSCameraService;
    }
#if QT_CONFIG(directshow_player)
    if (key == QLatin1String(Q_MEDIASERVICE_MEDIAPLAYER)) {
        addRefCount();
        return new DirectShowPlayerService;
    }
#endif

    return 0;
}

void DSServicePlugin::release(QMediaService *service)
{
    delete service;
    releaseRefCount();
}

QMediaServiceProviderHint::Features DSServicePlugin::supportedFeatures(
        const QByteArray &service) const
{
    return service == Q_MEDIASERVICE_MEDIAPLAYER
        ? (QMediaServiceProviderHint::StreamPlayback | QMediaServiceProviderHint::VideoSurface)
        : QMediaServiceProviderHint::Features();
}

QByteArray DSServicePlugin::defaultDevice(const QByteArray &service) const
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        addRefCount();
        const QList<DSVideoDeviceInfo> &devs = DSVideoDeviceControl::availableDevices();
        releaseRefCount();
        if (!devs.isEmpty())
            return devs.first().first;
    }
    return QByteArray();
}

QList<QByteArray> DSServicePlugin::devices(const QByteArray &service) const
{
    QList<QByteArray> result;

    if (service == Q_MEDIASERVICE_CAMERA) {
        addRefCount();
        const QList<DSVideoDeviceInfo> &devs = DSVideoDeviceControl::availableDevices();
        releaseRefCount();
        for (const DSVideoDeviceInfo &info : devs)
            result.append(info.first);
    }

    return result;
}

QString DSServicePlugin::deviceDescription(const QByteArray &service, const QByteArray &device)
{
    if (service == Q_MEDIASERVICE_CAMERA) {
        addRefCount();
        const QList<DSVideoDeviceInfo> &devs = DSVideoDeviceControl::availableDevices();
        releaseRefCount();
        for (const DSVideoDeviceInfo &info : devs) {
            if (info.first == device)
                return info.second;
        }
    }
    return QString();
}

QT_END_NAMESPACE
