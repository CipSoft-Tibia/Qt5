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

#include "qsgd3d12shadereffectnode_p.h"
#include "qsgd3d12rendercontext_p.h"
#include "qsgd3d12texture_p.h"
#include "qsgd3d12engine_p.h"
#include <QtCore/qthreadpool.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileselector.h>
#include <QtQml/qqmlfile.h>
#include <qsgtextureprovider.h>

#include <d3d12shader.h>
#include <d3dcompiler.h>

#include "vs_shadereffectdefault.hlslh"
#include "ps_shadereffectdefault.hlslh"

QT_BEGIN_NAMESPACE

// NOTE: Avoid categorized logging. It is slow.

#define DECLARE_DEBUG_VAR(variable) \
    static bool debug_ ## variable() \
    { static bool value = qgetenv("QSG_RENDERER_DEBUG").contains(QT_STRINGIFY(variable)); return value; }

DECLARE_DEBUG_VAR(shader)

void QSGD3D12ShaderLinker::reset(const QByteArray &vertBlob, const QByteArray &fragBlob)
{
    Q_ASSERT(!vertBlob.isEmpty() && !fragBlob.isEmpty());
    vs = vertBlob;
    fs = fragBlob;

    error = false;

    constantBufferSize = 0;
    constants.clear();
    samplers.clear();
    textures.clear();
    textureNameMap.clear();
}

void QSGD3D12ShaderLinker::feedConstants(const QSGShaderEffectNode::ShaderData &shader, const QSet<int> *dirtyIndices)
{
    Q_ASSERT(shader.shaderInfo.variables.count() == shader.varData.count());
    if (!dirtyIndices) {
        constantBufferSize = qMax(constantBufferSize, shader.shaderInfo.constantDataSize);
        for (int i = 0; i < shader.shaderInfo.variables.count(); ++i) {
            const auto &var(shader.shaderInfo.variables.at(i));
            if (var.type == QSGGuiThreadShaderEffectManager::ShaderInfo::Constant) {
                const auto &vd(shader.varData.at(i));
                Constant c;
                c.size = var.size;
                c.specialType = vd.specialType;
                if (c.specialType != QSGShaderEffectNode::VariableData::SubRect) {
                    c.value = vd.value;
                } else {
                    Q_ASSERT(var.name.startsWith(QByteArrayLiteral("qt_SubRect_")));
                    c.value = var.name.mid(11);
                }
                constants[var.offset] = c;
            }
        }
    } else {
        for (int idx : *dirtyIndices)
            constants[shader.shaderInfo.variables.at(idx).offset].value = shader.varData.at(idx).value;
    }
}

void QSGD3D12ShaderLinker::feedSamplers(const QSGShaderEffectNode::ShaderData &shader)
{
    for (int i = 0; i < shader.shaderInfo.variables.count(); ++i) {
        const auto &var(shader.shaderInfo.variables.at(i));
        if (var.type == QSGGuiThreadShaderEffectManager::ShaderInfo::Sampler) {
            Q_ASSERT(shader.varData.at(i).specialType == QSGShaderEffectNode::VariableData::Unused);
            samplers.insert(var.bindPoint);
        }
    }
}

void QSGD3D12ShaderLinker::feedTextures(const QSGShaderEffectNode::ShaderData &shader, const QSet<int> *dirtyIndices)
{
    if (!dirtyIndices) {
        for (int i = 0; i < shader.shaderInfo.variables.count(); ++i) {
            const auto &var(shader.shaderInfo.variables.at(i));
            const auto &vd(shader.varData.at(i));
            if (var.type == QSGGuiThreadShaderEffectManager::ShaderInfo::Texture) {
                Q_ASSERT(vd.specialType == QSGShaderEffectNode::VariableData::Source);
                textures.insert(var.bindPoint, vd.value);
                textureNameMap.insert(var.name, var.bindPoint);
            }
        }
    } else {
        for (int idx : *dirtyIndices) {
            const auto &var(shader.shaderInfo.variables.at(idx));
            const auto &vd(shader.varData.at(idx));
            textures.insert(var.bindPoint, vd.value);
            textureNameMap.insert(var.name, var.bindPoint);
        }
    }
}

void QSGD3D12ShaderLinker::linkTextureSubRects()
{
    // feedConstants stores <name> in Constant::value for subrect entries. Now
    // that both constants and textures are known, replace the name with the
    // texture bind point.
    for (Constant &c : constants) {
        if (c.specialType == QSGShaderEffectNode::VariableData::SubRect) {
            if (c.value.type() == QVariant::ByteArray) {
                const QByteArray name = c.value.toByteArray();
                if (!textureNameMap.contains(name))
                    qWarning("ShaderEffect: qt_SubRect_%s refers to unknown source texture", qPrintable(name));
                c.value = textureNameMap[name];
            }
        }
    }
}

