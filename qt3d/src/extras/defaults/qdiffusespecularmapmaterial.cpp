// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdiffusespecularmapmaterial.h"
#include "qdiffusespecularmapmaterial_p.h"

#include <Qt3DRender/qfilterkey.h>
#include <Qt3DRender/qmaterial.h>
#include <Qt3DRender/qeffect.h>
#include <Qt3DRender/qtexture.h>
#include <Qt3DRender/qtechnique.h>
#include <Qt3DRender/qshaderprogram.h>
#include <Qt3DRender/qshaderprogrambuilder.h>
#include <Qt3DRender/qparameter.h>
#include <Qt3DRender/qrenderpass.h>
#include <Qt3DRender/qgraphicsapifilter.h>
#include <QtCore/QUrl>
#include <QtGui/QVector3D>
#include <QtGui/QVector4D>

QT_BEGIN_NAMESPACE

using namespace Qt3DRender;

namespace Qt3DExtras {

QDiffuseSpecularMapMaterialPrivate::QDiffuseSpecularMapMaterialPrivate()
    : QMaterialPrivate()
    , m_diffuseSpecularMapEffect(new QEffect())
    , m_diffuseTexture(new QTexture2D())
    , m_specularTexture(new QTexture2D())
    , m_ambientParameter(new QParameter(QStringLiteral("ka"), QColor::fromRgbF(0.05f, 0.05f, 0.05f, 1.0f)))
    , m_diffuseParameter(new QParameter(QStringLiteral("diffuseTexture"), m_diffuseTexture))
    , m_specularParameter(new QParameter(QStringLiteral("specularTexture"), m_specularTexture))
    , m_shininessParameter(new QParameter(QStringLiteral("shininess"), 150.0f))
    , m_textureScaleParameter(new QParameter(QStringLiteral("texCoordScale"), 1.0f))
    , m_diffuseSpecularMapGL3Technique(new QTechnique())
    , m_diffuseSpecularMapGL2Technique(new QTechnique())
    , m_diffuseSpecularMapES2Technique(new QTechnique())
    , m_diffuseSpecularMapRHITechnique(new QTechnique())
    , m_diffuseSpecularMapGL3RenderPass(new QRenderPass())
    , m_diffuseSpecularMapGL2RenderPass(new QRenderPass())
    , m_diffuseSpecularMapES2RenderPass(new QRenderPass())
    , m_diffuseSpecularMapRHIRenderPass(new QRenderPass())
    , m_diffuseSpecularMapGL3Shader(new QShaderProgram())
    , m_diffuseSpecularMapGL3ShaderBuilder(new QShaderProgramBuilder())
    , m_diffuseSpecularMapGL2ES2Shader(new QShaderProgram())
    , m_diffuseSpecularMapGL2ES2ShaderBuilder(new QShaderProgramBuilder())
    , m_diffuseSpecularMapRHIShader(new QShaderProgram())
    , m_diffuseSpecularMapRHIShaderBuilder(new QShaderProgramBuilder())
    , m_filterKey(new QFilterKey)
{
    m_diffuseTexture->setMagnificationFilter(QAbstractTexture::Linear);
    m_diffuseTexture->setMinificationFilter(QAbstractTexture::LinearMipMapLinear);
    m_diffuseTexture->setWrapMode(QTextureWrapMode(QTextureWrapMode::Repeat));
    m_diffuseTexture->setGenerateMipMaps(true);
    m_diffuseTexture->setMaximumAnisotropy(16.0f);

    m_specularTexture->setMagnificationFilter(QAbstractTexture::Linear);
    m_specularTexture->setMinificationFilter(QAbstractTexture::LinearMipMapLinear);
    m_specularTexture->setWrapMode(QTextureWrapMode(QTextureWrapMode::Repeat));
    m_specularTexture->setGenerateMipMaps(true);
    m_specularTexture->setMaximumAnisotropy(16.0f);
}

void QDiffuseSpecularMapMaterialPrivate::init()
{
    Q_Q(QDiffuseSpecularMapMaterial);

    connect(m_ambientParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QDiffuseSpecularMapMaterialPrivate::handleAmbientChanged);
    connect(m_diffuseParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QDiffuseSpecularMapMaterialPrivate::handleDiffuseChanged);
    connect(m_specularParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QDiffuseSpecularMapMaterialPrivate::handleSpecularChanged);
    connect(m_shininessParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QDiffuseSpecularMapMaterialPrivate::handleShininessChanged);
    connect(m_textureScaleParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QDiffuseSpecularMapMaterialPrivate::handleTextureScaleChanged);

    m_diffuseSpecularMapGL3Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/gl3/default.vert"))));
    m_diffuseSpecularMapGL3ShaderBuilder->setParent(q);
    m_diffuseSpecularMapGL3ShaderBuilder->setShaderProgram(m_diffuseSpecularMapGL3Shader);
    m_diffuseSpecularMapGL3ShaderBuilder->setFragmentShaderGraph(QUrl(QStringLiteral("qrc:/shaders/graphs/phong.frag.json")));
    m_diffuseSpecularMapGL3ShaderBuilder->setEnabledLayers({QStringLiteral("diffuseTexture"),
                                                            QStringLiteral("specularTexture"),
                                                            QStringLiteral("normal")});

    m_diffuseSpecularMapGL2ES2Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/es2/default.vert"))));
    m_diffuseSpecularMapGL2ES2ShaderBuilder->setParent(q);
    m_diffuseSpecularMapGL2ES2ShaderBuilder->setShaderProgram(m_diffuseSpecularMapGL2ES2Shader);
    m_diffuseSpecularMapGL2ES2ShaderBuilder->setFragmentShaderGraph(QUrl(QStringLiteral("qrc:/shaders/graphs/phong.frag.json")));
    m_diffuseSpecularMapGL2ES2ShaderBuilder->setEnabledLayers({QStringLiteral("diffuseTexture"),
                                                               QStringLiteral("specularTexture"),
                                                               QStringLiteral("normal")});

    m_diffuseSpecularMapRHIShader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/rhi/default_pos_norm_tex.vert"))));
    m_diffuseSpecularMapRHIShaderBuilder->setParent(q);
    m_diffuseSpecularMapRHIShaderBuilder->setShaderProgram(m_diffuseSpecularMapRHIShader);
    m_diffuseSpecularMapRHIShaderBuilder->setFragmentShaderGraph(QUrl(QStringLiteral("qrc:/shaders/graphs/phong.frag.json")));
    m_diffuseSpecularMapRHIShaderBuilder->setEnabledLayers({QStringLiteral("diffuseTexture"),
                                                            QStringLiteral("specularTexture"),
                                                            QStringLiteral("normal")});

    m_diffuseSpecularMapGL3Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_diffuseSpecularMapGL3Technique->graphicsApiFilter()->setMajorVersion(3);
    m_diffuseSpecularMapGL3Technique->graphicsApiFilter()->setMinorVersion(1);
    m_diffuseSpecularMapGL3Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::CoreProfile);

