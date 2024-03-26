// Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qrawfont.h>
#include <QtGui/qglyphrun.h>
#include <QtGui/private/qrawfont_p.h>

#include "qdistancefieldglyphcache_p.h"
#include "qtextureatlas_p.h"

#include <QtGui/qpainterpath.h>
#include <QtGui/qfont.h>
#include <QtGui/qpainterpath.h>
#include <QtGui/private/qdistancefield_p.h>
#include <Qt3DCore/private/qnode_p.h>
#include <Qt3DExtras/private/qtextureatlas_p.h>

QT_BEGIN_NAMESPACE

#define DEFAULT_IMAGE_PADDING 1

using namespace Qt3DCore;

namespace Qt3DExtras {

// ref-count glyphs and keep track of where they are stored
class StoredGlyph {
public:
    StoredGlyph() = default;
    StoredGlyph(const QRawFont &font, quint32 glyph, bool doubleResolution);

    int refCount() const { return int(m_ref); }
    void ref() { ++m_ref; }
    int deref() { m_ref = std::max(m_ref - 1, quint32(0)); return int(m_ref); }

    bool addToTextureAtlas(QTextureAtlas *atlas);
    void removeFromTextureAtlas();

    QTextureAtlas *atlas() const { return m_atlas; }
    QRectF glyphPathBoundingRect() const { return m_glyphPathBoundingRect; }
    QRectF texCoords() const;

private:
    quint32 m_ref = 0;
    QTextureAtlas *m_atlas = nullptr;
    QTextureAtlas::TextureId m_atlasEntry = QTextureAtlas::InvalidTexture;
    QRectF m_glyphPathBoundingRect;
    QImage m_distanceFieldImage;    // only used until added to texture atlas
};

// A DistanceFieldFont stores all glyphs for a given QRawFont.
// it will use multiple QTextureAtlasess to store the distance
// fields and uses ref-counting for each glyph to ensure that
// unused glyphs are removed from the texture atlasses.
class DistanceFieldFont
{
public:
    DistanceFieldFont(const QRawFont &font, bool doubleRes, Qt3DCore::QNode *parent);
    ~DistanceFieldFont();

    StoredGlyph findGlyph(quint32 glyph) const;
    StoredGlyph refGlyph(quint32 glyph);
    void derefGlyph(quint32 glyph);

    bool doubleGlyphResolution() const { return m_doubleGlyphResolution; }

private:
    QRawFont m_font;
    bool m_doubleGlyphResolution;
    Qt3DCore::QNode *m_parentNode; // parent node for the QTextureAtlasses

    QHash<quint32, StoredGlyph> m_glyphs;

