// Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QT3DRENDER_QLINEWIDTH_P_H
#define QT3DRENDER_QLINEWIDTH_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <Qt3DRender/private/qrenderstate_p.h>
#include <Qt3DRender/qlinewidth.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

class QLineWidthPrivate : public QRenderStatePrivate
{
public:
    QLineWidthPrivate(float value)
        : QRenderStatePrivate(Render::LineWidthMask)
        , m_value(value)
        , m_smooth(false)
    {}

    float m_value;
    bool m_smooth;

    Q_DECLARE_PUBLIC(QLineWidth)
};

} // namespace Qt3DRender

QT_END_NAMESPACE

#endif // QT3DRENDER_QLINEWIDTH_P_H
