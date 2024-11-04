// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuick/private/qquickrectangle_p.h>
#include "../qml/qquickgraphscolor_p.h"
#include "QtQml/qjsengine.h"
#include "q3dtheme_p.h"
#include "thememanager_p.h"
#include "utils_p.h"

QT_BEGIN_NAMESPACE

/*!
 * \class Q3DTheme
 * \inmodule QtGraphs
 * \ingroup graphs_3D
 * \brief Q3DTheme class provides a visual style for graphs.
 *
 * Specifies visual properties that affect the whole graph. There are several
 * built-in themes that can be used as is or modified freely.
 *
 * The following properties can be overridden by using QAbstract3DSeries
 * properties to set them explicitly in the series: baseColors, baseGradients,
 * and colorStyle.
 *
 * Themes can be created from scratch using the UserDefined enum value.
 * Creating a theme using the default constructor produces a new user-defined
 * theme.
 *
 * \section1 Default Theme
 *
 * The following table lists the properties controlled by themes and the
 * default values for UserDefined.
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
 *     \li Uniform
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
 * Creating a built-in theme and modifying some properties after it has been
 * set:
 *
 * \snippet doc_src_q3dtheme.cpp 3
 *
 */

/*!
 * \enum Q3DTheme::ColorStyle
 *
 * Color styles.
 *
 * \value Uniform
 *        Objects are rendered in a single color. The color used is specified in baseColors,
 *        singleHighlightColor and multiHighlightColor properties.
 * \value ObjectGradient
 *        Objects are colored using a full gradient for each object regardless of object height. The
 *        gradient used is specified in baseGradients, singleHighlightGradient and
 *        multiHighlightGradient properties.
 * \value RangeGradient
 *        Objects are colored using a portion of the full gradient determined by the object's
 *        height and its position on the Y-axis. The gradient used is specified in baseGradients,
 *        singleHighlightGradient and multiHighlightGradient properties.
 */

/*!
 * \enum Q3DTheme::Theme
 *
 * Built-in themes.
 *
 * \value Qt
 *        A light theme with green as the base color.
 * \value PrimaryColors
 *        A light theme with yellow as the base color.
 * \value StoneMoss
 *        A medium dark theme with yellow as the base color.
 * \value ArmyBlue
 *        A medium light theme with blue as the base color.
 * \value Retro
 *        A medium light theme with brown as the base color.
 * \value Ebony
 *        A dark theme with white as the base color.
 * \value Isabelle
 *        A dark theme with yellow as the base color.
 * \value UserDefined
 *        A user-defined theme. For more information, see \l {Default Theme}.
 */

/*!
 * \qmltype Theme3D
 * \inqmlmodule QtGraphs
 * \ingroup graphs_qml_3D
 * \instantiates Q3DTheme
 * \brief A visual style for graphs.
 *
 * This type is used to specify visual properties that affect the whole graph.
 * There are several built-in themes that can be used as is or modified freely.
 *
 * The following properties can be overridden by using Abstract3DSeries
 * properties to set them explicitly in the series:
 * baseColors, baseGradients, and colorStyle.
 *
 * Themes can be created from scratch by using the
 * \l{Q3DTheme::Theme::UserDefined}{Theme3D.Theme.UserDefined} enum value.
 *
 * \section1 Default Theme
 *
 * The following table lists the properties controlled by themes and the
 * default values for \l{Q3DTheme::Theme::UserDefined}
 * {Theme3D.Theme.UserDefined}.
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
 *     \li Uniform
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
 * \qmlproperty list<Color> Theme3D::baseColors
 *
 * The list of base colors to be used for all the objects in the graph, series
 * by series. If there are more series than colors, color list wraps and starts
 * again with the first color in the list. Has no immediate effect if colorStyle
 * is not \c Theme3D.ColorStyle.Uniform.
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
 * The color of the label backgrounds. Has no effect if labelBackgroundEnabled
 * is \c false.
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
 * \qmlproperty list<Gradient> Theme3D::baseGradients
 *
 * The list of base gradients to be used for all the objects in the graph,
 * series by series. If there are more series than gradients, the gradient list
 * wraps and starts again with the first gradient in the list.
 *
 * Has no immediate effect if colorStyle is \l{Q3DTheme::ColorStyle::Uniform}
 * {Theme3D.ColorStyle.Uniform}.
 *
 * This value can be overridden by setting \l{Abstract3DSeries::baseGradient}
 *{Abstract3DSeries.baseGradient} explicitly in the series.
 */

