/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qsgdistancefieldglyphnode_p_p.h"
#include "qsgrhidistancefieldglyphcache_p.h"
#include <QtGui/qopenglfunctions.h>
#include <QtGui/qsurface.h>
#include <QtGui/qwindow.h>
#include <qmath.h>

QT_BEGIN_NAMESPACE

static float qt_sg_envFloat(const char *name, float defaultValue)
{
    if (Q_LIKELY(!qEnvironmentVariableIsSet(name)))
        return defaultValue;
    bool ok = false;
    const float value = qgetenv(name).toFloat(&ok);
    return ok ? value : defaultValue;
}

static float thresholdFunc(float glyphScale)
{
    static const float base = qt_sg_envFloat("QT_DF_BASE", 0.5f);
    static const float baseDev = qt_sg_envFloat("QT_DF_BASEDEVIATION", 0.065f);
    static const float devScaleMin = qt_sg_envFloat("QT_DF_SCALEFORMAXDEV", 0.15f);
    static const float devScaleMax = qt_sg_envFloat("QT_DF_SCALEFORNODEV", 0.3f);
    return base - ((qBound(devScaleMin, glyphScale, devScaleMax) - devScaleMin) / (devScaleMax - devScaleMin) * -baseDev + baseDev);
}

static float spreadFunc(float glyphScale)
{
    static const float range = qt_sg_envFloat("QT_DF_RANGE", 0.06f);
    return range / glyphScale;
}

class QSGDistanceFieldTextMaterialShader : public QSGMaterialShader
{
public:
    QSGDistanceFieldTextMaterialShader();

    void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;
    char const *const *attributeNames() const override;

protected:
    void initialize() override;

    void updateAlphaRange();
    void updateColor(const QVector4D &c);
    void updateTextureScale(const QVector2D &ts);

    float m_fontScale = 1.0;
    float m_matrixScale = 1.0;

    int m_matrix_id = -1;
    int m_textureScale_id = -1;
    int m_alphaMin_id = -1;
    int m_alphaMax_id = -1;
    int m_color_id = -1;

    QVector2D m_lastTextureScale;
    QVector4D m_lastColor;
    float m_lastAlphaMin = -1;
    float m_lastAlphaMax = -1;
};

char const *const *QSGDistanceFieldTextMaterialShader::attributeNames() const {
    static char const *const attr[] = { "vCoord", "tCoord", nullptr };
    return attr;
}

QSGDistanceFieldTextMaterialShader::QSGDistanceFieldTextMaterialShader()
{
    setShaderSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/qt-project.org/scenegraph/shaders/distancefieldtext.vert"));
    setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qt-project.org/scenegraph/shaders/distancefieldtext.frag"));
}

void QSGDistanceFieldTextMaterialShader::updateAlphaRange()
{
    float combinedScale = m_fontScale * m_matrixScale;
    float base = thresholdFunc(combinedScale);
    float range = spreadFunc(combinedScale);
    float alphaMin = qMax(0.0f, base - range);
    float alphaMax = qMin(base + range, 1.0f);
    if (alphaMin != m_lastAlphaMin) {
        program()->setUniformValue(m_alphaMin_id, GLfloat(alphaMin));
        m_lastAlphaMin = alphaMin;
    }
    if (alphaMax != m_lastAlphaMax) {
        program()->setUniformValue(m_alphaMax_id, GLfloat(alphaMax));
        m_lastAlphaMax = alphaMax;
    }
}

void QSGDistanceFieldTextMaterialShader::updateColor(const QVector4D &c)
{
    if (m_lastColor != c) {
        program()->setUniformValue(m_color_id, c);
        m_lastColor = c;
    }
}

void QSGDistanceFieldTextMaterialShader::updateTextureScale(const QVector2D &ts)
{
    if (m_lastTextureScale != ts) {
        program()->setUniformValue(m_textureScale_id, ts);
        m_lastTextureScale = ts;
    }
}

void QSGDistanceFieldTextMaterialShader::initialize()
{
    QSGMaterialShader::initialize();
    m_matrix_id = program()->uniformLocation("matrix");
    m_textureScale_id = program()->uniformLocation("textureScale");
    m_color_id = program()->uniformLocation("color");
    m_alphaMin_id = program()->uniformLocation("alphaMin");
    m_alphaMax_id = program()->uniformLocation("alphaMax");
}

void QSGDistanceFieldTextMaterialShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    Q_ASSERT(oldEffect == nullptr || newEffect->type() == oldEffect->type());
    QSGDistanceFieldTextMaterial *material = static_cast<QSGDistanceFieldTextMaterial *>(newEffect);
    QSGDistanceFieldTextMaterial *oldMaterial = static_cast<QSGDistanceFieldTextMaterial *>(oldEffect);

    bool updated = material->updateTextureSize();

    if (oldMaterial == nullptr
           || material->color() != oldMaterial->color()
           || state.isOpacityDirty()) {
        QVector4D color = material->color();
        color *= state.opacity();
        updateColor(color);
    }

    bool updateRange = false;
    if (oldMaterial == nullptr
            || material->fontScale() != oldMaterial->fontScale()) {
        m_fontScale = material->fontScale();
        updateRange = true;
    }
    if (state.isMatrixDirty()) {
        program()->setUniformValue(m_matrix_id, state.combinedMatrix());
        m_matrixScale = qSqrt(qAbs(state.determinant())) * state.devicePixelRatio();
        updateRange = true;
    }
    if (updateRange) {
        updateAlphaRange();
    }

    Q_ASSERT(material->glyphCache());

    if (updated
            || oldMaterial == nullptr
            || oldMaterial->texture()->textureId != material->texture()->textureId) {
        updateTextureScale(QVector2D(1.0 / material->textureSize().width(),
                                     1.0 / material->textureSize().height()));

        QOpenGLFunctions *funcs = state.context()->functions();
        funcs->glBindTexture(GL_TEXTURE_2D, material->texture()->textureId);

        if (updated) {
            // Set the mag/min filters to be linear. We only need to do this when the texture
            // has been recreated.
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
    }
}

class QSGDistanceFieldTextMaterialRhiShader : public QSGMaterialRhiShader
{
public:
    QSGDistanceFieldTextMaterialRhiShader(bool alphaTexture);

