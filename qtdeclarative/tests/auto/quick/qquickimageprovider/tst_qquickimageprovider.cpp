/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qtest.h>
#include <QtTest/QtTest>
#include <QtQml/qqmlengine.h>
#include <QtQuick/qquickimageprovider.h>
#include <private/qquickimage_p.h>
#include <QImageReader>
#include <QWaitCondition>
#include <QThreadPool>
#include <private/qqmlengine_p.h>

Q_DECLARE_METATYPE(QQuickImageProvider*);

class tst_qquickimageprovider : public QObject
{
    Q_OBJECT
public:
    tst_qquickimageprovider()
    {
    }

private slots:
    void requestImage_sync_data();
    void requestImage_sync();
    void requestImage_async_data();
    void requestImage_async();
    void requestImage_async_forced_data();
    void requestImage_async_forced();

    void requestPixmap_sync_data();
    void requestPixmap_sync();
    void requestPixmap_async();

    void removeProvider_data();
    void removeProvider();

    void imageProviderId_data();
    void imageProviderId();

    void threadTest();

    void asyncTextureTest();
    void instantAsyncTextureTest();

    void asyncImageThreadSafety();

private:
    QString newImageFileName() const;
    void fillRequestTestsData(const QString &id);
    void runTest(bool async, QQuickImageProvider *provider);
};


class TestQImageProvider : public QQuickImageProvider
{
public:
    TestQImageProvider(bool *deleteWatch = nullptr, bool forceAsync = false)
        : QQuickImageProvider(Image, (forceAsync ? ForceAsynchronousImageLoading : Flags()))
        , deleteWatch(deleteWatch)
    {
    }

    ~TestQImageProvider()
    {
        if (deleteWatch)
            *deleteWatch = true;
    }

    QImage requestImage(const QString &id, QSize *size, const QSize& requestedSize)
    {
        lastImageId = id;

        if (id == QLatin1String("no-such-file.png"))
            return QImage();

        int width = 100;
        int height = 100;
        QImage image(width, height, QImage::Format_RGB32);
        if (size)
            *size = QSize(width, height);
        if (requestedSize.isValid())
            image = image.scaled(requestedSize);
        return image;
    }

    bool *deleteWatch;
    QString lastImageId;
};
Q_DECLARE_METATYPE(TestQImageProvider*);


class TestQPixmapProvider : public QQuickImageProvider
{
public:
    TestQPixmapProvider(bool *deleteWatch = nullptr)
        : QQuickImageProvider(Pixmap), deleteWatch(deleteWatch)
    {
    }

    ~TestQPixmapProvider()
    {
        if (deleteWatch)
            *deleteWatch = true;
    }

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize& requestedSize)
    {
        lastImageId = id;

        if (id == QLatin1String("no-such-file.png"))
            return QPixmap();

        int width = 100;
        int height = 100;
        QPixmap image(width, height);
        if (size)
            *size = QSize(width, height);
        if (requestedSize.isValid())
            image = image.scaled(requestedSize);
        return image;
    }

    bool *deleteWatch;
    QString lastImageId;
};
Q_DECLARE_METATYPE(TestQPixmapProvider*);


QString tst_qquickimageprovider::newImageFileName() const
{
    // need to generate new filenames each time or else images are loaded
    // from cache and we won't get loading status changes when testing
    // async loading
    static int count = 0;
    return QString("image://test/image-%1.png").arg(count++);
}

