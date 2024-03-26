// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSHAPECURVENODE_P_P_H
#define QQUICKSHAPECURVENODE_P_P_H

#include <QtQuick/qsgmaterial.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QQuickShapeCurveNode;
class QQuickShapeCurveMaterial : public QSGMaterial
{
public:
    QQuickShapeCurveMaterial(QQuickShapeCurveNode *node);
    int compare(const QSGMaterial *other) const override;

    QQuickShapeCurveNode *node() const
    {
        return m_node;
    }

private:
    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;

    QQuickShapeCurveNode *m_node;
};

QT_END_NAMESPACE

#endif // QQUICKSHAPECURVENODE_P_P_H
