// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "q3dtheme_p.h"
#include "thememanager_p.h"
#include "utils_p.h"

QT_BEGIN_NAMESPACE

/*!
 * \class Q3DTheme
 * \inmodule QtGraphs
 * \brief Q3DTheme class provides a visual style for graphs.
 *
 * Specifies visual properties that affect the whole graph. There are several
 * built-in themes that can be used as is or modified freely.
 *
 * The following properties can be overridden by using QAbstract3DSeries
 * properties to set them explicitly in the series: baseColors, baseGradients,
 * and colorStyle.
 *
 * Themes can be created from scratch using the ThemeUserDefined enum value.
 * Creating a theme using the default constructor produces a new user-defined
 * theme.
 *
 * \section1 Default Theme
 *
 * The following table lists the properties controlled by themes and the
 * default values for ThemeUserDefined.
 *
 * \table
 *   \header
 *     \li Property
 *     \li Default Value
 *   \row
 *     \li ambientLightStrength
 *     \li 0.25
 *   \row
 *     \li backgroundColor
 *     \li Qt::black
 *   \row
 *     \li backgroundEnabled
 *     \li \c true
 *   \row
 *     \li baseColors
 *     \li Qt::black
 *   \row
 *     \li baseGradients
 *     \li QLinearGradient. Essentially fully black.
 *   \row
 *     \li colorStyle
 *     \li ColorStyleUniform
 *   \row
 *     \li \l font
 *     \li QFont
 *   \row
 *     \li gridEnabled
 *     \li \c true
 *   \row
 *     \li gridLineColor
 *     \li Qt::white
 *   \row
 *     \li highlightLightStrength
 *     \li 7.5
 *   \row
 *     \li labelBackgroundColor
 *     \li Qt::gray
 *   \row
 *     \li labelBackgroundEnabled
 *     \li \c true
 *   \row
 *     \li labelBorderEnabled
 *     \li \c true
 *   \row
 *     \li labelTextColor
 *     \li Qt::white
 *   \row
 *     \li labelsEnabled
 *     \li \c true
 *   \row
 *     \li lightColor
 *     \li Qt::white
 *   \row
 *     \li lightStrength
 *     \li 5.0
 *   \row
 *     \li multiHighlightColor
 *     \li Qt::blue
 *   \row
 *     \li multiHighlightGradient
 *     \li QLinearGradient. Essentially fully black.
 *   \row
 *     \li shadowStrength
 *     \li 25.0
 *   \row
 *     \li singleHighlightColor
 *     \li Qt::red
 *   \row
 *     \li singleHighlightGradient
 *     \li QLinearGradient. Essentially fully black.
 *   \row
 *     \li windowColor
 *     \li Qt::black
 * \endtable
 *
 * \section1 Usage Examples
 *
 * Creating a built-in theme without any modifications:
 *
 * \snippet doc_src_q3dtheme.cpp 0
 *
 * Creating a built-in theme and modifying some properties:
 *
 * \snippet doc_src_q3dtheme.cpp 1
 *
 * Creating a user-defined theme:
 *
 * \snippet doc_src_q3dtheme.cpp 2
 *
 * Creating a built-in theme and modifying some properties after it has been set:
 *
 * \snippet doc_src_q3dtheme.cpp 3
 *
 */

/*!
 * \enum Q3DTheme::ColorStyle
 *
 * Color styles.
 *
 * \value ColorStyleUniform
 *        Objects are rendered in a single color. The color used is specified in baseColors,
 *        singleHighlightColor and multiHighlightColor properties.
 * \value ColorStyleObjectGradient
 *        Objects are colored using a full gradient for each object regardless of object height. The
 *        gradient used is specified in baseGradients, singleHighlightGradient and
 *        multiHighlightGradient properties.
 * \value ColorStyleRangeGradient
 *        Objects are colored using a portion of the full gradient determined by the object's
 *        height and its position on the Y-axis. The gradient used is specified in baseGradients,
 *        singleHighlightGradient and multiHighlightGradient properties.
 */

/*!
 * \enum Q3DTheme::Theme
 *
 * Built-in themes.
 *
 * \value ThemeQt
 *        A light theme with green as the base color.
 * \value ThemePrimaryColors
 *        A light theme with yellow as the base color.
 * \value ThemeDigia
 *        A light theme with gray as the base color.
 * \value ThemeStoneMoss
 *        A medium dark theme with yellow as the base color.
 * \value ThemeArmyBlue
 *        A medium light theme with blue as the base color.
 * \value ThemeRetro
 *        A medium light theme with brown as the base color.
 * \value ThemeEbony
 *        A dark theme with white as the base color.
 * \value ThemeIsabelle
 *        A dark theme with yellow as the base color.
 * \value ThemeUserDefined
 *        A user-defined theme. For more information, see \l {Default Theme}.
 */

