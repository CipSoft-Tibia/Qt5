// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKNODEINFO_P_H
#define QQUICKNODEINFO_P_H

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

#include <QString>
#include <QPainter>
#include <QPainterPath>
#include <QMatrix4x4>
#include <QQuickItem>
#include <QtGui/private/qfixed_p.h>

QT_BEGIN_NAMESPACE

struct NodeInfo
{
    QString nodeId;
    QString typeName;
    QTransform transform;
    qreal opacity = 1.0;
    bool isDefaultTransform = true;
    bool isDefaultOpacity = true;
    bool isVisible = true;
    bool isDisplayed = true; // TODO: Map to display enum in QtSvg

    struct AnimateColor {
        int start = 0;
        int repeatCount = 0;
        bool fill = false;
        bool freeze = false;
        QList<QPair<qreal, QColor> > keyFrames;
    };
    QList<AnimateColor> animateColors;

    struct TransformAnimation {
        struct TransformKeyFrame {
            TransformKeyFrame() = default;

            QTransform baseMatrix;
            QList<qreal> values; // animationTypes.size() * 3, content depends on each type
            bool indefiniteAnimation = false;
        };

        QList<QTransform::TransformationType> animationTypes;
        QMap<QFixed, TransformKeyFrame> keyFrames;
    };

    TransformAnimation transformAnimation;
};

struct ImageNodeInfo : NodeInfo
{
    QImage image;
    QRectF rect;
    QString externalFileReference;
};

struct StrokeStyle
{
    Qt::PenCapStyle lineCapStyle = Qt::SquareCap;
    Qt::PenJoinStyle lineJoinStyle = Qt::MiterJoin;
    int miterLimit = 4;
    qreal dashOffset = 0;
    QList<qreal> dashArray;
    QColor color = QColorConstants::Transparent;
    qreal width = 1.0;

    static StrokeStyle fromPen(const QPen &p)
    {
        StrokeStyle style;
        style.lineCapStyle = p.capStyle();
        style.lineJoinStyle = p.joinStyle() == Qt::SvgMiterJoin ? Qt::MiterJoin : p.joinStyle(); //TODO support SvgMiterJoin
        style.miterLimit = qRound(p.miterLimit());
        style.dashOffset = p.dashOffset();
        style.dashArray = p.dashPattern();
        style.width = p.widthF();

        return style;
    }
};

struct PathNodeInfo : NodeInfo
{
    QPainterPath painterPath;
    Qt::FillRule fillRule = Qt::FillRule::WindingFill;
    QColor fillColor;
    StrokeStyle strokeStyle;
    QGradient grad;
    QTransform fillTransform;
};

struct TextNodeInfo : NodeInfo
{
    bool isTextArea;
    bool needsRichText;
    QPointF position;
    QSizeF size;
    QString text;
    QFont font;
    Qt::Alignment alignment;
    QColor fillColor;
    QColor strokeColor;
};

struct AnimateColorNodeInfo : NodeInfo
{
};

enum class StructureNodeStage
{
    Start,
    End
};

struct UseNodeInfo : NodeInfo
{
    QPointF startPos;
    StructureNodeStage stage;
};

struct StructureNodeInfo : NodeInfo
{
    StructureNodeStage stage = StructureNodeStage::Start;
    bool forceSeparatePaths = false;
    QRectF viewBox;
    QSize size;
    bool isPathContainer = false;
};


QT_END_NAMESPACE

#endif //QQUICKNODEINFO_P_H
