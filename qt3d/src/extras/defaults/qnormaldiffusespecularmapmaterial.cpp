// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qnormaldiffusespecularmapmaterial.h"
#include "qnormaldiffusespecularmapmaterial_p.h"

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

QNormalDiffuseSpecularMapMaterialPrivate::QNormalDiffuseSpecularMapMaterialPrivate()
    : QMaterialPrivate()
    , m_normalDiffuseSpecularEffect(new QEffect())
    , m_diffuseTexture(new QTexture2D())
    , m_normalTexture(new QTexture2D())
    , m_specularTexture(new QTexture2D())
    , m_ambientParameter(new QParameter(QStringLiteral("ka"), QColor::fromRgbF(0.05f, 0.05f, 0.05f, 1.0f)))
    , m_diffuseParameter(new QParameter(QStringLiteral("diffuseTexture"), m_diffuseTexture))
    , m_normalParameter(new QParameter(QStringLiteral("normalTexture"), m_normalTexture))
    , m_specularParameter(new QParameter(QStringLiteral("specularTexture"), m_specularTexture))
    , m_shininessParameter(new QParameter(QStringLiteral("shininess"), 150.0f))
    , m_textureScaleParameter(new QParameter(QStringLiteral("texCoordScale"), 1.0f))
    , m_normalDiffuseSpecularGL3Technique(new QTechnique())
    , m_normalDiffuseSpecularGL2Technique(new QTechnique())
    , m_normalDiffuseSpecularES2Technique(new QTechnique())
    , m_normalDiffuseSpecularRHITechnique(new QTechnique())
    , m_normalDiffuseSpecularGL3RenderPass(new QRenderPass())
    , m_normalDiffuseSpecularGL2RenderPass(new QRenderPass())
    , m_normalDiffuseSpecularES2RenderPass(new QRenderPass())
    , m_normalDiffuseSpecularRHIRenderPass(new QRenderPass())
    , m_normalDiffuseSpecularGL3Shader(new QShaderProgram())
    , m_normalDiffuseSpecularGL3ShaderBuilder(new QShaderProgramBuilder())
    , m_normalDiffuseSpecularGL2ES2Shader(new QShaderProgram())
    , m_normalDiffuseSpecularGL2ES2ShaderBuilder(new QShaderProgramBuilder())
    , m_normalDiffuseSpecularRHIShader(new QShaderProgram())
    , m_normalDiffuseSpecularRHIShaderBuilder(new QShaderProgramBuilder())
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

    m_specularTexture->setMagnificationFilter(QAbstractTexture::Linear);
    m_specularTexture->setMinificationFilter(QAbstractTexture::LinearMipMapLinear);
    m_specularTexture->setWrapMode(QTextureWrapMode(QTextureWrapMode::Repeat));
    m_specularTexture->setGenerateMipMaps(true);
    m_specularTexture->setMaximumAnisotropy(16.0f);
}

