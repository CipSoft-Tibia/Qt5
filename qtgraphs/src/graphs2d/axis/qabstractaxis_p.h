// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//  W A R N I N G
//  -------------
//
// This file is not part of the QtGraphs API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#ifndef QABSTRACTAXIS_P_H
#define QABSTRACTAXIS_P_H

#include <QColor>
#include <QtCore/QDebug>
#include <QtGraphs/qabstractaxis.h>
#include <private/qgraphsview_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QAbstractAxisPrivate : public QObject
{
    Q_OBJECT
public:
    QAbstractAxisPrivate(QAbstractAxis *q);
    ~QAbstractAxisPrivate();

public:
    Qt::Alignment alignment() const { return m_alignment; }
    Qt::Orientation orientation() const { return m_orientation; }
    void setAlignment(Qt::Alignment alignment);

    //interface for manipulating range form base class
    virtual void setMin(const QVariant &min) = 0;
    virtual void setMax(const QVariant &max) = 0;
    virtual void setRange(const QVariant &min, const QVariant &max) = 0;

    //interface manipulating range form domain
    virtual void setRange(qreal min, qreal max) = 0;
    virtual qreal min() = 0;
    virtual qreal max() = 0;

public Q_SLOTS:
    void handleRangeChanged(qreal min, qreal max);

Q_SIGNALS:
    void rangeChanged(qreal min, qreal max);

protected:
    QAbstractAxis *q_ptr;
    // TODO: Used?
    QGraphsView *m_graph = nullptr;

private:
    Qt::Alignment m_alignment;
    Qt::Orientation m_orientation = Qt::Orientation(0);

    bool m_visible = true;

    bool m_lineVisible = true;

    bool m_gridLineVisible = true;
    bool m_minorGridLineVisible = true;

    bool m_labelsVisible = true;
    qreal m_labelsAngle = 0;

    bool m_titleVisible = true;
    QColor m_titleColor;
    QFont m_titleFont;
    QString m_title;

    Q_DECLARE_PUBLIC(QAbstractAxis)
    friend class QAbstractAxis;
};

QT_END_NAMESPACE

#endif
