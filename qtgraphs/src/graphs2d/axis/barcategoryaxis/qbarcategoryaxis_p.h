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

#ifndef QBARCATEGORYAXIS_P_H
#define QBARCATEGORYAXIS_P_H

#include <QtGraphs/qbarcategoryaxis.h>
#include <private/qabstractaxis_p.h>

QT_BEGIN_NAMESPACE

class QBarCategoryAxisPrivate : public QAbstractAxisPrivate
{
    Q_OBJECT

public:
    QBarCategoryAxisPrivate(QBarCategoryAxis *q);
    ~QBarCategoryAxisPrivate();

public:
    //interface for manipulating range form base class
    void setRange(const QVariant &min, const QVariant &max) override;
    void setMin(const QVariant &min) override;
    void setMax(const QVariant &max) override;

    //interface manipulating range form domain
    qreal min() override { return m_min; }
    qreal max() override { return m_max; }
    void setRange(qreal min,qreal max) override;

private:
    //range handling
    void setRange(const QString &minCategory, const QString &maxCategory);

private:
    QStringList m_categories;
    QString m_minCategory;
    QString m_maxCategory;
    qreal m_min;
    qreal m_max;
    int m_count;

private:
    Q_DECLARE_PUBLIC(QBarCategoryAxis)
};

QT_END_NAMESPACE

#endif // QBARCATEGORYAXIS_P_H