void QSGD3D12ShaderLinker::dump()
{
    if (error) {
        qDebug() << "Failed to generate program data";
        return;
    }
    qDebug() << "Combined shader data" << vs.size() << fs.size() << "cbuffer size" << constantBufferSize;
    qDebug() << " - constants" << constants;
    qDebug() << " - samplers" << samplers;
    qDebug() << " - textures" << textures;
}

QDebug operator<<(QDebug debug, const QSGD3D12ShaderLinker::Constant &c)
{
    QDebugStateSaver saver(debug);
    debug.space();
    debug << "size" << c.size;
    if (c.specialType != QSGShaderEffectNode::VariableData::None)
        debug << "special" << c.specialType;
    else
        debug << "value" << c.value;
    return debug;
}

QSGD3D12ShaderEffectMaterial::QSGD3D12ShaderEffectMaterial(QSGD3D12ShaderEffectNode *node)
    : node(node)
{
    setFlag(Blending | RequiresFullMatrix, true); // may be changed in sync()
}

QSGD3D12ShaderEffectMaterial::~QSGD3D12ShaderEffectMaterial()
{
    delete dummy;
}

struct QSGD3D12ShaderMaterialTypeCache
{
    QSGMaterialType *get(const QByteArray &vs, const QByteArray &fs);
    void reset() { qDeleteAll(m_types); m_types.clear(); }

    struct Key {
        QByteArray blob[2];
        Key() { }
        Key(const QByteArray &vs, const QByteArray &fs) { blob[0] = vs; blob[1] = fs; }
        bool operator==(const Key &other) const {
            return blob[0] == other.blob[0] && blob[1] == other.blob[1];
        }
    };
    QHash<Key, QSGMaterialType *> m_types;
};

uint qHash(const QSGD3D12ShaderMaterialTypeCache::Key &key, uint seed = 0)
{
    uint hash = seed;
    for (int i = 0; i < 2; ++i)
        hash = hash * 31337 + qHash(key.blob[i]);
    return hash;
}

QSGMaterialType *QSGD3D12ShaderMaterialTypeCache::get(const QByteArray &vs, const QByteArray &fs)
{
    const Key k(vs, fs);
    if (m_types.contains(k))
        return m_types.value(k);

    QSGMaterialType *t = new QSGMaterialType;
    m_types.insert(k, t);
    return t;
}

Q_GLOBAL_STATIC(QSGD3D12ShaderMaterialTypeCache, shaderMaterialTypeCache)

void QSGD3D12ShaderEffectNode::cleanupMaterialTypeCache()
{
    shaderMaterialTypeCache()->reset();
}

QSGMaterialType *QSGD3D12ShaderEffectMaterial::type() const
{
    return mtype;
}

static bool hasAtlasTexture(const QVector<QSGTextureProvider *> &textureProviders)
{
    for (int i = 0; i < textureProviders.count(); ++i) {
        QSGTextureProvider *t = textureProviders.at(i);
        if (t && t->texture() && t->texture()->isAtlasTexture())
            return true;
    }
    return false;
}

int QSGD3D12ShaderEffectMaterial::compare(const QSGMaterial *other) const
{
    Q_ASSERT(other && type() == other->type());
    const QSGD3D12ShaderEffectMaterial *o = static_cast<const QSGD3D12ShaderEffectMaterial *>(other);

    if (int diff = cullMode - o->cullMode)
        return diff;

    if (int diff = textureProviders.count() - o->textureProviders.count())
        return diff;

    if (linker.constants != o->linker.constants)
        return 1;

    if ((hasAtlasTexture(textureProviders) && !geometryUsesTextureSubRect)
            || (hasAtlasTexture(o->textureProviders) && !o->geometryUsesTextureSubRect))
        return 1;

    for (int i = 0; i < textureProviders.count(); ++i) {
        QSGTextureProvider *tp1 = textureProviders.at(i);
        QSGTextureProvider *tp2 = o->textureProviders.at(i);
        if (!tp1 || !tp2)
            return tp1 == tp2 ? 0 : 1;
        QSGTexture *t1 = tp1->texture();
        QSGTexture *t2 = tp2->texture();
        if (!t1 || !t2)
            return t1 == t2 ? 0 : 1;
        if (int diff = t1->textureId() - t2->textureId())
            return diff;
    }

    return 0;
}

int QSGD3D12ShaderEffectMaterial::constantBufferSize() const
{
    return QSGD3D12Engine::alignedConstantBufferSize(linker.constantBufferSize);
}

void QSGD3D12ShaderEffectMaterial::preparePipeline(QSGD3D12PipelineState *pipelineState)
{
    pipelineState->shaders.vs = reinterpret_cast<const quint8 *>(linker.vs.constData());
    pipelineState->shaders.vsSize = linker.vs.size();
    pipelineState->shaders.ps = reinterpret_cast<const quint8 *>(linker.fs.constData());
    pipelineState->shaders.psSize = linker.fs.size();

    pipelineState->shaders.rootSig.textureViewCount = textureProviders.count();
}

static inline QColor qsg_premultiply_color(const QColor &c)
{
    return QColor::fromRgbF(c.redF() * c.alphaF(), c.greenF() * c.alphaF(), c.blueF() * c.alphaF(), c.alphaF());
}

