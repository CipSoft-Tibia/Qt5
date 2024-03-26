// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmorphphongmaterial.h"
#include "qmorphphongmaterial_p.h"
#include <Qt3DRender/qfilterkey.h>
#include <Qt3DRender/qmaterial.h>
#include <Qt3DRender/qeffect.h>
#include <Qt3DRender/qtechnique.h>
#include <Qt3DRender/qshaderprogram.h>
#include <Qt3DRender/qparameter.h>
#include <Qt3DRender/qrenderpass.h>
#include <Qt3DRender/qgraphicsapifilter.h>
#include <Qt3DRender/qshaderprogrambuilder.h>
#include <QUrl>
#include <QVector3D>
#include <QVector4D>

QT_BEGIN_NAMESPACE

using namespace Qt3DRender;

namespace Qt3DExtras {

QMorphPhongMaterialPrivate::QMorphPhongMaterialPrivate()
    : QMaterialPrivate()
    , m_phongEffect(new QEffect())
    , m_ambientParameter(new QParameter(QStringLiteral("ka"), QColor::fromRgbF(0.05f, 0.05f, 0.05f, 1.0f)))
    , m_diffuseParameter(new QParameter(QStringLiteral("kd"), QColor::fromRgbF(0.7f, 0.7f, 0.7f, 1.0f)))
    , m_specularParameter(new QParameter(QStringLiteral("ks"), QColor::fromRgbF(0.01f, 0.01f, 0.01f, 1.0f)))
    , m_shininessParameter(new QParameter(QStringLiteral("shininess"), 150.0f))
    , m_interpolatorParameter(new QParameter(QStringLiteral("interpolator"), 0.0f))
    , m_phongGL3Technique(new QTechnique())
    , m_phongGL2Technique(new QTechnique())
    , m_phongES2Technique(new QTechnique())
    , m_phongRHITechnique(new QTechnique())
    , m_phongGL3RenderPass(new QRenderPass())
    , m_phongGL2RenderPass(new QRenderPass())
    , m_phongES2RenderPass(new QRenderPass())
    , m_phongRHIRenderPass(new QRenderPass())
    , m_phongGL3Shader(new QShaderProgram())
    , m_phongGL2ES2Shader(new QShaderProgram())
    , m_phongRHIShader(new QShaderProgram())
    , m_phongGL3ShaderBuilder(new QShaderProgramBuilder())
    , m_phongGL2ES2ShaderBuilder(new QShaderProgramBuilder())
    , m_phongRHIShaderBuilder(new QShaderProgramBuilder())
    , m_filterKey(new QFilterKey)
{
}

void QMorphPhongMaterialPrivate::init()
{
    Q_Q(QMorphPhongMaterial);

    connect(m_ambientParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QMorphPhongMaterialPrivate::handleAmbientChanged);
    connect(m_diffuseParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QMorphPhongMaterialPrivate::handleDiffuseChanged);
    connect(m_specularParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QMorphPhongMaterialPrivate::handleSpecularChanged);
    connect(m_shininessParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QMorphPhongMaterialPrivate::handleShininessChanged);
    connect(m_interpolatorParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QMorphPhongMaterialPrivate::handleInterpolatorChanged);

    m_phongGL3Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/gl3/morphphong.vert"))));
    m_phongGL3ShaderBuilder->setParent(q);
    m_phongGL3ShaderBuilder->setShaderProgram(m_phongGL3Shader);
    m_phongGL3ShaderBuilder->setFragmentShaderGraph(QUrl(QStringLiteral("qrc:/shaders/graphs/phong.frag.json")));
    m_phongGL3ShaderBuilder->setEnabledLayers({QStringLiteral("diffuse"),
                                               QStringLiteral("specular"),
                                               QStringLiteral("normal")});

    m_phongGL2ES2Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/es2/morphphong.vert"))));
    m_phongGL2ES2ShaderBuilder->setParent(q);
    m_phongGL2ES2ShaderBuilder->setShaderProgram(m_phongGL2ES2Shader);
    m_phongGL2ES2ShaderBuilder->setFragmentShaderGraph(QUrl(QStringLiteral("qrc:/shaders/graphs/phong.frag.json")));
    m_phongGL2ES2ShaderBuilder->setEnabledLayers({QStringLiteral("diffuse"),
                                                  QStringLiteral("specular"),
                                                  QStringLiteral("normal")});

    m_phongRHIShader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/rhi/morphphong.vert"))));
    m_phongRHIShaderBuilder->setParent(q);
    m_phongRHIShaderBuilder->setShaderProgram(m_phongRHIShader);
    m_phongRHIShaderBuilder->setFragmentShaderGraph(QUrl(QStringLiteral("qrc:/shaders/graphs/phong.frag.json")));
    m_phongRHIShaderBuilder->setEnabledLayers({QStringLiteral("diffuse"),
                                               QStringLiteral("specular"),
                                               QStringLiteral("normal")});

    m_phongGL3Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_phongGL3Technique->graphicsApiFilter()->setMajorVersion(3);
    m_phongGL3Technique->graphicsApiFilter()->setMinorVersion(1);
    m_phongGL3Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::CoreProfile);

