// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef PIERENDERER_H
#define PIERENDERER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtGraphs API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QQuickItem>
#include <QtGui/qpainterpath.h>

QT_BEGIN_NAMESPACE

class QGraphsView;
class QPieSeries;
class QPieSlice;
class QQuickShape;
class QAbstractSeries;
class QQuickTapHandler;

class PieRenderer : public QQuickItem
{
    Q_OBJECT
public:
    PieRenderer(QGraphsView *graph, bool clipPlotArea);
    ~PieRenderer() override;

    void handlePolish(QPieSeries *series);
    void afterPolish(QList<QAbstractSeries *> &cleanupSeries);
    void updateSeries(QPieSeries *series);
    void afterUpdate(QList<QAbstractSeries *> &cleanupSeries);
    void markedDeleted(QList<QPieSlice *> deleted);

    void setSize(QSizeF size);

    bool handleHoverMove(QHoverEvent *event);

private:
    struct SliceData
    {
        bool initialized;
    };

    void onSingleTapped(QEventPoint eventPoint, Qt::MouseButton button);
    void onDoubleTapped(QEventPoint eventPoint, Qt::MouseButton button);
    void onPressedChanged();

    bool isPointInSlice(QPointF point, QPieSlice *slice, qreal *angle = nullptr);

    QGraphsView *m_graph = nullptr;
    QQuickShape *m_shape = nullptr;
    QHash<QPieSlice *, SliceData> m_activeSlices;

    QQuickTapHandler *m_tapHandler = nullptr;
    QPieSlice *m_currentHoverSlice = nullptr;

    QPainterPath m_painterPath;
    qsizetype m_colorIndex = -1;
};

QT_END_NAMESPACE

#endif // QPIERENDERER_H
