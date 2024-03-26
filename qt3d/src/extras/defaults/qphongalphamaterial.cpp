// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qphongalphamaterial.h"
#include "qphongalphamaterial_p.h"

#include <Qt3DRender/qfilterkey.h>
#include <Qt3DRender/qmaterial.h>
#include <Qt3DRender/qeffect.h>
#include <Qt3DRender/qtechnique.h>
#include <Qt3DRender/qshaderprogram.h>
#include <Qt3DRender/qshaderprogrambuilder.h>
#include <Qt3DRender/qparameter.h>
#include <Qt3DRender/qrenderpass.h>
#include <Qt3DRender/qgraphicsapifilter.h>
#include <Qt3DRender/qblendequation.h>
#include <Qt3DRender/qblendequationarguments.h>
#include <Qt3DRender/qnodepthmask.h>
#include <QtCore/QUrl>
#include <QtGui/QVector3D>
#include <QtGui/QVector4D>

QT_BEGIN_NAMESPACE

using namespace Qt3DRender;

namespace Qt3DExtras {

QPhongAlphaMaterialPrivate::QPhongAlphaMaterialPrivate()
    : QMaterialPrivate()
    , m_phongEffect(new QEffect())
    , m_ambientParameter(new QParameter(QStringLiteral("ka"), QColor::fromRgbF(0.05f, 0.05f, 0.05f, 1.0f)))
    , m_diffuseParameter(new QParameter(QStringLiteral("kd"), QColor::fromRgbF(0.7f, 0.7f, 0.7f, 0.5f)))
    , m_specularParameter(new QParameter(QStringLiteral("ks"), QColor::fromRgbF(0.01f, 0.01f, 0.01f, 1.0f)))
    , m_shininessParameter(new QParameter(QStringLiteral("shininess"), 150.0f))
    , m_phongAlphaGL3Technique(new QTechnique())
    , m_phongAlphaGL2Technique(new QTechnique())
    , m_phongAlphaES2Technique(new QTechnique())
    , m_phongAlphaRHITechnique(new QTechnique())
    , m_phongAlphaGL3RenderPass(new QRenderPass())
    , m_phongAlphaGL2RenderPass(new QRenderPass())
    , m_phongAlphaES2RenderPass(new QRenderPass())
    , m_phongAlphaRHIRenderPass(new QRenderPass())
    , m_phongAlphaGL3Shader(new QShaderProgram())
    , m_phongAlphaGL3ShaderBuilder(new QShaderProgramBuilder())
    , m_phongAlphaGL2ES2Shader(new QShaderProgram())
    , m_phongAlphaGL2ES2ShaderBuilder(new QShaderProgramBuilder())
    , m_phongAlphaRHIShader(new QShaderProgram())
    , m_phongAlphaRHIShaderBuilder(new QShaderProgramBuilder())
    , m_noDepthMask(new QNoDepthMask())
    , m_blendState(new QBlendEquationArguments())
    , m_blendEquation(new QBlendEquation())
    , m_filterKey(new QFilterKey)
{
}

// TODO: Define how lights are properties are set in the shaders. Ideally using a QShaderData
void QPhongAlphaMaterialPrivate::init()
{
    Q_Q(QPhongAlphaMaterial);

    connect(m_ambientParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QPhongAlphaMaterialPrivate::handleAmbientChanged);
    connect(m_diffuseParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QPhongAlphaMaterialPrivate::handleDiffuseChanged);
    connect(m_specularParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QPhongAlphaMaterialPrivate::handleSpecularChanged);
    connect(m_shininessParameter, &Qt3DRender::QParameter::valueChanged,
            this, &QPhongAlphaMaterialPrivate::handleShininessChanged);

    m_phongAlphaGL3Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/gl3/default.vert"))));
    m_phongAlphaGL3ShaderBuilder->setParent(q);
    m_phongAlphaGL3ShaderBuilder->setShaderProgram(m_phongAlphaGL3Shader);
    m_phongAlphaGL3ShaderBuilder->setFragmentShaderGraph(QUrl(QStringLiteral("qrc:/shaders/graphs/phong.frag.json")));
    m_phongAlphaGL3ShaderBuilder->setEnabledLayers({QStringLiteral("diffuse"),
                                                    QStringLiteral("specular"),
                                                    QStringLiteral("normal")});

    m_phongAlphaGL2ES2Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/es2/default.vert"))));
    m_phongAlphaGL2ES2ShaderBuilder->setParent(q);
    m_phongAlphaGL2ES2ShaderBuilder->setShaderProgram(m_phongAlphaGL2ES2Shader);
    m_phongAlphaGL2ES2ShaderBuilder->setFragmentShaderGraph(QUrl(QStringLiteral("qrc:/shaders/graphs/phong.frag.json")));
    m_phongAlphaGL2ES2ShaderBuilder->setEnabledLayers({QStringLiteral("diffuse"),
                                                       QStringLiteral("specular"),
                                                       QStringLiteral("normal")});

    m_phongAlphaRHIShader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/rhi/default_pos_norm.vert"))));
    m_phongAlphaRHIShaderBuilder->setParent(q);
    m_phongAlphaRHIShaderBuilder->setShaderProgram(m_phongAlphaRHIShader);
    m_phongAlphaRHIShaderBuilder->setFragmentShaderGraph(QUrl(QStringLiteral("qrc:/shaders/graphs/phong.frag.json")));
    m_phongAlphaRHIShaderBuilder->setEnabledLayers({QStringLiteral("diffuse"),
                                                       QStringLiteral("specular"),
                                                       QStringLiteral("normal")});

    m_phongAlphaGL3Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_phongAlphaGL3Technique->graphicsApiFilter()->setMajorVersion(3);
    m_phongAlphaGL3Technique->graphicsApiFilter()->setMinorVersion(1);
    m_phongAlphaGL3Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::CoreProfile);