void QNormalDiffuseSpecularMapMaterialPrivate::init()
{
    Q_Q(QNormalDiffuseSpecularMapMaterial);

    connect(m_ambientParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QNormalDiffuseSpecularMapMaterialPrivate::handleAmbientChanged);
    connect(m_diffuseParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QNormalDiffuseSpecularMapMaterialPrivate::handleDiffuseChanged);
    connect(m_normalParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QNormalDiffuseSpecularMapMaterialPrivate::handleNormalChanged);
    connect(m_specularParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QNormalDiffuseSpecularMapMaterialPrivate::handleSpecularChanged);
    connect(m_shininessParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QNormalDiffuseSpecularMapMaterialPrivate::handleShininessChanged);
    connect(m_textureScaleParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QNormalDiffuseSpecularMapMaterialPrivate::handleTextureScaleChanged);

    m_normalDiffuseSpecularGL3Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/gl3/default.vert"))));
    m_normalDiffuseSpecularGL3ShaderBuilder->setParent(q);
    m_normalDiffuseSpecularGL3ShaderBuilder->setShaderProgram(m_normalDiffuseSpecularGL3Shader);
    m_normalDiffuseSpecularGL3ShaderBuilder->setFragmentShaderGraph(QUrl(QStringLiteral("qrc:/shaders/graphs/phong.frag.json")));
    m_normalDiffuseSpecularGL3ShaderBuilder->setEnabledLayers({QStringLiteral("diffuseTexture"),
                                                               QStringLiteral("specularTexture"),
                                                               QStringLiteral("normalTexture")});

    m_normalDiffuseSpecularGL2ES2Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/es2/default.vert"))));
    m_normalDiffuseSpecularGL2ES2ShaderBuilder->setParent(q);
    m_normalDiffuseSpecularGL2ES2ShaderBuilder->setShaderProgram(m_normalDiffuseSpecularGL2ES2Shader);
    m_normalDiffuseSpecularGL2ES2ShaderBuilder->setFragmentShaderGraph(QUrl(QStringLiteral("qrc:/shaders/graphs/phong.frag.json")));
    m_normalDiffuseSpecularGL2ES2ShaderBuilder->setEnabledLayers({QStringLiteral("diffuseTexture"),
                                                                  QStringLiteral("specularTexture"),
                                                                  QStringLiteral("normalTexture")});

    m_normalDiffuseSpecularRHIShader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/rhi/default_pos_norm_tex_tan.vert"))));
    m_normalDiffuseSpecularRHIShaderBuilder->setParent(q);
    m_normalDiffuseSpecularRHIShaderBuilder->setShaderProgram(m_normalDiffuseSpecularRHIShader);
    m_normalDiffuseSpecularRHIShaderBuilder->setFragmentShaderGraph(QUrl(QStringLiteral("qrc:/shaders/graphs/phong.frag.json")));
    m_normalDiffuseSpecularRHIShaderBuilder->setEnabledLayers({QStringLiteral("diffuseTexture"),
                                                               QStringLiteral("specularTexture"),
                                                               QStringLiteral("normalTexture")});

    m_normalDiffuseSpecularGL3Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_normalDiffuseSpecularGL3Technique->graphicsApiFilter()->setMajorVersion(3);
    m_normalDiffuseSpecularGL3Technique->graphicsApiFilter()->setMinorVersion(1);
    m_normalDiffuseSpecularGL3Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::CoreProfile);

    m_normalDiffuseSpecularGL2Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_normalDiffuseSpecularGL2Technique->graphicsApiFilter()->setMajorVersion(2);
    m_normalDiffuseSpecularGL2Technique->graphicsApiFilter()->setMinorVersion(0);
    m_normalDiffuseSpecularGL2Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::NoProfile);

    m_normalDiffuseSpecularES2Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGLES);
    m_normalDiffuseSpecularES2Technique->graphicsApiFilter()->setMajorVersion(2);
    m_normalDiffuseSpecularES2Technique->graphicsApiFilter()->setMinorVersion(0);
    m_normalDiffuseSpecularES2Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::NoProfile);

    m_normalDiffuseSpecularRHITechnique->graphicsApiFilter()->setApi(QGraphicsApiFilter::RHI);
    m_normalDiffuseSpecularRHITechnique->graphicsApiFilter()->setMajorVersion(1);
    m_normalDiffuseSpecularRHITechnique->graphicsApiFilter()->setMinorVersion(0);

    m_filterKey->setParent(q);
    m_filterKey->setName(QStringLiteral("renderingStyle"));
    m_filterKey->setValue(QStringLiteral("forward"));

    m_normalDiffuseSpecularGL3Technique->addFilterKey(m_filterKey);
    m_normalDiffuseSpecularGL2Technique->addFilterKey(m_filterKey);
    m_normalDiffuseSpecularES2Technique->addFilterKey(m_filterKey);
    m_normalDiffuseSpecularRHITechnique->addFilterKey(m_filterKey);

    m_normalDiffuseSpecularGL3RenderPass->setShaderProgram(m_normalDiffuseSpecularGL3Shader);
    m_normalDiffuseSpecularGL2RenderPass->setShaderProgram(m_normalDiffuseSpecularGL2ES2Shader);
    m_normalDiffuseSpecularES2RenderPass->setShaderProgram(m_normalDiffuseSpecularGL2ES2Shader);
    m_normalDiffuseSpecularRHIRenderPass->setShaderProgram(m_normalDiffuseSpecularRHIShader);

    m_normalDiffuseSpecularGL3Technique->addRenderPass(m_normalDiffuseSpecularGL3RenderPass);
    m_normalDiffuseSpecularGL2Technique->addRenderPass(m_normalDiffuseSpecularGL2RenderPass);
    m_normalDiffuseSpecularES2Technique->addRenderPass(m_normalDiffuseSpecularES2RenderPass);
    m_normalDiffuseSpecularRHITechnique->addRenderPass(m_normalDiffuseSpecularRHIRenderPass);

    m_normalDiffuseSpecularEffect->addTechnique(m_normalDiffuseSpecularGL3Technique);
    m_normalDiffuseSpecularEffect->addTechnique(m_normalDiffuseSpecularGL2Technique);
    m_normalDiffuseSpecularEffect->addTechnique(m_normalDiffuseSpecularES2Technique);
    m_normalDiffuseSpecularEffect->addTechnique(m_normalDiffuseSpecularRHITechnique);

    m_normalDiffuseSpecularEffect->addParameter(m_ambientParameter);
    m_normalDiffuseSpecularEffect->addParameter(m_diffuseParameter);
    m_normalDiffuseSpecularEffect->addParameter(m_normalParameter);
    m_normalDiffuseSpecularEffect->addParameter(m_specularParameter);
    m_normalDiffuseSpecularEffect->addParameter(m_shininessParameter);
    m_normalDiffuseSpecularEffect->addParameter(m_textureScaleParameter);

    q->setEffect(m_normalDiffuseSpecularEffect);
}

