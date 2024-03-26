// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mockclient.h"
#include "mockseat.h"
#include "mockpointer.h"
#include "mockxdgoutputv1.h"
#include "testcompositor.h"
#include "testkeyboardgrabber.h"
#include "testseat.h"

#include "qwaylandview.h"
#include "qwaylandbufferref.h"
#include "qwaylandseat.h"

#include <QtGui/QScreen>
#include <QtWaylandCompositor/QWaylandXdgShell>
#include <QtWaylandCompositor/private/qwaylandkeyboard_p.h>
#include <QtWaylandCompositor/QWaylandIviApplication>
#include <QtWaylandCompositor/QWaylandIviSurface>
#include <QtWaylandCompositor/QWaylandSurface>
#include <QtWaylandCompositor/QWaylandResource>
#include <QtWaylandCompositor/QWaylandKeymap>
#include <QtWaylandCompositor/QWaylandViewporter>
#include <QtWaylandCompositor/QWaylandIdleInhibitManagerV1>
#include <QtWaylandCompositor/QWaylandXdgOutputManagerV1>
#include <qwayland-xdg-shell.h>
#include <qwayland-ivi-application.h>
#include <QtWaylandCompositor/private/qwaylandoutput_p.h>
#include <QtWaylandCompositor/private/qwaylandsurface_p.h>

#include <QtTest/QtTest>

class tst_WaylandCompositor : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void seatCapabilities();
#if QT_CONFIG(xkbcommon)
    void simpleKeyboard();
    void keyboardKeymaps();
    void keyboardLayoutSwitching();
#endif
    void keyboardGrab();
    void seatCreation();
    void seatKeyboardFocus();
    void seatMouseFocus();
    void inputRegion();
    void defaultInputRegionHiDpi();
    void singleClient();
    void multipleClients();
    void geometry();
    void availableGeometry();
    void modes();
    void comparingModes();
    void sizeFollowsWindow();
    void mapSurface();
    void mapSurfaceHiDpi();
    void frameCallback();
    void pixelFormats();
    void outputs();
    void customSurface();

    void advertisesXdgShellSupport();
    void createsXdgSurfaces();
    void reportsXdgSurfaceWindowGeometry();
    void setsXdgAppId();
    void sendsXdgConfigure();

    void advertisesIviApplicationSupport();
    void createsIviSurfaces();
    void emitsErrorOnSameIviId();
    void sendsIviConfigure();
    void destroysIviSurfaces();

    void viewporterGlobal();
    void viewportDestination();
    void viewportSource();
    void viewportSourceAndDestination();
    void viewportDestruction();
    void viewportProtocolErrors_data();
    void viewportProtocolErrors();
    void viewportClearDestination();
    void viewportClearSource();
    void viewportExistsError();
    void viewportDestinationNoSurfaceError();
    void viewportSourceNoSurfaceError();
    void viewportHiDpi();

    void idleInhibit();

    void xdgOutput();

private:
    QTemporaryDir m_tmpRuntimeDir;
};

void tst_WaylandCompositor::init() {
    // We need to set a test specific runtime dir so we don't conflict with other tests'
    // compositors by accident.
    qputenv("XDG_RUNTIME_DIR", m_tmpRuntimeDir.path().toLocal8Bit());
}

void tst_WaylandCompositor::singleClient()
{
    TestCompositor compositor;
    compositor.create();

    MockClient client;

    wl_surface *sa = client.createSurface();
    QTRY_COMPARE(compositor.surfaces.size(), 1);

    wl_surface *sb = client.createSurface();
    QTRY_COMPARE(compositor.surfaces.size(), 2);

    QWaylandClient *ca = compositor.surfaces.at(0)->client();
    QWaylandClient *cb = compositor.surfaces.at(1)->client();

    QCOMPARE(ca, cb);
    QVERIFY(ca != nullptr);

    QList<QWaylandSurface *> surfaces = compositor.surfacesForClient(ca);
    QCOMPARE(surfaces.size(), 2);
    QVERIFY((surfaces.at(0) == compositor.surfaces.at(0) && surfaces.at(1) == compositor.surfaces.at(1))
            || (surfaces.at(0) == compositor.surfaces.at(1) && surfaces.at(1) == compositor.surfaces.at(0)));

    wl_surface_destroy(sa);
    QTRY_COMPARE(compositor.surfaces.size(), 1);

    wl_surface_destroy(sb);
    QTRY_COMPARE(compositor.surfaces.size(), 0);
}

void tst_WaylandCompositor::multipleClients()
{
    TestCompositor compositor;
    compositor.create();

    MockClient a;
    MockClient b;
    MockClient c;

    wl_surface *sa = a.createSurface();
    wl_surface *sb = b.createSurface();
    wl_surface *sc = c.createSurface();

    QTRY_COMPARE(compositor.surfaces.size(), 3);

    QWaylandClient *ca = compositor.surfaces.at(0)->client();
    QWaylandClient *cb = compositor.surfaces.at(1)->client();
    QWaylandClient *cc = compositor.surfaces.at(2)->client();

    QVERIFY(ca != cb);
    QVERIFY(ca != cc);
    QVERIFY(cb != cc);
    QVERIFY(ca != nullptr);

    QCOMPARE(compositor.surfacesForClient(ca).size(), 1);
    QCOMPARE(compositor.surfacesForClient(ca).at(0), compositor.surfaces.at(0));

    QCOMPARE(compositor.surfacesForClient(cb).size(), 1);
    QCOMPARE(compositor.surfacesForClient(cb).at(0), compositor.surfaces.at(1));

    QCOMPARE(compositor.surfacesForClient(cc).size(), 1);
    QCOMPARE(compositor.surfacesForClient(cc).at(0), compositor.surfaces.at(2));

    wl_surface_destroy(sa);
    wl_surface_destroy(sb);
    wl_surface_destroy(sc);

    QTRY_COMPARE(compositor.surfaces.size(), 0);
}

#if QT_CONFIG(xkbcommon)

void tst_WaylandCompositor::simpleKeyboard()
{
    TestCompositor compositor;
    compositor.create();

    QWaylandSeat* seat = compositor.defaultSeat();
    seat->keymap()->setLayout("us");

    MockClient client;

    QTRY_COMPARE(client.m_seats.size(), 1);
    MockKeyboard *mockKeyboard = client.m_seats.at(0)->keyboard();

    wl_surface *mockSurface = client.createSurface();
    QTRY_COMPARE(compositor.surfaces.size(), 1);
    seat->setKeyboardFocus(compositor.surfaces.at(0));

    compositor.flushClients();
    QTRY_COMPARE(mockKeyboard->m_enteredSurface, mockSurface);

    seat->sendKeyEvent(Qt::Key_A, true);
    compositor.flushClients();
    QTRY_COMPARE(mockKeyboard->m_lastKeyState, 1u);
    QTRY_COMPARE(mockKeyboard->m_lastKeyCode, 30u); // 30 is the scan code for A on us keyboard layouts

    seat->sendKeyEvent(Qt::Key_A, false);
    compositor.flushClients();
    QTRY_COMPARE(mockKeyboard->m_lastKeyState, 0u);
    QTRY_COMPARE(mockKeyboard->m_lastKeyCode, 30u);

    seat->sendKeyEvent(Qt::Key_Super_L, true);
    seat->sendKeyEvent(Qt::Key_Super_L, false);
    compositor.flushClients();
    QTRY_COMPARE(mockKeyboard->m_lastKeyCode, 125u);
}

void tst_WaylandCompositor::keyboardKeymaps()
{
    TestCompositor compositor;
    compositor.create();
    QWaylandSeat* seat = compositor.defaultSeat();
    MockClient client;
    QTRY_COMPARE(client.m_seats.size(), 1);
    MockKeyboard *mockKeyboard = client.m_seats.at(0)->keyboard();
    client.createSurface();
    QTRY_COMPARE(compositor.surfaces.size(), 1);
    seat->setKeyboardFocus(compositor.surfaces.at(0));

    seat->keymap()->setLayout("us");

    seat->sendKeyEvent(Qt::Key_Y, true);
    seat->sendKeyEvent(Qt::Key_Y, false);
    compositor.flushClients();
    QTRY_COMPARE(mockKeyboard->m_lastKeyCode, 21u);

    seat->sendKeyEvent(Qt::Key_Z, true);
    seat->sendKeyEvent(Qt::Key_Z, false);
    compositor.flushClients();
    QTRY_COMPARE(mockKeyboard->m_lastKeyCode, 44u);

    seat->keymap()->setLayout("de"); // In the German layout y and z have changed places

    seat->sendKeyEvent(Qt::Key_Y, true);
    seat->sendKeyEvent(Qt::Key_Y, false);
    compositor.flushClients();
    QTRY_COMPARE(mockKeyboard->m_lastKeyCode, 44u);

    seat->sendKeyEvent(Qt::Key_Z, true);
    seat->sendKeyEvent(Qt::Key_Z, false);
    compositor.flushClients();
    QTRY_COMPARE(mockKeyboard->m_lastKeyCode, 21u);
}