    m_phongGL2Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_phongGL2Technique->graphicsApiFilter()->setMajorVersion(2);
    m_phongGL2Technique->graphicsApiFilter()->setMinorVersion(0);
    m_phongGL2Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::NoProfile);

    m_phongES2Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGLES);
    m_phongES2Technique->graphicsApiFilter()->setMajorVersion(2);
    m_phongES2Technique->graphicsApiFilter()->setMinorVersion(0);
    m_phongES2Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::NoProfile);

    m_phongRHITechnique->graphicsApiFilter()->setApi(QGraphicsApiFilter::RHI);
    m_phongRHITechnique->graphicsApiFilter()->setMajorVersion(1);
    m_phongRHITechnique->graphicsApiFilter()->setMinorVersion(0);

    m_phongGL3RenderPass->setShaderProgram(m_phongGL3Shader);
    m_phongGL2RenderPass->setShaderProgram(m_phongGL2ES2Shader);
    m_phongES2RenderPass->setShaderProgram(m_phongGL2ES2Shader);
    m_phongRHIRenderPass->setShaderProgram(m_phongRHIShader);

    m_phongGL3Technique->addRenderPass(m_phongGL3RenderPass);
    m_phongGL2Technique->addRenderPass(m_phongGL2RenderPass);
    m_phongES2Technique->addRenderPass(m_phongES2RenderPass);
    m_phongRHITechnique->addRenderPass(m_phongRHIRenderPass);

    m_filterKey->setParent(q);
    m_filterKey->setName(QStringLiteral("renderingStyle"));
    m_filterKey->setValue(QStringLiteral("forward"));

    m_phongGL3Technique->addFilterKey(m_filterKey);
    m_phongGL2Technique->addFilterKey(m_filterKey);
    m_phongES2Technique->addFilterKey(m_filterKey);
    m_phongRHITechnique->addFilterKey(m_filterKey);

    m_phongEffect->addTechnique(m_phongGL3Technique);
    m_phongEffect->addTechnique(m_phongGL2Technique);
    m_phongEffect->addTechnique(m_phongES2Technique);
    m_phongEffect->addTechnique(m_phongRHITechnique);

    m_phongEffect->addParameter(m_ambientParameter);
    m_phongEffect->addParameter(m_diffuseParameter);
    m_phongEffect->addParameter(m_specularParameter);
    m_phongEffect->addParameter(m_shininessParameter);
    m_phongEffect->addParameter(m_interpolatorParameter);

    q->setEffect(m_phongEffect);
}

void QMorphPhongMaterialPrivate::handleAmbientChanged(const QVariant &var)
{
    Q_Q(QMorphPhongMaterial);
    emit q->ambientChanged(var.value<QColor>());
}

void QMorphPhongMaterialPrivate::handleDiffuseChanged(const QVariant &var)
{
    Q_Q(QMorphPhongMaterial);
    emit q->diffuseChanged(var.value<QColor>());
}

void QMorphPhongMaterialPrivate::handleSpecularChanged(const QVariant &var)
{
    Q_Q(QMorphPhongMaterial);
    emit q->specularChanged(var.value<QColor>());
}

