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

#ifndef QBARDATAPROXY_P_H
#define QBARDATAPROXY_P_H

#include "qbardataproxy.h"
#include "qabstractdataproxy_p.h"

QT_BEGIN_NAMESPACE

class QBarDataProxyPrivate : public QAbstractDataProxyPrivate
{
    Q_DECLARE_PUBLIC(QBarDataProxy)

public:
    QBarDataProxyPrivate(QBarDataProxy *q);
    virtual ~QBarDataProxyPrivate();

    void resetArray(QBarDataArray *newArray, const QStringList *rowLabels,
                    const QStringList *columnLabels);
    void setRow(int rowIndex, QBarDataRow *row, const QString *label);
    void setRows(int rowIndex, const QBarDataArray &rows, const QStringList *labels);
    void setItem(int rowIndex, int columnIndex, const QBarDataItem &item);
    int addRow(QBarDataRow *row, const QString *label);
    int addRows(const QBarDataArray &rows, const QStringList *labels);
    void insertRow(int rowIndex, QBarDataRow *row, const QString *label);
    void insertRows(int rowIndex, const QBarDataArray &rows, const QStringList *labels);
    void removeRows(int rowIndex, int removeCount, bool removeLabels);

    QPair<float, float> limitValues(int startRow, int startColumn, int rowCount,
                                    int columnCount) const;

    void setSeries(QAbstract3DSeries *series) override;

private:
    void clearRow(int rowIndex);
    void clearArray();
    void fixRowLabels(int startIndex, int count, const QStringList &newLabels, bool isInsert);

    QBarDataArray *m_dataArray;
    QStringList m_rowLabels;
    QStringList m_columnLabels;
};

QT_END_NAMESPACE

#endif
