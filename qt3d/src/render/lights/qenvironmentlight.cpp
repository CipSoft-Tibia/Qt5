// Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qenvironmentlight.h"
#include "qenvironmentlight_p.h"
#include "qabstracttexture.h"
#include <QVector3D>

#include <cmath>

QT_BEGIN_NAMESPACE

namespace Qt3DRender
{

/*!
 * \qmltype EnvironmentLight
 * \inqmlmodule Qt3D.Render
 * \instantiates Qt3DRender::QEnvironmentLight
 * \brief Encapsulate an environment light object in a Qt 3D scene.
 * \since 5.9
 *
 * EnvironmentLight uses cubemaps to implement image-based lighting (IBL), a technique
 * often used in conjunction with physically-based rendering (PBR). The cubemaps are
 * typically expected be based on high dynamic range (HDR) images, with a suitable
 * OpenGL format (such as RGBA16F) that can handle the increased range of values.
 *
 * There are a variety of tools that can be used to produce the cubemaps needed by
 * EnvironmentLight. Some examples include
 *
 * \list
 * \li \l {https://github.com/dariomanesku/cmftStudio}{cmftStudio}
 * \li \l {https://github.com/derkreature/IBLBaker}{IBLBaker}
 * \li \l {https://www.knaldtech.com/lys/}{Lys}
 * \endlist
 *
 * \l {https://hdrihaven.com/hdris/}{HDRI Haven} provides many CC0-licensed HDR images
 * that can be used as source material for the above tools.
 */

QEnvironmentLightPrivate::QEnvironmentLightPrivate()
    : m_shaderData(new QShaderData)
    , m_irradiance(nullptr)
    , m_specular(nullptr)
{
}

QEnvironmentLightPrivate::~QEnvironmentLightPrivate()
{
}

void QEnvironmentLightPrivate::_q_updateEnvMapsSize()
{
    QVector3D irradianceSize;
    if (m_irradiance != nullptr)
        irradianceSize = QVector3D(m_irradiance->width(),
                                   m_irradiance->height(),
                                   m_irradiance->depth());
    m_shaderData->setProperty("irradianceSize", QVariant::fromValue(irradianceSize));

    QVector3D specularSize;
    if (m_specular != nullptr)
        specularSize = QVector3D(m_specular->width(),
                                 m_specular->height(),
                                 m_specular->depth());
    m_shaderData->setProperty("specularSize", QVariant::fromValue(specularSize));

    const int levels = int(std::log2(specularSize.x() > 0.0f ? specularSize.x() : 1.0f)) + 1;
    m_shaderData->setProperty("specularMipLevels", QVariant::fromValue(levels));
}

/*!
    \class Qt3DRender::QEnvironmentLight
    \inmodule Qt3DRender
    \brief Encapsulate an environment light object in a Qt 3D scene.
    \since 5.9

    QEnvironmentLight uses cubemaps to implement image-based lighting (IBL), a technique
    often used in conjunction with physically-based rendering (PBR). The cubemaps are
    typically expected be based on high dynamic range (HDR) images, with a suitable
    OpenGL format (such as RGBA16F) that can handle the increased range of values.

    There are a variety of tools that can be used to produce the cubemaps needed by
    QEnvironmentLight. Some examples include

    \list
    \li \l {https://github.com/dariomanesku/cmftStudio}{cmftStudio}
    \li \l {https://github.com/derkreature/IBLBaker}{IBLBaker}
    \li \l {https://www.knaldtech.com/lys/}{Lys}
    \endlist

    \l {https://hdrihaven.com/hdris/}{HDRI Haven} provides many CC0-licensed HDR images
    that can be used as source material for the above tools.
*/

QEnvironmentLight::QEnvironmentLight(Qt3DCore::QNode *parent)
    : QComponent(*new QEnvironmentLightPrivate, parent)
{
    Q_D(QEnvironmentLight);
    d->m_shaderData->setParent(this);
}

/*! \internal */
QEnvironmentLight::QEnvironmentLight(QEnvironmentLightPrivate &dd, QNode *parent)
    : QComponent(dd, parent)
{
    Q_D(QEnvironmentLight);
    d->m_shaderData->setParent(this);
}

QEnvironmentLight::~QEnvironmentLight()
{
}

/*!
    \qmlproperty Texture EnvironmentLight::irradiance

    Holds the current environment irradiance map texture.

    By default, the environment irradiance texture is null.

    \note The exact meaning and use of this property is up to the
          material implementation.
*/

/*!
    \property QEnvironmentLight::irradiance

    Holds the current environment irradiance map texture.

    By default, the environment irradiance texture is null.

    \note The exact meaning and use of this property is up to the
          material implementation.
*/
QAbstractTexture *QEnvironmentLight::irradiance() const
{
    Q_D(const QEnvironmentLight);
    return d->m_irradiance;
}

/*!
    \qmlproperty Texture EnvironmentLight::specular

    Holds the current environment specular map texture.

    By default, the environment specular texture is null.

    \note The exact meaning and use of this property is up to the
          material implementation.
*/

/*!
    \property QEnvironmentLight::specular

    Holds the current environment specular map texture.

    By default, the environment specular texture is null.

    \note The exact meaning and use of this property is up to the
          material implementation.
*/
QAbstractTexture *QEnvironmentLight::specular() const
{
    Q_D(const QEnvironmentLight);
    return d->m_specular;
}

void QEnvironmentLight::setIrradiance(QAbstractTexture *i)
{
    Q_D(QEnvironmentLight);
    if (irradiance() == i)
        return;

    if (irradiance()) {
        d->unregisterDestructionHelper(d->m_irradiance);
        QObject::disconnect(d->m_irradiance, SIGNAL(widthChanged(int)), this, SLOT(_q_updateEnvMapsSize()));
        QObject::disconnect(d->m_irradiance, SIGNAL(heightChanged(int)), this, SLOT(_q_updateEnvMapsSize()));
        QObject::disconnect(d->m_irradiance, SIGNAL(depthChanged(int)), this, SLOT(_q_updateEnvMapsSize()));
    }

    if (i && !i->parent())
        i->setParent(this);

    d->m_irradiance = i;
    d->m_shaderData->setProperty("irradiance", QVariant::fromValue(i));
    d->_q_updateEnvMapsSize();

    if (i) {
        d->registerDestructionHelper(i, &QEnvironmentLight::setIrradiance, i);
        QObject::connect(d->m_irradiance, SIGNAL(widthChanged(int)), this, SLOT(_q_updateEnvMapsSize()));
        QObject::connect(d->m_irradiance, SIGNAL(heightChanged(int)), this, SLOT(_q_updateEnvMapsSize()));
        QObject::connect(d->m_irradiance, SIGNAL(depthChanged(int)), this, SLOT(_q_updateEnvMapsSize()));
    }

    emit irradianceChanged(i);
}

void QEnvironmentLight::setSpecular(QAbstractTexture *s)
{
    Q_D(QEnvironmentLight);
    if (specular() == s)
        return;

    if (specular()) {
        d->unregisterDestructionHelper(d->m_specular);
        QObject::disconnect(d->m_specular, SIGNAL(widthChanged(int)), this, SLOT(_q_updateEnvMapsSize()));
        QObject::disconnect(d->m_specular, SIGNAL(heightChanged(int)), this, SLOT(_q_updateEnvMapsSize()));
        QObject::disconnect(d->m_specular, SIGNAL(depthChanged(int)), this, SLOT(_q_updateEnvMapsSize()));
    }

    if (s && !s->parent())
        s->setParent(this);

    d->m_specular = s;
    d->m_shaderData->setProperty("specular", QVariant::fromValue(s));
    d->_q_updateEnvMapsSize();

    if (s) {
        d->registerDestructionHelper(s, &QEnvironmentLight::setSpecular, s);
        QObject::connect(d->m_specular, SIGNAL(widthChanged(int)), this, SLOT(_q_updateEnvMapsSize()));
        QObject::connect(d->m_specular, SIGNAL(heightChanged(int)), this, SLOT(_q_updateEnvMapsSize()));
        QObject::connect(d->m_specular, SIGNAL(depthChanged(int)), this, SLOT(_q_updateEnvMapsSize()));
    }

    emit specularChanged(s);
}

} // namespace Qt3DRender

QT_END_NAMESPACE

#include "moc_qenvironmentlight.cpp"
