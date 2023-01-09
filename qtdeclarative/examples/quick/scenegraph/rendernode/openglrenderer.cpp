/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "openglrenderer.h"
#include <QQuickItem>

#if QT_CONFIG(opengl)

#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>

//! [1]
OpenGLRenderNode::~OpenGLRenderNode()
{
    releaseResources();
}

void OpenGLRenderNode::releaseResources()
{
    delete m_program;
    m_program = nullptr;
    delete m_vbo;
    m_vbo = nullptr;
}
//! [1]

void OpenGLRenderNode::init()
{
    m_program = new QOpenGLShaderProgram;

    static const char *vertexShaderSource =
        "attribute highp vec4 posAttr;\n"
        "attribute lowp vec4 colAttr;\n"
        "varying lowp vec4 col;\n"
        "uniform highp mat4 matrix;\n"
        "void main() {\n"
        "   col = colAttr;\n"
        "   gl_Position = matrix * posAttr;\n"
        "}\n";

    static const char *fragmentShaderSource =
        "varying lowp vec4 col;\n"
        "uniform lowp float opacity;\n"
        "void main() {\n"
        "   gl_FragColor = col * opacity;\n"
        "}\n";

    m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->bindAttributeLocation("posAttr", 0);
    m_program->bindAttributeLocation("colAttr", 1);
    m_program->link();

    m_matrixUniform = m_program->uniformLocation("matrix");
    m_opacityUniform = m_program->uniformLocation("opacity");

    const int VERTEX_SIZE = 6 * sizeof(GLfloat);

    static GLfloat colors[] = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };

    m_vbo = new QOpenGLBuffer;
    m_vbo->create();
    m_vbo->bind();
    m_vbo->allocate(VERTEX_SIZE + sizeof(colors));
    m_vbo->write(VERTEX_SIZE, colors, sizeof(colors));
    m_vbo->release();
}

//! [2]
void OpenGLRenderNode::render(const RenderState *state)
{
    if (!m_program)
        init();

    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    m_program->bind();
    m_program->setUniformValue(m_matrixUniform, *state->projectionMatrix() * *matrix());
    m_program->setUniformValue(m_opacityUniform, float(inheritedOpacity()));
//! [2]

    m_vbo->bind();

//! [5]
    QPointF p0(m_width - 1, m_height - 1);
    QPointF p1(0, 0);
    QPointF p2(0, m_height - 1);

    GLfloat vertices[6] = { GLfloat(p0.x()), GLfloat(p0.y()),
                            GLfloat(p1.x()), GLfloat(p1.y()),
                            GLfloat(p2.x()), GLfloat(p2.y()) };
    m_vbo->write(0, vertices, sizeof(vertices));
//! [5]

    m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2);
    m_program->setAttributeBuffer(1, GL_FLOAT, sizeof(vertices), 3);
    m_program->enableAttributeArray(0);
    m_program->enableAttributeArray(1);

    // We are prepared both for the legacy (direct OpenGL) and the modern
    // (abstracted by RHI) OpenGL scenegraph. So set all the states that are
    // important to us.

    //! [3]
    f->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // Clip support.
    if (state->scissorEnabled()) {
        f->glEnable(GL_SCISSOR_TEST);
        const QRect r = state->scissorRect(); // already bottom-up
        f->glScissor(r.x(), r.y(), r.width(), r.height());
    }
    if (state->stencilEnabled()) {
        f->glEnable(GL_STENCIL_TEST);
        f->glStencilFunc(GL_EQUAL, state->stencilValue(), 0xFF);
        f->glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    }

    f->glDrawArrays(GL_TRIANGLES, 0, 3);
    //! [3]
}

//! [4]
QSGRenderNode::StateFlags OpenGLRenderNode::changedStates() const
{
    return BlendState | ScissorState | StencilState;
}

QSGRenderNode::RenderingFlags OpenGLRenderNode::flags() const
{
    return BoundedRectRendering | DepthAwareRendering;
}

QRectF OpenGLRenderNode::rect() const
{
    return QRect(0, 0, m_width, m_height);
}
//! [4]

void OpenGLRenderNode::sync(QQuickItem *item)
{
    m_width = item->width();
    m_height = item->height();
}

#endif // opengl