void tst_WaylandCompositor::keyboardLayoutSwitching()
{
    TestCompositor compositor;
    compositor.create();
    QWaylandSeat* seat = compositor.defaultSeat();
    MockClient client;
    QTRY_COMPARE(client.m_seats.size(), 1);
    MockKeyboard *mockKeyboard = client.m_seats.at(0)->keyboard();
    client.createSurface();
    QTRY_COMPARE(compositor.surfaces.size(), 1);
    seat->setKeyboardFocus(compositor.surfaces.at(0));

    seat->keymap()->setLayout("us,de");
    seat->keymap()->setOptions("grp:lalt_toggle"); //toggle keyboard layout with left alt

    compositor.flushClients();
    QTRY_COMPARE(mockKeyboard->m_group, 0u);

    seat->sendKeyEvent(Qt::Key_Y, true);
    seat->sendKeyEvent(Qt::Key_Y, false);
    compositor.flushClients();
    QTRY_COMPARE(mockKeyboard->m_lastKeyCode, 21u);

    // It's not currently possible to switch layouts programmatically with the public APIs
    // We will just fake it with the private APIs here.
    auto keyboardPrivate = QWaylandKeyboardPrivate::get(seat->keyboard());
    const uint leftAltCode = 64;
    keyboardPrivate->updateModifierState(leftAltCode, WL_KEYBOARD_KEY_STATE_PRESSED);
    keyboardPrivate->updateModifierState(leftAltCode, WL_KEYBOARD_KEY_STATE_RELEASED);
    compositor.flushClients();
    QTRY_COMPARE(mockKeyboard->m_group, 1u);

    seat->sendKeyEvent(Qt::Key_Y, true);
    seat->sendKeyEvent(Qt::Key_Y, false);
    compositor.flushClients();
    QTRY_COMPARE(mockKeyboard->m_lastKeyCode, 44u);
}

#endif // QT_CONFIG(xkbcommon)

void tst_WaylandCompositor::keyboardGrab()
{
    TestCompositor compositor;
    compositor.create();
    MockClient mc;

    mc.createSurface();
    // This is needed for timing purposes, otherwise the query for the
    // compositor surfaces will return null
    QTRY_COMPARE(compositor.surfaces.size(), 1);

    // Set the focused surface so that key event will flow through
    QWaylandSurface *waylandSurface = compositor.surfaces.at(0);
    QWaylandSeat* seat = compositor.defaultSeat();

    TestKeyboardGrabber* grab = static_cast<TestKeyboardGrabber *>(seat->keyboard());
    QTRY_COMPARE(grab, seat->keyboard());
    QSignalSpy grabFocusSpy(grab, SIGNAL(focusedCalled()));
    QSignalSpy grabKeyPressSpy(grab, SIGNAL(keyPressCalled()));
    QSignalSpy grabKeyReleaseSpy(grab, SIGNAL(keyReleaseCalled()));
    //QSignalSpy grabModifierSpy(grab, SIGNAL(modifiersCalled()));

    seat->setKeyboardFocus(waylandSurface);
    QTRY_COMPARE(grabFocusSpy.size(), 1);

    QKeyEvent ke(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, 30, 0, 0);
    QKeyEvent ke1(QEvent::KeyRelease, Qt::Key_A, Qt::NoModifier, 30, 0, 0);
    seat->sendFullKeyEvent(&ke);
    seat->sendFullKeyEvent(&ke1);
    QTRY_COMPARE(grabKeyPressSpy.size(), 1);
    QTRY_COMPARE(grabKeyReleaseSpy.size(), 1);

    QKeyEvent ke2(QEvent::KeyPress, Qt::Key_Shift, Qt::NoModifier, 50, 0, 0);
    QKeyEvent ke3(QEvent::KeyRelease, Qt::Key_Shift, Qt::NoModifier, 50, 0, 0);
    seat->sendFullKeyEvent(&ke2);
    seat->sendFullKeyEvent(&ke3);
    //QTRY_COMPARE(grabModifierSpy.count(), 2);
    // Modifiers are also keys
    QTRY_COMPARE(grabKeyPressSpy.size(), 2);
    QTRY_COMPARE(grabKeyReleaseSpy.size(), 2);

    // Stop grabbing
    seat->setKeyboardFocus(nullptr);
    seat->sendFullKeyEvent(&ke);
    seat->sendFullKeyEvent(&ke1);
    QTRY_COMPARE(grabKeyPressSpy.size(), 2);
}

void tst_WaylandCompositor::geometry()
{
    TestCompositor compositor;
    compositor.create();

    QWaylandOutputMode mode(QSize(4096, 3072), 60000);
    compositor.defaultOutput()->setPosition(QPoint(1024, 0));
    compositor.defaultOutput()->addMode(mode, true);
    compositor.defaultOutput()->setCurrentMode(mode);

    MockClient client;

    QTRY_COMPARE(client.geometry, QRect(QPoint(1024, 0), QSize(4096, 3072)));
    QTRY_COMPARE(client.resolution, QSize(4096, 3072));
    QTRY_COMPARE(client.refreshRate, 60000);
}

void tst_WaylandCompositor::availableGeometry()
{
    TestCompositor compositor;
    compositor.create();

    QWaylandOutputMode mode(QSize(1024, 768), 60000);
    compositor.defaultOutput()->addMode(mode, true);
    compositor.defaultOutput()->setCurrentMode(mode);

    MockClient client;

    QRect availableGeometry(50, 100, 850, 600);
    compositor.defaultOutput()->setAvailableGeometry(availableGeometry);
    QCOMPARE(compositor.defaultOutput()->availableGeometry(), availableGeometry);
}

void tst_WaylandCompositor::modes()
{
    TestCompositor compositor;
    compositor.create();

    // mode3 is current, mode4 is preferred
    QWaylandOutputMode mode1(QSize(800, 600), 120000);
    QWaylandOutputMode mode2(QSize(1024, 768), 100000);
    QWaylandOutputMode mode3(QSize(1920, 1080), 60000);
    QWaylandOutputMode mode4(QSize(2560, 1440), 59000);
    compositor.defaultOutput()->addMode(mode1);
    compositor.defaultOutput()->addMode(mode2, true);
    compositor.defaultOutput()->addMode(mode3);
    compositor.defaultOutput()->addMode(mode4, true);
    compositor.defaultOutput()->setCurrentMode(mode3);

    MockClient client;

    QTRY_COMPARE(client.modes.size(), 4);
    QTRY_COMPARE(client.currentMode, mode3);
    QTRY_COMPARE(client.preferredMode, mode4);
    QTRY_COMPARE(client.geometry, QRect(QPoint(0, 0), QSize(1920, 1080)));
}

void tst_WaylandCompositor::comparingModes()
{
    QWaylandOutputMode mode1(QSize(800, 600), 120000);
    QWaylandOutputMode mode2(QSize(1024, 768), 100000);
    QWaylandOutputMode mode3(QSize(1024, 768), 120000);
    QWaylandOutputMode mode4(QSize(800, 600), 100000);

    QCOMPARE(mode1, mode1);
    QCOMPARE(mode2, mode2);
    QCOMPARE(mode3, mode3);
    QCOMPARE(mode4, mode4);

    for (auto mode: {mode2, mode3, mode4})
       QVERIFY(mode1 != mode);
    for (auto mode: {mode1, mode3, mode4})
       QVERIFY(mode2 != mode);
    for (auto mode: {mode1, mode2, mode4})
       QVERIFY(mode3 != mode);
    for (auto mode: {mode1, mode2, mode3})
       QVERIFY(mode4 != mode);
}

void tst_WaylandCompositor::sizeFollowsWindow()
{
    TestCompositor compositor;

    QWindow window;
    window.resize(800, 600);

    auto output = new QWaylandOutput(&compositor, &window);
    output->setSizeFollowsWindow(true);

    compositor.create();

    // window.size() is not in pixels
    auto pixelSize = window.size() * window.devicePixelRatio();
    QWaylandOutputMode mode(pixelSize, qFloor(window.screen()->refreshRate() * 1000));

    MockClient client;

    QTRY_COMPARE(client.modes.size(), 1);
    QTRY_COMPARE(client.currentMode, mode);
    QTRY_COMPARE(client.preferredMode, mode);
}

void tst_WaylandCompositor::mapSurface()
{
    TestCompositor compositor;
    compositor.create();

    MockClient client;

    wl_surface *surface = client.createSurface();
    QTRY_COMPARE(compositor.surfaces.size(), 1);

    QWaylandSurface *waylandSurface = compositor.surfaces.at(0);

    QSignalSpy hasContentSpy(waylandSurface, SIGNAL(hasContentChanged()));

    QCOMPARE(waylandSurface->bufferSize(), QSize());
    QCOMPARE(waylandSurface->destinationSize(), QSize());
    QCOMPARE(waylandSurface->hasContent(), false);

    QSize size(256, 256);
    ShmBuffer buffer(size, client.shm);

    // we need to create a shell surface here or the surface won't be mapped
    client.createShellSurface(surface);
    wl_surface_attach(surface, buffer.handle, 0, 0);
    wl_surface_damage(surface, 0, 0, size.width(), size.height());
    wl_surface_commit(surface);

    QTRY_COMPARE(hasContentSpy.size(), 1);
    QCOMPARE(waylandSurface->hasContent(), true);
    QCOMPARE(waylandSurface->bufferSize(), size);
    QCOMPARE(waylandSurface->destinationSize(), size);

    wl_surface_destroy(surface);
}

