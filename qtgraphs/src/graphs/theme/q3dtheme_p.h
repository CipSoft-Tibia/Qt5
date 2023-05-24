// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtGraphs API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#ifndef Q3DTHEME_P_H
#define Q3DTHEME_P_H

#include <private/graphsglobal_p.h>
#include "q3dtheme.h"

QT_BEGIN_NAMESPACE

struct Q3DThemeDirtyBitField {
    bool ambientLightStrengthDirty     : 1;
    bool backgroundColorDirty          : 1;
    bool backgroundEnabledDirty        : 1;
    bool baseColorDirty                : 1;
    bool baseGradientDirty             : 1;
    bool colorStyleDirty               : 1;
    bool fontDirty                     : 1;
    bool gridEnabledDirty              : 1;
    bool gridLineColorDirty            : 1;
    bool highlightLightStrengthDirty   : 1;
    bool labelBackgroundColorDirty     : 1;
    bool labelBackgroundEnabledDirty   : 1;
    bool labelBorderEnabledDirty       : 1;
    bool labelTextColorDirty           : 1;
    bool labelsEnabledDirty            : 1;
    bool lightColorDirty               : 1;
    bool lightStrengthDirty            : 1;
    bool multiHighlightColorDirty      : 1;
    bool multiHighlightGradientDirty   : 1;
    bool shadowStrengthDirty           : 1;
    bool singleHighlightColorDirty     : 1;
    bool singleHighlightGradientDirty  : 1;
    bool themeIdDirty                  : 1;
    bool windowColorDirty              : 1;

    Q3DThemeDirtyBitField()
        : ambientLightStrengthDirty(false),
          backgroundColorDirty(false),
          backgroundEnabledDirty(false),
          baseColorDirty(false),
          baseGradientDirty(false),
          colorStyleDirty(false),
          fontDirty(false),
          gridEnabledDirty(false),
          gridLineColorDirty(false),
          highlightLightStrengthDirty(false),
          labelBackgroundColorDirty(false),
          labelBackgroundEnabledDirty(false),
          labelBorderEnabledDirty(false),
          labelTextColorDirty(false),
          labelsEnabledDirty(false),
          lightColorDirty(false),
          lightStrengthDirty(false),
          multiHighlightColorDirty(false),
          multiHighlightGradientDirty(false),
          shadowStrengthDirty(false),
          singleHighlightColorDirty(false),
          singleHighlightGradientDirty(false),
          themeIdDirty(false),
          windowColorDirty(false)
    {
    }
};

class Q_GRAPHS_EXPORT Q3DThemePrivate : public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Q3DTheme)

public:
    Q3DThemePrivate(Q3DTheme *q);
    virtual ~Q3DThemePrivate();

    void resetDirtyBits();

    bool sync(Q3DThemePrivate &other);

    inline bool isDefaultTheme() { return m_isDefaultTheme; }
    inline void setDefaultTheme(bool isDefault) { m_isDefaultTheme = isDefault; }

    // If m_forcePredefinedType is true, it means we should forcibly update all properties
    // of the theme to those of the predefined theme, when setting the theme type. Otherwise
    // we only change the properties that haven't been explicitly changed since last render cycle.
    // Defaults to true, and is only ever set to false by DeclarativeTheme3D to enable using
    // predefined themes as base for custom themes, since the order of initial property sets cannot
    // be easily controlled in QML.
    inline bool isForcePredefinedType() { return m_forcePredefinedType; }
    inline void setForcePredefinedType(bool enable) { m_forcePredefinedType = enable; }

Q_SIGNALS:
    void needRender();

public:
    Q3DTheme::Theme m_themeId;

    Q3DThemeDirtyBitField m_dirtyBits;

    Q3DTheme::ColorStyle m_colorStyle;
    QColor m_backgroundColor;
    QColor m_gridLineColor;
    QColor m_lightColor;
    QColor m_multiHighlightColor;
    QColor m_singleHighlightColor;
    QColor m_textBackgroundColor;
    QColor m_textColor;
    QColor m_windowColor;
    QFont m_font;
    QLinearGradient m_multiHighlightGradient;
    QLinearGradient m_singleHighlightGradient;
    QList<QColor> m_baseColors;
    QList<QLinearGradient> m_baseGradients;
    bool m_backgoundEnabled;
    bool m_forcePredefinedType;
    bool m_gridEnabled;
    bool m_isDefaultTheme;
    bool m_labelBackground;
    bool m_labelBorders;
    bool m_labelsEnabled;
    float m_ambientLightStrength;
    float m_highlightLightStrength;
    float m_lightStrength;
    float m_shadowStrength;

protected:
    Q3DTheme *q_ptr;
};

QT_END_NAMESPACE

#endif
