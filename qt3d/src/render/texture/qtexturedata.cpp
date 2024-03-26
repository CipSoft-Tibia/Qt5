// Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qtexturedata.h"

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

/*!
 * \class Qt3DRender::QTextureData
 * \inheaderfile Qt3DRender/QTextureData
 * \brief The QTextureData class stores texture information such as
 * the target, height, width, depth, layers, wrap, and if mipmaps are enabled.
 * \since 5.7
 * \inmodule Qt3DRender
 */

class QTextureDataPrivate
{
public:
    QAbstractTexture::Target m_target;
    QAbstractTexture::TextureFormat m_format = QAbstractTexture::NoFormat;
    int m_width = 0;
    int m_height = 0;
    int m_depth = 0;
    int m_layers = 0;
    bool m_autoMipMap = false;
    float m_maximumAnisotropy = 0.0f;
    QAbstractTexture::Filter m_minFilter;
    QAbstractTexture::Filter m_magFilter;
    QTextureWrapMode::WrapMode m_wrapModeX;
    QTextureWrapMode::WrapMode m_wrapModeY;
    QTextureWrapMode::WrapMode m_wrapModeZ;
    QAbstractTexture::ComparisonFunction m_comparisonFunction;
    QAbstractTexture::ComparisonMode m_comparisonMode;
    QList<QTextureImageDataPtr> m_imagesData;

};

/*!
 * Creates a new QTextureData
 * instance.
 */
QTextureData::QTextureData()
    : d_ptr(new QTextureDataPrivate())
{
}

/*!
 * \internal
 */
QTextureData::~QTextureData()
{
    delete d_ptr;
}

/*!
 * Returns the texture data target.
 */
QAbstractTexture::Target QTextureData::target() const
{
    Q_D(const QTextureData);
    return d->m_target;
}

/*!
 * Sets the target texture to \a target.
 */
void QTextureData::setTarget(QAbstractTexture::Target target)
{
    Q_D(QTextureData);
    d->m_target = target;
}

/*!
 * Returns the texture format
 */
QAbstractTexture::TextureFormat QTextureData::format() const
{
    Q_D(const QTextureData);
    return d->m_format;
}

/*!
 * Sets the texture format to \a format.
 */
void QTextureData::setFormat(QAbstractTexture::TextureFormat format)
{
    Q_D(QTextureData);
    d->m_format = format;
}

/*!
 * Returns the texture width.
 */
int QTextureData::width() const
{
    Q_D(const QTextureData);
    return d->m_width;
}

/*!
 * Sets the texture width to \a width.
 */
void QTextureData::setWidth(int width)
{
    Q_D(QTextureData);
    d->m_width = width;
}

/*!
 * Returns the texture height.
 */
int QTextureData::height() const
{
    Q_D(const QTextureData);
    return d->m_height;
}

/*!
 * Sets the target height to \a height.
 */
void QTextureData::setHeight(int height)
{
    Q_D(QTextureData);
    d->m_height = height;
}

/*!
 * Returns the texture depth.
 */
int QTextureData::depth() const
{
    Q_D(const QTextureData);
    return d->m_depth;
}

/*!
 * Sets the texture depth to \a depth
 */
void QTextureData::setDepth(int depth)
{
    Q_D(QTextureData);
    d->m_depth = depth;
}

/*!
 * Returns the texture layers.
 */
int QTextureData::layers() const
{
    Q_D(const QTextureData);
    return d->m_layers;
}

/*!
 * Sets the texture layers to \a layers.
 */
void QTextureData::setLayers(int layers)
{
    Q_D(QTextureData);
    d->m_layers = layers;
}

/*!
 * Returns whether the texture has auto mipmap generation enabled.
 */
bool QTextureData::isAutoMipMapGenerationEnabled() const
{
    Q_D(const QTextureData);
    return d->m_autoMipMap;
}

/*!
 * Sets whether the texture has automatic mipmap generation enabled, to \a autoMipMap.
 */
void QTextureData::setAutoMipMapGenerationEnabled(bool autoMipMap)
{
    Q_D(QTextureData);
    d->m_autoMipMap = autoMipMap;
}

/*!
 * Returns the current maximum anisotropy.
 */