/*!
 * \qmlproperty Gradient Theme3D::singleHighlightGradient
 *
 * The highlight gradient for a selected object. Used if
 * \l{AbstractGraph3D::selectionMode}{selectionMode}
 * has the \c AbstractGraph3D.SelectionItem flag set.
 */

/*!
 * \qmlproperty Gradient Theme3D::multiHighlightGradient
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
 * Defines whether labels are drawn at all. If this is \c{false}, all other
 * label properties have no effect.
 */

/*!
 * \qmlproperty Theme3D.Theme Theme3D::type
 *
 * The type of the theme. If no type is set, the type is
 * \l{Q3DTheme::Theme::UserDefined}{Theme3D.Theme.UserDefined}.
 * Changing the theme type after the item has been constructed will change all
 * other properties of the theme to what the predefined theme specifies.
 * Changing the theme type of the active theme of the graph will also reset all
 * attached series to use the new theme.
 */

/*!
 * \qmlproperty real Theme3D::shadowStrength
 *
 * The shadow strength for the whole graph. The higher the number, the darker
 * the shadows will be. The value must be between \c 0.0 and \c 100.0.
 *
 * This value affects the light specified in Scene3D.
 */

/*!
 * Constructs a new theme of type UserDefined. An optional \a parent parameter
 * can be given and is then passed to QObject constructor.
 */
Q3DTheme::Q3DTheme(QObject *parent)
    : QObject(*(new Q3DThemePrivate()), parent)
{}

/*!
 * Constructs a new theme with \a themeType, which can be one of the built-in
 * themes from \l Theme. An optional \a parent parameter can be given and is
 * then passed to QObject constructor.
 */
Q3DTheme::Q3DTheme(Theme themeType, QObject *parent)
    : QObject(*(new Q3DThemePrivate()), parent)

{
    setType(themeType);
}

/*!
 * \internal
 */
Q3DTheme::Q3DTheme(Q3DThemePrivate &d, Theme themeType, QObject *parent)
    : QObject(d, parent)
{
    setType(themeType);
}

/*!
 * Destroys the theme.
 */
Q3DTheme::~Q3DTheme() {}

/*!
 * \property Q3DTheme::baseColors
 *
 * \brief The list of base colors to be used for all the objects in the graph,
 * series by series.
 *
 * If there are more series than colors, the color list wraps and starts again
 * with the first color in the list.
 *
 * Has no immediate effect if colorStyle is not Uniform.
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
        emit needRender();
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
        emit needRender();
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
        emit needRender();
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
        emit needRender();
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
        emit needRender();
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
        emit needRender();
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
 * Has no immediate effect if colorStyle is Uniform.
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
        qWarning("Invalid value. Valid range for lightStrength is between 0.0f and "
                 "10.0f");
    } else if (d->m_lightStrength != strength) {
        d->m_lightStrength = strength;
        emit lightStrengthChanged(strength);
        emit needRender();
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
        qWarning("Invalid value. Valid range for ambientLightStrength is between "
                 "0.0f and 1.0f");
    } else if (d->m_ambientLightStrength != strength) {
        d->m_ambientLightStrength = strength;
        emit ambientLightStrengthChanged(strength);
        emit needRender();
    }
}

float Q3DTheme::ambientLightStrength() const
{
    const Q_D(Q3DTheme);
    return d->m_ambientLightStrength;
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
        emit needRender();
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
        emit needRender();
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
        emit needRender();
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
        emit needRender();
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
        emit needRender();
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
        emit needRender();
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
 * The higher the number, the darker the shadows will be. The value must be
 * between \c 0.0 and \c 100.0.
 *
 * This value affects the light specified in Q3DScene.
 */
