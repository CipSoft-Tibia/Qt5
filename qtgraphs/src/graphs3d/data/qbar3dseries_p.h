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
    static QBar3DSeriesPrivate *get(QBar3DSeries *item) { return item->d_func(); }
    QBar3DSeriesPrivate();
    ~QBar3DSeriesPrivate() override;

    void setDataProxy(QAbstractDataProxy *proxy) override;
    void connectGraphAndProxy(QQuickGraphsItem *newGraph) override;
    void createItemLabel() override;

    void setSelectedBar(QPoint position);

    void setRowColors(const QList<QColor> &colors);

    void setValueColoringEnabled(bool enabled);

    void setDataArray(const QBarDataArray &newDataArray);
    void clearRow(qsizetype rowIndex);
    void clearArray();

    void setRowLabels(const QStringList &labels);
    void setColumnLabels(const QStringList &labels);
    void fixRowLabels(qsizetype startIndex,
                      qsizetype count,
                      const QStringList &newLabels,
                      bool isInsert);

private:
    QBarDataArray m_dataArray;
    QStringList m_rowLabels;
    QStringList m_columnLabels;
    QPoint m_selectedBar;
    QList<QColor> m_rowColors;
    bool m_valueColoring;

    friend class QQuickGraphsBars;
};

QT_END_NAMESPACE

#endif