/*!
 * \qmltype Theme3D
 * \inqmlmodule QtGraphs
 * \ingroup graphs_qml
 * \instantiates Q3DTheme
 * \brief A visual style for graphs.
 *
 * This type is used to specify visual properties that affect the whole graph. There are several
 * built-in themes that can be used as is or modified freely.
 *
 * The following properties can be overridden by using Abstract3DSeries
 * properties to set them explicitly in the series:
 * baseColors, baseGradients, and colorStyle.
 *
 * Themes can be created from scratch by using the
 * \l{Q3DTheme::ThemeUserDefined}{Theme3D.ThemeUserDefined} enum value.
 *
 * \section1 Default Theme
 *
 * The following table lists the properties controlled by themes and the
 * default values for \l{Q3DTheme::ThemeUserDefined}
 * {Theme3D.ThemeUserDefined}.
 *
 * \table
 *   \header
 *     \li Property
 *     \li Default Value
 *   \row
 *     \li ambientLightStrength
 *     \li 0.25
 *   \row
 *     \li backgroundColor
 *     \li "black". For more information, see \l [QtQuick] color.
 *   \row
 *     \li backgroundEnabled
 *     \li \c true
 *   \row
 *     \li baseColors
 *     \li "black"
 *   \row
 *     \li baseGradients
 *     \li QLinearGradient. Essentially fully black.
 *   \row
 *     \li colorStyle
 *     \li ColorStyleUniform
 *   \row
 *     \li \l font
 *     \li \l [QtQuick] font
 *   \row
 *     \li gridEnabled
 *     \li \c true
 *   \row
 *     \li gridLineColor
 *     \li "white"
 *   \row
 *     \li highlightLightStrength
 *     \li 7.5
 *   \row
 *     \li labelBackgroundColor
 *     \li "gray"
 *   \row
 *     \li labelBackgroundEnabled
 *     \li \c true
 *   \row
 *     \li labelBorderEnabled
 *     \li \c true
 *   \row
 *     \li labelTextColor
 *     \li "white"
 *   \row
 *     \li labelsEnabled
 *     \li \c true
 *   \row
 *     \li lightColor
 *     \li "white"
 *   \row
 *     \li lightStrength
 *     \li 5.0
 *   \row
 *     \li multiHighlightColor
 *     \li "blue"
 *   \row
 *     \li multiHighlightGradient
 *     \li QLinearGradient. Essentially fully black.
 *   \row
 *     \li singleHighlightColor
 *     \li "red"
 *   \row
 *     \li singleHighlightGradient
 *     \li QLinearGradient. Essentially fully black.
 *   \row
 *     \li windowColor
 *     \li "black"
 * \endtable
 *
 * \section1 Usage examples
 *
 * Using a built-in theme without any modifications:
 *
 * \snippet doc_src_q3dtheme.cpp 4
 *
 * Using a built-in theme and modifying some properties:
 *
 * \snippet doc_src_q3dtheme.cpp 5
 *
 * Using a user-defined theme:
 *
 * \snippet doc_src_q3dtheme.cpp 6
 *
 * For Theme3D enums, see \l Q3DTheme::ColorStyle and \l{Q3DTheme::Theme}.
 */

/*!
 * \qmlproperty list<ThemeColor> Theme3D::baseColors
 *
 * The list of base colors to be used for all the objects in the graph, series by series. If there
 * are more series than colors, color list wraps and starts again with the first color in the list.
 * Has no immediate effect if colorStyle is not \c Theme3D.ColorStyleUniform.
 *
 * This can be overridden by setting \l{Abstract3DSeries::baseColor}
 * {Abstract3DSeries.baseColor} explicitly in the series.
 */

/*!
 * \qmlproperty color Theme3D::backgroundColor
 *
 * The color of the graph background.
 */

/*!
 * \qmlproperty color Theme3D::windowColor
 *
 * The color of the application window the graph is drawn into.
 */

/*!
 * \qmlproperty color Theme3D::labelTextColor
 *
 * The color of the font used for labels.
 */

/*!
 * \qmlproperty color Theme3D::labelBackgroundColor
 *
 * The color of the label backgrounds. Has no effect if labelBackgroundEnabled is \c false.
 */

/*!
 * \qmlproperty color Theme3D::gridLineColor
 *
 * The color of the grid lines.
 */

/*!
 * \qmlproperty color Theme3D::singleHighlightColor
 *
 * The highlight color for a selected object. Used if
 * \l{AbstractGraph3D::selectionMode}{selectionMode}
 * has the \c AbstractGraph3D.SelectionItem flag set.
 */

/*!
 * \qmlproperty color Theme3D::multiHighlightColor
 *
 * The highlight color for selected objects. Used if
 * \l{AbstractGraph3D::selectionMode}{selectionMode}
 * has the \c AbstractGraph3D.SelectionRow or \c AbstractGraph3D.SelectionColumn
 * flag set.
 */

/*!
 * \qmlproperty color Theme3D::lightColor
 *
 * The color of the ambient and specular light defined in Scene3D.
 */

