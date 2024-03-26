// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickimageprovider.h"

#include "qquickimageprovider_p.h"
#include "qquickpixmapcache_p.h"
#include <QtQuick/private/qsgcontext_p.h>
#include <private/qqmlglobal_p.h>
#include <QtGui/qcolorspace.h>

QT_BEGIN_NAMESPACE

/*!
    \class QQuickTextureFactory
    \since 5.0
    \brief The QQuickTextureFactory class provides an interface for loading custom textures from QML.
    \inmodule QtQuick

    The purpose of the texture factory is to provide a placeholder for a image
    data that can be converted into an OpenGL texture.

    Creating a texture directly is not possible as there is rarely an OpenGL context
    available in the thread that is responsible for loading the image data.
*/

/*!
    Constructs a texture factory. Since QQuickTextureFactory is abstract, it
    cannot be instantiated directly.
*/

QQuickTextureFactory::QQuickTextureFactory()
{
}

/*!
    Destroys the texture factory.
*/

QQuickTextureFactory::~QQuickTextureFactory()
{
}

/*!
    \fn int QQuickTextureFactory::textureByteCount() const

    Returns the number of bytes of memory the texture consumes.
*/

/*!
    \fn QImage QQuickTextureFactory::image() const

    Returns an image version of this texture.

    The lifespan of the returned image is unknown, so the implementation should
    return a self contained QImage, not make use of the QImage(uchar *, ...)
    constructor.

    This function is not commonly used and is expected to be slow.
 */

QImage QQuickTextureFactory::image() const
{
    return QImage();
}

/*!
    Returns a QQuickTextureFactory holding the given \a image.

    This is typically used as a helper in QQuickImageResponse::textureFactory.

    \since 5.6
 */

QQuickTextureFactory *QQuickTextureFactory::textureFactoryForImage(const QImage &image)
{
    if (image.isNull())
        return nullptr;
    QQuickTextureFactory *texture = QSGContext::createTextureFactoryFromImage(image);
    if (texture)
        return texture;
    return new QQuickDefaultTextureFactory(image);
}



/*!
    \fn QSGTexture *QQuickTextureFactory::createTexture(QQuickWindow *window) const

    This function is called on the scene graph rendering thread to create a QSGTexture
    instance from the factory. \a window provides the context which this texture is
    created in.

    QML will internally cache the returned texture as needed. Each call to this
    function should return a unique instance.

    The OpenGL context used for rendering is bound when this function is called.
 */

/*!
    \fn QSize QQuickTextureFactory::textureSize() const

    Returns the size of the texture. This function will be called from arbitrary threads
    and should not rely on an OpenGL context bound.
 */


/*!
    \class QQuickImageResponse
    \since 5.6
    \brief The QQuickImageResponse class provides an interface for asynchronous image loading in QQuickAsyncImageProvider.
    \inmodule QtQuick

    The purpose of an image response is to provide a way for image provider jobs to be executed
    in an asynchronous way.

    Responses are deleted via \l deleteLater once the finished() signal has been emitted.
    If you are using QRunnable as base for your QQuickImageResponse
    ensure automatic deletion is disabled.

    See the \l {imageresponseprovider}{Image Response Provider Example} for a complete implementation.

    \sa QQuickImageProvider
*/

/*!
    Constructs the image response
*/
QQuickImageResponse::QQuickImageResponse()
    : QObject(*(new QQuickImageResponsePrivate))
{
    qmlobject_connect(this, QQuickImageResponse, SIGNAL(finished()),
                      this, QQuickImageResponse, SLOT(_q_finished()));
}

/*!
    Destructs the image response
*/
QQuickImageResponse::~QQuickImageResponse()
{
}

/*!
    Returns the error string for the job execution. An empty string means no error.
*/
QString QQuickImageResponse::errorString() const
{
    return QString();
}

