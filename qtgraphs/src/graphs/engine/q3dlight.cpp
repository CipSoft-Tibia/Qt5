// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "q3dlight_p.h"

QT_BEGIN_NAMESPACE

/*!
 * \class Q3DLight
 * \inmodule QtGraphs
 * \brief Representation of a light source in 3D space.
 *
 * Q3DLight represents a monochrome light source in 3D space.
 *
 * \note Default light has isAutoPosition() \c true.
 */

/*!
 * \qmltype Light3D
 * \inqmlmodule QtGraphs
 * \ingroup graphs_qml
 * \instantiates Q3DLight
 * \brief Representation of a light source in 3D space.
 *
 * Light3D represents a monochrome light source in 3D space.
 *
 * \note Default light has autoPosition \c true.
 */

/*!
 * \qmlproperty bool Light3D::autoPosition
 * Defines whether the light position follows the camera automatically.
 * \note Has no effect if shadows are enabled. Remember to disable shadows before setting light's
 * position, or it will be overwritten by automatic positioning if this
 * property is \c false.
 */

/*!
 * Constructs a new 3D light located at origin. An optional \a parent parameter can be given
 * and is then passed to QObject constructor.
 */
Q3DLight::Q3DLight(QObject *parent) :
    Q3DObject(new Q3DLightPrivate(this), parent)
{
}

/*!
 * Destroys the light object.
 */
Q3DLight::~Q3DLight()
{
}

/*!
 * \property Q3DLight::autoPosition
 * \brief Whether the light position follows the camera automatically.
 * \note Has no effect if shadows are enabled. Remember to disable shadows before setting light's
 * position, or it will be overwritten by automatic positioning if
 * \c isAutoPosition() is \c false.
 */
void Q3DLight::setAutoPosition(bool enabled)
{
    Q_D(Q3DLight);
    if (enabled != d->m_automaticLight) {
        d->m_automaticLight = enabled;
        setDirty(true);
        emit autoPositionChanged(enabled);
    }
}

bool Q3DLight::isAutoPosition()
{
    const Q_D(Q3DLight);
    return d->m_automaticLight;
}

Q3DLightPrivate::Q3DLightPrivate(Q3DLight *q) :
    Q3DObjectPrivate(q),
    m_automaticLight(false)
{
}

Q3DLightPrivate::~Q3DLightPrivate()
{
}

void Q3DLightPrivate::sync(Q3DLight &other)
{
    Q_Q(Q3DLight);
    if (q->isDirty()) {
        other.setPosition(q->position());
        other.setAutoPosition(q->isAutoPosition());
        q->setDirty(false);
    }
}

QT_END_NAMESPACE