QSGD3D12Material::UpdateResults QSGD3D12ShaderEffectMaterial::updatePipeline(const QSGD3D12MaterialRenderState &state,
                                                                             QSGD3D12PipelineState *pipelineState,
                                                                             ExtraState *,
                                                                             quint8 *constantBuffer)
{
    QSGD3D12Material::UpdateResults r = 0;
    quint8 *p = constantBuffer;

    for (auto it = linker.constants.constBegin(), itEnd = linker.constants.constEnd(); it != itEnd; ++it) {
        quint8 *dst = p + it.key();
        const QSGD3D12ShaderLinker::Constant &c(it.value());
        if (c.specialType == QSGShaderEffectNode::VariableData::Opacity) {
            if (state.isOpacityDirty()) {
                const float f = state.opacity();
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, &f, sizeof(f));
                r |= UpdatedConstantBuffer;
            }
        } else if (c.specialType == QSGShaderEffectNode::VariableData::Matrix) {
            if (state.isMatrixDirty()) {
                const int sz = 16 * sizeof(float);
                Q_ASSERT(sz == c.size);
                memcpy(dst, state.combinedMatrix().constData(), sz);
                r |= UpdatedConstantBuffer;
            }
        } else if (c.specialType == QSGShaderEffectNode::VariableData::SubRect) {
            // float4
            QRectF subRect(0, 0, 1, 1);
            int srcBindPoint = c.value.toInt(); // filled in by linkTextureSubRects
            if (QSGTexture *t = textureProviders.at(srcBindPoint)->texture())
                subRect = t->normalizedTextureSubRect();
            const float f[4] = { float(subRect.x()), float(subRect.y()),
                                 float(subRect.width()), float(subRect.height()) };
            Q_ASSERT(sizeof(f) == c.size);
            memcpy(dst, f, sizeof(f));
        } else if (c.specialType == QSGShaderEffectNode::VariableData::None) {
            r |= UpdatedConstantBuffer;
            switch (c.value.type()) {
            case QMetaType::QColor: {
                const QColor v = qsg_premultiply_color(qvariant_cast<QColor>(c.value));
                const float f[4] = { float(v.redF()), float(v.greenF()), float(v.blueF()), float(v.alphaF()) };
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, f, sizeof(f));
                break;
            }
            case QMetaType::Float: {
                const float f = qvariant_cast<float>(c.value);
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, &f, sizeof(f));
                break;
            }
            case QMetaType::Double: {
                const float f = float(qvariant_cast<double>(c.value));
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, &f, sizeof(f));
                break;
            }
            case QMetaType::Int: {
                const int i = c.value.toInt();
                Q_ASSERT(sizeof(i) == c.size);
                memcpy(dst, &i, sizeof(i));
                break;
            }
            case QMetaType::Bool: {
                const bool b = c.value.toBool();
                Q_ASSERT(sizeof(b) == c.size);
                memcpy(dst, &b, sizeof(b));
                break;
            }
            case QMetaType::QTransform: { // float3x3
                const QTransform v = qvariant_cast<QTransform>(c.value);
                const float m[3][3] = {
                    { float(v.m11()), float(v.m12()), float(v.m13()) },
                    { float(v.m21()), float(v.m22()), float(v.m23()) },
                    { float(v.m31()), float(v.m32()), float(v.m33()) }
                };
                Q_ASSERT(sizeof(m) == c.size);
                memcpy(dst, m[0], sizeof(m));
                break;
            }
            case QMetaType::QSize:
            case QMetaType::QSizeF: { // float2
                const QSizeF v = c.value.toSizeF();
                const float f[2] = { float(v.width()), float(v.height()) };
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, f, sizeof(f));
                break;
            }
            case QMetaType::QPoint:
            case QMetaType::QPointF: { // float2
                const QPointF v = c.value.toPointF();
                const float f[2] = { float(v.x()), float(v.y()) };
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, f, sizeof(f));
                break;
            }
            case QMetaType::QRect:
            case QMetaType::QRectF: { // float4
                const QRectF v = c.value.toRectF();
                const float f[4] = { float(v.x()), float(v.y()), float(v.width()), float(v.height()) };
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, f, sizeof(f));
                break;
            }
            case QMetaType::QVector2D: { // float2
                const QVector2D v = qvariant_cast<QVector2D>(c.value);
                const float f[2] = { float(v.x()), float(v.y()) };
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, f, sizeof(f));
                break;
            }
            case QMetaType::QVector3D: { // float3
                const QVector3D v = qvariant_cast<QVector3D>(c.value);
                const float f[3] = { float(v.x()), float(v.y()), float(v.z()) };
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, f, sizeof(f));
                break;
            }
            case QMetaType::QVector4D: { // float4
                const QVector4D v = qvariant_cast<QVector4D>(c.value);
                const float f[4] = { float(v.x()), float(v.y()), float(v.z()), float(v.w()) };
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, f, sizeof(f));
                break;
            }
            case QMetaType::QQuaternion: { // float4
                const QQuaternion v = qvariant_cast<QQuaternion>(c.value);
                const float f[4] = { float(v.x()), float(v.y()), float(v.z()), float(v.scalar()) };
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, f, sizeof(f));
                break;
            }
            case QMetaType::QMatrix4x4: { // float4x4
                const QMatrix4x4 v = qvariant_cast<QMatrix4x4>(c.value);
                const int sz = 16 * sizeof(float);
                Q_ASSERT(sz == c.size);
                memcpy(dst, v.constData(), sz);
                break;
            }
            default:
                break;
            }
        }
    }

    for (int i = 0; i < textureProviders.count(); ++i) {
        QSGTextureProvider *tp = textureProviders[i];
        QSGD3D12TextureView &tv(pipelineState->shaders.rootSig.textureViews[i]);
        if (tp) {
            if (QSGTexture *t = tp->texture()) {
                if (t->isAtlasTexture() && !geometryUsesTextureSubRect) {
                    QSGTexture *newTexture = t->removedFromAtlas();
                    if (newTexture)
                        t = newTexture;
                }
                tv.filter = t->filtering() == QSGTexture::Linear
                        ? QSGD3D12TextureView::FilterLinear : QSGD3D12TextureView::FilterNearest;
                tv.addressModeHoriz = t->horizontalWrapMode() == QSGTexture::ClampToEdge
                        ? QSGD3D12TextureView::AddressClamp : QSGD3D12TextureView::AddressWrap;
                tv.addressModeVert = t->verticalWrapMode() == QSGTexture::ClampToEdge
                        ? QSGD3D12TextureView::AddressClamp : QSGD3D12TextureView::AddressWrap;
                t->bind();
                continue;
            }
        }
        if (!dummy) {
            dummy = new QSGD3D12Texture(node->renderContext()->engine());
            QImage img(128, 128, QImage::Format_ARGB32_Premultiplied);
            img.fill(0);
            dummy->create(img, QSGRenderContext::CreateTexture_Alpha);
        }
        tv.filter = QSGD3D12TextureView::FilterNearest;
        tv.addressModeHoriz = QSGD3D12TextureView::AddressWrap;
        tv.addressModeVert = QSGD3D12TextureView::AddressWrap;
        dummy->bind();
    }

    switch (cullMode) {
    case QSGShaderEffectNode::FrontFaceCulling:
        pipelineState->cullMode = QSGD3D12PipelineState::CullFront;
        break;
    case QSGShaderEffectNode::BackFaceCulling:
        pipelineState->cullMode = QSGD3D12PipelineState::CullBack;
        break;
    default:
        pipelineState->cullMode = QSGD3D12PipelineState::CullNone;
        break;
    }

    return r;
}