    m_phongAlphaGL2Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_phongAlphaGL2Technique->graphicsApiFilter()->setMajorVersion(2);
    m_phongAlphaGL2Technique->graphicsApiFilter()->setMinorVersion(0);
    m_phongAlphaGL2Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::NoProfile);

    m_phongAlphaES2Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGLES);
    m_phongAlphaES2Technique->graphicsApiFilter()->setMajorVersion(2);
    m_phongAlphaES2Technique->graphicsApiFilter()->setMinorVersion(0);
    m_phongAlphaES2Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::NoProfile);

    m_phongAlphaRHITechnique->graphicsApiFilter()->setApi(QGraphicsApiFilter::RHI);
    m_phongAlphaRHITechnique->graphicsApiFilter()->setMajorVersion(1);
    m_phongAlphaRHITechnique->graphicsApiFilter()->setMinorVersion(0);

    m_filterKey->setParent(q);
    m_filterKey->setName(QStringLiteral("renderingStyle"));
    m_filterKey->setValue(QStringLiteral("forward"));

    m_phongAlphaGL3Technique->addFilterKey(m_filterKey);
    m_phongAlphaGL2Technique->addFilterKey(m_filterKey);
    m_phongAlphaES2Technique->addFilterKey(m_filterKey);
    m_phongAlphaRHITechnique->addFilterKey(m_filterKey);

    m_blendState->setSourceRgb(QBlendEquationArguments::SourceAlpha);
    m_blendState->setDestinationRgb(QBlendEquationArguments::OneMinusSourceAlpha);
    m_blendEquation->setBlendFunction(QBlendEquation::Add);

    m_phongAlphaGL3RenderPass->setShaderProgram(m_phongAlphaGL3Shader);
    m_phongAlphaGL2RenderPass->setShaderProgram(m_phongAlphaGL2ES2Shader);
    m_phongAlphaES2RenderPass->setShaderProgram(m_phongAlphaGL2ES2Shader);
    m_phongAlphaRHIRenderPass->setShaderProgram(m_phongAlphaRHIShader);

    m_phongAlphaGL3RenderPass->addRenderState(m_noDepthMask);
    m_phongAlphaGL3RenderPass->addRenderState(m_blendState);
    m_phongAlphaGL3RenderPass->addRenderState(m_blendEquation);

    m_phongAlphaGL2RenderPass->addRenderState(m_noDepthMask);
    m_phongAlphaGL2RenderPass->addRenderState(m_blendState);
    m_phongAlphaGL2RenderPass->addRenderState(m_blendEquation);

    m_phongAlphaES2RenderPass->addRenderState(m_noDepthMask);
    m_phongAlphaES2RenderPass->addRenderState(m_blendState);
    m_phongAlphaES2RenderPass->addRenderState(m_blendEquation);

    m_phongAlphaRHIRenderPass->addRenderState(m_noDepthMask);
    m_phongAlphaRHIRenderPass->addRenderState(m_blendState);
    m_phongAlphaRHIRenderPass->addRenderState(m_blendEquation);

    m_phongAlphaGL3Technique->addRenderPass(m_phongAlphaGL3RenderPass);
    m_phongAlphaGL2Technique->addRenderPass(m_phongAlphaGL2RenderPass);
    m_phongAlphaES2Technique->addRenderPass(m_phongAlphaES2RenderPass);
    m_phongAlphaRHITechnique->addRenderPass(m_phongAlphaRHIRenderPass);

    m_phongEffect->addTechnique(m_phongAlphaGL3Technique);
    m_phongEffect->addTechnique(m_phongAlphaGL2Technique);
    m_phongEffect->addTechnique(m_phongAlphaES2Technique);
    m_phongEffect->addTechnique(m_phongAlphaRHITechnique);

    m_phongEffect->addParameter(m_ambientParameter);
    m_phongEffect->addParameter(m_diffuseParameter);
    m_phongEffect->addParameter(m_specularParameter);
    m_phongEffect->addParameter(m_shininessParameter);

    q->setEffect(m_phongEffect);
}

