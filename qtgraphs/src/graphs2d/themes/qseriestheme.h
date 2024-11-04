// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef SERIESTHEME_H
#define SERIESTHEME_H

#if 0
#  pragma qt_class(QSeriesTheme)
#endif

#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtCore/QObject>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlParserStatus>
#include <QtGraphs/qgraphsglobal.h>

QT_BEGIN_NAMESPACE

class QT_TECH_PREVIEW_API Q_GRAPHS_EXPORT QSeriesTheme : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(SeriesColorTheme colorTheme READ colorTheme WRITE setColorTheme NOTIFY colorThemeChanged FINAL)
    Q_PROPERTY(QList<QColor> colors READ colors WRITE setColors NOTIFY colorsChanged FINAL)
    Q_PROPERTY(QList<QColor> borderColors READ borderColors WRITE setBorderColors NOTIFY borderColorsChanged FINAL)
    Q_PROPERTY(qreal borderWidth READ borderWidth WRITE setBorderWidth NOTIFY borderWidthChanged FINAL)
    QML_NAMED_ELEMENT(SeriesTheme)

public:
    enum SeriesColorTheme {
        SeriesTheme1 = 0,
        SeriesTheme2
    };
    Q_ENUM(SeriesColorTheme)

    explicit QSeriesTheme(QObject *parent = nullptr);

    void resetColorTheme();

    int graphSeriesCount() const;
    void setGraphSeriesCount(int count);
    QColor indexColorFrom(const QList<QColor> &colors, int index) const;
    QColor graphSeriesColor(int index) const;
    QColor graphSeriesBorderColor(int index) const;

    QSeriesTheme::SeriesColorTheme colorTheme() const;
    void setColorTheme(const QSeriesTheme::SeriesColorTheme &newColorTheme);

    QList<QColor> colors() const;
    void setColors(const QList<QColor> &newColors);

    QList<QColor> borderColors() const;
    void setBorderColors(const QList<QColor> &newBorderColors);

    qreal borderWidth() const;
    void setBorderWidth(qreal newBorderWidth);

protected:
    // from QDeclarativeParserStatus
    void classBegin() override;
    void componentComplete() override;

Q_SIGNALS:
    void update();
    void colorThemeChanged();
    void colorsChanged();
    void borderColorsChanged();
    void borderWidthChanged();

private:
    void setColorTheme1();
    void setColorTheme2();

private:
    bool m_componentComplete = false;
    // TODO: Consider more detailed dirty flags
    bool m_themeDirty = true;

    SeriesColorTheme m_defaultColorTheme = SeriesColorTheme::SeriesTheme1;
    SeriesColorTheme m_colorTheme = SeriesColorTheme::SeriesTheme1;
    int m_seriesCount = 0;
    bool m_useCustomColors = false;
    // TODO: Support for these?
    //QList<QColor> m_seriesLabelColors;
    QList<QColor> m_colors;
    QList<QColor> m_borderColors;
    qreal m_borderWidth = 0;
};

QT_END_NAMESPACE

#endif // SERIESTHEME_H