/*!
 * \qmlproperty list<ColorGradient> Theme3D::baseGradients
 *
 * The list of base gradients to be used for all the objects in the graph,
 * series by series. If there are more series than gradients, the gradient list
 * wraps and starts again with the first gradient in the list.
 *
 * Has no immediate effect if colorStyle is \l{Q3DTheme::ColorStyleUniform}
 * {Theme3D.ColorStyleUniform}.
 *
 * This value can be overridden by setting \l{Abstract3DSeries::baseGradient}
 *{Abstract3DSeries.baseGradient} explicitly in the series.
 */

/*!
 * \qmlproperty ColorGradient Theme3D::singleHighlightGradient
 *
 * The highlight gradient for a selected object. Used if
 * \l{AbstractGraph3D::selectionMode}{selectionMode}
 * has the \c AbstractGraph3D.SelectionItem flag set.
 */

/*!
 * \qmlproperty ColorGradient Theme3D::multiHighlightGradient
 *
 * The highlight gradient for selected objects. Used if
 * \l{AbstractGraph3D::selectionMode}{selectionMode}
 * has the \c AbstractGraph3D.SelectionRow or \c AbstractGraph3D.SelectionColumn
 * flag set.
 */

/*!
 * \qmlproperty real Theme3D::lightStrength
 *
 * The specular light strength for the whole graph. The value must be between
 * \c 0.0 and \c 10.0.
 *
 * This value affects the light specified in Scene3D.
 */

/*!
 * \qmlproperty real Theme3D::ambientLightStrength
 *
 * The ambient light strength for the whole graph. This value determines how
 * evenly and brightly the colors are shown throughout the graph regardless of
 * the light position. The value must be between \c 0.0 and \c 1.0.
 */

/*!
 * \qmlproperty real Theme3D::highlightLightStrength
 *
 * The specular light strength for selected objects. The value must be
 * between \c 0.0 and \c 10.0.
 */

/*!
 * \qmlproperty bool Theme3D::labelBorderEnabled
 *
 * Defines whether label borders are drawn for labels that have a background.
 * Has no effect if labelBackgroundEnabled is \c false.
 */

/*!
 * \qmlproperty font Theme3D::font
 *
 * Sets the font to be used for labels.
 */

/*!
 * \qmlproperty bool Theme3D::backgroundEnabled
 *
 * Defines whether the background is drawn by using the value of
 * backgroundColor.
 */

/*!
 * \qmlproperty bool Theme3D::gridEnabled
 *
 * Defines whether the grid lines are drawn. This value affects all grid lines.
 */

/*!
 * \qmlproperty bool Theme3D::labelBackgroundEnabled
 *
 * Defines whether the label is drawn with a background that uses
 * labelBackgroundColor (including alpha), or with a fully transparent
 * background. Labels with a background are drawn to equal sizes per axis based
 * on the longest label, and the text is centered in them. Labels without
 * a background are drawn as is and are left or right aligned based on their
 * position in the graph.
 */

/*!
 * \qmlproperty Theme3D.ColorStyle Theme3D::colorStyle
 *
 * The style of the graph colors. One of Q3DTheme::ColorStyle enum values.
 *
 * This value can be overridden by setting \l{Abstract3DSeries::colorStyle}
 * {Abstract3DSeries.colorStyle} explicitly in the series.
 *
 * \sa Q3DTheme::ColorStyle
 */

/*!
 * \qmlproperty bool Theme3D::labelsEnabled
 *
 * Defines whether labels are drawn at all. If this is \c{false}, all other label properties
 * have no effect.
 */

/*!
 * \qmlproperty Theme3D.Theme Theme3D::type
 *
 * The type of the theme. If no type is set, the type is
 * \l{Q3DTheme::ThemeUserDefined}{Theme3D.ThemeUserDefined}.
 * Changing the theme type after the item has been constructed will change all other properties
 * of the theme to what the predefined theme specifies. Changing the theme type of the active theme
 * of the graph will also reset all attached series to use the new theme.
 */

/*!
 * \qmlproperty real Theme3D::shadowStrength
 *
 * The shadow strength for the whole graph. The higher the number, the darker the shadows will be.
 * The value must be between \c 0.0 and \c 100.0.
 *
 * This value affects the light specified in Scene3D.
 */

/*!
 * Constructs a new theme of type ThemeUserDefined. An optional \a parent parameter
 * can be given and is then passed to QObject constructor.
 */
Q3DTheme::Q3DTheme(QObject *parent)
    : QObject(parent),
      d_ptr(new Q3DThemePrivate(this))
{
}

/*!
 * Constructs a new theme with \a themeType, which can be one of the built-in themes from
 * \l Theme. An optional \a parent parameter can be given and is then passed to QObject
 * constructor.
 */
Q3DTheme::Q3DTheme(Theme themeType, QObject *parent)
    : QObject(parent),
      d_ptr(new Q3DThemePrivate(this))
{
    setType(themeType);
}

/*!
 * \internal
 */
Q3DTheme::Q3DTheme(Q3DThemePrivate *d, Theme themeType,
                   QObject *parent) :
    QObject(parent),
    d_ptr(d)
{
    setType(themeType);
}