void tst_WaylandCompositor::mapSurfaceHiDpi()
{
    TestCompositor compositor;
    compositor.create();

    MockClient client;

    wl_surface *surface = client.createSurface();
    QTRY_COMPARE(compositor.surfaces.size(), 1);

    QWaylandSurface *waylandSurface = compositor.surfaces.at(0);

    constexpr int bufferScale = 2;
    const QSize surfaceSize(128, 128);
    const QSize bufferSize = surfaceSize * bufferScale;
    const QPoint attachOffset(1, 2); // in surface-local coordinates

    client.createShellSurface(surface);
    ShmBuffer buffer(bufferSize, client.shm);
    wl_surface_attach(surface, buffer.handle, attachOffset.x(), attachOffset.y());
    wl_surface_set_buffer_scale(surface, bufferScale);
    // wl_surface_damage is given in surface coordinates
    wl_surface_damage(surface, 0, 0, surfaceSize.width(), surfaceSize.height());

    auto verifyComittedState = [=]() {
        QCOMPARE(waylandSurface->bufferSize(), bufferSize);
        QCOMPARE(waylandSurface->destinationSize(), surfaceSize);
        QCOMPARE(waylandSurface->bufferScale(), bufferScale);
        QCOMPARE(waylandSurface->hasContent(), true);
    };

    QObject::connect(waylandSurface, &QWaylandSurface::damaged, [=] (const QRegion &damage) {
        QCOMPARE(damage, QRect(QPoint(), surfaceSize));
        verifyComittedState();
    });
    QSignalSpy damagedSpy(waylandSurface, SIGNAL(damaged(const QRegion &)));

    QObject::connect(waylandSurface, &QWaylandSurface::hasContentChanged, verifyComittedState);
    QSignalSpy hasContentSpy(waylandSurface, SIGNAL(hasContentChanged()));

    QObject::connect(waylandSurface, &QWaylandSurface::bufferSizeChanged, verifyComittedState);
    QSignalSpy bufferSizeSpy(waylandSurface, SIGNAL(bufferSizeChanged()));

    QObject::connect(waylandSurface, &QWaylandSurface::destinationSizeChanged, verifyComittedState);
    QSignalSpy destinationSizeSpy(waylandSurface, SIGNAL(destinationSizeChanged()));

    QObject::connect(waylandSurface, &QWaylandSurface::bufferScaleChanged, verifyComittedState);
    QSignalSpy bufferScaleSpy(waylandSurface, SIGNAL(bufferScaleChanged()));

    QObject::connect(waylandSurface, &QWaylandSurface::offsetForNextFrame, [=](const QPoint &offset) {
        QCOMPARE(offset, attachOffset);
        verifyComittedState();
    });
    QSignalSpy offsetSpy(waylandSurface, SIGNAL(offsetForNextFrame(const QPoint &)));

    // No state should be applied before the commit
    QCOMPARE(waylandSurface->bufferSize(), QSize());
    QCOMPARE(waylandSurface->destinationSize(), QSize());
    QCOMPARE(waylandSurface->hasContent(), false);
    QCOMPARE(waylandSurface->bufferScale(), 1);
    QCOMPARE(offsetSpy.size(), 0);

    wl_surface_commit(surface);

    QTRY_COMPARE(hasContentSpy.size(), 1);
    QTRY_COMPARE(bufferSizeSpy.size(), 1);
    QTRY_COMPARE(destinationSizeSpy.size(), 1);
    QTRY_COMPARE(bufferScaleSpy.size(), 1);
    QTRY_COMPARE(offsetSpy.size(), 1);
    QTRY_COMPARE(damagedSpy.size(), 1);

    // Now verify that wl_surface_damage_buffer gets mapped properly
    wl_surface_damage_buffer(surface, 0, 0, bufferSize.width(), bufferSize.height());
    wl_surface_commit(surface);
    QTRY_COMPARE(damagedSpy.size(), 2);

    wl_surface_destroy(surface);
}

static void frameCallbackFunc(void *data, wl_callback *callback, uint32_t)
{
    ++*static_cast<int *>(data);
    wl_callback_destroy(callback);
}

static void registerFrameCallback(wl_surface *surface, int *counter)
{
    static const wl_callback_listener frameCallbackListener = {
        frameCallbackFunc
    };

    wl_callback_add_listener(wl_surface_frame(surface), &frameCallbackListener, counter);
}

class BufferView : public QWaylandView
{
public:
    void bufferCommitted(const QWaylandBufferRef &ref, const QRegion &damage) override
    {
        Q_UNUSED(damage);
        bufferRef = ref;
    }

    QImage image() const
    {
        if (bufferRef.isNull() || !bufferRef.isSharedMemory())
            return QImage();
        return bufferRef.image();
    }

    QWaylandBufferRef bufferRef;
};

void tst_WaylandCompositor::frameCallback()
{
    TestCompositor compositor;
    compositor.create();

    MockClient client;

    wl_surface *surface = client.createSurface();

    int frameCounter = 0;

    QTRY_COMPARE(compositor.surfaces.size(), 1);
    QWaylandSurface *waylandSurface = compositor.surfaces.at(0);
    BufferView* view = new BufferView;
    view->setSurface(waylandSurface);
    view->setOutput(compositor.defaultOutput());

    QSignalSpy damagedSpy(waylandSurface, SIGNAL(damaged(const QRegion &)));

    for (int i = 0; i < 10; ++i) {
        QSize size(i * 8 + 2, i * 8 + 2);
        ShmBuffer buffer(size, client.shm);

        // attach a new buffer every frame, else the damage signal won't be fired
        wl_surface_attach(surface, buffer.handle, 0, 0);
        registerFrameCallback(surface, &frameCounter);
        wl_surface_damage(surface, 0, 0, size.width(), size.height());
        wl_surface_commit(surface);

        QTRY_COMPARE(waylandSurface->hasContent(), true);
        QTRY_COMPARE(damagedSpy.size(), i + 1);

        QCOMPARE(static_cast<BufferView*>(waylandSurface->views().first())->image(), buffer.image);
        compositor.defaultOutput()->frameStarted();
        compositor.defaultOutput()->sendFrameCallbacks();

        QTRY_COMPARE(frameCounter, i + 1);
    }

    wl_surface_destroy(surface);
}

void tst_WaylandCompositor::pixelFormats()
{
    TestCompositor compositor;
    compositor.create();

    MockClient client;

    wl_surface *surface = client.createSurface();
    QTRY_COMPARE(compositor.surfaces.size(), 1);
    QWaylandSurface *waylandSurface = compositor.surfaces.at(0);
    BufferView* view = new BufferView;
    view->setSurface(waylandSurface);
    view->setOutput(compositor.defaultOutput());

    QSize size(32, 32);
    ShmBuffer buffer(size, client.shm); // Will be WL_SHM_FORMAT_ARGB8888;
    wl_surface_attach(surface, buffer.handle, 0, 0);
    wl_surface_damage(surface, 0, 0, size.width(), size.height());
    wl_surface_commit(surface);

    QTRY_COMPARE(waylandSurface->hasContent(), true);

    // According to https://lists.freedesktop.org/archives/wayland-devel/2017-August/034791.html
    // all RGB formats with alpha are premultiplied. Verify it here:
    QCOMPARE(view->image().format(), QImage::Format_ARGB32_Premultiplied);

    wl_surface_destroy(surface);
}

void tst_WaylandCompositor::outputs()
{
    TestCompositor compositor;

    QSignalSpy defaultOutputSpy(&compositor, SIGNAL(defaultOutputChanged()));

    compositor.create();

    QSignalSpy outputAddedSpy(&compositor, SIGNAL(outputAdded(QWaylandOutput*)));
    QSignalSpy outputRemovedSpy(&compositor, SIGNAL(outputRemoved(QWaylandOutput*)));

    QWindow window;
    window.resize(800, 600);

    auto output = new QWaylandOutput(&compositor, &window);
    QTRY_COMPARE(outputAddedSpy.size(), 1);

    compositor.setDefaultOutput(output);
    QTRY_COMPARE(defaultOutputSpy.size(), 2);

    MockClient client;
    QTRY_COMPARE(client.m_outputs.size(), 2);

    delete output;
    QTRY_COMPARE(outputRemovedSpy.size(), 1);
    QEXPECT_FAIL("", "FIXME: defaultOutputChanged() is not emitted when the default output is removed", Continue);
    QTRY_COMPARE(defaultOutputSpy.size(), 3);
    compositor.flushClients();
    QTRY_COMPARE(client.m_outputs.size(), 1);
}

class CustomSurface : public QWaylandSurface {
    Q_OBJECT
public:
    explicit CustomSurface() = default;
};

