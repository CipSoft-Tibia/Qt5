// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <Qt3DRender/QParameter>
#include <Qt3DRender/QEffect>
#include <Qt3DRender/QTechnique>
#include <Qt3DRender/QAbstractTexture>
#include <Qt3DRender/QShaderProgram>
#include <Qt3DRender/QRenderPass>
#include <Qt3DRender/QBlendEquation>
#include <Qt3DRender/QBlendEquationArguments>
#include <Qt3DRender/QFilterKey>
#include <Qt3DRender/QGraphicsApiFilter>
#include <QColor>
#include <QVector2D>
#include <QUrl>

#include "planematerial.h"

PlaneMaterial::PlaneMaterial(Qt3DRender::QAbstractTexture *texture, Qt3DCore::QNode *parent)
    : QMaterial(parent)
    , m_effect(new Qt3DRender::QEffect(this))
    , m_texture(texture)
{
    setEffect(m_effect);

    m_texCoordScaleParam = new Qt3DRender::QParameter(QStringLiteral("texCoordScale"), QVector2D(1.0f, -1.0f));
    m_texCoordBiasParam = new Qt3DRender::QParameter(QStringLiteral("texCoordBias"), QVector2D(0.0f, 1.0f));
    m_textureParam = new Qt3DRender::QParameter(QStringLiteral("surfaceTexture"), m_texture);

    m_effect->addParameter(m_texCoordScaleParam);
    m_effect->addParameter(m_texCoordBiasParam);
    m_effect->addParameter(m_textureParam);

    m_filter = new Qt3DRender::QFilterKey(this);
    m_filter->setName(QStringLiteral("renderingStyle"));
    m_filter->setValue(QStringLiteral("forward"));

    m_techniqueGLES = new Qt3DRender::QTechnique(m_effect);
    m_techniqueGL3 = new Qt3DRender::QTechnique(m_effect);
    m_techniqueGL2 = new Qt3DRender::QTechnique(m_effect);

    m_techniqueGLES->addFilterKey(m_filter);
    m_techniqueGL3->addFilterKey(m_filter);
    m_techniqueGL2->addFilterKey(m_filter);

    m_effect->addTechnique(m_techniqueGLES);
    m_effect->addTechnique(m_techniqueGL3);
    m_effect->addTechnique(m_techniqueGL2);

    m_programGLES = new Qt3DRender::QShaderProgram(m_effect);
    m_programGLES->setVertexShaderCode(Qt3DRender::QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/es2/texturing.vert"))));
    m_programGLES->setFragmentShaderCode(Qt3DRender::QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/es2/texturing.frag"))));

    m_programGL3 = new Qt3DRender::QShaderProgram(m_effect);
    m_programGL3->setVertexShaderCode(Qt3DRender::QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/gl3/texturing.vert"))));
    m_programGL3->setFragmentShaderCode(Qt3DRender::QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/gl3/texturing.frag"))));

    m_renderPassGLES = new Qt3DRender::QRenderPass(m_effect);
    m_renderPassGL3 = new Qt3DRender::QRenderPass(m_effect);
    m_renderPassGL2 = new Qt3DRender::QRenderPass(m_effect);

    m_renderPassGLES->setShaderProgram(m_programGLES);
    m_renderPassGL3->setShaderProgram(m_programGL3);
    m_renderPassGL2->setShaderProgram(m_programGL3);

    m_techniqueGL2->addRenderPass(m_renderPassGL2);
    m_techniqueGLES->addRenderPass(m_renderPassGLES);
    m_techniqueGL3->addRenderPass(m_renderPassGL3);

    m_techniqueGLES->graphicsApiFilter()->setApi(Qt3DRender::QGraphicsApiFilter::OpenGLES);
    m_techniqueGLES->graphicsApiFilter()->setMajorVersion(2);
    m_techniqueGLES->graphicsApiFilter()->setMinorVersion(0);
    m_techniqueGLES->graphicsApiFilter()->setProfile(Qt3DRender::QGraphicsApiFilter::NoProfile);

    m_techniqueGL2->graphicsApiFilter()->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
    m_techniqueGL2->graphicsApiFilter()->setMajorVersion(2);
    m_techniqueGL2->graphicsApiFilter()->setMinorVersion(0);
    m_techniqueGL2->graphicsApiFilter()->setProfile(Qt3DRender::QGraphicsApiFilter::NoProfile);

    m_techniqueGL3->graphicsApiFilter()->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
    m_techniqueGL3->graphicsApiFilter()->setMajorVersion(3);
    m_techniqueGL3->graphicsApiFilter()->setMinorVersion(1);
    m_techniqueGL3->graphicsApiFilter()->setProfile(Qt3DRender::QGraphicsApiFilter::CoreProfile);
}