    m_diffuseSpecularMapGL2Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_diffuseSpecularMapGL2Technique->graphicsApiFilter()->setMajorVersion(2);
    m_diffuseSpecularMapGL2Technique->graphicsApiFilter()->setMinorVersion(0);
    m_diffuseSpecularMapGL2Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::NoProfile);

    m_diffuseSpecularMapES2Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGLES);
    m_diffuseSpecularMapES2Technique->graphicsApiFilter()->setMajorVersion(2);
    m_diffuseSpecularMapES2Technique->graphicsApiFilter()->setMinorVersion(0);
    m_diffuseSpecularMapES2Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::NoProfile);

    m_diffuseSpecularMapRHITechnique->graphicsApiFilter()->setApi(QGraphicsApiFilter::RHI);
    m_diffuseSpecularMapRHITechnique->graphicsApiFilter()->setMajorVersion(1);
    m_diffuseSpecularMapRHITechnique->graphicsApiFilter()->setMinorVersion(0);

    m_filterKey->setParent(q);
    m_filterKey->setName(QStringLiteral("renderingStyle"));
    m_filterKey->setValue(QStringLiteral("forward"));

    m_diffuseSpecularMapGL3Technique->addFilterKey(m_filterKey);
    m_diffuseSpecularMapGL2Technique->addFilterKey(m_filterKey);
    m_diffuseSpecularMapES2Technique->addFilterKey(m_filterKey);
    m_diffuseSpecularMapRHITechnique->addFilterKey(m_filterKey);

    m_diffuseSpecularMapGL3RenderPass->setShaderProgram(m_diffuseSpecularMapGL3Shader);
    m_diffuseSpecularMapGL2RenderPass->setShaderProgram(m_diffuseSpecularMapGL2ES2Shader);
    m_diffuseSpecularMapES2RenderPass->setShaderProgram(m_diffuseSpecularMapGL2ES2Shader);
    m_diffuseSpecularMapRHIRenderPass->setShaderProgram(m_diffuseSpecularMapRHIShader);

    m_diffuseSpecularMapGL3Technique->addRenderPass(m_diffuseSpecularMapGL3RenderPass);
    m_diffuseSpecularMapGL2Technique->addRenderPass(m_diffuseSpecularMapGL2RenderPass);
    m_diffuseSpecularMapES2Technique->addRenderPass(m_diffuseSpecularMapES2RenderPass);
    m_diffuseSpecularMapRHITechnique->addRenderPass(m_diffuseSpecularMapRHIRenderPass);

    m_diffuseSpecularMapEffect->addTechnique(m_diffuseSpecularMapGL3Technique);
    m_diffuseSpecularMapEffect->addTechnique(m_diffuseSpecularMapGL2Technique);
    m_diffuseSpecularMapEffect->addTechnique(m_diffuseSpecularMapES2Technique);
    m_diffuseSpecularMapEffect->addTechnique(m_diffuseSpecularMapRHITechnique);

    m_diffuseSpecularMapEffect->addParameter(m_ambientParameter);
    m_diffuseSpecularMapEffect->addParameter(m_diffuseParameter);
    m_diffuseSpecularMapEffect->addParameter(m_specularParameter);
    m_diffuseSpecularMapEffect->addParameter(m_shininessParameter);
    m_diffuseSpecularMapEffect->addParameter(m_textureScaleParameter);

    q->setEffect(m_diffuseSpecularMapEffect);
}