void QPhongAlphaMaterialPrivate::handleAmbientChanged(const QVariant &var)
{
    Q_Q(QPhongAlphaMaterial);
    emit q->ambientChanged(var.value<QColor>());
}

void QPhongAlphaMaterialPrivate::handleDiffuseChanged(const QVariant &var)
{
    Q_Q(QPhongAlphaMaterial);
    emit q->diffuseChanged(var.value<QColor>());
    emit q->alphaChanged(var.value<QColor>().alphaF());
}

void QPhongAlphaMaterialPrivate::handleSpecularChanged(const QVariant &var)
{
    Q_Q(QPhongAlphaMaterial);
    emit q->specularChanged(var.value<QColor>());
}

void QPhongAlphaMaterialPrivate::handleShininessChanged(const QVariant &var)
{
    Q_Q(QPhongAlphaMaterial);
    emit q->shininessChanged(var.toFloat());
}

/*!
    \class Qt3DExtras::QPhongAlphaMaterial

    \brief The QPhongAlphaMaterial class provides a default implementation of
    the phong lighting effect with alpha.
    \inmodule Qt3DExtras
    \since 5.7
    \inherits Qt3DRender::QMaterial

    \deprecated
    This class is deprecated; use QDiffuseSpecularMaterial instead.

    The phong lighting effect is based on the combination of 3 lighting components ambient, diffuse
    and specular. The relative strengths of these components are controlled by means of their
    reflectivity coefficients which are modelled as RGB triplets:

    \list
    \li Ambient is the color that is emitted by an object without any other light source.
    \li Diffuse is the color that is emitted for rought surface reflections with the lights.
    \li Specular is the color emitted for shiny surface reflections with the lights.
    \li The shininess of a surface is controlled by a float property.
    \li Alpha is the transparency of the surface between 0 (fully transparent) and 1 (opaque).
    \endlist

    This material uses an effect with a single render pass approach and performs per fragment
    lighting. Techniques are provided for OpenGL 2, OpenGL 3 or above as well as OpenGL ES 2.
*/

