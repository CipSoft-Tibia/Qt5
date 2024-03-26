// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCharts/QPolarChart>
#include <QtWidgets/QMainWindow>
#include <QtGui/QFont>
#include <QtCharts/QChart>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QAreaSeries>

QT_BEGIN_NAMESPACE
class QBrush;
class QPen;

namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE


QT_USE_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void angularTicksChanged(int value);
    void radialTicksChanged(int value);
    void angularMinorTicksChanged(int value);
    void radialMinorTicksChanged(int value);
    void anglesChanged(int value);
    void angularMinChanged(double value);
    void angularMaxChanged(double value);
    void radialMinChanged(double value);
    void radialMaxChanged(double value);
    void angularShadesIndexChanged(int index);
    void radialShadesIndexChanged(int index);
    void labelFormatEdited(const QString &text);
    void labelFontChanged(const QFont &font);
    void labelFontSizeChanged(int value);
    void animationIndexChanged(int index);
    void labelsIndexChanged(int index);
    void titleIndexChanged(int index);
    void titleFontChanged(const QFont &font);
    void titleFontSizeChanged(int value);
    void gridIndexChanged(int index);
    void minorGridIndexChanged(int index);
    void gridLineColorIndexChanged(int index);
    void arrowIndexChanged(int index);
    void angularRangeChanged(qreal min, qreal max);
    void radialRangeChanged(qreal min, qreal max);
    void angularAxisIndexChanged(int index);
    void radialAxisIndexChanged(int index);
    void logBaseChanged(double value);
    void niceNumbersChecked();
    void dateFormatEdited(const QString &text);
    void moreCategoriesChecked();
    void categoryLabelLocationChecked();
    void series1CheckBoxChecked();
    void series2CheckBoxChecked();
    void series3CheckBoxChecked();
    void series4CheckBoxChecked();
    void series5CheckBoxChecked();
    void series6CheckBoxChecked();
    void series7CheckBoxChecked();
    void themeIndexChanged(int index);
    void seriesHovered(QPointF point, bool state);
    void seriesClicked(const QPointF &point);
    void backgroundIndexChanged(int index);
    void plotAreaIndexChanged(int index);

private:
    enum AxisMode {
        AxisModeNone,
        AxisModeValue,
        AxisModeLogValue,
        AxisModeDateTime,
        AxisModeCategory
    };

    void initXYValueChart();
    void setAngularAxis(AxisMode mode);
    void setRadialAxis(AxisMode mode);

    void applyRanges();
    void applyCategories();

    Ui::MainWindow *ui;

    int m_angularTickCount;
    int m_radialTickCount;
    int m_angularMinorTickCount;
    int m_radialMinorTickCount;
    qreal m_labelsAngle;
    qreal m_angularMin;
    qreal m_angularMax;
    qreal m_radialMin;
    qreal m_radialMax;
    bool m_angularShadesVisible;
    bool m_radialShadesVisible;
    bool m_labelsVisible;
    bool m_titleVisible;
    bool m_gridVisible;
    bool m_arrowVisible;
    bool m_minorGridVisible;
    bool m_minorArrowVisible;
    QBrush *m_angularShadesBrush;
    QBrush *m_radialShadesBrush;
    QBrush *m_labelBrush;
    QBrush *m_titleBrush;
    QBrush *m_backgroundBrush;
    QBrush *m_plotAreaBackgroundBrush;
    QPen *m_angularShadesPen;
    QPen *m_radialShadesPen;
    QPen *m_gridPen;
    QPen *m_arrowPen;
    QPen *m_minorGridPen;
    QPen *m_backgroundPen;
    QPen *m_plotAreaBackgroundPen;
    QString m_labelFormat;
    QFont m_currentLabelFont;
    QFont m_currentTitleFont;
    QChart::AnimationOptions m_animationOptions;
    QString m_angularTitle;
    QString m_radialTitle;
    qreal m_base;
    QString m_dateFormat;

    QPolarChart *m_chart;
    QAbstractAxis *m_angularAxis;
    QAbstractAxis *m_radialAxis;
    AxisMode m_angularAxisMode;
    AxisMode m_radialAxisMode;
    bool m_moreCategories;

    QScatterSeries *m_series1;
    QLineSeries *m_series2;
    QLineSeries *m_series3;
    QLineSeries *m_series4;
    QAreaSeries *m_series5;
    QSplineSeries *m_series6;
    QScatterSeries *m_series7;
};

#endif // MAINWINDOW_H
