// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qcustom3dlabel_p.h"
#include "utils_p.h"

QT_BEGIN_NAMESPACE

/*!
 * \class QCustom3DLabel
 * \inmodule QtGraphs
 * \brief The QCustom3DLabel class adds a custom label to a graph.
 *
 * The text, font, position, scaling, rotation, and colors of a custom label can
 * be set. In addition, the visibility of the borders and background of the
 * label can be toggled. Colors, borders, and background are determined by the
 * active theme unless set explicitly.
 *
 * \note In scaling, the z-coordinate has no effect. Setting the same x- and
 * y-coordinates retains the original font dimensions.
 *
 * \sa QAbstract3DGraph::addCustomItem()
 */

/*!
 * \qmltype Custom3DLabel
 * \inqmlmodule QtGraphs
 * \ingroup graphs_qml
 * \instantiates QCustom3DLabel
 * \inherits Custom3DItem
 * \brief Adds a custom label to a graph.
 *
 * The text, font, position, scaling, rotation, and colors of a custom label can
 * be set. In addition, the visibility of the borders and background of the
 * label can be toggled. Colors, borders, and background are determined by the
 * active theme unless set explicitly.
 *
 * \note In scaling, the z-coordinate has no effect. Setting the same x- and
 * y-coordinates retains the original font dimensions.
 */

/*! \qmlproperty string Custom3DLabel::text
 *
 * The text for the label. Rich text is not supported.
 */

/*! \qmlproperty font Custom3DLabel::font
 *
 * The font to be used for the label. Defaults to \c{Font {family: "Arial"; pointSize: 20}}.
 * Special formatting (for example, outlined) is not supported.
 */

/*! \qmlproperty color Custom3DLabel::textColor
 *
 * The color for the label text. Also affects label border, if enabled. Defaults to \c{"white"}.
 *
 * \sa borderEnabled
 */

/*! \qmlproperty color Custom3DLabel::backgroundColor
 *
 * The color for the label background, if enabled. Defaults to \c{"gray"}.
 *
 * \sa backgroundEnabled
 */

/*! \qmlproperty bool Custom3DLabel::backgroundEnabled
 *
 * Defines whether the label background is enabled. If set to \c{false},
 * backgroundColor has no effect. Defaults to \c{true}.
 */

/*! \qmlproperty bool Custom3DLabel::borderEnabled
 *
 * Defines whether label borders are enabled. Defaults to \c{true}.
 */

/*! \qmlproperty bool Custom3DLabel::facingCamera
 *
 * Defines whether the label will always face the camera. Defaults to \c{false}.
 * If set to \c{true}, \l {QCustom3DItem::}{rotation} has no effect.
 */

/*!
 * Constructs a custom 3D label with the given \a parent.
 */
QCustom3DLabel::QCustom3DLabel(QObject *parent) :
    QCustom3DItem(new QCustom3DLabelPrivate(this), parent)
{
}

/*!
 * Constructs a custom 3D label with the given \a text, \a font, \a position, \a scaling,
 * \a rotation, and optional \a parent.
 *
 * \note Setting the same x- and y-coordinates for \a scaling retains the
 * original font dimensions.
 */
QCustom3DLabel::QCustom3DLabel(const QString &text, const QFont &font,
                               const QVector3D &position, const QVector3D &scaling,
                               const QQuaternion &rotation, QObject *parent) :
    QCustom3DItem(new QCustom3DLabelPrivate(this, text, font, position, scaling, rotation),
                  parent)
{
}

/*!
 * Deletes the custom 3D label.
 */
QCustom3DLabel::~QCustom3DLabel()
{
}

/*! \property QCustom3DLabel::text
 *
 * \brief The text for the label.
 *
 * Rich text is not supported.
 */
void QCustom3DLabel::setText(const QString &text)
{
    Q_D(QCustom3DLabel);
    if (d->m_text != text) {
        d->m_text = text;
        emit textChanged(text);
        emit needUpdate();
    }
}

QString QCustom3DLabel::text() const
{
    const Q_D(QCustom3DLabel);
    return d->m_text;
}

/*! \property QCustom3DLabel::font
 *
 * \brief The font to be used for the label.
 *
 * Defaults to \c{QFont("Arial", 20)}. Special formatting
 * (for example, outlined) is not supported.
 */
void QCustom3DLabel::setFont(const QFont &font)
{
    Q_D(QCustom3DLabel);
    if (d->m_font != font) {
        d->m_font = font;
        emit fontChanged(font);
        emit needUpdate();
    }
}

QFont QCustom3DLabel::font() const
{
    const Q_D(QCustom3DLabel);
    return d->m_font;
}

