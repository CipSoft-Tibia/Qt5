// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKCANVASITEM_P_H
#define QQUICKCANVASITEM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_canvas);

#include <QtQuick/qquickitem.h>
#include <private/qqmlrefcount_p.h>
#include <QtCore/QThread>
#include <QtCore/qmutex.h>
#include <QtGui/QImage>

QT_BEGIN_NAMESPACE

class QQuickCanvasContext;

class QQuickCanvasItemPrivate;
class QQuickPixmap;
class QQmlV4Function;

class QQuickCanvasPixmap : public QQmlRefCounted<QQuickCanvasPixmap>
{
public:
    QQuickCanvasPixmap(const QImage& image);
    QQuickCanvasPixmap(QQuickPixmap *pixmap);
    ~QQuickCanvasPixmap();

    QImage image();

    qreal width() const;
    qreal height() const;
    bool isValid() const;
    QQuickPixmap *pixmap() const { return m_pixmap;}

private:
    QQuickPixmap *m_pixmap;
    QImage m_image;
};

class QQuickCanvasItem : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged FINAL)
    Q_PROPERTY(QString contextType READ contextType WRITE setContextType NOTIFY contextTypeChanged FINAL)
    Q_PROPERTY(QJSValue context READ context NOTIFY contextChanged FINAL)
    Q_PROPERTY(QSizeF canvasSize READ canvasSize WRITE setCanvasSize NOTIFY canvasSizeChanged FINAL)
    Q_PROPERTY(QSize tileSize READ tileSize WRITE setTileSize NOTIFY tileSizeChanged FINAL)
    Q_PROPERTY(QRectF canvasWindow READ canvasWindow WRITE setCanvasWindow NOTIFY canvasWindowChanged FINAL)
    Q_PROPERTY(RenderTarget renderTarget READ renderTarget WRITE setRenderTarget NOTIFY renderTargetChanged FINAL)
    Q_PROPERTY(RenderStrategy renderStrategy READ renderStrategy WRITE setRenderStrategy NOTIFY renderStrategyChanged FINAL)
    QML_NAMED_ELEMENT(Canvas)
    QML_ADDED_IN_VERSION(2, 0)

public:
    enum RenderTarget {
        Image,
        FramebufferObject
    };
    Q_ENUM(RenderTarget)

    enum RenderStrategy {
        Immediate,
        Threaded,
        Cooperative
    };
    Q_ENUM(RenderStrategy)

    QQuickCanvasItem(QQuickItem *parent = nullptr);
    ~QQuickCanvasItem();

    bool isAvailable() const;

    QString contextType() const;
    void setContextType(const QString &contextType);

    QJSValue context() const;

    QSizeF canvasSize() const;
    void setCanvasSize(const QSizeF &);

    QSize tileSize() const;
    void setTileSize(const QSize &);

    QRectF canvasWindow() const;
    void setCanvasWindow(const QRectF& rect);

    RenderTarget renderTarget() const;
    void setRenderTarget(RenderTarget target);

    RenderStrategy renderStrategy() const;
    void setRenderStrategy(RenderStrategy strategy);

    QQuickCanvasContext *rawContext() const;

    QImage toImage(const QRectF& rect = QRectF()) const;

    Q_INVOKABLE void getContext(QQmlV4Function *args);

    Q_INVOKABLE void requestAnimationFrame(QQmlV4Function *args);
    Q_INVOKABLE void cancelRequestAnimationFrame(QQmlV4Function *args);

    Q_INVOKABLE void requestPaint();
    Q_INVOKABLE void markDirty(const QRectF& dirtyRect = QRectF());

    Q_INVOKABLE bool save(const QString &filename, const QSizeF &imageSize = QSizeF()) const;
    Q_INVOKABLE QString toDataURL(const QString& type = QLatin1String("image/png")) const;
    QQmlRefPointer<QQuickCanvasPixmap> loadedPixmap(const QUrl& url);

    bool isTextureProvider() const override;
    QSGTextureProvider *textureProvider() const override;

Q_SIGNALS:
    void paint(const QRect &region);
    void painted();
    void availableChanged();
    void contextTypeChanged();
    void contextChanged();
    void canvasSizeChanged();
    void tileSizeChanged();
    void canvasWindowChanged();
    void renderTargetChanged();
    void renderStrategyChanged();
    void imageLoaded();

public Q_SLOTS:
    void loadImage(const QUrl& url);
    void unloadImage(const QUrl& url);
    bool isImageLoaded(const QUrl& url) const;
    bool isImageLoading(const QUrl& url) const;
    bool isImageError(const QUrl& url) const;

private Q_SLOTS:
    void sceneGraphInitialized();
    void checkAnimationCallbacks();
    void invalidateSceneGraph();
    void schedulePolish();

protected:
    void componentComplete() override;
    void itemChange(QQuickItem::ItemChange, const QQuickItem::ItemChangeData &) override;
    void updatePolish() override;
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void releaseResources() override;
    bool event(QEvent *event) override;
private:
    Q_DECLARE_PRIVATE(QQuickCanvasItem)
    Q_INVOKABLE void delayedCreate();
    bool createContext(const QString &contextType);
    void initializeContext(QQuickCanvasContext *context, const QVariantMap &args = QVariantMap());
    static QRect tiledRect(const QRectF &window, const QSize &tileSize);
    bool isPaintConnected();
};

class QQuickContext2DRenderThread : public QThread
{
    Q_OBJECT
public:
    QQuickContext2DRenderThread(QQmlEngine *eng);
    ~QQuickContext2DRenderThread();

    static QQuickContext2DRenderThread *instance(QQmlEngine *engine);

private:
    QQmlEngine *m_engine;
    QObject *m_eventLoopQuitHack;
    static QHash<QQmlEngine *,QQuickContext2DRenderThread*> renderThreads;
    static QMutex renderThreadsMutex;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickCanvasItem)

#endif //QQUICKCANVASITEM_P_H
