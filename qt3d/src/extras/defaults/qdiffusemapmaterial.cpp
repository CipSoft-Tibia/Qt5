// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdiffusemapmaterial.h"
#include "qdiffusemapmaterial_p.h"

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

QDiffuseMapMaterialPrivate::QDiffuseMapMaterialPrivate()
    : QMaterialPrivate()
    , m_diffuseMapEffect(new QEffect())
    , m_diffuseTexture(new QTexture2D())
    , m_ambientParameter(new QParameter(QStringLiteral("ka"), QColor::fromRgbF(0.05f, 0.05f, 0.05f, 1.0f)))
    , m_diffuseParameter(new QParameter(QStringLiteral("diffuseTexture"), m_diffuseTexture))
    , m_specularParameter(new QParameter(QStringLiteral("ks"), QColor::fromRgbF(0.01f, 0.01f, 0.01f, 1.0f)))
    , m_shininessParameter(new QParameter(QStringLiteral("shininess"), 150.0f))
    , m_textureScaleParameter(new QParameter(QStringLiteral("texCoordScale"), 1.0f))
    , m_diffuseMapGL3Technique(new QTechnique())
    , m_diffuseMapGL2Technique(new QTechnique())
    , m_diffuseMapES2Technique(new QTechnique())
    , m_diffuseMapRHITechnique(new QTechnique())
    , m_diffuseMapGL3RenderPass(new QRenderPass())
    , m_diffuseMapGL2RenderPass(new QRenderPass())
    , m_diffuseMapES2RenderPass(new QRenderPass())
    , m_diffuseMapRHIRenderPass(new QRenderPass())
    , m_diffuseMapGL3Shader(new QShaderProgram())
    , m_diffuseMapGL3ShaderBuilder(new QShaderProgramBuilder())
    , m_diffuseMapGL2ES2Shader(new QShaderProgram())
    , m_diffuseMapGL2ES2ShaderBuilder(new QShaderProgramBuilder())
    , m_diffuseMapRHIShader(new QShaderProgram())
    , m_diffuseMapRHIShaderBuilder(new QShaderProgramBuilder())
    , m_filterKey(new QFilterKey)
{
    m_diffuseTexture->setMagnificationFilter(QAbstractTexture::Linear);
    m_diffuseTexture->setMinificationFilter(QAbstractTexture::LinearMipMapLinear);
    m_diffuseTexture->setWrapMode(QTextureWrapMode(QTextureWrapMode::Repeat));
    m_diffuseTexture->setGenerateMipMaps(true);
    m_diffuseTexture->setMaximumAnisotropy(16.0f);
}