void tst_WaylandCompositor::customSurface()
{
    TestCompositor compositor;
    QObject::connect(&compositor, &TestCompositor::surfaceRequested, this, [&compositor] (QWaylandClient *client, uint id, int version) {
        auto *s = new CustomSurface();
        QCOMPARE(s->waylandClient(), nullptr);
        s->initialize(&compositor, client, id, version);
        QCOMPARE(s->waylandClient(), client->client());
    });
    QObject::connect(&compositor, &TestCompositor::surfaceCreated, this, [] (QWaylandSurface *surface) {
        auto *custom = qobject_cast<CustomSurface *>(surface);
        QVERIFY(custom != nullptr);
    });
    compositor.create();

    MockClient client;
    wl_surface *surface = client.createSurface();
    QTRY_COMPARE(compositor.surfaces.size(), 1);
    wl_surface_destroy(surface);
    QTRY_COMPARE(compositor.surfaces.size(), 0);
}

void tst_WaylandCompositor::seatCapabilities()
{
    TestCompositor compositor;
    compositor.create();

    MockClient client;
    Q_UNUSED(client);

    QWaylandSeat dev(&compositor, QWaylandSeat::Pointer);

    QTRY_VERIFY(dev.pointer());
    QTRY_VERIFY(!dev.keyboard());
    QTRY_VERIFY(!dev.touch());

    QWaylandSeat dev2(&compositor, QWaylandSeat::Keyboard | QWaylandSeat::Touch);
    QTRY_VERIFY(!dev2.pointer());
    QTRY_VERIFY(dev2.keyboard());
    QTRY_VERIFY(dev2.touch());
}

void tst_WaylandCompositor::seatCreation()
{
    TestCompositor compositor(true);
    compositor.create();

    MockClient client;
    Q_UNUSED(client);

    TestSeat* seat = qobject_cast<TestSeat *>(compositor.defaultSeat());
    QTRY_VERIFY(seat);

    // The compositor will create the default input device
    QTRY_VERIFY(seat->isInitialized());

    const QList<QMouseEvent *> allEvents = seat->createMouseEvents(5);
    for (QMouseEvent *me : allEvents) {
        compositor.seatFor(me);
    }

    // The default input device will get called exatly the number of times it has created
    // the events
    QTRY_COMPARE(seat->queryCount(), 5);
}

void tst_WaylandCompositor::seatKeyboardFocus()
{
    TestCompositor compositor(true);
    compositor.create();

    // Create client after all the input devices have been set up as the mock client
    // does not dynamically listen to new seats
    MockClient client;

    QTRY_COMPARE(client.m_seats.size(), 1);
    MockKeyboard *mockKeyboard = client.m_seats.first()->keyboard();
    QVERIFY(mockKeyboard);
    QCOMPARE(mockKeyboard->m_enteredSurface, nullptr);

    wl_surface *surface = client.createSurface();
    QTRY_COMPARE(compositor.surfaces.size(), 1);

    QWaylandSurface *waylandSurface = compositor.surfaces.at(0);
    QWaylandSeat* seat = compositor.defaultSeat();
    QVERIFY(seat->setKeyboardFocus(waylandSurface));
    QCOMPARE(compositor.defaultSeat()->keyboardFocus(), waylandSurface);

    compositor.flushClients();

    qDebug() << mockKeyboard->m_enteredSurface;
    QTRY_COMPARE(mockKeyboard->m_enteredSurface, surface);

    wl_surface_destroy(surface);
    QTRY_VERIFY(compositor.surfaces.size() == 0);

    QTRY_VERIFY(!compositor.defaultSeat()->keyboardFocus());
}

void tst_WaylandCompositor::seatMouseFocus()
{
    TestCompositor compositor(true);
    compositor.create();

    // Create client after all the seats have been set up as the mock client
    // does not dynamically listen to new seats
    MockClient client;
    wl_surface *surface = client.createSurface();
    QTRY_COMPARE(compositor.surfaces.size(), 1);

    QWaylandSurface *waylandSurface = compositor.surfaces.at(0);
    auto view = new QWaylandView;
    view->setSurface(waylandSurface);

    QWaylandSeat* seat = compositor.defaultSeat();
    seat->setMouseFocus(view);
    seat->sendMouseMoveEvent(view, QPointF(10, 10), QPointF(100, 100));

    compositor.flushClients();

    QTRY_VERIFY(seat->mouseFocus());
    QTRY_VERIFY(seat->pointer());
    QTRY_COMPARE(seat->mouseFocus()->surface(), waylandSurface);

    QTRY_COMPARE(client.m_seats.size(), 1);
    MockPointer *mockPointer = client.m_seats.first()->pointer();
    QVERIFY(mockPointer);
    QTRY_COMPARE(mockPointer->m_enteredSurface, surface);

    delete view;

    compositor.flushClients();

    QTRY_COMPARE(mockPointer->m_enteredSurface, nullptr);
    QTRY_VERIFY(!compositor.defaultSeat()->mouseFocus());

    view = new QWaylandView;
    view->setSurface(waylandSurface);
    seat->sendMouseMoveEvent(view, QPointF(10, 10), QPointF(100, 100));
    QTRY_COMPARE(compositor.defaultSeat()->mouseFocus(), view);

    compositor.flushClients();

    QTRY_COMPARE(mockPointer->m_enteredSurface, surface);

    wl_surface_destroy(surface);
    QTRY_VERIFY(compositor.surfaces.size() == 0);
    QTRY_VERIFY(!compositor.defaultSeat()->mouseFocus());

    delete view;
}

void tst_WaylandCompositor::inputRegion()
{
    TestCompositor compositor(true);
    compositor.create();

    // Create client after all the seats have been set up as the mock client
    // does not dynamically listen to new seats
    MockClient client;
    wl_surface *surface = client.createSurface();

    // We need to attach a buffer, since QWaylandSurface::inputRegionContains will will return
    // false for coordinates outside the buffer (so don't let it be 0x0).
    QSize size(16, 16);
    ShmBuffer buffer(size, client.shm);
    wl_surface_attach(surface, buffer.handle, 0, 0);
    wl_surface_damage(surface, 0, 0, size.width(), size.height());

    // Set the input region
    wl_region *region = wl_compositor_create_region(client.compositor);
    wl_region_add(region, 1, 2, 3, 4);
    wl_surface_set_input_region(surface, region);

    // Commit everything
    wl_surface_commit(surface);

    QTRY_COMPARE(compositor.surfaces.size(), 1);
    QWaylandSurface *waylandSurface = compositor.surfaces.at(0);

    QVERIFY(waylandSurface->inputRegionContains(QPoint(1, 2)));
    QVERIFY(waylandSurface->inputRegionContains(QPoint(3, 5)));
    QVERIFY(!waylandSurface->inputRegionContains(QPoint(0, 0)));
    QVERIFY(!waylandSurface->inputRegionContains(QPoint(1, 6)));
    QVERIFY(!waylandSurface->inputRegionContains(QPoint(4, 2)));

    QVERIFY(!waylandSurface->inputRegionContains(QPointF(0.99, 1.99)));
    QVERIFY(waylandSurface->inputRegionContains(QPointF(1, 2)));
    QVERIFY(waylandSurface->inputRegionContains(QPointF(3.99, 4.99)));
    QVERIFY(!waylandSurface->inputRegionContains(QPointF(4, 5)));

    // Setting a nullptr input region means we want all events
    wl_surface_set_input_region(surface, nullptr);
    wl_surface_commit(surface);

    QTRY_VERIFY(waylandSurface->inputRegionContains(QPoint(0, 0)));
    QVERIFY(waylandSurface->inputRegionContains(QPoint(1, 6)));
    QVERIFY(waylandSurface->inputRegionContains(QPoint(4, 2)));

    // But points outside the buffer should still return false
    QVERIFY(!waylandSurface->inputRegionContains(QPoint(-1, -1)));
    QVERIFY(!waylandSurface->inputRegionContains(QPoint(16, 16)));

    // Setting an empty region means we want no events
    wl_region *emptyRegion = wl_compositor_create_region(client.compositor);
    wl_surface_set_input_region(surface, emptyRegion);
    wl_surface_commit(surface);

    QTRY_VERIFY(!waylandSurface->inputRegionContains(QPoint(0, 0)));
    QVERIFY(!waylandSurface->inputRegionContains(QPoint(1, 2)));
}

void tst_WaylandCompositor::defaultInputRegionHiDpi()
{
    TestCompositor compositor(true);
    compositor.create();

    MockClient client;
    wl_surface *surface = client.createSurface();

    int bufferScale = 2;
    QSize surfaceSize(16, 16);
    QSize bufferSize = surfaceSize * bufferScale;
    ShmBuffer buffer(bufferSize, client.shm);
    wl_surface_attach(surface, buffer.handle, 0, 0);
    wl_surface_damage(surface, 0, 0, surfaceSize.width(), surfaceSize.height());
    wl_surface_set_buffer_scale(surface, bufferScale);
    wl_surface_commit(surface);

    QTRY_COMPARE(compositor.surfaces.size(), 1);
    QWaylandSurface *waylandSurface = compositor.surfaces.at(0);

    QCOMPARE(waylandSurface->bufferScale(), bufferScale);
    QVERIFY(waylandSurface->inputRegionContains(QPoint(0, 0)));
    QVERIFY(waylandSurface->inputRegionContains(QPoint(15, 15)));
    QVERIFY(!waylandSurface->inputRegionContains(QPoint(-1, -1)));
    QVERIFY(!waylandSurface->inputRegionContains(QPoint(16, 16)));
}

