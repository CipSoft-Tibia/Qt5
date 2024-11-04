// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QXYSERIES_H
#define QXYSERIES_H

#if 0
#  pragma qt_class(QXYSeries)
#endif

#include <QtGraphs/qabstractseries.h>
#include <QtGraphs/qgraphsglobal.h>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE

class QXYSeriesPrivate;
class QXYModelMapper;

class QT_TECH_PREVIEW_API Q_GRAPHS_EXPORT QXYSeries : public QAbstractSeries
{
    Q_OBJECT
    Q_PROPERTY(QAbstractAxis *axisX READ axisX WRITE setAxisX NOTIFY axisXChanged)
    Q_PROPERTY(QAbstractAxis *axisY READ axisY WRITE setAxisY NOTIFY axisYChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QColor selectedColor READ selectedColor WRITE setSelectedColor NOTIFY selectedColorChanged)
    Q_PROPERTY(qreal markerSize READ markerSize WRITE setMarkerSize NOTIFY markerSizeChanged)
    Q_PROPERTY(QQmlComponent *pointMarker READ pointMarker WRITE setPointMarker NOTIFY pointMarkerChanged FINAL)

protected:
    explicit QXYSeries(QXYSeriesPrivate &d, QObject *parent = nullptr);

public:
    ~QXYSeries();
    // TODO: Consider making these slots, available from QML.
    void append(qreal x, qreal y);
    void append(const QPointF &point);
    void append(const QList<QPointF> &points);
    void replace(qreal oldX, qreal oldY, qreal newX, qreal newY);
    void replace(const QPointF &oldPoint, const QPointF &newPoint);
    void replace(int index, qreal newX, qreal newY);
    void replace(int index, const QPointF &newPoint);
    void replace(const QList<QPointF> &points);
    void remove(qreal x, qreal y);
    void remove(const QPointF &point);
    void remove(int index);
    void removePoints(int index, int count);
    void insert(int index, const QPointF &point);
    void clear();

    int count() const;
    QList<QPointF> points() const;
    const QPointF &at(int index) const;

    QXYSeries &operator<<(const QPointF &point);
    QXYSeries &operator<<(const QList<QPointF> &points);

    virtual void setColor(const QColor &newColor);
    virtual QColor color() const;

    void setSelectedColor(const QColor &color);
    QColor selectedColor() const;

    bool isPointSelected(int index);
    void selectPoint(int index);
    void deselectPoint(int index);
    void setPointSelected(int index, bool selected);
    void selectAllPoints();
    void deselectAllPoints();
    void selectPoints(const QList<int> &indexes);
    void deselectPoints(const QList<int> &indexes);
    void toggleSelection(const QList<int> &indexes);
    QList<int> selectedPoints() const;

    void setMarkerSize(qreal size);
    qreal markerSize() const;

    QAbstractAxis *axisX() const;
    void setAxisX(QAbstractAxis *axis);

    QAbstractAxis *axisY() const;
    void setAxisY(QAbstractAxis *axis);

    QQmlComponent *pointMarker() const;
    void setPointMarker(QQmlComponent *newPointMarker);

Q_SIGNALS:
    void pointReplaced(int index);
    void pointRemoved(int index);
    void pointAdded(int index);
    void colorChanged(QColor color);
    void selectedColorChanged(const QColor &color);
    void pointsReplaced();
    void pointsRemoved(int index, int count);
    void selectedPointsChanged();
    void markerSizeChanged(qreal size);
    void axisXChanged();
    void axisYChanged();
    void pointMarkerChanged();

private:
    Q_DECLARE_PRIVATE(QXYSeries)
    Q_DISABLE_COPY(QXYSeries)
};

QT_END_NAMESPACE

#endif // QXYSERIES_H
