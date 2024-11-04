// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QABSTRACTSERIES_H
#define QABSTRACTSERIES_H

#if 0
#  pragma qt_class(QAbstractSeries)
#endif

#include <QtCore/QObject>
#include <QtGraphs/qabstractaxis.h>
#include <QtGraphs/qgraphsglobal.h>
#include <QtGraphs/qseriestheme.h>
#include <QtGui/QPen>
#include <QtQml/QQmlListProperty>
#include <QtQml/QQmlParserStatus>

QT_BEGIN_NAMESPACE

class QAbstractSeriesPrivate;
class QGraphsView;

class QT_TECH_PREVIEW_API Q_GRAPHS_EXPORT QAbstractSeries : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QSeriesTheme *theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(bool selectable READ selectable WRITE setSelectable NOTIFY selectableChanged)
    Q_PROPERTY(bool hoverable READ hoverable WRITE setHoverable NOTIFY hoverableChanged)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)
    Q_PROPERTY(qreal valuesMultiplier READ valuesMultiplier WRITE setValuesMultiplier NOTIFY valuesMultiplierChanged)
    Q_PROPERTY(SeriesType type READ type CONSTANT)
    Q_PROPERTY(QQmlListProperty<QObject> seriesChildren READ seriesChildren CONSTANT)
    Q_CLASSINFO("DefaultProperty", "seriesChildren")

public:
    enum SeriesType {
        SeriesTypeLine,
        //SeriesTypeArea,
        SeriesTypeBar,
        //SeriesTypeStackedBar,
        //SeriesTypePercentBar,
        //SeriesTypePie,
        SeriesTypeScatter,
        //SeriesTypeSpline,
        //SeriesTypeHorizontalBar,
        //SeriesTypeHorizontalStackedBar,
        //SeriesTypeHorizontalPercentBar,
        //SeriesTypeBoxPlot,
    };
    Q_ENUM(SeriesType)

protected:
    QAbstractSeries(QAbstractSeriesPrivate &d, QObject *parent = nullptr);

    // from QDeclarativeParserStatus
    void classBegin() override;
    void componentComplete() override;

public:
    ~QAbstractSeries();
    virtual SeriesType type() const = 0;

    QSeriesTheme *theme() const;
    void setTheme(QSeriesTheme *newTheme);

    QString name() const;
    void setName(const QString &name);

    bool isVisible() const;
    void setVisible(bool visible = true);

    bool selectable() const;
    void setSelectable(bool selectable);

    qreal opacity() const;
    void setOpacity(qreal opacity);

    qreal valuesMultiplier() const;
    void setValuesMultiplier(qreal valuesMultiplier);

    QGraphsView *graph() const;
    void setGraph(QGraphsView *graph);

    bool attachAxis(QAbstractAxis *axis);
    bool detachAxis(QAbstractAxis *axis);
    QList<QAbstractAxis*> attachedAxes();

    void show();
    void hide();

    QQmlListProperty<QObject> seriesChildren();

    bool hoverable() const;
    void setHoverable(bool newHoverable);

public Q_SLOTS:
    static void appendSeriesChildren(QQmlListProperty<QObject> *list, QObject *element);

Q_SIGNALS:
    void update();
    void themeChanged();
    void nameChanged();
    void visibleChanged();
    void selectableChanged();
    void hoverableChanged();
    void opacityChanged();
    void valuesMultiplierChanged();
    void hoverEnter(QString seriesName, QPointF position, QPointF value);
    void hoverExit(QString seriesName, QPointF position);
    void hover(QString seriesName, QPointF position, QPointF value);

protected:
    QScopedPointer<QAbstractSeriesPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QABSTRACTSERIES_H