void QSGD3D12ShaderEffectMaterial::updateTextureProviders(bool layoutChange)
{
    if (layoutChange) {
        for (QSGTextureProvider *tp : textureProviders) {
            if (tp) {
                QObject::disconnect(tp, SIGNAL(textureChanged()), node,
                                    SLOT(handleTextureChange()));
                QObject::disconnect(tp, SIGNAL(destroyed(QObject*)), node,
                                    SLOT(handleTextureProviderDestroyed(QObject*)));
            }
        }

        textureProviders.fill(nullptr, linker.textures.count());
    }

    for (auto it = linker.textures.constBegin(), itEnd = linker.textures.constEnd(); it != itEnd; ++it) {
        const int bindPoint = it.key();
        // Now that the linker has merged the textures, we can switch over to a
        // simple vector indexed by the binding point for textureProviders.
        Q_ASSERT(bindPoint >= 0 && bindPoint < textureProviders.count());
        QQuickItem *source = qobject_cast<QQuickItem *>(qvariant_cast<QObject *>(it.value()));
        QSGTextureProvider *newProvider = source && source->isTextureProvider() ? source->textureProvider() : nullptr;
        QSGTextureProvider *&activeProvider(textureProviders[bindPoint]);
        if (newProvider != activeProvider) {
            if (activeProvider) {
                QObject::disconnect(activeProvider, SIGNAL(textureChanged()), node,
                                    SLOT(handleTextureChange()));
                QObject::disconnect(activeProvider, SIGNAL(destroyed(QObject*)), node,
                                    SLOT(handleTextureProviderDestroyed(QObject*)));
            }
            if (newProvider) {
                Q_ASSERT_X(newProvider->thread() == QThread::currentThread(),
                           "QSGD3D12ShaderEffectMaterial::updateTextureProviders",
                           "Texture provider must belong to the rendering thread");
                QObject::connect(newProvider, SIGNAL(textureChanged()), node, SLOT(handleTextureChange()));
                QObject::connect(newProvider, SIGNAL(destroyed(QObject*)), node,
                                 SLOT(handleTextureProviderDestroyed(QObject*)));
            } else {
                const char *typeName = source ? source->metaObject()->className() : it.value().typeName();
                qWarning("ShaderEffect: Texture t%d is not assigned a valid texture provider (%s).",
                         bindPoint, typeName);
            }
            activeProvider = newProvider;
        }
    }
}

