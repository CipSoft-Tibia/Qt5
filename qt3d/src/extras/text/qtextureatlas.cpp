// Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtextureatlas_p.h"
#include "qtextureatlas_p_p.h"
#include <Qt3DRender/qtexturedata.h>
#include <Qt3DRender/qabstracttextureimage.h>

QT_BEGIN_NAMESPACE


namespace Qt3DExtras {

using namespace Qt3DCore;

QTextureAtlasData::QTextureAtlasData(int w, int h, QImage::Format fmt)
    : m_image(w, h, fmt)
{
    m_image.fill(0);
}

QTextureAtlasData::~QTextureAtlasData()
{
}

void QTextureAtlasData::addImage(const AtlasTexture &texture, const QImage &image)
{
    QMutexLocker lock(&m_mutex);

    Update update;
    update.textureInfo = texture;
    update.image = image;
    m_updates << update;
}

QByteArray QTextureAtlasData::createUpdatedImageData()
{
    m_mutex.lock();
    const QList<Update> updates = std::move(m_updates);
    m_mutex.unlock();

    // copy sub-images into the actual texture image
    for (const Update &update : updates) {
        const QImage &image = update.image;

        const int padding = update.textureInfo.padding;
        const QRect imgRect = update.textureInfo.position;
        const QRect alloc = imgRect.adjusted(-padding, -padding, padding, padding);

        // bytes per pixel
        if (image.depth() != m_image.depth()) {
            qWarning() << "[QTextureAtlas] Image depth does not match. Original =" << m_image.depth() << ", Sub-Image =" << image.depth();
            continue;
        }
        int bpp = image.depth() / 8;

        // copy image contents into texture image
        // use image border pixels to fill the padding region
        for (int y = alloc.top(); y <= alloc.bottom(); y++) {
            uchar *dstLine = m_image.scanLine(y);

            uchar *dstPadL = &dstLine[bpp * alloc.left()];
            uchar *dstPadR = &dstLine[bpp * imgRect.right()];
            uchar *dstImg  = &dstLine[bpp * imgRect.left()];

            // do padding with 0 in the upper/lower padding parts around the actual image
            if (y < imgRect.top() || y > imgRect.bottom()) {
                memset(dstPadL, 0, bpp * (imgRect.width() + 2 * padding));
                continue;
            }

            // copy left and right padding pixels
            memset(dstPadL, 0, bpp * padding);
            memset(dstPadR, 0, bpp * padding);

            // copy image scanline
            const int ySrc = qBound(0, y - imgRect.top(), image.height()-1);
            memcpy(dstImg, image.scanLine(ySrc), bpp * imgRect.width());
        }
    }

    return QByteArray(reinterpret_cast<const char*>(m_image.constBits()), m_image.sizeInBytes());
}

QTextureAtlasPrivate::QTextureAtlasPrivate()
    : Qt3DRender::QAbstractTexturePrivate()
{
    m_target = Qt3DRender::QAbstractTexture::TargetAutomatic;
    m_format = Qt3DRender::QAbstractTexture::RGBA8_UNorm;
    m_width = 256;
    m_height = 256;
    m_depth = 1;
}

QTextureAtlasPrivate::~QTextureAtlasPrivate()
{
}

QTextureAtlasGenerator::QTextureAtlasGenerator(const QTextureAtlasPrivate *texAtlas)
    : m_data(texAtlas->m_data)
    , m_format(texAtlas->m_format)
    , m_pixelFormat(texAtlas->m_pixelFormat)
    , m_generation(texAtlas->m_currGen)
    , m_atlasId(texAtlas->m_id)
{
}

QTextureAtlasGenerator::~QTextureAtlasGenerator()
{
}

Qt3DRender::QTextureDataPtr QTextureAtlasGenerator::operator()()
{
    Qt3DRender::QTextureImageDataPtr texImage = Qt3DRender::QTextureImageDataPtr::create();
    texImage->setTarget(QOpenGLTexture::Target2D);
    texImage->setWidth(m_data->width());
    texImage->setHeight(m_data->height());
    texImage->setDepth(1);
    texImage->setFaces(1);
    texImage->setLayers(1);
    texImage->setMipLevels(1);
    texImage->setFormat(static_cast<QOpenGLTexture::TextureFormat>(m_format));
    texImage->setPixelFormat(m_pixelFormat);
    texImage->setPixelType(QOpenGLTexture::UInt8);

    const QByteArray bytes = m_data->createUpdatedImageData();
    texImage->setData(bytes, 1);

    Qt3DRender::QTextureDataPtr generatedData = Qt3DRender::QTextureDataPtr::create();
    generatedData->setTarget(Qt3DRender::QAbstractTexture::Target2D);
    generatedData->setFormat(m_format);
    generatedData->setWidth(m_data->width());
    generatedData->setHeight(m_data->height());
    generatedData->setDepth(1);
    generatedData->setLayers(1);
    generatedData->addImageData(texImage);

    return generatedData;
}

bool QTextureAtlasGenerator::operator==(const QTextureGenerator &other) const
{
    const QTextureAtlasGenerator *otherFunctor = Qt3DCore::functor_cast<QTextureAtlasGenerator>(&other);
    return (otherFunctor != nullptr
            && otherFunctor->m_data == m_data
            && otherFunctor->m_atlasId == m_atlasId
            && otherFunctor->m_generation == m_generation);
}

QTextureAtlas::QTextureAtlas(Qt3DCore::QNode *parent)
    : QAbstractTexture(*new QTextureAtlasPrivate(), parent)
{
}

QOpenGLTexture::PixelFormat QTextureAtlas::pixelFormat() const
{
    Q_D(const QTextureAtlas);
    return d->m_pixelFormat;
}

void QTextureAtlas::setPixelFormat(QOpenGLTexture::PixelFormat fmt)
{
    Q_D(QTextureAtlas);
    d->m_pixelFormat = fmt;
}

QTextureAtlas::~QTextureAtlas()
{
}

QTextureAtlas::TextureId QTextureAtlas::addImage(const QImage &image, int padding)
{
    Q_D(QTextureAtlas);

    // lazily create image and allocator to allow setWidth/setHeight after object construction
    if (!d->m_allocator) {
        Q_ASSERT(d->m_data.isNull());

        d->m_allocator.reset(new AreaAllocator(QSize(width(), height())));
        d->m_data = QTextureAtlasDataPtr::create(width(), height(), image.format());
    }

    const QSize allocSz = image.size() + QSize(2 * padding, 2 * padding);

    // try to allocate space within image space
    const QRect alloc = d->m_allocator->allocate(allocSz);
    if (alloc.isEmpty())
        return InvalidTexture;

    const QRect imgRect = alloc.adjusted(padding, padding, -padding, -padding);
    AtlasTexture tex;
    tex.position = imgRect;
    tex.padding = padding;

    // store texture
    TextureId id = d->m_currId++;
    d->m_textures[id] = tex;
    d->m_data->addImage(tex, image);

    // update data functor
    d->m_currGen++;
    d->setDataFunctor(QTextureAtlasGeneratorPtr::create(d));

    return id;
}

void QTextureAtlas::removeImage(TextureId id)
{
    Q_D(QTextureAtlas);
    auto it = d->m_textures.find(id);
    if (it != d->m_textures.end()) {
        QRect imgRect = it->position;
        imgRect.adjust(-it->padding, -it->padding, 2*it->padding, 2*it->padding);

        if (d->m_allocator)
            d->m_allocator->deallocate(imgRect);
        d->m_textures.erase(it);
    }
}

bool QTextureAtlas::hasImage(TextureId id) const
{
    Q_D(const QTextureAtlas);
    return d->m_textures.contains(id);
}

qsizetype QTextureAtlas::imageCount() const
{
    Q_D(const QTextureAtlas);
    return d->m_textures.size();
}

QRect QTextureAtlas::imagePosition(TextureId id) const
{
    Q_D(const QTextureAtlas);
    const auto it = d->m_textures.find(id);
    return (it != d->m_textures.cend()) ? it->position : QRect();
}

QRectF QTextureAtlas::imageTexCoords(TextureId id) const
{
    Q_D(const QTextureAtlas);
    const auto it = d->m_textures.find(id);
    if (it != d->m_textures.cend()) {
        const float w = d->m_data->width();
        const float h = d->m_data->height();
        return QRectF(static_cast<float>(it->position.x()) / w,
                      static_cast<float>(it->position.y()) / h,
                      static_cast<float>(it->position.width()) / w,
                      static_cast<float>(it->position.height()) / h);
    }
    return QRectF();
}

int QTextureAtlas::imagePadding(TextureId id) const
{
    Q_D(const QTextureAtlas);
    const auto it = d->m_textures.find(id);
    return (it != d->m_textures.cend()) ? it->padding : -1;
}

} // namespace Qt3DExtras

QT_END_NAMESPACE

#include "moc_qtextureatlas_p.cpp"
