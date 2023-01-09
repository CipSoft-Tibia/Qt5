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

#include "qsgvertexcolormaterial.h"
#if QT_CONFIG(opengl)
# include <qopenglshaderprogram.h>
#endif
QT_BEGIN_NAMESPACE

class QSGVertexColorMaterialShader : public QSGMaterialShader
{
public:
    QSGVertexColorMaterialShader();

    void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;
    char const *const *attributeNames() const override;

private:
    void initialize() override;
#if QT_CONFIG(opengl)
    int m_matrix_id;
    int m_opacity_id;
#endif
};

QSGVertexColorMaterialShader::QSGVertexColorMaterialShader()
{
#if QT_CONFIG(opengl)
    setShaderSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/qt-project.org/scenegraph/shaders/vertexcolor.vert"));
    setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qt-project.org/scenegraph/shaders/vertexcolor.frag"));
#endif
}

void QSGVertexColorMaterialShader::updateState(const RenderState &state, QSGMaterial * /*newEffect*/, QSGMaterial *)
{
#if QT_CONFIG(opengl)
    if (state.isOpacityDirty())
        program()->setUniformValue(m_opacity_id, state.opacity());

    if (state.isMatrixDirty())
        program()->setUniformValue(m_matrix_id, state.combinedMatrix());
#else
    Q_UNUSED(state)
#endif
}

char const *const *QSGVertexColorMaterialShader::attributeNames() const
{
    static const char *const attr[] = { "vertexCoord", "vertexColor", nullptr };
    return attr;
}

void QSGVertexColorMaterialShader::initialize()
{
#if QT_CONFIG(opengl)
    m_matrix_id = program()->uniformLocation("matrix");
    m_opacity_id = program()->uniformLocation("opacity");
#endif
}


class QSGVertexColorMaterialRhiShader : public QSGMaterialRhiShader
{
public:
    QSGVertexColorMaterialRhiShader();

    bool updateUniformData(RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;
};

QSGVertexColorMaterialRhiShader::QSGVertexColorMaterialRhiShader()
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/vertexcolor.vert.qsb"));
    setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/vertexcolor.frag.qsb"));
}

bool QSGVertexColorMaterialRhiShader::updateUniformData(RenderState &state,
                                                        QSGMaterial * /*newEffect*/,
                                                        QSGMaterial * /*oldEffect*/)
{
    bool changed = false;
    QByteArray *buf = state.uniformData();

    if (state.isMatrixDirty()) {
        const QMatrix4x4 m = state.combinedMatrix();
        memcpy(buf->data(), m.constData(), 64);
        changed = true;
    }

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(buf->data() + 64, &opacity, 4);
        changed = true;
    }

    return changed;
}


/*!
    \class QSGVertexColorMaterial
    \brief The QSGVertexColorMaterial class provides a convenient way of rendering per-vertex
    colored geometry in the scene graph.

    \inmodule QtQuick
    \ingroup qtquick-scenegraph-materials

    \warning This utility class is only functional when running with the
    default backend of the Qt Quick scenegraph.

    The vertex color material will give each vertex in a geometry a color. Pixels between
    vertices will be linearly interpolated. The colors can contain transparency.

    The geometry to be rendered with vertex color must have the following layout. Attribute
    position 0 must contain vertices. Attribute position 1 must contain colors, a tuple of
    4 values with RGBA layout. Both floats in the range of 0 to 1 and unsigned bytes in
    the range 0 to 255 are valid for the color values.

    \note The rendering pipeline expects pixels with premultiplied alpha.

    QSGGeometry::defaultAttributes_ColoredPoint2D() can be used to construct an attribute
    set that is compatible with this material.

    The vertex color material respects both current opacity and current matrix when
    updating it's rendering state.
 */


/*!
    Creates a new vertex color material.
 */

QSGVertexColorMaterial::QSGVertexColorMaterial()
{
    setFlag(Blending, true);
    setFlag(SupportsRhiShader, true);
}


/*!
    int QSGVertexColorMaterial::compare() const

    As the vertex color material has all its state in the vertex attributes,
    all materials will be equal.

    \internal
 */

int QSGVertexColorMaterial::compare(const QSGMaterial * /* other */) const
{
    return 0;
}

/*!
    \internal
 */

QSGMaterialType *QSGVertexColorMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}



/*!
    \internal
 */

QSGMaterialShader *QSGVertexColorMaterial::createShader() const
{
    if (flags().testFlag(RhiShaderWanted))
        return new QSGVertexColorMaterialRhiShader;
    else
        return new QSGVertexColorMaterialShader;
}

QT_END_NAMESPACE