void QDiffuseSpecularMapMaterialPrivate::handleAmbientChanged(const QVariant &var)
{
    Q_Q(QDiffuseSpecularMapMaterial);
    emit q->ambientChanged(var.value<QColor>());
}

void QDiffuseSpecularMapMaterialPrivate::handleDiffuseChanged(const QVariant &var)
{
    Q_Q(QDiffuseSpecularMapMaterial);
    emit q->diffuseChanged(var.value<QAbstractTexture *>());
}

void QDiffuseSpecularMapMaterialPrivate::handleSpecularChanged(const QVariant &var)
{
    Q_Q(QDiffuseSpecularMapMaterial);
    emit q->specularChanged(var.value<QAbstractTexture *>());
}

void QDiffuseSpecularMapMaterialPrivate::handleShininessChanged(const QVariant &var)
{
    Q_Q(QDiffuseSpecularMapMaterial);
    emit q->shininessChanged(var.toFloat());
}

void QDiffuseSpecularMapMaterialPrivate::handleTextureScaleChanged(const QVariant &var)
{
    Q_Q(QDiffuseSpecularMapMaterial);
    emit q->textureScaleChanged(var.toFloat());
}

/*!
    \class Qt3DExtras::QDiffuseSpecularMapMaterial
    \brief The QDiffuseSpecularMapMaterial provides a default implementation of the phong lighting
    effect where the diffuse and specular light components are read from texture maps.
    \inmodule Qt3DExtras
    \since 5.7
    \inherits Qt3DRender::QMaterial

    \deprecated
    This class is deprecated; use Qt3DExtras::QDiffuseSpecularMaterial instead.

    The specular lighting effect is based on the combination of 3 lighting components ambient,
    diffuse and specular. The relative strengths of these components are controlled by means of
    their reflectivity coefficients which are modelled as RGB triplets:

    \list
    \li Ambient is the color that is emitted by an object without any other light source.
    \li Diffuse is the color that is emitted for rough surface reflections with the lights.
    \li Specular is the color emitted for shiny surface reflections with the lights.
    \li The shininess of a surface is controlled by a float property.
    \endlist

    This material uses an effect with a single render pass approach and performs per fragment
    lighting. Techniques are provided for OpenGL 2, OpenGL 3 or above as well as OpenGL ES 2.
*/