/*!
    This method is used to communicate that the response is no longer required by the engine.

    It may be reimplemented to cancel a request in the provider side, however, it is not mandatory.

    A cancelled QQuickImageResponse still needs to emit finished() so that the
    engine may clean up the QQuickImageResponse.

    \note finished() should not be emitted until the response is complete,
    regardless of whether or not cancel() was called. If it is called prematurely,
    the engine may destroy the response while it is still active, leading to a crash.
*/
void QQuickImageResponse::cancel()
{
}

/*!
    \fn void QQuickImageResponse::finished()

    Signals that the job execution has finished (be it successfully, because an
    error happened or because it was cancelled).

    \note Emission of this signal must be the final action the response performs:
    once the signal is received, the response will subsequently be destroyed by
    the engine.
 */

/*!
    \fn QQuickTextureFactory *QQuickImageResponse::textureFactory() const

    Returns the texture factory for the job. You can use QQuickTextureFactory::textureFactoryForImage
    if your provider works with QImage. The engine takes ownership of the returned QQuickTextureFactory.

     \note This method will be called only when needed. For example, it may not be called if there is an
     error or the job is cancelled. Therefore, allocate the QQuickTextureFactory instance only in this
     method or otherwise ensure its deletion.
 */


/*!
    \class QQuickImageProvider
    \since 5.0
    \inmodule QtQuick
    \brief The QQuickImageProvider class provides an interface for supporting pixmaps and threaded image requests in QML.

    QQuickImageProvider is used to provide advanced image loading features
    in QML applications. It allows images in QML to be:

    \list
    \li Loaded using QPixmaps rather than actual image files
    \li Loaded asynchronously in a separate thread
    \endlist

    To specify that an image should be loaded by an image provider, use the
    \b {"image:"} scheme for the URL source of the image, followed by the
    identifiers of the image provider and the requested image. For example:

    \qml
    Image { source: "image://myimageprovider/image.png" }
    \endqml

    This specifies that the image should be loaded by the image provider named
    "myimageprovider", and the image to be loaded is named "image.png". The QML engine
    invokes the appropriate image provider according to the providers that have
    been registered through QQmlEngine::addImageProvider().

    Note that the identifiers are case-insensitive, but the rest of the URL will be passed on with
    preserved case. For example, the below snippet would still specify that the image is loaded by the
    image provider named "myimageprovider", but it would request a different image than the above snippet
    ("Image.png" instead of "image.png").
    \qml
    Image { source: "image://MyImageProvider/Image.png" }
    \endqml

    If you want the rest of the URL to be case insensitive, you will have to take care
    of that yourself inside your image provider.

    \section2 An Example

    Here are two images. Their \c source values indicate they should be loaded by
    an image provider named "colors", and the images to be loaded are "yellow"
    and "red", respectively:

    \snippet imgprovider/imageprovider-example.qml 0

    When these images are loaded by QML, it looks for a matching image provider
    and calls its requestImage() or requestPixmap() method (depending on its
    imageType()) to load the image. The method is called with the \c id
    parameter set to "yellow" for the first image, and "red" for the second.

    Here is an image provider implementation that can load the images
    requested by the above QML. This implementation dynamically
    generates QPixmap images that are filled with the requested color:

    \snippet imgprovider/imageprovider.cpp 0

    To make this provider accessible to QML, it is registered with the QML engine
    with a "colors" identifier:

    \snippet imgprovider/imageprovider.cpp 1
    \codeline
    \snippet imgprovider/imageprovider.cpp 2

    Now the images can be successfully loaded in QML:

    \image imageprovider.png

    See the \l {imageprovider}{Image Provider Example} for the complete implementation.
    Note that the example registers the provider via a \l{QQmlEngineExtensionPlugin}{plugin}
    instead of registering it in the application \c main() function as shown above.


    \section2 Asynchronous Image Loading

    Image providers that support QImage or Texture loading automatically include support
    for asychronous loading of images. To enable asynchronous loading for an
    image source, set the \c asynchronous property to \c true for the relevant
    \l Image or \l BorderImage object. When this is enabled,
    the image request to the provider is run in a low priority thread,
    allowing image loading to be executed in the background, and reducing the
    performance impact on the user interface.

    To force asynchronous image loading, even for image sources that do not
    have the \c asynchronous property set to \c true, you may pass the
    \c QQmlImageProviderBase::ForceAsynchronousImageLoading flag to the image
    provider constructor. This ensures that all image requests for the
    provider are handled in a separate thread.

    Asynchronous loading for image providers that provide QPixmap is only supported
    in platforms that have the ThreadedPixmaps feature, in platforms where
    pixmaps can only be created in the main thread (i.e. ThreadedPixmaps is not supported)
    if \l {Image::}{asynchronous} is set to \c true, the value is ignored
    and the image is loaded synchronously.

    Asynchronous image loading for providers of type other than ImageResponse are
    executed on a single thread per engine basis. That means that a slow image provider
    will block the loading of any other request. To avoid that we suggest using QQuickAsyncImageProvider
    and implement threading on the provider side via a \c QThreadPool or similar.
    See the \l {imageresponseprovider}{Image Response Provider Example} for a complete implementation.


    \section2 Image Caching

    Images returned by a QQuickImageProvider are automatically cached,
    similar to any image loaded by the QML engine. When an image with a
    "image://" prefix is loaded from cache, requestImage() and requestPixmap()
    will not be called for the relevant image provider. If an image should
    always be fetched from the image provider, and should not be cached at
    all, set the \c cache property to \c false for the relevant \l Image
    or \l BorderImage object.

    \sa QQmlEngine::addImageProvider()
*/

