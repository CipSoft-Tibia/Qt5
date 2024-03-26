// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef Q3DTHEME_H
#define Q3DTHEME_H

#include <QtGraphs/qgraphsglobal.h>
#include <QtCore/QObject>
#include <QtGui/QLinearGradient>
#include <QtGui/QFont>
#include <QtGui/QColor>

QT_BEGIN_NAMESPACE

class Q3DThemePrivate;

class Q_GRAPHS_EXPORT Q3DTheme : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Q3DTheme)
    Q_PROPERTY(Q3DTheme::ColorStyle colorStyle READ colorStyle WRITE setColorStyle NOTIFY colorStyleChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(QColor gridLineColor READ gridLineColor WRITE setGridLineColor NOTIFY gridLineColorChanged)
    Q_PROPERTY(QColor labelBackgroundColor READ labelBackgroundColor WRITE setLabelBackgroundColor NOTIFY labelBackgroundColorChanged)
    Q_PROPERTY(QColor labelTextColor READ labelTextColor WRITE setLabelTextColor NOTIFY labelTextColorChanged)
    Q_PROPERTY(QColor lightColor READ lightColor WRITE setLightColor NOTIFY lightColorChanged)
    Q_PROPERTY(QColor multiHighlightColor READ multiHighlightColor WRITE setMultiHighlightColor NOTIFY multiHighlightColorChanged)
    Q_PROPERTY(QColor singleHighlightColor READ singleHighlightColor WRITE setSingleHighlightColor NOTIFY singleHighlightColorChanged)
    Q_PROPERTY(QColor windowColor READ windowColor WRITE setWindowColor NOTIFY windowColorChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(QLinearGradient multiHighlightGradient READ multiHighlightGradient WRITE setMultiHighlightGradient NOTIFY multiHighlightGradientChanged)
    Q_PROPERTY(QLinearGradient singleHighlightGradient READ singleHighlightGradient WRITE setSingleHighlightGradient NOTIFY singleHighlightGradientChanged)
    Q_PROPERTY(QList<QColor> baseColors READ baseColors WRITE setBaseColors NOTIFY baseColorsChanged)
    Q_PROPERTY(QList<QLinearGradient> baseGradients READ baseGradients WRITE setBaseGradients NOTIFY baseGradientsChanged)
    Q_PROPERTY(Q3DTheme::Theme type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(bool backgroundEnabled READ isBackgroundEnabled WRITE setBackgroundEnabled NOTIFY backgroundEnabledChanged)
    Q_PROPERTY(bool gridEnabled READ isGridEnabled WRITE setGridEnabled NOTIFY gridEnabledChanged)
    Q_PROPERTY(bool labelBackgroundEnabled READ isLabelBackgroundEnabled WRITE setLabelBackgroundEnabled NOTIFY labelBackgroundEnabledChanged)
    Q_PROPERTY(bool labelBorderEnabled READ isLabelBorderEnabled WRITE setLabelBorderEnabled NOTIFY labelBorderEnabledChanged)
    Q_PROPERTY(bool labelsEnabled READ isLabelsEnabled WRITE setLabelsEnabled NOTIFY labelsEnabledChanged)
    Q_PROPERTY(float ambientLightStrength READ ambientLightStrength WRITE setAmbientLightStrength NOTIFY ambientLightStrengthChanged)
    Q_PROPERTY(float highlightLightStrength READ highlightLightStrength WRITE setHighlightLightStrength NOTIFY highlightLightStrengthChanged)
    Q_PROPERTY(float lightStrength READ lightStrength WRITE setLightStrength NOTIFY lightStrengthChanged)
    Q_PROPERTY(float shadowStrength READ shadowStrength WRITE setShadowStrength NOTIFY shadowStrengthChanged)

public:
    enum ColorStyle {
        ColorStyleUniform = 0,
        ColorStyleObjectGradient,
        ColorStyleRangeGradient
    };
    Q_ENUM(ColorStyle)

    enum Theme {
        ThemeQt,
        ThemePrimaryColors,
        ThemeDigia,
        ThemeStoneMoss,
        ThemeArmyBlue,
        ThemeRetro,
        ThemeEbony,
        ThemeIsabelle,
        ThemeUserDefined
    };
    Q_ENUM(Theme)

public:
    explicit Q3DTheme(QObject *parent = nullptr);
    explicit Q3DTheme(Theme themeType, QObject *parent = nullptr);
    virtual ~Q3DTheme();

    void setType(Q3DTheme::Theme themeType);
    Q3DTheme::Theme type() const;

    void setBaseColors(const QList<QColor> &colors);
    QList<QColor> baseColors() const;

    void setBackgroundColor(const QColor &color);
    QColor backgroundColor() const;

    void setWindowColor(const QColor &color);
    QColor windowColor() const;

    void setLabelTextColor(const QColor &color);
    QColor labelTextColor() const;

    void setLabelBackgroundColor(const QColor &color);
    QColor labelBackgroundColor() const;

    void setGridLineColor(const QColor &color);
    QColor gridLineColor() const;

    void setSingleHighlightColor(const QColor &color);
    QColor singleHighlightColor() const;

    void setMultiHighlightColor(const QColor &color);
    QColor multiHighlightColor() const;

    void setLightColor(const QColor &color);
    QColor lightColor() const;

    void setBaseGradients(const QList<QLinearGradient> &gradients);
    QList<QLinearGradient> baseGradients() const;

    void setSingleHighlightGradient(const QLinearGradient &gradient);
    QLinearGradient singleHighlightGradient() const;

    void setMultiHighlightGradient(const QLinearGradient &gradient);
    QLinearGradient multiHighlightGradient() const;

    void setLightStrength(float strength);
    float lightStrength() const;

    void setAmbientLightStrength(float strength);
    float ambientLightStrength() const;

    void setHighlightLightStrength(float strength);
    float highlightLightStrength() const;

    void setLabelBorderEnabled(bool enabled);
    bool isLabelBorderEnabled() const;

    void setFont(const QFont &font);
    QFont font() const;

    void setBackgroundEnabled(bool enabled);
    bool isBackgroundEnabled() const;

    void setGridEnabled(bool enabled);
    bool isGridEnabled() const;

    void setLabelBackgroundEnabled(bool enabled);
    bool isLabelBackgroundEnabled() const;

    void setColorStyle(Q3DTheme::ColorStyle style);
    Q3DTheme::ColorStyle colorStyle() const;

    void setLabelsEnabled(bool enabled);
    bool isLabelsEnabled() const;

    void setShadowStrength(float strength);
    float shadowStrength() const;

Q_SIGNALS:
    void typeChanged(Q3DTheme::Theme themeType);
    void baseColorsChanged(const QList<QColor> &colors);
    void backgroundColorChanged(const QColor &color);
    void windowColorChanged(const QColor &color);
    void labelTextColorChanged(const QColor &color);
    void labelBackgroundColorChanged(const QColor &color);
    void gridLineColorChanged(const QColor &color);
    void singleHighlightColorChanged(const QColor &color);
    void multiHighlightColorChanged(const QColor &color);
    void lightColorChanged(const QColor &color);
    void baseGradientsChanged(const QList<QLinearGradient> &gradients);
    void singleHighlightGradientChanged(const QLinearGradient &gradient);
    void multiHighlightGradientChanged(const QLinearGradient &gradient);
    void lightStrengthChanged(float strength);
    void ambientLightStrengthChanged(float strength);
    void highlightLightStrengthChanged(float strength);
    void labelBorderEnabledChanged(bool enabled);
    void fontChanged(const QFont &font);
    void backgroundEnabledChanged(bool enabled);
    void gridEnabledChanged(bool enabled);
    void labelBackgroundEnabledChanged(bool enabled);
    void colorStyleChanged(Q3DTheme::ColorStyle style);
    void labelsEnabledChanged(bool enabled);
    void shadowStrengthChanged(float strength);

protected:
    explicit Q3DTheme(Q3DThemePrivate *d, Theme themeType, QObject *parent = nullptr);

    QScopedPointer<Q3DThemePrivate> d_ptr;

private:
    Q_DISABLE_COPY(Q3DTheme)

    friend class ThemeManager;
    friend class QQuickGraphsBars;
    friend class Abstract3DController;
    friend class QQuickGraphsItem;
    friend class DeclarativeTheme3D;
};

QT_END_NAMESPACE

#endif