float QTextureData::maximumAnisotropy() const
{
    Q_D(const QTextureData);
    return d->m_maximumAnisotropy;
}

/*!
 * Sets the maximum anisotropy to \a maximumAnisotropy.
 */
void QTextureData::setMaximumAnisotropy(float maximumAnisotropy)
{
    Q_D(QTextureData);
    d->m_maximumAnisotropy = maximumAnisotropy;
}

/*!
 * Returns the current minification filter.
 */
QAbstractTexture::Filter QTextureData::minificationFilter() const
{
    Q_D(const QTextureData);
    return d->m_minFilter;
}

/*!
 * Sets the minification filter to \a filter.
 */
void QTextureData::setMinificationFilter(QAbstractTexture::Filter filter)
{
    Q_D(QTextureData);
    d->m_minFilter = filter;
}

/*!
 * Returns the current magnification filter.
 */
QAbstractTexture::Filter QTextureData::magnificationFilter() const
{
    Q_D(const QTextureData);
    return d->m_magFilter;
}

/*!
 * Sets the magnification filter to \a filter.
 */
void QTextureData::setMagnificationFilter(QAbstractTexture::Filter filter)
{
    Q_D(QTextureData);
    d->m_magFilter = filter;
}

/*!
 * Returns the current wrap mode X.
 */
QTextureWrapMode::WrapMode QTextureData::wrapModeX() const
{
    Q_D(const QTextureData);
    return d->m_wrapModeX;
}

/*!
 * Sets the wrap mode X to \a wrapModeX.
 */
void QTextureData::setWrapModeX(QTextureWrapMode::WrapMode wrapModeX)
{
    Q_D(QTextureData);
    d->m_wrapModeX = wrapModeX;
}

/*!
 * Returns the current wrap mode Y.
 */
QTextureWrapMode::WrapMode QTextureData::wrapModeY() const
{
    Q_D(const QTextureData);
    return d->m_wrapModeY;
}

/*!
 * Sets the wrap mode Y to \a wrapModeY.
 */
void QTextureData::setWrapModeY(QTextureWrapMode::WrapMode wrapModeY)
{
    Q_D(QTextureData);
    d->m_wrapModeY = wrapModeY;
}

/*!
 * Returns the current wrap mode Z.
 */
QTextureWrapMode::WrapMode QTextureData::wrapModeZ() const
{
    Q_D(const QTextureData);
    return d->m_wrapModeZ;
}

/*!
 * Sets the wrap mode Z to \a wrapModeZ.
 */
void QTextureData::setWrapModeZ(QTextureWrapMode::WrapMode wrapModeZ)
{
    Q_D(QTextureData);
    d->m_wrapModeZ = wrapModeZ;
}

/*!
 * Returns the current comparison function.
 */
QAbstractTexture::ComparisonFunction QTextureData::comparisonFunction() const
{
    Q_D(const QTextureData);
    return d->m_comparisonFunction;
}

/*!
 * Sets the comparison function to \a comparisonFunction.
 */
void QTextureData::setComparisonFunction(QAbstractTexture::ComparisonFunction comparisonFunction)
{
    Q_D(QTextureData);
    d->m_comparisonFunction = comparisonFunction;
}

/*!
 * Returns the current comparison mode.
 */
QAbstractTexture::ComparisonMode QTextureData::comparisonMode() const
{
    Q_D(const QTextureData);
    return d->m_comparisonMode;
}

/*!
 * Sets the comparison mode to \a comparisonMode.
 */
void QTextureData::setComparisonMode(QAbstractTexture::ComparisonMode comparisonMode)
{
    Q_D(QTextureData);
    d->m_comparisonMode = comparisonMode;
}

/*!
 * Returns the data of the images used by this texture.
 */
QList<QTextureImageDataPtr> QTextureData::imageData() const
{
    Q_D(const QTextureData);
    return d->m_imagesData;
}

/*!
 * Adds an extra image layer to the texture using \a imageData.
 *
 * \note The texture image should be loaded with the size specified on the texture.
 * However, if no size is specified, the size of the first texture image file is used as default.
 */
void QTextureData::addImageData(const QTextureImageDataPtr &imageData)
{
    Q_D(QTextureData);
    d->m_imagesData.push_back(imageData);
}

} // Qt3DRender

QT_END_NAMESPACE