/*!
    Constructs a new QDiffuseSpecularMapMaterial instance with parent object \a parent.
*/
QDiffuseSpecularMapMaterial::QDiffuseSpecularMapMaterial(QNode *parent)
    : QMaterial(*new QDiffuseSpecularMapMaterialPrivate, parent)
{
    Q_D(QDiffuseSpecularMapMaterial);
    d->init();
}

/*!
    Destroys the QDiffuseSpecularMapMaterial instance.
*/
QDiffuseSpecularMapMaterial::~QDiffuseSpecularMapMaterial()
{
}

/*!
    \property QDiffuseSpecularMapMaterial::ambient

    Holds the current ambient color that is emitted by an object without any
    other light source.
*/
QColor QDiffuseSpecularMapMaterial::ambient() const
{
    Q_D(const QDiffuseSpecularMapMaterial);
    return d->m_ambientParameter->value().value<QColor>();
}

/*!
    \property QDiffuseSpecularMapMaterial::diffuse

    Holds the current diffuse map texture.

    By default, the diffuse texture has the following properties:

    \list
        \li Linear minification and magnification filters
        \li Linear mipmap with mipmapping enabled
        \li Repeat wrap mode
        \li Maximum anisotropy of 16.0
    \endlist
*/
QAbstractTexture *QDiffuseSpecularMapMaterial::diffuse() const
{
    Q_D(const QDiffuseSpecularMapMaterial);
    return d->m_diffuseParameter->value().value<QAbstractTexture *>();
}

/*!
    \property QDiffuseSpecularMapMaterial::specular

    Holds the current specular map texture.

    By default, the specular texture has the following properties:

    \list
        \li Linear minification and magnification filters
        \li Linear mipmap with mipmapping enabled
        \li Repeat wrap mode
        \li Maximum anisotropy of 16.0
    \endlist
*/
QAbstractTexture *QDiffuseSpecularMapMaterial::specular() const
{
    Q_D(const QDiffuseSpecularMapMaterial);
    return d->m_specularParameter->value().value<QAbstractTexture *>();
}

/*!
    \property QDiffuseSpecularMapMaterial::shininess

    Holds the current shininess as a float value. Higher values of shininess result in
    a smaller and brighter highlight.

    Defaults to 150.0.
*/
float QDiffuseSpecularMapMaterial::shininess() const
{
    Q_D(const QDiffuseSpecularMapMaterial);
    return d->m_shininessParameter->value().toFloat();
}

/*!
    \property QDiffuseSpecularMapMaterial::textureScale

    Holds the current texture scale. It is applied as a multiplier to texture
    coordinates at render time. Defaults to 1.0.

    When used in conjunction with QTextureWrapMode::Repeat, textureScale provides a simple
    way to tile a texture across a surface. For example, a texture scale of \c 4.0
    would result in 16 (4x4) tiles.
*/
float QDiffuseSpecularMapMaterial::textureScale() const
{
    Q_D(const QDiffuseSpecularMapMaterial);
    return d->m_textureScaleParameter->value().toFloat();
}

void QDiffuseSpecularMapMaterial::setAmbient(const QColor &ambient)
{
    Q_D(QDiffuseSpecularMapMaterial);
    d->m_ambientParameter->setValue(ambient);
}

void QDiffuseSpecularMapMaterial::setDiffuse(QAbstractTexture *diffuse)
{
    Q_D(QDiffuseSpecularMapMaterial);
    d->m_diffuseParameter->setValue(QVariant::fromValue(diffuse));
}

void QDiffuseSpecularMapMaterial::setSpecular(QAbstractTexture *specular)
{
    Q_D(QDiffuseSpecularMapMaterial);
    d->m_specularParameter->setValue(QVariant::fromValue(specular));
}

void QDiffuseSpecularMapMaterial::setShininess(float shininess)
{
    Q_D(QDiffuseSpecularMapMaterial);
    d->m_shininessParameter->setValue(shininess);
}

void QDiffuseSpecularMapMaterial::setTextureScale(float textureScale)
{
    Q_D(QDiffuseSpecularMapMaterial);
    d->m_textureScaleParameter->setValue(textureScale);
}

} // namespace Qt3DExtras

QT_END_NAMESPACE

#include "moc_qdiffusespecularmapmaterial.cpp"
