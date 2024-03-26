// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qnormaldiffusemapmaterial.h"
#include "qnormaldiffusemapmaterial_p.h"

#include <Qt3DRender/qfilterkey.h>
#include <Qt3DRender/qeffect.h>
#include <Qt3DRender/qtexture.h>
#include <Qt3DRender/qtechnique.h>
#include <Qt3DRender/qparameter.h>
#include <Qt3DRender/qshaderprogram.h>
#include <Qt3DRender/qshaderprogrambuilder.h>
#include <Qt3DRender/qrenderpass.h>
#include <Qt3DRender/qgraphicsapifilter.h>
#include <QtCore/QUrl>
#include <QtGui/QVector3D>
#include <QtGui/QVector4D>

QT_BEGIN_NAMESPACE

using namespace Qt3DRender;

namespace Qt3DExtras {

QNormalDiffuseMapMaterialPrivate::QNormalDiffuseMapMaterialPrivate()
    : QMaterialPrivate()
    , m_normalDiffuseEffect(new QEffect())
    , m_diffuseTexture(new QTexture2D())
    , m_normalTexture(new QTexture2D())
    , m_ambientParameter(new QParameter(QStringLiteral("ka"), QColor::fromRgbF(0.1f, 0.1f, 0.1f, 1.0f)))
    , m_diffuseParameter(new QParameter(QStringLiteral("diffuseTexture"), m_diffuseTexture))
    , m_normalParameter(new QParameter(QStringLiteral("normalTexture"), m_normalTexture))
    , m_specularParameter(new QParameter(QStringLiteral("ks"), QColor::fromRgbF(0.01f, 0.01f, 0.01f, 1.0f)))
    , m_shininessParameter(new QParameter(QStringLiteral("shininess"), 150.0f))
    , m_textureScaleParameter(new QParameter(QStringLiteral("texCoordScale"), 1.0f))
    , m_normalDiffuseGL3Technique(new QTechnique())
    , m_normalDiffuseGL2Technique(new QTechnique())
    , m_normalDiffuseES2Technique(new QTechnique())
    , m_normalDiffuseRHITechnique(new QTechnique())
    , m_normalDiffuseGL3RenderPass(new QRenderPass())
    , m_normalDiffuseGL2RenderPass(new QRenderPass())
    , m_normalDiffuseES2RenderPass(new QRenderPass())
    , m_normalDiffuseRHIRenderPass(new QRenderPass())
    , m_normalDiffuseGL3Shader(new QShaderProgram())
    , m_normalDiffuseGL3ShaderBuilder(new QShaderProgramBuilder())
    , m_normalDiffuseGL2ES2Shader(new QShaderProgram())
    , m_normalDiffuseGL2ES2ShaderBuilder(new QShaderProgramBuilder())
    , m_normalDiffuseRHIShader(new QShaderProgram())
    , m_normalDiffuseRHIShaderBuilder(new QShaderProgramBuilder())
    , m_filterKey(new QFilterKey)
{
    m_diffuseTexture->setMagnificationFilter(QAbstractTexture::Linear);
    m_diffuseTexture->setMinificationFilter(QAbstractTexture::LinearMipMapLinear);
    m_diffuseTexture->setWrapMode(QTextureWrapMode(QTextureWrapMode::Repeat));
    m_diffuseTexture->setGenerateMipMaps(true);
    m_diffuseTexture->setMaximumAnisotropy(16.0f);

    m_normalTexture->setMagnificationFilter(QAbstractTexture::Linear);
    m_normalTexture->setMinificationFilter(QAbstractTexture::LinearMipMapLinear);
    m_normalTexture->setWrapMode(QTextureWrapMode(QTextureWrapMode::Repeat));
    m_normalTexture->setGenerateMipMaps(true);
    m_normalTexture->setMaximumAnisotropy(16.0f);
}

void QNormalDiffuseMapMaterialPrivate::init()
{
    Q_Q(QNormalDiffuseMapMaterial);

    connect(m_ambientParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QNormalDiffuseMapMaterialPrivate::handleAmbientChanged);
    connect(m_diffuseParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QNormalDiffuseMapMaterialPrivate::handleDiffuseChanged);
    connect(m_normalParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QNormalDiffuseMapMaterialPrivate::handleNormalChanged);
    connect(m_specularParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QNormalDiffuseMapMaterialPrivate::handleSpecularChanged);
    connect(m_shininessParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QNormalDiffuseMapMaterialPrivate::handleShininessChanged);
    connect(m_textureScaleParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QNormalDiffuseMapMaterialPrivate::handleTextureScaleChanged);

    m_normalDiffuseGL3Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/gl3/default.vert"))));
    m_normalDiffuseGL3ShaderBuilder->setParent(q);
    m_normalDiffuseGL3ShaderBuilder->setShaderProgram(m_normalDiffuseGL3Shader);
    m_normalDiffuseGL3ShaderBuilder->setFragmentShaderGraph(QUrl(QStringLiteral("qrc:/shaders/graphs/phong.frag.json")));
    m_normalDiffuseGL3ShaderBuilder->setEnabledLayers({QStringLiteral("diffuseTexture"),
                                                       QStringLiteral("specular"),
                                                       QStringLiteral("normalTexture")});

    m_normalDiffuseGL2ES2Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/es2/default.vert"))));
    m_normalDiffuseGL2ES2ShaderBuilder->setParent(q);
    m_normalDiffuseGL2ES2ShaderBuilder->setShaderProgram(m_normalDiffuseGL2ES2Shader);
    m_normalDiffuseGL2ES2ShaderBuilder->setFragmentShaderGraph(QUrl(QStringLiteral("qrc:/shaders/graphs/phong.frag.json")));
    m_normalDiffuseGL2ES2ShaderBuilder->setEnabledLayers({QStringLiteral("diffuseTexture"),
                                                          QStringLiteral("specular"),
                                                          QStringLiteral("normalTexture")});

    m_normalDiffuseRHIShader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/rhi/default_pos_norm_tex_tan.vert"))));
    m_normalDiffuseRHIShaderBuilder->setParent(q);
    m_normalDiffuseRHIShaderBuilder->setShaderProgram(m_normalDiffuseRHIShader);
    m_normalDiffuseRHIShaderBuilder->setFragmentShaderGraph(QUrl(QStringLiteral("qrc:/shaders/graphs/phong.frag.json")));
    m_normalDiffuseRHIShaderBuilder->setEnabledLayers({QStringLiteral("diffuseTexture"),
                                                       QStringLiteral("specular"),
                                                       QStringLiteral("normalTexture")});

    m_normalDiffuseGL3Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_normalDiffuseGL3Technique->graphicsApiFilter()->setMajorVersion(3);
    m_normalDiffuseGL3Technique->graphicsApiFilter()->setMinorVersion(1);
    m_normalDiffuseGL3Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::CoreProfile);

