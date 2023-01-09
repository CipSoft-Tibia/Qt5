/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qwaylanddisplay_p.h"

#include "qwaylandintegration_p.h"
#include "qwaylandwindow_p.h"
#include "qwaylandsurface_p.h"
#include "qwaylandabstractdecoration_p.h"
#include "qwaylandscreen_p.h"
#include "qwaylandcursor_p.h"
#include "qwaylandinputdevice_p.h"
#if QT_CONFIG(clipboard)
#include "qwaylandclipboard_p.h"
#endif
#if QT_CONFIG(wayland_datadevice)
#include "qwaylanddatadevicemanager_p.h"
#include "qwaylanddatadevice_p.h"
#endif // QT_CONFIG(wayland_datadevice)
#if QT_CONFIG(wayland_client_primary_selection)
#include "qwaylandprimaryselectionv1_p.h"
#endif // QT_CONFIG(wayland_client_primary_selection)
#if QT_CONFIG(cursor)
#include <wayland-cursor.h>
#endif
#include "qwaylandhardwareintegration_p.h"
#include "qwaylandinputcontext_p.h"

#include "qwaylandwindowmanagerintegration_p.h"
#include "qwaylandshellintegration_p.h"
#include "qwaylandclientbufferintegration_p.h"

#include "qwaylandextendedsurface_p.h"
#include "qwaylandsubsurface_p.h"
#include "qwaylandtouch_p.h"
#include "qwaylandtabletv2_p.h"
#include "qwaylandqtkey_p.h"

#include <QtWaylandClient/private/qwayland-text-input-unstable-v2.h>
#include <QtWaylandClient/private/qwayland-wp-primary-selection-unstable-v1.h>

#include <QtCore/private/qcore_unix_p.h>

#include <QtCore/QAbstractEventDispatcher>
#include <QtGui/qpa/qwindowsysteminterface.h>
#include <QtGui/private/qguiapplication_p.h>

#include <QtCore/QDebug>

#include <errno.h>

#include <tuple> // for std::tie

