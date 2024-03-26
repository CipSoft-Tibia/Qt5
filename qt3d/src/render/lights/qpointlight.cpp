// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpointlight.h"
#include "qpointlight_p.h"

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

/*
  Expected Shader struct

  \code

  struct PointLight
  {
   vec3 position;
   vec4 color;
   float intensity;
  };

  uniform PointLight pointLights[10];

  \endcode
 */

QPointLightPrivate::QPointLightPrivate()
    : QAbstractLightPrivate(QAbstractLight::PointLight)
{
    m_shaderData->setProperty("constantAttenuation", 1.0f);
    m_shaderData->setProperty("linearAttenuation", 0.0f);
    m_shaderData->setProperty("quadraticAttenuation", 0.0f);
}

/*!
  \class Qt3DRender::QPointLight
  \inmodule Qt3DRender
  \since 5.5
    \brief Encapsulate a Point Light object in a Qt 3D scene.

    A point light is a light source that emits light in all directions, from a single point.
    Conceptually, this is similar to light given off by a standard light bulb.

    A point light uses three attenuation factors to describe how the intensity of the light
    decreases over distance. These factors are designed to be used together in calcuating total
    attenuation. For the materials in Qt3D Extras the following formula is used, where distance
    is the distance from the light to the surface being rendered:

    \code
    totalAttenuation = 1.0 / (constantAttenuation + (linearAttenuation * distance) + (quadraticAttenuation * distance * distance));
    \endcode

    Custom materials may choose to interpret these factors differently.
 */

/*!
    \qmltype PointLight
    \instantiates Qt3DRender::QPointLight
    \inherits AbstractLight
    \inqmlmodule Qt3D.Render
    \since 5.5
    \brief Encapsulate a Point Light object in a Qt 3D scene.

    A point light is a light source that emits light in all directions, from a single point.
    Conceptually, this is similar to light given off by a standard light bulb.

    A point light uses three attenuation factors to describe how the intensity of the light
    decreases over distance. These factors are designed to be used together in calcuating total
    attenuation. For the materials in Qt3D Extras the following formula is used, where distance
    is the distance from the light to the surface being rendered:

    \code
    totalAttenuation = 1.0 / (constantAttenuation + (linearAttenuation * distance) + (quadraticAttenuation * distance * distance));
    \endcode

    Custom materials may choose to interpret these factors differently.
*/

/*!
  \fn Qt3DRender::QPointLight::QPointLight(Qt3DCore::QNode *parent)
  Constructs a new QPointLight with the specified \a parent.
 */
QPointLight::QPointLight(QNode *parent)
    : QAbstractLight(*new QPointLightPrivate, parent)
{
}

/*! \internal */
QPointLight::~QPointLight()
{
}

/*! \internal */
QPointLight::QPointLight(QPointLightPrivate &dd, QNode *parent)
    : QAbstractLight(dd, parent)
{
}

/*!
  \qmlproperty float Qt3D.Render::PointLight::constantAttenuation
    Specifies the constant attenuation of the point light.

    \note The exact meaning and use of this property is up to the
          material implementation.
*/

/*!
  \property Qt3DRender::QPointLight::constantAttenuation
    Specifies the constant attenuation of the point light.

    \note The exact meaning and use of this property is up to the
          material implementation.
 */
float QPointLight::constantAttenuation() const
{
    Q_D(const QPointLight);
    return d->m_shaderData->property("constantAttenuation").toFloat();
}

void QPointLight::setConstantAttenuation(float value)
{
    Q_D(QPointLight);
    if (constantAttenuation() != value) {
        d->m_shaderData->setProperty("constantAttenuation", value);
        emit constantAttenuationChanged(value);
    }
}

/*!
  \qmlproperty float Qt3D.Render::PointLight::linearAttenuation
    Specifies the linear attenuation of the point light.

    \note The exact meaning and use of this property is up to the
          material implementation.
*/

/*!
  \property Qt3DRender::QPointLight::linearAttenuation
    Specifies the linear attenuation of the point light.

    \note The exact meaning and use of this property is up to the
          material implementation.
 */
float QPointLight::linearAttenuation() const
{
    Q_D(const QPointLight);
    return d->m_shaderData->property("linearAttenuation").toFloat();
}

void QPointLight::setLinearAttenuation(float value)
{
    Q_D(QPointLight);
    if (linearAttenuation() != value) {
        d->m_shaderData->setProperty("linearAttenuation", value);
        emit linearAttenuationChanged(value);
    }
}

/*!
  \qmlproperty float Qt3D.Render::PointLight::quadraticAttenuation
    Specifies the quadratic attenuation of the point light.

    \note The exact meaning and use of this property is up to the
          material implementation.
*/

/*!
  \property Qt3DRender::QPointLight::quadraticAttenuation
    Specifies the quadratic attenuation of the point light.

    \note The exact meaning and use of this property is up to the
          material implementation.
 */
float QPointLight::quadraticAttenuation() const
{
    Q_D(const QPointLight);
    return d->m_shaderData->property("quadraticAttenuation").toFloat();
}

void QPointLight::setQuadraticAttenuation(float value)
{
    Q_D(QPointLight);
    if (quadraticAttenuation() != value) {
        d->m_shaderData->setProperty("quadraticAttenuation", value);
        emit quadraticAttenuationChanged(value);
    }
}

} // namespace Qt3DRender

QT_END_NAMESPACE

#include "moc_qpointlight.cpp"
