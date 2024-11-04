// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef GRAPHTHEME_H
#define GRAPHTHEME_H

#if 0
#  pragma qt_class(QGraphTheme)
#endif

#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtCore/QObject>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlParserStatus>
#include <QtGraphs/qgraphsglobal.h>

QT_BEGIN_NAMESPACE

class QT_TECH_PREVIEW_API Q_GRAPHS_EXPORT QGraphTheme : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(ColorTheme colorTheme READ colorTheme WRITE setColorTheme NOTIFY colorThemeChanged FINAL)
    Q_PROPERTY(qreal gridMajorBarsWidth READ gridMajorBarsWidth WRITE setGridMajorBarsWidth NOTIFY gridMajorBarsWidthChanged FINAL)
    Q_PROPERTY(qreal gridMinorBarsWidth READ gridMinorBarsWidth WRITE setGridMinorBarsWidth NOTIFY gridMinorBarsWidthChanged FINAL)
    Q_PROPERTY(qreal gridSmoothing READ gridSmoothing WRITE setGridSmoothing NOTIFY gridSmoothingChanged FINAL)
    Q_PROPERTY(QColor gridMajorBarsColor READ gridMajorBarsColor WRITE setGridMajorBarsColor NOTIFY gridMajorBarsColorChanged FINAL)
    Q_PROPERTY(QColor gridMinorBarsColor READ gridMinorBarsColor WRITE setGridMinorBarsColor NOTIFY gridMinorBarsColorChanged FINAL)

    Q_PROPERTY(QColor axisYMajorColor READ axisYMajorColor WRITE setAxisYMajorColor NOTIFY axisYMajorColorChanged FINAL)
    Q_PROPERTY(QColor axisYMinorColor READ axisYMinorColor WRITE setAxisYMinorColor NOTIFY axisYMinorColorChanged FINAL)
    Q_PROPERTY(qreal axisYMajorBarWidth READ axisYMajorBarWidth WRITE setAxisYMajorBarWidth NOTIFY axisYMajorBarWidthChanged FINAL)
    Q_PROPERTY(qreal axisYMinorBarWidth READ axisYMinorBarWidth WRITE setAxisYMinorBarWidth NOTIFY axisYMinorBarWidthChanged FINAL)
    Q_PROPERTY(qreal axisYSmoothing READ axisYSmoothing WRITE setAxisYSmoothing NOTIFY axisYSmoothingChanged FINAL)
    Q_PROPERTY(QColor axisYLabelsColor READ axisYLabelsColor WRITE setAxisYLabelsColor NOTIFY axisYLabelsColorChanged FINAL)
    Q_PROPERTY(QFont axisYLabelsFont READ axisYLabelsFont WRITE setAxisYLabelsFont NOTIFY axisYLabelsFontChanged FINAL)

    Q_PROPERTY(QColor axisXMajorColor READ axisXMajorColor WRITE setAxisXMajorColor NOTIFY axisXMajorColorChanged FINAL)
    Q_PROPERTY(QColor axisXMinorColor READ axisXMinorColor WRITE setAxisXMinorColor NOTIFY axisXMinorColorChanged FINAL)
    Q_PROPERTY(qreal axisXMajorBarWidth READ axisXMajorBarWidth WRITE setAxisXMajorBarWidth NOTIFY axisXMajorBarWidthChanged FINAL)
    Q_PROPERTY(qreal axisXMinorBarWidth READ axisXMinorBarWidth WRITE setAxisXMinorBarWidth NOTIFY axisXMinorBarWidthChanged FINAL)
    Q_PROPERTY(qreal axisXSmoothing READ axisXSmoothing WRITE setAxisXSmoothing NOTIFY axisXSmoothingChanged FINAL)
    Q_PROPERTY(QColor axisXLabelsColor READ axisXLabelsColor WRITE setAxisXLabelsColor NOTIFY axisXLabelsColorChanged FINAL)
    Q_PROPERTY(QFont axisXLabelsFont READ axisXLabelsFont WRITE setAxisXLabelsFont NOTIFY axisXLabelsFontChanged FINAL)

    Q_PROPERTY(bool shadowEnabled READ shadowEnabled WRITE setShadowEnabled NOTIFY shadowEnabledChanged FINAL)
    Q_PROPERTY(QColor shadowColor READ shadowColor WRITE setShadowColor NOTIFY shadowColorChanged FINAL)
    Q_PROPERTY(qreal shadowBarWidth READ shadowBarWidth WRITE setShadowBarWidth NOTIFY shadowBarWidthChanged FINAL)
    Q_PROPERTY(qreal shadowXOffset READ shadowXOffset WRITE setShadowXOffset NOTIFY shadowXOffsetChanged FINAL)
    Q_PROPERTY(qreal shadowYOffset READ shadowYOffset WRITE setShadowYOffset NOTIFY shadowYOffsetChanged FINAL)
    Q_PROPERTY(qreal shadowSmoothing READ shadowSmoothing WRITE setShadowSmoothing NOTIFY shadowSmoothingChanged FINAL)

    QML_NAMED_ELEMENT(GraphTheme)