    bool updateUniformData(RenderState &state,
                           QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

protected:
    float m_fontScale = 1.0;
    float m_matrixScale = 1.0;
};

QSGDistanceFieldTextMaterialRhiShader::QSGDistanceFieldTextMaterialRhiShader(bool alphaTexture)
{
    setShaderFileName(VertexStage,
                      QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldtext.vert.qsb"));
    if (alphaTexture)
        setShaderFileName(FragmentStage,
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldtext_a.frag.qsb"));
    else
        setShaderFileName(FragmentStage,
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldtext.frag.qsb"));
}

bool QSGDistanceFieldTextMaterialRhiShader::updateUniformData(RenderState &state,
                                                              QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_ASSERT(oldMaterial == nullptr || newMaterial->type() == oldMaterial->type());
    QSGDistanceFieldTextMaterial *mat = static_cast<QSGDistanceFieldTextMaterial *>(newMaterial);
    QSGDistanceFieldTextMaterial *oldMat = static_cast<QSGDistanceFieldTextMaterial *>(oldMaterial);

    // updateUniformData() is called before updateSampledImage() by the
    // renderer. Hence updating the glyph cache stuff here.
    const bool textureUpdated = mat->updateTextureSizeAndWrapper();
    Q_ASSERT(mat->wrapperTexture());
    Q_ASSERT(oldMat == nullptr || oldMat->texture());

    bool changed = false;
    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 104);

    bool updateRange = false;
    if (!oldMat || mat->fontScale() != oldMat->fontScale()) {
        m_fontScale = mat->fontScale();
        updateRange = true;
    }
    if (state.isMatrixDirty()) {
        const QMatrix4x4 m = state.combinedMatrix();
        memcpy(buf->data(), m.constData(), 64);
        changed = true;
        m_matrixScale = qSqrt(qAbs(state.determinant())) * state.devicePixelRatio();
        updateRange = true;
    }
    if (textureUpdated || !oldMat || oldMat->texture()->texture != mat->texture()->texture) {
        const QVector2D ts(1.0f / mat->textureSize().width(), 1.0f / mat->textureSize().height());
        Q_ASSERT(sizeof(ts) == 8);
        memcpy(buf->data() + 64, &ts, 8);
        changed = true;
    }
    if (!oldMat || mat->color() != oldMat->color() || state.isOpacityDirty()) {
        const QVector4D color = mat->color() * state.opacity();
        Q_ASSERT(sizeof(color) == 16);
        memcpy(buf->data() + 80, &color, 16);
        changed = true;
    }
    if (updateRange) { // deferred because depends on m_fontScale and m_matrixScale
        const float combinedScale = m_fontScale * m_matrixScale;
        const float base = thresholdFunc(combinedScale);
        const float range = spreadFunc(combinedScale);
        const QVector2D alphaMinMax(qMax(0.0f, base - range), qMin(base + range, 1.0f));
        memcpy(buf->data() + 96, &alphaMinMax, 8);
        changed = true;
    }

    // move texture uploads/copies onto the renderer's soon-to-be-committed list
    static_cast<QSGRhiDistanceFieldGlyphCache *>(mat->glyphCache())->commitResourceUpdates(state.resourceUpdateBatch());

    return changed;
}

void QSGDistanceFieldTextMaterialRhiShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                                               QSGMaterial *newMaterial, QSGMaterial *)
{
    Q_UNUSED(state);
    if (binding != 1)
        return;

    QSGDistanceFieldTextMaterial *mat = static_cast<QSGDistanceFieldTextMaterial *>(newMaterial);
    QSGTexture *t = mat->wrapperTexture();
    t->setFiltering(QSGTexture::Linear);
    *texture = t;
}

QSGDistanceFieldTextMaterial::QSGDistanceFieldTextMaterial()
    : m_glyph_cache(nullptr)
    , m_texture(nullptr)
    , m_fontScale(1.0)
    , m_sgTexture(nullptr)
{
   setFlag(Blending | RequiresDeterminant | SupportsRhiShader, true);
}

QSGDistanceFieldTextMaterial::~QSGDistanceFieldTextMaterial()
{
    delete m_sgTexture;
}

QSGMaterialType *QSGDistanceFieldTextMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

void QSGDistanceFieldTextMaterial::setColor(const QColor &color)
{
    m_color = QVector4D(color.redF() * color.alphaF(),
                        color.greenF() * color.alphaF(),
                        color.blueF() * color.alphaF(),
                        color.alphaF());
}

QSGMaterialShader *QSGDistanceFieldTextMaterial::createShader() const
{
    if (flags().testFlag(RhiShaderWanted))
        return new QSGDistanceFieldTextMaterialRhiShader(m_glyph_cache->eightBitFormatIsAlphaSwizzled());
    else
        return new QSGDistanceFieldTextMaterialShader;
}

bool QSGDistanceFieldTextMaterial::updateTextureSize()
{
    if (!m_texture)
        m_texture = m_glyph_cache->glyphTexture(0); // invalid texture

    if (m_texture->size != m_size) {
        m_size = m_texture->size;
        return true;
    }

    return false;
}

// When using the RHI we need a QSGTexture wrapping the QRhiTexture, just
// exposing a QRhiTexture * (which would be the equivalent of GLuint textureId)
// is not sufficient to play nice with the material.
bool QSGDistanceFieldTextMaterial::updateTextureSizeAndWrapper()
{
    bool updated = updateTextureSize();
    if (updated) {
        if (m_sgTexture)
            delete m_sgTexture;
        m_sgTexture = new QSGPlainTexture;
        m_sgTexture->setTexture(m_texture->texture);
        m_sgTexture->setTextureSize(m_size);
        m_sgTexture->setOwnsTexture(false);
    }
    return updated;
}

int QSGDistanceFieldTextMaterial::compare(const QSGMaterial *o) const
{
    Q_ASSERT(o && type() == o->type());
    const QSGDistanceFieldTextMaterial *other = static_cast<const QSGDistanceFieldTextMaterial *>(o);
    if (m_glyph_cache != other->m_glyph_cache)
        return m_glyph_cache - other->m_glyph_cache;
    if (m_fontScale != other->m_fontScale) {
        return int(other->m_fontScale < m_fontScale) - int(m_fontScale < other->m_fontScale);
    }
    if (m_color != other->m_color)
        return &m_color < &other->m_color ? -1 : 1;
    int t0 = m_texture ? (m_texture->rhiBased ? qintptr(m_texture->texture) : m_texture->textureId) : 0;
    int t1 = other->m_texture ? (other->m_texture->rhiBased ? qintptr(other->m_texture->texture) : other->m_texture->textureId) : 0;
    return t0 - t1;
}


class DistanceFieldStyledTextMaterialShader : public QSGDistanceFieldTextMaterialShader
{
public:
    DistanceFieldStyledTextMaterialShader();

    void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;

protected:
    void initialize() override;

    int m_styleColor_id = -1;
};

DistanceFieldStyledTextMaterialShader::DistanceFieldStyledTextMaterialShader()
    : QSGDistanceFieldTextMaterialShader()
{
}

void DistanceFieldStyledTextMaterialShader::initialize()
{
    QSGDistanceFieldTextMaterialShader::initialize();
    m_styleColor_id = program()->uniformLocation("styleColor");
}

void DistanceFieldStyledTextMaterialShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    QSGDistanceFieldTextMaterialShader::updateState(state, newEffect, oldEffect);

    QSGDistanceFieldStyledTextMaterial *material = static_cast<QSGDistanceFieldStyledTextMaterial *>(newEffect);
    QSGDistanceFieldStyledTextMaterial *oldMaterial = static_cast<QSGDistanceFieldStyledTextMaterial *>(oldEffect);

    if (oldMaterial == nullptr
           || material->styleColor() != oldMaterial->styleColor()
           || (state.isOpacityDirty())) {
        QVector4D color = material->styleColor();
        color *= state.opacity();
        program()->setUniformValue(m_styleColor_id, color);
    }
}

class DistanceFieldStyledTextMaterialRhiShader : public QSGDistanceFieldTextMaterialRhiShader
{
public:
    DistanceFieldStyledTextMaterialRhiShader(bool alphaTexture);

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

DistanceFieldStyledTextMaterialRhiShader::DistanceFieldStyledTextMaterialRhiShader(bool alphaTexture)
    : QSGDistanceFieldTextMaterialRhiShader(alphaTexture)
{
}

bool DistanceFieldStyledTextMaterialRhiShader::updateUniformData(RenderState &state,
                                                                 QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = QSGDistanceFieldTextMaterialRhiShader::updateUniformData(state, newMaterial, oldMaterial);
    QSGDistanceFieldStyledTextMaterial *mat = static_cast<QSGDistanceFieldStyledTextMaterial *>(newMaterial);
    QSGDistanceFieldStyledTextMaterial *oldMat = static_cast<QSGDistanceFieldStyledTextMaterial *>(oldMaterial);

    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 128);

