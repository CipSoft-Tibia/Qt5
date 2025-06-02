// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//  W A R N I N G
//  -------------
//
// This file is not part of the Qt Graphs API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#ifndef QSPLINESERIES_P_H
#define QSPLINESERIES_P_H

#include <QtGraphs/QSplineSeries>
#include <private/qsplinecontrolanimation_p.h>
#include <private/qxyseries_p.h>

QT_BEGIN_NAMESPACE

class QSplineSeriesPrivate : public QXYSeriesPrivate
{
public:
    QSplineSeriesPrivate();

protected:
    qreal m_width;
    Qt::PenCapStyle m_capStyle;
    QList<QPointF> m_controlPoints;

    void calculateSplinePoints();
    QList<qreal> calculateControlPoints(const QList<qreal> &list);

private:
    Q_DECLARE_PUBLIC(QSplineSeries)

    friend class QSplineControlAnimation;
    friend class QGraphTransition;
};

QT_END_NAMESPACE

#endif // QSPLINESERIES_H_H