QSGD3D12ShaderEffectNode::QSGD3D12ShaderEffectNode(QSGD3D12RenderContext *rc, QSGD3D12GuiThreadShaderEffectManager *mgr)
    : QSGShaderEffectNode(mgr),
      m_rc(rc),
      m_mgr(mgr),
      m_material(this)
{
    setFlag(UsePreprocess, true);
    setMaterial(&m_material);
}

QRectF QSGD3D12ShaderEffectNode::updateNormalizedTextureSubRect(bool supportsAtlasTextures)
{
    QRectF srcRect(0, 0, 1, 1);
    bool geometryUsesTextureSubRect = false;
    if (supportsAtlasTextures && m_material.textureProviders.count() == 1) {
        QSGTextureProvider *provider = m_material.textureProviders.at(0);
        if (provider->texture()) {
            srcRect = provider->texture()->normalizedTextureSubRect();
            geometryUsesTextureSubRect = true;
        }
    }

    if (m_material.geometryUsesTextureSubRect != geometryUsesTextureSubRect) {
        m_material.geometryUsesTextureSubRect = geometryUsesTextureSubRect;
        markDirty(QSGNode::DirtyMaterial);
    }

    return srcRect;
}

void QSGD3D12ShaderEffectNode::syncMaterial(SyncData *syncData)
{
    if (Q_UNLIKELY(debug_shader()))
        qDebug() << "shadereffect node sync" << syncData->dirty;

    if (bool(m_material.flags() & QSGMaterial::Blending) != syncData->blending) {
        m_material.setFlag(QSGMaterial::Blending, syncData->blending);
        markDirty(QSGNode::DirtyMaterial);
    }

    if (m_material.cullMode != syncData->cullMode) {
        m_material.cullMode = syncData->cullMode;
        markDirty(QSGNode::DirtyMaterial);
    }

    if (syncData->dirty & QSGShaderEffectNode::DirtyShaders) {
        QByteArray vertBlob, fragBlob;

        m_material.hasCustomVertexShader = syncData->vertex.shader->hasShaderCode;
        if (m_material.hasCustomVertexShader) {
            vertBlob = syncData->vertex.shader->shaderInfo.blob;
        } else {
            vertBlob = QByteArray::fromRawData(reinterpret_cast<const char *>(g_VS_DefaultShaderEffect),
                                               sizeof(g_VS_DefaultShaderEffect));
        }

        m_material.hasCustomFragmentShader = syncData->fragment.shader->hasShaderCode;
        if (m_material.hasCustomFragmentShader) {
            fragBlob = syncData->fragment.shader->shaderInfo.blob;
        } else {
            fragBlob = QByteArray::fromRawData(reinterpret_cast<const char *>(g_PS_DefaultShaderEffect),
                                               sizeof(g_PS_DefaultShaderEffect));
        }

        m_material.mtype = shaderMaterialTypeCache()->get(vertBlob, fragBlob);
        m_material.linker.reset(vertBlob, fragBlob);

        if (m_material.hasCustomVertexShader) {
            m_material.linker.feedConstants(*syncData->vertex.shader);
        } else {
            QSGShaderEffectNode::ShaderData defaultSD;
            defaultSD.shaderInfo.blob = vertBlob;
            defaultSD.shaderInfo.type = QSGGuiThreadShaderEffectManager::ShaderInfo::TypeVertex;

            // { float4x4 qt_Matrix; float qt_Opacity; } where only the matrix is used
            QSGGuiThreadShaderEffectManager::ShaderInfo::Variable v;
            v.name = QByteArrayLiteral("qt_Matrix");
            v.offset = 0;
            v.size = 16 * sizeof(float);
            defaultSD.shaderInfo.variables.append(v);
            QSGShaderEffectNode::VariableData vd;
            vd.specialType = QSGShaderEffectNode::VariableData::Matrix;
            defaultSD.varData.append(vd);
            defaultSD.shaderInfo.constantDataSize = (16 + 1) * sizeof(float);
            m_material.linker.feedConstants(defaultSD);
        }

        m_material.linker.feedSamplers(*syncData->vertex.shader);
        m_material.linker.feedTextures(*syncData->vertex.shader);

        if (m_material.hasCustomFragmentShader) {
            m_material.linker.feedConstants(*syncData->fragment.shader);
        } else {
            QSGShaderEffectNode::ShaderData defaultSD;
            defaultSD.shaderInfo.blob = fragBlob;
            defaultSD.shaderInfo.type = QSGGuiThreadShaderEffectManager::ShaderInfo::TypeFragment;

            // { float4x4 qt_Matrix; float qt_Opacity; } where only the opacity is used
            QSGGuiThreadShaderEffectManager::ShaderInfo::Variable v;
            v.name = QByteArrayLiteral("qt_Opacity");
            v.offset = 16 * sizeof(float);
            v.size = sizeof(float);
            defaultSD.shaderInfo.variables.append(v);
            QSGShaderEffectNode::VariableData vd;
            vd.specialType = QSGShaderEffectNode::VariableData::Opacity;
            defaultSD.varData.append(vd);

            v.name = QByteArrayLiteral("source");
            v.bindPoint = 0;
            v.type = QSGGuiThreadShaderEffectManager::ShaderInfo::Texture;
            defaultSD.shaderInfo.variables.append(v);
            vd.specialType = QSGShaderEffectNode::VariableData::Source;
            defaultSD.varData.append(vd);

            v.name = QByteArrayLiteral("sourceSampler");
            v.bindPoint = 0;
            v.type = QSGGuiThreadShaderEffectManager::ShaderInfo::Sampler;
            defaultSD.shaderInfo.variables.append(v);
            vd.specialType = QSGShaderEffectNode::VariableData::Unused;
            defaultSD.varData.append(vd);

            defaultSD.shaderInfo.constantDataSize = (16 + 1) * sizeof(float);

            m_material.linker.feedConstants(defaultSD);
            m_material.linker.feedSamplers(defaultSD);
            m_material.linker.feedTextures(defaultSD);
        }

        // While this may seem unnecessary for the built-in shaders, the value
        // of 'source' is still in there and we have to process it.
        m_material.linker.feedSamplers(*syncData->fragment.shader);
        m_material.linker.feedTextures(*syncData->fragment.shader);

        m_material.linker.linkTextureSubRects();

        m_material.updateTextureProviders(true);

        markDirty(QSGNode::DirtyMaterial);

        if (Q_UNLIKELY(debug_shader()))
            m_material.linker.dump();
    } else  {
        if (syncData->dirty & QSGShaderEffectNode::DirtyShaderConstant) {
            if (!syncData->vertex.dirtyConstants->isEmpty())
                m_material.linker.feedConstants(*syncData->vertex.shader, syncData->vertex.dirtyConstants);
            if (!syncData->fragment.dirtyConstants->isEmpty())
                m_material.linker.feedConstants(*syncData->fragment.shader, syncData->fragment.dirtyConstants);
            markDirty(QSGNode::DirtyMaterial);
            if (Q_UNLIKELY(debug_shader()))
                m_material.linker.dump();
        }

        if (syncData->dirty & QSGShaderEffectNode::DirtyShaderTexture) {
            if (!syncData->vertex.dirtyTextures->isEmpty())
                m_material.linker.feedTextures(*syncData->vertex.shader, syncData->vertex.dirtyTextures);
            if (!syncData->fragment.dirtyTextures->isEmpty())
                m_material.linker.feedTextures(*syncData->fragment.shader, syncData->fragment.dirtyTextures);
            m_material.linker.linkTextureSubRects();
            m_material.updateTextureProviders(false);
            markDirty(QSGNode::DirtyMaterial);
            if (Q_UNLIKELY(debug_shader()))
                m_material.linker.dump();
        }
    }

    if (bool(m_material.flags() & QSGMaterial::RequiresFullMatrix) != m_material.hasCustomVertexShader) {
        m_material.setFlag(QSGMaterial::RequiresFullMatrix, m_material.hasCustomVertexShader);
        markDirty(QSGNode::DirtyMaterial);
    }
}

