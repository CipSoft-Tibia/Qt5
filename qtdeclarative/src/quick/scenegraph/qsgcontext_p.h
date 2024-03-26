// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGCONTEXT_H
#define QSGCONTEXT_H

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

#include <QtCore/QObject>
#include <QtCore/qabstractanimation.h>
#include <QtCore/QMutex>

#include <QtGui/QImage>
#include <QtGui/QSurfaceFormat>

#include <private/qtquickglobal_p.h>
#include <private/qrawfont_p.h>

#include <QtQuick/qsgnode.h>
#include <QtQuick/qsgrendererinterface.h>

QT_BEGIN_NAMESPACE

class QSGContextPrivate;
class QSGInternalRectangleNode;
class QSGInternalImageNode;
class QSGPainterNode;
class QSGGlyphNode;
class QSGRenderer;
class QSGDistanceFieldGlyphCache;
class QQuickWindow;
class QSGTexture;
class QSGMaterial;
class QSGRenderLoop;
class QSGLayer;
class QQuickTextureFactory;
class QSGCompressedTextureFactory;
class QSGContext;
class QQuickPaintedItem;
class QSGRendererInterface;
class QSGShaderEffectNode;
class QSGGuiThreadShaderEffectManager;
class QSGRectangleNode;
class QSGImageNode;
class QSGNinePatchNode;
class QSGSpriteNode;
class QSGRenderContext;
class QSGRenderTarget;
class QRhi;
class QRhiRenderTarget;
class QRhiRenderPassDescriptor;
class QRhiCommandBuffer;
class QQuickGraphicsConfiguration;

Q_DECLARE_LOGGING_CATEGORY(QSG_LOG_TIME_RENDERLOOP)
Q_DECLARE_LOGGING_CATEGORY(QSG_LOG_TIME_COMPILATION)
Q_DECLARE_LOGGING_CATEGORY(QSG_LOG_TIME_TEXTURE)
Q_DECLARE_LOGGING_CATEGORY(QSG_LOG_TIME_GLYPH)
Q_DECLARE_LOGGING_CATEGORY(QSG_LOG_TIME_RENDERER)

Q_DECLARE_LOGGING_CATEGORY(QSG_LOG_INFO)
Q_DECLARE_LOGGING_CATEGORY(QSG_LOG_RENDERLOOP)

class Q_QUICK_PRIVATE_EXPORT QSGContext : public QObject
{
    Q_OBJECT

public:
    enum AntialiasingMethod {
        UndecidedAntialiasing,
        VertexAntialiasing,
        MsaaAntialiasing
    };

    explicit QSGContext(QObject *parent = nullptr);
    ~QSGContext() override;

    virtual void renderContextInitialized(QSGRenderContext *renderContext);
    virtual void renderContextInvalidated(QSGRenderContext *renderContext);
    virtual QSGRenderContext *createRenderContext() = 0;

    QSGInternalRectangleNode *createInternalRectangleNode(const QRectF &rect, const QColor &c);
    virtual QSGInternalRectangleNode *createInternalRectangleNode() = 0;
    virtual QSGInternalImageNode *createInternalImageNode(QSGRenderContext *renderContext) = 0;
    virtual QSGPainterNode *createPainterNode(QQuickPaintedItem *item) = 0;
    virtual QSGGlyphNode *createGlyphNode(QSGRenderContext *rc, bool preferNativeGlyphNode, int renderTypeQuality) = 0;
    virtual QSGLayer *createLayer(QSGRenderContext *renderContext) = 0;
    virtual QSGGuiThreadShaderEffectManager *createGuiThreadShaderEffectManager();
    virtual QSGShaderEffectNode *createShaderEffectNode(QSGRenderContext *renderContext);
#if QT_CONFIG(quick_sprite)
    virtual QSGSpriteNode *createSpriteNode() = 0;
#endif
    virtual QAnimationDriver *createAnimationDriver(QObject *parent);
    virtual float vsyncIntervalForAnimationDriver(QAnimationDriver *driver);
    virtual bool isVSyncDependent(QAnimationDriver *driver);

    virtual QSize minimumFBOSize() const;
    virtual QSurfaceFormat defaultSurfaceFormat() const = 0;

    virtual QSGRendererInterface *rendererInterface(QSGRenderContext *renderContext);

    virtual QSGRectangleNode *createRectangleNode() = 0;
    virtual QSGImageNode *createImageNode() = 0;
    virtual QSGNinePatchNode *createNinePatchNode() = 0;

    static QSGContext *createDefaultContext();
    static QQuickTextureFactory *createTextureFactoryFromImage(const QImage &image);
    static QSGRenderLoop *createWindowManager();

    static void setBackend(const QString &backend);
    static QString backend();
};

class Q_QUICK_PRIVATE_EXPORT QSGRenderContext : public QObject
{
    Q_OBJECT
public:
    enum CreateTextureFlags {
        CreateTexture_Alpha       = 0x1,
        CreateTexture_Atlas       = 0x2,
        CreateTexture_Mipmap      = 0x4
    };

    QSGRenderContext(QSGContext *context);
    ~QSGRenderContext() override;

    QSGContext *sceneGraphContext() const { return m_sg; }
    virtual bool isValid() const { return true; }

    struct InitParams { };
    virtual void initialize(const InitParams *params);
    virtual void invalidate();

    using RenderPassCallback = void (*)(void *);

    virtual void prepareSync(qreal devicePixelRatio,
                             QRhiCommandBuffer *cb,
                             const QQuickGraphicsConfiguration &config);

    virtual void beginNextFrame(QSGRenderer *renderer, const QSGRenderTarget &renderTarget,
                                RenderPassCallback mainPassRecordingStart,
                                RenderPassCallback mainPassRecordingEnd,
                                void *callbackUserData);
    virtual void renderNextFrame(QSGRenderer *renderer) = 0;
    virtual void endNextFrame(QSGRenderer *renderer);

    virtual void endSync();

    virtual void preprocess();
    virtual void invalidateGlyphCaches();
    virtual QSGDistanceFieldGlyphCache *distanceFieldGlyphCache(const QRawFont &font, int renderTypeQuality);
    QSGTexture *textureForFactory(QQuickTextureFactory *factory, QQuickWindow *window);

    virtual QSGTexture *createTexture(const QImage &image, uint flags = CreateTexture_Alpha) const = 0;
    virtual QSGRenderer *createRenderer(QSGRendererInterface::RenderMode renderMode = QSGRendererInterface::RenderMode2D) = 0;
    virtual QSGTexture *compressedTextureForFactory(const QSGCompressedTextureFactory *) const;

    virtual int maxTextureSize() const = 0;

    void unregisterFontengineForCleanup(QFontEngine *engine);
    void registerFontengineForCleanup(QFontEngine *engine);

    virtual QRhi *rhi() const;

Q_SIGNALS:
    void initialized();
    void invalidated();
    void releaseCachedResourcesRequested();

public Q_SLOTS:
    void textureFactoryDestroyed(QObject *o);

protected:
    // Hold m_sg with QPointer in the rare case it gets deleted before us.
    QPointer<QSGContext> m_sg;

    QMutex m_mutex;
    QHash<QObject *, QSGTexture *> m_textures;
    QSet<QSGTexture *> m_texturesToDelete;
    QHash<QString, QSGDistanceFieldGlyphCache *> m_glyphCaches;

    // References to font engines that are currently in use by native rendering glyph nodes
    // and which must be kept alive as long as they are used in the render thread.
    QHash<QFontEngine *, int> m_fontEnginesToClean;
};

QT_END_NAMESPACE

#endif // QSGCONTEXT_H