void QNormalDiffuseSpecularMapMaterialPrivate::handleAmbientChanged(const QVariant &var)
{
    Q_Q(QNormalDiffuseSpecularMapMaterial);
    emit q->ambientChanged(var.value<QColor>());
}

void QNormalDiffuseSpecularMapMaterialPrivate::handleDiffuseChanged(const QVariant &var)
{
    Q_Q(QNormalDiffuseSpecularMapMaterial);
    emit q->diffuseChanged(var.value<QAbstractTexture *>());
}

void QNormalDiffuseSpecularMapMaterialPrivate::handleNormalChanged(const QVariant &var)
{
    Q_Q(QNormalDiffuseSpecularMapMaterial);
    emit q->normalChanged(var.value<QAbstractTexture *>());
}

void QNormalDiffuseSpecularMapMaterialPrivate::handleSpecularChanged(const QVariant &var)
{
    Q_Q(QNormalDiffuseSpecularMapMaterial);
    emit q->specularChanged(var.value<QAbstractTexture *>());
}

void QNormalDiffuseSpecularMapMaterialPrivate::handleShininessChanged(const QVariant &var)
{
    Q_Q(QNormalDiffuseSpecularMapMaterial);
    emit q->shininessChanged(var.toFloat());
}

void QNormalDiffuseSpecularMapMaterialPrivate::handleTextureScaleChanged(const QVariant &var)
{
    Q_Q(QNormalDiffuseSpecularMapMaterial);
    emit q->textureScaleChanged(var.toFloat());
}

/*!
    \class Qt3DExtras::QNormalDiffuseSpecularMapMaterial
    \brief The QNormalDiffuseSpecularMapMaterial provides a default implementation of the phong
    lighting and bump effect where the diffuse and specular light components are read from texture
    maps and the normals of the mesh being rendered from a normal texture map.
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
    Constructs a new QNormalDiffuseSpecularMapMaterial instance with parent object \a parent.
*/
QNormalDiffuseSpecularMapMaterial::QNormalDiffuseSpecularMapMaterial(QNode *parent)
    : QMaterial(*new QNormalDiffuseSpecularMapMaterialPrivate, parent)
{
    Q_D(QNormalDiffuseSpecularMapMaterial);
    d->init();
}

/*! \internal */
QNormalDiffuseSpecularMapMaterial::QNormalDiffuseSpecularMapMaterial(QNormalDiffuseSpecularMapMaterialPrivate &dd, QNode *parent)
    : QMaterial(dd, parent)
{
    Q_D(QNormalDiffuseSpecularMapMaterial);
    d->init();
}

/*!
    Destroys the QNormalDiffuseSpecularMapMaterial instance.
*/
QNormalDiffuseSpecularMapMaterial::~QNormalDiffuseSpecularMapMaterial()
{
}

/*!
    \property QNormalDiffuseSpecularMapMaterial::ambient

    Holds the current ambient color.
*/
QColor QNormalDiffuseSpecularMapMaterial::ambient() const
{
    Q_D(const QNormalDiffuseSpecularMapMaterial);
    return d->m_ambientParameter->value().value<QColor>();
}

