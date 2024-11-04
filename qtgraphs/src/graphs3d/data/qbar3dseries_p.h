// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtGraphs API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#ifndef QBAR3DSERIES_P_H
#define QBAR3DSERIES_P_H

#include "qabstract3dseries_p.h"
#include "qbar3dseries.h"

QT_BEGIN_NAMESPACE

class QBar3DSeriesPrivate : public QAbstract3DSeriesPrivate
{
    Q_DECLARE_PUBLIC(QBar3DSeries)

public:
    QBar3DSeriesPrivate();
    ~QBar3DSeriesPrivate() override;

    void setDataProxy(QAbstractDataProxy *proxy) override;
    void connectGraphAndProxy(QQuickGraphsItem *newGraph) override;
    void createItemLabel() override;

    void setSelectedBar(const QPoint &position);

    void setRowColors(const QList<QColor> &colors);

private:
    QPoint m_selectedBar;
    QList<QColor> m_rowColors;

    friend class QQuickGraphsBars;
};

QT_END_NAMESPACE

#endif
