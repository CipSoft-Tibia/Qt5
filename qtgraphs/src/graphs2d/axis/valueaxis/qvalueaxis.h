// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QVALUEAXIS_H
#define QVALUEAXIS_H

#if 0
#  pragma qt_class(QValueAxis)
#endif

#include <QtGraphs/qabstractaxis.h>
#include <QtGraphs/qgraphsglobal.h>
#include <QtQml/QQmlEngine>

QT_BEGIN_NAMESPACE

class QValueAxisPrivate;

class QT_TECH_PREVIEW_API Q_GRAPHS_EXPORT QValueAxis : public QAbstractAxis
{
    Q_OBJECT
    Q_PROPERTY(qreal min READ min WRITE setMin NOTIFY minChanged)
    Q_PROPERTY(qreal max READ max WRITE setMax NOTIFY maxChanged)
    Q_PROPERTY(QString labelFormat READ labelFormat WRITE setLabelFormat NOTIFY labelFormatChanged)
    Q_PROPERTY(int labelDecimals READ labelDecimals WRITE setLabelDecimals NOTIFY labelDecimalsChanged)
    Q_PROPERTY(int minorTickCount READ minorTickCount WRITE setMinorTickCount NOTIFY minorTickCountChanged)
    Q_PROPERTY(qreal tickAnchor READ tickAnchor WRITE setTickAnchor NOTIFY tickAnchorChanged)
    Q_PROPERTY(qreal tickInterval READ tickInterval WRITE setTickInterval NOTIFY tickIntervalChanged)
    QML_NAMED_ELEMENT(ValueAxis)

public:
    explicit QValueAxis(QObject *parent = nullptr);
    ~QValueAxis();

protected:
    QValueAxis(QValueAxisPrivate &d, QObject *parent = nullptr);

public:
    AxisType type() const override;

    //range handling
    void setMin(qreal min);
    qreal min() const;
    void setMax(qreal max);
    qreal max() const;
    void setRange(qreal min, qreal max);

    //ticks handling
    void setMinorTickCount(int count);
    int minorTickCount() const;
    void setTickAnchor(qreal anchor);
    qreal tickAnchor() const;
    void setTickInterval(qreal insterval);
    qreal tickInterval() const;

    //label formatting
    void setLabelFormat(const QString &format);
    QString labelFormat() const;
    void setLabelDecimals(int decimals);
    int labelDecimals() const;

Q_SIGNALS:
    void minChanged(qreal min);
    void maxChanged(qreal max);
    void rangeChanged(qreal min, qreal max);
    void minorTickCountChanged(int tickCount);
    void labelFormatChanged(const QString &format);
    void labelDecimalsChanged(int decimals);
    void tickAnchorChanged(qreal tickAnchor);
    void tickIntervalChanged(qreal tickInterval);

private:
    Q_DECLARE_PRIVATE(QValueAxis)
    Q_DISABLE_COPY(QValueAxis)
};

QT_END_NAMESPACE

#endif // QVALUEAXIS_H