/*! \property QCustom3DLabel::textColor
 *
 * \brief The color for the label text.
 *
 * Also affects the label border, if enabled. Defaults to \c{Qt::white}.
 *
 * \sa borderEnabled
 */
void QCustom3DLabel::setTextColor(const QColor &color)
{
    Q_D(QCustom3DLabel);
    if (d->m_txtColor != color) {
        d->m_txtColor = color;
        d->m_customVisuals = true;
        emit textColorChanged(color);
        emit needUpdate();
    }
}

QColor QCustom3DLabel::textColor() const
{
    const Q_D(QCustom3DLabel);
    return d->m_txtColor;
}

/*! \property QCustom3DLabel::backgroundColor
 *
 * \brief The color for the label background, if enabled.
 *
 * Defaults to \c{Qt::gray}.
 *
 * \sa backgroundEnabled
 */
void QCustom3DLabel::setBackgroundColor(const QColor &color)
{
    Q_D(QCustom3DLabel);
    if (d->m_bgrColor != color) {
        d->m_bgrColor = color;
        d->m_customVisuals = true;
        emit backgroundColorChanged(color);
        emit needUpdate();
    }
}

QColor QCustom3DLabel::backgroundColor() const
{
    const Q_D(QCustom3DLabel);
    return d->m_bgrColor;
}

/*! \property QCustom3DLabel::borderEnabled
 *
 * \brief Whether label borders are enabled.
 *
 * Defaults to \c{true}.
 */
void QCustom3DLabel::setBorderEnabled(bool enabled)
{
    Q_D(QCustom3DLabel);
    if (d->m_borders != enabled) {
        d->m_borders = enabled;
        d->m_customVisuals = true;
        emit borderEnabledChanged(enabled);
        emit needUpdate();
    }
}

bool QCustom3DLabel::isBorderEnabled() const
{
    const Q_D(QCustom3DLabel);
    return d->m_borders;
}

/*! \property QCustom3DLabel::backgroundEnabled
 *
 * \brief Whether the label background is enabled.
 *
 * If set to \c{false}, backgroundColor() has no effect. Defaults
 * to \c{true}.
 */
void QCustom3DLabel::setBackgroundEnabled(bool enabled)
{
    Q_D(QCustom3DLabel);
    if (d->m_background != enabled) {
        d->m_background = enabled;
        d->m_customVisuals = true;
        emit backgroundEnabledChanged(enabled);
        emit needUpdate();
    }
}

bool QCustom3DLabel::isBackgroundEnabled() const
{
    const Q_D(QCustom3DLabel);
    return d->m_background;
}

/*! \property QCustom3DLabel::facingCamera
 *
 * \brief Whether the label will always face the camera.
 *
 * Defaults to \c{false}. If set to \c{true}, rotation()
 * has no effect.
 */
void QCustom3DLabel::setFacingCamera(bool enabled)
{
    Q_D(QCustom3DLabel);
    if (d->m_facingCamera != enabled) {
        d->m_facingCamera = enabled;
        d->m_facingCameraDirty = true;
        emit facingCameraChanged(enabled);
        emit needUpdate();
    }
}

bool QCustom3DLabel::isFacingCamera() const
{
    const Q_D(QCustom3DLabel);
    return d->m_facingCamera;
}

QCustom3DLabelPrivate::QCustom3DLabelPrivate(QCustom3DLabel *q) :
    QCustom3DItemPrivate(q),
    m_font(QFont(QStringLiteral("Arial"), 20)),
    m_bgrColor(Qt::gray),
    m_txtColor(Qt::white),
    m_background(true),
    m_borders(true),
    m_facingCamera(false),
    m_customVisuals(false),
    m_facingCameraDirty(false)
{
    m_isLabelItem = true;
    m_shadowCasting = false;
    m_meshFile = QStringLiteral(":/defaultMeshes/plane");
}

QCustom3DLabelPrivate::QCustom3DLabelPrivate(QCustom3DLabel *q, const QString &text,
                                             const QFont &font, const QVector3D &position,
                                             const QVector3D &scaling,
                                             const QQuaternion &rotation) :
    QCustom3DItemPrivate(q, QStringLiteral(":/defaultMeshes/plane"), position, scaling, rotation),
    m_text(text),
    m_font(font),
    m_bgrColor(Qt::gray),
    m_txtColor(Qt::white),
    m_background(true),
    m_borders(true),
    m_facingCamera(false),
    m_customVisuals(false),
    m_facingCameraDirty(false)
{
    m_isLabelItem = true;
    m_shadowCasting = false;
}

QCustom3DLabelPrivate::~QCustom3DLabelPrivate()
{
}

void QCustom3DLabelPrivate::resetDirtyBits()
{
    QCustom3DItemPrivate::resetDirtyBits();
    m_facingCameraDirty = false;
}

QT_END_NAMESPACE