/*!
    Constructs a new QPhongAlphaMaterial instance with parent object \a parent.
*/
QPhongAlphaMaterial::QPhongAlphaMaterial(QNode *parent)
    : QMaterial(*new QPhongAlphaMaterialPrivate, parent)
{
    Q_D(QPhongAlphaMaterial);
    d->init();

    QObject::connect(d->m_blendEquation, &Qt3DRender::QBlendEquation::blendFunctionChanged,
                     this, &QPhongAlphaMaterial::blendFunctionArgChanged);
    QObject::connect(d->m_blendState, &Qt3DRender::QBlendEquationArguments::destinationAlphaChanged,
                     this, &QPhongAlphaMaterial::destinationAlphaArgChanged);
    QObject::connect(d->m_blendState, &Qt3DRender::QBlendEquationArguments::destinationRgbChanged,
                     this, &QPhongAlphaMaterial::destinationRgbArgChanged);
    QObject::connect(d->m_blendState, &Qt3DRender::QBlendEquationArguments::sourceAlphaChanged,
                     this, &QPhongAlphaMaterial::sourceAlphaArgChanged);
    QObject::connect(d->m_blendState, &Qt3DRender::QBlendEquationArguments::sourceRgbChanged,
                     this, &QPhongAlphaMaterial::sourceRgbArgChanged);
}

/*!
   Destroys the QPhongAlphaMaterial.
*/
QPhongAlphaMaterial::~QPhongAlphaMaterial()
{
}

/*!
    \property QPhongAlphaMaterial::ambient

    Holds the ambient color.
*/
QColor QPhongAlphaMaterial::ambient() const
{
    Q_D(const QPhongAlphaMaterial);
    return d->m_ambientParameter->value().value<QColor>();
}

/*!
    \property QPhongAlphaMaterial::diffuse

    Holds the diffuse color.
*/
QColor QPhongAlphaMaterial::diffuse() const
{
    Q_D(const QPhongAlphaMaterial);
    return d->m_diffuseParameter->value().value<QColor>();
}

/*!
    \property QPhongAlphaMaterial::specular

    Holds the specular color.
*/
QColor QPhongAlphaMaterial::specular() const
{
    Q_D(const QPhongAlphaMaterial);
    return d->m_specularParameter->value().value<QColor>();
}

/*!
    \property QPhongAlphaMaterial::shininess

    Holds the shininess exponent.
*/
float QPhongAlphaMaterial::shininess() const
{
    Q_D(const QPhongAlphaMaterial);
    return d->m_shininessParameter->value().toFloat();
}

/*!
    \property QPhongAlphaMaterial::alpha

    Holds the alpha component of the object which varies between 0 and 1.

    The default value is 0.5f.
*/
float QPhongAlphaMaterial::alpha() const
{
    Q_D(const QPhongAlphaMaterial);
    return d->m_diffuseParameter->value().value<QColor>().alphaF();
}

/*!
    \property QPhongAlphaMaterial::sourceRgbArg

    Holds the blend equation source RGB blending argument.

    \sa Qt3DRender::QBlendEquationArguments::Blending
*/
QBlendEquationArguments::Blending QPhongAlphaMaterial::sourceRgbArg() const
{
    Q_D(const QPhongAlphaMaterial);
    return d->m_blendState->sourceRgb();
}

/*!
    \property QPhongAlphaMaterial::destinationRgbArg

    Holds the blend equation destination RGB blending argument.

    \sa Qt3DRender::QBlendEquationArguments::Blending
*/
QBlendEquationArguments::Blending QPhongAlphaMaterial::destinationRgbArg() const
{
    Q_D(const QPhongAlphaMaterial);
    return d->m_blendState->destinationRgb();
}