/*!
    Creates an image provider that will provide images of the given \a type and
    behave according to the given \a flags.
*/
QQuickImageProvider::QQuickImageProvider(ImageType type, Flags flags)
    : d(new QQuickImageProviderPrivate)
{
    d->type = type;
    d->flags = flags;
    d->isProviderWithOptions = false;
}

/*!
    Destroys the QQuickImageProvider

    \note The destructor of your derived class need to be thread safe.
*/
QQuickImageProvider::~QQuickImageProvider()
{
    delete d;
}

/*!
    Returns the image type supported by this provider.
*/
QQuickImageProvider::ImageType QQuickImageProvider::imageType() const
{
    return d->type;
}

/*!
    Returns the flags set for this provider.
*/
QQuickImageProvider::Flags QQuickImageProvider::flags() const
{
    return d->flags;
}

/*!
    Implement this method to return the image with \a id. The default
    implementation returns an empty image.

    The \a id is the requested image source, with the "image:" scheme and
    provider identifier removed. For example, if the image \l{Image::}{source}
    was "image://myprovider/icons/home", the given \a id would be "icons/home".

    The \a requestedSize corresponds to the \l {Image::sourceSize} requested by
    an Image item. If \a requestedSize is a valid size, the image
    returned should be of that size.

    In all cases, \a size must be set to the original size of the image. This
    is used to set the \l {Item::}{width} and \l {Item::}{height} of the
    relevant \l Image if these values have not been set explicitly.

    \note this method may be called by multiple threads, so ensure the
    implementation of this method is reentrant.
*/
QImage QQuickImageProvider::requestImage(const QString &id, QSize *size, const QSize& requestedSize)
{
    Q_UNUSED(id);
    Q_UNUSED(size);
    Q_UNUSED(requestedSize);
    if (d->type == Image)
        qWarning("ImageProvider supports Image type but has not implemented requestImage()");
    return QImage();
}