class XdgTestCompositor: public TestCompositor {
    Q_OBJECT
public:
    XdgTestCompositor() : xdgShell(this) {}
    QWaylandXdgShell xdgShell;
};

void tst_WaylandCompositor::advertisesXdgShellSupport()
{
    XdgTestCompositor compositor;
    compositor.create();

    MockClient client;
    QTRY_VERIFY(client.xdgWmBase);
}

void tst_WaylandCompositor::createsXdgSurfaces()
{
    XdgTestCompositor compositor;
    compositor.create();

    MockClient client;
    QTRY_VERIFY(client.xdgWmBase);

    QSignalSpy xdgSurfaceCreatedSpy(&compositor.xdgShell, &QWaylandXdgShell::xdgSurfaceCreated);
    QWaylandXdgSurface *xdgSurface = nullptr;
    QObject::connect(&compositor.xdgShell, &QWaylandXdgShell::xdgSurfaceCreated, [&](QWaylandXdgSurface *s) {
        xdgSurface = s;
    });

    wl_surface *surface = client.createSurface();
    xdg_surface *clientXdgSurface = client.createXdgSurface(surface);
    QTRY_COMPARE(xdgSurfaceCreatedSpy.size(), 1);
    QTRY_VERIFY(xdgSurface);
    QTRY_VERIFY(xdgSurface->surface());

    xdg_surface_destroy(clientXdgSurface);
    wl_surface_destroy(surface);
}

void tst_WaylandCompositor::reportsXdgSurfaceWindowGeometry()
{
    XdgTestCompositor compositor;
    compositor.create();

    QWaylandXdgSurface *xdgSurface = nullptr;
    QObject::connect(&compositor.xdgShell, &QWaylandXdgShell::xdgSurfaceCreated, [&](QWaylandXdgSurface *s) {
        xdgSurface = s;
    });

    MockClient client;
    wl_surface *surface = client.createSurface();
    xdg_surface *clientXdgSurface = client.createXdgSurface(surface);
    xdg_toplevel *clientToplevel = client.createXdgToplevel(clientXdgSurface);

    QSize size(256, 256);
    ShmBuffer buffer(size, client.shm);

    QTRY_VERIFY(xdgSurface);
    //TODO: Here we should ideally be acking the configure, we're techically making a protocol error

    wl_surface_attach(surface, buffer.handle, 0, 0);
    wl_surface_damage(surface, 0, 0, size.width(), size.height());
    wl_surface_commit(surface);

    QTRY_COMPARE(xdgSurface->windowGeometry(), QRect(QPoint(0, 0), QSize(256, 256)));

    xdg_surface_set_window_geometry(clientXdgSurface, 10, 20, 100, 50);
    wl_surface_commit(surface);

    QTRY_COMPARE(xdgSurface->windowGeometry(), QRect(QPoint(10, 20), QSize(100, 50)));

    xdg_toplevel_destroy(clientToplevel);
    xdg_surface_destroy(clientXdgSurface);
    wl_surface_destroy(surface);
}

void tst_WaylandCompositor::setsXdgAppId()
{
    XdgTestCompositor compositor;
    compositor.create();

    QWaylandXdgToplevel *toplevel = nullptr;
    QObject::connect(&compositor.xdgShell, &QWaylandXdgShell::toplevelCreated, [&](QWaylandXdgToplevel *t) {
        toplevel = t;
    });

    MockClient client;
    wl_surface *surface = client.createSurface();
    xdg_surface *clientXdgSurface = client.createXdgSurface(surface);
    xdg_toplevel *clientToplevel = client.createXdgToplevel(clientXdgSurface);

    xdg_toplevel_set_app_id(clientToplevel, "org.foo.bar");

    QTRY_VERIFY(toplevel);
    QTRY_COMPARE(toplevel->appId(), QString("org.foo.bar"));
}

void tst_WaylandCompositor::sendsXdgConfigure()
{
    class MockXdgSurface : public QtWayland::xdg_surface
    {
    public:
        explicit MockXdgSurface(::xdg_surface *xdgSurface) : QtWayland::xdg_surface(xdgSurface) {}
        void xdg_surface_configure(uint32_t serial) override { configureSerial = serial; }
        uint configureSerial = 0;
    };

    class MockXdgToplevel : public QtWayland::xdg_toplevel
    {
    public:
        explicit MockXdgToplevel(::xdg_toplevel *toplevel) : QtWayland::xdg_toplevel(toplevel) {}
        void xdg_toplevel_configure(int32_t width, int32_t height, wl_array *rawStates) override
        {
            configureSize = QSize(width, height);
            uint *states = reinterpret_cast<uint*>(rawStates->data);
            configureStates.clear();
            size_t numStates = rawStates->size / sizeof(uint);
            for (size_t i = 0; i < numStates; ++i)
                configureStates.push_back(states[i]);
        }
        QList<uint> configureStates;
        QSize configureSize;
    };

    XdgTestCompositor compositor;
    compositor.create();

    QWaylandXdgToplevel *toplevel = nullptr;
    QObject::connect(&compositor.xdgShell, &QWaylandXdgShell::toplevelCreated, [&](QWaylandXdgToplevel *t) {
        toplevel = t;
    });

    MockClient client;
    wl_surface *surface = client.createSurface();

    xdg_surface *clientXdgSurface = client.createXdgSurface(surface);
    MockXdgSurface mockXdgSurface(clientXdgSurface);

    xdg_toplevel *clientToplevel = client.createXdgToplevel(clientXdgSurface);
    MockXdgToplevel mockToplevel(clientToplevel);

    QTRY_VERIFY(toplevel);
    QTRY_VERIFY(!toplevel->activated());
    QTRY_VERIFY(!toplevel->maximized());
    QTRY_VERIFY(!toplevel->fullscreen());
    QTRY_VERIFY(!toplevel->resizing());

    toplevel->sendConfigure(QSize(10, 20), QList<QWaylandXdgToplevel::State>{QWaylandXdgToplevel::State::ActivatedState});
    compositor.flushClients();
    QTRY_COMPARE(mockToplevel.configureStates, QList<uint>{QWaylandXdgToplevel::State::ActivatedState});
    QTRY_COMPARE(mockToplevel.configureSize, QSize(10, 20));

    toplevel->sendMaximized(QSize(800, 600));
    compositor.flushClients();
    QTRY_VERIFY(mockToplevel.configureStates.contains(QWaylandXdgToplevel::State::MaximizedState));
    QTRY_VERIFY(mockToplevel.configureStates.contains(QWaylandXdgToplevel::State::ActivatedState));
    QTRY_COMPARE(mockToplevel.configureSize, QSize(800, 600));

    // There hasn't been any ack_configures, so state should still be unchanged
    QTRY_VERIFY(!toplevel->activated());
    QTRY_VERIFY(!toplevel->maximized());
    xdg_surface_ack_configure(clientXdgSurface, mockXdgSurface.configureSerial);
    wl_display_dispatch_pending(client.display);
    wl_display_flush(client.display);
    QTRY_VERIFY(toplevel->activated());
    QTRY_VERIFY(toplevel->maximized());

    toplevel->sendUnmaximized();
    compositor.flushClients();
    QTRY_VERIFY(!mockToplevel.configureStates.contains(QWaylandXdgToplevel::State::MaximizedState));
    QTRY_VERIFY(mockToplevel.configureStates.contains(QWaylandXdgToplevel::State::ActivatedState));
    QTRY_COMPARE(mockToplevel.configureSize, QSize(0, 0));

    // The unmaximized configure hasn't been acked, so maximized should still be true
    QTRY_VERIFY(toplevel->maximized());
    QTRY_VERIFY(toplevel->activated());

    toplevel->sendResizing(QSize(800, 600));
    compositor.flushClients();
    QTRY_VERIFY(mockToplevel.configureStates.contains(QWaylandXdgToplevel::State::ResizingState));
    QTRY_COMPARE(mockToplevel.configureSize, QSize(800, 600));

    toplevel->sendFullscreen(QSize(1024, 768));
    compositor.flushClients();
    QTRY_VERIFY(mockToplevel.configureStates.contains(QWaylandXdgToplevel::State::ActivatedState));
    QTRY_VERIFY(mockToplevel.configureStates.contains(QWaylandXdgToplevel::State::FullscreenState));
    QTRY_COMPARE(mockToplevel.configureSize, QSize(1024, 768));
    uint fullscreenSerial = mockXdgSurface.configureSerial;

    toplevel->sendUnmaximized();
    compositor.flushClients();
    QTRY_VERIFY(mockToplevel.configureStates.contains(QWaylandXdgToplevel::State::ActivatedState));
    QTRY_VERIFY(!mockToplevel.configureStates.contains(QWaylandXdgToplevel::State::FullscreenState));

    toplevel->sendConfigure(QSize(0, 0), QList<QWaylandXdgToplevel::State>{});
    compositor.flushClients();
    QTRY_VERIFY(!mockToplevel.configureStates.contains(QWaylandXdgToplevel::State::ActivatedState));

    toplevel->sendMaximized(QSize(800, 600));
    compositor.flushClients();
    QTRY_VERIFY(!mockToplevel.configureStates.contains(QWaylandXdgToplevel::State::ActivatedState));

    toplevel->sendFullscreen(QSize(800, 600));
    compositor.flushClients();
    QTRY_VERIFY(!mockToplevel.configureStates.contains(QWaylandXdgToplevel::State::MaximizedState));

    // Verify that acking a configure that's not the most recently sent works
    xdg_surface_ack_configure(clientXdgSurface, fullscreenSerial);
    wl_display_dispatch_pending(client.display);
    wl_display_flush(client.display);
    QTRY_VERIFY(toplevel->fullscreen());
    QTRY_VERIFY(toplevel->activated());
    QTRY_VERIFY(!toplevel->maximized());
    QTRY_VERIFY(!toplevel->resizing());
}