void Q3DTheme::setShadowStrength(float strength)
{
    Q_D(Q3DTheme);
    d->m_dirtyBits.shadowStrengthDirty = true;
    if (strength < 0.0f || strength > 100.0f) {
        qWarning("Invalid value. Valid range for shadowStrength is between 0.0f "
                 "and 100.0f");
    } else if (d->m_shadowStrength != strength) {
        d->m_shadowStrength = strength;
        emit shadowStrengthChanged(strength);
        emit needRender();
    }
}

float Q3DTheme::shadowStrength() const
{
    const Q_D(Q3DTheme);
    return d->m_shadowStrength;
}

/*
 * \internal
 */
QQmlListProperty<QObject> Q3DTheme::themeChildren()
{
    return QQmlListProperty<QObject>(this, this, &Q3DTheme::appendThemeChildren, 0, 0, 0);
}

/*
 * \internal
 */
void Q3DTheme::appendThemeChildren(QQmlListProperty<QObject> *list, QObject *element)
{
    Q_UNUSED(list);
    Q_UNUSED(element);
    // Nothing to do, themeChildren is there only to enable scoping gradient items
    // in Theme3D item.
}

/*
 * \internal
 */
void Q3DTheme::setSingleHighlightGradient(QJSValue gradient)
{
    Q_D(Q3DTheme);
    // connect new / disconnect old
    if (gradient.isQObject() && !gradient.equals(d->m_singleHLGradient)) {
        auto quickGradient = qobject_cast<QQuickGradient *>(d->m_singleHLGradient.toQObject());
        if (quickGradient)
            QObject::disconnect(quickGradient, 0, this, 0);

        d->m_singleHLGradient = gradient;

        const int signalIndex = QMetaMethod::fromSignal(&QQuickGradient::updated).methodIndex();

        if (quickGradient) {
            QMetaObject::connect(quickGradient,
                                 signalIndex,
                                 this,
                                 this->metaObject()->indexOfSlot(
                                     "handleSingleHighlightGradientUpdate()"));
        }

        emit singleHighlightGradientQMLChanged(d->m_singleHLGradient);
    }

    if (!d->m_singleHLGradient.isNull())
        d->setThemeGradient(d->m_singleHLGradient, Q3DThemePrivate::GradientType::SingleHL);
}

/*
 * \internal
 */
QJSValue Q3DTheme::singleHighlightGradientQML() const
{
    const Q_D(Q3DTheme);
    return d->m_singleHLGradient;
}

/*
 * \internal
 */
void Q3DTheme::setMultiHighlightGradient(QJSValue gradient)
{
    Q_D(Q3DTheme);
    // connect new / disconnect old
    if (gradient.isQObject() && !gradient.equals(d->m_multiHLGradient)) {
        auto quickGradient = qobject_cast<QQuickGradient *>(d->m_multiHLGradient.toQObject());
        if (quickGradient)
            QObject::disconnect(quickGradient, 0, this, 0);

        d->m_multiHLGradient = gradient;

        const int signalIndex = QMetaMethod::fromSignal(&QQuickGradient::updated).methodIndex();

        if (quickGradient) {
            QMetaObject::connect(quickGradient,
                                 signalIndex,
                                 this,
                                 this->metaObject()->indexOfSlot(
                                     "handleMultiHighlightGradientUpdate()"));
        }

        emit multiHighlightGradientChangedQML(d->m_multiHLGradient);
    }

    if (!d->m_multiHLGradient.isNull())
        d->setThemeGradient(d->m_multiHLGradient, Q3DThemePrivate::GradientType::MultiHL);
}