    m_normalDiffuseGL2Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_normalDiffuseGL2Technique->graphicsApiFilter()->setMajorVersion(2);
    m_normalDiffuseGL2Technique->graphicsApiFilter()->setMinorVersion(0);
    m_normalDiffuseGL2Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::NoProfile);

    m_normalDiffuseES2Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGLES);
    m_normalDiffuseES2Technique->graphicsApiFilter()->setMajorVersion(2);
    m_normalDiffuseES2Technique->graphicsApiFilter()->setMinorVersion(0);
    m_normalDiffuseES2Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::NoProfile);

    m_normalDiffuseRHITechnique->graphicsApiFilter()->setApi(QGraphicsApiFilter::RHI);
    m_normalDiffuseRHITechnique->graphicsApiFilter()->setMajorVersion(1);
    m_normalDiffuseRHITechnique->graphicsApiFilter()->setMinorVersion(0);

    m_filterKey->setParent(q);
    m_filterKey->setName(QStringLiteral("renderingStyle"));
    m_filterKey->setValue(QStringLiteral("forward"));

    m_normalDiffuseGL3Technique->addFilterKey(m_filterKey);
    m_normalDiffuseGL2Technique->addFilterKey(m_filterKey);
    m_normalDiffuseES2Technique->addFilterKey(m_filterKey);
    m_normalDiffuseRHITechnique->addFilterKey(m_filterKey);

    m_normalDiffuseGL3RenderPass->setShaderProgram(m_normalDiffuseGL3Shader);
    m_normalDiffuseGL2RenderPass->setShaderProgram(m_normalDiffuseGL2ES2Shader);
    m_normalDiffuseES2RenderPass->setShaderProgram(m_normalDiffuseGL2ES2Shader);
    m_normalDiffuseRHIRenderPass->setShaderProgram(m_normalDiffuseRHIShader);

    m_normalDiffuseGL3Technique->addRenderPass(m_normalDiffuseGL3RenderPass);
    m_normalDiffuseGL2Technique->addRenderPass(m_normalDiffuseGL2RenderPass);
    m_normalDiffuseES2Technique->addRenderPass(m_normalDiffuseES2RenderPass);
    m_normalDiffuseRHITechnique->addRenderPass(m_normalDiffuseRHIRenderPass);

    m_normalDiffuseEffect->addTechnique(m_normalDiffuseGL3Technique);
    m_normalDiffuseEffect->addTechnique(m_normalDiffuseGL2Technique);
    m_normalDiffuseEffect->addTechnique(m_normalDiffuseES2Technique);
    m_normalDiffuseEffect->addTechnique(m_normalDiffuseRHITechnique);

    m_normalDiffuseEffect->addParameter(m_ambientParameter);
    m_normalDiffuseEffect->addParameter(m_diffuseParameter);
    m_normalDiffuseEffect->addParameter(m_normalParameter);
    m_normalDiffuseEffect->addParameter(m_specularParameter);
    m_normalDiffuseEffect->addParameter(m_shininessParameter);
    m_normalDiffuseEffect->addParameter(m_textureScaleParameter);

    q->setEffect(m_normalDiffuseEffect);
}

