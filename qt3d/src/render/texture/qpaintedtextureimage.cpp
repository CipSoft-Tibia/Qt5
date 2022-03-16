/****************************************************************************
**
** Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
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
   Schedules the painted texture's paint() function to be called,
   which in turn uploads the new image to the GPU.
   Parameter \a rect is currently unused.
*/
void QPaintedTextureImage::update(const QRect &rect)
{
    Q_UNUSED(rect)
    Q_D(QPaintedTextureImage);

    d->repaint();
}

/*!
    \fn Qt3DRender::QPaintedTextureImage::paint(QPainter *painter)

    Paints the texture image with the specified QPainter object \a painter.
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