void tst_qquickimageprovider::fillRequestTestsData(const QString &id)
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("imageId");
    QTest::addColumn<QString>("properties");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QString>("error");

    QString fileName = newImageFileName();
    QTest::newRow(QTest::toString(id + " simple test"))
            << "image://test/" + fileName << fileName << "" << QSize(100,100) << "";

    fileName = newImageFileName();
    QTest::newRow(QTest::toString(id + " simple test with capitalization"))//As it's a URL, should make no difference
            << "image://Test/" + fileName << fileName << "" << QSize(100,100) << "";

    fileName = newImageFileName();
    QTest::newRow(QTest::toString(id + " url with no id"))
        << "image://test/" + fileName << "" + fileName << "" << QSize(100,100) << "";

    fileName = newImageFileName();
    QTest::newRow(QTest::toString(id + " url with path"))
        << "image://test/test/path" + fileName << "test/path" + fileName << "" << QSize(100,100) << "";

    fileName = newImageFileName();
    QTest::newRow(QTest::toString(id + " url with fragment"))
        << "image://test/faq.html?#question13" + fileName << "faq.html?#question13" + fileName << "" << QSize(100,100) << "";

    fileName = newImageFileName();
    QTest::newRow(QTest::toString(id + " url with query"))
        << "image://test/cgi-bin/drawgraph.cgi?type=pie&color=green" + fileName << "cgi-bin/drawgraph.cgi?type=pie&color=green" + fileName
        << "" << QSize(100,100) << "";

    fileName = newImageFileName();
    QTest::newRow(QTest::toString(id + " scaled image"))
            << "image://test/" + fileName << fileName << "sourceSize: \"80x30\"" << QSize(80,30) << "";

    QTest::newRow(QTest::toString(id + " missing"))
        << "image://test/no-such-file.png" << "no-such-file.png" << "" << QSize(100,100)
        << "<Unknown File>:2:1: QML Image: Failed to get image from provider: image://test/no-such-file.png";

    QTest::newRow(QTest::toString(id + " unknown provider"))
        << "image://bogus/exists.png" << "" << "" << QSize()
        << "<Unknown File>:2:1: QML Image: Invalid image provider: image://bogus/exists.png";
}

void tst_qquickimageprovider::runTest(bool async, QQuickImageProvider *provider)
{
    QFETCH(QString, source);
    QFETCH(QString, imageId);
    QFETCH(QString, properties);
    QFETCH(QSize, size);
    QFETCH(QString, error);

    if (!error.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, error.toUtf8());

    QQmlEngine engine;

    engine.addImageProvider("test", provider);
    QVERIFY(engine.imageProvider("test") != nullptr);

    QString componentStr = "import QtQuick 2.0\nImage { source: \"" + source + "\"; "
            + (async ? "asynchronous: true; " : "")
            + properties + " }";
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickImage *obj = qobject_cast<QQuickImage*>(component.create());
    QVERIFY(obj != nullptr);

    // From this point on, treat forced async providers as async behaviour-wise
    if (engine.imageProvider(QUrl(source).host()) == provider)
        async |= (provider->flags() & QQuickImageProvider::ForceAsynchronousImageLoading) != 0;

    if (async)
        QTRY_COMPARE(obj->status(), QQuickImage::Loading);

    QCOMPARE(obj->source(), QUrl(source));

    if (error.isEmpty()) {
        if (async)
            QTRY_COMPARE(obj->status(), QQuickImage::Ready);
        else
            QCOMPARE(obj->status(), QQuickImage::Ready);
        if (QByteArray(QTest::currentDataTag()).startsWith("qimage"))
            QCOMPARE(static_cast<TestQImageProvider*>(provider)->lastImageId, imageId);
        else
            QCOMPARE(static_cast<TestQPixmapProvider*>(provider)->lastImageId, imageId);

        QCOMPARE(obj->width(), qreal(size.width()));
        QCOMPARE(obj->height(), qreal(size.height()));
        QCOMPARE(obj->fillMode(), QQuickImage::Stretch);
        QCOMPARE(obj->progress(), 1.0);
    } else {
        if (async)
            QTRY_COMPARE(obj->status(), QQuickImage::Error);
        else
            QCOMPARE(obj->status(), QQuickImage::Error);
    }

    delete obj;
}

void tst_qquickimageprovider::requestImage_sync_data()
{
    fillRequestTestsData("qimage|sync");
}

void tst_qquickimageprovider::requestImage_sync()
{
    bool deleteWatch = false;
    runTest(false, new TestQImageProvider(&deleteWatch));
    QVERIFY(deleteWatch);
}

void tst_qquickimageprovider::requestImage_async_data()
{
    fillRequestTestsData("qimage|async");
}