    if (!oldMat || mat->styleColor() != oldMat->styleColor() || state.isOpacityDirty()) {
        QVector4D styleColor = mat->styleColor();
        styleColor *= state.opacity();
        memcpy(buf->data() + 112, &styleColor, 16);
        changed = true;
    }

    return changed;
}

QSGDistanceFieldStyledTextMaterial::QSGDistanceFieldStyledTextMaterial()
    : QSGDistanceFieldTextMaterial()
{
}

QSGDistanceFieldStyledTextMaterial::~QSGDistanceFieldStyledTextMaterial()
{
}

void QSGDistanceFieldStyledTextMaterial::setStyleColor(const QColor &color)
{
    m_styleColor = QVector4D(color.redF() * color.alphaF(),
                             color.greenF() * color.alphaF(),
                             color.blueF() * color.alphaF(),
                             color.alphaF());
}

int QSGDistanceFieldStyledTextMaterial::compare(const QSGMaterial *o) const
{
    Q_ASSERT(o && type() == o->type());
    const QSGDistanceFieldStyledTextMaterial *other = static_cast<const QSGDistanceFieldStyledTextMaterial *>(o);
    if (m_styleColor != other->m_styleColor)
        return &m_styleColor < &other->m_styleColor ? -1 : 1;
    return QSGDistanceFieldTextMaterial::compare(o);
}


class DistanceFieldOutlineTextMaterialShader : public DistanceFieldStyledTextMaterialShader
{
public:
    DistanceFieldOutlineTextMaterialShader();

    void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;

protected:
    void initialize() override;

    void updateOutlineAlphaRange(int dfRadius);

    int m_outlineAlphaMax0_id = -1;
    int m_outlineAlphaMax1_id = -1;
};

DistanceFieldOutlineTextMaterialShader::DistanceFieldOutlineTextMaterialShader()
    : DistanceFieldStyledTextMaterialShader()
{
    setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qt-project.org/scenegraph/shaders/distancefieldoutlinetext.frag"));
}

void DistanceFieldOutlineTextMaterialShader::initialize()
{
    DistanceFieldStyledTextMaterialShader::initialize();
    m_outlineAlphaMax0_id = program()->uniformLocation("outlineAlphaMax0");
    m_outlineAlphaMax1_id = program()->uniformLocation("outlineAlphaMax1");
}

void DistanceFieldOutlineTextMaterialShader::updateOutlineAlphaRange(int dfRadius)
{
    float combinedScale = m_fontScale * m_matrixScale;
    float base = thresholdFunc(combinedScale);
    float range = spreadFunc(combinedScale);
    float outlineLimit = qMax(0.2f, base - 0.5f / dfRadius / m_fontScale);

    float alphaMin = qMax(0.0f, base - range);
    float styleAlphaMin0 = qMax(0.0f, outlineLimit - range);
    float styleAlphaMin1 = qMin(outlineLimit + range, alphaMin);
    program()->setUniformValue(m_outlineAlphaMax0_id, GLfloat(styleAlphaMin0));
    program()->setUniformValue(m_outlineAlphaMax1_id, GLfloat(styleAlphaMin1));
}

void DistanceFieldOutlineTextMaterialShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    DistanceFieldStyledTextMaterialShader::updateState(state, newEffect, oldEffect);