class IviTestCompositor: public TestCompositor {
    Q_OBJECT
public:
    IviTestCompositor() : iviApplication(this) {}
    QWaylandIviApplication iviApplication;
};

void tst_WaylandCompositor::advertisesIviApplicationSupport()
{
    IviTestCompositor compositor;
    compositor.create();

    MockClient client;
    QTRY_VERIFY(client.iviApplication);
}

void tst_WaylandCompositor::createsIviSurfaces()
{
    IviTestCompositor compositor;
    compositor.create();

    MockClient client;
    QTRY_VERIFY(client.iviApplication);

    QSignalSpy iviSurfaceCreatedSpy(&compositor.iviApplication, &QWaylandIviApplication::iviSurfaceRequested);
    QWaylandIviSurface *iviSurface = nullptr;
    QObject::connect(&compositor.iviApplication, &QWaylandIviApplication::iviSurfaceCreated, [&](QWaylandIviSurface *s) {
        iviSurface = s;
    });

    wl_surface *surface = client.createSurface();
    client.createIviSurface(surface, 123);
    QTRY_COMPARE(iviSurfaceCreatedSpy.size(), 1);
    QTRY_VERIFY(iviSurface);
    QTRY_VERIFY(iviSurface->surface());
    QTRY_COMPARE(iviSurface->iviId(), 123u);
}

void tst_WaylandCompositor::emitsErrorOnSameIviId()
{
    IviTestCompositor compositor;
    compositor.create();

    {
        MockClient firstClient;
        QTRY_VERIFY(&firstClient.iviApplication);

        QWaylandIviSurface *firstIviSurface = nullptr;
        QObject::connect(&compositor.iviApplication, &QWaylandIviApplication::iviSurfaceCreated, [&](QWaylandIviSurface *s) {
            firstIviSurface = s;
        });

        firstClient.createIviSurface(firstClient.createSurface(), 123);
        QTRY_VERIFY(firstIviSurface);
        QTRY_COMPARE(firstIviSurface->iviId(), 123u);

        {
            MockClient secondClient;
            QTRY_VERIFY(&secondClient.iviApplication);
            QTRY_COMPARE(compositor.clients().size(), 2);

            secondClient.createIviSurface(secondClient.createSurface(), 123);
            compositor.flushClients();

            QTRY_COMPARE(secondClient.error, EPROTO);
            QTRY_COMPARE(secondClient.protocolError.interface, &ivi_application_interface);
            QTRY_COMPARE(static_cast<ivi_application_error>(secondClient.protocolError.code), IVI_APPLICATION_ERROR_IVI_ID);
            QTRY_COMPARE(compositor.clients().size(), 1);
        }
    }

    // The other clients have passed out of scope and have been destroyed,
    // it should now be ok to create new application with the same id
    MockClient thirdClient;
    QTRY_VERIFY(&thirdClient.iviApplication);

    QWaylandIviSurface *thirdIviSurface = nullptr;
    QObject::connect(&compositor.iviApplication, &QWaylandIviApplication::iviSurfaceCreated, [&](QWaylandIviSurface *s) {
        thirdIviSurface = s;
    });
    thirdClient.createIviSurface(thirdClient.createSurface(), 123);
    compositor.flushClients();

    QTRY_VERIFY(thirdIviSurface);
    QTRY_COMPARE(thirdIviSurface->iviId(), 123u);
    QTRY_COMPARE(thirdClient.error, 0);
}

void tst_WaylandCompositor::sendsIviConfigure()
{
    class MockIviSurface : public QtWayland::ivi_surface
    {
    public:
        MockIviSurface(::ivi_surface *iviSurface) : QtWayland::ivi_surface(iviSurface) {}
        void ivi_surface_configure(int32_t width, int32_t height) override
        {
            configureSize = QSize(width, height);
        }
        QSize configureSize;
    };

    IviTestCompositor compositor;
    compositor.create();

    MockClient client;
    QTRY_VERIFY(client.iviApplication);

    QWaylandIviSurface *iviSurface = nullptr;
    QObject::connect(&compositor.iviApplication, &QWaylandIviApplication::iviSurfaceCreated, [&](QWaylandIviSurface *s) {
        iviSurface = s;
    });

    wl_surface *surface = client.createSurface();
    ivi_surface *clientIviSurface = client.createIviSurface(surface, 123);
    MockIviSurface mockIviSurface(clientIviSurface);

    QTRY_VERIFY(iviSurface);
    iviSurface->sendConfigure(QSize(800, 600));
    compositor.flushClients();

    QTRY_COMPARE(mockIviSurface.configureSize, QSize(800, 600));
}

void tst_WaylandCompositor::destroysIviSurfaces()
{
    IviTestCompositor compositor;
    compositor.create();

    MockClient client;
    QTRY_VERIFY(client.iviApplication);

    QWaylandIviSurface *iviSurface = nullptr;
    QObject::connect(&compositor.iviApplication, &QWaylandIviApplication::iviSurfaceCreated, [&](QWaylandIviSurface *s) {
        iviSurface = s;
    });

    QtWayland::ivi_surface mockIviSurface(client.createIviSurface(client.createSurface(), 123));
    QTRY_VERIFY(iviSurface);

    QSignalSpy destroySpy(iviSurface, SIGNAL(destroyed()));
    mockIviSurface.destroy();
    QTRY_VERIFY(destroySpy.size() == 1);
}

class ViewporterTestCompositor: public TestCompositor {
    Q_OBJECT
public:
    ViewporterTestCompositor() : viewporter(this) {}
    QWaylandViewporter viewporter;
};

void tst_WaylandCompositor::viewporterGlobal()
{
    ViewporterTestCompositor compositor;
    compositor.create();
    MockClient client;
    QTRY_VERIFY(client.viewporter);
}

void tst_WaylandCompositor::viewportDestination()
{
    ViewporterTestCompositor compositor;
    compositor.create();
    MockClient client;
    QTRY_VERIFY(client.viewporter);

    wl_surface *surface = client.createSurface();
    QTRY_COMPARE(compositor.surfaces.size(), 1);
    QWaylandSurface *waylandSurface = compositor.surfaces.at(0);

    QCOMPARE(waylandSurface->destinationSize(), QSize());
    QCOMPARE(waylandSurface->sourceGeometry(), QRect());

    const QSize bufferSize(64, 64);
    ShmBuffer buffer(bufferSize, client.shm);
    wl_surface_attach(surface, buffer.handle, 0, 0);
    wl_surface_damage(surface, 0, 0, bufferSize.width(), bufferSize.height());
    wp_viewport *viewport = wp_viewporter_get_viewport(client.viewporter, surface);
    const QSize destinationSize(128, 123);
    wp_viewport_set_destination(viewport, destinationSize.width(), destinationSize.height());
    wl_surface_commit(surface);

    QTRY_COMPARE(waylandSurface->bufferSize(), bufferSize);
    QCOMPARE(waylandSurface->destinationSize(), QSize(128, 123));
    QCOMPARE(waylandSurface->sourceGeometry(), QRect(QPoint(), bufferSize));

    wp_viewport_destroy(viewport);
    wl_surface_destroy(surface);
    QCOMPARE(client.error, 0);
}

void tst_WaylandCompositor::viewportSource()
{
    ViewporterTestCompositor compositor;
    compositor.create();
    MockClient client;
    QTRY_VERIFY(client.viewporter);

    wl_surface *surface = client.createSurface();
    QTRY_COMPARE(compositor.surfaces.size(), 1);
    QWaylandSurface *waylandSurface = compositor.surfaces.at(0);

    QCOMPARE(waylandSurface->destinationSize(), QSize());
    QCOMPARE(waylandSurface->sourceGeometry(), QRect());

    const QSize bufferSize(64, 64);
    ShmBuffer buffer(bufferSize, client.shm);
    wl_surface_attach(surface, buffer.handle, 0, 0);
    wl_surface_damage(surface, 0, 0, bufferSize.width(), bufferSize.height());
    wp_viewport *viewport = wp_viewporter_get_viewport(client.viewporter, surface);
    const QRectF sourceGeometry(QPointF(10.5, 20.5), QSizeF(30, 40));
    wp_viewport_set_source(viewport,
                           wl_fixed_from_double(sourceGeometry.x()),
                           wl_fixed_from_double(sourceGeometry.y()),
                           wl_fixed_from_double(sourceGeometry.width()),
                           wl_fixed_from_double(sourceGeometry.height()));
    wl_surface_commit(surface);

    QTRY_COMPARE(waylandSurface->bufferSize(), bufferSize);
    QCOMPARE(waylandSurface->destinationSize(), sourceGeometry.size().toSize());
    QCOMPARE(waylandSurface->sourceGeometry(), sourceGeometry);

    wp_viewport_destroy(viewport);
    wl_surface_destroy(surface);
    QCOMPARE(client.error, 0);
}

