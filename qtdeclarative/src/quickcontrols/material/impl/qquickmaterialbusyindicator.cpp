// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmaterialbusyindicator_p.h"

#include <QtCore/qmath.h>
#include <QtCore/qeasingcurve.h>
#include <QtGui/qpainter.h>
#include <QtQuick/qsgimagenode.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuickControls2Impl/private/qquickanimatednode_p.h>

QT_BEGIN_NAMESPACE

/*
    Relevant Android code:

    - core/res/res/anim/progress_indeterminate_rotation_material.xml contains
      the rotation animation data.
    - core/res/res/anim/progress_indeterminate_material.xml contains the trim
      animation data.
    - core/res/res/interpolator/trim_start_interpolator.xml and
      core/res/res/interpolator/trim_end_interpolator.xml contain the start
      and end trim path interpolators.
    - addCommand() in core/java/android/util/PathParser.java has a list of the
      different path commands available.
*/

static const int SpanAnimationDuration = 700;
static const int RotationAnimationDuration = SpanAnimationDuration * 6;
static const int TargetRotation = 720;
static const int OneDegree = 16;
static const qreal MinSweepSpan = 10 * OneDegree;
static const qreal MaxSweepSpan = 300 * OneDegree;

class QQuickMaterialBusyIndicatorNode : public QQuickAnimatedNode
{
public:
    QQuickMaterialBusyIndicatorNode(QQuickMaterialBusyIndicator *item);

    void sync(QQuickItem *item) override;

protected:
    void updateCurrentTime(int time) override;

private:
    int m_lastStartAngle = 0;
    int m_lastEndAngle = 0;
    qreal m_width = 0;
    qreal m_height = 0;
    qreal m_devicePixelRatio = 1;
    QColor m_color;
};

QQuickMaterialBusyIndicatorNode::QQuickMaterialBusyIndicatorNode(QQuickMaterialBusyIndicator *item)
    : QQuickAnimatedNode(item)
{
    setLoopCount(Infinite);
    setCurrentTime(item->elapsed());
    setDuration(RotationAnimationDuration);

    QSGImageNode *textureNode = item->window()->createImageNode();
    textureNode->setOwnsTexture(true);
    appendChildNode(textureNode);

    // A texture seems to be required here, but we don't have one yet, as we haven't drawn anything,
    // so just use a blank image.
    QImage blankImage(item->width(), item->height(), QImage::Format_ARGB32_Premultiplied);
    blankImage.fill(Qt::transparent);
    textureNode->setTexture(item->window()->createTextureFromImage(blankImage));
}

void QQuickMaterialBusyIndicatorNode::updateCurrentTime(int time)
{
    const qreal w = m_width;
    const qreal h = m_height;
    const qreal size = qMin(w, h);
    const qreal dx = (w - size) / 2;
    const qreal dy = (h - size) / 2;

    QImage image(size * m_devicePixelRatio, size * m_devicePixelRatio, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen;
    QSGImageNode *textureNode = static_cast<QSGImageNode *>(firstChild());
    pen.setColor(m_color);
    pen.setWidth(qCeil(size / 12) * m_devicePixelRatio);
    painter.setPen(pen);

    const qreal percentageComplete = time / qreal(RotationAnimationDuration);
    const qreal spanPercentageComplete = (time % SpanAnimationDuration) / qreal(SpanAnimationDuration);
    const int iteration = time / SpanAnimationDuration;
    int startAngle = 0;
    int endAngle = 0;

    if (iteration % 2 == 0) {
        if (m_lastStartAngle > 360 * OneDegree)
            m_lastStartAngle -= 360 * OneDegree;

        // The start angle is only affected by the rotation animation for the "grow" phase.
        startAngle = m_lastStartAngle;
        QEasingCurve angleCurve(QEasingCurve::OutQuad);
        const qreal percentage = angleCurve.valueForProgress(spanPercentageComplete);
        endAngle = m_lastStartAngle + MinSweepSpan + percentage * (MaxSweepSpan - MinSweepSpan);
        m_lastEndAngle = endAngle;
    } else {
        // Both the start angle *and* the span are affected by the "shrink" phase.
        QEasingCurve angleCurve(QEasingCurve::InQuad);
        const qreal percentage = angleCurve.valueForProgress(spanPercentageComplete);
        startAngle = m_lastEndAngle - MaxSweepSpan + percentage * (MaxSweepSpan - MinSweepSpan);
        endAngle = m_lastEndAngle;
        m_lastStartAngle = startAngle;
    }

    const int halfPen = pen.width() / 2;
    const QRectF arcBounds = QRectF(halfPen, halfPen,
                                    m_devicePixelRatio * size - pen.width(),
                                    m_devicePixelRatio * size - pen.width());
    // The current angle of the rotation animation.
    const qreal rotation = OneDegree * percentageComplete * -TargetRotation;
    startAngle -= rotation;
    endAngle -= rotation;
    const int angleSpan = endAngle - startAngle;
    painter.drawArc(arcBounds, -startAngle, -angleSpan);
    painter.end();

    textureNode->setRect(QRectF(dx, dy, size, size));
    textureNode->setTexture(window()->createTextureFromImage(image));
}

void QQuickMaterialBusyIndicatorNode::sync(QQuickItem *item)
{
    QQuickMaterialBusyIndicator *indicator = static_cast<QQuickMaterialBusyIndicator *>(item);
    m_color = indicator->color();
    m_width = indicator->width();
    m_height = indicator->height();
    m_devicePixelRatio = indicator->window()->effectiveDevicePixelRatio();
}

QQuickMaterialBusyIndicator::QQuickMaterialBusyIndicator(QQuickItem *parent) :
    QQuickItem(parent)
{
    setFlag(ItemHasContents);
}

QColor QQuickMaterialBusyIndicator::color() const
{
    return m_color;
}

void QQuickMaterialBusyIndicator::setColor(const QColor &color)
{
    if (m_color == color)
        return;

    m_color = color;
    update();
}

bool QQuickMaterialBusyIndicator::isRunning() const
{
    return isVisible();
}

void QQuickMaterialBusyIndicator::setRunning(bool running)
{
    if (running)
        setVisible(true);
}

int QQuickMaterialBusyIndicator::elapsed() const
{
    return m_elapsed;
}

void QQuickMaterialBusyIndicator::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data)
{
    QQuickItem::itemChange(change, data);
    switch (change) {
    case ItemOpacityHasChanged:
        if (qFuzzyIsNull(data.realValue))
            setVisible(false);
        break;
    case ItemVisibleHasChanged:
        update();
        break;
    default:
        break;
    }
}

QSGNode *QQuickMaterialBusyIndicator::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QQuickMaterialBusyIndicatorNode *node = static_cast<QQuickMaterialBusyIndicatorNode *>(oldNode);
    if (isRunning() && width() > 0 && height() > 0) {
        if (!node) {
            node = new QQuickMaterialBusyIndicatorNode(this);
            node->start();
        }
        node->sync(this);
    } else {
        m_elapsed = node ? node->currentTime() : 0;
        delete node;
        node = nullptr;
    }
    return node;
}

QT_END_NAMESPACE

#include "moc_qquickmaterialbusyindicator_p.cpp"