/*!
 * Destroys the theme.
 */
Q3DTheme::~Q3DTheme()
{
}

/*!
 * \property Q3DTheme::baseColors
 *
 * \brief The list of base colors to be used for all the objects in the graph,
 * series by series.
 *
 * If there are more series than colors, the color list wraps and starts again
 * with the first color in the list.
 *
 * Has no immediate effect if colorStyle is not ColorStyleUniform.
 *
 * This value can be overridden by setting the \l{QAbstract3DSeries::baseColor}
 * {baseColor} explicitly in the series.
 */
void Q3DTheme::setBaseColors(const QList<QColor> &colors)
{
    Q_D(Q3DTheme);
    if (colors.size()) {
        d->m_dirtyBits.baseColorDirty = true;
        if (d->m_baseColors != colors) {
            d->m_baseColors.clear();
            d->m_baseColors = colors;
            emit baseColorsChanged(colors);
        }
    } else {
        d->m_baseColors.clear();
    }
}

QList<QColor> Q3DTheme::baseColors() const
{
    const Q_D(Q3DTheme);
    return d->m_baseColors;
}

/*!
 * \property Q3DTheme::backgroundColor
 *
 * \brief The color of the graph background.
 */
void Q3DTheme::setBackgroundColor(const QColor &color)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.backgroundColorDirty = true;
    if (d->m_backgroundColor != color) {
        d->m_backgroundColor = color;
        emit backgroundColorChanged(color);
        emit d->needRender();
    }
}

QColor Q3DTheme::backgroundColor() const
{
    const Q_D(Q3DTheme);
    return d->m_backgroundColor;
}

/*!
 * \property Q3DTheme::windowColor
 *
 * \brief The color of the application window the graph is drawn into.
 */
void Q3DTheme::setWindowColor(const QColor &color)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.windowColorDirty = true;
    if (d->m_windowColor != color) {
        d->m_windowColor = color;
        emit windowColorChanged(color);
        emit d->needRender();
    }
}

QColor Q3DTheme::windowColor() const
{
    const Q_D(Q3DTheme);
    return d->m_windowColor;
}

/*!
 * \property Q3DTheme::labelTextColor
 *
 * \brief The color of the font used for labels.
 */
void Q3DTheme::setLabelTextColor(const QColor &color)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.labelTextColorDirty = true;
    if (d->m_textColor != color) {
        d->m_textColor = color;
        emit labelTextColorChanged(color);
        emit d->needRender();
    }
}

QColor Q3DTheme::labelTextColor() const
{
    const Q_D(Q3DTheme);
    return d->m_textColor;
}

/*!
 * \property Q3DTheme::labelBackgroundColor
 *
 * \brief The color of the label backgrounds.
 *
 * Has no effect if labelBackgroundEnabled is \c false.
 */
void Q3DTheme::setLabelBackgroundColor(const QColor &color)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.labelBackgroundColorDirty = true;
    if (d->m_textBackgroundColor != color) {
        d->m_textBackgroundColor = color;
        emit labelBackgroundColorChanged(color);
        emit d->needRender();
    }
}

QColor Q3DTheme::labelBackgroundColor() const
{
    const Q_D(Q3DTheme);
    return d->m_textBackgroundColor;
}

/*!
 * \property Q3DTheme::gridLineColor
 *
 * \brief The color of the grid lines.
 */
void Q3DTheme::setGridLineColor(const QColor &color)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.gridLineColorDirty = true;
    if (d->m_gridLineColor != color) {
        d->m_gridLineColor = color;
        emit gridLineColorChanged(color);
        emit d->needRender();
    }
}

QColor Q3DTheme::gridLineColor() const
{
    const Q_D(Q3DTheme);
    return d->m_gridLineColor;
}

/*!
 * \property Q3DTheme::singleHighlightColor
 *
 * \brief The highlight color for a selected object.
 *
 * Used if \l{QAbstract3DGraph::selectionMode}{selectionMode} has the
 * \c QAbstract3DGraph::SelectionItem flag set.
 */
void Q3DTheme::setSingleHighlightColor(const QColor &color)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.singleHighlightColorDirty = true;
    if (d->m_singleHighlightColor != color) {
        d->m_singleHighlightColor = color;
        emit singleHighlightColorChanged(color);
    }
}

QColor Q3DTheme::singleHighlightColor() const
{
    const Q_D(Q3DTheme);
    return d->m_singleHighlightColor;
}

/*!
 * \property Q3DTheme::multiHighlightColor
 *
 * \brief The highlight color for selected objects.
 *
 * Used if \l{QAbstract3DGraph::selectionMode}{selectionMode} has the
 * \c QAbstract3DGraph::SelectionRow or \c QAbstract3DGraph::SelectionColumn
 * flag set.
 */
void Q3DTheme::setMultiHighlightColor(const QColor &color)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.multiHighlightColorDirty = true;
    if (d->m_multiHighlightColor != color) {
        d->m_multiHighlightColor = color;
        emit multiHighlightColorChanged(color);
    }
}

