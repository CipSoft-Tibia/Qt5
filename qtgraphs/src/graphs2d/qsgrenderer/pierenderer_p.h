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

class PieRenderer : public QQuickItem
{
    Q_OBJECT
public:
    PieRenderer(QGraphsView *graph);
    ~PieRenderer() override;

    void handlePolish(QPieSeries *series);
    void afterPolish(QList<QAbstractSeries *> &cleanupSeries);
    void updateSeries(QPieSeries *series);
    void afterUpdate(QList<QAbstractSeries *> &cleanupSeries);
    void markedDeleted(QList<QPieSlice *> deleted);

    void setSize(QSizeF size);

private:
    struct SliceData
    {
        bool initialized;
    };

    QGraphsView *m_graph;
    QQuickShape *m_shape;
    QHash<QPieSlice *, SliceData> m_activeSlices;

    QPainterPath m_painterPath;
    qsizetype m_colorIndex = -1;
};

QT_END_NAMESPACE

#endif // QPIERENDERER_H
