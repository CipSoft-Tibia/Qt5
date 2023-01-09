/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgtexturematerial_p.h"
#include <private/qsgtexture_p.h>
#if QT_CONFIG(opengl)
# include <QtGui/qopenglshaderprogram.h>
# include <QtGui/qopenglfunctions.h>
#endif
#include <QtGui/private/qrhi_p.h>

QT_BEGIN_NAMESPACE

inline static bool isPowerOfTwo(int x)
{
    // Assumption: x >= 1
    return x == (x & -x);
}

QSGOpaqueTextureMaterialShader::QSGOpaqueTextureMaterialShader()
{
#if QT_CONFIG(opengl)
    setShaderSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/qt-project.org/scenegraph/shaders/opaquetexture.vert"));
    setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qt-project.org/scenegraph/shaders/opaquetexture.frag"));
#endif
}

char const *const *QSGOpaqueTextureMaterialShader::attributeNames() const
{
    static char const *const attr[] = { "qt_VertexPosition", "qt_VertexTexCoord", nullptr };
    return attr;
}

void QSGOpaqueTextureMaterialShader::initialize()
{
#if QT_CONFIG(opengl)
    m_matrix_id = program()->uniformLocation("qt_Matrix");
#endif
}

void QSGOpaqueTextureMaterialShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    Q_ASSERT(oldEffect == nullptr || newEffect->type() == oldEffect->type());
    QSGOpaqueTextureMaterial *tx = static_cast<QSGOpaqueTextureMaterial *>(newEffect);
    QSGOpaqueTextureMaterial *oldTx = static_cast<QSGOpaqueTextureMaterial *>(oldEffect);

    QSGTexture *t = tx->texture();

#ifndef QT_NO_DEBUG
    if (!qsg_safeguard_texture(t))
        return;
#endif

    t->setFiltering(tx->filtering());

    t->setHorizontalWrapMode(tx->horizontalWrapMode());
    t->setVerticalWrapMode(tx->verticalWrapMode());
#if QT_CONFIG(opengl)
    bool npotSupported = const_cast<QOpenGLContext *>(state.context())
        ->functions()->hasOpenGLFeature(QOpenGLFunctions::NPOTTextureRepeat);
    if (!npotSupported) {
        QSize size = t->textureSize();
        const bool isNpot = !isPowerOfTwo(size.width()) || !isPowerOfTwo(size.height());
        if (isNpot) {
            t->setHorizontalWrapMode(QSGTexture::ClampToEdge);
            t->setVerticalWrapMode(QSGTexture::ClampToEdge);
        }
    }
#else
    Q_UNUSED(state)
#endif
    t->setMipmapFiltering(tx->mipmapFiltering());
    t->setAnisotropyLevel(tx->anisotropyLevel());

    if (oldTx == nullptr || oldTx->texture()->textureId() != t->textureId())
        t->bind();
    else
        t->updateBindOptions();
#if QT_CONFIG(opengl)
    if (state.isMatrixDirty())
        program()->setUniformValue(m_matrix_id, state.combinedMatrix());
#endif
}


QSGOpaqueTextureMaterialRhiShader::QSGOpaqueTextureMaterialRhiShader()
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/opaquetexture.vert.qsb"));
    setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/opaquetexture.frag.qsb"));
}

bool QSGOpaqueTextureMaterialRhiShader::updateUniformData(RenderState &state, QSGMaterial *, QSGMaterial *)
{
    bool changed = false;
    QByteArray *buf = state.uniformData();

    if (state.isMatrixDirty()) {
        const QMatrix4x4 m = state.combinedMatrix();
        memcpy(buf->data(), m.constData(), 64);
        changed = true;
    }

    return changed;
}

void QSGOpaqueTextureMaterialRhiShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                                           QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    if (binding != 1)
        return;

#ifdef QT_NO_DEBUG
    Q_UNUSED(oldMaterial);
#endif
    Q_ASSERT(oldMaterial == nullptr || newMaterial->type() == oldMaterial->type());
    QSGOpaqueTextureMaterial *tx = static_cast<QSGOpaqueTextureMaterial *>(newMaterial);
    QSGTexture *t = tx->texture();

    t->setFiltering(tx->filtering());
    t->setMipmapFiltering(tx->mipmapFiltering());
    t->setAnisotropyLevel(tx->anisotropyLevel());

    t->setHorizontalWrapMode(tx->horizontalWrapMode());
    t->setVerticalWrapMode(tx->verticalWrapMode());
    if (!state.rhi()->isFeatureSupported(QRhi::NPOTTextureRepeat)) {
        QSize size = t->textureSize();
        const bool isNpot = !isPowerOfTwo(size.width()) || !isPowerOfTwo(size.height());
        if (isNpot) {
            t->setHorizontalWrapMode(QSGTexture::ClampToEdge);
            t->setVerticalWrapMode(QSGTexture::ClampToEdge);
            t->setMipmapFiltering(QSGTexture::None);
        }
    }

    t->updateRhiTexture(state.rhi(), state.resourceUpdateBatch());
    *texture = t;
}