/*
 * \internal
 */
QJSValue Q3DTheme::multiHighlightGradientQML() const
{
    const Q_D(Q3DTheme);
    return d->m_multiHLGradient;
}

/*
 * \internal
 */
void Q3DTheme::classBegin()
{
    Q_D(Q3DTheme);
    // Turn off predefined type forcing for the duration of initial class
    // construction so that predefined type customization can be done.
    d->setForcePredefinedType(false);
}

/*
 * \internal
 */
void Q3DTheme::componentComplete()
{
    Q_D(Q3DTheme);
    d->setForcePredefinedType(true);
}

/*
 * \internal
 */
QQmlListProperty<QQuickGraphsColor> Q3DTheme::baseColorsQML()
{
    return QQmlListProperty<QQuickGraphsColor>(this,
                                               this,
                                               &Q3DTheme::appendBaseColorsFunc,
                                               &Q3DTheme::countBaseColorsFunc,
                                               &Q3DTheme::atBaseColorsFunc,
                                               &Q3DTheme::clearBaseColorsFunc);
}

/*
 * \internal
 */
void Q3DTheme::appendBaseColorsFunc(QQmlListProperty<QQuickGraphsColor> *list,
                                    QQuickGraphsColor *color)
{
    reinterpret_cast<Q3DTheme *>(list->data)->addColor(color);
}

/*
 * \internal
 */
qsizetype Q3DTheme::countBaseColorsFunc(QQmlListProperty<QQuickGraphsColor> *list)
{
    return reinterpret_cast<Q3DTheme *>(list->data)->colorList().size();
}

/*
 * \internal
 */
QQuickGraphsColor *Q3DTheme::atBaseColorsFunc(QQmlListProperty<QQuickGraphsColor> *list,
                                              qsizetype index)
{
    return reinterpret_cast<Q3DTheme *>(list->data)->colorList().at(index);
}

/*
 * \internal
 */
void Q3DTheme::clearBaseColorsFunc(QQmlListProperty<QQuickGraphsColor> *list)
{
    reinterpret_cast<Q3DTheme *>(list->data)->clearColors();
}

/*
 * \internal
 */
QQmlListProperty<QObject> Q3DTheme::baseGradientsQML()
{
    return QQmlListProperty<QObject>(this,
                                     this,
                                     &Q3DTheme::appendBaseGradientsFunc,
                                     &Q3DTheme::countBaseGradientsFunc,
                                     &Q3DTheme::atBaseGradientsFunc,
                                     &Q3DTheme::clearBaseGradientsFunc);
}

/*
 * \internal
 */
void Q3DTheme::appendBaseGradientsFunc(QQmlListProperty<QObject> *list, QObject *gradient)
{
    QJSEngine engine;
    QJSValue value = engine.newQObject(gradient);
    reinterpret_cast<Q3DTheme *>(list->data)->addGradient(value);
}

/*
 * \internal
 */
qsizetype Q3DTheme::countBaseGradientsFunc(QQmlListProperty<QObject> *list)
{
    return reinterpret_cast<Q3DTheme *>(list->data)->gradientList().size();
}

/*
 * \internal
 */
QObject *Q3DTheme::atBaseGradientsFunc(QQmlListProperty<QObject> *list, qsizetype index)
{
    return reinterpret_cast<Q3DTheme *>(list->data)->gradientList().at(index);
}

/*
 * \internal
 */
void Q3DTheme::clearBaseGradientsFunc(QQmlListProperty<QObject> *list)
{
    reinterpret_cast<Q3DTheme *>(list->data)->clearGradients();
}

/*
 * \internal
 */
void Q3DTheme::addColor(QQuickGraphsColor *color)
{
    Q_D(Q3DTheme);
    if (!color) {
        qWarning("Color is invalid, use Color");
        return;
    }
    d->clearDummyColors();
    d->m_colors.append(color);
    connect(color, &QQuickGraphsColor::colorChanged, this, &Q3DTheme::handleBaseColorUpdate);
    QList<QColor> list = baseColors();
    list.append(color->color());
    setBaseColors(list);
}