void tst_WaylandCompositor::viewportSourceAndDestination()
{
    ViewporterTestCompositor compositor;
    compositor.create();
    MockClient client;
    QTRY_VERIFY(client.viewporter);

    wl_surface *surface = client.createSurface();
    QTRY_COMPARE(compositor.surfaces.size(), 1);
    QWaylandSurface *waylandSurface = compositor.surfaces.at(0);

    QCOMPARE(waylandSurface->destinationSize(), QSize());
    QCOMPARE(waylandSurface->sourceGeometry(), QRect());

    const QSize bufferSize(64, 64);
    ShmBuffer buffer(bufferSize, client.shm);
    wl_surface_attach(surface, buffer.handle, 0, 0);
    wl_surface_damage(surface, 0, 0, bufferSize.width(), bufferSize.height());

    wp_viewport *viewport = wp_viewporter_get_viewport(client.viewporter, surface);

    const QSize destinationSize(128, 123);
    wp_viewport_set_destination(viewport, destinationSize.width(), destinationSize.height());

    const QRectF sourceGeometry(QPointF(10, 20), QSizeF(30, 40));
    wp_viewport_set_source(viewport,
                           wl_fixed_from_double(sourceGeometry.x()),
                           wl_fixed_from_double(sourceGeometry.y()),
                           wl_fixed_from_double(sourceGeometry.width()),
                           wl_fixed_from_double(sourceGeometry.height()));

    wl_surface_commit(surface);

    QTRY_COMPARE(waylandSurface->bufferSize(), bufferSize);
    QCOMPARE(waylandSurface->destinationSize(), destinationSize);
    QCOMPARE(waylandSurface->sourceGeometry(), sourceGeometry);

    wp_viewport_destroy(viewport);
    wl_surface_destroy(surface);
    QCOMPARE(client.error, 0);
}

void tst_WaylandCompositor::viewportDestruction()
{
    ViewporterTestCompositor compositor;
    compositor.create();
    MockClient client;
    QTRY_VERIFY(client.viewporter);

    wl_surface *surface = client.createSurface();
    QTRY_COMPARE(compositor.surfaces.size(), 1);
    QWaylandSurface *waylandSurface = compositor.surfaces.at(0);

    QCOMPARE(waylandSurface->destinationSize(), QSize());
    QCOMPARE(waylandSurface->sourceGeometry(), QRect());

    const QSize bufferSize(64, 64);
    ShmBuffer buffer(bufferSize, client.shm);
    wl_surface_attach(surface, buffer.handle, 0, 0);
    wl_surface_damage(surface, 0, 0, bufferSize.width(), bufferSize.height());

    wp_viewport *viewport = wp_viewporter_get_viewport(client.viewporter, surface);

    const QSize destinationSize(128, 123);
    wp_viewport_set_destination(viewport, destinationSize.width(), destinationSize.height());

    const QRectF sourceGeometry(QPointF(10, 20), QSizeF(30, 40));
    wp_viewport_set_source(viewport,
                           wl_fixed_from_double(sourceGeometry.x()),
                           wl_fixed_from_double(sourceGeometry.y()),
                           wl_fixed_from_double(sourceGeometry.width()),
                           wl_fixed_from_double(sourceGeometry.height()));

    wl_surface_commit(surface);

    QTRY_COMPARE(waylandSurface->bufferSize(), bufferSize);
    QCOMPARE(waylandSurface->destinationSize(), destinationSize);
    QCOMPARE(waylandSurface->sourceGeometry(), sourceGeometry);

    wp_viewport_destroy(viewport);
    wl_surface_commit(surface);

    QTRY_COMPARE(waylandSurface->destinationSize(), bufferSize);
    QCOMPARE(waylandSurface->sourceGeometry(), QRectF(QPoint(), bufferSize));

    wl_surface_destroy(surface);
    QCOMPARE(client.error, 0);
}

void tst_WaylandCompositor::viewportProtocolErrors_data()
{
    QTest::addColumn<QRectF>("source");
    QTest::addColumn<QSize>("destination");
    QTest::addColumn<uint>("error");

    QTest::newRow("invalid source position") << QRectF(-1, 0, 16, 16) << QSize(64, 64) << uint(WP_VIEWPORT_ERROR_BAD_VALUE);
    QTest::newRow("invalid source size") << QRectF(0, 0, -1, 16) << QSize(64, 64) << uint(WP_VIEWPORT_ERROR_BAD_VALUE);
    QTest::newRow("invalid destination size") << QRectF(0, 0, 16, 16) << QSize(-16, 64) << uint(WP_VIEWPORT_ERROR_BAD_VALUE);
    QTest::newRow("invalid non-integer source with unset size") << QRectF(0, 0, 15.5, 15.5) << QSize(-1, -1) << uint(WP_VIEWPORT_ERROR_BAD_SIZE);
    QTest::newRow("bigger source than buffer") << QRectF(0, 0, 13337, 13337) << QSize(-1, -1) << uint(WP_VIEWPORT_ERROR_OUT_OF_BUFFER);
}

void tst_WaylandCompositor::viewportProtocolErrors()
{
    QFETCH(QRectF, source);
    QFETCH(QSize, destination);
    QFETCH(uint, error);

    ViewporterTestCompositor compositor;
    compositor.create();
    MockClient client;
    QTRY_VERIFY(client.viewporter);

    wl_surface *surface = client.createSurface();

    const QSize bufferSize(64, 64);
    ShmBuffer buffer(bufferSize, client.shm);
    wl_surface_attach(surface, buffer.handle, 0, 0);
    wl_surface_damage(surface, 0, 0, bufferSize.width(), bufferSize.height());
    wp_viewport *viewport = wp_viewporter_get_viewport(client.viewporter, surface);
    wp_viewport_set_source(viewport,
                           wl_fixed_from_double(source.x()),
                           wl_fixed_from_double(source.y()),
                           wl_fixed_from_double(source.width()),
                           wl_fixed_from_double(source.height()));
    wp_viewport_set_destination(viewport, destination.width(), destination.height());
    wl_surface_commit(surface);

    QTRY_COMPARE(client.error, EPROTO);
    QCOMPARE(client.protocolError.interface, &wp_viewport_interface);
    QCOMPARE(static_cast<wp_viewport_error>(client.protocolError.code), error);
}

void tst_WaylandCompositor::viewportClearDestination()
{
    ViewporterTestCompositor compositor;
    compositor.create();
    MockClient client;
    QTRY_VERIFY(client.viewporter);

    wl_surface *surface = client.createSurface();
    QTRY_COMPARE(compositor.surfaces.size(), 1);
    QWaylandSurface *waylandSurface = compositor.surfaces.at(0);

    QCOMPARE(waylandSurface->destinationSize(), QSize());
    QCOMPARE(waylandSurface->sourceGeometry(), QRect());

    const QSize bufferSize(64, 64);
    ShmBuffer buffer(bufferSize, client.shm);
    wl_surface_attach(surface, buffer.handle, 0, 0);
    wl_surface_damage(surface, 0, 0, bufferSize.width(), bufferSize.height());

    wp_viewport *viewport = wp_viewporter_get_viewport(client.viewporter, surface);

    const QSize destinationSize(128, 123);
    wp_viewport_set_destination(viewport, destinationSize.width(), destinationSize.height());
    wl_surface_commit(surface);

    QTRY_COMPARE(waylandSurface->bufferSize(), bufferSize);
    QCOMPARE(waylandSurface->destinationSize(), destinationSize);

    wp_viewport_set_destination(viewport, -1, -1);
    wl_surface_commit(surface);

    QTRY_COMPARE(waylandSurface->destinationSize(), bufferSize);
    QCOMPARE(waylandSurface->sourceGeometry(), QRectF(QPoint(), bufferSize));

    wp_viewport_destroy(viewport);
    wl_surface_destroy(surface);
    QCOMPARE(client.error, 0);
}

void tst_WaylandCompositor::viewportClearSource()
{
    ViewporterTestCompositor compositor;
    compositor.create();
    MockClient client;
    QTRY_VERIFY(client.viewporter);

    wl_surface *surface = client.createSurface();
    QTRY_COMPARE(compositor.surfaces.size(), 1);
    QWaylandSurface *waylandSurface = compositor.surfaces.at(0);

    QCOMPARE(waylandSurface->destinationSize(), QSize());
    QCOMPARE(waylandSurface->sourceGeometry(), QRect());

    const QSize bufferSize(64, 64);
    ShmBuffer buffer(bufferSize, client.shm);
    wl_surface_attach(surface, buffer.handle, 0, 0);
    wl_surface_damage(surface, 0, 0, bufferSize.width(), bufferSize.height());

    wp_viewport *viewport = wp_viewporter_get_viewport(client.viewporter, surface);
    QRectF source(10, 20, 30, 40);
    wp_viewport_set_source(viewport,
                           wl_fixed_from_double(source.x()),
                           wl_fixed_from_double(source.y()),
                           wl_fixed_from_double(source.width()),
                           wl_fixed_from_double(source.height()));
    wl_surface_commit(surface);

    QTRY_COMPARE(waylandSurface->sourceGeometry(), source);

    wp_viewport_set_source(viewport,
                           wl_fixed_from_double(-1),
                           wl_fixed_from_double(-1),
                           wl_fixed_from_double(-1),
                           wl_fixed_from_double(-1));
    wl_surface_commit(surface);

    QTRY_COMPARE(waylandSurface->sourceGeometry(), QRectF(QPoint(), bufferSize));

    wp_viewport_destroy(viewport);
    wl_surface_destroy(surface);
    QCOMPARE(client.error, 0);
}