/*!
    \class QSGOpaqueTextureMaterial
    \brief The QSGOpaqueTextureMaterial class provides a convenient way of
    rendering textured geometry in the scene graph.
    \inmodule QtQuick
    \ingroup qtquick-scenegraph-materials

    \warning This utility class is only functional when running with the
    default backend of the Qt Quick scenegraph.

    The opaque textured material will fill every pixel in a geometry with
    the supplied texture. The material does not respect the opacity of the
    QSGMaterialShader::RenderState, so opacity nodes in the parent chain
    of nodes using this material, have no effect.

    The geometry to be rendered with an opaque texture material requires
    vertices in attribute location 0 and texture coordinates in attribute
    location 1. The texture coordinate is a 2-dimensional floating-point
    tuple. The QSGGeometry::defaultAttributes_TexturedPoint2D returns an
    attribute set compatible with this material.

    The texture to be rendered can be set using setTexture(). How the
    texture should be rendered can be specified using setMipmapFiltering(),
    setFiltering(), setHorizontalWrapMode() and setVerticalWrapMode().
    The rendering state is set on the texture instance just before it
    is bound.

    The opaque textured material respects the current matrix and the alpha
    channel of the texture. It will disregard the accumulated opacity in
    the scenegraph.

    A texture material must have a texture set before it is used as
    a material in the scene graph.
 */



/*!
    Creates a new QSGOpaqueTextureMaterial.

    The default mipmap filtering and filtering mode is set to
    QSGTexture::Nearest. The default wrap modes is set to
    \c QSGTexture::ClampToEdge.

 */
QSGOpaqueTextureMaterial::QSGOpaqueTextureMaterial()
    : m_texture(nullptr)
    , m_filtering(QSGTexture::Nearest)
    , m_mipmap_filtering(QSGTexture::None)
    , m_horizontal_wrap(QSGTexture::ClampToEdge)
    , m_vertical_wrap(QSGTexture::ClampToEdge)
    , m_anisotropy_level(QSGTexture::AnisotropyNone)
{
    setFlag(SupportsRhiShader, true);
}


/*!
    \internal
 */
QSGMaterialType *QSGOpaqueTextureMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

/*!
    \internal
 */
QSGMaterialShader *QSGOpaqueTextureMaterial::createShader() const
{
    if (flags().testFlag(RhiShaderWanted))
        return new QSGOpaqueTextureMaterialRhiShader;
    else
        return new QSGOpaqueTextureMaterialShader;
}


/*!
    \fn QSGTexture *QSGOpaqueTextureMaterial::texture() const

    Returns this texture material's texture.
 */



/*!
    Sets the texture of this material to \a texture.

    The material does not take ownership of the texture.
 */

void QSGOpaqueTextureMaterial::setTexture(QSGTexture *texture)
{
    m_texture = texture;
    setFlag(Blending, m_texture ? m_texture->hasAlphaChannel() : false);
}



/*!
    \fn void QSGOpaqueTextureMaterial::setMipmapFiltering(QSGTexture::Filtering filtering)

    Sets the mipmap mode to \a filtering.

    The mipmap filtering mode is set on the texture instance just before the
    texture is bound for rendering.

    If the texture does not have mipmapping support, enabling mipmapping has no
    effect.
 */



/*!
    \fn QSGTexture::Filtering QSGOpaqueTextureMaterial::mipmapFiltering() const

    Returns this material's mipmap filtering mode.

    The default mipmap mode is \c QSGTexture::Nearest.
 */



/*!
    \fn void QSGOpaqueTextureMaterial::setFiltering(QSGTexture::Filtering filtering)

    Sets the filtering to \a filtering.

    The filtering mode is set on the texture instance just before the texture
    is bound for rendering.
 */



/*!
    \fn QSGTexture::Filtering QSGOpaqueTextureMaterial::filtering() const

    Returns this material's filtering mode.

    The default filtering is \c QSGTexture::Nearest.
 */