/*
 * \internal
 */
QList<QQuickGraphsColor *> Q3DTheme::colorList()
{
    Q_D(Q3DTheme);
    if (d->m_colors.isEmpty()) {
        // Create dummy Colors from theme's colors
        d->m_dummyColors = true;
        QList<QColor> list = baseColors();
        for (const QColor &item : list) {
            QQuickGraphsColor *color = new QQuickGraphsColor(this);
            color->setColor(item);
            d->m_colors.append(color);
            connect(color, &QQuickGraphsColor::colorChanged, this, &Q3DTheme::handleBaseColorUpdate);
        }
    }
    return d->m_colors;
}

/*
 * \internal
 */
void Q3DTheme::clearColors()
{
    Q_D(Q3DTheme);
    d->clearDummyColors();
    for (QQuickGraphsColor *item : d->m_colors)
        disconnect(item, 0, this, 0);
    d->m_colors.clear();
    setBaseColors(QList<QColor>());
}

/*
 * \internal
 */
void Q3DThemePrivate::clearDummyColors()
{
    if (m_dummyColors) {
        for (QQuickGraphsColor *item : m_colors)
            delete item;
        m_colors.clear();
        m_dummyColors = false;
    }
}

/*
 * \internal
 */
void Q3DTheme::addGradient(QJSValue gradient)
{
    Q_D(Q3DTheme);
    auto quickGradient = qobject_cast<QQuickGradient *>(gradient.toQObject());
    d->m_gradients.append(quickGradient);

    const int updatedIndex = QMetaMethod::fromSignal(&QQuickGradient::updated).methodIndex();

    QMetaObject::connect(quickGradient,
                         updatedIndex,
                         this,
                         this->metaObject()->indexOfSlot("handleBaseGradientUpdate()"));

    QList<QLinearGradient> list = baseGradients();
    list.append(d->convertGradient(gradient));
    setBaseGradients(list);
}

/*
 * \internal
 */
QList<QQuickGradient *> Q3DTheme::gradientList()
{
    Q_D(Q3DTheme);
    return d->m_gradients;
}

void Q3DTheme::clearGradients()
{
    Q_D(Q3DTheme);
    d->m_gradients.clear();
    setBaseGradients(QList<QLinearGradient>());
}

// Q3DThemePrivate

Q3DThemePrivate::Q3DThemePrivate()
    : m_themeId(Q3DTheme::Theme::UserDefined)
    , m_colorStyle(Q3DTheme::ColorStyle::Uniform)
    , m_backgroundColor(Qt::black)
    , m_gridLineColor(Qt::white)
    , m_lightColor(Qt::white)
    , m_multiHighlightColor(Qt::blue)
    , m_singleHighlightColor(Qt::red)
    , m_textBackgroundColor(Qt::gray)
    , m_textColor(Qt::white)
    , m_windowColor(Qt::black)
    , m_font(QFont())
    , m_multiHighlightGradient(
          QLinearGradient(gradientTextureWidth, gradientTextureHeight, 0.0, 0.0))
    , m_singleHighlightGradient(
          QLinearGradient(gradientTextureWidth, gradientTextureHeight, 0.0, 0.0))
    , m_backgoundEnabled(true)
    , m_forcePredefinedType(true)
    , m_gridEnabled(true)
    , m_isDefaultTheme(false)
    , m_labelBackground(true)
    , m_labelBorders(true)
    , m_labelsEnabled(true)
    , m_ambientLightStrength(0.25f)
    , m_lightStrength(5.0f)
    , m_shadowStrength(25.0f)
    , m_colors(QList<QQuickGraphsColor *>())
    , m_gradients(QList<QQuickGradient *>())
    , m_singleHLGradient(QJSValue(0))
    , m_multiHLGradient(QJSValue(0))
    , m_dummyColors(false)
{
    m_baseColors.append(QColor(Qt::black));
    m_baseGradients.append(QLinearGradient(gradientTextureWidth, gradientTextureHeight, 0.0, 0.0));
}

