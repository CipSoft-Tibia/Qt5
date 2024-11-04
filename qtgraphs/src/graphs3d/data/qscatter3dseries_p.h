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

#ifndef QSCATTER3DSERIES_P_H
#define QSCATTER3DSERIES_P_H

#include "qabstract3dseries_p.h"
#include "qscatter3dseries.h"

QT_BEGIN_NAMESPACE

class QScatter3DSeriesPrivate : public QAbstract3DSeriesPrivate
{
    Q_DECLARE_PUBLIC(QScatter3DSeries)

public:
    QScatter3DSeriesPrivate();
    ~QScatter3DSeriesPrivate() override;

    void setDataProxy(QAbstractDataProxy *proxy) override;
    void connectGraphAndProxy(QQuickGraphsItem *newGraph) override;
    void createItemLabel() override;

    void setSelectedItem(int index);
    void setItemSize(float size);

private:
    int m_selectedItem;
    float m_itemSize;

    friend class QQuickGraphsScatter;
};

QT_END_NAMESPACE

#endif