void QDiffuseMapMaterialPrivate::init()
{
    Q_Q(QDiffuseMapMaterial);

    connect(m_ambientParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QDiffuseMapMaterialPrivate::handleAmbientChanged);
    connect(m_diffuseParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QDiffuseMapMaterialPrivate::handleDiffuseChanged);
    connect(m_specularParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QDiffuseMapMaterialPrivate::handleSpecularChanged);
    connect(m_shininessParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QDiffuseMapMaterialPrivate::handleShininessChanged);
    connect(m_textureScaleParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QDiffuseMapMaterialPrivate::handleTextureScaleChanged);

    m_diffuseMapGL3Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/gl3/default.vert"))));
    m_diffuseMapGL3ShaderBuilder->setParent(q);
    m_diffuseMapGL3ShaderBuilder->setShaderProgram(m_diffuseMapGL3Shader);
    m_diffuseMapGL3ShaderBuilder->setFragmentShaderGraph(QUrl(QStringLiteral("qrc:/shaders/graphs/phong.frag.json")));
    m_diffuseMapGL3ShaderBuilder->setEnabledLayers({QStringLiteral("diffuseTexture"),
                                                    QStringLiteral("specular"),
                                                    QStringLiteral("normal")});

    m_diffuseMapGL2ES2Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/es2/default.vert"))));
    m_diffuseMapGL2ES2ShaderBuilder->setParent(q);
    m_diffuseMapGL2ES2ShaderBuilder->setShaderProgram(m_diffuseMapGL2ES2Shader);
    m_diffuseMapGL2ES2ShaderBuilder->setFragmentShaderGraph(QUrl(QStringLiteral("qrc:/shaders/graphs/phong.frag.json")));
    m_diffuseMapGL2ES2ShaderBuilder->setEnabledLayers({QStringLiteral("diffuseTexture"),
                                                       QStringLiteral("specular"),
                                                       QStringLiteral("normal")});

    m_diffuseMapRHIShader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/rhi/default_pos_norm_tex.vert"))));
    m_diffuseMapRHIShaderBuilder->setParent(q);
    m_diffuseMapRHIShaderBuilder->setShaderProgram(m_diffuseMapRHIShader);
    m_diffuseMapRHIShaderBuilder->setFragmentShaderGraph(QUrl(QStringLiteral("qrc:/shaders/graphs/phong.frag.json")));
    m_diffuseMapRHIShaderBuilder->setEnabledLayers({QStringLiteral("diffuseTexture"),
                                                    QStringLiteral("specular"),
                                                    QStringLiteral("normal")});

    m_diffuseMapGL3Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_diffuseMapGL3Technique->graphicsApiFilter()->setMajorVersion(3);
    m_diffuseMapGL3Technique->graphicsApiFilter()->setMinorVersion(1);
    m_diffuseMapGL3Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::CoreProfile);

    m_diffuseMapGL2Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_diffuseMapGL2Technique->graphicsApiFilter()->setMajorVersion(2);
    m_diffuseMapGL2Technique->graphicsApiFilter()->setMinorVersion(0);
    m_diffuseMapGL2Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::NoProfile);

    m_diffuseMapES2Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGLES);
    m_diffuseMapES2Technique->graphicsApiFilter()->setMajorVersion(2);
    m_diffuseMapES2Technique->graphicsApiFilter()->setMinorVersion(0);
    m_diffuseMapES2Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::NoProfile);

    m_diffuseMapRHITechnique->graphicsApiFilter()->setApi(QGraphicsApiFilter::RHI);
    m_diffuseMapRHITechnique->graphicsApiFilter()->setMajorVersion(1);
    m_diffuseMapRHITechnique->graphicsApiFilter()->setMinorVersion(0);

    m_filterKey->setParent(q);
    m_filterKey->setName(QStringLiteral("renderingStyle"));
    m_filterKey->setValue(QStringLiteral("forward"));

    m_diffuseMapGL3Technique->addFilterKey(m_filterKey);
    m_diffuseMapGL2Technique->addFilterKey(m_filterKey);
    m_diffuseMapES2Technique->addFilterKey(m_filterKey);
    m_diffuseMapRHITechnique->addFilterKey(m_filterKey);

    m_diffuseMapGL3RenderPass->setShaderProgram(m_diffuseMapGL3Shader);
    m_diffuseMapGL2RenderPass->setShaderProgram(m_diffuseMapGL2ES2Shader);
    m_diffuseMapES2RenderPass->setShaderProgram(m_diffuseMapGL2ES2Shader);
    m_diffuseMapRHIRenderPass->setShaderProgram(m_diffuseMapRHIShader);

    m_diffuseMapGL3Technique->addRenderPass(m_diffuseMapGL3RenderPass);
    m_diffuseMapGL2Technique->addRenderPass(m_diffuseMapGL2RenderPass);
    m_diffuseMapES2Technique->addRenderPass(m_diffuseMapES2RenderPass);
    m_diffuseMapRHITechnique->addRenderPass(m_diffuseMapRHIRenderPass);

    m_diffuseMapEffect->addTechnique(m_diffuseMapGL3Technique);
    m_diffuseMapEffect->addTechnique(m_diffuseMapGL2Technique);
    m_diffuseMapEffect->addTechnique(m_diffuseMapES2Technique);
    m_diffuseMapEffect->addTechnique(m_diffuseMapRHITechnique);

    m_diffuseMapEffect->addParameter(m_ambientParameter);
    m_diffuseMapEffect->addParameter(m_diffuseParameter);
    m_diffuseMapEffect->addParameter(m_specularParameter);
    m_diffuseMapEffect->addParameter(m_shininessParameter);
    m_diffuseMapEffect->addParameter(m_textureScaleParameter);

    q->setEffect(m_diffuseMapEffect);
}

void QDiffuseMapMaterialPrivate::handleAmbientChanged(const QVariant &var)
{
    Q_Q(QDiffuseMapMaterial);
    emit q->ambientChanged(var.value<QColor>());
}

void QDiffuseMapMaterialPrivate::handleDiffuseChanged(const QVariant &var)
{
    Q_Q(QDiffuseMapMaterial);
    emit q->diffuseChanged(var.value<QAbstractTexture *>());
}

void QDiffuseMapMaterialPrivate::handleSpecularChanged(const QVariant &var)
{
    Q_Q(QDiffuseMapMaterial);
    emit q->specularChanged(var.value<QColor>());
}

