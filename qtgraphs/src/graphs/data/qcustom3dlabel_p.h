// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtGraphs API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#ifndef QCUSTOMLABELITEM_P_H
#define QCUSTOMLABELITEM_P_H

#include "qcustom3dlabel.h"
#include "qcustom3ditem_p.h"

QT_BEGIN_NAMESPACE

class QCustom3DLabelPrivate : public QCustom3DItemPrivate
{
    Q_DECLARE_PUBLIC(QCustom3DLabel)

public:
    QCustom3DLabelPrivate(QCustom3DLabel *q);
    QCustom3DLabelPrivate(QCustom3DLabel *q, const QString &text, const QFont &font,
                          const QVector3D &position, const QVector3D &scaling,
                          const QQuaternion &rotation);
    virtual ~QCustom3DLabelPrivate();

    void resetDirtyBits();

public:
    QString m_text;
    QFont m_font;
    QColor m_bgrColor;
    QColor m_txtColor;
    bool m_background;
    bool m_borders;
    bool m_facingCamera;

    bool m_customVisuals;

    bool m_facingCameraDirty;
};

QT_END_NAMESPACE

#endif