void QNormalDiffuseMapMaterialPrivate::handleAmbientChanged(const QVariant &var)
{
    Q_Q(QNormalDiffuseMapMaterial);
    emit q->ambientChanged(var.value<QColor>());
}

void QNormalDiffuseMapMaterialPrivate::handleDiffuseChanged(const QVariant &var)
{
    Q_Q(QNormalDiffuseMapMaterial);
    emit q->diffuseChanged(var.value<QAbstractTexture *>());
}

void QNormalDiffuseMapMaterialPrivate::handleNormalChanged(const QVariant &var)
{
    Q_Q(QNormalDiffuseMapMaterial);
    emit q->normalChanged(var.value<QAbstractTexture *>());
}

void QNormalDiffuseMapMaterialPrivate::handleSpecularChanged(const QVariant &var)
{
    Q_Q(QNormalDiffuseMapMaterial);
    emit q->specularChanged(var.value<QColor>());
}

void QNormalDiffuseMapMaterialPrivate::handleShininessChanged(const QVariant &var)
{
    Q_Q(QNormalDiffuseMapMaterial);
    emit q->shininessChanged(var.toFloat());
}

void QNormalDiffuseMapMaterialPrivate::handleTextureScaleChanged(const QVariant &var)
{
    Q_Q(QNormalDiffuseMapMaterial);
    emit q->textureScaleChanged(var.toFloat());
}

/*!
    \class Qt3DExtras::QNormalDiffuseMapMaterial
    \brief The QNormalDiffuseMapMaterial provides a default implementation of the phong lighting
    and bump effect where the diffuse light component is read from a texture map and the normals of
    the mesh being rendered from a normal texture map.
    \inmodule Qt3DExtras
    \since 5.7
    \inherits Qt3DRender::QMaterial

    \deprecated
    This class is deprecated; use QDiffuseSpecularMaterial instead.

    The specular lighting effect is based on the combination of 3 lighting components ambient,
    diffuse and specular. The relative strengths of these components are controlled by means of
    their reflectivity coefficients which are modelled as RGB triplets:

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
    Constructs a new QNormalDiffuseMapMaterial instance with parent object \a parent.
*/
QNormalDiffuseMapMaterial::QNormalDiffuseMapMaterial(QNode *parent)
    : QMaterial(*new QNormalDiffuseMapMaterialPrivate, parent)
{
    Q_D(QNormalDiffuseMapMaterial);
    d->init();
}

/*! \internal */
QNormalDiffuseMapMaterial::QNormalDiffuseMapMaterial(QNormalDiffuseMapMaterialPrivate &dd, QNode *parent)
    : QMaterial(dd, parent)
{
    Q_D(QNormalDiffuseMapMaterial);
    d->init();
}

