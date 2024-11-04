// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//  W A R N I N G
//  -------------
//
// This file is not part of the Qt Graphs API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#ifndef QXYSERIES_P_H
#define QXYSERIES_P_H

#include <QtGraphs/qxyseries.h>
#include <private/qabstractseries_p.h>

QT_BEGIN_NAMESPACE

class QAbstractAxis;

class QXYSeriesPrivate : public QAbstractSeriesPrivate
{
    Q_OBJECT

public:
    QXYSeriesPrivate(QXYSeries *q);

    void initializeAxes() override;

    void setPointSelected(int index, bool selected, bool &callSignal);
    bool isPointSelected(int index);

    bool isMarkerSizeDefault();
    void setMarkerSize(qreal markerSize);

Q_SIGNALS:
    void seriesUpdated();

protected:
    QList<QPointF> m_points;
    QSet<int> m_selectedPoints;
    QColor m_color = QColorConstants::White;
    QColor m_selectedColor;
    qreal m_markerSize;
    bool m_markerSizeDefault = true;
    QAbstractAxis *m_axisX = nullptr;
    QAbstractAxis *m_axisY = nullptr;
    QQmlComponent *m_marker = nullptr;

private:
    Q_DECLARE_PUBLIC(QXYSeries)
};

QT_END_NAMESPACE

#endif
