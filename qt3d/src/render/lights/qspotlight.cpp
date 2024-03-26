// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qspotlight.h"
#include "qspotlight_p.h"
#include "shaderdata_p.h"

QT_BEGIN_NAMESPACE

namespace Qt3DRender {


/*
  Expected Shader struct

  \code

  struct SpotLight
  {
   vec3 position;
   vec3 localDirection;
   vec4 color;
   float intensity;
   float cutOffAngle;
  };

  uniform SpotLight spotLights[10];

  \endcode
 */

QSpotLightPrivate::QSpotLightPrivate()
    : QAbstractLightPrivate(QAbstractLight::SpotLight)
{
    m_shaderData->setProperty("constantAttenuation", 1.0f);
    m_shaderData->setProperty("linearAttenuation", 0.0f);
    m_shaderData->setProperty("quadraticAttenuation", 0.0f);
    m_shaderData->setProperty("direction", QVector3D(0.0f, -1.0f, 0.0f));
    m_shaderData->setProperty("directionTransformed", Render::ShaderData::ModelToWorldDirection);
    m_shaderData->setProperty("cutOffAngle", 45.0f);
}

/*!
  \class Qt3DRender::QSpotLight
  \inmodule Qt3DRender
  \since 5.5
  \brief Encapsulate a Spot Light object in a Qt 3D scene.

    A spotlight is a light source that emits a cone of light in a particular direction.

    A spotlight uses three attenuation factors to describe how the intensity of the light
    decreases over distance. These factors are designed to be used together in calcuating total
    attenuation. For the materials in Qt3D Extras the following formula is used, where distance
    is the distance from the light to the surface being rendered:

    \code
    totalAttenuation = 1.0 / (constantAttenuation + (linearAttenuation * distance) + (quadraticAttenuation * distance * distance));
    \endcode

    Custom materials may choose to interpret these factors differently.
 */

/*!
    \qmltype SpotLight
    \instantiates Qt3DRender::QSpotLight
    \inherits AbstractLight
    \inqmlmodule Qt3D.Render
    \since 5.5
    \brief Encapsulate a Spot Light object in a Qt 3D scene.

    A spotlight is a light source that emits a cone of light in a particular direction.

    A spotlight uses three attenuation factors to describe how the intensity of the light
    decreases over distance. These factors are designed to be used together in calcuating total
    attenuation. For the materials in Qt3D Extras the following formula is used, where distance
    is the distance from the light to the surface being rendered:

    \code
    totalAttenuation = 1.0 / (constantAttenuation + (linearAttenuation * distance) + (quadraticAttenuation * distance * distance));
    \endcode

    Custom materials may choose to interpret these factors differently.
*/

/*!
  \fn Qt3DRender::QSpotLight::QSpotLight(Qt3DCore::QNode *parent)
  Constructs a new QSpotLight with the specified \a parent.
 */
QSpotLight::QSpotLight(QNode *parent)
    : QAbstractLight(*new QSpotLightPrivate, parent)
{
}

/*! \internal */
QSpotLight::~QSpotLight()
{
}

/*! \internal */
QSpotLight::QSpotLight(QSpotLightPrivate &dd, QNode *parent)
    : QAbstractLight(dd, parent)
{
}

/*!
  \qmlproperty float Qt3D.Render::SpotLight::constantAttenuation
    Specifies the constant attenuation of the spot light.

    \note The exact meaning and use of this property is up to the
          material implementation.
*/

/*!
  \property Qt3DRender::QSpotLight::constantAttenuation
    Specifies the constant attenuation of the spot light.

    \note The exact meaning and use of this property is up to the
          material implementation.
 */
float QSpotLight::constantAttenuation() const
{
    Q_D(const QSpotLight);
    return d->m_shaderData->property("constantAttenuation").toFloat();
}

void QSpotLight::setConstantAttenuation(float value)
{
    Q_D(QSpotLight);
    if (constantAttenuation() != value) {
        d->m_shaderData->setProperty("constantAttenuation", value);
        emit constantAttenuationChanged(value);
    }
}

/*!
  \qmlproperty float Qt3D.Render::SpotLight::linearAttenuation
    Specifies the linear attenuation of the spot light.

    \note The exact meaning and use of this property is up to the
          material implementation.
*/

/*!
  \property Qt3DRender::QSpotLight::linearAttenuation
    Specifies the linear attenuation of the spot light.

    \note The exact meaning and use of this property is up to the
          material implementation.
 */
float QSpotLight::linearAttenuation() const
{
    Q_D(const QSpotLight);
    return d->m_shaderData->property("linearAttenuation").toFloat();
}

void QSpotLight::setLinearAttenuation(float value)
{
    Q_D(QSpotLight);
    if (linearAttenuation() != value) {
        d->m_shaderData->setProperty("linearAttenuation", value);
        emit linearAttenuationChanged(value);
    }
}

/*!
  \qmlproperty float Qt3D.Render::SpotLight::quadraticAttenuation
    Specifies the quadratic attenuation of the spot light.

    \note The exact meaning and use of this property is up to the
          material implementation.
*/

/*!
  \property Qt3DRender::QSpotLight::quadraticAttenuation
    Specifies the quadratic attenuation of the spot light.

    \note The exact meaning and use of this property is up to the
          material implementation.
 */
float QSpotLight::quadraticAttenuation() const
{
    Q_D(const QSpotLight);
    return d->m_shaderData->property("quadraticAttenuation").toFloat();
}

void QSpotLight::setQuadraticAttenuation(float value)
{
    Q_D(QSpotLight);
    if (quadraticAttenuation() != value) {
        d->m_shaderData->setProperty("quadraticAttenuation", value);
        emit quadraticAttenuationChanged(value);
    }
}

/*!
  \qmlproperty vector3d Qt3D.Render::SpotLight::localDirection
    Specifies the local direction of the spot light.

    \note The exact meaning and use of this property is up to the
          material implementation.
*/

/*!
  \property Qt3DRender::QSpotLight::localDirection
    Specifies the local direction of the spot light.

    \note The exact meaning and use of this property is up to the
          material implementation.
 */
QVector3D QSpotLight::localDirection() const
{
    Q_D(const QSpotLight);
    return d->m_shaderData->property("direction").value<QVector3D>();
}

/*!
  \qmlproperty float Qt3D.Render::SpotLight::cutOffAngle
    Specifies the cut off angle of the spot light.

    \note The exact meaning and use of this property is up to the
          material implementation.
*/

/*!
  \property Qt3DRender::QSpotLight::cutOffAngle
    Specifies the cut off angle of the spot light.

    \note The exact meaning and use of this property is up to the
          material implementation.
 */
float QSpotLight::cutOffAngle() const
{
    Q_D(const QSpotLight);
    return d->m_shaderData->property("cutOffAngle").toFloat();
}

void QSpotLight::setLocalDirection(const QVector3D &direction)
{
    Q_D(QSpotLight);
    if (localDirection() != direction) {
        const QVector3D dir = direction.normalized();
        d->m_shaderData->setProperty("direction", dir);
        emit localDirectionChanged(dir);
    }
}

void QSpotLight::setCutOffAngle(float value)
{
    Q_D(QSpotLight);
    if (cutOffAngle() != value) {
        d->m_shaderData->setProperty("cutOffAngle", value);
        emit cutOffAngleChanged(value);
    }
}

} // namespace Qt3DRender

QT_END_NAMESPACE

#include "moc_qspotlight.cpp"