QColor Q3DTheme::multiHighlightColor() const
{
    const Q_D(Q3DTheme);
    return d->m_multiHighlightColor;
}

/*!
 * \property Q3DTheme::lightColor
 *
 * \brief The color for the ambient and specular light.
 *
 * This value affects the light specified in Q3DScene.
 */
void Q3DTheme::setLightColor(const QColor &color)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.lightColorDirty = true;
    if (d->m_lightColor != color) {
        d->m_lightColor = color;
        emit lightColorChanged(color);
        emit d->needRender();
    }
}

QColor Q3DTheme::lightColor() const
{
    const Q_D(Q3DTheme);
    return d->m_lightColor;
}

/*!
 * \property Q3DTheme::baseGradients
 *
 * \brief The list of base gradients to be used for all the objects in the
 * graph, series by series.
 *
 * If there are more series than gradients, the gradient list wraps and starts
 * again with the first gradient in the list
 *
 * Has no immediate effect if colorStyle is ColorStyleUniform.
 *
 * This value can be overridden by setting the
 * \l{QAbstract3DSeries::baseGradient}{baseGradient} explicitly in the series.
 */
void Q3DTheme::setBaseGradients(const QList<QLinearGradient> &gradients)
{
    Q_D(Q3DTheme);
    if (gradients.size()) {
        d->m_dirtyBits.baseGradientDirty = true;
        if (d->m_baseGradients != gradients) {
            d->m_baseGradients.clear();
            d->m_baseGradients = gradients;
            emit baseGradientsChanged(gradients);
        }
    } else {
        d->m_baseGradients.clear();
    }
}

QList<QLinearGradient> Q3DTheme::baseGradients() const
{
    const Q_D(Q3DTheme);
    return d->m_baseGradients;
}

/*!
 * \property Q3DTheme::singleHighlightGradient
 *
 * \brief The highlight gradient for a selected object.
 *
 * Used if \l{QAbstract3DGraph::selectionMode}{selectionMode}
 * has the \c QAbstract3DGraph::SelectionItem flag set.
 */
void Q3DTheme::setSingleHighlightGradient(const QLinearGradient &gradient)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.singleHighlightGradientDirty = true;
    if (d->m_singleHighlightGradient != gradient) {
        Utils::verifyGradientCompleteness(d->m_singleHighlightGradient);
        d->m_singleHighlightGradient = gradient;
        emit singleHighlightGradientChanged(gradient);
    }
}

QLinearGradient Q3DTheme::singleHighlightGradient() const
{
    const Q_D(Q3DTheme);
    return d->m_singleHighlightGradient;
}

/*!
 * \property Q3DTheme::multiHighlightGradient
 *
 * \brief The highlight gradient for selected objects.
 *
 * Used if \l{QAbstract3DGraph::selectionMode}{selectionMode}
 * has the \c QAbstract3DGraph::SelectionRow or
 * \c QAbstract3DGraph::SelectionColumn flag set.
 */
void Q3DTheme::setMultiHighlightGradient(const QLinearGradient &gradient)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.multiHighlightGradientDirty = true;
    if (d->m_multiHighlightGradient != gradient) {
        Utils::verifyGradientCompleteness(d->m_multiHighlightGradient);
        d->m_multiHighlightGradient = gradient;
        emit multiHighlightGradientChanged(gradient);
    }
}

QLinearGradient Q3DTheme::multiHighlightGradient() const
{
    const Q_D(Q3DTheme);
    return d->m_multiHighlightGradient;
}

/*!
 * \property Q3DTheme::lightStrength
 *
 * \brief The specular light strength for the whole graph.
 *
 * The value must be between \c 0.0f and \c 10.0f.
 *
 * This value affects the light specified in Q3DScene.
 */
void Q3DTheme::setLightStrength(float strength)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.lightStrengthDirty = true;
    if (strength < 0.0f || strength > 10.0f) {
        qWarning("Invalid value. Valid range for lightStrength is between 0.0f and 10.0f");
    } else if (d->m_lightStrength != strength) {
        d->m_lightStrength = strength;
        emit lightStrengthChanged(strength);
        emit d->needRender();
    }
}

float Q3DTheme::lightStrength() const
{
    const Q_D(Q3DTheme);
    return d->m_lightStrength;
}

/*!
 * \property Q3DTheme::ambientLightStrength
 *
 * \brief The ambient light strength for the whole graph.
 *
 * This value determines how evenly and brightly the colors are shown throughout
 * the graph regardless of the light position.
 *
 * The value must be between \c 0.0f and \c 1.0f.
 */
void Q3DTheme::setAmbientLightStrength(float strength)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.ambientLightStrengthDirty = true;
    if (strength < 0.0f || strength > 1.0f) {
        qWarning("Invalid value. Valid range for ambientLightStrength is between 0.0f and 1.0f");
    } else if (d->m_ambientLightStrength != strength) {
        d->m_ambientLightStrength = strength;
        emit ambientLightStrengthChanged(strength);
        emit d->needRender();
    }
}

