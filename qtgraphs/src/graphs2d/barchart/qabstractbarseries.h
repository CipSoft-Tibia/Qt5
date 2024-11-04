// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QABSTRACTBARSERIES_H
#define QABSTRACTBARSERIES_H

#if 0
#  pragma qt_class(QAbstractBarSeries)
#endif

#include <QtGraphs/qabstractseries.h>
#include <QtCore/QStringList>
#include <QtQml/QQmlEngine>
#include <QtGraphs/qgraphsglobal.h>

QT_BEGIN_NAMESPACE

class QBarSet;
class QAbstractBarSeriesPrivate;

// Container for series
class QT_TECH_PREVIEW_API Q_GRAPHS_EXPORT QAbstractBarSeries : public QAbstractSeries
{
    Q_OBJECT
    Q_PROPERTY(qreal barWidth READ barWidth WRITE setBarWidth NOTIFY barWidthChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool labelsVisible READ isLabelsVisible WRITE setLabelsVisible NOTIFY labelsVisibleChanged)
    Q_PROPERTY(QString labelsFormat READ labelsFormat WRITE setLabelsFormat NOTIFY labelsFormatChanged)
    Q_PROPERTY(LabelsPosition labelsPosition READ labelsPosition WRITE setLabelsPosition NOTIFY labelsPositionChanged)
    Q_PROPERTY(qreal labelsAngle READ labelsAngle WRITE setLabelsAngle NOTIFY labelsAngleChanged)
    Q_PROPERTY(int labelsPrecision READ labelsPrecision WRITE setLabelsPrecision NOTIFY labelsPrecisionChanged)
public:
    enum LabelsPosition {
        LabelsCenter = 0,
        LabelsInsideEnd,
        LabelsInsideBase,
        LabelsOutsideEnd
    };
    Q_ENUM(LabelsPosition)

public:
    virtual ~QAbstractBarSeries();

    void setBarWidth(qreal width);
    qreal barWidth() const;

    // TODO: Consider making these slots, available from QML.
    bool append(QBarSet *set);
    bool remove(QBarSet *set);
    bool take(QBarSet *set);
    bool append(const QList<QBarSet *> &sets);
    bool insert(int index, QBarSet *set);
    int count() const;
    QList<QBarSet *> barSets() const;
    void clear();

    void setLabelsVisible(bool visible = true);
    bool isLabelsVisible() const;

    void setLabelsFormat(const QString &format);
    QString labelsFormat() const;

    void setLabelsAngle(qreal angle);
    qreal labelsAngle() const;

    void setLabelsPosition(QAbstractBarSeries::LabelsPosition position);
    QAbstractBarSeries::LabelsPosition labelsPosition() const;

    void setLabelsPrecision(int precision);
    int labelsPrecision() const;

public Q_SLOTS:
    void selectAll();
    void deselectAll();

protected:
    explicit QAbstractBarSeries(QAbstractBarSeriesPrivate &d, QObject *parent = nullptr);

    void componentComplete() override;

Q_SIGNALS:
    void clicked(int index, QBarSet *barset);
    void hovered(bool status, int index, QBarSet *barset);
    void pressed(int index, QBarSet *barset);
    void released(int index, QBarSet *barset);
    void doubleClicked(int index, QBarSet *barset);
    void countChanged();
    void barWidthChanged();
    void labelsVisibleChanged();
    void labelsFormatChanged(const QString &format);
    void labelsPositionChanged(QAbstractBarSeries::LabelsPosition position);
    void labelsAngleChanged(qreal angle);
    void labelsPrecisionChanged(int precision);

    void barsetsAdded(const QList<QBarSet *> &sets);
    void barsetsRemoved(const QList<QBarSet *> &sets);

protected:
    Q_DECLARE_PRIVATE(QAbstractBarSeries)
    friend class BarSet;
};

QT_END_NAMESPACE

#endif // QABSTRACTBARSERIES_H
