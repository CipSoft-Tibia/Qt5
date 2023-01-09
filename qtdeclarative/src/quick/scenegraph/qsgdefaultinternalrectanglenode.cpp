
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

#include "qsgdefaultinternalrectanglenode_p.h"

#include <QtQuick/qsgvertexcolormaterial.h>
#include <QtQuick/qsgtexturematerial.h>

#include <QtQuick/private/qsgcontext_p.h>

#include <QtCore/qmath.h>
#include <QtCore/qvarlengtharray.h>

QT_BEGIN_NAMESPACE

class SmoothColorMaterialShader : public QSGMaterialShader
{
public:
    SmoothColorMaterialShader();

    void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;
    char const *const *attributeNames() const override;

private:
    void initialize() override;

    int m_matrixLoc;
    int m_opacityLoc;
    int m_pixelSizeLoc;
};

SmoothColorMaterialShader::SmoothColorMaterialShader()
{
    setShaderSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/qt-project.org/scenegraph/shaders/smoothcolor.vert"));
    setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qt-project.org/scenegraph/shaders/smoothcolor.frag"));
}

void SmoothColorMaterialShader::updateState(const RenderState &state, QSGMaterial *, QSGMaterial *oldEffect)
{
    if (state.isOpacityDirty())
        program()->setUniformValue(m_opacityLoc, state.opacity());

    if (state.isMatrixDirty())
        program()->setUniformValue(m_matrixLoc, state.combinedMatrix());

    if (oldEffect == nullptr) {
        // The viewport is constant, so set the pixel size uniform only once.
        QRect r = state.viewportRect();
        program()->setUniformValue(m_pixelSizeLoc, 2.0f / r.width(), 2.0f / r.height());
    }
}

char const *const *SmoothColorMaterialShader::attributeNames() const
{
    static char const *const attributes[] = {
        "vertex",
        "vertexColor",
        "vertexOffset",
        nullptr
    };
    return attributes;
}

void SmoothColorMaterialShader::initialize()
{
    m_matrixLoc = program()->uniformLocation("matrix");
    m_opacityLoc = program()->uniformLocation("opacity");
    m_pixelSizeLoc = program()->uniformLocation("pixelSize");
}


class SmoothColorMaterialRhiShader : public QSGMaterialRhiShader
{
public:
    SmoothColorMaterialRhiShader();

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

SmoothColorMaterialRhiShader::SmoothColorMaterialRhiShader()
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/smoothcolor.vert.qsb"));
    setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/smoothcolor.frag.qsb"));
}

bool SmoothColorMaterialRhiShader::updateUniformData(RenderState &state, QSGMaterial *, QSGMaterial *oldMaterial)
{
    bool changed = false;
    QByteArray *buf = state.uniformData();

    if (state.isMatrixDirty()) {
        const QMatrix4x4 m = state.combinedMatrix();
        memcpy(buf->data(), m.constData(), 64);
        changed = true;
    }

    if (oldMaterial == nullptr) {
        // The viewport is constant, so set the pixel size uniform only once.
        const QRect r = state.viewportRect();
        const QVector2D v(2.0f / r.width(), 2.0f / r.height());
        Q_ASSERT(sizeof(v) == 8);
        memcpy(buf->data() + 64, &v, 8);
        changed = true;
    }

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(buf->data() + 72, &opacity, 4);
        changed = true;
    }

    return changed;
}


QSGSmoothColorMaterial::QSGSmoothColorMaterial()
{
    setFlag(RequiresFullMatrixExceptTranslate, true);
    setFlag(Blending, true);
    setFlag(SupportsRhiShader, true);
}

int QSGSmoothColorMaterial::compare(const QSGMaterial *) const
{
    // all state in vertex attributes -> all smoothcolor materials are equal
    return 0;
}

QSGMaterialType *QSGSmoothColorMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *QSGSmoothColorMaterial::createShader() const
{
    if (flags().testFlag(RhiShaderWanted))
        return new SmoothColorMaterialRhiShader;
    else
        return new SmoothColorMaterialShader;
}

QSGDefaultInternalRectangleNode::QSGDefaultInternalRectangleNode()
{
    setMaterial(&m_material);
}

void QSGDefaultInternalRectangleNode::updateMaterialAntialiasing()
{
    if (m_antialiasing)
        setMaterial(&m_smoothMaterial);
    else
        setMaterial(&m_material);
}

void QSGDefaultInternalRectangleNode::updateMaterialBlending(QSGNode::DirtyState *state)
{
    // smoothed material is always blended, so no change in material state
    if (material() == &m_material) {
        bool wasBlending = (m_material.flags() & QSGMaterial::Blending);
        bool isBlending = (m_gradient_stops.size() > 0 && !m_gradient_is_opaque)
                           || (m_color.alpha() < 255 && m_color.alpha() != 0)
                           || (m_pen_width > 0 && m_border_color.alpha() < 255);
        if (wasBlending != isBlending) {
            m_material.setFlag(QSGMaterial::Blending, isBlending);
            *state |= QSGNode::DirtyMaterial;
        }
    }
}

QT_END_NAMESPACE
