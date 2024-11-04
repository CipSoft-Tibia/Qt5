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

#ifndef QVALUEAXIS_P_H
#define QVALUEAXIS_P_H

#include <QtGraphs/qvalueaxis.h>
#include <private/qabstractaxis_p.h>

QT_BEGIN_NAMESPACE

class QValueAxisPrivate : public QAbstractAxisPrivate
{
    Q_OBJECT
public:
    QValueAxisPrivate(QValueAxis *q);
    ~QValueAxisPrivate();

public:
    qreal min() override { return m_min; }
    qreal max() override { return m_max; }
    void setRange(qreal min,qreal max) override;

protected:
    void setMin(const QVariant &min) override;
    void setMax(const QVariant &max) override;
    void setRange(const QVariant &min, const QVariant &max) override;

private:
    qreal m_min;
    qreal m_max;
    int m_minorTickCount;
    QString m_format;
    int m_decimals;
    qreal m_tickAnchor;
    qreal m_tickInterval;
    Q_DECLARE_PUBLIC(QValueAxis)
};

QT_END_NAMESPACE

#endif // QVALUEAXIS_P_H