public:
    enum ColorTheme {
        ColorThemeLight = 0,
        ColorThemeDark
    };
    Q_ENUM(ColorTheme)

    explicit QGraphTheme(QObject *parent = nullptr);

    bool themeDirty() const { return m_themeDirty; };
    void resetThemeDirty() { m_themeDirty = false; };
    void resetColorTheme();

    QGraphTheme::ColorTheme colorTheme() const;
    void setColorTheme(const ColorTheme &newColorTheme);

    qreal gridMajorBarsWidth() const;
    void setGridMajorBarsWidth(qreal newGridMajorBarsWidth);

    qreal gridMinorBarsWidth() const;
    void setGridMinorBarsWidth(qreal newGridMinorBarsWidth);

    qreal gridSmoothing() const;
    void setGridSmoothing(qreal newGridSmoothing);

    QColor gridMajorBarsColor() const;
    void setGridMajorBarsColor(const QColor &newGridMajorBarsColor);

    QColor gridMinorBarsColor() const;
    void setGridMinorBarsColor(const QColor &newGridMinorBarsColor);

    QColor axisYMajorColor() const;
    void setAxisYMajorColor(const QColor &newAxisYMajorColor);

    QColor axisYMinorColor() const;
    void setAxisYMinorColor(const QColor &newAxisYMinorColor);

    qreal axisYMajorBarWidth() const;
    void setAxisYMajorBarWidth(qreal newAxisYMajorBarWidth);

    qreal axisYMinorBarWidth() const;
    void setAxisYMinorBarWidth(qreal newAxisYMinorBarWidth);

    qreal axisYSmoothing() const;
    void setAxisYSmoothing(qreal newAxisYSmoothing);

    QColor axisXMajorColor() const;
    void setAxisXMajorColor(const QColor &newAxisXMajorColor);

    QColor axisXMinorColor() const;
    void setAxisXMinorColor(const QColor &newAxisXMinorColor);

    qreal axisXMajorBarWidth() const;
    void setAxisXMajorBarWidth(qreal newAxisXMajorBarWidth);

    qreal axisXMinorBarWidth() const;
    void setAxisXMinorBarWidth(qreal newAxisXMinorBarWidth);

    qreal axisXSmoothing() const;
    void setAxisXSmoothing(qreal newAxisXSmoothing);

    QColor axisYLabelsColor() const;
    void setAxisYLabelsColor(const QColor &newAxisYLabelsColor);

    QFont axisYLabelsFont() const;
    void setAxisYLabelsFont(const QFont &newAxisYLabelsFont);

    QColor axisXLabelsColor() const;
    void setAxisXLabelsColor(const QColor &newAxisXLabelsColor);

    QFont axisXLabelsFont() const;
    void setAxisXLabelsFont(const QFont &newAxisXLabelsFont);

    bool shadowEnabled() const;
    void setShadowEnabled(bool newShadowEnabled);

    QColor shadowColor() const;
    void setShadowColor(const QColor &newShadowColor);

    qreal shadowBarWidth() const;
    void setShadowBarWidth(qreal newShadowBarWidth);

    qreal shadowXOffset() const;
    void setShadowXOffset(qreal newShadowXOffset);

    qreal shadowYOffset() const;
    void setShadowYOffset(qreal newShadowYOffset);

    qreal shadowSmoothing() const;
    void setShadowSmoothing(qreal newShadowSmoothing);