static void checkWaylandError(struct wl_display *display)
{
    int ecode = wl_display_get_error(display);
    if ((ecode == EPIPE || ecode == ECONNRESET)) {
        // special case this to provide a nicer error
        qWarning("The Wayland connection broke. Did the Wayland compositor die?");
    } else {
        qWarning("The Wayland connection experienced a fatal error: %s", strerror(ecode));
    }
    _exit(1);
}

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class EventThread : public QThread
{
    Q_OBJECT
public:
    enum OperatingMode {
        EmitToDispatch, // Emit the signal, allow dispatching in a differnt thread.
        SelfDispatch, // Dispatch the events inside this thread.
    };

    EventThread(struct wl_display * wl, struct wl_event_queue * ev_queue,
                OperatingMode mode)
        : m_fd(wl_display_get_fd(wl))
        , m_pipefd{ -1, -1 }
        , m_wldisplay(wl)
        , m_wlevqueue(ev_queue)
        , m_mode(mode)
        , m_reading(true)
        , m_quitting(false)
    {
        setObjectName(QStringLiteral("WaylandEventThread"));
    }

    void readAndDispatchEvents()
    {
        /*
         * Dispatch pending events and flush the requests at least once. If the event thread
         * is not reading, try to call _prepare_read() to allow the event thread to poll().
         * If that fails, re-try dispatch & flush again until _prepare_read() is successful.
         *
         * This allow any call to readAndDispatchEvents() to start event thread's polling,
         * not only the one issued from event thread's waitForReading(), which means functions
         * called from dispatch_pending() can safely spin an event loop.
         */
        for (;;) {
            if (dispatchQueuePending() < 0) {
                checkWaylandError(m_wldisplay);
                return;
            }

            wl_display_flush(m_wldisplay);

            // We have to check if event thread is reading every time we dispatch
            // something, as that may recursively call this function.
            if (m_reading.loadAcquire())
                break;

            if (prepareReadQueue() == 0) {
                QMutexLocker l(&m_mutex);
                m_reading.storeRelease(true);
                m_cond.wakeOne();
                break;
            }
        }
    }

    void stop()
    {
        // We have to both write to the pipe and set the flag, as the thread may be
        // either in the poll() or waiting for _prepare_read().
        if (m_pipefd[1] != -1 && write(m_pipefd[1], "\0", 1) == -1)
            qWarning("Failed to write to the pipe: %s.", strerror(errno));

        {
            QMutexLocker l(&m_mutex);
            m_quitting = true;
            m_cond.wakeOne();
        }

        wait();
    }

Q_SIGNALS:
    void needReadAndDispatch();

protected:
    void run() override
    {
        // we use this pipe to make the loop exit otherwise if we simply used a flag on the loop condition, if stop() gets
        // called while poll() is blocking the thread will never quit since there are no wayland messages coming anymore.
        struct Pipe
        {
            Pipe(int *fds)
                : fds(fds)
            {
                if (qt_safe_pipe(fds) != 0)
                    qWarning("Pipe creation failed. Quitting may hang.");
            }
            ~Pipe()
            {
                if (fds[0] != -1) {
                    close(fds[0]);
                    close(fds[1]);
                }
            }

            int *fds;
        } pipe(m_pipefd);

        // Make the main thread call wl_prepare_read(), dispatch the pending messages and flush the
        // outbound ones. Wait until it's done before proceeding, unless we're told to quit.
        while (waitForReading()) {
            pollfd fds[2] = { { m_fd, POLLIN, 0 }, { m_pipefd[0], POLLIN, 0 } };
            poll(fds, 2, -1);

            if (fds[1].revents & POLLIN) {
                // we don't really care to read the byte that was written here since we're closing down
                wl_display_cancel_read(m_wldisplay);
                break;
            }

            if (fds[0].revents & POLLIN)
                wl_display_read_events(m_wldisplay);
                // The polll was succesfull and the event thread did the wl_display_read_events(). On the next iteration of the loop
                // the event sent to the main thread will cause it to dispatch the messages just read, unless the loop exits in which
                // case we don't care anymore about them.
            else
                wl_display_cancel_read(m_wldisplay);
        }
    }

private:
    bool waitForReading()
    {
        Q_ASSERT(QThread::currentThread() == this);

        m_reading.storeRelease(false);

        if (m_mode == SelfDispatch) {
            readAndDispatchEvents();
        } else {
            Q_EMIT needReadAndDispatch();

            QMutexLocker lock(&m_mutex);
            // m_reading might be set from our emit or some other invocation of
            // readAndDispatchEvents().
            while (!m_reading.loadRelaxed() && !m_quitting)
                m_cond.wait(&m_mutex);
        }

        return !m_quitting;
    }

    int dispatchQueuePending()
    {
        if (m_wlevqueue)
            return wl_display_dispatch_queue_pending(m_wldisplay, m_wlevqueue);
        else
            return wl_display_dispatch_pending(m_wldisplay);
    }

    int prepareReadQueue()
    {
        if (m_wlevqueue)
            return wl_display_prepare_read_queue(m_wldisplay, m_wlevqueue);
        else
            return wl_display_prepare_read(m_wldisplay);
    }

    int m_fd;
    int m_pipefd[2];
    wl_display *m_wldisplay;
    wl_event_queue *m_wlevqueue;
    OperatingMode m_mode;

    /* Concurrency note when operating in EmitToDispatch mode:
     * m_reading is set to false inside event thread's waitForReading(), and is
     * set to true inside main thread's readAndDispatchEvents().
     * The lock is not taken when setting m_reading to false, as the main thread
     * is not actively waiting for it to turn false. However, the lock is taken
     * inside readAndDispatchEvents() before setting m_reading to true,
     * as the event thread is actively waiting for it under the wait condition.
     */

    QAtomicInteger<bool> m_reading;
    bool m_quitting;
    QMutex m_mutex;
    QWaitCondition m_cond;
};

Q_LOGGING_CATEGORY(lcQpaWayland, "qt.qpa.wayland"); // for general (uncategorized) Wayland platform logging

struct wl_surface *QWaylandDisplay::createSurface(void *handle)
{
    struct wl_surface *surface = mCompositor.create_surface();
    wl_surface_set_user_data(surface, handle);
    return surface;
}

struct ::wl_region *QWaylandDisplay::createRegion(const QRegion &qregion)
{
    struct ::wl_region *region = mCompositor.create_region();

    for (const QRect &rect : qregion)
        wl_region_add(region, rect.x(), rect.y(), rect.width(), rect.height());

    return region;
}

