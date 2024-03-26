// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDIRECTFBEGL_HOOKS_H
#define QDIRECTFBEGL_HOOKS_H

#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

struct QDirectFBEGLHooks {
    void platformInit();
    void platformDestroy();
    bool hasCapability(QPlatformIntegration::Capability) const;
};

QT_END_NAMESPACE

#endif