/*!
    Implement this method to return the pixmap with \a id. The default
    implementation returns an empty pixmap.

    The \a id is the requested image source, with the "image:" scheme and
    provider identifier removed. For example, if the image \l{Image::}{source}
    was "image://myprovider/icons/home", the given \a id would be "icons/home".

    The \a requestedSize corresponds to the \l {Image::sourceSize} requested by
    an Image item. If \a requestedSize is a valid size, the image
    returned should be of that size.

    In all cases, \a size must be set to the original size of the image. This
    is used to set the \l {Item::}{width} and \l {Item::}{height} of the
    relevant \l Image if these values have not been set explicitly.

    \note this method may be called by multiple threads, so ensure the
    implementation of this method is reentrant.
*/
QPixmap QQuickImageProvider::requestPixmap(const QString &id, QSize *size, const QSize& requestedSize)
{
    Q_UNUSED(id);
    Q_UNUSED(size);
    Q_UNUSED(requestedSize);
    if (d->type == Pixmap)
        qWarning("ImageProvider supports Pixmap type but has not implemented requestPixmap()");
    return QPixmap();
}


/*!
    Implement this method to return the texture with \a id. The default
    implementation returns \nullptr.

    The \a id is the requested image source, with the "image:" scheme and
    provider identifier removed. For example, if the image \l{Image::}{source}
    was "image://myprovider/icons/home", the given \a id would be "icons/home".

    The \a requestedSize corresponds to the \l {Image::sourceSize} requested by
    an Image item. If \a requestedSize is a valid size, the image
    returned should be of that size.

    In all cases, \a size must be set to the original size of the image. This
    is used to set the \l {Item::}{width} and \l {Item::}{height} of the
    relevant \l Image if these values have not been set explicitly.

    \note this method may be called by multiple threads, so ensure the
    implementation of this method is reentrant.
*/

QQuickTextureFactory *QQuickImageProvider::requestTexture(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(id);
    Q_UNUSED(size);
    Q_UNUSED(requestedSize);
    if (d->type == Texture)
        qWarning("ImageProvider supports Texture type but has not implemented requestTexture()");
    return nullptr;
}

/*!
    \class QQuickAsyncImageProvider
    \since 5.6
    \inmodule QtQuick
    \brief The QQuickAsyncImageProvider class provides an interface for asynchronous control of QML image requests.

    See the \l {imageresponseprovider}{Image Response Provider Example} for a complete implementation.

    \sa QQuickImageProvider
*/
QQuickAsyncImageProvider::QQuickAsyncImageProvider()
 : QQuickImageProvider(ImageResponse, ForceAsynchronousImageLoading)
 , d(nullptr) // just as a placeholder in case we need it for the future
{
    Q_UNUSED(d);
}

QQuickAsyncImageProvider::~QQuickAsyncImageProvider()
{
}

/*!
   \fn QQuickImageResponse *QQuickAsyncImageProvider::requestImageResponse(const QString &id, const QSize &requestedSize)

    Implement this method to return the job that will provide the texture with \a id.

    The \a id is the requested image source, with the "image:" scheme and
    provider identifier removed. For example, if the image \l{Image::}{source}
    was "image://myprovider/icons/home", the given \a id would be "icons/home".

    The \a requestedSize corresponds to the \l {Image::sourceSize} requested by
    an Image item. If \a requestedSize is a valid size, the image
    returned should be of that size.

    \note this method may be called by multiple threads, so ensure the
    implementation of this method is reentrant.
*/


class QQuickImageProviderOptionsPrivate : public QSharedData
{
public:
    QQuickImageProviderOptionsPrivate()
    {
    }

    QColorSpace targetColorSpace;
    QQuickImageProviderOptions::AutoTransform autoTransform = QQuickImageProviderOptions::UsePluginDefaultTransform;
    bool preserveAspectRatioCrop = false;
    bool preserveAspectRatioFit = false;
};

/*!
    \class QQuickImageProviderOptions
    \brief The QQuickImageProviderOptions class provides options for QQuickImageProviderWithOptions image requests.
    \inmodule QtQuick
    \internal

    \sa QQuickImageProviderWithOptions
*/

