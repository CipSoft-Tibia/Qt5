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

#ifndef QBARSET_P_H
#define QBARSET_P_H

#include <QtGraphs/qbarset.h>
#include <QtCore/QMap>
#include <QtGui/QPen>
#include <QtGui/QBrush>
#include <QtGui/QFont>
#include <QSet>

QT_BEGIN_NAMESPACE

class QBarSetPrivate : public QObject
{
    Q_OBJECT

public:
    QBarSetPrivate(const QString label, QBarSet *parent);
    ~QBarSetPrivate();

    void append(QPointF value);
    void append(const QList<QPointF> &values);
    void append(const QList<qreal> &values);

    void insert(const int index, const qreal value);
    void insert(const int index, const QPointF value);
    int remove(const int index, const int count);

    void replace(const int index, const qreal value);

    qreal pos(const int index);
    qreal value(const int index);

    void setVisualsDirty(bool dirty) { m_visualsDirty = dirty; }
    bool visualsDirty() const { return m_visualsDirty; }
    void setLabelsDirty(bool dirty) { m_labelsDirty = dirty; }
    bool labelsDirty() const { return m_labelsDirty; }

    void setBarSelected(int index, bool selected, bool &callSignal);
    bool isBarSelected(int index) const;

Q_SIGNALS:
    void updatedBars();
    void valueChanged(int index);
    void valueAdded(int index, int count);
    void valueRemoved(int index, int count);

public:
    QBarSet * const q_ptr;
    QString m_label;
    QList<QPointF> m_values;
    QSet<int> m_selectedBars;
    // By default colors are transparent, meaning that use the ones from theme
    QColor m_color = QColor(Qt::transparent);
    QColor m_borderColor = QColor(Qt::transparent);
    QColor m_labelColor = QColor(Qt::transparent);
    QColor m_selectedColor = QColor(Qt::transparent);
    // By default border width is -1, meaning that use the one from theme
    qreal m_borderWidth = -1;
    bool m_visualsDirty;
    bool m_labelsDirty;

    friend class QBarSet;
};

QT_END_NAMESPACE

#endif // QBARSETPRIVATE_P_H
