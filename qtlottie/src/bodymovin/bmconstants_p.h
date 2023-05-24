// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BMCONSTANTS_P_H
#define BMCONSTANTS_P_H

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

#include <QObject>
#include <QLoggingCategory>

#include <QtBodymovin/bmglobal.h>
#include <QtCore/private/qglobal_p.h>

#define BM_LAYER_PRECOMP_IX     0x10000
#define BM_LAYER_SOLID_IX       0x10001
#define BM_LAYER_IMAGE_IX       0x10002
#define BM_LAYER_NULL_IX        0x10004
#define BM_LAYER_SHAPE_IX       0x10008
#define BM_LAYER_TEXT_IX        0x1000f

#define BM_EFFECT_FILL          0x20000

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcLottieQtBodymovinParser);
Q_DECLARE_LOGGING_CATEGORY(lcLottieQtBodymovinUpdate);
Q_DECLARE_LOGGING_CATEGORY(lcLottieQtBodymovinRender);
Q_DECLARE_LOGGING_CATEGORY(lcLottieQtBodymovinRenderThread);

class BODYMOVIN_EXPORT BMLiteral : public QObject
{
    Q_OBJECT
public:
    enum ElementType {
        Animation = 0,
        LayerImage,
        LayerNull,
        LayerPrecomp,
        LayerShape
    };

    enum PropertyType {
        RectPosition,
        RectSize,
        RectRoundness
    };
    Q_ENUM(PropertyType)

    explicit BMLiteral(QObject *parent = nullptr) : QObject(parent) {}
};

QT_END_NAMESPACE

#endif // BMCONSTANTS_P_H