/*!
    \enum QQuickImageProviderOptions::AutoTransform

    Whether the image provider should apply transformation metadata on read().

    \value UsePluginDefaultTransform Image provider should do its default behavior on whether applying transformation metadata on read or not
    \value ApplyTransform Image provider should apply transformation metadata on read
    \value DoNotApplyTransform Image provider should not apply transformation metadata on read
*/

QQuickImageProviderOptions::QQuickImageProviderOptions()
 : d(new QQuickImageProviderOptionsPrivate())
{
}

QQuickImageProviderOptions::~QQuickImageProviderOptions()
{
}

QQuickImageProviderOptions::QQuickImageProviderOptions(const QQuickImageProviderOptions &other)
 : d(other.d)
{
}

QQuickImageProviderOptions& QQuickImageProviderOptions::operator=(const QQuickImageProviderOptions &other)
{
    d = other.d;
    return *this;
}

bool QQuickImageProviderOptions::operator==(const QQuickImageProviderOptions &other) const
{
    return d->autoTransform == other.d->autoTransform &&
           d->preserveAspectRatioCrop == other.d->preserveAspectRatioCrop &&
           d->preserveAspectRatioFit == other.d->preserveAspectRatioFit &&
           d->targetColorSpace == other.d->targetColorSpace;
}

/*!
    Returns whether the image provider should apply transformation metadata on read().
*/
QQuickImageProviderOptions::AutoTransform QQuickImageProviderOptions::autoTransform() const
{
    return d->autoTransform;
}

void QQuickImageProviderOptions::setAutoTransform(QQuickImageProviderOptions::AutoTransform autoTransform)
{
    d->autoTransform = autoTransform;
}

/*!
    Returns whether the image request is for a PreserveAspectCrop Image.
    This allows the provider to better optimize the size of the returned image.
*/
bool QQuickImageProviderOptions::preserveAspectRatioCrop() const
{
    return d->preserveAspectRatioCrop;
}

void QQuickImageProviderOptions::setPreserveAspectRatioCrop(bool preserveAspectRatioCrop)
{
    d->preserveAspectRatioCrop = preserveAspectRatioCrop;
}

/*!
    Returns whether the image request is for a PreserveAspectFit Image.
    This allows the provider to better optimize the size of the returned image.
*/
bool QQuickImageProviderOptions::preserveAspectRatioFit() const
{
    return d->preserveAspectRatioFit;
}

void QQuickImageProviderOptions::setPreserveAspectRatioFit(bool preserveAspectRatioFit)
{
    d->preserveAspectRatioFit = preserveAspectRatioFit;
}

/*!
    Returns the color space the image provider should return the image in.
*/
QColorSpace QQuickImageProviderOptions::targetColorSpace() const
{
    return d->targetColorSpace;
}

void QQuickImageProviderOptions::setTargetColorSpace(const QColorSpace &colorSpace)
{
    d->targetColorSpace = colorSpace;
}

QQuickImageProviderWithOptions::QQuickImageProviderWithOptions(ImageType type, Flags flags)
 : QQuickAsyncImageProvider()
{
    QQuickImageProvider::d->type = type;
    QQuickImageProvider::d->flags = flags;
    QQuickImageProvider::d->isProviderWithOptions = true;
}

QImage QQuickImageProviderWithOptions::requestImage(const QString &id, QSize *size, const QSize& requestedSize)
{
    return requestImage(id, size, requestedSize, QQuickImageProviderOptions());
}

QPixmap QQuickImageProviderWithOptions::requestPixmap(const QString &id, QSize *size, const QSize& requestedSize)
{
    return requestPixmap(id, size, requestedSize, QQuickImageProviderOptions());
}

QQuickTextureFactory *QQuickImageProviderWithOptions::requestTexture(const QString &id, QSize *size, const QSize &requestedSize)
{
    return requestTexture(id, size, requestedSize, QQuickImageProviderOptions());
}