::wl_subsurface *QWaylandDisplay::createSubSurface(QWaylandWindow *window, QWaylandWindow *parent)
{
    if (!mSubCompositor) {
        qCWarning(lcQpaWayland) << "Can't create subsurface, not supported by the compositor.";
        return nullptr;
    }

    // Make sure we don't pass NULL surfaces to libwayland (crashes)
    Q_ASSERT(parent->wlSurface());
    Q_ASSERT(window->wlSurface());

    return mSubCompositor->get_subsurface(window->wlSurface(), parent->wlSurface());
}

QWaylandShellIntegration *QWaylandDisplay::shellIntegration() const
{
    return mWaylandIntegration->shellIntegration();
}

QWaylandClientBufferIntegration * QWaylandDisplay::clientBufferIntegration() const
{
    return mWaylandIntegration->clientBufferIntegration();
}

QWaylandWindowManagerIntegration *QWaylandDisplay::windowManagerIntegration() const
{
    return mWindowManagerIntegration.data();
}

QWaylandDisplay::QWaylandDisplay(QWaylandIntegration *waylandIntegration)
    : mWaylandIntegration(waylandIntegration)
{
    qRegisterMetaType<uint32_t>("uint32_t");

    mDisplay = wl_display_connect(nullptr);
    if (!mDisplay) {
        qErrnoWarning(errno, "Failed to create wl_display");
        return;
    }

    struct ::wl_registry *registry = wl_display_get_registry(mDisplay);
    init(registry);

    mWindowManagerIntegration.reset(new QWaylandWindowManagerIntegration(this));

#if QT_CONFIG(xkbcommon)
    mXkbContext.reset(xkb_context_new(XKB_CONTEXT_NO_FLAGS));
    if (!mXkbContext)
        qCWarning(lcQpaWayland, "failed to create xkb context");
#endif
}

QWaylandDisplay::~QWaylandDisplay(void)
{
    if (m_eventThread)
        m_eventThread->stop();

    if (m_frameEventQueueThread)
        m_frameEventQueueThread->stop();

    if (mSyncCallback)
        wl_callback_destroy(mSyncCallback);

    qDeleteAll(qExchange(mInputDevices, {}));

    for (QWaylandScreen *screen : qExchange(mScreens, {})) {
        QWindowSystemInterface::handleScreenRemoved(screen);
    }
    qDeleteAll(mWaitingScreens);

#if QT_CONFIG(wayland_datadevice)
    delete mDndSelectionHandler.take();
#endif
#if QT_CONFIG(cursor)
    qDeleteAll(mCursorThemes);
#endif
    if (mDisplay)
        wl_display_disconnect(mDisplay);

    if (m_frameEventQueue)
        wl_event_queue_destroy(m_frameEventQueue);
}

// Steps which is called just after constructor. This separates registry_global() out of the constructor
// so that factory functions in integration can be overridden.
void QWaylandDisplay::initialize()
{
    forceRoundTrip();

    if (!mWaitingScreens.isEmpty()) {
        // Give wl_output.done and zxdg_output_v1.done events a chance to arrive
        forceRoundTrip();
    }
}

void QWaylandDisplay::ensureScreen()
{
    if (!mScreens.empty() || mPlaceholderScreen)
        return; // There are real screens or we already have a fake one

    qCInfo(lcQpaWayland) << "Creating a fake screen in order for Qt not to crash";

    mPlaceholderScreen = new QPlatformPlaceholderScreen();
    QWindowSystemInterface::handleScreenAdded(mPlaceholderScreen);
    Q_ASSERT(!QGuiApplication::screens().empty());
}

void QWaylandDisplay::checkError() const
{
    checkWaylandError(mDisplay);
}

// Called in main thread, either from queued signal or directly.
void QWaylandDisplay::flushRequests()
{
    m_eventThread->readAndDispatchEvents();
}

// We have to wait until we have an eventDispatcher before creating the eventThread,
// otherwise forceRoundTrip() may block inside _events_read() because eventThread is
// polling.
void QWaylandDisplay::initEventThread()
{
    m_eventThread.reset(
            new EventThread(mDisplay, /* default queue */ nullptr, EventThread::EmitToDispatch));
    connect(m_eventThread.get(), &EventThread::needReadAndDispatch, this,
            &QWaylandDisplay::flushRequests, Qt::QueuedConnection);
    m_eventThread->start();

    // wl_display_disconnect() free this.
    m_frameEventQueue = wl_display_create_queue(mDisplay);
    m_frameEventQueueThread.reset(
            new EventThread(mDisplay, m_frameEventQueue, EventThread::SelfDispatch));
    m_frameEventQueueThread->start();
}

