// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXVIDEODEVICES_H
#define QQNXVIDEODEVICES_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qplatformvideodevices_p.h>

QT_BEGIN_NAMESPACE

class QQnxVideoDevices : public QPlatformVideoDevices
{
public:
    explicit QQnxVideoDevices(QPlatformMediaIntegration *integration);

    QList<QCameraDevice> videoDevices() const override;
};

QT_END_NAMESPACE

#endif