QImage QQuickImageProviderWithOptions::requestImage(const QString &id, QSize *size, const QSize& requestedSize, const QQuickImageProviderOptions &options)
{
    Q_UNUSED(options);
    return QQuickAsyncImageProvider::requestImage(id, size, requestedSize);
}

QPixmap QQuickImageProviderWithOptions::requestPixmap(const QString &id, QSize *size, const QSize& requestedSize, const QQuickImageProviderOptions &options)
{
    Q_UNUSED(options);
    return QQuickAsyncImageProvider::requestPixmap(id, size, requestedSize);
}

QQuickTextureFactory *QQuickImageProviderWithOptions::requestTexture(const QString &id, QSize *size, const QSize &requestedSize, const QQuickImageProviderOptions &options)
{
    Q_UNUSED(options);
    return QQuickAsyncImageProvider::requestTexture(id, size, requestedSize);
}

QQuickImageResponse *QQuickImageProviderWithOptions::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    Q_UNUSED(id);
    Q_UNUSED(requestedSize);
    if (imageType() == ImageResponse)
        qWarning("ImageProvider is of ImageResponse type but has not implemented requestImageResponse()");
    return nullptr;
}

QQuickImageResponse *QQuickImageProviderWithOptions::requestImageResponse(const QString &id, const QSize &requestedSize, const QQuickImageProviderOptions &options)
{
    Q_UNUSED(options);
    return requestImageResponse(id, requestedSize);
}

/*!
    Returns the recommended scaled image size for loading and storage. This is
    calculated according to the native pixel size of the image \a originalSize,
    the requested sourceSize \a requestedSize, the image file format \a format,
    and \a options. If the calculation otherwise concludes that scaled loading
    is not recommended, an invalid size is returned.
*/
QSize QQuickImageProviderWithOptions::loadSize(const QSize &originalSize, const QSize &requestedSize, const QByteArray &format, const QQuickImageProviderOptions &options,
                                               qreal devicePixelRatio)
{
    QSize res;
    const bool formatIsScalable = (format == "svg" || format == "svgz" || format == "pdf");
    const bool noRequestedSize = requestedSize.width() <= 0 && requestedSize.height() <= 0;
    if ((noRequestedSize && !formatIsScalable) || originalSize.isEmpty())
        return res;

    // If no sourceSize was set and we're loading an SVG, ensure that we provide
    // a default size that accounts for DPR so that the image isn't blurry.
    if (noRequestedSize && formatIsScalable)
        return originalSize * devicePixelRatio;

    const bool preserveAspectCropOrFit = options.preserveAspectRatioCrop() || options.preserveAspectRatioFit();

    if (!preserveAspectCropOrFit && formatIsScalable && !requestedSize.isEmpty())
        return requestedSize;

    qreal ratio = 0.0;
    if (requestedSize.width() && (preserveAspectCropOrFit || formatIsScalable ||
                                  requestedSize.width() < originalSize.width())) {
        ratio = qreal(requestedSize.width()) / originalSize.width();
    }
    if (requestedSize.height() && (preserveAspectCropOrFit || formatIsScalable ||
                                   requestedSize.height() < originalSize.height())) {
        qreal hr = qreal(requestedSize.height()) / originalSize.height();
        if (ratio == 0.0)
            ratio = hr;
        else if (!preserveAspectCropOrFit && (hr < ratio))
            ratio = hr;
        else if (preserveAspectCropOrFit && (hr > ratio))
            ratio = hr;
    }
    if (ratio > 0.0) {
        res.setHeight(qRound(originalSize.height() * ratio));
        res.setWidth(qRound(originalSize.width() * ratio));
    }
    return res;
}

QQuickImageProviderWithOptions *QQuickImageProviderWithOptions::checkedCast(QQuickImageProvider *provider)
{
    if (provider && provider->d && provider->d->isProviderWithOptions)
        return static_cast<QQuickImageProviderWithOptions *>(provider);

    return nullptr;
}

QT_END_NAMESPACE

#include "moc_qquickimageprovider.cpp"