void QSGD3D12ShaderEffectNode::handleTextureChange()
{
    markDirty(QSGNode::DirtyMaterial);
    emit m_mgr->textureChanged();
}

void QSGD3D12ShaderEffectNode::handleTextureProviderDestroyed(QObject *object)
{
    for (QSGTextureProvider *&tp : m_material.textureProviders) {
        if (tp == object)
            tp = nullptr;
    }
}

void QSGD3D12ShaderEffectNode::preprocess()
{
    for (QSGTextureProvider *tp : m_material.textureProviders) {
        if (tp) {
            if (QSGDynamicTexture *texture = qobject_cast<QSGDynamicTexture *>(tp->texture()))
                texture->updateTexture();
        }
    }
}

bool QSGD3D12GuiThreadShaderEffectManager::hasSeparateSamplerAndTextureObjects() const
{
    return true;
}

QString QSGD3D12GuiThreadShaderEffectManager::log() const
{
    return m_log;
}

QSGGuiThreadShaderEffectManager::Status QSGD3D12GuiThreadShaderEffectManager::status() const
{
    return m_status;
}

struct RefGuard {
    RefGuard(IUnknown *p) : p(p) { }
    ~RefGuard() { p->Release(); }
    IUnknown *p;
};

class QSGD3D12ShaderCompileTask : public QRunnable
{
public:
    QSGD3D12ShaderCompileTask(QSGD3D12GuiThreadShaderEffectManager *mgr,
                              QSGD3D12GuiThreadShaderEffectManager::ShaderInfo::Type typeHint,
                              const QByteArray &src,
                              QSGD3D12GuiThreadShaderEffectManager::ShaderInfo *result)
        : mgr(mgr), typeHint(typeHint), src(src), result(result) { }

