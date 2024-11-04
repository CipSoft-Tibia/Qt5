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

#ifndef QQUICKGRAPHSTEXTUREDATA_P_H
#define QQUICKGRAPHSTEXTUREDATA_P_H
#include <QLinearGradient>
#include <QList>
#include <QtQuick3D/qquick3dtexturedata.h>

class QQuickGraphsTextureData : public QQuick3DTextureData
{
    Q_OBJECT

public:
    QQuickGraphsTextureData();
    ~QQuickGraphsTextureData();

    void createGradient(QLinearGradient gradient);

private:
    QColor linearInterpolate(QColor startColor, QColor endColor, float value);
};

#endif // QQUICKGRAPHSTEXTUREDATA_P_H
