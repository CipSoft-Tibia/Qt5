// Copyright (C) 2019 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "texturesubmissioncontext_p.h"

#include <graphicscontext_p.h>
#include <gltexture_p.h>
#include <logging_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {
namespace Render {
namespace OpenGL {

class TextureExtRendererLocker
{
public:
    static void lock(GLTexture *tex)
    {
        if (!tex->isExternalRenderingEnabled())
            return;
        if (s_lockHash.keys().contains(tex)) {
            ++s_lockHash[tex];
        } else {
            tex->externalRenderingLock()->lock();
            s_lockHash[tex] = 1;
        }
    }
    static void unlock(GLTexture *tex)
    {
        if (!tex->isExternalRenderingEnabled())
            return;
        if (!s_lockHash.keys().contains(tex))
            return;

        --s_lockHash[tex];
        if (s_lockHash[tex] == 0) {
            s_lockHash.remove(tex);
            tex->externalRenderingLock()->unlock();
        }
    }
private:
    static QHash<GLTexture*, int> s_lockHash;
};

QHash<GLTexture*, int> TextureExtRendererLocker::s_lockHash = QHash<GLTexture*, int>();


TextureSubmissionContext::TextureSubmissionContext()
{

}

TextureSubmissionContext::~TextureSubmissionContext()
{

}

void TextureSubmissionContext::initialize(GraphicsContext *context)
{
    m_activeTextures.resize(context->maxTextureUnitsCount());
}

void TextureSubmissionContext::endDrawing()
{
    decayTextureScores();
    for (size_t i = 0; i < m_activeTextures.size(); ++i)
        if (m_activeTextures[i].texture)
            TextureExtRendererLocker::unlock(m_activeTextures[i].texture);
}

int TextureSubmissionContext::activateTexture(TextureSubmissionContext::TextureScope scope,
                                              QOpenGLContext *m_gl,
                                              GLTexture *tex)
{
    // Returns the texture unit to use for the texture
    // This always return a valid unit, unless there are more textures than
    // texture unit available for the current material
    const int onUnit = assignUnitForTexture(tex);

    // check we didn't overflow the available units
    if (onUnit == -1)
        return -1;

    const int sharedTextureId = tex->sharedTextureId();
    // We have a valid texture id provided by a shared context
    if (sharedTextureId > 0) {
        m_gl->functions()->glActiveTexture(GL_TEXTURE0 + onUnit);
        const QAbstractTexture::Target target = tex->properties().target;
        // For now we know that target values correspond to the GL values
        m_gl->functions()->glBindTexture(target, tex->sharedTextureId());
    } else {
        // Texture must have been created and updated at this point
        QOpenGLTexture *glTex = tex->getGLTexture();
        if (glTex == nullptr)
            return -1;
        glTex->bind(uint(onUnit));
    }
    if (m_activeTextures[onUnit].texture != tex) {
        if (m_activeTextures[onUnit].texture)
            TextureExtRendererLocker::unlock(m_activeTextures[onUnit].texture);
        m_activeTextures[onUnit].texture = tex;
        TextureExtRendererLocker::lock(tex);
    }

#if defined(QT3D_RENDER_ASPECT_OPENGL_DEBUG)
    int err = m_gl->functions()->glGetError();
    if (err)
        qCWarning(Backend) << "GL error after activating texture" << QString::number(err, 16)
                           << tex->getGLTexture()->textureId() << "on unit" << onUnit;
#endif

    m_activeTextures[onUnit].score = 200;
    m_activeTextures[onUnit].pinned = true;
    m_activeTextures[onUnit].scope = scope;

    return onUnit;
}

void TextureSubmissionContext::deactivateTexturesWithScope(TextureSubmissionContext::TextureScope ts)
{
    for (size_t u=0; u<m_activeTextures.size(); ++u) {
        if (!m_activeTextures[u].pinned)
            continue; // inactive, ignore

        if (m_activeTextures[u].scope == ts) {
            m_activeTextures[u].pinned = false;
            m_activeTextures[u].score = qMax(m_activeTextures[u].score, 1) - 1;
        }
    } // of units iteration
}

void TextureSubmissionContext::deactivateTexture(GLTexture* tex)
{
    for (size_t u=0; u<m_activeTextures.size(); ++u) {
        if (m_activeTextures[u].texture == tex) {
            Q_ASSERT(m_activeTextures[u].pinned);
            m_activeTextures[u].pinned = false;
            return;
        }
    } // of units iteration

    qCWarning(Backend) << Q_FUNC_INFO << "texture not active:" << tex;
}

/*!
    \internal
    Returns a texture unit for a texture, -1 if all texture units are assigned.
    Tries to use the texture unit with the texture that hasn't been used for the longest time
    if the texture happens not to be already pinned on a texture unit.
 */
int TextureSubmissionContext::assignUnitForTexture(GLTexture *tex)
{
    int lowestScoredUnit = -1;
    int lowestScore = 0xfffffff;

    for (size_t u=0; u<m_activeTextures.size(); ++u) {
        if (m_activeTextures[u].texture == tex)
            return int(u);
    }

    for (size_t u=0; u<m_activeTextures.size(); ++u) {
        // No texture is currently active on the texture unit
        // we save the texture unit with the texture that has been on there
        // the longest time while not being used
        if (!m_activeTextures[u].pinned) {
            int score = m_activeTextures[u].score;
            if (score < lowestScore) {
                lowestScore = score;
                lowestScoredUnit = int(u);
            }
        }
    } // of units iteration

    if (lowestScoredUnit == -1)
        qCWarning(Backend) << Q_FUNC_INFO << "No free texture units!";

    return lowestScoredUnit;
}

void TextureSubmissionContext::decayTextureScores()
{
    for (size_t u = 0; u < m_activeTextures.size(); u++)
        m_activeTextures[u].score = qMax(m_activeTextures[u].score - 1, 0);
}

} // namespace OpenGL
} // namespace Render
} // namespace Qt3DRender of namespace

QT_END_NAMESPACE
