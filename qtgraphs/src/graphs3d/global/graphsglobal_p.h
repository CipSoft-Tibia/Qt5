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

#ifndef GRAPHSGLOBAL_P_H
#define GRAPHSGLOBAL_P_H

#include "qgraphsglobal.h"
#include <QtGui/qvector3d.h>

QT_BEGIN_NAMESPACE

// Constants used in several files
// Distance from camera to origin
static const float cameraDistance = 6.0f;
// Size of font to be used in label texture rendering. Doesn't affect the actual font size.
static const int textureFontSize = 50;
static const QVector3D zeroVector = QVector3D(0.0f, 0.0f, 0.0f);
static const QVector3D upVector = QVector3D(0.0f, 1.0f, 0.0f);
static const float itemAlpha = 0.0f;
static const qreal gradientTextureHeight = 1.;
static const qreal gradientTextureWidth = 8192.;

QT_END_NAMESPACE

#endif