void tst_qquickimageprovider::requestImage_async()
{
    bool deleteWatch = false;
    runTest(true, new TestQImageProvider(&deleteWatch));
    QVERIFY(deleteWatch);
}

void tst_qquickimageprovider::requestImage_async_forced_data()
{
    fillRequestTestsData("qimage|async_forced");
}

void tst_qquickimageprovider::requestImage_async_forced()
{
    bool deleteWatch = false;
    runTest(false, new TestQImageProvider(&deleteWatch, true));
    QVERIFY(deleteWatch);
}

void tst_qquickimageprovider::requestPixmap_sync_data()
{
    fillRequestTestsData("qpixmap");
}

void tst_qquickimageprovider::requestPixmap_sync()
{
    bool deleteWatch = false;
    runTest(false, new TestQPixmapProvider(&deleteWatch));
    QVERIFY(deleteWatch);
}

void tst_qquickimageprovider::requestPixmap_async()
{
    QQmlEngine engine;
    QQuickImageProvider *provider = new TestQPixmapProvider();

    engine.addImageProvider("test", provider);
    QVERIFY(engine.imageProvider("test") != nullptr);

    // pixmaps are loaded synchronously regardless of 'asynchronous' value
    QString componentStr = "import QtQuick 2.0\nImage { asynchronous: true; source: \"image://test/pixmap-async-test.png\" }";
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickImage *obj = qobject_cast<QQuickImage*>(component.create());
    QVERIFY(obj != nullptr);

    delete obj;
}

void tst_qquickimageprovider::removeProvider_data()
{
    QTest::addColumn<QQuickImageProvider*>("provider");

    QTest::newRow("qimage") << static_cast<QQuickImageProvider*>(new TestQImageProvider);
    QTest::newRow("qpixmap") << static_cast<QQuickImageProvider*>(new TestQPixmapProvider);
}

void tst_qquickimageprovider::removeProvider()
{
    QFETCH(QQuickImageProvider*, provider);

    QQmlEngine engine;

    engine.addImageProvider("test", provider);
    QVERIFY(engine.imageProvider("test") != nullptr);

    // add provider, confirm it works
    QString componentStr = "import QtQuick 2.0\nImage { source: \"" + newImageFileName() + "\" }";
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickImage *obj = qobject_cast<QQuickImage*>(component.create());
    QVERIFY(obj != nullptr);

    QCOMPARE(obj->status(), QQuickImage::Ready);

    // remove the provider and confirm
    QString fileName = newImageFileName();
    QString error("<Unknown File>:2:1: QML Image: Invalid image provider: " + fileName);
    QTest::ignoreMessage(QtWarningMsg, error.toUtf8());

    engine.removeImageProvider("test");

    obj->setSource(QUrl(fileName));
    QCOMPARE(obj->status(), QQuickImage::Error);

    delete obj;
}

void tst_qquickimageprovider::imageProviderId_data()
{
    QTest::addColumn<QString>("providerId");

    QTest::newRow("lowercase") << QStringLiteral("imageprovider");
    QTest::newRow("CamelCase") << QStringLiteral("ImageProvider");
    QTest::newRow("UPPERCASE") << QStringLiteral("IMAGEPROVIDER");
}

void tst_qquickimageprovider::imageProviderId()
{
    QFETCH(QString, providerId);

    QQmlEngine engine;

    bool deleteWatch = false;
    TestQImageProvider *provider = new TestQImageProvider(&deleteWatch);

    engine.addImageProvider(providerId, provider);
    QVERIFY(engine.imageProvider(providerId) != nullptr);

    engine.removeImageProvider(providerId);
    QVERIFY(deleteWatch);
}

class TestThreadProvider : public QQuickImageProvider
{
    public:
        TestThreadProvider() : QQuickImageProvider(Image) {}

        ~TestThreadProvider() {}

        QImage requestImage(const QString &id, QSize *size, const QSize& requestedSize)
        {
            mutex.lock();
            if (!ok)
                cond.wait(&mutex);
            mutex.unlock();
            QVector<int> v;
            for (int i = 0; i < 10000; i++)
                v.prepend(i); //do some computation
            QImage image(50,50, QImage::Format_RGB32);
            image.fill(QColor(id).rgb());
            if (size)
                *size = image.size();
            if (requestedSize.isValid())
                image = image.scaled(requestedSize);
            return image;
        }

