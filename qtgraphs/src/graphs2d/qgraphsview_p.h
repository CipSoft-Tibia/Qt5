// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QGRAPHSVIEW_H
#define QGRAPHSVIEW_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtGraphs API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtQuick/QQuickItem>
#include <QList>
#include <QQmlListProperty>
#include <QtGraphs/qabstractseries.h>

#include <QPen>
#include <QBrush>
#include <private/axisrenderer_p.h>
#include <private/barsrenderer_p.h>
#include <private/pointrenderer_p.h>
#include <QtGraphs/qgraphtheme.h>
#include <QtQuick/QSGClipNode>

QT_BEGIN_NAMESPACE

class QAbstractAxis;

class QGraphsView : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QGraphTheme *theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QQmlListProperty<QObject> seriesList READ seriesList CONSTANT)
    // TODO: Remove this?
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(qreal marginTop READ marginTop WRITE setMarginTop NOTIFY marginTopChanged)
    Q_PROPERTY(qreal marginBottom READ marginBottom WRITE setMarginBottom NOTIFY marginBottomChanged)
    Q_PROPERTY(qreal marginLeft READ marginLeft WRITE setMarginLeft NOTIFY marginLeftChanged)
    Q_PROPERTY(qreal marginRight READ marginRight WRITE setMarginRight NOTIFY marginRightChanged)
    Q_CLASSINFO("DefaultProperty", "seriesList")
    QML_NAMED_ELEMENT(GraphsView)

public:
    explicit QGraphsView(QQuickItem *parent = nullptr);
    virtual ~QGraphsView();

    void setBackgroundColor(QColor color);
    QColor backgroundColor();

    Q_INVOKABLE void addSeries(QObject *series);
    Q_INVOKABLE void removeSeries(QObject *series);
    Q_INVOKABLE void insertSeries(int index, QObject *series);
    Q_INVOKABLE bool hasSeries(QObject *series);

    QList<QObject *> getSeriesList() const {
        return m_seriesList;
    }

    QQmlListProperty<QObject> seriesList();
    static void appendSeriesFunc(QQmlListProperty<QObject> *list, QObject *series);
    static qsizetype countSeriesFunc(QQmlListProperty<QObject> *list);
    static QObject *atSeriesFunc(QQmlListProperty<QObject> *list, qsizetype index);
    static void clearSeriesFunc(QQmlListProperty<QObject> *list);

    QGraphTheme *theme() const;
    void setTheme(QGraphTheme *newTheme);

    qreal marginTop() const;
    void setMarginTop(qreal newMarginTop);

    qreal marginBottom() const;
    void setMarginBottom(qreal newMarginBottom);

    qreal marginLeft() const;
    void setMarginLeft(qreal newMarginLeft);

    qreal marginRight() const;
    void setMarginRight(qreal newMarginRight);

    void addAxis(QAbstractAxis *axis);
    void removeAxis(QAbstractAxis *axis);

    // Returns the graph series area.
    // So graphview - margins - axis.
    QRectF seriesRect() const;

    void createBarsRenderer();
    void createAxisRenderer();
    void createPointRenderer();

protected:
    void handleHoverEnter(QString seriesName, QPointF position, QPointF value);
    void handleHoverExit(QString seriesName, QPointF position);
    void handleHover(QString seriesName, QPointF position, QPointF value);
    void updateComponentSizes();
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    QSGNode *updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *updatePaintNodeData) override;
    void updatePolish() override;

Q_SIGNALS:
    void backgroundColorChanged();
    void themeChanged();
    void marginTopChanged();
    void marginBottomChanged();
    void marginLeftChanged();
    void marginRightChanged();
    void hoverEnter(QString seriesName, QPointF position, QPointF value);
    void hoverExit(QString seriesName, QPointF position);
    void hover(QString seriesName, QPointF position, QPointF value);

private:
    friend class AxisRenderer;
    friend class BarsRenderer;
    friend class PointRenderer;

    void polishAndUpdate();

    AxisRenderer *m_axisRenderer = nullptr;
    BarsRenderer *m_barsRenderer = nullptr;
    PointRenderer *m_pointRenderer = nullptr;
    QList<QObject *> m_seriesList;
    QBrush m_backgroundBrush;
    QSGClipNode *m_backgroundNode = nullptr;

    QList<QAbstractAxis *> m_axis;

    QGraphTheme *m_theme = nullptr;
    qreal m_marginTop = 20;
    qreal m_marginBottom = 20;
    qreal m_marginLeft = 20;
    qreal m_marginRight = 20;

    int m_hoverCount = 0;
};

QT_END_NAMESPACE

#endif