float Q3DTheme::ambientLightStrength() const
{
    const Q_D(Q3DTheme);
    return d->m_ambientLightStrength;
}

/*!
 * \property Q3DTheme::highlightLightStrength
 *
 * \brief The specular light strength for selected objects.
 *
 * The value must be between \c 0.0f and \c 10.0f.
 */
void Q3DTheme::setHighlightLightStrength(float strength)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.highlightLightStrengthDirty = true;
    if (strength < 0.0f || strength > 10.0f) {
        qWarning("Invalid value. Valid range for highlightLightStrength is between 0.0f and 10.0f");
    } else if (d->m_highlightLightStrength != strength) {
        d->m_highlightLightStrength = strength;
        emit highlightLightStrengthChanged(strength);
        emit d->needRender();
    }
}

float Q3DTheme::highlightLightStrength() const
{
    const Q_D(Q3DTheme);
    return d->m_highlightLightStrength;
}

/*!
 * \property Q3DTheme::labelBorderEnabled
 *
 * \brief Whether label borders are drawn for labels that have a background.
 *
 * Has no effect if labelBackgroundEnabled is \c false.
 */
void Q3DTheme::setLabelBorderEnabled(bool enabled)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.labelBorderEnabledDirty = true;
    if (d->m_labelBorders != enabled) {
        d->m_labelBorders = enabled;
        emit labelBorderEnabledChanged(enabled);
        emit d->needRender();
    }
}

bool Q3DTheme::isLabelBorderEnabled() const
{
    const Q_D(Q3DTheme);
    return d->m_labelBorders;
}

/*!
 * \property Q3DTheme::font
 *
 * \brief The font to be used for labels.
 */
void Q3DTheme::setFont(const QFont &font)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.fontDirty = true;
    if (d->m_font != font) {
        d->m_font = font;
        emit fontChanged(font);
        emit d->needRender();
    }
}

QFont Q3DTheme::font() const
{
    const Q_D(Q3DTheme);
    return d->m_font;
}

/*!
 * \property Q3DTheme::backgroundEnabled
 *
 * \brief Whether the background is visible.
 *
 * The background is drawn by using the value of backgroundColor.
 */
void Q3DTheme::setBackgroundEnabled(bool enabled)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.backgroundEnabledDirty = true;
    if (d->m_backgoundEnabled != enabled) {
        d->m_backgoundEnabled = enabled;
        emit backgroundEnabledChanged(enabled);
        emit d->needRender();
    }
}

bool Q3DTheme::isBackgroundEnabled() const
{
    const Q_D(Q3DTheme);
    return d->m_backgoundEnabled;
}

/*!
 * \property Q3DTheme::gridEnabled
 *
 * \brief Whether the grid lines are drawn.
 *
 * This value affects all grid lines.
 */
void Q3DTheme::setGridEnabled(bool enabled)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.gridEnabledDirty = true;
    if (d->m_gridEnabled != enabled) {
        d->m_gridEnabled = enabled;
        emit gridEnabledChanged(enabled);
        emit d->needRender();
    }
}

bool Q3DTheme::isGridEnabled() const
{
    const Q_D(Q3DTheme);
    return d->m_gridEnabled;
}

/*!
 * \property Q3DTheme::labelBackgroundEnabled
 *
 *\brief Whether the label is drawn with a color background or with a fully
 * transparent background.
 *
 * The labelBackgroundColor value (including alpha) is used for drawing the
 * background.
 *
 * Labels with a background are drawn to equal sizes per axis based
 * on the longest label, and the text is centered in them. Labels without a
 * background are drawn as is and are left or right aligned based on their
 * position in the graph.
 */
void Q3DTheme::setLabelBackgroundEnabled(bool enabled)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.labelBackgroundEnabledDirty = true;
    if (d->m_labelBackground != enabled) {
        d->m_labelBackground = enabled;
        emit labelBackgroundEnabledChanged(enabled);
        emit d->needRender();
    }
}

bool Q3DTheme::isLabelBackgroundEnabled() const
{
    const Q_D(Q3DTheme);
    return d->m_labelBackground;
}

/*!
 * \property Q3DTheme::colorStyle
 *
 * \brief The style of the graph colors.
 *
 * One of the ColorStyle enum values.
 *
 * This value can be overridden by setting \l{Abstract3DSeries::colorStyle}
 * {colorStyle} explicitly in the series.
 */
void Q3DTheme::setColorStyle(Q3DTheme::ColorStyle style)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.colorStyleDirty = true;
    if (d->m_colorStyle != style) {
        d->m_colorStyle = style;
        emit colorStyleChanged(style);
    }
}

Q3DTheme::ColorStyle Q3DTheme::colorStyle() const
{
    const Q_D(Q3DTheme);
    return d->m_colorStyle;
}

/*!
 * \property Q3DTheme::labelsEnabled
 *
 * \brief Whether labels are drawn at all.
 *
 * If this is \c{false}, all other label properties have no effect.
 */