    void run() override;

private:
    QSGD3D12GuiThreadShaderEffectManager *mgr;
    QSGD3D12GuiThreadShaderEffectManager::ShaderInfo::Type typeHint;
    QByteArray src;
    QSGD3D12GuiThreadShaderEffectManager::ShaderInfo *result;
};

void QSGD3D12ShaderCompileTask::run()
{
    const char *target = typeHint == QSGD3D12GuiThreadShaderEffectManager::ShaderInfo::TypeVertex ? "vs_5_0" : "ps_5_0";
    ID3DBlob *bytecode = nullptr;
    ID3DBlob *errors = nullptr;
    HRESULT hr = D3DCompile(src.constData(), src.size(), nullptr, nullptr, nullptr,
                            "main", target, 0, 0, &bytecode, &errors);
    if (FAILED(hr) || !bytecode) {
        qWarning("HLSL shader compilation failed: 0x%x", hr);
        if (errors) {
            mgr->m_log += QString::fromUtf8(static_cast<const char *>(errors->GetBufferPointer()), errors->GetBufferSize());
            errors->Release();
        }
        mgr->m_status = QSGGuiThreadShaderEffectManager::Error;
        emit mgr->shaderCodePrepared(false, typeHint, src, result);
        emit mgr->logAndStatusChanged();
        return;
    }

    result->blob.resize(bytecode->GetBufferSize());
    memcpy(result->blob.data(), bytecode->GetBufferPointer(), result->blob.size());
    bytecode->Release();

    const bool ok = mgr->reflect(result);
    mgr->m_status = ok ? QSGGuiThreadShaderEffectManager::Compiled : QSGGuiThreadShaderEffectManager::Error;
    emit mgr->shaderCodePrepared(ok, typeHint, src, result);
    emit mgr->logAndStatusChanged();
}

static const int BYTECODE_MAGIC = 0x43425844; // 'DXBC'

void QSGD3D12GuiThreadShaderEffectManager::prepareShaderCode(ShaderInfo::Type typeHint, const QByteArray &src, ShaderInfo *result)
{
    // The D3D12 backend's ShaderEffect implementation supports both HLSL
    // source strings and bytecode or source in files as input. Bytecode is
    // strongly recommended, but in order to make ShaderEffect users' (and
    // anything that stiches shader strings together dynamically, e.g.
    // qtgraphicaleffects) life easier, and since we link to d3dcompiler
    // anyways, compiling from source is also supported.

    QByteArray shaderSourceCode = src;
    QUrl srcUrl(QString::fromUtf8(src));
    if (!srcUrl.scheme().compare(QLatin1String("qrc"), Qt::CaseInsensitive) || srcUrl.isLocalFile()) {
        if (!m_fileSelector) {
            m_fileSelector = new QFileSelector(this);
            m_fileSelector->setExtraSelectors(QStringList() << QStringLiteral("hlsl"));
        }
        const QString fn = m_fileSelector->select(QQmlFile::urlToLocalFileOrQrc(srcUrl));
        QFile f(fn);
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning("ShaderEffect: Failed to read %s", qPrintable(fn));
            emit shaderCodePrepared(false, typeHint, src, result);
            return;
        }
        QByteArray blob = f.readAll();
        f.close();
        if (blob.size() > 4) {
            const quint32 *p = reinterpret_cast<const quint32 *>(blob.constData());
            if (*p == BYTECODE_MAGIC) {
                // already compiled D3D bytecode, skip straight to reflection
                result->blob = blob;
                const bool ok = reflect(result);
                m_status = ok ? Compiled : Error;
                emit shaderCodePrepared(ok, typeHint, src, result);
                emit logAndStatusChanged();
                return;
            }
            // assume the file contained HLSL source code
            shaderSourceCode = blob;
        }
    }

    QThreadPool::globalInstance()->start(new QSGD3D12ShaderCompileTask(this, typeHint, shaderSourceCode, result));
}