void tst_WaylandCompositor::viewportExistsError()
{
    ViewporterTestCompositor compositor;
    compositor.create();
    MockClient client;
    QTRY_VERIFY(client.viewporter);

    wl_surface *surface = client.createSurface();
    wp_viewporter_get_viewport(client.viewporter, surface);
    wp_viewporter_get_viewport(client.viewporter, surface);

    QTRY_COMPARE(client.error, EPROTO);
    QCOMPARE(client.protocolError.interface, &wp_viewporter_interface);
    QCOMPARE(static_cast<wp_viewporter_error>(client.protocolError.code), WP_VIEWPORTER_ERROR_VIEWPORT_EXISTS);
}

void tst_WaylandCompositor::viewportDestinationNoSurfaceError()
{
    ViewporterTestCompositor compositor;
    compositor.create();
    MockClient client;
    QTRY_VERIFY(client.viewporter);

    wl_surface *surface = client.createSurface();
    wp_viewport *viewport = wp_viewporter_get_viewport(client.viewporter, surface);
    wl_surface_destroy(surface);
    wp_viewport_set_destination(viewport, 32, 32);

    QTRY_COMPARE(client.error, EPROTO);
    QCOMPARE(client.protocolError.interface, &wp_viewport_interface);
    QCOMPARE(static_cast<wp_viewport_error>(client.protocolError.code), WP_VIEWPORT_ERROR_NO_SURFACE);
}

void tst_WaylandCompositor::viewportSourceNoSurfaceError()
{
    ViewporterTestCompositor compositor;
    compositor.create();
    MockClient client;
    QTRY_VERIFY(client.viewporter);

    wl_surface *surface = client.createSurface();
    wp_viewport *viewport = wp_viewporter_get_viewport(client.viewporter, surface);
    wl_surface_destroy(surface);
    wp_viewport_set_source(viewport,
                           wl_fixed_from_double(0),
                           wl_fixed_from_double(0),
                           wl_fixed_from_double(1),
                           wl_fixed_from_double(1));

    QTRY_COMPARE(client.error, EPROTO);
    QCOMPARE(client.protocolError.interface, &wp_viewport_interface);
    QCOMPARE(static_cast<wp_viewport_error>(client.protocolError.code), WP_VIEWPORT_ERROR_NO_SURFACE);
}

void tst_WaylandCompositor::viewportHiDpi()
{
    ViewporterTestCompositor compositor;
    compositor.create();
    MockClient client;
    QTRY_VERIFY(client.viewporter);

    wl_surface *surface = client.createSurface();
    QTRY_COMPARE(compositor.surfaces.size(), 1);
    QWaylandSurface *waylandSurface = compositor.surfaces.at(0);

    const QSize bufferSize(128, 128);
    ShmBuffer buffer(bufferSize, client.shm);
    wl_surface_attach(surface, buffer.handle, 0, 0);
    wl_surface_damage(surface, 0, 0, bufferSize.width(), bufferSize.height());
    constexpr int bufferScale = 2;
    wl_surface_set_buffer_scale(surface, bufferScale);

    wl_surface_commit(surface);
    QTRY_COMPARE(waylandSurface->destinationSize(), bufferSize / bufferScale);

    wp_viewport *viewport = wp_viewporter_get_viewport(client.viewporter, surface);
    const QRectF sourceGeometry(QPointF(10, 20), QSizeF(30, 40));
    wp_viewport_set_source(viewport,
                           wl_fixed_from_double(sourceGeometry.x()),
                           wl_fixed_from_double(sourceGeometry.y()),
                           wl_fixed_from_double(sourceGeometry.width()),
                           wl_fixed_from_double(sourceGeometry.height()));
    wl_surface_commit(surface);

    QTRY_COMPARE(waylandSurface->destinationSize(), sourceGeometry.size());
    QCOMPARE(waylandSurface->sourceGeometry(), sourceGeometry);
    QCOMPARE(waylandSurface->bufferSize(), bufferSize);

    const QSize destinationSize(128, 123);
    wp_viewport_set_destination(viewport, destinationSize.width(), destinationSize.height());
    wl_surface_commit(surface);

    QTRY_COMPARE(waylandSurface->destinationSize(), destinationSize);
    QCOMPARE(waylandSurface->sourceGeometry(), sourceGeometry);
    QCOMPARE(waylandSurface->bufferSize(), bufferSize);

    QCOMPARE(client.error, 0);

    wp_viewport_destroy(viewport);
    wl_surface_destroy(surface);
}

class IdleInhibitCompositor : public TestCompositor
{
    Q_OBJECT
public:
    IdleInhibitCompositor() : idleInhibitManager(this) {}
    QWaylandIdleInhibitManagerV1 idleInhibitManager;
};

void tst_WaylandCompositor::idleInhibit()
{
    IdleInhibitCompositor compositor;
    compositor.create();
    MockClient client;
    QTRY_VERIFY(client.idleInhibitManager);

    auto *surface = client.createSurface();
    QVERIFY(surface);
    QTRY_COMPARE(compositor.surfaces.size(), 1);

    QWaylandSurface *waylandSurface = compositor.surfaces.at(0);
    auto *waylandSurfacePrivate =
            QWaylandSurfacePrivate::get(waylandSurface);
    QVERIFY(waylandSurfacePrivate);

    QSignalSpy changedSpy(waylandSurface, SIGNAL(inhibitsIdleChanged()));

    QCOMPARE(waylandSurface->inhibitsIdle(), false);

    auto *idleInhibitor = client.createIdleInhibitor(surface);
    QVERIFY(idleInhibitor);
    QTRY_COMPARE(waylandSurfacePrivate->idleInhibitors.size(), 1);
    QCOMPARE(waylandSurface->inhibitsIdle(), true);
    QTRY_COMPARE(changedSpy.size(), 1);
}

class XdgOutputCompositor : public TestCompositor
{
    Q_OBJECT
public:
    XdgOutputCompositor() : xdgOutputManager(this) {}
    QWaylandXdgOutputManagerV1 xdgOutputManager;
};

void tst_WaylandCompositor::xdgOutput()
{
    XdgOutputCompositor compositor;
    compositor.create();

    QWaylandOutputMode mode(QSize(1024, 768), 60000);
    compositor.defaultOutput()->addMode(mode, true);
    compositor.defaultOutput()->setCurrentMode(mode);

    MockClient client;
    QTRY_VERIFY(client.xdgOutputManager);
    QTRY_COMPARE(client.m_outputs.size(), 1);

    auto *wlOutput = client.m_outputs.first();
    QVERIFY(wlOutput);

    // Output is not associated yet
    QCOMPARE(QWaylandOutputPrivate::get(compositor.defaultOutput())->xdgOutput.isNull(), true);

    // Create xdg-output on the server
    auto *xdgOutputServer = new QWaylandXdgOutputV1(compositor.defaultOutput(), &compositor.xdgOutputManager);
    QVERIFY(xdgOutputServer);
    xdgOutputServer->setName(QStringLiteral("OUTPUT1"));
    xdgOutputServer->setDescription(QStringLiteral("This is a test output"));

    // Create it on the client
    auto *xdgOutput = client.createXdgOutput(wlOutput);
    QVERIFY(xdgOutput);
    QVERIFY(client.m_xdgOutputs.contains(wlOutput));

    // Now it should be associated
    QCOMPARE(QWaylandOutputPrivate::get(compositor.defaultOutput())->xdgOutput.isNull(), false);

    // Verify initial values
    QTRY_COMPARE(xdgOutput->name, "OUTPUT1");
    QTRY_COMPARE(xdgOutput->logicalPosition, QPoint());
    QTRY_COMPARE(xdgOutput->logicalSize, QSize());

    // Change properties
    xdgOutputServer->setName(QStringLiteral("OUTPUT2"));
    xdgOutputServer->setDescription(QStringLiteral("New description"));
    xdgOutputServer->setLogicalPosition(QPoint(100, 100));
    xdgOutputServer->setLogicalSize(QSize(1000, 1000));
    compositor.flushClients();

    // Name and description can't be changed after initialization,
    // so we expect them to be the same
    // TODO: With protocol version 3 the description will be allowed to change,
    // but we implement version 2 now
    QTRY_COMPARE(xdgOutput->name, "OUTPUT1");
    QTRY_COMPARE(xdgOutput->description, "This is a test output");
    QTRY_COMPARE(xdgOutput->logicalPosition, QPoint(100, 100));
    QTRY_COMPARE(xdgOutput->logicalSize, QSize(1000, 1000));
}

#include <tst_compositor.moc>
QTEST_MAIN(tst_WaylandCompositor);