void Q3DTheme::setLabelsEnabled(bool enabled)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.labelsEnabledDirty = true;
    if (d->m_labelsEnabled != enabled) {
        d->m_labelsEnabled = enabled;
        emit labelsEnabledChanged(enabled);
        emit d->needRender();
    }
}

bool Q3DTheme::isLabelsEnabled() const
{
    const Q_D(Q3DTheme);
    return d->m_labelsEnabled;
}

/*!
 * \property Q3DTheme::type
 *
 * \brief The type of the theme.
 *
 * The type is automatically set when constructing a theme,
 * but can also be changed later. Changing the theme type will change all other
 * properties of the theme to what the predefined theme specifies.
 * Changing the theme type of the active theme of the graph will also reset all
 * attached series to use the new theme.
 */
void Q3DTheme::setType(Q3DTheme::Theme themeType)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.themeIdDirty = true;
    if (d->m_themeId != themeType) {
        d->m_themeId = themeType;
        ThemeManager::setPredefinedPropertiesToTheme(this, themeType);
        emit typeChanged(themeType);
    }
}

Q3DTheme::Theme Q3DTheme::type() const
{
    const Q_D(Q3DTheme);
    return d->m_themeId;
}

/*!
 * \property Q3DTheme::shadowStrength
 *
 * \brief The shadow strength for the whole graph.
 *
 * The higher the number, the darker the shadows will be. The value must be between \c 0.0 and
 * \c 100.0.
 *
 * This value affects the light specified in Q3DScene.
 */
void Q3DTheme::setShadowStrength(float strength)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.shadowStrengthDirty = true;
    if (strength < 0.0f || strength > 100.0f) {
        qWarning("Invalid value. Valid range for shadowStrength is between 0.0f and 100.0f");
    } else if (d->m_shadowStrength != strength) {
        d->m_shadowStrength = strength;
        emit shadowStrengthChanged(strength);
        emit d->needRender();
    }
}

float Q3DTheme::shadowStrength() const
{
    const Q_D(Q3DTheme);
    return d->m_shadowStrength;
}

// Q3DThemePrivate

Q3DThemePrivate::Q3DThemePrivate(Q3DTheme *q)
    : m_themeId(Q3DTheme::ThemeUserDefined),
      m_colorStyle(Q3DTheme::ColorStyleUniform),
      m_backgroundColor(Qt::black),
      m_gridLineColor(Qt::white),
      m_lightColor(Qt::white),
      m_multiHighlightColor(Qt::blue),
      m_singleHighlightColor(Qt::red),
      m_textBackgroundColor(Qt::gray),
      m_textColor(Qt::white),
      m_windowColor(Qt::black),
      m_font(QFont()),
      m_multiHighlightGradient(QLinearGradient(gradientTextureWidth,
                                               gradientTextureHeight,
                                               0.0, 0.0)),
      m_singleHighlightGradient(QLinearGradient(gradientTextureWidth,
                                                gradientTextureHeight,
                                                0.0, 0.0)),
      m_backgoundEnabled(true),
      m_forcePredefinedType(true),
      m_gridEnabled(true),
      m_isDefaultTheme(false),
      m_labelBackground(true),
      m_labelBorders(true),
      m_labelsEnabled(true),
      m_ambientLightStrength(0.25f),
      m_highlightLightStrength(7.5f),
      m_lightStrength(5.0f),
      m_shadowStrength(25.0f),
      q_ptr(q)
{
    m_baseColors.append(QColor(Qt::black));
    m_baseGradients.append(QLinearGradient(gradientTextureWidth,
                                           gradientTextureHeight,
                                           0.0, 0.0));
}

Q3DThemePrivate::~Q3DThemePrivate()
{
}

void Q3DThemePrivate::resetDirtyBits()
{
    m_dirtyBits.ambientLightStrengthDirty = true;
    m_dirtyBits.backgroundColorDirty = true;
    m_dirtyBits.backgroundEnabledDirty = true;
    m_dirtyBits.baseColorDirty = true;
    m_dirtyBits.baseGradientDirty = true;
    m_dirtyBits.colorStyleDirty = true;
    m_dirtyBits.fontDirty = true;
    m_dirtyBits.gridEnabledDirty = true;
    m_dirtyBits.gridLineColorDirty = true;
    m_dirtyBits.highlightLightStrengthDirty = true;
    m_dirtyBits.labelBackgroundColorDirty = true;
    m_dirtyBits.labelBackgroundEnabledDirty = true;
    m_dirtyBits.labelBorderEnabledDirty = true;
    m_dirtyBits.labelTextColorDirty = true;
    m_dirtyBits.lightColorDirty = true;
    m_dirtyBits.lightStrengthDirty = true;
    m_dirtyBits.multiHighlightColorDirty = true;
    m_dirtyBits.multiHighlightGradientDirty = true;
    m_dirtyBits.shadowStrengthDirty = true;
    m_dirtyBits.singleHighlightColorDirty = true;
    m_dirtyBits.singleHighlightGradientDirty = true;
    m_dirtyBits.themeIdDirty = true;
    m_dirtyBits.windowColorDirty = true;
}

