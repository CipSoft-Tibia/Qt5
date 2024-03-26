// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDWINDOW_H
#define QWAYLANDWINDOW_H

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

#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>
#include <QtCore/QReadWriteLock>

#include <QtGui/QIcon>
#include <QtGui/QEventPoint>
#include <QtCore/QVariant>
#include <QtCore/QLoggingCategory>
#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QMap> // for QVariantMap

#include <qpa/qplatformwindow.h>
#include <qpa/qplatformwindow_p.h>

#include <QtWaylandClient/private/qwayland-wayland.h>
#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtWaylandClient/qtwaylandclientglobal.h>
#include <QtWaylandClient/private/qwaylandshellsurface_p.h>

struct wl_egl_window;

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

Q_DECLARE_LOGGING_CATEGORY(lcWaylandBackingstore)

class QWaylandDisplay;
class QWaylandBuffer;
class QWaylandShellSurface;
class QWaylandSubSurface;
class QWaylandAbstractDecoration;
class QWaylandInputDevice;
class QWaylandScreen;
class QWaylandShellIntegration;
class QWaylandShmBackingStore;
class QWaylandPointerEvent;
class QWaylandPointerGestureSwipeEvent;
class QWaylandPointerGesturePinchEvent;
class QWaylandSurface;
class QWaylandFractionalScale;
class QWaylandViewport;

