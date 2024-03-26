// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDEVICEDISCOVERY_DUMMY_H
#define QDEVICEDISCOVERY_DUMMY_H

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

#include "qdevicediscovery_p.h"

QT_BEGIN_NAMESPACE

class QDeviceDiscoveryDummy : public QDeviceDiscovery
{
    Q_OBJECT

public:
    QDeviceDiscoveryDummy(QDeviceTypes types, QObject *parent = nullptr);
    QStringList scanConnectedDevices() override;
};

QT_END_NAMESPACE

#endif // QDEVICEDISCOVERY_DUMMY_H