Q_SIGNALS:
    void update();
    void colorThemeChanged();
    void gridMajorBarsWidthChanged();
    void gridMinorBarsWidthChanged();
    void gridSmoothingChanged();
    void gridMajorBarsColorChanged();
    void gridMinorBarsColorChanged();

    void axisYMajorColorChanged();
    void axisYMinorColorChanged();
    void axisYMajorBarWidthChanged();
    void axisYMinorBarWidthChanged();
    void axisYSmoothingChanged();
    void axisYLabelsColorChanged();
    void axisYLabelsFontChanged();

    void axisXMajorColorChanged();
    void axisXMinorColorChanged();
    void axisXMajorBarWidthChanged();
    void axisXMinorBarWidthChanged();
    void axisXSmoothingChanged();
    void axisXLabelsColorChanged();
    void axisXLabelsFontChanged();

    void shadowEnabledChanged();
    void shadowColorChanged();
    void shadowBarWidthChanged();
    void shadowXOffsetChanged();
    void shadowYOffsetChanged();
    void shadowSmoothingChanged();

protected:
    // from QDeclarativeParserStatus
    void classBegin() override;
    void componentComplete() override;

private:
    void updateTheme();
    void setColorThemeLight();
    void setColorThemeDark();

private:
    friend class tst_graphtheme; // For autotests

    struct CustomFlags {
        // All properties should have this to inform they have been set
        // in QML side. Otherwise setting the common themes can override
        // the values as QML properties have no order of evaluation.
        bool gridMajorBarsColor = false;
        bool gridMinorBarsColor = false;
        bool axisYMajorColor = false;
        bool axisYMinorColor = false;
        bool axisYLabelsColor = false;
        //bool axisYLabelsFont = false;
        bool axisXMajorColor = false;
        bool axisXMinorColor = false;
        bool axisXLabelsColor = false;
        //bool axisXLabelsFont = false;
    };

    bool m_componentComplete = false;
    // TODO: Consider more detailed dirty flags
    bool m_themeDirty = true;
    CustomFlags m_customFlags;

    ColorTheme m_colorTheme = QGraphTheme::ColorThemeDark;
    qreal m_gridMajorBarsWidth = 2.0;
    qreal m_gridMinorBarsWidth = 1.0;
    qreal m_gridSmoothing = 1.0;
    QColor m_gridMajorBarsColor;
    QColor m_gridMinorBarsColor;
    QColor m_axisYMajorColor;
    QColor m_axisYMinorColor;
    qreal m_axisYMajorBarWidth = 2.0;
    qreal m_axisYMinorBarWidth = 1.0;
    qreal m_axisYSmoothing = 1.0;
    QColor m_axisYLabelsColor;
    QFont m_axisYLabelsFont;
    QColor m_axisXMajorColor;
    QColor m_axisXMinorColor;
    qreal m_axisXMajorBarWidth = 2.0;
    qreal m_axisXMinorBarWidth = 1.0;
    qreal m_axisXSmoothing = 1.0;
    QColor m_axisXLabelsColor;
    QFont m_axisXLabelsFont;
    bool m_shadowEnabled = false;
    QColor m_shadowColor;
    qreal m_shadowBarWidth = 0.0;
    qreal m_shadowXOffset = 0.0;
    qreal m_shadowYOffset = 0.0;
    qreal m_shadowSmoothing = 5.0;
};

QT_END_NAMESPACE

#endif // GRAPHTHEME_H