void QDiffuseMapMaterialPrivate::handleShininessChanged(const QVariant &var)
{
    Q_Q(QDiffuseMapMaterial);
    emit q->shininessChanged(var.toFloat());
}

void QDiffuseMapMaterialPrivate::handleTextureScaleChanged(const QVariant &var)
{
    Q_Q(QDiffuseMapMaterial);
    emit q->textureScaleChanged(var.toFloat());
}

/*!
    \class Qt3DExtras::QDiffuseMapMaterial
    \brief The QDiffuseMapMaterial provides a default implementation of the phong lighting effect
    where the diffuse light component is read from a texture map.
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
    \li Diffuse is the color that is emitted for rought surface reflections with the lights.
    \li Specular is the color emitted for shiny surface reflections with the lights.
    \li The shininess of a surface is controlled by a float property.
    \endlist

    This material uses an effect with a single render pass approach and performs per fragment
    lighting. Techniques are provided for OpenGL 2, OpenGL 3 or above as well as OpenGL ES 2.
*/

/*!
    Constructs a new QDiffuseMapMaterial instance with parent object \a parent.
 */
QDiffuseMapMaterial::QDiffuseMapMaterial(QNode *parent)
    : QMaterial(*new QDiffuseMapMaterialPrivate, parent)
{
    Q_D(QDiffuseMapMaterial);
    d->init();
}

/*!
    Destroys the QDiffuseMapMaterial instance.
*/
QDiffuseMapMaterial::~QDiffuseMapMaterial()
{
}

/*!
    \property QDiffuseMapMaterial::ambient

    Holds the current ambient color.
*/

QColor QDiffuseMapMaterial::ambient() const
{
    Q_D(const QDiffuseMapMaterial);
    return d->m_ambientParameter->value().value<QColor>();
}

/*!
    \property QDiffuseMapMaterial::specular

    Holds the current specular color.
*/
QColor QDiffuseMapMaterial::specular() const
{
    Q_D(const QDiffuseMapMaterial);
    return d->m_specularParameter->value().value<QColor>();
}

/*!
    \property QDiffuseMapMaterial::shininess

    Holds the current shininess as a float value.
*/
float QDiffuseMapMaterial::shininess() const
{
    Q_D(const QDiffuseMapMaterial);
    return d->m_shininessParameter->value().toFloat();
}

/*!
    \property QDiffuseMapMaterial::diffuse

    Holds the current texture used as the diffuse map.

    By default, the diffuse texture has the following properties:

    \list
        \li Linear minification and magnification filters
        \li Linear mipmap with mipmapping enabled
        \li Repeat wrap mode
        \li Maximum anisotropy of 16.0
    \endlist
*/
QAbstractTexture *QDiffuseMapMaterial::diffuse() const
{
    Q_D(const QDiffuseMapMaterial);
    return d->m_diffuseParameter->value().value<QAbstractTexture *>();
}

/*!
    \property QDiffuseMapMaterial::textureScale

    Holds the current texture scale. It is applied as a multiplier to texture
    coordinates at render time. Defaults to 1.0.

    When used in conjunction with QTextureWrapMode::Repeat, textureScale provides a simple
    way to tile a texture across a surface. For example, a texture scale of \c 4.0
    would result in 16 (4x4) tiles.
*/
float QDiffuseMapMaterial::textureScale() const
{
    Q_D(const QDiffuseMapMaterial);
    return d->m_textureScaleParameter->value().toFloat();
}

void QDiffuseMapMaterial::setAmbient(const QColor &ambient)
{
    Q_D(const QDiffuseMapMaterial);
    d->m_ambientParameter->setValue(ambient);
}

void QDiffuseMapMaterial::setSpecular(const QColor &specular)
{
    Q_D(QDiffuseMapMaterial);
    d->m_specularParameter->setValue(specular);
}

void QDiffuseMapMaterial::setShininess(float shininess)
{
    Q_D(QDiffuseMapMaterial);
    d->m_shininessParameter->setValue(shininess);
}

void QDiffuseMapMaterial::setDiffuse(QAbstractTexture *diffuseMap)
{
    Q_D(QDiffuseMapMaterial);
    d->m_diffuseParameter->setValue(QVariant::fromValue(diffuseMap));
}

void QDiffuseMapMaterial::setTextureScale(float textureScale)
{
    Q_D(QDiffuseMapMaterial);
    d->m_textureScaleParameter->setValue(textureScale);
}

} // namespace Qt3DExtras

QT_END_NAMESPACE

#include "moc_qdiffusemapmaterial.cpp"