/*!
    \property QPhongAlphaMaterial::sourceAlphaArg

    Holds the blend equation source alpha blending argument.

    \sa Qt3DRender::QBlendEquationArguments::Blending
*/
QBlendEquationArguments::Blending QPhongAlphaMaterial::sourceAlphaArg() const
{
    Q_D(const QPhongAlphaMaterial);
    return d->m_blendState->sourceAlpha();
}

/*!
    \property QPhongAlphaMaterial::destinationAlphaArg

    Holds the blend equation destination alpha blending argument.

    \sa Qt3DRender::QBlendEquationArguments::Blending
*/
QBlendEquationArguments::Blending QPhongAlphaMaterial::destinationAlphaArg() const
{
    Q_D(const QPhongAlphaMaterial);
    return d->m_blendState->destinationAlpha();
}

/*!
    \property QPhongAlphaMaterial::blendFunctionArg

    Holds the blend equation function argument.

    \sa Qt3DRender::QBlendEquation::BlendFunction
*/
QBlendEquation::BlendFunction QPhongAlphaMaterial::blendFunctionArg() const
{
    Q_D(const QPhongAlphaMaterial);
    return d->m_blendEquation->blendFunction();
}

void QPhongAlphaMaterial::setAmbient(const QColor &ambient)
{
    Q_D(QPhongAlphaMaterial);
    d->m_ambientParameter->setValue(ambient);
}

void QPhongAlphaMaterial::setDiffuse(const QColor &diffuse)
{
    Q_D(QPhongAlphaMaterial);
    QColor currentDiffuse = d->m_diffuseParameter->value().value<QColor>();
    QColor newDiffuse = diffuse;
    newDiffuse.setAlphaF(currentDiffuse.alphaF());
    d->m_diffuseParameter->setValue(newDiffuse);
}

void QPhongAlphaMaterial::setSpecular(const QColor &specular)
{
    Q_D(QPhongAlphaMaterial);
    d->m_specularParameter->setValue(specular);
}

void QPhongAlphaMaterial::setShininess(float shininess)
{
    Q_D(QPhongAlphaMaterial);
    d->m_shininessParameter->setValue(shininess);
}

void QPhongAlphaMaterial::setAlpha(float alpha)
{
    Q_D(QPhongAlphaMaterial);
    QColor diffuse = d->m_diffuseParameter->value().value<QColor>();
    diffuse.setAlphaF(alpha);
    d->m_diffuseParameter->setValue(diffuse);
}

void QPhongAlphaMaterial::setSourceRgbArg(QBlendEquationArguments::Blending sourceRgbArg)
{
    Q_D(QPhongAlphaMaterial);
    d->m_blendState->setSourceRgb(sourceRgbArg);
}

void QPhongAlphaMaterial::setDestinationRgbArg(QBlendEquationArguments::Blending destinationRgbArg)
{
    Q_D(QPhongAlphaMaterial);
    d->m_blendState->setDestinationRgb(destinationRgbArg);
}

void QPhongAlphaMaterial::setSourceAlphaArg(QBlendEquationArguments::Blending sourceAlphaArg)
{
    Q_D(QPhongAlphaMaterial);
    d->m_blendState->setSourceAlpha(sourceAlphaArg);
}

void QPhongAlphaMaterial::setDestinationAlphaArg(QBlendEquationArguments::Blending destinationAlphaArg)
{
    Q_D(QPhongAlphaMaterial);
    d->m_blendState->setDestinationAlpha(destinationAlphaArg);
}

void QPhongAlphaMaterial::setBlendFunctionArg(QBlendEquation::BlendFunction blendFunctionArg)
{
    Q_D(QPhongAlphaMaterial);
    d->m_blendEquation->setBlendFunction(blendFunctionArg);
}

} // namespace Qt3DExtras

QT_END_NAMESPACE

#include "moc_qphongalphamaterial.cpp"