bool Q3DThemePrivate::sync(Q3DThemePrivate &other)
{
    bool updateDrawer = false;
    if (m_dirtyBits.ambientLightStrengthDirty) {
        other.q_func()->setAmbientLightStrength(m_ambientLightStrength);
        m_dirtyBits.ambientLightStrengthDirty = false;
    }
    if (m_dirtyBits.backgroundColorDirty) {
        other.q_func()->setBackgroundColor(m_backgroundColor);
        m_dirtyBits.backgroundColorDirty = false;
    }
    if (m_dirtyBits.backgroundEnabledDirty) {
        other.q_func()->setBackgroundEnabled(m_backgoundEnabled);
        m_dirtyBits.backgroundEnabledDirty = false;
    }
    if (m_dirtyBits.baseColorDirty) {
        other.q_func()->setBaseColors(m_baseColors);
        m_dirtyBits.baseColorDirty = false;
    }
    if (m_dirtyBits.baseGradientDirty) {
        other.q_func()->setBaseGradients(m_baseGradients);
        m_dirtyBits.baseGradientDirty = false;
    }
    if (m_dirtyBits.colorStyleDirty) {
        other.q_func()->setColorStyle(m_colorStyle);
        m_dirtyBits.colorStyleDirty = false;
    }
    if (m_dirtyBits.fontDirty) {
        other.q_func()->setFont(m_font);
        m_dirtyBits.fontDirty = false;
        updateDrawer = true;
    }
    if (m_dirtyBits.gridEnabledDirty) {
        other.q_func()->setGridEnabled(m_gridEnabled);
        m_dirtyBits.gridEnabledDirty = false;
    }
    if (m_dirtyBits.gridLineColorDirty) {
        other.q_func()->setGridLineColor(m_gridLineColor);
        m_dirtyBits.gridLineColorDirty = false;
    }
    if (m_dirtyBits.highlightLightStrengthDirty) {
        other.q_func()->setHighlightLightStrength(m_highlightLightStrength);
        m_dirtyBits.highlightLightStrengthDirty = false;
    }
    if (m_dirtyBits.labelBackgroundColorDirty) {
        other.q_func()->setLabelBackgroundColor(m_textBackgroundColor);
        m_dirtyBits.labelBackgroundColorDirty = false;
        updateDrawer = true;
    }
    if (m_dirtyBits.labelBackgroundEnabledDirty) {
        other.q_func()->setLabelBackgroundEnabled(m_labelBackground);
        m_dirtyBits.labelBackgroundEnabledDirty = false;
        updateDrawer = true;
    }
    if (m_dirtyBits.labelBorderEnabledDirty) {
        other.q_func()->setLabelBorderEnabled(m_labelBorders);
        m_dirtyBits.labelBorderEnabledDirty = false;
        updateDrawer = true;
    }
    if (m_dirtyBits.labelTextColorDirty) {
        other.q_func()->setLabelTextColor(m_textColor);
        m_dirtyBits.labelTextColorDirty = false;
        updateDrawer = true;
    }
    if (m_dirtyBits.lightColorDirty) {
        other.q_func()->setLightColor(m_lightColor);
        m_dirtyBits.lightColorDirty = false;
    }
    if (m_dirtyBits.lightStrengthDirty) {
        other.q_func()->setLightStrength(m_lightStrength);
        m_dirtyBits.lightStrengthDirty = false;
    }
    if (m_dirtyBits.multiHighlightColorDirty) {
        other.q_func()->setMultiHighlightColor(m_multiHighlightColor);
        m_dirtyBits.multiHighlightColorDirty = false;
    }
    if (m_dirtyBits.multiHighlightGradientDirty) {
        other.q_func()->setMultiHighlightGradient(m_multiHighlightGradient);
        m_dirtyBits.multiHighlightGradientDirty = false;
    }
    if (m_dirtyBits.shadowStrengthDirty) {
        other.q_func()->setShadowStrength(m_shadowStrength);
        m_dirtyBits.shadowStrengthDirty = false;
    }
    if (m_dirtyBits.singleHighlightColorDirty) {
        other.q_func()->setSingleHighlightColor(m_singleHighlightColor);
        m_dirtyBits.singleHighlightColorDirty = false;
    }
    if (m_dirtyBits.singleHighlightGradientDirty) {
        other.q_func()->setSingleHighlightGradient(m_singleHighlightGradient);
        m_dirtyBits.singleHighlightGradientDirty = false;
    }
    if (m_dirtyBits.themeIdDirty) {
        other.m_themeId = m_themeId; // Set directly to avoid a call to ThemeManager's useTheme()
        m_dirtyBits.themeIdDirty = false;
    }
    if (m_dirtyBits.windowColorDirty) {
        other.q_func()->setWindowColor(m_windowColor);
        m_dirtyBits.windowColorDirty = false;
    }

    return updateDrawer;
}

QT_END_NAMESPACE
