// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QABSTRACTSERIES_P_H
#define QABSTRACTSERIES_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtGraphs API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtGraphs/qabstractseries.h>
#include <memory>

QT_BEGIN_NAMESPACE

class QAbstractAxis;
class QGraphsView;

class QAbstractSeriesPrivate : public QObject
{
    Q_OBJECT
public:
    QAbstractSeriesPrivate(QAbstractSeries *q);
    ~QAbstractSeriesPrivate();

    virtual void initializeAxes() = 0;

Q_SIGNALS:
    void countChanged();

protected:
    QAbstractSeries *q_ptr;
    QGraphsView *m_graph;
    QList<QAbstractAxis*> m_axes;

private:
    QSeriesTheme *m_theme = nullptr;
    QString m_name;
    bool m_visible;
    bool m_selectable = false;
    bool m_hoverable = false;
    qreal m_opacity;
    qreal m_valuesMultiplier;

    friend class QAbstractSeries;
};

QT_END_NAMESPACE

#endif