    QSGDistanceFieldOutlineTextMaterial *material = static_cast<QSGDistanceFieldOutlineTextMaterial *>(newEffect);
    QSGDistanceFieldOutlineTextMaterial *oldMaterial = static_cast<QSGDistanceFieldOutlineTextMaterial *>(oldEffect);

    if (oldMaterial == nullptr
            || material->fontScale() != oldMaterial->fontScale()
            || state.isMatrixDirty())
        updateOutlineAlphaRange(material->glyphCache()->distanceFieldRadius());
}

class DistanceFieldOutlineTextMaterialRhiShader : public DistanceFieldStyledTextMaterialRhiShader
{
public:
    DistanceFieldOutlineTextMaterialRhiShader(bool alphaTexture);

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

DistanceFieldOutlineTextMaterialRhiShader::DistanceFieldOutlineTextMaterialRhiShader(bool alphaTexture)
    : DistanceFieldStyledTextMaterialRhiShader(alphaTexture)
{
    setShaderFileName(VertexStage,
                      QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldoutlinetext.vert.qsb"));
    if (alphaTexture)
        setShaderFileName(FragmentStage,
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldoutlinetext_a.frag.qsb"));
    else
        setShaderFileName(FragmentStage,
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldoutlinetext.frag.qsb"));
}

bool DistanceFieldOutlineTextMaterialRhiShader::updateUniformData(RenderState &state,
                                                                  QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = DistanceFieldStyledTextMaterialRhiShader::updateUniformData(state, newMaterial, oldMaterial);
    QSGDistanceFieldOutlineTextMaterial *mat = static_cast<QSGDistanceFieldOutlineTextMaterial *>(newMaterial);
    QSGDistanceFieldOutlineTextMaterial *oldMat = static_cast<QSGDistanceFieldOutlineTextMaterial *>(oldMaterial);

    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 136);

    if (!oldMat || mat->fontScale() != oldMat->fontScale() || state.isMatrixDirty()) {
        float dfRadius = mat->glyphCache()->distanceFieldRadius();
        float combinedScale = m_fontScale * m_matrixScale;
        float base = thresholdFunc(combinedScale);
        float range = spreadFunc(combinedScale);
        float outlineLimit = qMax(0.2f, base - 0.5f / dfRadius / m_fontScale);
        float alphaMin = qMax(0.0f, base - range);
        float styleAlphaMin0 = qMax(0.0f, outlineLimit - range);
        float styleAlphaMin1 = qMin(outlineLimit + range, alphaMin);
        memcpy(buf->data() + 128, &styleAlphaMin0, 4);
        memcpy(buf->data() + 132, &styleAlphaMin1, 4);
        changed = true;
    }

    return changed;
}

QSGDistanceFieldOutlineTextMaterial::QSGDistanceFieldOutlineTextMaterial()
    : QSGDistanceFieldStyledTextMaterial()
{
}

QSGDistanceFieldOutlineTextMaterial::~QSGDistanceFieldOutlineTextMaterial()
{
}

QSGMaterialType *QSGDistanceFieldOutlineTextMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *QSGDistanceFieldOutlineTextMaterial::createShader() const
{
    if (flags().testFlag(RhiShaderWanted))
        return new DistanceFieldOutlineTextMaterialRhiShader(m_glyph_cache->eightBitFormatIsAlphaSwizzled());
    else
        return new DistanceFieldOutlineTextMaterialShader;
}


class DistanceFieldShiftedStyleTextMaterialShader : public DistanceFieldStyledTextMaterialShader
{
public:
    DistanceFieldShiftedStyleTextMaterialShader();

    void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;

protected:
    void initialize() override;

    void updateShift(qreal fontScale, const QPointF& shift);

    int m_shift_id = -1;
};

DistanceFieldShiftedStyleTextMaterialShader::DistanceFieldShiftedStyleTextMaterialShader()
    : DistanceFieldStyledTextMaterialShader()
{
    setShaderSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/qt-project.org/scenegraph/shaders/distancefieldshiftedtext.vert"));
    setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qt-project.org/scenegraph/shaders/distancefieldshiftedtext.frag"));
}

void DistanceFieldShiftedStyleTextMaterialShader::initialize()
{
    DistanceFieldStyledTextMaterialShader::initialize();
    m_shift_id = program()->uniformLocation("shift");
}

void DistanceFieldShiftedStyleTextMaterialShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    DistanceFieldStyledTextMaterialShader::updateState(state, newEffect, oldEffect);

    QSGDistanceFieldShiftedStyleTextMaterial *material = static_cast<QSGDistanceFieldShiftedStyleTextMaterial *>(newEffect);
    QSGDistanceFieldShiftedStyleTextMaterial *oldMaterial = static_cast<QSGDistanceFieldShiftedStyleTextMaterial *>(oldEffect);

    if (oldMaterial == nullptr
            || oldMaterial->fontScale() != material->fontScale()
            || oldMaterial->shift() != material->shift()
            || oldMaterial->textureSize() != material->textureSize()) {
        updateShift(material->fontScale(), material->shift());
    }
}

void DistanceFieldShiftedStyleTextMaterialShader::updateShift(qreal fontScale, const QPointF &shift)
{
    QPointF texel(1.0 / fontScale * shift.x(),
                  1.0 / fontScale * shift.y());
    program()->setUniformValue(m_shift_id, texel);
}

class DistanceFieldShiftedStyleTextMaterialRhiShader : public DistanceFieldStyledTextMaterialRhiShader
{
public:
    DistanceFieldShiftedStyleTextMaterialRhiShader(bool alphaTexture);

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

DistanceFieldShiftedStyleTextMaterialRhiShader::DistanceFieldShiftedStyleTextMaterialRhiShader(bool alphaTexture)
    : DistanceFieldStyledTextMaterialRhiShader(alphaTexture)
{
    setShaderFileName(VertexStage,
                      QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldshiftedtext.vert.qsb"));
    if (alphaTexture)
        setShaderFileName(FragmentStage,
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldshiftedtext_a.frag.qsb"));
    else
        setShaderFileName(FragmentStage,
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldshiftedtext.frag.qsb"));
}

bool DistanceFieldShiftedStyleTextMaterialRhiShader::updateUniformData(RenderState &state,
                                                                       QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = DistanceFieldStyledTextMaterialRhiShader::updateUniformData(state, newMaterial, oldMaterial);
    QSGDistanceFieldShiftedStyleTextMaterial *mat = static_cast<QSGDistanceFieldShiftedStyleTextMaterial *>(newMaterial);
    QSGDistanceFieldShiftedStyleTextMaterial *oldMat = static_cast<QSGDistanceFieldShiftedStyleTextMaterial *>(oldMaterial);

    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 136);

    if (!oldMat || oldMat->fontScale() != mat->fontScale() || oldMat->shift() != mat->shift()
            || oldMat->textureSize() != mat->textureSize())
    {
        QPointF shift(1.0 / mat->fontScale() * mat->shift().x(),
                      1.0 / mat->fontScale() * mat->shift().y());
        memcpy(buf->data() + 128, &shift, 8);
        changed = true;
    }

    return changed;
}

QSGDistanceFieldShiftedStyleTextMaterial::QSGDistanceFieldShiftedStyleTextMaterial()
    : QSGDistanceFieldStyledTextMaterial()
{
}

QSGDistanceFieldShiftedStyleTextMaterial::~QSGDistanceFieldShiftedStyleTextMaterial()
{
}

QSGMaterialType *QSGDistanceFieldShiftedStyleTextMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *QSGDistanceFieldShiftedStyleTextMaterial::createShader() const
{
    if (flags().testFlag(RhiShaderWanted))
        return new DistanceFieldShiftedStyleTextMaterialRhiShader(m_glyph_cache->eightBitFormatIsAlphaSwizzled());
    else
        return new DistanceFieldShiftedStyleTextMaterialShader;
}

int QSGDistanceFieldShiftedStyleTextMaterial::compare(const QSGMaterial *o) const
{
    const QSGDistanceFieldShiftedStyleTextMaterial *other = static_cast<const QSGDistanceFieldShiftedStyleTextMaterial *>(o);
    if (m_shift != other->m_shift)
        return &m_shift < &other->m_shift ? -1 : 1;
    return QSGDistanceFieldStyledTextMaterial::compare(o);
}


class QSGHiQSubPixelDistanceFieldTextMaterialShader : public QSGDistanceFieldTextMaterialShader
{
public:
    QSGHiQSubPixelDistanceFieldTextMaterialShader();

    void initialize() override;
    void activate() override;
    void deactivate() override;
    void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;

private:
    int m_fontScale_id = -1;
    int m_vecDelta_id = -1;
};

QSGHiQSubPixelDistanceFieldTextMaterialShader::QSGHiQSubPixelDistanceFieldTextMaterialShader()
    : QSGDistanceFieldTextMaterialShader()
{
    setShaderSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/qt-project.org/scenegraph/shaders/hiqsubpixeldistancefieldtext.vert"));
    setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qt-project.org/scenegraph/shaders/hiqsubpixeldistancefieldtext.frag"));
}

void QSGHiQSubPixelDistanceFieldTextMaterialShader::initialize()
{
    QSGDistanceFieldTextMaterialShader::initialize();
    m_fontScale_id = program()->uniformLocation("fontScale");
    m_vecDelta_id = program()->uniformLocation("vecDelta");
}

void QSGHiQSubPixelDistanceFieldTextMaterialShader::activate()
{
    QSGDistanceFieldTextMaterialShader::activate();
    QOpenGLContext::currentContext()->functions()->glBlendFunc(GL_CONSTANT_COLOR, GL_ONE_MINUS_SRC_COLOR);
}

void QSGHiQSubPixelDistanceFieldTextMaterialShader::deactivate()
{
    QSGDistanceFieldTextMaterialShader::deactivate();
    QOpenGLContext::currentContext()->functions()->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

void QSGHiQSubPixelDistanceFieldTextMaterialShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    Q_ASSERT(oldEffect == nullptr || newEffect->type() == oldEffect->type());
    QSGDistanceFieldTextMaterial *material = static_cast<QSGDistanceFieldTextMaterial *>(newEffect);
    QSGDistanceFieldTextMaterial *oldMaterial = static_cast<QSGDistanceFieldTextMaterial *>(oldEffect);

    if (oldMaterial == nullptr || material->color() != oldMaterial->color()) {
        QVector4D c = material->color();
        state.context()->functions()->glBlendColor(c.x(), c.y(), c.z(), 1.0f);
    }

    if (oldMaterial == nullptr || material->fontScale() != oldMaterial->fontScale())
        program()->setUniformValue(m_fontScale_id, GLfloat(material->fontScale()));

    if (oldMaterial == nullptr || state.isMatrixDirty()) {
        int viewportWidth = state.viewportRect().width();
        QMatrix4x4 mat = state.combinedMatrix().inverted();
        program()->setUniformValue(m_vecDelta_id, mat.column(0) * (qreal(2) / viewportWidth));
    }

    QSGDistanceFieldTextMaterialShader::updateState(state, newEffect, oldEffect);
}

class QSGHiQSubPixelDistanceFieldTextMaterialRhiShader : public QSGDistanceFieldTextMaterialRhiShader
{
public:
    QSGHiQSubPixelDistanceFieldTextMaterialRhiShader(bool alphaTexture);

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    bool updateGraphicsPipelineState(RenderState &state, GraphicsPipelineState *ps,
                                     QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

QSGHiQSubPixelDistanceFieldTextMaterialRhiShader::QSGHiQSubPixelDistanceFieldTextMaterialRhiShader(bool alphaTexture)
    : QSGDistanceFieldTextMaterialRhiShader(alphaTexture)
{
    setFlag(UpdatesGraphicsPipelineState, true);

    setShaderFileName(VertexStage,
                      QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/hiqsubpixeldistancefieldtext.vert.qsb"));
    if (alphaTexture)
        setShaderFileName(FragmentStage,
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/hiqsubpixeldistancefieldtext_a.frag.qsb"));
    else
        setShaderFileName(FragmentStage,
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/hiqsubpixeldistancefieldtext.frag.qsb"));
}

bool QSGHiQSubPixelDistanceFieldTextMaterialRhiShader::updateUniformData(RenderState &state,
                                                                         QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = QSGDistanceFieldTextMaterialRhiShader::updateUniformData(state, newMaterial, oldMaterial);
    QSGHiQSubPixelDistanceFieldTextMaterial *mat = static_cast<QSGHiQSubPixelDistanceFieldTextMaterial *>(newMaterial);
    QSGHiQSubPixelDistanceFieldTextMaterial *oldMat = static_cast<QSGHiQSubPixelDistanceFieldTextMaterial *>(oldMaterial);

    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 128);

    if (!oldMat || mat->fontScale() != oldMat->fontScale()) {
        float fontScale = mat->fontScale();
        memcpy(buf->data() + 104, &fontScale, 4);
        changed = true;
    }

    if (!oldMat || state.isMatrixDirty()) {
        int viewportWidth = state.viewportRect().width();
        QMatrix4x4 mat = state.combinedMatrix().inverted();
        QVector4D vecDelta = mat.column(0) * (qreal(2) / viewportWidth);
        memcpy(buf->data() + 112, &vecDelta, 16);
    }

    return changed;
}

bool QSGHiQSubPixelDistanceFieldTextMaterialRhiShader::updateGraphicsPipelineState(RenderState &state, GraphicsPipelineState *ps,
                                                                                   QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_UNUSED(state);
    Q_UNUSED(oldMaterial);
    QSGHiQSubPixelDistanceFieldTextMaterial *mat = static_cast<QSGHiQSubPixelDistanceFieldTextMaterial *>(newMaterial);

    ps->blendEnable = true;
    ps->srcColor = GraphicsPipelineState::ConstantColor;
    ps->dstColor = GraphicsPipelineState::OneMinusSrcColor;

    const QVector4D color = mat->color();
    // this is dynamic state but it's - magic! - taken care of by the renderer
    ps->blendConstant = QColor::fromRgbF(color.x(), color.y(), color.z(), 1.0f);

    return true;
}

QSGMaterialType *QSGHiQSubPixelDistanceFieldTextMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *QSGHiQSubPixelDistanceFieldTextMaterial::createShader() const
{
    if (flags().testFlag(RhiShaderWanted))
        return new QSGHiQSubPixelDistanceFieldTextMaterialRhiShader(m_glyph_cache->eightBitFormatIsAlphaSwizzled());
    else
        return new QSGHiQSubPixelDistanceFieldTextMaterialShader;
}


class QSGLoQSubPixelDistanceFieldTextMaterialShader : public QSGHiQSubPixelDistanceFieldTextMaterialShader
{
public:
    QSGLoQSubPixelDistanceFieldTextMaterialShader();
};

QSGLoQSubPixelDistanceFieldTextMaterialShader::QSGLoQSubPixelDistanceFieldTextMaterialShader()
    : QSGHiQSubPixelDistanceFieldTextMaterialShader()
{
    setShaderSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/qt-project.org/scenegraph/shaders/loqsubpixeldistancefieldtext.vert"));
    setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qt-project.org/scenegraph/shaders/loqsubpixeldistancefieldtext.frag"));
}

class QSGLoQSubPixelDistanceFieldTextMaterialRhiShader : public QSGHiQSubPixelDistanceFieldTextMaterialRhiShader
{
public:
    QSGLoQSubPixelDistanceFieldTextMaterialRhiShader(bool alphaTexture);
};

QSGLoQSubPixelDistanceFieldTextMaterialRhiShader::QSGLoQSubPixelDistanceFieldTextMaterialRhiShader(bool alphaTexture)
    : QSGHiQSubPixelDistanceFieldTextMaterialRhiShader(alphaTexture)
{
    setShaderFileName(VertexStage,
                      QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/loqsubpixeldistancefieldtext.vert.qsb"));
    if (alphaTexture)
        setShaderFileName(FragmentStage,
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/loqsubpixeldistancefieldtext_a.frag.qsb"));
    else
        setShaderFileName(FragmentStage,
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/loqsubpixeldistancefieldtext.frag.qsb"));
}

QSGMaterialType *QSGLoQSubPixelDistanceFieldTextMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *QSGLoQSubPixelDistanceFieldTextMaterial::createShader() const
{
    if (flags().testFlag(RhiShaderWanted))
        return new QSGLoQSubPixelDistanceFieldTextMaterialRhiShader(m_glyph_cache->eightBitFormatIsAlphaSwizzled());
    else
        return new QSGLoQSubPixelDistanceFieldTextMaterialShader;
}

QT_END_NAMESPACE
