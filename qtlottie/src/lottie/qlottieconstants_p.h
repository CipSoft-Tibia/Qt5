// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QLOTTIECONSTANTS_P_H
#define QLOTTIECONSTANTS_P_H

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

#include <QtLottie/qtlottieexports.h>

#define LOTTIE_LAYER_PRECOMP_IX     0x10000
#define LOTTIE_LAYER_SOLID_IX       0x10001
#define LOTTIE_LAYER_IMAGE_IX       0x10002
#define LOTTIE_LAYER_NULL_IX        0x10004
#define LOTTIE_LAYER_SHAPE_IX       0x10008
#define LOTTIE_LAYER_TEXT_IX        0x1000f

#define LOTTIE_EFFECT_FILL          0x20000

QT_BEGIN_NAMESPACE

QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(lcLottieQtLottieParser, Q_LOTTIE_EXPORT);
Q_DECLARE_LOGGING_CATEGORY(lcLottieQtLottieUpdate);
Q_DECLARE_LOGGING_CATEGORY(lcLottieQtLottieRender);
Q_DECLARE_LOGGING_CATEGORY(lcLottieQtLottieRenderThread);

class Q_LOTTIE_EXPORT QLottieLiteral : public QObject
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

    explicit QLottieLiteral(QObject *parent = nullptr) : QObject(parent) {}
};

QT_END_NAMESPACE

#endif // QLOTTIECONSTANTS_P_H