/*!
    Destroys the QNormalDiffuseMapMaterial instance.
*/
QNormalDiffuseMapMaterial::~QNormalDiffuseMapMaterial()
{
}

/*!
    \property QNormalDiffuseMapMaterial::ambient

    Holds the current ambient color.
*/
QColor QNormalDiffuseMapMaterial::ambient() const
{
    Q_D(const QNormalDiffuseMapMaterial);
    return d->m_ambientParameter->value().value<QColor>();
}

/*!
    \property QNormalDiffuseMapMaterial::specular

    Holds the current specular color.
*/
QColor QNormalDiffuseMapMaterial::specular() const
{
    Q_D(const QNormalDiffuseMapMaterial);
    return d->m_specularParameter->value().value<QColor>();
}

/*!
    \property QNormalDiffuseMapMaterial::diffuse

    Holds the current diffuse map texture.

    By default, the diffuse texture has these properties:

    \list
        \li Linear minification and magnification filters
        \li Linear mipmap with mipmapping enabled
        \li Repeat wrap modeM
        \li Maximum anisotropy of 16.0
    \endlist
*/
QAbstractTexture *QNormalDiffuseMapMaterial::diffuse() const
{
    Q_D(const QNormalDiffuseMapMaterial);
    return d->m_diffuseParameter->value().value<QAbstractTexture *>();
}

/*!
    \property QNormalDiffuseMapMaterial::normal

    Holds the current normal map texture.

    By default, the normal texture has the following properties:

    \list
        \li Linear minification and magnification filters
        \li Repeat wrap mode
        \li Maximum anisotropy of 16.0
    \endlist
*/
QAbstractTexture *QNormalDiffuseMapMaterial::normal() const
{
    Q_D(const QNormalDiffuseMapMaterial);
    return d->m_normalParameter->value().value<QAbstractTexture *>();
}

/*!
    \property QNormalDiffuseMapMaterial::shininess

    Holds the current shininess as a float value.
*/
float QNormalDiffuseMapMaterial::shininess() const
{
    Q_D(const QNormalDiffuseMapMaterial);
    return d->m_shininessParameter->value().toFloat();
}

/*!
    \property QNormalDiffuseMapMaterial::textureScale

    Holds the current texture scale. It is applied as a multiplier to texture
    coordinates at render time. Defaults to 1.0.

    When used in conjunction with QTextureWrapMode::Repeat, textureScale provides a simple
    way to tile a texture across a surface. For example, a texture scale of \c 4.0
    would result in 16 (4x4) tiles.
*/
float QNormalDiffuseMapMaterial::textureScale() const
{
    Q_D(const QNormalDiffuseMapMaterial);
    return d->m_textureScaleParameter->value().toFloat();
}

void QNormalDiffuseMapMaterial::setAmbient(const QColor &ambient)
{
    Q_D(QNormalDiffuseMapMaterial);
    d->m_ambientParameter->setValue(ambient);
}

void QNormalDiffuseMapMaterial::setSpecular(const QColor &specular)
{
    Q_D(QNormalDiffuseMapMaterial);
    d->m_specularParameter->setValue(specular);
}

void QNormalDiffuseMapMaterial::setDiffuse(QAbstractTexture *diffuse)
{
    Q_D(QNormalDiffuseMapMaterial);
    d->m_diffuseParameter->setValue(QVariant::fromValue(diffuse));
}

void QNormalDiffuseMapMaterial::setNormal(QAbstractTexture *normal)
{
    Q_D(QNormalDiffuseMapMaterial);
    d->m_normalParameter->setValue(QVariant::fromValue(normal));
}

void QNormalDiffuseMapMaterial::setShininess(float shininess)
{
    Q_D(QNormalDiffuseMapMaterial);
    d->m_shininessParameter->setValue(shininess);
}

void QNormalDiffuseMapMaterial::setTextureScale(float textureScale)
{
    Q_D(QNormalDiffuseMapMaterial);
    d->m_textureScaleParameter->setValue(textureScale);
}

} // namespace Qt3DRender

QT_END_NAMESPACE

#include "moc_qnormaldiffusemapmaterial.cpp"