        QWaitCondition cond;
        QMutex mutex;
        bool ok = false;
};


void tst_qquickimageprovider::threadTest()
{
    QQmlEngine engine;

    TestThreadProvider *provider = new TestThreadProvider;

    engine.addImageProvider("test_thread", provider);
    QVERIFY(engine.imageProvider("test_thread") != nullptr);

    QString componentStr = "import QtQuick 2.0\nItem { \n"
            "Image { source: \"image://test_thread/blue\";  asynchronous: true; }\n"
            "Image { source: \"image://test_thread/red\";  asynchronous: true; }\n"
            "Image { source: \"image://test_thread/green\";  asynchronous: true; }\n"
            "Image { source: \"image://test_thread/yellow\";  asynchronous: true; }\n"
            " }";
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QObject *obj = component.create();
    //MUST not deadlock
    QVERIFY(obj != nullptr);
    QList<QQuickImage *> images = obj->findChildren<QQuickImage *>();
    QCOMPARE(images.count(), 4);
    QTest::qWait(100);
    foreach (QQuickImage *img, images) {
        QCOMPARE(img->status(), QQuickImage::Loading);
    }
    {
        QMutexLocker lock(&provider->mutex);
        provider->ok = true;
        provider->cond.wakeAll();
    }
    QTest::qWait(250);
    foreach (QQuickImage *img, images) {
        QTRY_COMPARE(img->status(), QQuickImage::Ready);
    }
}

class TestImageResponseRunner : public QObject, public QRunnable {

    Q_OBJECT

public:
    Q_SIGNAL void finished(QQuickTextureFactory *texture);
    TestImageResponseRunner(QMutex *lock, QWaitCondition *condition, bool *ok, const QString &id, const QSize &requestedSize)
        : m_lock(lock), m_condition(condition), m_ok(ok), m_id(id), m_requestedSize(requestedSize) {}
    void run()
    {
        m_lock->lock();
        if (!(*m_ok)) {
            m_condition->wait(m_lock);
        }
        m_lock->unlock();
        QImage image(50, 50, QImage::Format_RGB32);
        image.fill(QColor(m_id).rgb());
        if (m_requestedSize.isValid())
            image = image.scaled(m_requestedSize);
        emit finished(QQuickTextureFactory::textureFactoryForImage(image));
    }

private:
    QMutex *m_lock;
    QWaitCondition *m_condition;
    bool *m_ok;
    QString m_id;
    QSize m_requestedSize;
};

class TestImageResponse : public QQuickImageResponse
{
    public:
        TestImageResponse(QMutex *lock, QWaitCondition *condition, bool *ok, const QString &id, const QSize &requestedSize, QThreadPool *pool)
         : m_lock(lock), m_condition(condition), m_ok(ok), m_id(id), m_requestedSize(requestedSize), m_texture(nullptr)
        {
            auto runnable = new TestImageResponseRunner(m_lock, m_condition, m_ok, m_id, m_requestedSize);
            QObject::connect(runnable, &TestImageResponseRunner::finished, this, &TestImageResponse::handleResponse);
            pool->start(runnable);
        }

        QQuickTextureFactory *textureFactory() const
        {
            return m_texture;
        }

        void handleResponse(QQuickTextureFactory *factory) {
            this->m_texture = factory;
            emit finished();
        }

        QMutex *m_lock;
        QWaitCondition *m_condition;
        bool *m_ok;
        QString m_id;
        QSize m_requestedSize;
        QQuickTextureFactory *m_texture;
};

class TestAsyncProvider : public QQuickAsyncImageProvider
{
    public:
        TestAsyncProvider()
        {
            pool.setMaxThreadCount(4);
        }

        ~TestAsyncProvider() {}

        QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize)
        {
            TestImageResponse *response = new TestImageResponse(&lock, &condition, &ok, id, requestedSize, &pool);
            return response;
        }

        QThreadPool pool;
        QMutex lock;
        QWaitCondition condition;
        bool ok = false;
};


void tst_qquickimageprovider::asyncTextureTest()
{
    QQmlEngine engine;

    TestAsyncProvider *provider = new TestAsyncProvider;

    engine.addImageProvider("test_async", provider);
    QVERIFY(engine.imageProvider("test_async") != nullptr);

    QString componentStr = "import QtQuick 2.0\nItem { \n"
            "Image { source: \"image://test_async/blue\"; }\n"
            "Image { source: \"image://test_async/red\"; }\n"
            "Image { source: \"image://test_async/green\";  }\n"
            "Image { source: \"image://test_async/yellow\";  }\n"
            " }";
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QObject *obj = component.create();
    //MUST not deadlock
    QVERIFY(obj != nullptr);
    QList<QQuickImage *> images = obj->findChildren<QQuickImage *>();
    QCOMPARE(images.count(), 4);

    QTRY_COMPARE(provider->pool.activeThreadCount(), 4);
    foreach (QQuickImage *img, images) {
        QTRY_COMPARE(img->status(), QQuickImage::Loading);
    }
    {
        QMutexLocker lock(&provider->lock);
        provider->ok = true;
        provider->condition.wakeAll();
    }
    foreach (QQuickImage *img, images) {
        QTRY_COMPARE(img->status(), QQuickImage::Ready);
    }
}

class InstantAsyncImageResponse : public QQuickImageResponse
{
    public:
        InstantAsyncImageResponse(const QString &id, const QSize &requestedSize)
        {
            QImage image(50, 50, QImage::Format_RGB32);
            image.fill(QColor(id).rgb());
            if (requestedSize.isValid())
                image = image.scaled(requestedSize);
            m_texture = QQuickTextureFactory::textureFactoryForImage(image);
            emit finished();
        }

        QQuickTextureFactory *textureFactory() const
        {
            return m_texture;
        }

        QQuickTextureFactory *m_texture;
};

class InstancAsyncProvider : public QQuickAsyncImageProvider
{
    public:
        InstancAsyncProvider()
        {
        }

        ~InstancAsyncProvider() {}

        QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize)
        {
            return new InstantAsyncImageResponse(id, requestedSize);
        }
};

void tst_qquickimageprovider::instantAsyncTextureTest()
{
    QQmlEngine engine;

    InstancAsyncProvider *provider = new InstancAsyncProvider;

    engine.addImageProvider("test_instantasync", provider);
    QVERIFY(engine.imageProvider("test_instantasync") != nullptr);

    QString componentStr = "import QtQuick 2.0\nItem { \n"
            "Image { source: \"image://test_instantasync/blue\"; }\n"
            "Image { source: \"image://test_instantasync/red\"; }\n"
            "Image { source: \"image://test_instantasync/green\";  }\n"
            "Image { source: \"image://test_instantasync/yellow\";  }\n"
            " }";
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QScopedPointer<QObject> obj(component.create());

    QVERIFY(!obj.isNull());
    const QList<QQuickImage *> images = obj->findChildren<QQuickImage *>();
    QCOMPARE(images.count(), 4);

    for (QQuickImage *img: images) {
        QTRY_COMPARE(img->status(), QQuickImage::Ready);
    }
}


class WaitingAsyncImageResponse : public QQuickImageResponse, public QRunnable
{
public:
    WaitingAsyncImageResponse(QMutex *providerRemovedMutex, QWaitCondition *providerRemovedCond, bool *providerRemoved, QMutex *imageRequestedMutex,  QWaitCondition *imageRequestedCond, bool *imageRequested)
        : m_providerRemovedMutex(providerRemovedMutex), m_providerRemovedCond(providerRemovedCond), m_providerRemoved(providerRemoved),
          m_imageRequestedMutex(imageRequestedMutex),  m_imageRequestedCondition(imageRequestedCond),  m_imageRequested(imageRequested)
    {
        setAutoDelete(false);
    }