void QWaylandDisplay::blockingReadEvents()
{
    if (wl_display_dispatch(mDisplay) < 0)
        checkWaylandError(mDisplay);
}

QWaylandScreen *QWaylandDisplay::screenForOutput(struct wl_output *output) const
{
    for (auto screen : qAsConst(mScreens)) {
        if (screen->output() == output)
            return screen;
    }
    return nullptr;
}

void QWaylandDisplay::handleScreenInitialized(QWaylandScreen *screen)
{
    if (!mWaitingScreens.removeOne(screen))
        return;
    mScreens.append(screen);
    QWindowSystemInterface::handleScreenAdded(screen);
    if (mPlaceholderScreen) {
        QWindowSystemInterface::handleScreenRemoved(mPlaceholderScreen);
        // handleScreenRemoved deletes the platform screen
        mPlaceholderScreen = nullptr;
    }
}

void QWaylandDisplay::waitForScreens()
{
    flushRequests();

    while (true) {
        bool screensReady = !mScreens.isEmpty();

        for (int ii = 0; screensReady && ii < mScreens.count(); ++ii) {
            if (mScreens.at(ii)->geometry() == QRect(0, 0, 0, 0))
                screensReady = false;
        }

        if (!screensReady)
            blockingReadEvents();
        else
            return;
    }
}

void QWaylandDisplay::registry_global(uint32_t id, const QString &interface, uint32_t version)
{
    struct ::wl_registry *registry = object();

    if (interface == QStringLiteral("wl_output")) {
        mWaitingScreens << new QWaylandScreen(this, version, id);
    } else if (interface == QStringLiteral("wl_compositor")) {
        mCompositorVersion = qMin((int)version, 4);
        mCompositor.init(registry, id, mCompositorVersion);
    } else if (interface == QStringLiteral("wl_shm")) {
        mShm.reset(new QWaylandShm(this, version, id));
    } else if (interface == QStringLiteral("wl_seat")) {
        QWaylandInputDevice *inputDevice = mWaylandIntegration->createInputDevice(this, version, id);
        mInputDevices.append(inputDevice);
#if QT_CONFIG(wayland_datadevice)
    } else if (interface == QStringLiteral("wl_data_device_manager")) {
        mDndSelectionHandler.reset(new QWaylandDataDeviceManager(this, version, id));
#endif
    } else if (interface == QStringLiteral("qt_surface_extension")) {
        mWindowExtension.reset(new QtWayland::qt_surface_extension(registry, id, 1));
    } else if (interface == QStringLiteral("wl_subcompositor")) {
        mSubCompositor.reset(new QtWayland::wl_subcompositor(registry, id, 1));
    } else if (interface == QStringLiteral("qt_touch_extension")) {
        mTouchExtension.reset(new QWaylandTouchExtension(this, id));
    } else if (interface == QStringLiteral("zqt_key_v1")) {
        mQtKeyExtension.reset(new QWaylandQtKeyExtension(this, id));
    } else if (interface == QStringLiteral("zwp_tablet_manager_v2")) {
        mTabletManager.reset(new QWaylandTabletManagerV2(this, id, qMin(1, int(version))));
#if QT_CONFIG(wayland_client_primary_selection)
    } else if (interface == QStringLiteral("zwp_primary_selection_device_manager_v1")) {
        mPrimarySelectionManager.reset(new QWaylandPrimarySelectionDeviceManagerV1(this, id, 1));
        for (QWaylandInputDevice *inputDevice : qAsConst(mInputDevices))
            inputDevice->setPrimarySelectionDevice(mPrimarySelectionManager->createDevice(inputDevice));
#endif
    } else if (interface == QStringLiteral("zwp_text_input_manager_v2") && !mClientSideInputContextRequested) {
        mTextInputManager.reset(new QtWayland::zwp_text_input_manager_v2(registry, id, 1));
        for (QWaylandInputDevice *inputDevice : qAsConst(mInputDevices))
            inputDevice->setTextInput(new QWaylandTextInput(this, mTextInputManager->get_text_input(inputDevice->wl_seat())));
        mWaylandIntegration->reconfigureInputContext();
    } else if (interface == QStringLiteral("qt_hardware_integration")) {
        bool disableHardwareIntegration = qEnvironmentVariableIntValue("QT_WAYLAND_DISABLE_HW_INTEGRATION");
        if (!disableHardwareIntegration) {
            mHardwareIntegration.reset(new QWaylandHardwareIntegration(registry, id));
            // make a roundtrip here since we need to receive the events sent by
            // qt_hardware_integration before creating windows
            forceRoundTrip();
        }
    } else if (interface == QLatin1String("zxdg_output_manager_v1")) {
        mXdgOutputManager.reset(new QWaylandXdgOutputManagerV1(this, id, version));
        for (auto *screen : qAsConst(mWaitingScreens))
            screen->initXdgOutput(xdgOutputManager());
        forceRoundTrip();
    }

    mGlobals.append(RegistryGlobal(id, interface, version, registry));

    const auto copy = mRegistryListeners; // be prepared for listeners unregistering on notification
    for (Listener l : copy)
        (*l.listener)(l.data, registry, id, interface, version);
}