/*!
    \fn void QSGOpaqueTextureMaterial::setHorizontalWrapMode(QSGTexture::WrapMode mode)

    Sets the horizontal wrap mode to \a mode.

    The horizontal wrap mode is set on the texture instance just before the texture
    is bound for rendering.
 */



 /*!
     \fn QSGTexture::WrapMode QSGOpaqueTextureMaterial::horizontalWrapMode() const

     Returns this material's horizontal wrap mode.

     The default horizontal wrap mode is \c QSGTexture::ClampToEdge.
  */



/*!
    \fn void QSGOpaqueTextureMaterial::setVerticalWrapMode(QSGTexture::WrapMode mode)

    Sets the vertical wrap mode to \a mode.

    The vertical wrap mode is set on the texture instance just before the texture
    is bound for rendering.
 */



 /*!
     \fn QSGTexture::WrapMode QSGOpaqueTextureMaterial::verticalWrapMode() const

     Returns this material's vertical wrap mode.

     The default vertical wrap mode is \c QSGTexture::ClampToEdge.
  */

/*!
  \fn void QSGOpaqueTextureMaterial::setAnisotropyLevel(QSGTexture::AnisotropyLevel level)

  Sets this material's anistropy level to \a level.
*/

/*!
  \fn QSGTexture::AnisotropyLevel QSGOpaqueTextureMaterial::anisotropyLevel() const

  Returns this material's anistropy level.
*/

/*!
    \internal
 */

int QSGOpaqueTextureMaterial::compare(const QSGMaterial *o) const
{
    Q_ASSERT(o && type() == o->type());
    const QSGOpaqueTextureMaterial *other = static_cast<const QSGOpaqueTextureMaterial *>(o);
    if (int diff = m_texture->comparisonKey() - other->texture()->comparisonKey())
        return diff;
    return int(m_filtering) - int(other->m_filtering);
}



/*!
    \class QSGTextureMaterial
    \brief The QSGTextureMaterial class provides a convenient way of
    rendering textured geometry in the scene graph.
    \inmodule QtQuick
    \ingroup qtquick-scenegraph-materials

    \warning This utility class is only functional when running with the
    default backend of the Qt Quick scenegraph.

    The textured material will fill every pixel in a geometry with
    the supplied texture.

    The geometry to be rendered with a texture material requires
    vertices in attribute location 0 and texture coordinates in attribute
    location 1. The texture coordinate is a 2-dimensional floating-point
    tuple. The QSGGeometry::defaultAttributes_TexturedPoint2D returns an
    attribute set compatible with this material.

    The texture to be rendered can be set using setTexture(). How the
    texture should be rendered can be specified using setMipmapFiltering(),
    setFiltering(), setHorizontalWrapMode() and setVerticalWrapMode().
    The rendering state is set on the texture instance just before it
    is bound.

    The textured material respects the current matrix and the alpha
    channel of the texture. It will also respect the accumulated opacity
    in the scenegraph.

    A texture material must have a texture set before it is used as
    a material in the scene graph.
 */

/*!
    \internal
 */

QSGMaterialType *QSGTextureMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

/*!
    \internal
 */

QSGMaterialShader *QSGTextureMaterial::createShader() const
{
    if (flags().testFlag(RhiShaderWanted))
        return new QSGTextureMaterialRhiShader;
    else
        return new QSGTextureMaterialShader;
}


QSGTextureMaterialShader::QSGTextureMaterialShader()
{
#if QT_CONFIG(opengl)
    setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qt-project.org/scenegraph/shaders/texture.frag"));
#endif
}

void QSGTextureMaterialShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    Q_ASSERT(oldEffect == nullptr || newEffect->type() == oldEffect->type());
#if QT_CONFIG(opengl)
    if (state.isOpacityDirty())
        program()->setUniformValue(m_opacity_id, state.opacity());
#endif
    QSGOpaqueTextureMaterialShader::updateState(state, newEffect, oldEffect);
}

void QSGTextureMaterialShader::initialize()
{
    QSGOpaqueTextureMaterialShader::initialize();
#if QT_CONFIG(opengl)
    m_opacity_id = program()->uniformLocation("opacity");
#endif
}


QSGTextureMaterialRhiShader::QSGTextureMaterialRhiShader()
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/texture.vert.qsb"));
    setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/texture.frag.qsb"));
}

bool QSGTextureMaterialRhiShader::updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = false;
    QByteArray *buf = state.uniformData();

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(buf->data() + 64, &opacity, 4);
        changed = true;
    }

    changed |= QSGOpaqueTextureMaterialRhiShader::updateUniformData(state, newMaterial, oldMaterial);

    return changed;
}

QT_END_NAMESPACE