Q3DThemePrivate::~Q3DThemePrivate() {}

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

void Q3DThemePrivate::setThemeGradient(QJSValue gradient, GradientType type)
{
    Q_Q(Q3DTheme);
    QLinearGradient linearGradient = convertGradient(gradient);

    switch (type) {
    case GradientType::SingleHL:
        q->setSingleHighlightGradient(linearGradient);
        break;
    case GradientType::MultiHL:
        q->setMultiHighlightGradient(linearGradient);
        break;
    default:
        qWarning("Incorrect usage. Type may be GradientType::SingleHL or "
                 "GradientType::MultiHL.");
        break;
    }
}

QLinearGradient Q3DThemePrivate::convertGradient(QJSValue gradient)
{
    // Create QLinearGradient out of QJSValue
    QLinearGradient newGradient;
    if (gradient.isQObject()) {
        auto quickGradient = qobject_cast<QQuickGradient *>(gradient.toQObject());
        newGradient.setStops(quickGradient->gradientStops());
    }
    return newGradient;
}

void Q3DTheme::handleTypeChange(Q3DTheme::Theme themeType)
{
    Q_UNUSED(themeType);
    Q_D(Q3DTheme);
    // Theme changed, disconnect base color/gradient connections
    if (!d->m_colors.isEmpty()) {
        for (QQuickGraphsColor *item : d->m_colors)
            disconnect(item, 0, this, 0);
        d->m_colors.clear();
    }
    if (!d->m_gradients.isEmpty()) {
        for (auto item : d->m_gradients)
            disconnect(item, 0, this, 0);
        d->m_gradients.clear();
    }
}

void Q3DTheme::handleBaseColorUpdate()
{
    Q_D(Q3DTheme);
    int colorCount = d->m_colors.size();
    int changed = 0;
    // Check which one changed
    QQuickGraphsColor *color = qobject_cast<QQuickGraphsColor *>(QObject::sender());
    for (int i = 0; i < colorCount; i++) {
        if (color == d->m_colors.at(i)) {
            changed = i;
            break;
        }
    }
    // Update the changed one from the list
    QList<QColor> list = baseColors();
    list[changed] = d->m_colors.at(changed)->color();
    // Set the changed list
    setBaseColors(list);
}

void Q3DTheme::handleBaseGradientUpdate()
{
    Q_D(Q3DTheme);
    // Find out which gradient has changed, and update the list with it
    int gradientCount = d->m_gradients.size();
    int changed = 0;

    // Check which one changed
    QQuickGradient *newGradient = qobject_cast<QQuickGradient *>(QObject::sender());
    QJSEngine engine;
    QJSValue updatedGradient = engine.newQObject(newGradient);

    for (int i = 0; i < gradientCount; ++i) {
        if (newGradient == d->m_gradients.at(i)) {
            changed = i;
            break;
        }
    }

    // Update the changed one from the list
    QList<QLinearGradient> list = baseGradients();
    list[changed] = d->convertGradient(updatedGradient);

    // Set the changed list
    setBaseGradients(list);
}

void Q3DTheme::handleSingleHLGradientUpdate()
{
    Q_D(Q3DTheme);
    if (!d->m_singleHLGradient.isNull())
        d->setThemeGradient(d->m_singleHLGradient, Q3DThemePrivate::GradientType::SingleHL);
}

void Q3DTheme::handleMultiHLGradientUpdate()
{
    Q_D(Q3DTheme);
    if (!d->m_multiHLGradient.isNull())
        d->setThemeGradient(d->m_multiHLGradient, Q3DThemePrivate::GradientType::MultiHL);
}

QT_END_NAMESPACE