void QMorphPhongMaterialPrivate::handleShininessChanged(const QVariant &var)
{
    Q_Q(QMorphPhongMaterial);
    emit q->shininessChanged(var.toFloat());
}

void QMorphPhongMaterialPrivate::handleInterpolatorChanged(const QVariant &var)
{
    Q_Q(QMorphPhongMaterial);
    emit q->interpolatorChanged(var.toFloat());
}

/*!
    \class Qt3DExtras::QMorphPhongMaterial
    \ingroup qt3d-extras-materials
    \brief The QMorphPhongMaterial class provides a default implementation of the phong lighting effect.
    \inmodule Qt3DExtras
    \since 5.7
    \inherits Qt3DRender::QMaterial

    The phong lighting effect is based on the combination of 3 lighting components ambient, diffuse
    and specular. The relative strengths of these components are controlled by means of their
    reflectivity coefficients which are modelled as RGB triplets:

    \list
    \li Ambient is the color that is emitted by an object without any other light source.
    \li Diffuse is the color that is emitted for rought surface reflections with the lights.
    \li Specular is the color emitted for shiny surface reflections with the lights.
    \li The shininess of a surface is controlled by a float property.
    \endlist

    This material uses an effect with a single render pass approach and performs per fragment
    lighting. Techniques are provided for OpenGL 2, OpenGL 3 or above as well as OpenGL ES 2.
*/

/*!
    Constructs a new QMorphPhongMaterial instance with parent object \a parent.
*/
QMorphPhongMaterial::QMorphPhongMaterial(QNode *parent)
    : QMaterial(*new QMorphPhongMaterialPrivate, parent)
{
    Q_D(QMorphPhongMaterial);
    d->init();
}

/*!
   Destroys the QMorphPhongMaterial.
*/
QMorphPhongMaterial::~QMorphPhongMaterial()
{
}

/*!
    \property QMorphPhongMaterial::ambient

    Holds the ambient color.
*/
QColor QMorphPhongMaterial::ambient() const
{
    Q_D(const QMorphPhongMaterial);
    return d->m_ambientParameter->value().value<QColor>();
}

/*!
    \property QMorphPhongMaterial::diffuse

    Holds the diffuse color.
*/
QColor QMorphPhongMaterial::diffuse() const
{
    Q_D(const QMorphPhongMaterial);
    return d->m_diffuseParameter->value().value<QColor>();
}

/*!
    \property QMorphPhongMaterial::specular

    Holds the specular color.
*/
QColor QMorphPhongMaterial::specular() const
{
    Q_D(const QMorphPhongMaterial);
    return d->m_specularParameter->value().value<QColor>();
}

/*!
    \property QMorphPhongMaterial::shininess

    Holds the shininess exponent.
*/
float QMorphPhongMaterial::shininess() const
{
    Q_D(const QMorphPhongMaterial);
    return d->m_shininessParameter->value().toFloat();
}

/*!
    \property QMorphPhongMaterial::interpolator

    Contains the interpolation method of the Phong lighting effect.
*/
float QMorphPhongMaterial::interpolator() const
{
    Q_D(const QMorphPhongMaterial);
    return d->m_interpolatorParameter->value().toFloat();
}

void QMorphPhongMaterial::setAmbient(const QColor &ambient)
{
    Q_D(QMorphPhongMaterial);
    d->m_ambientParameter->setValue(ambient);
}

void QMorphPhongMaterial::setDiffuse(const QColor &diffuse)
{
    Q_D(QMorphPhongMaterial);
    d->m_diffuseParameter->setValue(diffuse);
}

void QMorphPhongMaterial::setSpecular(const QColor &specular)
{
    Q_D(QMorphPhongMaterial);
    d->m_specularParameter->setValue(specular);
}

void QMorphPhongMaterial::setShininess(float shininess)
{
    Q_D(QMorphPhongMaterial);
    d->m_shininessParameter->setValue(shininess);
}

void QMorphPhongMaterial::setInterpolator(float interpolator)
{
    Q_D(QMorphPhongMaterial);
    d->m_interpolatorParameter->setValue(interpolator);
}

} // namespace Qt3DExtras

QT_END_NAMESPACE

#include "moc_qmorphphongmaterial.cpp"