    QList<QTextureAtlas*> m_atlasses;
};

StoredGlyph::StoredGlyph(const QRawFont &font, quint32 glyph, bool doubleResolution)
    : m_ref(1)
    , m_atlas(nullptr)
    , m_atlasEntry(QTextureAtlas::InvalidTexture)
{
    // create new single-channel distance field image for given glyph
    const QPainterPath path = font.pathForGlyph(glyph);
    const QDistanceField dfield(font, glyph, doubleResolution);
    m_distanceFieldImage = dfield.toImage(QImage::Format_Alpha8);

    // scale bounding rect down (as in QSGDistanceFieldGlyphCache::glyphData())
    const QRectF pathBound = path.boundingRect();
    float f = 1.0f / QT_DISTANCEFIELD_SCALE(doubleResolution);
    m_glyphPathBoundingRect = QRectF(pathBound.left() * f, -pathBound.top() * f, pathBound.width() * f, pathBound.height() * f);
}

bool StoredGlyph::addToTextureAtlas(QTextureAtlas *atlas)
{
    if (m_atlas || m_distanceFieldImage.isNull())
        return false;

    const auto texId = atlas->addImage(m_distanceFieldImage, DEFAULT_IMAGE_PADDING);
    if (texId != QTextureAtlas::InvalidTexture) {
        m_atlas = atlas;
        m_atlasEntry = texId;
        m_distanceFieldImage = QImage();    // free glyph image data
        return true;
    }

    return false;
}

void StoredGlyph::removeFromTextureAtlas()
{
    if (m_atlas) {
        m_atlas->removeImage(m_atlasEntry);
        m_atlas = nullptr;
        m_atlasEntry = QTextureAtlas::InvalidTexture;
    }
}

QRectF StoredGlyph::texCoords() const
{
    return m_atlas ? m_atlas->imageTexCoords(m_atlasEntry) : QRectF();
}

DistanceFieldFont::DistanceFieldFont(const QRawFont &font, bool doubleRes, Qt3DCore::QNode *parent)
    : m_font(font)
    , m_doubleGlyphResolution(doubleRes)
    , m_parentNode(parent)
{
    Q_ASSERT(m_parentNode);
}

DistanceFieldFont::~DistanceFieldFont()
{
    qDeleteAll(m_atlasses);
}

StoredGlyph DistanceFieldFont::findGlyph(quint32 glyph) const
{
    const auto it = m_glyphs.find(glyph);
    return (it != m_glyphs.cend()) ? it.value() : StoredGlyph();
}

StoredGlyph DistanceFieldFont::refGlyph(quint32 glyph)
{
    // if glyph already exists, just increase ref-count
    auto it = m_glyphs.find(glyph);
    if (it != m_glyphs.end()) {
        it.value().ref();
        return it.value();
    }

    // need to create new glyph
    StoredGlyph storedGlyph(m_font, glyph, m_doubleGlyphResolution);

    // see if one of the existing atlasses can hold the distance field image
    for (int i = 0; i < m_atlasses.size(); i++)
        if (storedGlyph.addToTextureAtlas(m_atlasses[i]))
            break;

    // if no texture atlas is big enough (or no exists yet), allocate a new one
    if (!storedGlyph.atlas()) {
        // this should be enough to store 40-60 glyphs, which should be sufficient for most
        // scenarios
        const int size = m_doubleGlyphResolution ? 512 : 256;

        QTextureAtlas *atlas = new QTextureAtlas();
        atlas->setWidth(size);
        atlas->setHeight(size);
        atlas->setFormat(Qt3DRender::QAbstractTexture::R8_UNorm);
        atlas->setPixelFormat(QOpenGLTexture::Red);
        atlas->setMinificationFilter(Qt3DRender::QAbstractTexture::Linear);
        atlas->setMagnificationFilter(Qt3DRender::QAbstractTexture::Linear);
        atlas->setParent(m_parentNode);
        m_atlasses << atlas;

        if (!storedGlyph.addToTextureAtlas(atlas))
            qWarning() << Q_FUNC_INFO << "Couldn't add glyph to newly allocated atlas. Glyph could be huge?";
    }

    m_glyphs.insert(glyph, storedGlyph);
    return storedGlyph;
}

void DistanceFieldFont::derefGlyph(quint32 glyph)
{
    auto it = m_glyphs.find(glyph);
    if (it == m_glyphs.end())
        return;

    // TODO
    // possible optimization: keep unreferenced glyphs as the texture atlas
    // still has space. only if a new glyph needs to be allocated, and there
    // is no more space within the atlas, then we can actually remove the glyphs
    // from the atlasses.

    // remove glyph if no refs anymore
    if (it.value().deref() <= 0) {
        QTextureAtlas *atlas = it.value().atlas();
        it.value().removeFromTextureAtlas();

        // remove atlas, if it contains no glyphs anymore
        if (atlas && atlas->imageCount() == 0) {
            Q_ASSERT(m_atlasses.contains(atlas));

            m_atlasses.removeAll(atlas);

            // This function might have been called as a result of destroying
            // the scene root which traverses the entire scene tree. Calling
            // delete on the atlas here could lead to dangling pointers in the
            // least of children being traversed for destruction.
            atlas->deleteLater();
        }

        m_glyphs.erase(it);
    }
}

// copied from QSGDistanceFieldGlyphCacheManager::fontKey
// we use this function to compare QRawFonts, as QRawFont doesn't
// implement a stable comparison function
QString QDistanceFieldGlyphCache::fontKey(const QRawFont &font)
{
    QFontEngine *fe = QRawFontPrivate::get(font)->fontEngine;
    if (!fe->faceId().filename.isEmpty()) {
        QByteArray keyName = fe->faceId().filename;
        if (font.style() != QFont::StyleNormal)
            keyName += QByteArray(" I");
        if (font.weight() != QFont::Normal)
            keyName += ' ' + QByteArray::number(font.weight());
        keyName += QByteArray(" DF");
        return QString::fromUtf8(keyName);
    } else {
        return QString::fromLatin1("%1_%2_%3_%4")
                  .arg(font.familyName())
                  .arg(font.styleName())
                  .arg(font.weight())
                  .arg(font.style());
    }
}

DistanceFieldFont* QDistanceFieldGlyphCache::getOrCreateDistanceFieldFont(const QRawFont &font)
{
    // return, if font already exists (make sure to only create one DistanceFieldFont for
    // each unique QRawFont, by building a hash on the QRawFont that ignores the font size)
    const QString key = fontKey(font);
    const auto it = m_fonts.constFind(key);
    if (it != m_fonts.cend())
        return it.value();

    // logic taken from QSGDistanceFieldGlyphCache::QSGDistanceFieldGlyphCache
    QRawFontPrivate *fontD = QRawFontPrivate::get(font);
    const int glyphCount = fontD->fontEngine->glyphCount();
    const bool useDoubleRes = qt_fontHasNarrowOutlines(font) && glyphCount < QT_DISTANCEFIELD_HIGHGLYPHCOUNT();

    // only keep one FontCache with a fixed pixel size for each distinct font type
    QRawFont actualFont = font;
    actualFont.setPixelSize(QT_DISTANCEFIELD_BASEFONTSIZE(useDoubleRes) * QT_DISTANCEFIELD_SCALE(useDoubleRes));

    // create new font cache
    // we set the parent node to nullptr, since the parent node of QTextureAtlasses
    // will be set when we pass them to QText2DMaterial later
    Q_ASSERT(m_rootNode);
    DistanceFieldFont *dff = new DistanceFieldFont(actualFont, useDoubleRes, m_rootNode);
    m_fonts.insert(key, dff);
    return dff;
}

QDistanceFieldGlyphCache::QDistanceFieldGlyphCache()
    : m_rootNode(nullptr)
{
}

QDistanceFieldGlyphCache::~QDistanceFieldGlyphCache()
{
}

void QDistanceFieldGlyphCache::setRootNode(QNode *rootNode)
{
    m_rootNode = rootNode;
}

QNode *QDistanceFieldGlyphCache::rootNode() const
{
    return m_rootNode;
}

bool QDistanceFieldGlyphCache::doubleGlyphResolution(const QRawFont &font)
{
    return getOrCreateDistanceFieldFont(font)->doubleGlyphResolution();
}

namespace {
QDistanceFieldGlyphCache::Glyph refAndGetGlyph(DistanceFieldFont *dff, quint32 glyph)
{
    QDistanceFieldGlyphCache::Glyph ret;

    if (dff) {
        const auto entry = dff->refGlyph(glyph);

        Q_ASSERT(entry.atlas());
        ret.glyphPathBoundingRect = entry.glyphPathBoundingRect();
        ret.texCoords = entry.texCoords();
        ret.texture = entry.atlas();
    }

    return ret;
}
} // anonymous

QList<QDistanceFieldGlyphCache::Glyph> QDistanceFieldGlyphCache::refGlyphs(const QGlyphRun &run)
{
    DistanceFieldFont *dff = getOrCreateDistanceFieldFont(run.rawFont());
    QList<QDistanceFieldGlyphCache::Glyph> ret;

    const auto glyphs = run.glyphIndexes();
    for (quint32 glyph : glyphs)
        ret << refAndGetGlyph(dff, glyph);

    return ret;
}

QDistanceFieldGlyphCache::Glyph QDistanceFieldGlyphCache::refGlyph(const QRawFont &font, quint32 glyph)
{
    return refAndGetGlyph(getOrCreateDistanceFieldFont(font), glyph);
}

void QDistanceFieldGlyphCache::derefGlyphs(const QGlyphRun &run)
{
    DistanceFieldFont *dff = getOrCreateDistanceFieldFont(run.rawFont());

    const auto glyphs = run.glyphIndexes();
    for (quint32 glyph : glyphs)
        dff->derefGlyph(glyph);
}

void QDistanceFieldGlyphCache::derefGlyph(const QRawFont &font, quint32 glyph)
{
    getOrCreateDistanceFieldFont(font)->derefGlyph(glyph);
}

} // namespace Qt3DExtras

QT_END_NAMESPACE