void QWaylandDisplay::registry_global_remove(uint32_t id)
{
    for (int i = 0, ie = mGlobals.count(); i != ie; ++i) {
        RegistryGlobal &global = mGlobals[i];
        if (global.id == id) {
            if (global.interface == QStringLiteral("wl_output")) {
                for (auto *screen : mWaitingScreens) {
                    if (screen->outputId() == id) {
                        mWaitingScreens.removeOne(screen);
                        delete screen;
                        break;
                    }
                }

                for (QWaylandScreen *screen : qAsConst(mScreens)) {
                    if (screen->outputId() == id) {
                        mScreens.removeOne(screen);
                        // If this is the last screen, we have to add a fake screen, or Qt will break.
                        ensureScreen();
                        QWindowSystemInterface::handleScreenRemoved(screen);
                        break;
                    }
                }
            }
            if (global.interface == QStringLiteral("zwp_text_input_manager_v2")) {
                mTextInputManager.reset();
                for (QWaylandInputDevice *inputDevice : qAsConst(mInputDevices))
                    inputDevice->setTextInput(nullptr);
                mWaylandIntegration->reconfigureInputContext();
            }
#if QT_CONFIG(wayland_client_primary_selection)
            if (global.interface == QStringLiteral("zwp_primary_selection_device_manager_v1")) {
                mPrimarySelectionManager.reset();
                for (QWaylandInputDevice *inputDevice : qAsConst(mInputDevices))
                    inputDevice->setPrimarySelectionDevice(nullptr);
            }
#endif
            mGlobals.removeAt(i);
            break;
        }
    }
}

bool QWaylandDisplay::hasRegistryGlobal(QStringView interfaceName) const
{
    for (const RegistryGlobal &global : mGlobals)
        if (global.interface == interfaceName)
            return true;

    return false;
}

void QWaylandDisplay::addRegistryListener(RegistryListener listener, void *data)
{
    Listener l = { listener, data };
    mRegistryListeners.append(l);
    for (int i = 0, ie = mGlobals.count(); i != ie; ++i)
        (*l.listener)(l.data, mGlobals[i].registry, mGlobals[i].id, mGlobals[i].interface, mGlobals[i].version);
}

void QWaylandDisplay::removeListener(RegistryListener listener, void *data)
{
    auto iter = std::remove_if(mRegistryListeners.begin(), mRegistryListeners.end(), [=](Listener l){
        return (l.listener == listener && l.data == data);
    });
    mRegistryListeners.erase(iter, mRegistryListeners.end());
}

uint32_t QWaylandDisplay::currentTimeMillisec()
{
    //### we throw away the time information
    struct timeval tv;
    int ret = gettimeofday(&tv, nullptr);
    if (ret == 0)
        return tv.tv_sec*1000 + tv.tv_usec/1000;
    return 0;
}

void QWaylandDisplay::forceRoundTrip()
{
     wl_display_roundtrip(mDisplay);
}

bool QWaylandDisplay::supportsWindowDecoration() const
{
    static bool disabled = qgetenv("QT_WAYLAND_DISABLE_WINDOWDECORATION").toInt();
    // Stop early when disabled via the environment. Do not try to load the integration in
    // order to play nice with SHM-only, buffer integration-less systems.
    if (disabled)
        return false;

    static bool integrationSupport = clientBufferIntegration() && clientBufferIntegration()->supportsWindowDecoration();
    return integrationSupport;
}

QWaylandWindow *QWaylandDisplay::lastInputWindow() const
{
    return mLastInputWindow.data();
}