/*!
    \property QNormalDiffuseSpecularMapMaterial::diffuse

    Holds the current diffuse map texture.

    By default, the diffuse texture has the following properties:

    \list
        \li Linear minification and magnification filters
        \li Linear mipmap with mipmapping enabled
        \li Repeat wrap mode
        \li Maximum anisotropy of 16.0
    \endlist
*/
QAbstractTexture *QNormalDiffuseSpecularMapMaterial::diffuse() const
{
    Q_D(const QNormalDiffuseSpecularMapMaterial);
    return d->m_diffuseParameter->value().value<QAbstractTexture *>();
}

/*!
    \property QNormalDiffuseSpecularMapMaterial::normal

    Holds the current normal map texture.

    By default, the normal texture has the following properties:

    \list
        \li Linear minification and magnification filters
        \li Repeat wrap mode
        \li Maximum anisotropy of 16.0
    \endlist
*/
QAbstractTexture *QNormalDiffuseSpecularMapMaterial::normal() const
{
    Q_D(const QNormalDiffuseSpecularMapMaterial);
    return d->m_normalParameter->value().value<QAbstractTexture *>();
}

/*!
    \property QNormalDiffuseSpecularMapMaterial::specular

    Holds the current specular map texture.

    By default, the specular texture has the following properties:

    \list
        \li Linear minification and magnification filters
        \li Linear mipmap with mipmapping enabled
        \li Repeat wrap mode
        \li Maximum anisotropy of 16.0
    \endlist
*/
QAbstractTexture *QNormalDiffuseSpecularMapMaterial::specular() const
{
    Q_D(const QNormalDiffuseSpecularMapMaterial);
    return d->m_specularParameter->value().value<QAbstractTexture *>();
}

/*!
    \property QNormalDiffuseSpecularMapMaterial::shininess

    Holds the current shininess as a float value.
*/
float QNormalDiffuseSpecularMapMaterial::shininess() const
{
    Q_D(const QNormalDiffuseSpecularMapMaterial);
    return d->m_shininessParameter->value().toFloat();
}

/*!
    \property QNormalDiffuseSpecularMapMaterial::textureScale

    Holds the current texture scale. It is applied as a multiplier to texture
    coordinates at render time. Defaults to 1.0.

    When used in conjunction with QTextureWrapMode::Repeat, textureScale provides a simple
    way to tile a texture across a surface. For example, a texture scale of \c 4.0
    would result in 16 (4x4) tiles.
*/
float QNormalDiffuseSpecularMapMaterial::textureScale() const
{
    Q_D(const QNormalDiffuseSpecularMapMaterial);
    return d->m_textureScaleParameter->value().toFloat();
}

void QNormalDiffuseSpecularMapMaterial::setAmbient(const QColor &ambient)
{
    Q_D(QNormalDiffuseSpecularMapMaterial);
    d->m_ambientParameter->setValue(ambient);
}

void QNormalDiffuseSpecularMapMaterial::setDiffuse(QAbstractTexture *diffuse)
{
    Q_D(QNormalDiffuseSpecularMapMaterial);
    d->m_diffuseParameter->setValue(QVariant::fromValue(diffuse));
}

void QNormalDiffuseSpecularMapMaterial::setNormal(QAbstractTexture *normal)
{
    Q_D(QNormalDiffuseSpecularMapMaterial);
    d->m_normalParameter->setValue(QVariant::fromValue(normal));
}

void QNormalDiffuseSpecularMapMaterial::setSpecular(QAbstractTexture *specular)
{
    Q_D(QNormalDiffuseSpecularMapMaterial);
    d->m_specularParameter->setValue(QVariant::fromValue(specular));
}

void QNormalDiffuseSpecularMapMaterial::setShininess(float shininess)
{
    Q_D(QNormalDiffuseSpecularMapMaterial);
    d->m_shininessParameter->setValue(shininess);
}

void QNormalDiffuseSpecularMapMaterial::setTextureScale(float textureScale)
{
    Q_D(QNormalDiffuseSpecularMapMaterial);
    d->m_textureScaleParameter->setValue(textureScale);
}

} // namespace Qt3DExtras

QT_END_NAMESPACE

#include "moc_qnormaldiffusespecularmapmaterial.cpp"