    void run() override
    {
        m_imageRequestedMutex->lock();
        *m_imageRequested  = true;
        m_imageRequestedCondition->wakeAll();
        m_imageRequestedMutex->unlock();
        m_providerRemovedMutex->lock();
        while (!*m_providerRemoved)
            m_providerRemovedCond->wait(m_providerRemovedMutex);
        m_providerRemovedMutex->unlock();
        emit finished();
    }

    QQuickTextureFactory *textureFactory() const override
    {
        QImage image(50, 50, QImage::Format_RGB32);
        auto texture = QQuickTextureFactory::textureFactoryForImage(image);
        return texture;
    }

    QMutex *m_providerRemovedMutex;
    QWaitCondition *m_providerRemovedCond;
    bool *m_providerRemoved;
    QMutex *m_imageRequestedMutex;
    QWaitCondition *m_imageRequestedCondition;
    bool *m_imageRequested;

};

class WaitingAsyncProvider : public QQuickAsyncImageProvider
{
public:
    WaitingAsyncProvider(QMutex *providerRemovedMutex, QWaitCondition *providerRemovedCond, bool *providerRemoved, QMutex *imageRequestedMutex, QWaitCondition *imageRequestedCond, bool *imageRequested)
        : m_providerRemovedMutex(providerRemovedMutex), m_providerRemovedCond(providerRemovedCond), m_providerRemoved(providerRemoved),
          m_imageRequestedMutex(imageRequestedMutex),  m_imageRequestedCondition(imageRequestedCond),  m_imageRequested(imageRequested)
    {
    }

    ~WaitingAsyncProvider() {}

    QQuickImageResponse *requestImageResponse(const QString & /* id */, const QSize & /* requestedSize */)
    {
        auto response = new WaitingAsyncImageResponse(m_providerRemovedMutex, m_providerRemovedCond, m_providerRemoved, m_imageRequestedMutex, m_imageRequestedCondition, m_imageRequested);
        pool.start(response);
        return response;
    }

    QMutex *m_providerRemovedMutex;
    QWaitCondition *m_providerRemovedCond;
    bool *m_providerRemoved;
    QMutex *m_imageRequestedMutex;
    QWaitCondition *m_imageRequestedCondition;
    bool *m_imageRequested;
    QThreadPool pool;
};


// QTBUG-76527
void tst_qquickimageprovider::asyncImageThreadSafety()
{
    QQmlEngine engine;
    QMutex providerRemovedMutex;
    bool providerRemoved = false;
    QWaitCondition providerRemovedCond;
    QMutex imageRequestedMutex;
    bool imageRequested = false;
    QWaitCondition imageRequestedCond;
    auto imageProvider = new WaitingAsyncProvider(&providerRemovedMutex, &providerRemovedCond, &providerRemoved, &imageRequestedMutex, &imageRequestedCond, &imageRequested);
    engine.addImageProvider("test_waiting", imageProvider);
    QVERIFY(engine.imageProvider("test_waiting") != nullptr);
    auto privateEngine = QQmlEnginePrivate::get(&engine);

    QString componentStr = "import QtQuick 2.0\nItem { \n"
                           "Image { source: \"image://test_waiting/blue\"; }\n"
                           " }";
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QWeakPointer<QQmlImageProviderBase> observer = privateEngine->imageProvider("test_waiting").toWeakRef();
    QVERIFY(!observer.isNull()); // engine still own the object
    imageRequestedMutex.lock();
    while (!imageRequested)
        imageRequestedCond.wait(&imageRequestedMutex);
    imageRequestedMutex.unlock();
    engine.removeImageProvider("test_waiting");

    QVERIFY(engine.imageProvider("test_waiting") == nullptr);
    QVERIFY(!observer.isNull()); // lifetime has been extended

    providerRemovedMutex.lock();
    providerRemoved = true;
    providerRemovedCond.wakeAll();
    providerRemovedMutex.unlock();

    QTRY_VERIFY(observer.isNull()); // once the reply has finished, the imageprovider gets deleted
}


QTEST_MAIN(tst_qquickimageprovider)

#include "tst_qquickimageprovider.moc"