void QWaylandDisplay::setLastInputDevice(QWaylandInputDevice *device, uint32_t serial, QWaylandWindow *win)
{
    mLastInputDevice = device;
    mLastInputSerial = serial;
    mLastInputWindow = win;
}

bool QWaylandDisplay::isWindowActivated(const QWaylandWindow *window)
{
    return mActiveWindows.contains(const_cast<QWaylandWindow *>(window));
}

void QWaylandDisplay::handleWindowActivated(QWaylandWindow *window)
{
    if (mActiveWindows.contains(window))
        return;

    mActiveWindows.append(window);
    requestWaylandSync();

    if (auto *decoration = window->decoration())
        decoration->update();
}

void QWaylandDisplay::handleWindowDeactivated(QWaylandWindow *window)
{
    Q_ASSERT(!mActiveWindows.empty());

    if (mActiveWindows.last() == window)
        requestWaylandSync();

    mActiveWindows.removeOne(window);

    if (auto *decoration = window->decoration())
        decoration->update();
}

void QWaylandDisplay::handleKeyboardFocusChanged(QWaylandInputDevice *inputDevice)
{
    QWaylandWindow *keyboardFocus = inputDevice->keyboardFocus();

    if (mLastKeyboardFocus == keyboardFocus)
        return;

    if (keyboardFocus)
        handleWindowActivated(keyboardFocus);
    if (mLastKeyboardFocus)
        handleWindowDeactivated(mLastKeyboardFocus);

    mLastKeyboardFocus = keyboardFocus;
}

void QWaylandDisplay::handleWindowDestroyed(QWaylandWindow *window)
{
    if (mActiveWindows.contains(window))
        handleWindowDeactivated(window);
}

void QWaylandDisplay::handleWaylandSync()
{
    // This callback is used to set the window activation because we may get an activate/deactivate
    // pair, and the latter one would be lost in the QWindowSystemInterface queue, if we issue the
    // handleWindowActivated() calls immediately.
    QWindow *activeWindow = mActiveWindows.empty() ? nullptr : mActiveWindows.last()->window();
    if (activeWindow != QGuiApplication::focusWindow())
        QWindowSystemInterface::handleWindowActivated(activeWindow);

    if (!activeWindow) {
        if (lastInputDevice()) {
#if QT_CONFIG(clipboard)
            if (auto *dataDevice = lastInputDevice()->dataDevice())
                dataDevice->invalidateSelectionOffer();
#endif
#if QT_CONFIG(wayland_client_primary_selection)
            if (auto *device = lastInputDevice()->primarySelectionDevice())
                device->invalidateSelectionOffer();
#endif
        }
    }
}

const wl_callback_listener QWaylandDisplay::syncCallbackListener = {
    [](void *data, struct wl_callback *callback, uint32_t time){
        Q_UNUSED(time);
        wl_callback_destroy(callback);
        QWaylandDisplay *display = static_cast<QWaylandDisplay *>(data);
        display->mSyncCallback = nullptr;
        display->handleWaylandSync();
    }
};

void QWaylandDisplay::requestWaylandSync()
{
    if (mSyncCallback)
        return;

    mSyncCallback = wl_display_sync(mDisplay);
    wl_callback_add_listener(mSyncCallback, &syncCallbackListener, this);
}

QWaylandInputDevice *QWaylandDisplay::defaultInputDevice() const
{
    return mInputDevices.isEmpty() ? 0 : mInputDevices.first();
}

bool QWaylandDisplay::isKeyboardAvailable() const
{
    return std::any_of(
            mInputDevices.constBegin(), mInputDevices.constEnd(),
            [this](const QWaylandInputDevice *device) { return device->keyboard() != nullptr; });
}

#if QT_CONFIG(cursor)

QWaylandCursor *QWaylandDisplay::waylandCursor()
{
    if (!mCursor)
        mCursor.reset(new QWaylandCursor(this));
    return mCursor.data();
}

QWaylandCursorTheme *QWaylandDisplay::loadCursorTheme(const QString &name, int pixelSize)
{
    if (auto *theme = mCursorThemes.value({name, pixelSize}, nullptr))
        return theme;

    if (auto *theme = QWaylandCursorTheme::create(shm(), pixelSize, name)) {
        mCursorThemes[{name, pixelSize}] = theme;
        return theme;
    }

    return nullptr;
}

#endif // QT_CONFIG(cursor)

} // namespace QtWaylandClient

#include "qwaylanddisplay.moc"

QT_END_NAMESPACE