class Q_WAYLANDCLIENT_EXPORT QWaylandWindow : public QNativeInterface::Private::QWaylandWindow,
                                              public QPlatformWindow
{
    Q_OBJECT
public:
    enum WindowType {
        Shm,
        Egl,
        Vulkan
    };

    enum ToplevelWindowTilingState {
        WindowNoState = 0,
        WindowTiledLeft = 1,
        WindowTiledRight = 2,
        WindowTiledTop = 4,
        WindowTiledBottom = 8
    };
    Q_DECLARE_FLAGS(ToplevelWindowTilingStates, ToplevelWindowTilingState)

    QWaylandWindow(QWindow *window, QWaylandDisplay *display);
    ~QWaylandWindow() override;

    // Keep Toplevels position on the top left corner of their screen
    static inline bool fixedToplevelPositions = true;

    virtual WindowType windowType() const = 0;
    virtual void ensureSize();
    WId winId() const override;
    void setVisible(bool visible) override;
    void setParent(const QPlatformWindow *parent) override;

    void setWindowTitle(const QString &title) override;

    inline QIcon windowIcon() const;
    void setWindowIcon(const QIcon &icon) override;

    void setGeometry(const QRect &rect) override;
    void resizeFromApplyConfigure(const QSize &sizeWithMargins, const QPoint &offset = {0, 0});
    void repositionFromApplyConfigure(const QPoint &position);
    void setGeometryFromApplyConfigure(const QPoint &globalPosition, const QSize &sizeWithMargins);

    void applyConfigureWhenPossible(); //rename to possible?

    void attach(QWaylandBuffer *buffer, int x, int y);
    void attachOffset(QWaylandBuffer *buffer);
    QPoint attachOffset() const;

    void damage(const QRect &rect);

    void safeCommit(QWaylandBuffer *buffer, const QRegion &damage);
    void handleExpose(const QRegion &region);
    void commit(QWaylandBuffer *buffer, const QRegion &damage);

    void commit();

    bool waitForFrameSync(int timeout);

    QMargins frameMargins() const override;
    QMargins clientSideMargins() const;
    void setCustomMargins(const QMargins &margins) override;
    QSize surfaceSize() const;
    QMargins windowContentMargins() const;
    QRect windowContentGeometry() const;
    QPointF mapFromWlSurface(const QPointF &surfacePosition) const;

    QWaylandSurface *waylandSurface() const { return mSurface.data(); }
    ::wl_surface *wlSurface();
    ::wl_surface *surface() const override
    {
        return const_cast<QWaylandWindow *>(this)->wlSurface();
    }
    static QWaylandWindow *fromWlSurface(::wl_surface *surface);

    QWaylandDisplay *display() const { return mDisplay; }
    QWaylandShellSurface *shellSurface() const;
    std::any _surfaceRole() const override;
    QWaylandSubSurface *subSurfaceWindow() const;
    QWaylandScreen *waylandScreen() const;

    void handleContentOrientationChange(Qt::ScreenOrientation orientation) override;
    void setOrientationMask(Qt::ScreenOrientations mask);

    ToplevelWindowTilingStates toplevelWindowTilingStates() const;
    void handleToplevelWindowTilingStatesChanged(ToplevelWindowTilingStates states);

    Qt::WindowStates windowStates() const;
    void setWindowState(Qt::WindowStates states) override;
    void setWindowFlags(Qt::WindowFlags flags) override;
    void handleWindowStatesChanged(Qt::WindowStates states);

    void raise() override;
    void lower() override;

    void setMask(const QRegion &region) override;

    void setAlertState(bool enabled) override;
    bool isAlertState() const override;

    qreal scale() const;
    qreal devicePixelRatio() const override;

    void requestActivateWindow() override;
    bool isExposed() const override;
    bool isActive() const override;

    QWaylandAbstractDecoration *decoration() const;

    void handleMouse(QWaylandInputDevice *inputDevice, const QWaylandPointerEvent &e);
#ifndef QT_NO_GESTURES
    void handleSwipeGesture(QWaylandInputDevice *inputDevice,
                            const QWaylandPointerGestureSwipeEvent &e);
    void handlePinchGesture(QWaylandInputDevice *inputDevice,
                            const QWaylandPointerGesturePinchEvent &e);
#endif

    bool touchDragDecoration(QWaylandInputDevice *inputDevice, const QPointF &local, const QPointF &global,
                             QEventPoint::State state, Qt::KeyboardModifiers mods);

    bool createDecoration();

#if QT_CONFIG(cursor)
    void setMouseCursor(QWaylandInputDevice *device, const QCursor &cursor);
    void restoreMouseCursor(QWaylandInputDevice *device);
#endif

    QWaylandWindow *transientParent() const;

    QMutex *resizeMutex() { return &mResizeLock; }
    void doApplyConfigure();
    void setCanResize(bool canResize);

    bool setMouseGrabEnabled(bool grab) override;
    static QWaylandWindow *mouseGrab() { return mMouseGrab; }

    void sendProperty(const QString &name, const QVariant &value);
    void setProperty(const QString &name, const QVariant &value);

    QVariantMap properties() const;
    QVariant property(const QString &name);
    QVariant property(const QString &name, const QVariant &defaultValue);

#ifdef QT_PLATFORM_WINDOW_HAS_VIRTUAL_SET_BACKING_STORE
    void setBackingStore(QPlatformBackingStore *store) override;
#else
    void setBackingStore(QWaylandShmBackingStore *backingStore) { mBackingStore = backingStore; }
#endif
    QWaylandShmBackingStore *backingStore() const { return mBackingStore; }

    void setShellIntegration(QWaylandShellIntegration *shellIntegration);
    QWaylandShellIntegration *shellIntegration() const { return mShellIntegration; }

    bool setKeyboardGrabEnabled(bool) override { return false; }
    void propagateSizeHints() override;
    void addAttachOffset(const QPoint point);

    bool startSystemResize(Qt::Edges edges) override;
    bool startSystemMove() override;

    void timerEvent(QTimerEvent *event) override;
    void requestUpdate() override;
    void handleUpdate();
    void deliverUpdateRequest() override;

    void setXdgActivationToken(const QString &token);
    void requestXdgActivationToken(uint serial) override;

    void beginFrame();
    void endFrame();

    void closeChildPopups();

    virtual void reinit();
    void reset();

public slots:
    void applyConfigure();

signals:
    void wlSurfaceCreated();
    void wlSurfaceDestroyed();

protected:
    virtual void doHandleFrameCallback();
    virtual QRect defaultGeometry() const;
    void sendExposeEvent(const QRect &rect);

    QWaylandDisplay *mDisplay = nullptr;

    // mSurface can be written by the main thread. Other threads should claim a read lock for access
    mutable QReadWriteLock mSurfaceLock;
    QScopedPointer<QWaylandSurface> mSurface;
    QScopedPointer<QWaylandFractionalScale> mFractionalScale;
    QScopedPointer<QWaylandViewport> mViewport;

    QWaylandShellIntegration *mShellIntegration = nullptr;
    QWaylandShellSurface *mShellSurface = nullptr;
    QWaylandSubSurface *mSubSurfaceWindow = nullptr;
    QList<QWaylandSubSurface *> mChildren;

    QWaylandAbstractDecoration *mWindowDecoration = nullptr;
    bool mWindowDecorationEnabled = false;
    bool mMouseEventsInContentArea = false;
    Qt::MouseButtons mMousePressedInContentArea = Qt::NoButton;

#ifndef QT_NO_GESTURES
    enum GestureState {
        GestureNotActive,
        GestureActiveInContentArea,
        GestureActiveInDecoration
    };

    // We want gestures started in the decoration area to be completely ignored even if the mouse
    // pointer is later moved to content area. Likewise, gestures started in the content area should
    // keep sending events even if the mouse pointer is moved over the decoration (consider that
    // the events for that gesture will be sent to us even if it's moved outside the window).
    // So we track the gesture state and accept or ignore events based on that. Note that
    // concurrent gestures of different types are not allowed in the protocol, so single state is
    // enough
    GestureState mGestureState = GestureNotActive;
#endif

    WId mWindowId;
    bool mFrameCallbackTimedOut = false; // Whether the frame callback has timed out
    int mFrameCallbackCheckIntervalTimerId = -1;
    QAtomicInt mWaitingForUpdateDelivery = false;

    bool mWaitingForFrameCallback = false; // Protected by mFrameSyncMutex
    QElapsedTimer mFrameCallbackElapsedTimer; // Protected by mFrameSyncMutex
    struct ::wl_callback *mFrameCallback = nullptr; // Protected by mFrameSyncMutex
    QMutex mFrameSyncMutex;
    QWaitCondition mFrameSyncWait;

    // True when we have called deliverRequestUpdate, but the client has not yet attached a new buffer
    bool mWaitingForUpdate = false;

    QMutex mResizeLock;
    bool mWaitingToApplyConfigure = false;
    bool mCanResize = true;
    bool mResizeDirty = false;
    bool mResizeAfterSwap;
    int mFrameCallbackTimeout = 100;
    QVariantMap m_properties;

    bool mSentInitialResize = false;
    QPoint mOffset;
    qreal mScale = 1;
    QPlatformScreen *mLastReportedScreen = nullptr;

    QIcon mWindowIcon;

    Qt::WindowFlags mFlags;
    QRegion mMask;

    // Empty QRegion maps to "infinite" input region, needs a dedicated "deliberately empty" state.
    QRegion mInputRegion;
    bool mTransparentInputRegion = false;

    QRegion mOpaqueArea;
    Qt::WindowStates mLastReportedWindowStates = Qt::WindowNoState;
    ToplevelWindowTilingStates mLastReportedToplevelWindowTilingStates = WindowNoState;

    QWaylandShmBackingStore *mBackingStore = nullptr;
    QWaylandBuffer *mQueuedBuffer = nullptr;
    QRegion mQueuedBufferDamage;

    QMargins mCustomMargins;

    QPointer<QWaylandWindow> mTransientParent;
    QList<QPointer<QWaylandWindow>> mChildPopups;

private slots:
    void doApplyConfigureFromOtherThread();

private:
    void setGeometry_helper(const QRect &rect);
    void initWindow();
    void initializeWlSurface();
    bool shouldCreateShellSurface() const;
    bool shouldCreateSubSurface() const;
    QPlatformScreen *calculateScreenFromSurfaceEvents() const;
    void setOpaqueArea(const QRegion &opaqueArea);
    bool isOpaque() const;
    void updateInputRegion();
    void updateViewport();

    void handleMouseEventWithDecoration(QWaylandInputDevice *inputDevice, const QWaylandPointerEvent &e);
    void handleScreensChanged();
    void sendRecursiveExposeEvent();

    QWaylandWindow *guessTransientParent() const;
    void addChildPopup(QWaylandWindow *child);
    void removeChildPopup(QWaylandWindow *child);

    bool mInResizeFromApplyConfigure = false;
    bool lastVisible = false;
    QRect mLastExposeGeometry;

    static const wl_callback_listener callbackListener;
    void handleFrameCallback(struct ::wl_callback* callback);

    static QWaylandWindow *mMouseGrab;
    static QWaylandWindow *mTopPopup;

    friend class QWaylandSubSurface;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QWaylandWindow::ToplevelWindowTilingStates)

inline QIcon QWaylandWindow::windowIcon() const
{
    return mWindowIcon;
}

inline QPoint QWaylandWindow::attachOffset() const
{
    return mOffset;
}

}

QT_END_NAMESPACE

#endif // QWAYLANDWINDOW_H
