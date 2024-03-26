// Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpaintedtextureimage.h"
#include "qpaintedtextureimage_p.h"

#include <QtGui/qpainter.h>
#include <QtGui/qimage.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

/*!
    \class Qt3DRender::QPaintedTextureImage
    \inmodule Qt3DRender
    \since 5.8
    \brief A QAbstractTextureImage that can be written through a QPainter.

    A QPaintedTextureImage provides a way to specify a texture image
    (and thus an OpenGL texture) through a QPainter. The width and height of the
    texture image can be specified through the width and height or size
    properties.

    A QPaintedTextureImage must be subclassed and the virtual paint() function
    implemented. Each time update() is called on the QPaintedTextureImage,
    the paint() function is invoked and the resulting image is uploaded.

    The QPaintedTextureImage must be attached to some QAbstractTexture.
 */



QPaintedTextureImagePrivate::QPaintedTextureImagePrivate()
    : m_imageSize(256,256)
    , m_devicePixelRatio(1.0)
    , m_generation(0)
{
}

QPaintedTextureImagePrivate::~QPaintedTextureImagePrivate()
{
}

void QPaintedTextureImagePrivate::repaint()
{
    // create or re-allocate QImage with current size
    if (m_image.isNull()
            || m_image->size() != m_imageSize
            || m_image->devicePixelRatio() != m_devicePixelRatio)
    {
        m_image.reset(new QImage(m_imageSize, QImage::Format_RGBA8888));
        m_image->setDevicePixelRatio(m_devicePixelRatio);
        m_image->fill(Qt::transparent);
    }

    QPainter painter(m_image.data());
    q_func()->paint(&painter);
    painter.end();

    ++m_generation;
    m_currentGenerator = QSharedPointer<QPaintedTextureImageDataGenerator>::create(*m_image.data(), m_generation, q_func()->id());
    q_func()->notifyDataGeneratorChanged();
}

QPaintedTextureImage::QPaintedTextureImage(Qt3DCore::QNode *parent)
    : QAbstractTextureImage(*new QPaintedTextureImagePrivate, parent)
{
    Q_D(QPaintedTextureImage);

    d->m_currentGenerator = QSharedPointer<QPaintedTextureImageDataGenerator>::create(QImage{}, 0, id());
}

QPaintedTextureImage::~QPaintedTextureImage()
{
}

/*!
    \property QPaintedTextureImage::width

    This property holds the width of the texture image.
    The width must be greater than or equal to 1.
*/
int QPaintedTextureImage::width() const
{
    Q_D(const QPaintedTextureImage);
    return d->m_imageSize.width();
}

/*!
    \property QPaintedTextureImage::height

    This property holds the height of the texture image.
    The height must be greater than or equal to 1.
*/
int QPaintedTextureImage::height() const
{
    Q_D(const QPaintedTextureImage);
    return d->m_imageSize.height();
}

/*!
    \property QPaintedTextureImage::size

    This property holds the size of the texture image.

    \sa height, width

*/
QSize QPaintedTextureImage::size() const
{
    Q_D(const QPaintedTextureImage);
    return d->m_imageSize;
}

/*!
    Sets the width (\a w) of the texture image. Triggers an update, if the size changes.
 */
void QPaintedTextureImage::setWidth(int w)
{
    if (w < 1) {
        qWarning() << "QPaintedTextureImage: Attempting to set invalid width" << w << ". Will be ignored";
        return;
    }
    setSize(QSize(w, height()));
}

/*!
    Sets the height (\a h) of the texture image. Triggers an update, if the size changes.
 */
void QPaintedTextureImage::setHeight(int h)
{
    if (h < 1) {
        qWarning() << "QPaintedTextureImage: Attempting to set invalid height" << h << ". Will be ignored";
        return;
    }
    setSize(QSize(width(), h));
}

/*!
    Sets the width and height of the texture image. Triggers an update, if the \a size changes.
 */
void QPaintedTextureImage::setSize(QSize size)
{
    Q_D(QPaintedTextureImage);

    if (d->m_imageSize != size) {
        if (size.isEmpty()) {
            qWarning() << "QPaintedTextureImage: Attempting to set invalid size" << size << ". Will be ignored";
            return;
        }

        const bool changeW = d->m_imageSize.width() != size.width();
        const bool changeH = d->m_imageSize.height() != size.height();

        d->m_imageSize = size;

        if (changeW)
            Q_EMIT widthChanged(d->m_imageSize.height());
        if (changeH)
            Q_EMIT heightChanged(d->m_imageSize.height());

        Q_EMIT sizeChanged(d->m_imageSize);

        d->repaint();
    }
}

/*!
   Immediately triggers the painted texture's paint() function,
   which in turn uploads the new image to the GPU. If you are
   making multiple changes to a painted texture, consider waiting
   until all changes are complete before calling update, in order
   to minimize the number of repaints required.

   Parameter \a rect is currently unused.
*/
void QPaintedTextureImage::update(const QRect &rect)
{
    Q_UNUSED(rect);
    Q_D(QPaintedTextureImage);

    d->repaint();
}

/*!
    \fn Qt3DRender::QPaintedTextureImage::paint(QPainter *painter)

    Paints the texture image with the specified QPainter object \a painter.

    QPainter considers the top-left corner of an image as its origin, while OpenGL considers
    the bottom-left corner of a texture as its origin. An easy way to account for this difference
    is to set a custom viewport on the painter before doing any other painting:

    \code
    painter->setViewport(0, height(), width(), -height());
    ...
    \endcode
*/
QTextureImageDataGeneratorPtr QPaintedTextureImage::dataGenerator() const
{
    Q_D(const QPaintedTextureImage);
    return d->m_currentGenerator;
}


QPaintedTextureImageDataGenerator::QPaintedTextureImageDataGenerator(const QImage &image, int gen, Qt3DCore::QNodeId texId)
    : m_image(image)    // pixels are implicitly shared, no copying
    , m_generation(gen)
    , m_paintedTextureImageId(texId)
{
}

QPaintedTextureImageDataGenerator::~QPaintedTextureImageDataGenerator()
{
}

QTextureImageDataPtr QPaintedTextureImageDataGenerator::operator ()()
{
    QTextureImageDataPtr textureData = QTextureImageDataPtr::create();
    textureData->setImage(m_image);
    return textureData;
}

bool QPaintedTextureImageDataGenerator::operator ==(const QTextureImageDataGenerator &other) const
{
    const QPaintedTextureImageDataGenerator *otherFunctor = functor_cast<QPaintedTextureImageDataGenerator>(&other);
    return (otherFunctor != nullptr && otherFunctor->m_generation == m_generation && otherFunctor->m_paintedTextureImageId == m_paintedTextureImageId);
}

} // namespace Qt3DRender

QT_END_NAMESPACE

#include "moc_qpaintedtextureimage.cpp"