bool QSGD3D12GuiThreadShaderEffectManager::reflect(ShaderInfo *result)
{
    ID3D12ShaderReflection *reflector;
    HRESULT hr = D3DReflect(result->blob.constData(), result->blob.size(), IID_PPV_ARGS(&reflector));
    if (FAILED(hr)) {
        qWarning("D3D shader reflection failed: 0x%x", hr);
        return false;
    }
    RefGuard rg(reflector);

    D3D12_SHADER_DESC shaderDesc;
    reflector->GetDesc(&shaderDesc);

    const uint progType = (shaderDesc.Version & 0xFFFF0000) >> 16;
    const uint major = (shaderDesc.Version & 0x000000F0) >> 4;
    const uint minor = (shaderDesc.Version & 0x0000000F);

    switch (progType) {
    case D3D12_SHVER_VERTEX_SHADER:
        result->type = ShaderInfo::TypeVertex;
        break;
    case D3D12_SHVER_PIXEL_SHADER:
        result->type = ShaderInfo::TypeFragment;
        break;
    default:
        result->type = ShaderInfo::TypeOther;
        qWarning("D3D shader is of unknown type 0x%x", shaderDesc.Version);
        return false;
    }

    if (major < 5) {
        qWarning("D3D shader model version %u.%u is too low", major, minor);
        return false;
    }

    const int ieCount = shaderDesc.InputParameters;
    const int cbufferCount = shaderDesc.ConstantBuffers;
    const int boundResCount = shaderDesc.BoundResources;

    result->constantDataSize = 0;

    if (ieCount < 1) {
        qWarning("Invalid shader: Not enough input parameters (%d)", ieCount);
        return false;
    }
    if (cbufferCount < 1) {
        qWarning("Invalid shader: Shader has no constant buffers");
        return false;
    }
    if (boundResCount < 1) {
        qWarning("Invalid shader: No resources bound. Expected to have at least a constant buffer bound.");
        return false;
    }

    if (Q_UNLIKELY(debug_shader()))
        qDebug("Shader reflection size %d type %d v%u.%u input elems %d cbuffers %d boundres %d",
               result->blob.size(), result->type, major, minor, ieCount, cbufferCount, boundResCount);

    for (int i = 0; i < boundResCount; ++i) {
        D3D12_SHADER_INPUT_BIND_DESC desc;
        if (FAILED(reflector->GetResourceBindingDesc(i, &desc))) {
            qWarning("D3D reflection: Failed to query resource binding %d", i);
            continue;
        }
        bool gotCBuffer = false;
        if (desc.Type == D3D_SIT_CBUFFER) {
            ID3D12ShaderReflectionConstantBuffer *cbuf = reflector->GetConstantBufferByName(desc.Name);
            D3D12_SHADER_BUFFER_DESC bufDesc;
            if (FAILED(cbuf->GetDesc(&bufDesc))) {
                qWarning("D3D reflection: Failed to query constant buffer description");
                continue;
            }
            if (gotCBuffer) {
                qWarning("D3D reflection: Found more than one constant buffers. Only the first one is used.");
                continue;
            }
            gotCBuffer = true;
            result->constantDataSize = bufDesc.Size;
            for (uint cbIdx = 0; cbIdx < bufDesc.Variables; ++cbIdx) {
                ID3D12ShaderReflectionVariable *cvar = cbuf->GetVariableByIndex(cbIdx);
                D3D12_SHADER_VARIABLE_DESC varDesc;
                if (FAILED(cvar->GetDesc(&varDesc))) {
                    qWarning("D3D reflection: Failed to query constant buffer variable %d", cbIdx);
                    return false;
                }
                // we report the full size of the buffer but only return variables that are actually used by this shader
                if (!(varDesc.uFlags & D3D_SVF_USED))
                    continue;
                ShaderInfo::Variable v;
                v.type = ShaderInfo::Constant;
                v.name = QByteArray(varDesc.Name);
                v.offset = varDesc.StartOffset;
                v.size = varDesc.Size;
                result->variables.append(v);
            }
        } else if (desc.Type == D3D_SIT_TEXTURE) {
            if (desc.Dimension != D3D_SRV_DIMENSION_TEXTURE2D) {
                qWarning("D3D reflection: Texture %s is not a 2D texture, ignoring.", qPrintable(desc.Name));
                continue;
            }
            if (desc.NumSamples != (UINT) -1) {
                qWarning("D3D reflection: Texture %s is multisample (%u), ignoring.", qPrintable(desc.Name), desc.NumSamples);
                continue;
            }
            if (desc.BindCount != 1) {
                qWarning("D3D reflection: Texture %s is an array, ignoring.", qPrintable(desc.Name));
                continue;
            }
            if (desc.Space != 0) {
                qWarning("D3D reflection: Texture %s is not using register space 0, ignoring.", qPrintable(desc.Name));
                continue;
            }
            ShaderInfo::Variable v;
            v.type = ShaderInfo::Texture;
            v.name = QByteArray(desc.Name);
            v.bindPoint = desc.BindPoint;
            result->variables.append(v);
        } else if (desc.Type == D3D_SIT_SAMPLER) {
            if (desc.BindCount != 1) {
                qWarning("D3D reflection: Sampler %s is an array, ignoring.", qPrintable(desc.Name));
                continue;
            }
            if (desc.Space != 0) {
                qWarning("D3D reflection: Sampler %s is not using register space 0, ignoring.", qPrintable(desc.Name));
                continue;
            }
            ShaderInfo::Variable v;
            v.type = ShaderInfo::Sampler;
            v.name = QByteArray(desc.Name);
            v.bindPoint = desc.BindPoint;
            result->variables.append(v);
        } else {
            qWarning("D3D reflection: Resource binding %d has an unknown type of %d and will be ignored.", i, desc.Type);
            continue;
        }
    }

    if (Q_UNLIKELY(debug_shader()))
        qDebug() << "Variables:" << result->variables << "cbuffer size" << result->constantDataSize;

    return true;
}

QT_END_NAMESPACE
