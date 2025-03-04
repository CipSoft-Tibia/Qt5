// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qrasterwindow.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformwindow.h>
#include <private/qguiapplication_p.h>
#include <private/qhighdpiscaling_p.h>
#include <private/qwindow_p.h>
#include <QtGui/QPainter>

#include <QTest>
#include <QSignalSpy>
#include <QEvent>
#include <QStyleHints>

#if defined(Q_OS_QNX)
#include <QOpenGLContext>
#elif defined(Q_OS_WIN)
#  include <QtCore/qt_windows.h>
#endif

Q_LOGGING_CATEGORY(lcTests, "qt.gui.tests")

static bool isPlatformEglFS()
{
    static const bool isEglFS = !QGuiApplication::platformName().compare(QLatin1String("eglfs"), Qt::CaseInsensitive);
    return isEglFS;
}

class tst_QWindow: public QObject
{
    Q_OBJECT

private slots:
    void create();
    void setParent();
    void setVisible();
    void setVisibleThenCreate();
    void setVisibleFalseDoesNotCreateWindow();
    void eventOrderOnShow();
    void paintEvent();
    void resizeEventAfterResize();
    void exposeEventOnShrink_QTBUG54040();
    void mapGlobal();
    void positioning();
    void framePositioning();
    void framePositioning_data();
    void positioningDuringMinimized();
    void childWindowPositioning_data();
    void childWindowPositioning();
    void childWindowLevel();
    void platformSurface();
    void isExposed();
    void isActive();
    void testInputEvents();
    void touchToMouseTranslation();
    void touchToMouseTranslationForDevices();
    void mouseToTouchTranslation();
    void mouseToTouchLoop();
    void touchCancel();
    void touchCancelWithTouchToMouse();
    void touchInterruptedByPopup();
    void orientation();
    void sizes();
    void close();
    void activateAndClose();
    void mouseEventSequence();
    void windowModality();
    void inputReentrancy();
    void tabletEvents();
    void windowModality_QTBUG27039();
    void visibility();
    void mask();
    void initialSize();
    void modalDialog();
    void modalDialogClosingOneOfTwoModal();
    void modalWithChildWindow();
    void modalWindowModallity();
    void modalWindowPosition();
    void modalCloseWhileBlocked();
#ifndef QT_NO_CURSOR
    void modalWindowEnterEventOnHide_QTBUG35109();
    void spuriousMouseMove();
#endif
    void windowsTransientChildren();
    void requestUpdate();
    void initTestCase();
    void stateChange_data();
    void stateChange();
    void flags();
    void cleanup();
    void testBlockingWindowShownAfterModalDialog();
    void generatedMouseMove();
    void keepPendingUpdateRequests();
    void activateDeactivateEvent();
    void qobject_castOnDestruction();
    void touchToMouseTranslationByPopup();
    void stateChangeSignal();
#ifndef QT_NO_CURSOR
    void enterLeaveOnWindowShowHide_data();
    void enterLeaveOnWindowShowHide();
#endif
    void windowExposedAfterReparent();
    void childEvents();
    void parentEvents();

private:
    QPoint m_availableTopLeft;
    QSize m_testWindowSize;
    QPointingDevice *touchDevice = QTest::createTouchDevice();
    QPointingDevice *touchDeviceWithMouseEmulation =
            QTest::createTouchDevice(QInputDevice::DeviceType::TouchScreen,
                                     QInputDevice::Capability::Position | QInputDevice::Capability::MouseEmulation);
};

static bool isPlatformWayland()
{
    return QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive);
}

void tst_QWindow::initTestCase()
{
#ifdef Q_OS_ANDROID
    if (QNativeInterface::QAndroidApplication::sdkVersion() == 33)
        QSKIP("Is flaky on Android 13 / RHEL 8.6 and 8.8 (QTQAINFRA-5606)");
#endif
    // Size of reference window, 200 for < 2000, scale up for larger screens
    // to avoid Windows warnings about minimum size for decorated windows.
    int width = 200;
    const QScreen *screen = QGuiApplication::primaryScreen();
    m_availableTopLeft = screen->availableGeometry().topLeft();
    const int screenWidth = screen->geometry().width();
    if (screenWidth > 2000)
        width = 100 * ((screenWidth + 500) / 1000);
    m_testWindowSize = QSize(width, width);

    // Make sure test runs consistently on all compositors by force-disabling window decorations
    if (isPlatformWayland())
        qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
}

void tst_QWindow::cleanup()
{
    QVERIFY(QGuiApplication::allWindows().isEmpty());
}

void tst_QWindow::create()
{
    QWindow a;
    QVERIFY2(!a.handle(), "QWindow should lazy init the platform window");

    a.create();
    QVERIFY2(a.handle(), "Explicitly creating a platform window should never fail");

    QWindow b;
    QWindow c(&b);
    b.create();
    QVERIFY(b.handle());
    QVERIFY2(!c.handle(), "Creating a parent window should not automatically create children");

    QWindow d;
    QWindow e(&d);
    e.create();
    QVERIFY(e.handle());
    QVERIFY2(d.handle(), "Creating a child window should automatically create parents");

    QWindow f;
    QWindow g(&f);
    f.create();
    QVERIFY(f.handle());
    QPlatformWindow *platformWindow = f.handle();
    g.create();
    QVERIFY(g.handle());
    QVERIFY2(f.handle() == platformWindow, "Creating a child window should not affect parent if already created");
}

void tst_QWindow::setParent()
{
    QWindow a;
    QWindow b(&a);
    QVERIFY2(b.parent() == &a, "Setting parent at construction time should work");
    QVERIFY2(a.children().contains(&b), "Parent should have child in list of children");

    QWindow c;
    QWindow d;
    d.setParent(&c);
    QVERIFY2(d.parent() == &c, "Setting parent after construction should work");
    QVERIFY2(c.children().contains(&d), "Parent should have child in list of children");

    a.create();
    b.setParent(nullptr);
    QVERIFY2(!b.handle(), "Making window top level shouild not automatically create it");

    QWindow e;
    c.create();
    e.setParent(&c);
    QVERIFY2(!e.handle(), "Making window a child of a created window should not automatically create it");

    QWindow f;
    QWindow g;
    g.create();
    QVERIFY(g.handle());
    g.setParent(&f);
    QVERIFY2(f.handle(), "Making a created window a child of a non-created window should automatically create it");
}

void tst_QWindow::setVisible()
{
    QWindow a;
    QWindow b(&a);
    a.setVisible(true);
    QVERIFY2(!b.handle(), "Making a top level window visible doesn't create its children");
    QVERIFY2(!b.isVisible(), "Making a top level window visible doesn't make its children visible");
    QVERIFY(QTest::qWaitForWindowExposed(&a));

    QWindow c;
    QWindow d(&c);
    d.setVisible(true);
    QVERIFY2(!c.handle(), "Making a child window visible doesn't create parent window if parent is hidden");
    QVERIFY2(!c.isVisible(), "Making a child window visible doesn't make its parent visible");

    QVERIFY2(!d.handle(), "Making a child window visible doesn't create platform window if parent is hidden");

    c.create();
    QVERIFY(c.handle());
    QVERIFY2(d.handle(), "Creating a parent window should automatically create children if they are visible");
    QVERIFY2(!c.isVisible(), "Creating a parent window should not make it visible just because it has visible children");

    QWindow e;
    QWindow f(&e);
    f.setVisible(true);
    QVERIFY(!f.handle());
    QVERIFY(!e.handle());
    f.setParent(nullptr);
    QVERIFY2(f.handle(), "Making a visible but not created child window top level should create it");
    QVERIFY(QTest::qWaitForWindowExposed(&f));

    QWindow g;
    QWindow h;
    QWindow i(&g);
    i.setVisible(true);
    h.setVisible(true);
    QVERIFY(QTest::qWaitForWindowExposed(&h));
    QVERIFY(!i.handle());
    QVERIFY(!g.handle());
    QVERIFY(h.handle());
    i.setParent(&h);
    QVERIFY2(i.handle(), "Making a visible but not created child window child of a created window should create it");
    QVERIFY(QTest::qWaitForWindowExposed(&i));
}

class SurfaceCreatedWindow : public QWindow
{
    Q_OBJECT
public:
    using QWindow::QWindow;

    bool eventFilter(QObject *o, QEvent *e) override
    {
        if (e->type() == QEvent::PlatformSurface) {
            auto type = static_cast<QPlatformSurfaceEvent*>(e)->surfaceEventType();
            if (type == QPlatformSurfaceEvent::SurfaceCreated)
                ++surfaceCreatedEvents;
        }
        return QWindow::eventFilter(o, e);
    }

    int surfaceCreatedEvents = 0;
};

void tst_QWindow::setVisibleThenCreate()
{
    QWindow parent;
    parent.setObjectName("Parent");
    SurfaceCreatedWindow child(&parent);
    child.installEventFilter(&child);
    child.setObjectName("Child");
    child.setVisible(true);
    child.create();
    QCOMPARE(child.surfaceCreatedEvents, 1);
    parent.setVisible(true);
    QCOMPARE(child.surfaceCreatedEvents, 1);
    QVERIFY(QTest::qWaitForWindowExposed(&child));
}

void tst_QWindow::setVisibleFalseDoesNotCreateWindow()
{
    QWindow w;
    QVERIFY(!w.handle());
    w.setVisible(false);
    QVERIFY2(!w.handle(), "Hiding a non-created window doesn't create it");
    w.setVisible(true);
    QVERIFY2(w.handle(), "Showing a non-created window creates it");
}

void tst_QWindow::mapGlobal()
{
    QWindow a;
    QWindow b(&a);
    QWindow c(&b);

    a.setGeometry(10, 10, 300, 300);
    b.setGeometry(20, 20, 200, 200);
    c.setGeometry(40, 40, 100, 100);

    QCOMPARE(a.mapToGlobal(QPoint(100, 100)), QPoint(110, 110));
    auto delta = a.mapToGlobal(QPointF(100.5, 100.5)) - QPointF(110.5, 110.5);
    QVERIFY(qFuzzyIsNull(delta.manhattanLength()));

    QCOMPARE(b.mapToGlobal(QPoint(100, 100)), QPoint(130, 130));
    QCOMPARE(c.mapToGlobal(QPoint(100, 100)), QPoint(170, 170));

    QCOMPARE(a.mapFromGlobal(QPoint(100, 100)), QPoint(90, 90));
    delta = a.mapFromGlobal(QPointF(100.5, 100.5)) - QPointF(90.5, 90.5);
    QVERIFY(qFuzzyIsNull(delta.manhattanLength()));

    QCOMPARE(b.mapFromGlobal(QPoint(100, 100)), QPoint(70, 70));
    QCOMPARE(c.mapFromGlobal(QPoint(100, 100)), QPoint(30, 30));
}

class Window : public QWindow
{
public:
    Window(const Qt::WindowFlags flags = Qt::Window | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)
        : QWindow(), lastReceivedWindowState(windowState())
    {
        reset();
        setFlags(flags);
#if defined(Q_OS_QNX)
        setSurfaceType(QSurface::OpenGLSurface);
#endif

#if !defined(Q_OS_MACOS)
        // FIXME: All platforms should send window-state change events, regardless
        // of the sync/async nature of the the underlying platform, but they don't.
        connect(this, &QWindow::windowStateChanged, [this]() {
            lastReceivedWindowState = windowState();
        });
#endif
    }

    void reset()
    {
        m_received.clear();
        m_framePositionsOnMove.clear();
    }

    bool event(QEvent *event) override
    {
        m_received[event->type()]++;
        m_order << event->type();
        switch (event->type()) {
        case QEvent::PlatformSurface:
            m_surfaceventType = static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType();
            break;

        case QEvent::Move:
            m_framePositionsOnMove << framePosition();
            break;

        case QEvent::WindowStateChange:
            lastReceivedWindowState = windowState();
            break;

        default:
            break;
        }

        return QWindow::event(event);
    }

    int received(QEvent::Type type)
    {
        return m_received.value(type, 0);
    }

    int eventIndex(QEvent::Type type)
    {
        return m_order.indexOf(type);
    }

    QPlatformSurfaceEvent::SurfaceEventType surfaceEventType() const
    {
        return m_surfaceventType;
    }

    QList<QPoint> m_framePositionsOnMove;
    Qt::WindowStates lastReceivedWindowState;

private:
    QHash<QEvent::Type, int> m_received;
    QList<QEvent::Type> m_order;
    QPlatformSurfaceEvent::SurfaceEventType m_surfaceventType;
};

class ColoredWindow : public QRasterWindow {
public:
    explicit ColoredWindow(const QColor &color, QWindow *parent = nullptr) : QRasterWindow(parent), m_color(color) {}
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.fillRect(QRect(QPoint(0, 0), size()), m_color);
    }

private:
    const QColor m_color;
};

void tst_QWindow::eventOrderOnShow()
{
    // Some platforms enforce minimum widths for windows, which can cause extra resize
    // events, so set the width to suitably large value to avoid those.
    QRect geometry(m_availableTopLeft + QPoint(80, 80), m_testWindowSize);

    Window window;
    window.setTitle(QLatin1String(QTest::currentTestFunction()));
    window.setGeometry(geometry);
    window.show();
    QCoreApplication::processEvents();

    QTRY_COMPARE(window.received(QEvent::Show), 1);
    QTRY_COMPARE(window.received(QEvent::Resize), 1);
    QTRY_VERIFY(window.isExposed());

    QVERIFY(window.eventIndex(QEvent::Show) < window.eventIndex(QEvent::Resize));
    QVERIFY(window.eventIndex(QEvent::Resize) < window.eventIndex(QEvent::Expose));
}

class PaintWindow : public Window
{
public:
    using Window::Window;

protected:
    void paintEvent(QPaintEvent *) override
    {
        // Handled, not calling base class
    }
};

void tst_QWindow::paintEvent()
{
    PaintWindow window;
    window.setTitle(QLatin1String(QTest::currentTestFunction()));
    window.setGeometry(QRect(m_availableTopLeft, m_testWindowSize));
    window.show();

    QTRY_VERIFY(window.received(QEvent::Expose));
    QTRY_VERIFY(window.received(QEvent::Paint));
    QVERIFY(window.isExposed());

    // There is no defined order between paint and expose, so we don't test that

    window.reset();
    window.resize(m_testWindowSize * 2);

    QTRY_VERIFY(window.received(QEvent::Paint));
    QVERIFY(!window.received(QEvent::Expose));

    window.reset();
    window.hide();

    QTRY_VERIFY(window.received(QEvent::Expose));
    QVERIFY(!window.received(QEvent::Paint));
    QVERIFY(!window.isExposed());
}

void tst_QWindow::resizeEventAfterResize()
{
    // Some platforms enforce minimum widths for windows, which can cause extra resize
    // events, so set the width to suitably large value to avoid those.
    QRect geometry(m_availableTopLeft + QPoint(80, 80), m_testWindowSize * 2);

    Window window;
    window.setGeometry(geometry);
    window.showNormal();

    QTRY_COMPARE(window.received(QEvent::Resize), 1);

    // QTBUG-32706
    // Make sure we get a resizeEvent after calling resize
    window.resize(m_testWindowSize);

    if (isPlatformEglFS())
        QEXPECT_FAIL("", "eglfs windows are fullscreen by default.", Continue);

    QTRY_COMPARE(window.received(QEvent::Resize), 2);
}

void tst_QWindow::exposeEventOnShrink_QTBUG54040()
{
    if (isPlatformEglFS())
        QSKIP("", "eglfs windows are fullscreen by default.", Continue);
    Window window;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.setTitle(QTest::currentTestFunction());
    window.showNormal();

    QVERIFY(QTest::qWaitForWindowExposed(&window));

    int exposeCount = window.received(QEvent::Expose);
    window.resize(window.width(), window.height() - 5);
    QTRY_VERIFY(window.received(QEvent::Expose) > exposeCount);

    exposeCount = window.received(QEvent::Expose);
    window.resize(window.width() - 5, window.height());
    QTRY_VERIFY(window.received(QEvent::Expose) > exposeCount);

    exposeCount = window.received(QEvent::Expose);
    window.resize(window.width() - 5, window.height() - 5);
    QTRY_VERIFY(window.received(QEvent::Expose) > exposeCount);
}

// Compare a window position that may go through scaling in the platform plugin with fuzz.
static inline bool qFuzzyCompareWindowPosition(const QPoint &p1, const QPoint p2, int fuzz)
{
    return (p1 - p2).manhattanLength() <= fuzz;
}

static inline bool qFuzzyCompareWindowSize(const QSize &s1, const QSize &s2, int fuzz)
{
    const int manhattanLength = qAbs(s1.width() - s2.width()) + qAbs(s1.height() - s2.height());
    return manhattanLength <= fuzz;
}

static inline bool qFuzzyCompareWindowGeometry(const QRect &r1, const QRect &r2, int fuzz)
{
    return qFuzzyCompareWindowPosition(r1.topLeft(), r2.topLeft(), fuzz)
        && qFuzzyCompareWindowSize(r1.size(), r2.size(), fuzz);
}

static QString msgPointMismatch(const QPoint &p1, const QPoint p2)
{
    QString result;
    QDebug(&result) << p1 << "!=" << p2 << ", manhattanLength=" << (p1 - p2).manhattanLength();
    return result;
}

static QString msgRectMismatch(const QRect &r1, const QRect &r2)
{
    QString result;
    QDebug(&result) << r1 << "!=" << r2;
    return result;
}

void tst_QWindow::positioning()
{
#ifdef Q_OS_ANDROID
    QSKIP("Fails on Android. QTBUG-105201");
#endif
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(
                QPlatformIntegration::NonFullScreenWindows)) {
        QSKIP("This platform does not support non-fullscreen windows");
    }

    if (isPlatformWayland())
        QSKIP("Wayland: This fails. See QTBUG-68660.");

    // Some platforms enforce minimum widths for windows, which can cause extra resize
    // events, so set the width to suitably large value to avoid those.
    const QRect geometry(m_availableTopLeft + QPoint(80, 80), m_testWindowSize);

    Window window;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(20, 20), m_testWindowSize));
    window.setFramePosition(m_availableTopLeft + QPoint(40, 40)); // Move window around before show, size must not change.
    QCOMPARE(window.geometry().size(), m_testWindowSize);
    window.setGeometry(geometry);
    QCOMPARE(window.geometry(), geometry);
    //  explicitly use non-fullscreen show. show() can be fullscreen on some platforms
    window.showNormal();

    QVERIFY(QTest::qWaitForWindowActive(&window));

    QMargins originalMargins = window.frameMargins();

    QCOMPARE(window.position(), window.framePosition() + QPoint(originalMargins.left(), originalMargins.top()));
    QVERIFY(window.frameGeometry().contains(window.geometry()));

    QPoint originalPos = window.position();
    QPoint originalFramePos = window.framePosition();

    window.reset();
    window.setWindowState(Qt::WindowFullScreen);
    QTRY_COMPARE(window.lastReceivedWindowState, Qt::WindowFullScreen);

    QTRY_VERIFY(window.received(QEvent::Resize) > 0);

    window.reset();
    window.setWindowState(Qt::WindowNoState);
    QTRY_COMPARE(window.lastReceivedWindowState, Qt::WindowNoState);

    QTRY_VERIFY(window.received(QEvent::Resize) > 0);

    QTRY_COMPARE(originalPos, window.position());
    QTRY_COMPARE(originalFramePos, window.framePosition());
    QTRY_COMPARE(originalMargins, window.frameMargins());
}

void tst_QWindow::framePositioning_data()
{
    QTest::addColumn<bool>("showBeforePositioning");

    QTest::newRow("before show") << false;
    QTest::newRow("after show") << true;
}

void tst_QWindow::framePositioning()
{
    QFETCH(bool, showBeforePositioning);

    Window window;
    const QScreen *screen = window.screen();
    const QRect availableGeometry = screen->availableGeometry();
    const QPoint screenCenter = availableGeometry.center();

    const QPoint oldFramePos = window.framePosition();
    QMargins originalMargins;

    if (showBeforePositioning) {
        window.showNormal();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        originalMargins = window.frameMargins();
        window.setFramePosition(screenCenter);
    } else {
        window.setFramePosition(screenCenter);
        window.showNormal();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
    }

    QTRY_VERIFY(window.received(QEvent::Move));
    const int fuzz = int(QHighDpiScaling::factor(&window));
    if (!qFuzzyCompareWindowPosition(window.framePosition(), screenCenter, fuzz)) {
        qDebug() << "About to fail auto-test. Here is some additional information:";
        qDebug() << "window.framePosition() == " << window.framePosition();
        qDebug() << "old frame position == " << oldFramePos;
        qDebug() << "We received " << window.received(QEvent::Move) << " move events";
        qDebug() << "frame positions after each move event:" << window.m_framePositionsOnMove;
    }
    QTRY_VERIFY2(qFuzzyCompareWindowPosition(window.framePosition(), screenCenter, fuzz),
                 qPrintable(msgPointMismatch(window.framePosition(), screenCenter)));

    if (showBeforePositioning) {
        // Repositioning should not affect existing margins
        QTRY_COMPARE(originalMargins, window.frameMargins());
        QCOMPARE(window.position(), window.framePosition() + QPoint(originalMargins.left(), originalMargins.top()));
    }

    // Check that regular positioning still works

    const QPoint screenCenterAdjusted = screenCenter + QPoint(50, 50);
    window.reset();
    window.setPosition(screenCenterAdjusted);
    QTRY_VERIFY(window.received(QEvent::Move));
    QTRY_COMPARE(screenCenterAdjusted, window.position());
}

void tst_QWindow::positioningDuringMinimized()
{
    // QTBUG-39544, setting a geometry in minimized state should work as well.
    if (QGuiApplication::platformName().compare("windows", Qt::CaseInsensitive) != 0
        && QGuiApplication::platformName().compare("cocoa", Qt::CaseInsensitive) != 0)
        QSKIP("Not supported on this platform");
    Window window;
    window.setTitle(QStringLiteral("positioningDuringMinimized"));
    const QRect initialGeometry(m_availableTopLeft + QPoint(100, 100), m_testWindowSize);
    window.setGeometry(initialGeometry);
    window.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCOMPARE(window.geometry(), initialGeometry);
    window.setWindowState(Qt::WindowMinimized);
    QCOMPARE(window.geometry(), initialGeometry);
    const QRect newGeometry(initialGeometry.topLeft() + QPoint(50, 50), initialGeometry.size() + QSize(50, 50));
    window.setGeometry(newGeometry);
    QTRY_COMPARE(window.geometry(), newGeometry);
    window.setWindowState(Qt::WindowNoState);
    QTRY_COMPARE(window.geometry(), newGeometry);
}

void tst_QWindow::childWindowPositioning_data()
{
    QTest::addColumn<bool>("showInsteadOfCreate");

    QTest::newRow("create") << false;
    QTest::newRow("show") << true;
}

void tst_QWindow::childWindowPositioning()
{
    if (isPlatformEglFS())
        QSKIP("eglfs does not support child windows.");
    const QPoint topLeftOrigin(0, 0);

    ColoredWindow topLevelWindowFirst(Qt::green);
    topLevelWindowFirst.setObjectName("topLevelWindowFirst");
    ColoredWindow childWindowAfter(Qt::yellow, &topLevelWindowFirst);
    childWindowAfter.setObjectName("childWindowAfter");

    topLevelWindowFirst.setFramePosition(m_availableTopLeft);
    childWindowAfter.setFramePosition(topLeftOrigin);

    ColoredWindow topLevelWindowAfter(Qt::green);
    topLevelWindowAfter.setObjectName("topLevelWindowAfter");
    ColoredWindow childWindowFirst(Qt::yellow, &topLevelWindowAfter);
    childWindowFirst.setObjectName("childWindowFirst");

    topLevelWindowAfter.setFramePosition(m_availableTopLeft);
    childWindowFirst.setFramePosition(topLeftOrigin);

    QFETCH(bool, showInsteadOfCreate);

    QWindow *windows[] = {&topLevelWindowFirst, &childWindowAfter, &childWindowFirst, &topLevelWindowAfter};
    for (QWindow *window : windows) {
        if (showInsteadOfCreate)
            window->showNormal();
        else
            window->create();
    }

    if (showInsteadOfCreate) {
        QVERIFY(QTest::qWaitForWindowExposed(&topLevelWindowFirst));
        QVERIFY(QTest::qWaitForWindowExposed(&topLevelWindowAfter));
    }

    // Creation order shouldn't affect the geometry
    // Use try compare since on X11 the window manager may still re-position the window after expose
    QTRY_COMPARE(topLevelWindowFirst.geometry(), topLevelWindowAfter.geometry());
    QTRY_COMPARE(childWindowAfter.geometry(), childWindowFirst.geometry());

    // Creation order shouldn't affect the child ending up at 0,0
    QCOMPARE(childWindowFirst.framePosition(), topLeftOrigin);
    QCOMPARE(childWindowAfter.framePosition(), topLeftOrigin);
}

void tst_QWindow::childWindowLevel()
{
    ColoredWindow topLevel(Qt::green);
    topLevel.setObjectName("topLevel");
    ColoredWindow yellowChild(Qt::yellow, &topLevel);
    yellowChild.setObjectName("yellowChild");
    ColoredWindow redChild(Qt::red, &topLevel);
    redChild.setObjectName("redChild");
    ColoredWindow blueChild(Qt::blue, &topLevel);
    blueChild.setObjectName("blueChild");

    const QObjectList &siblings = topLevel.children();

    QCOMPARE(siblings.constFirst(), &yellowChild);
    QCOMPARE(siblings.constLast(), &blueChild);

    yellowChild.raise();
    QCOMPARE(siblings.constLast(), &yellowChild);

    blueChild.lower();
    QCOMPARE(siblings.constFirst(), &blueChild);
}

// QTBUG-49709: Verify that the normal geometry is correctly restored
// when executing a sequence of window state changes. So far, Windows
// only where state changes have immediate effect.

typedef QList<Qt::WindowState> WindowStateList;

Q_DECLARE_METATYPE(WindowStateList)

void tst_QWindow::stateChange_data()
{
    QTest::addColumn<WindowStateList>("stateSequence");

    QTest::newRow("normal->min->normal") <<
        (WindowStateList() << Qt::WindowMinimized << Qt::WindowNoState);
    QTest::newRow("normal->maximized->normal") <<
        (WindowStateList() << Qt::WindowMaximized << Qt::WindowNoState);
    QTest::newRow("normal->fullscreen->normal") <<
        (WindowStateList() << Qt::WindowFullScreen << Qt::WindowNoState);
    QTest::newRow("normal->maximized->fullscreen->normal") <<
        (WindowStateList() << Qt::WindowMaximized << Qt::WindowFullScreen << Qt::WindowNoState);
}

void tst_QWindow::stateChange()
{
    QFETCH(WindowStateList, stateSequence);

    if (QGuiApplication::platformName().compare(QLatin1String("windows"), Qt::CaseInsensitive))
        QSKIP("Windows-only test");

    Window window;
    window.setTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1Char(' ') + QLatin1String(QTest::currentDataTag()));
    const QRect normalGeometry(m_availableTopLeft + QPoint(40, 40), m_testWindowSize);
    window.setGeometry(normalGeometry);
    //  explicitly use non-fullscreen show. show() can be fullscreen on some platforms
    window.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    for (Qt::WindowState state : std::as_const(stateSequence)) {
        window.setWindowState(state);
        QCoreApplication::processEvents();
    }
    const QRect geometry = window.geometry();
    const int fuzz = int(QHighDpiScaling::factor(&window));
    QVERIFY2(qFuzzyCompareWindowGeometry(geometry, normalGeometry, fuzz),
             qPrintable(msgRectMismatch(geometry, normalGeometry)));
}

class PlatformWindowFilter : public QObject
{
    Q_OBJECT
public:
    explicit PlatformWindowFilter(Window *window) : m_window(window) {}

    bool eventFilter(QObject *o, QEvent *e) override
    {
        // Check that the platform surface events are delivered synchronously.
        // If they are, the native platform surface should always exist when we
        // receive a QPlatformSurfaceEvent
        if (e->type() == QEvent::PlatformSurface && o == m_window) {
            m_alwaysExisted &= (m_window->handle() != nullptr);
        }
        return false;
    }

    bool surfaceExisted() const { return m_alwaysExisted; }

private:
    Window *m_window;
    bool m_alwaysExisted = true;
};

void tst_QWindow::platformSurface()
{
    QRect geometry(m_availableTopLeft + QPoint(80, 80), m_testWindowSize);

    Window window;
    PlatformWindowFilter filter(&window);
    window.installEventFilter(&filter);

    window.setGeometry(geometry);
    QCOMPARE(window.geometry(), geometry);
    window.create();

    QTRY_COMPARE(window.received(QEvent::PlatformSurface), 1);
    QTRY_COMPARE(window.surfaceEventType(), QPlatformSurfaceEvent::SurfaceCreated);
    QTRY_VERIFY(window.handle() != nullptr);

    window.destroy();
    QTRY_COMPARE(window.received(QEvent::PlatformSurface), 2);
    QTRY_COMPARE(window.surfaceEventType(), QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed);
    QTRY_VERIFY(!window.handle());

    // Check for synchronous delivery of platform surface events and that the platform
    // surface always existed upon event delivery
    QTRY_VERIFY(filter.surfaceExisted());
}

void tst_QWindow::isExposed()
{
    QRect geometry(m_availableTopLeft + QPoint(80, 80), m_testWindowSize);

    Window window;
    window.setTitle(QLatin1String(QTest::currentTestFunction()));
    window.setGeometry(geometry);
    QCOMPARE(window.geometry(), geometry);
    window.show();
    QCoreApplication::processEvents();

    QTRY_VERIFY(window.received(QEvent::Expose) > 0);
    QTRY_VERIFY(window.isExposed());

    window.hide();

    QCoreApplication::processEvents();
    QTRY_VERIFY(window.received(QEvent::Expose) > 1);
    QTRY_VERIFY(!window.isExposed());
}


void tst_QWindow::isActive()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("QWindow::requestActivate() is not supported.");

    Window window;
    window.setTitle(QLatin1String(QTest::currentTestFunction()));
    // Some platforms enforce minimum widths for windows, which can cause extra resize
    // events, so set the width to suitably large value to avoid those.
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.show();
    QCoreApplication::processEvents();

    QTRY_VERIFY(window.isExposed());
#if defined(Q_OS_QNX) // We either need to create a eglSurface or a create a backing store
                      // and then post the window in order for screen to show the window
    QOpenGLContext context;
    context.create();
    context.makeCurrent(&window);
    context.swapBuffers(&window);
#endif
    QTRY_COMPARE(window.received(QEvent::Resize), 1);
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);
    QVERIFY(window.isActive());

    Window child;
    child.setParent(&window);
    child.setGeometry(10, 10, 20, 20);
    child.show();

    QTRY_VERIFY(child.isExposed());

    child.requestActivate();

    QTRY_COMPARE(QGuiApplication::focusWindow(), &child);
    QVERIFY(child.isActive());

    // parent shouldn't receive new resize events from child being shown
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.received(QEvent::Resize), 1);
    QTRY_COMPARE(window.received(QEvent::FocusIn), 1);
    QTRY_COMPARE(window.received(QEvent::FocusOut), 1);
    QTRY_COMPARE(child.received(QEvent::FocusIn), 1);

    // child has focus
    QVERIFY(window.isActive());

    // test focus back to parent and then back to child (QTBUG-39362)
    // also verify the cumulative FocusOut and FocusIn counts
    // activate parent
    window.requestActivate();
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);
    QVERIFY(window.isActive());
    QCoreApplication::processEvents();
    QTRY_COMPARE(child.received(QEvent::FocusOut), 1);
    QTRY_COMPARE(window.received(QEvent::FocusIn), 2);

    // activate child again
    child.requestActivate();
    QTRY_COMPARE(QGuiApplication::focusWindow(), &child);
    QVERIFY(child.isActive());
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.received(QEvent::FocusOut), 2);
    QTRY_COMPARE(child.received(QEvent::FocusIn), 2);

    Window dialog;
    dialog.setTransientParent(&window);
    dialog.setGeometry(QRect(m_availableTopLeft + QPoint(110, 100), m_testWindowSize));
    dialog.show();

    dialog.requestActivate();

    QTRY_VERIFY(dialog.isExposed());
    QCoreApplication::processEvents();
    QTRY_COMPARE(dialog.received(QEvent::Resize), 1);
    QTRY_COMPARE(QGuiApplication::focusWindow(), &dialog);
    QVERIFY(dialog.isActive());

    // transient child has focus
    QVERIFY(window.isActive());

    // parent is active
    QVERIFY(child.isActive());

    window.requestActivate();

    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);
    QCoreApplication::processEvents();
    QTRY_COMPARE(dialog.received(QEvent::FocusOut), 1);
    // We should be checking for exactly three, but since this is a try-compare _loop_, we might
    // loose and regain focus multiple times in the event of a system popup. This has been observed
    // to fail on Windows, see QTBUG-77769.
    QTRY_VERIFY2(window.received(QEvent::FocusIn) >= 3,
                 qPrintable(
                 QStringLiteral("Expected more than three focus in events, received: %1")
                 .arg(window.received(QEvent::FocusIn))));

    QVERIFY(window.isActive());

    // transient parent has focus
    QVERIFY(dialog.isActive());

    // parent has focus
    QVERIFY(child.isActive());
}

class InputTestWindow : public ColoredWindow
{
public:
    void keyPressEvent(QKeyEvent *event) override
    {
        keyPressCode = event->key();
    }
    void keyReleaseEvent(QKeyEvent *event) override
    {
        keyReleaseCode = event->key();
    }
    void mousePressEvent(QMouseEvent *event) override
    {
        qCDebug(lcTests) << event;
        mouseDevice = event->pointingDevice();
        if (ignoreMouse) {
            event->ignore();
        } else {
            QCOMPARE(event->isBeginEvent(), true);
            QCOMPARE(event->isUpdateEvent(), false);
            QCOMPARE(event->isEndEvent(), false);
            QCOMPARE(event->points().first().state(), QEventPoint::State::Pressed);
            ++mousePressedCount;
            mouseSequenceSignature += 'p';
            mousePressButton = event->button();
            mousePressScreenPos = event->globalPosition();
            mousePressLocalPos = event->position();
            if (spinLoopWhenPressed)
                QCoreApplication::processEvents();
        }
        if (closeOnTap)
            this->close();

    }
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        qCDebug(lcTests) << event;
        if (ignoreMouse) {
            event->ignore();
        } else {
            QCOMPARE(event->isBeginEvent(), false);
            QCOMPARE(event->isUpdateEvent(), false);
            QCOMPARE(event->isEndEvent(), true);
            QCOMPARE(event->points().first().state(), QEventPoint::State::Released);
            ++mouseReleasedCount;
            mouseSequenceSignature += 'r';
            mouseReleaseButton = event->button();
        }
    }
    void mouseMoveEvent(QMouseEvent *event) override
    {
        qCDebug(lcTests) << event;
        buttonStateInGeneratedMove = event->buttons();
        if (ignoreMouse) {
            event->ignore();
        } else {
            QCOMPARE(event->isBeginEvent(), false);
            QCOMPARE(event->isUpdateEvent(), true);
            QCOMPARE(event->isEndEvent(), false);
            QCOMPARE(event->points().first().state(), QEventPoint::State::Updated);
            ++mouseMovedCount;
            mouseMoveButton = event->button();
            mouseMoveScreenPos = event->globalPosition();
        }
    }
    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        qCDebug(lcTests) << event;
        if (ignoreMouse) {
            event->ignore();
        } else {
            QCOMPARE(event->isBeginEvent(), false);
            QCOMPARE(event->isUpdateEvent(), true);
            QCOMPARE(event->isEndEvent(), false);
            QCOMPARE(event->points().first().state(), QEventPoint::State::Stationary);
            ++mouseDoubleClickedCount;
            mouseSequenceSignature += 'd';
        }
    }
    void touchEvent(QTouchEvent *event) override
    {
        qCDebug(lcTests) << event;
        touchDevice = event->pointingDevice();
        if (ignoreTouch) {
            event->ignore();
            return;
        }
        touchEventType = event->type();
        QList<QTouchEvent::TouchPoint> points = event->points();
        for (int i = 0; i < points.size(); ++i) {
            const auto &point = points.at(i);
            switch (point.state()) {
            case QEventPoint::State::Pressed:
                ++touchPressedCount;
                if (spinLoopWhenPressed)
                    QCoreApplication::processEvents();
                if (i == 0) {
                    touchPressLocalPos = point.position();
                    touchPressGlobalPos = point.globalPosition();
                }
                if (closeOnTap)
                    this->close();
                break;
            case QEventPoint::State::Released:
                ++touchReleasedCount;
                break;
            case QEventPoint::State::Updated:
                ++touchMovedCount;
                break;
            default:
                break;
            }
        }
    }
    bool event(QEvent *e) override
    {
        switch (e->type()) {
        case QEvent::Enter:
            qCDebug(lcTests) << e;
            ++enterEventCount;
            break;
        case QEvent::Leave:
            qCDebug(lcTests) << e;
            ++leaveEventCount;
            break;
        default:
            break;
        }
        return ColoredWindow::event(e);
    }
    void resetCounters()
    {
        mousePressedCount = mouseReleasedCount = mouseMovedCount = mouseDoubleClickedCount = 0;
        mouseSequenceSignature.clear();
        touchPressedCount = touchReleasedCount = touchMovedCount = 0;
        enterEventCount = leaveEventCount = 0;
    }

    explicit InputTestWindow(const QColor &color = Qt::white, QWindow *parent = nullptr)
        : ColoredWindow(color, parent) {}

    int keyPressCode = 0, keyReleaseCode = 0;
    int mousePressButton = 0, mouseReleaseButton = 0, mouseMoveButton = 0;
    int mousePressedCount = 0, mouseReleasedCount = 0, mouseMovedCount = 0, mouseDoubleClickedCount = 0;
    QString mouseSequenceSignature;
    QPointF mousePressScreenPos, mouseMoveScreenPos, mousePressLocalPos;
    QPointF touchPressGlobalPos, touchPressLocalPos;
    int touchPressedCount = 0, touchReleasedCount = 0, touchMovedCount = 0;
    QEvent::Type touchEventType = QEvent::None;
    int enterEventCount = 0, leaveEventCount = 0;

    bool ignoreMouse = false, ignoreTouch = false;

    bool spinLoopWhenPressed = false;
    Qt::MouseButtons buttonStateInGeneratedMove;

    const QPointingDevice *mouseDevice = nullptr;
    const QPointingDevice *touchDevice = nullptr;

    bool closeOnTap = false;
};

static void simulateMouseClick(QWindow *target, const QPointF &local, const QPointF &global)
{
    QWindowSystemInterface::handleMouseEvent(target, local, global,
                                             Qt::LeftButton, Qt::LeftButton, QEvent::MouseButtonPress);
    QWindowSystemInterface::handleMouseEvent(target, local, global,
                                             {}, Qt::LeftButton, QEvent::MouseButtonRelease);
}

static void simulateMouseClick(QWindow *target, ulong &timeStamp,
                               const QPointF &local, const QPointF &global)
{
    QWindowSystemInterface::handleMouseEvent(target, timeStamp++, local, global,
                                             Qt::LeftButton, Qt::LeftButton, QEvent::MouseButtonPress);
    QWindowSystemInterface::handleMouseEvent(target, timeStamp++, local, global,
                                             {}, Qt::LeftButton, QEvent::MouseButtonRelease);
}

void tst_QWindow::testInputEvents()
{
    InputTestWindow window;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QTest::keyClick(&window, Qt::Key_A, Qt::NoModifier);
    QCoreApplication::processEvents();
    QCOMPARE(window.keyPressCode, int(Qt::Key_A));
    QCOMPARE(window.keyReleaseCode, int(Qt::Key_A));

    QPointF local(12, 34);
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, local.toPoint());
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressButton, int(Qt::LeftButton));
    QCOMPARE(window.mouseReleaseButton, int(Qt::LeftButton));
    QCOMPARE(window.mousePressLocalPos, local);

    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp1, tp2;
    tp1.id = 1;
    tp1.state = QEventPoint::State::Pressed;
    tp1.area = QRect(10, 10, 4, 4);
    tp2.id = 2;
    tp2.state = QEventPoint::State::Pressed;
    tp2.area = QRect(20, 20, 4, 4);
    points << tp1 << tp2;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    points[0].state = QEventPoint::State::Released;
    points[1].state = QEventPoint::State::Released;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchPressedCount, 2);
    QTRY_COMPARE(window.touchReleasedCount, 2);

    // Now with null pointer as window. local param should not be utilized:
    // handleMouseEvent() with tlw == 0 means the event is in global coords only.
    window.mousePressButton = window.mouseReleaseButton = 0;
    const QPointF nonWindowGlobal(window.geometry().topRight() + QPoint(200, 50)); // not inside the window
    const QPointF deviceNonWindowGlobal = QHighDpi::toNativePixels(nonWindowGlobal, window.screen());
    simulateMouseClick(nullptr, deviceNonWindowGlobal, deviceNonWindowGlobal);
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressButton, 0);
    QCOMPARE(window.mouseReleaseButton, 0);
    const QPointF windowGlobal = window.mapToGlobal(local.toPoint());
    const QPointF deviceWindowGlobal = QHighDpi::toNativePixels(windowGlobal, window.screen());
    simulateMouseClick(nullptr, deviceWindowGlobal, deviceWindowGlobal);
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressButton, int(Qt::LeftButton));
    QCOMPARE(window.mouseReleaseButton, int(Qt::LeftButton));
    QCOMPARE(window.mousePressScreenPos, windowGlobal);
    QCOMPARE(window.mousePressLocalPos, local); // the local we passed was bogus, verify that qGuiApp calculated the proper one
}

void tst_QWindow::touchToMouseTranslation()
{
    InputTestWindow window;
    window.setTitle(QLatin1String(QTest::currentTestFunction()));
    window.ignoreTouch = true;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp1, tp2;
    const QRectF pressArea(101, 102, 4, 4);
    const QRectF moveArea(105, 108, 4, 4);
    tp1.id = 1;
    tp1.state = QEventPoint::State::Pressed;
    tp1.area = QHighDpi::toNativePixels(pressArea, &window);
    tp2.id = 2;
    tp2.state = QEventPoint::State::Pressed;
    points << tp1 << tp2;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    // Now an update but with changed list order. The mouse event should still
    // be generated from the point with id 1.
    tp1.id = 2;
    tp1.state = QEventPoint::State::Stationary;
    tp2.id = 1;
    tp2.state = QEventPoint::State::Updated;
    tp2.area = QHighDpi::toNativePixels(moveArea, &window);
    points.clear();
    points << tp1 << tp2;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    points[0].state = QEventPoint::State::Released;
    points[1].state = QEventPoint::State::Released;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();

    QTRY_COMPARE(window.mousePressButton, int(Qt::LeftButton));
    QTRY_COMPARE(window.mouseReleaseButton, int(Qt::LeftButton));
    QTRY_COMPARE(window.mousePressScreenPos, pressArea.center());
    QTRY_COMPARE(window.mouseMoveScreenPos, moveArea.center());
    QCOMPARE(window.mouseDevice, window.touchDevice);
    QCOMPARE(window.mouseDevice->type(), QInputDevice::DeviceType::TouchScreen);

    window.mousePressButton = 0;
    window.mouseReleaseButton = 0;

    window.ignoreTouch = false;

    points[0].state = QEventPoint::State::Pressed;
    points[1].state = QEventPoint::State::Pressed;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    points[0].state = QEventPoint::State::Released;
    points[1].state = QEventPoint::State::Released;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();

    // no new mouse events should be generated since the input window handles the touch events
    QTRY_COMPARE(window.mousePressButton, 0);
    QTRY_COMPARE(window.mouseReleaseButton, 0);

    QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, false);

    window.ignoreTouch = true;
    points[0].state = QEventPoint::State::Pressed;
    points[1].state = QEventPoint::State::Pressed;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    points[0].state = QEventPoint::State::Released;
    points[1].state = QEventPoint::State::Released;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();

    QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, true);

    // mouse event synthesizing disabled
    QTRY_COMPARE(window.mousePressButton, 0);
    QTRY_COMPARE(window.mouseReleaseButton, 0);

    points.clear();
    points.append(tp2);
    points[0].state = QEventPoint::State::Pressed;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    points.clear();
    points.append(tp1);
    points[0].state = QEventPoint::State::Pressed;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.mousePressButton, 1);

    points.clear();
    points.append(tp2);
    points[0].state = QEventPoint::State::Released;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    points.clear();
    points.append(tp1);
    points[0].state = QEventPoint::State::Released;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.mouseReleaseButton, 1);
}

void tst_QWindow::touchToMouseTranslationForDevices()
{
    InputTestWindow window;
    window.setTitle(QLatin1String(QTest::currentTestFunction()));
    window.ignoreTouch = true;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QPoint touchPoint(10, 10);

    QTest::touchEvent(&window, touchDevice).press(0, touchPoint, &window);
    QTest::touchEvent(&window, touchDevice).release(0, touchPoint, &window);
    QCoreApplication::processEvents();

    QCOMPARE(window.mousePressedCount, 1);
    QCOMPARE(window.mouseReleasedCount, 1);

    window.resetCounters();

    QTest::touchEvent(&window, touchDeviceWithMouseEmulation).press(0, touchPoint, &window);
    QTest::touchEvent(&window, touchDeviceWithMouseEmulation).release(0, touchPoint, &window);
    QCoreApplication::processEvents();

    QCOMPARE(window.mousePressedCount, 0);
    QCOMPARE(window.mouseReleasedCount, 0);
}

void tst_QWindow::mouseToTouchTranslation()
{
    QCoreApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, true);

    InputTestWindow window;
    window.setTitle(QLatin1String(QTest::currentTestFunction()));
    window.ignoreMouse = true;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    const QPoint localPos(10, 10);
    QTest::mouseClick(&window, Qt::LeftButton, {}, localPos);
    QCoreApplication::processEvents();

    QCoreApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, false);

    QTRY_COMPARE(window.touchPressedCount, 1);
    QTRY_COMPARE(window.touchReleasedCount, 1);
    QCOMPARE(window.mouseDevice, window.touchDevice);
    if (isPlatformWayland())
        QEXPECT_FAIL("", "Wayland: This fails. See QTBUG-100887.", Abort);
    QCOMPARE(window.touchDevice->type(), QInputDevice::DeviceType::Mouse);
    QCOMPARE(window.touchPressLocalPos.toPoint(), localPos);
    QCOMPARE(window.touchPressGlobalPos.toPoint(), window.mapToGlobal(localPos));

    QCoreApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, true);

    window.ignoreMouse = false;

    QTest::mouseClick(&window, Qt::LeftButton, {}, localPos);
    QCoreApplication::processEvents();

    QCoreApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, false);

    // no new touch events should be generated since the input window handles the mouse events
    QTRY_COMPARE(window.touchPressedCount, 1);
    QTRY_COMPARE(window.touchReleasedCount, 1);

    window.ignoreMouse = true;

    QTest::mouseClick(&window, Qt::LeftButton, {}, localPos);
    QCoreApplication::processEvents();

    // touch event synthesis disabled
    QTRY_COMPARE(window.touchPressedCount, 1);
    QTRY_COMPARE(window.touchReleasedCount, 1);
}

void tst_QWindow::mouseToTouchLoop()
{
    // make sure there's no infinite loop when synthesizing both ways
    QCoreApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, true);
    QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, true);

    InputTestWindow window;
    window.setTitle(QLatin1String(QTest::currentTestFunction()));

    window.ignoreMouse = true;
    window.ignoreTouch = true;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QTest::mouseClick(&window, Qt::LeftButton, {}, QPoint(10, 10));
    QCoreApplication::processEvents();

    QCoreApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, false);
    QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, true);
}

void tst_QWindow::touchCancel()
{
    InputTestWindow window;
    window.setTitle(QLatin1String(QTest::currentTestFunction()));
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp1;
    tp1.id = 1;

    // Start a touch.
    tp1.state = QEventPoint::State::Pressed;
    tp1.area = QRect(10, 10, 4, 4);
    points << tp1;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchEventType, QEvent::TouchBegin);
    QTRY_COMPARE(window.touchPressedCount, 1);

    // Cancel the active touch sequence.
    QWindowSystemInterface::handleTouchCancelEvent(&window, touchDevice);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchEventType, QEvent::TouchCancel);

    // Send a move -> will not be delivered due to the cancellation.
    QTRY_COMPARE(window.touchMovedCount, 0);
    points[0].state = QEventPoint::State::Updated;
    tp1.area.adjust(2, 2, 2, 2);
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchMovedCount, 0);

    // Likewise. The only allowed transition is TouchCancel -> TouchBegin.
    QTRY_COMPARE(window.touchReleasedCount, 0);
    points[0].state = QEventPoint::State::Released;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchReleasedCount, 0);

    // Start a new sequence -> from this point on everything should go through normally.
    points[0].state = QEventPoint::State::Pressed;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchEventType, QEvent::TouchBegin);
    QTRY_COMPARE(window.touchPressedCount, 2);

    points[0].state = QEventPoint::State::Updated;
    tp1.area.adjust(2, 2, 2, 2);
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchMovedCount, 1);

    points[0].state = QEventPoint::State::Released;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchReleasedCount, 1);
}

void tst_QWindow::touchCancelWithTouchToMouse()
{
    InputTestWindow window;
    window.setTitle(QLatin1String(QTest::currentTestFunction()));
    window.ignoreTouch = true;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp1;
    tp1.id = 1;

    tp1.state = QEventPoint::State::Pressed;
    const QRect area(100, 100, 4, 4);
    tp1.area = QHighDpi::toNativePixels(area, &window);
    points << tp1;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.mousePressButton, int(Qt::LeftButton));
    const int fuzz = 2 * int(QHighDpiScaling::factor(&window));
    QTRY_VERIFY2(qFuzzyCompareWindowPosition(window.mousePressScreenPos.toPoint(), area.center(), fuzz),
                 qPrintable(msgPointMismatch(window.mousePressScreenPos.toPoint(), area.center())));

    // Cancel the touch. Should result in a mouse release for windows that have
    // have an active touch-to-mouse sequence.
    QWindowSystemInterface::handleTouchCancelEvent(nullptr, touchDevice);
    QCoreApplication::processEvents();

    QTRY_COMPARE(window.mouseReleaseButton, int(Qt::LeftButton));

    // Now change the window to accept touches.
    window.mousePressButton = window.mouseReleaseButton = 0;
    window.ignoreTouch = false;

    // Send a touch, there will be no mouse event generated.
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.mousePressButton, 0);

    // Cancel the touch. It should not result in a mouse release with this window.
    QWindowSystemInterface::handleTouchCancelEvent(nullptr, touchDevice);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.mouseReleaseButton, 0);
}

void tst_QWindow::touchInterruptedByPopup()
{
    InputTestWindow window;
    window.setTitle(QLatin1String(QTest::currentTestFunction()));
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp1;
    tp1.id = 1;

    // Start a touch.
    tp1.state = QEventPoint::State::Pressed;
    tp1.area = QRect(10, 10, 4, 4);
    points << tp1;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchEventType, QEvent::TouchBegin);
    QTRY_COMPARE(window.touchPressedCount, 1);

    // Launch a popup window
    InputTestWindow popup;
    popup.setFlags(Qt::Popup);
    popup.setModality(Qt::WindowModal);
    popup.resize(m_testWindowSize /  2);
    popup.setTransientParent(&window);
    popup.show();
    QVERIFY(QTest::qWaitForWindowExposed(&popup));

    // Send a move -> will not be delivered to the original window
    // (TODO verify where it is forwarded, after we've defined that)
    QTRY_COMPARE(window.touchMovedCount, 0);
    points[0].state = QEventPoint::State::Updated;
    tp1.area.adjust(2, 2, 2, 2);
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchMovedCount, 0);

    // Send a touch end -> will not be delivered to the original window
    QTRY_COMPARE(window.touchReleasedCount, 0);
    points[0].state = QEventPoint::State::Released;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.touchReleasedCount, 0);

    // Due to temporary fix for QTBUG-37371: the original window should receive a TouchCancel
    QTRY_COMPARE(window.touchEventType, QEvent::TouchCancel);
}

void tst_QWindow::orientation()
{
    qRegisterMetaType<Qt::ScreenOrientation>("Qt::ScreenOrientation");

    QWindow window;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.create();

    window.reportContentOrientationChange(Qt::PortraitOrientation);
    QCOMPARE(window.contentOrientation(), Qt::PortraitOrientation);

    window.reportContentOrientationChange(Qt::PrimaryOrientation);
    QCOMPARE(window.contentOrientation(), Qt::PrimaryOrientation);

    QSignalSpy spy(&window, SIGNAL(contentOrientationChanged(Qt::ScreenOrientation)));
    window.reportContentOrientationChange(Qt::LandscapeOrientation);
    QCOMPARE(spy.size(), 1);
}

void tst_QWindow::sizes()
{
    QWindow window;

    QSignalSpy minimumWidthSpy(&window, SIGNAL(minimumWidthChanged(int)));
    QSignalSpy minimumHeightSpy(&window, SIGNAL(minimumHeightChanged(int)));
    QSignalSpy maximumWidthSpy(&window, SIGNAL(maximumWidthChanged(int)));
    QSignalSpy maximumHeightSpy(&window, SIGNAL(maximumHeightChanged(int)));

    QSize oldMaximum = window.maximumSize();

    window.setMinimumWidth(10);
    QCOMPARE(window.minimumWidth(), 10);
    QCOMPARE(window.minimumHeight(), 0);
    QCOMPARE(window.minimumSize(), QSize(10, 0));
    QCOMPARE(window.maximumSize(), oldMaximum);
    QCOMPARE(minimumWidthSpy.size(), 1);
    QCOMPARE(minimumHeightSpy.size(), 0);
    QCOMPARE(maximumWidthSpy.size(), 0);
    QCOMPARE(maximumHeightSpy.size(), 0);

    window.setMinimumHeight(10);
    QCOMPARE(window.minimumWidth(), 10);
    QCOMPARE(window.minimumHeight(), 10);
    QCOMPARE(window.minimumSize(), QSize(10, 10));
    QCOMPARE(window.maximumSize(), oldMaximum);
    QCOMPARE(minimumWidthSpy.size(), 1);
    QCOMPARE(minimumHeightSpy.size(), 1);
    QCOMPARE(maximumWidthSpy.size(), 0);
    QCOMPARE(maximumHeightSpy.size(), 0);

    window.setMaximumWidth(100);
    QCOMPARE(window.maximumWidth(), 100);
    QCOMPARE(window.maximumHeight(), oldMaximum.height());
    QCOMPARE(window.minimumSize(), QSize(10, 10));
    QCOMPARE(window.maximumSize(), QSize(100, oldMaximum.height()));
    QCOMPARE(minimumWidthSpy.size(), 1);
    QCOMPARE(minimumHeightSpy.size(), 1);
    QCOMPARE(maximumWidthSpy.size(), 1);
    QCOMPARE(maximumHeightSpy.size(), 0);

    window.setMaximumHeight(100);
    QCOMPARE(window.maximumWidth(), 100);
    QCOMPARE(window.maximumHeight(), 100);
    QCOMPARE(window.minimumSize(), QSize(10, 10));
    QCOMPARE(window.maximumSize(), QSize(100, 100));
    QCOMPARE(minimumWidthSpy.size(), 1);
    QCOMPARE(minimumHeightSpy.size(), 1);
    QCOMPARE(maximumWidthSpy.size(), 1);
    QCOMPARE(maximumHeightSpy.size(), 1);

    // test if min and max limits will change the size
    QVERIFY(window.minimumWidth() < 50 && window.maximumWidth() > 80);
    QVERIFY(window.minimumHeight() < 50 && window.maximumHeight() > 80);
    window.resize(50, 50);
    QCOMPARE(window.size(), QSize(50, 50));
    window.setMinimumSize(QSize(60, 60));
    QCOMPARE(window.size(), QSize(60, 60));
    window.resize(80, 80);
    window.setMaximumSize(QSize(70, 70));
    QCOMPARE(window.size(), QSize(70, 70));

    // QTBUG-113233
    // test for an invalid min/max pair
    window.setMinimumSize(QSize(80, 80)); // current maximumSize = QSize(70, 70)
    QCOMPARE(window.size(), QSize(70, 70));
    window.setMaximumSize(QSize(90, 90));
    QCOMPARE(window.size(), QSize(80, 80));
}

class CloseOnCloseEventWindow : public QWindow
{
public:
    inline static int closeEvents;
    CloseOnCloseEventWindow() { closeEvents = 0; }

protected:
    void closeEvent(QCloseEvent *e) override
    {
        if (++closeEvents > 1)
            return;

        close();
        e->accept();
    }
};

void tst_QWindow::close()
{
    {
        QWindow a;
        QWindow b;
        QWindow c(&a);

        a.show();
        b.show();

        // we can not close a non top level window
        QVERIFY(!c.close());
        QVERIFY(a.close());
        QVERIFY(b.close());
    }

    // Verify that closing a QWindow deletes its platform window,
    // independent of API used to close the window.
    {
        // Close with QWindow::close
        {
            QWindow w;
            w.create();
            QVERIFY(w.handle());
            w.close();
            QVERIFY(!w.handle());
        }

        // Close with QWindowSystemInterface::handleCloseEvent();
        {
            QWindow w;
            w.create();
            QVERIFY(w.handle());
            QWindowSystemInterface::handleCloseEvent(&w);
            QCoreApplication::processEvents();
            QVERIFY(!w.handle());
        }
    }

    // Verify that closing a QWindow deletes the platform window for
    // child windows
    {
        QWindow w;
        QWindow c(&w);
        w.create();
        c.create();
        QVERIFY(w.handle());
        QVERIFY(c.handle());
        w.close();
        QVERIFY(!w.handle());
        QVERIFY(!c.handle());
    }

    // Verify that re-creating closed windows is possble.
    {
        // Re-create top-level window
        {
            QWindow w;
            w.create();
            QVERIFY(w.handle());
            w.close();
            QVERIFY(!w.handle());
            w.create();
            QVERIFY(w.handle());
        }

        // Re-create top-level window with child window
        {
            QWindow w;
            QWindow c(&w);
            c.create();
            QVERIFY(w.handle());
            QVERIFY(c.handle());
            w.close();
            QVERIFY(!w.handle());
            QVERIFY(!c.handle());
            c.create();
            QVERIFY(w.handle());
            QVERIFY(c.handle());
        }
    }

    {
        // A QWidget will call close() from the destructor, and
        // we allow widgets deleting itself in the closeEvent,
        // so we need to guard against close being called recursively.
        CloseOnCloseEventWindow w;
        w.create();
        w.close();
        QCOMPARE(CloseOnCloseEventWindow::closeEvents, 1);
    }
}

void tst_QWindow::activateAndClose()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("QWindow::requestActivate() is not supported.");

    for (int i = 0; i < 10; ++i) {
       QWindow window;
       window.setTitle(QLatin1String(QTest::currentTestFunction()) + QString::number(i));
#if defined(Q_OS_QNX)
       window.setSurfaceType(QSurface::OpenGLSurface);
#endif
       // qWaitForWindowActive will block for the duration of
       // of the timeout if the window is at 0,0
       window.setGeometry(QGuiApplication::primaryScreen()->availableGeometry().adjusted(1, 1, -1, -1));
       window.showNormal();
#if defined(Q_OS_QNX) // We either need to create a eglSurface or a create a backing store
                      // and then post the window in order for screen to show the window
       QVERIFY(QTest::qWaitForWindowExposed(&window));
       QOpenGLContext context;
       context.create();
       context.makeCurrent(&window);
       context.swapBuffers(&window);
#endif
       window.requestActivate();
       QVERIFY(QTest::qWaitForWindowActive(&window));
       QCOMPARE(QGuiApplication::focusWindow(), &window);
    }
}

void tst_QWindow::mouseEventSequence()
{
    const auto doubleClickInterval = ulong(QGuiApplication::styleHints()->mouseDoubleClickInterval());

    InputTestWindow window;
    window.setTitle(QLatin1String(QTest::currentTestFunction()));
    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    ulong timestamp = 0;
    QPointF local(12, 34);
    const QPointF deviceLocal = QHighDpi::toNativePixels(local, &window);

    simulateMouseClick(&window, timestamp, deviceLocal, deviceLocal);
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressedCount, 1);
    QCOMPARE(window.mouseReleasedCount, 1);
    QCOMPARE(window.mouseDoubleClickedCount, 0);
    QCOMPARE(window.mouseSequenceSignature, QLatin1String("pr"));

    window.resetCounters();
    timestamp += doubleClickInterval;

    // A double click must result in press, release, press, doubleclick, release.
    // Check that no unexpected event suppression occurs and that the order is correct.
    simulateMouseClick(&window, timestamp, local, local);
    simulateMouseClick(&window, timestamp, local, local);
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressedCount, 2);
    QCOMPARE(window.mouseReleasedCount, 2);
    QCOMPARE(window.mouseDoubleClickedCount, 1);
    QCOMPARE(window.mouseSequenceSignature, QLatin1String("prpdr"));

    timestamp += doubleClickInterval;
    window.resetCounters();

    // Triple click = double + single click
    simulateMouseClick(&window, timestamp, local, local);
    simulateMouseClick(&window, timestamp, local, local);
    simulateMouseClick(&window, timestamp, local, local);
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressedCount, 3);
    QCOMPARE(window.mouseReleasedCount, 3);
    QCOMPARE(window.mouseDoubleClickedCount, 1);
    QCOMPARE(window.mouseSequenceSignature, QLatin1String("prpdrpr"));

    timestamp += doubleClickInterval;
    window.resetCounters();

    // Two double clicks.
    simulateMouseClick(&window, timestamp, local, local);
    simulateMouseClick(&window, timestamp, local, local);
    simulateMouseClick(&window, timestamp, local, local);
    simulateMouseClick(&window, timestamp, local, local);
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressedCount, 4);
    QCOMPARE(window.mouseReleasedCount, 4);
    QCOMPARE(window.mouseDoubleClickedCount, 2);
    QCOMPARE(window.mouseSequenceSignature, QLatin1String("prpdrprpdr"));

    timestamp += doubleClickInterval;
    window.resetCounters();

    // Four clicks, none of which qualifies as a double click.
    simulateMouseClick(&window, timestamp, local, local);
    timestamp += doubleClickInterval;
    simulateMouseClick(&window, timestamp, local, local);
    timestamp += doubleClickInterval;
    simulateMouseClick(&window, timestamp, local, local);
    timestamp += doubleClickInterval;
    simulateMouseClick(&window, timestamp, local, local);
    timestamp += doubleClickInterval;
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressedCount, 4);
    QCOMPARE(window.mouseReleasedCount, 4);
    QCOMPARE(window.mouseDoubleClickedCount, 0);
    QCOMPARE(window.mouseSequenceSignature, QLatin1String("prprprpr"));
}

void tst_QWindow::windowModality()
{
    qRegisterMetaType<Qt::WindowModality>("Qt::WindowModality");

    QWindow window;
    QSignalSpy spy(&window, SIGNAL(modalityChanged(Qt::WindowModality)));

    QCOMPARE(window.modality(), Qt::NonModal);
    window.setModality(Qt::NonModal);
    QCOMPARE(window.modality(), Qt::NonModal);
    QCOMPARE(spy.size(), 0);

    window.setModality(Qt::WindowModal);
    QCOMPARE(window.modality(), Qt::WindowModal);
    QCOMPARE(spy.size(), 1);
    window.setModality(Qt::WindowModal);
    QCOMPARE(window.modality(), Qt::WindowModal);
    QCOMPARE(spy.size(), 1);

    window.setModality(Qt::ApplicationModal);
    QCOMPARE(window.modality(), Qt::ApplicationModal);
    QCOMPARE(spy.size(), 2);
    window.setModality(Qt::ApplicationModal);
    QCOMPARE(window.modality(), Qt::ApplicationModal);
    QCOMPARE(spy.size(), 2);

    window.setModality(Qt::NonModal);
    QCOMPARE(window.modality(), Qt::NonModal);
    QCOMPARE(spy.size(), 3);
}

void tst_QWindow::inputReentrancy()
{
    InputTestWindow window;
    window.setTitle(QLatin1String(QTest::currentTestFunction()));
    window.spinLoopWhenPressed = true;

    window.setGeometry(QRect(m_availableTopLeft + QPoint(80, 80), m_testWindowSize));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // Queue three events.
    QPointF local(12, 34);
    QWindowSystemInterface::handleMouseEvent(&window, local, local, Qt::LeftButton,
                                             Qt::LeftButton, QEvent::MouseButtonPress);
    local += QPointF(2, 2);
    QWindowSystemInterface::handleMouseEvent(&window, local, local,
                                             Qt::LeftButton, {}, QEvent::MouseMove);
    QWindowSystemInterface::handleMouseEvent(&window, local, local, {},
                                             Qt::LeftButton, QEvent::MouseButtonRelease);
    // Process them. However, the event handler for the press will also call
    // processEvents() so the move and release will be delivered before returning
    // from mousePressEvent(). The point is that no events should get lost.
    QCoreApplication::processEvents();
    QCOMPARE(window.mousePressButton, int(Qt::LeftButton));
    QCOMPARE(window.mouseReleaseButton, int(Qt::LeftButton));
    QCOMPARE(window.mousePressedCount, 1);
    // The mouse press may have generated a synthetic move in QGuiApplicationPrivate::processMouseEvent()
    QVERIFY(window.mouseMovedCount == 1 || window.mouseMovedCount == 2);
    QCOMPARE(window.mouseReleasedCount, 1);

    // Now the same for touch.
    QList<QWindowSystemInterface::TouchPoint> points;
    QWindowSystemInterface::TouchPoint tp1;
    tp1.id = 1;
    tp1.state = QEventPoint::State::Pressed;
    tp1.area = QRectF(10, 10, 4, 4);
    points << tp1;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    points[0].state = QEventPoint::State::Updated;
    points[0].area = QRectF(20, 20, 8, 8);
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    points[0].state = QEventPoint::State::Released;
    QWindowSystemInterface::handleTouchEvent(&window, touchDevice, points);
    QCoreApplication::processEvents();
    QCOMPARE(window.touchPressedCount, 1);
    QCOMPARE(window.touchMovedCount, 1);
    QCOMPARE(window.touchReleasedCount, 1);
}

#if QT_CONFIG(tabletevent)
class TabletTestWindow : public QWindow
{
public:
    void tabletEvent(QTabletEvent *ev) override
    {
        eventType = ev->type();
        eventGlobal = ev->globalPosition();
        eventLocal = ev->position();
        eventDevice = ev->deviceType();
    }

    QEvent::Type eventType = QEvent::None;
    QPointF eventGlobal, eventLocal;
    QInputDevice::DeviceType eventDevice = QInputDevice::DeviceType::Unknown;
    QPointingDevice::PointerType eventPointerType = QPointingDevice::PointerType::Unknown;

    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        if (ev->type() == QEvent::TabletEnterProximity
                || ev->type() == QEvent::TabletLeaveProximity) {
            eventType = ev->type();
            QTabletEvent *te = static_cast<QTabletEvent *>(ev);
            eventDevice = te->deviceType();
            eventPointerType = te->pointerType();
        }
        return QWindow::eventFilter(obj, ev);
    }
};
#endif

void tst_QWindow::tabletEvents()
{
#if QT_CONFIG(tabletevent)
    // the fake USB tablet device is "plugged in"
    QPointingDevice tabletDevice("macow", 0xbeef, QInputDevice::DeviceType::Unknown, QPointingDevice::PointerType::Generic,
                                 QInputDevice::Capability::Position, 1, 0);
    QWindowSystemInterface::registerInputDevice(&tabletDevice);

    TabletTestWindow window;
    window.setGeometry(QRect(m_availableTopLeft + QPoint(10, 10), m_testWindowSize));
    qGuiApp->installEventFilter(&window);

    const QPoint local(10, 10);
    const QPoint global = window.mapToGlobal(local);
    const QPoint deviceLocal = QHighDpi::toNativeLocalPosition(local, &window);
    const QPoint deviceGlobal = QHighDpi::toNativePixels(global, window.screen());
    ulong timestamp = 1234;

    // the stylus is just now seen for the first time, as it comes into proximity
    // its QObject-parent will be the tablet device
    QPointingDevice tabletStylus("macow stylus eraser", 0xe6a5e6, QInputDevice::DeviceType::Stylus, QPointingDevice::PointerType::Eraser,
                                 QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3, QString(),
                                 QPointingDeviceUniqueId::fromNumericId(42), &tabletDevice);
    QWindowSystemInterface::registerInputDevice(&tabletStylus);
    QWindowSystemInterface::handleTabletEnterLeaveProximityEvent(&window, timestamp++, &tabletStylus, true);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.eventType, QEvent::TabletEnterProximity);
    QCOMPARE(window.eventDevice, QInputDevice::DeviceType::Stylus);
    QCOMPARE(window.eventPointerType, QPointingDevice::PointerType::Eraser);

    // the eraser is pressed into contact with the tablet surface
    QWindowSystemInterface::handleTabletEvent(&window, timestamp++, &tabletStylus, deviceLocal, deviceGlobal,
                                              Qt::LeftButton, 0.5, 1, 2, 0.1, 0, 0, {});
    QCoreApplication::processEvents();
    QTRY_VERIFY(window.eventType == QEvent::TabletPress);
    QTRY_COMPARE(window.eventGlobal.toPoint(), global);
    QTRY_COMPARE(window.eventLocal.toPoint(), local);

    // now it's lifted
    QWindowSystemInterface::handleTabletEvent(&window, timestamp++, &tabletStylus, deviceLocal, deviceGlobal,
                                              Qt::NoButton, 0, 3, 4, 0.11, 2, 1, {});
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.eventType, QEvent::TabletRelease);

    // and is taken away (goes out of proxmity)
    QWindowSystemInterface::handleTabletEnterLeaveProximityEvent(&window, timestamp, &tabletStylus, false);
    QCoreApplication::processEvents();
    QTRY_COMPARE(window.eventType, QEvent::TabletLeaveProximity);
    QCOMPARE(window.eventDevice, QInputDevice::DeviceType::Stylus);
    QCOMPARE(window.eventPointerType, QPointingDevice::PointerType::Eraser);
#endif
}

void tst_QWindow::windowModality_QTBUG27039()
{
    QWindow parent;
    parent.setTitle(QLatin1String(QTest::currentTestFunction()));
    parent.setGeometry(QRect(m_availableTopLeft + QPoint(10, 10), m_testWindowSize));
    parent.show();

    InputTestWindow modalA;
    modalA.setTransientParent(&parent);
    modalA.setGeometry(QRect(m_availableTopLeft + QPoint(20, 10), m_testWindowSize));
    modalA.setModality(Qt::ApplicationModal);
    modalA.show();

    InputTestWindow modalB;
    modalB.setTransientParent(&parent);
    modalA.setGeometry(QRect(m_availableTopLeft + QPoint(30, 10), m_testWindowSize));
    modalB.setModality(Qt::ApplicationModal);
    modalB.show();

    QPoint local(5, 5);
    QTest::mouseClick(&modalA, Qt::LeftButton, {}, local);
    QTest::mouseClick(&modalB, Qt::LeftButton, {}, local);
    QCoreApplication::processEvents();

    // modal A should be blocked since it was shown first, but modal B should not be blocked
    QCOMPARE(modalB.mousePressedCount, 1);
    QCOMPARE(modalA.mousePressedCount, 0);

    modalB.hide();
    QTest::mouseClick(&modalA, Qt::LeftButton, {}, local);
    QCoreApplication::processEvents();

    // modal B has been hidden, modal A should be unblocked again
    QCOMPARE(modalA.mousePressedCount, 1);
}

void tst_QWindow::visibility()
{
    qRegisterMetaType<Qt::WindowModality>("QWindow::Visibility");

    Window window;
    QSignalSpy spy(&window, SIGNAL(visibilityChanged(QWindow::Visibility)));

    window.setVisibility(QWindow::AutomaticVisibility);
    QVERIFY(window.isVisible());
    QVERIFY(window.visibility() != QWindow::Hidden);
    QVERIFY(window.visibility() != QWindow::AutomaticVisibility);
    QCOMPARE(spy.size(), 1);
    spy.clear();

    window.setVisibility(QWindow::Hidden);
    QVERIFY(!window.isVisible());
    QCOMPARE(window.visibility(), QWindow::Hidden);
    QCOMPARE(spy.size(), 1);
    spy.clear();

    window.setVisibility(QWindow::FullScreen);
    QVERIFY(window.isVisible());
    QCOMPARE(window.windowState(), Qt::WindowFullScreen);
    QCOMPARE(window.visibility(), QWindow::FullScreen);
    QCOMPARE(spy.size(), 1);
    QTRY_COMPARE(window.lastReceivedWindowState, Qt::WindowFullScreen);
    spy.clear();

    window.setWindowState(Qt::WindowNoState);
    QCOMPARE(window.visibility(), QWindow::Windowed);
    QCOMPARE(spy.size(), 1);
    QTRY_COMPARE(window.lastReceivedWindowState, Qt::WindowNoState);
    spy.clear();

    window.setVisible(false);
    QCOMPARE(window.visibility(), QWindow::Hidden);
    QCOMPARE(spy.size(), 1);
    spy.clear();
}

void tst_QWindow::mask()
{
    QRegion mask = QRect(10, 10, 800 - 20, 600 - 20);

    {
        QWindow window;
        window.resize(800, 600);
        QCOMPARE(window.mask(), QRegion());

        window.create();
        window.setMask(mask);
        QCOMPARE(window.mask(), mask);
    }

    {
        QWindow window;
        window.resize(800, 600);
        QCOMPARE(window.mask(), QRegion());

        window.setMask(mask);
        QCOMPARE(window.mask(), mask);
        window.create();
        QCOMPARE(window.mask(), mask);
    }

}

void tst_QWindow::initialSize()
{
    QSize defaultSize(0,0);
    {
    Window w;
    w.setTitle(QLatin1String(QTest::currentTestFunction()));
    w.showNormal();
    QTRY_VERIFY(w.width() > 0);
    QTRY_VERIFY(w.height() > 0);
    defaultSize = QSize(w.width(), w.height());
    }
    {
    Window w;
    w.setTitle(QLatin1String(QTest::currentTestFunction()));
    w.setWidth(m_testWindowSize.width());
    w.showNormal();

    if (isPlatformEglFS())
        QEXPECT_FAIL("", "eglfs windows are fullscreen by default.", Continue);

    QTRY_COMPARE(w.width(), m_testWindowSize.width());
    QTRY_VERIFY(w.height() > 0);
    }
    {
    Window w;
    w.setTitle(QLatin1String(QTest::currentTestFunction()));
    const QSize testSize(m_testWindowSize.width(), 42);
    w.resize(testSize);
    w.showNormal();

    const QSize expectedSize = testSize;
    if (isPlatformEglFS())
        QEXPECT_FAIL("", "eglfs windows are fullscreen by default.", Continue);
    QTRY_COMPARE(w.size(), expectedSize);
    }
}

static bool isPlatformOffscreenOrMinimal()
{
    return ((QGuiApplication::platformName() == QLatin1String("offscreen"))
             || (QGuiApplication::platformName() == QLatin1String("minimal")));
}

void tst_QWindow::modalDialog()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("QWindow::requestActivate() is not supported.");

    if (QGuiApplication::platformName() == QLatin1String("cocoa"))
        QSKIP("Test fails due to QTBUG-61965, and is slow due to QTBUG-61964");

    QWindow normalWindow;
    normalWindow.setTitle(QLatin1String(QTest::currentTestFunction()));
    normalWindow.setFramePosition(m_availableTopLeft + QPoint(80, 80));
    normalWindow.resize(m_testWindowSize);
    normalWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&normalWindow));

    QWindow dialog;
    dialog.setFramePosition(m_availableTopLeft + QPoint(200, 200));
    dialog.resize(m_testWindowSize);
    dialog.setModality(Qt::ApplicationModal);
    dialog.setFlags(Qt::Dialog);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));

    normalWindow.requestActivate();

    QGuiApplication::sync();
    QGuiApplication::processEvents();

    if (isPlatformOffscreenOrMinimal()) {
        qWarning("Focus stays in normalWindow on offscreen/minimal platforms");
        QTRY_COMPARE(QGuiApplication::focusWindow(), &normalWindow);
        return;
    }

    QTRY_COMPARE(QGuiApplication::focusWindow(), &dialog);
}

void tst_QWindow::modalDialogClosingOneOfTwoModal()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("QWindow::requestActivate() is not supported.");

    QWindow normalWindow;
    normalWindow.setTitle(QLatin1String(QTest::currentTestFunction()));
    normalWindow.setFramePosition(m_availableTopLeft + QPoint(80, 80));
    normalWindow.resize(m_testWindowSize);
    normalWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&normalWindow));

    QWindow first_dialog;
    first_dialog.setFramePosition(m_availableTopLeft + QPoint(200, 200));
    first_dialog.resize(m_testWindowSize);
    first_dialog.setModality(Qt::ApplicationModal);
    first_dialog.setFlags(Qt::Dialog);
    first_dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&first_dialog));

    {
        QWindow second_dialog;
        second_dialog.setFramePosition(m_availableTopLeft + QPoint(300, 300));
        second_dialog.resize(m_testWindowSize);
        second_dialog.setModality(Qt::ApplicationModal);
        second_dialog.setFlags(Qt::Dialog);
        second_dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&second_dialog));

        QTRY_COMPARE(QGuiApplication::focusWindow(), &second_dialog);

        second_dialog.close();
    }

    QGuiApplication::sync();
    QGuiApplication::processEvents();

    if (isPlatformOffscreenOrMinimal()) {
        qWarning("Focus is lost when closing modal dialog on offscreen/minimal platforms");
        QTRY_COMPARE(QGuiApplication::focusWindow(), nullptr);
        return;
    }

    QTRY_COMPARE(QGuiApplication::focusWindow(), &first_dialog);
}

void tst_QWindow::modalWithChildWindow()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("QWindow::requestActivate() is not supported.");

    QWindow normalWindow;
    normalWindow.setTitle(QLatin1String(QTest::currentTestFunction()));
    normalWindow.setFramePosition(m_availableTopLeft + QPoint(80, 80));
    normalWindow.resize(m_testWindowSize);
    normalWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&normalWindow));

    QWindow tlw_dialog;
    tlw_dialog.setFramePosition(m_availableTopLeft + QPoint(200, 200));
    tlw_dialog.resize(m_testWindowSize);
    tlw_dialog.setModality(Qt::ApplicationModal);
    tlw_dialog.setFlags(Qt::Dialog);
    tlw_dialog.create();

    QWindow sub_window(&tlw_dialog);
    sub_window.resize(200,300);
    sub_window.show();

    tlw_dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tlw_dialog));
    QVERIFY(QTest::qWaitForWindowExposed(&sub_window));

    QTRY_COMPARE(QGuiApplication::focusWindow(), &tlw_dialog);

    sub_window.requestActivate();
    QGuiApplication::sync();
    QGuiApplication::processEvents();
    QTRY_COMPARE(QGuiApplication::focusWindow(), &sub_window);
}

void tst_QWindow::modalWindowModallity()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("QWindow::requestActivate() is not supported.");

    QWindow normal_window;
    normal_window.setTitle(QLatin1String(QTest::currentTestFunction()));
    normal_window.setFramePosition(m_availableTopLeft + QPoint(80, 80));
    normal_window.resize(m_testWindowSize);
    normal_window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&normal_window));

    QWindow parent_to_modal;
    parent_to_modal.setFramePosition(normal_window.geometry().topRight() + QPoint(100, 0));
    parent_to_modal.resize(m_testWindowSize);
    parent_to_modal.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent_to_modal));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &parent_to_modal);

    QWindow modal_dialog;
    modal_dialog.resize(m_testWindowSize);
    modal_dialog.setFramePosition(normal_window.geometry().bottomLeft() + QPoint(0, 100));
    modal_dialog.setModality(Qt::WindowModal);
    modal_dialog.setFlags(Qt::Dialog);
    modal_dialog.setTransientParent(&parent_to_modal);
    modal_dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&modal_dialog));
    QTRY_COMPARE(QGuiApplication::focusWindow(), &modal_dialog);

    normal_window.requestActivate();
    QTRY_COMPARE(QGuiApplication::focusWindow(), &normal_window);

}

void tst_QWindow::modalWindowPosition()
{
    if (isPlatformWayland())
        QSKIP("Window position not queryable on Wayland");

    QWindow window;
    window.setTitle(QLatin1String(QTest::currentTestFunction()));
    window.setGeometry(QRect(m_availableTopLeft + QPoint(100, 100), m_testWindowSize));
    // Allow for any potential resizing due to constraints
    QRect origGeo = window.geometry();
    window.setModality(Qt::WindowModal);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    if (isPlatformEglFS())
        QEXPECT_FAIL("", "eglfs windows are fullscreen by default.", Continue);

    QCOMPARE(window.geometry(), origGeo);
}

void tst_QWindow::modalCloseWhileBlocked()
{
    QWindow first;
    first.setModality(Qt::ApplicationModal);
    first.show();
    QVERIFY(QTest::qWaitForWindowExposed(&first));

    QWindow second;
    second.setModality(Qt::ApplicationModal);
    second.show();
    QVERIFY(QTest::qWaitForWindowExposed(&first));

    first.close();
    QTRY_VERIFY(!first.isVisible());
}

#ifndef QT_NO_CURSOR
void tst_QWindow::modalWindowEnterEventOnHide_QTBUG35109()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("QWindow::requestActivate() is not supported.");

    if (isPlatformOffscreenOrMinimal())
        QSKIP("Can't test window focusing on offscreen/minimal");

    if (isPlatformEglFS())
        QSKIP("QCursor::setPos() is not supported on this platform");

    const QPoint center = QGuiApplication::primaryScreen()->availableGeometry().center();

    const int childOffset = 16;
    const QPoint rootPos = center - QPoint(m_testWindowSize.width(),
                                           m_testWindowSize.height())/2;
    const QPoint modalPos = rootPos + QPoint(childOffset * 5,
                                             childOffset * 5);
    const QPoint cursorPos = rootPos - QPoint(80, 80);

    // Test whether tlw can receive the enter event
    {
        QCursor::setPos(cursorPos);
        QCoreApplication::processEvents();

        InputTestWindow root;
        root.setTitle(__FUNCTION__);
        root.setGeometry(QRect(rootPos, m_testWindowSize));
        root.show();
        QVERIFY(QTest::qWaitForWindowExposed(&root));
        root.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&root));

        // Move the mouse over the root window, but not over the modal window.
        QCursor::setPos(rootPos + QPoint(childOffset * 5 / 2,
                                         childOffset * 5 / 2));

        // Wait for the enter event. It must be delivered here, otherwise second
        // compare can PASS because of this event even after "resetCounters()".
        QTRY_COMPARE(root.enterEventCount, 1);
        QTRY_COMPARE(root.leaveEventCount, 0);

        QWindow modal;
        modal.setTitle(QLatin1String("Modal - ") + __FUNCTION__);
        modal.setTransientParent(&root);
        modal.resize(m_testWindowSize/2);
        modal.setFramePosition(modalPos);
        modal.setModality(Qt::ApplicationModal);
        modal.show();
        QVERIFY(QTest::qWaitForWindowExposed(&modal));
        modal.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&modal));

        QCoreApplication::processEvents();
        QTRY_COMPARE(root.leaveEventCount, 1);

        root.resetCounters();
        modal.close();

        // Check for the enter event
        QTRY_COMPARE(root.enterEventCount, 1);
    }

    // Test whether child window can receive the enter event
    {
        QCursor::setPos(cursorPos);
        QCoreApplication::processEvents();

        QWindow root;
        root.setTitle(__FUNCTION__);
        root.setGeometry(QRect(rootPos, m_testWindowSize));

        QWindow childLvl1;
        childLvl1.setParent(&root);
        childLvl1.setGeometry(childOffset,
                              childOffset,
                              m_testWindowSize.width() - childOffset,
                              m_testWindowSize.height() - childOffset);

        InputTestWindow childLvl2;
        childLvl2.setParent(&childLvl1);
        childLvl2.setGeometry(childOffset,
                              childOffset,
                              childLvl1.width() - childOffset,
                              childLvl1.height() - childOffset);

        root.show();
        childLvl1.show();
        childLvl2.show();

        QVERIFY(QTest::qWaitForWindowExposed(&root));
        root.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&root));
        QVERIFY(childLvl1.isVisible());
        QVERIFY(childLvl2.isVisible());

        // Move the mouse over the child window, but not over the modal window.
        // Be sure that the value is almost left-top of second child window for
        // checking proper position mapping.
        QCursor::setPos(rootPos + QPoint(childOffset * 5 / 2,
                                         childOffset * 5 / 2));

        // Wait for the enter event. It must be delivered here, otherwise second
        // compare can PASS because of this event even after "resetCounters()".
        QTRY_COMPARE(childLvl2.enterEventCount, 1);
        QTRY_COMPARE(childLvl2.leaveEventCount, 0);

        QWindow modal;
        modal.setTitle(QLatin1String("Modal - ") + __FUNCTION__);
        modal.setTransientParent(&root);
        modal.resize(m_testWindowSize/2);
        modal.setFramePosition(modalPos);
        modal.setModality(Qt::ApplicationModal);
        modal.show();
        QVERIFY(QTest::qWaitForWindowExposed(&modal));
        modal.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&modal));

        QCoreApplication::processEvents();
        QTRY_COMPARE(childLvl2.leaveEventCount, 1);

        childLvl2.resetCounters();
        modal.close();

        // Check for the enter event
        QTRY_COMPARE(childLvl2.enterEventCount, 1);
    }

    // Test whether tlw can receive the enter event if mouse is over the invisible child windnow
    {
        QCursor::setPos(cursorPos);
        QCoreApplication::processEvents();

        InputTestWindow root;
        root.setTitle(__FUNCTION__);
        root.setGeometry(QRect(rootPos, m_testWindowSize));

        QWindow child;
        child.setParent(&root);
        child.setGeometry(QRect(QPoint(), m_testWindowSize));

        root.show();

        QVERIFY(QTest::qWaitForWindowExposed(&root));
        root.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&root));
        QVERIFY(!child.isVisible());

        // Move the mouse over the child window, but not over the modal window.
        QCursor::setPos(rootPos + QPoint(childOffset * 5 / 2,
                                         childOffset * 5 / 2));

        // Wait for the enter event. It must be delivered here, otherwise second
        // compare can PASS because of this event even after "resetCounters()".
        QTRY_COMPARE(root.enterEventCount, 1);
        QTRY_COMPARE(root.leaveEventCount, 0);

        QWindow modal;
        modal.setTitle(QLatin1String("Modal - ") + __FUNCTION__);
        modal.setTransientParent(&root);
        modal.resize(m_testWindowSize/2);
        modal.setFramePosition(modalPos);
        modal.setModality(Qt::ApplicationModal);
        modal.show();
        QVERIFY(QTest::qWaitForWindowExposed(&modal));
        modal.requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(&modal));

        QCoreApplication::processEvents();
        QTRY_COMPARE(root.leaveEventCount, 1);

        root.resetCounters();
        modal.close();

        // Check for the enter event
        QTRY_COMPARE(root.enterEventCount, 1);
    }
}

// Verify that no spurious mouse move events are received. On Windows, there is
// no enter event, the OS sends mouse move events instead. Test that the QPA
// plugin properly suppresses those since they can interfere with tests.
// Simulate a main window setup with a modal dialog on top, keep the cursor
// in the center and check that no mouse events are recorded.
void tst_QWindow::spuriousMouseMove()
{
    const QString &platformName = QGuiApplication::platformName();
    if (platformName == QLatin1String("offscreen") || platformName == QLatin1String("cocoa"))
        QSKIP("No enter events sent");
    if (platformName == QLatin1String("wayland"))
        QSKIP("Setting mouse cursor position is not possible on Wayland");
    if (isPlatformEglFS())
        QSKIP("QCursor::setPos() is not supported on this platform");
    const QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    const QPoint center = screenGeometry.center();
    QCursor::setPos(center);
    QRect windowGeometry(QPoint(), 2 * m_testWindowSize);
    windowGeometry.moveCenter(center);
    QTRY_COMPARE(QCursor::pos(), center);
    InputTestWindow topLevel;
    topLevel.setTitle(QTest::currentTestFunction());
    topLevel.setGeometry(windowGeometry);
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QTRY_VERIFY(topLevel.enterEventCount > 0);
    InputTestWindow dialog(Qt::yellow);
    dialog.setTransientParent(&topLevel);
    dialog.setTitle("Dialog " + topLevel.title());
    dialog.setModality(Qt::ApplicationModal);
    windowGeometry.setSize(m_testWindowSize);
    windowGeometry.moveCenter(center);
    dialog.setGeometry(windowGeometry);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    QTRY_VERIFY(dialog.enterEventCount > 0);
    dialog.setVisible(false);
    QCOMPARE(dialog.mousePressedCount, 0);
    QCOMPARE(dialog.mouseReleasedCount, 0);
    QCOMPARE(dialog.mouseMovedCount, 0);
    QCOMPARE(dialog.mouseDoubleClickedCount, 0);
    topLevel.setVisible(false);
    QCOMPARE(topLevel.mousePressedCount, 0);
    QCOMPARE(topLevel.mouseReleasedCount, 0);
    QCOMPARE(topLevel.mouseMovedCount, 0);
    QCOMPARE(topLevel.mouseDoubleClickedCount, 0);
}
#endif // !QT_NO_CURSOR

static bool isNativeWindowVisible(const QWindow *window)
{
#if defined(Q_OS_WIN)
    return IsWindowVisible(reinterpret_cast<HWND>(window->winId()));
#else
    Q_UNIMPLEMENTED();
    return window->isVisible();
#endif
}

void tst_QWindow::windowsTransientChildren()
{
    if (QGuiApplication::platformName().compare(QStringLiteral("windows"), Qt::CaseInsensitive))
        QSKIP("Windows only test");

    ColoredWindow mainWindow(Qt::yellow);
    mainWindow.setGeometry(QRect(m_availableTopLeft + QPoint(100, 100), m_testWindowSize));
    mainWindow.setTitle(QStringLiteral("Main"));
    ColoredWindow child(Qt::blue, &mainWindow);
    child.setGeometry(QRect(QPoint(0, 0), m_testWindowSize / 2));

    ColoredWindow dialog(Qt::red);
    dialog.setGeometry(QRect(m_availableTopLeft + QPoint(200, 200), m_testWindowSize));
    dialog.setTitle(QStringLiteral("Dialog"));
    dialog.setTransientParent(&mainWindow);

    mainWindow.show();
    child.show();
    dialog.show();

    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    mainWindow.setWindowState(Qt::WindowMinimized);
    QVERIFY(!isNativeWindowVisible(&dialog));
    dialog.hide();
    mainWindow.setWindowState(Qt::WindowNoState);
    // QTBUG-40696, transient children hidden by Qt should not be re-shown by Windows.
    QVERIFY(!isNativeWindowVisible(&dialog));
    QVERIFY(isNativeWindowVisible(&child)); // Real children should be visible.
}

void tst_QWindow::requestUpdate()
{
    QRect geometry(m_availableTopLeft + QPoint(80, 80), m_testWindowSize);

    Window window;
    window.setTitle(QLatin1String(QTest::currentTestFunction()));
    window.setGeometry(geometry);
    window.show();
    QCoreApplication::processEvents();
    QTRY_VERIFY(window.isExposed());

    QCOMPARE(window.received(QEvent::UpdateRequest), 0);

    window.requestUpdate();
    QTRY_COMPARE(window.received(QEvent::UpdateRequest), 1);

    window.requestUpdate();
    QTRY_COMPARE(window.received(QEvent::UpdateRequest), 2);
}

void tst_QWindow::flags()
{
    Window window;
    const auto baseFlags = window.flags();
    window.setFlags(window.flags() | Qt::FramelessWindowHint);
    QCOMPARE(window.flags(), baseFlags | Qt::FramelessWindowHint);
    window.setFlag(Qt::WindowStaysOnTopHint, true);
    QCOMPARE(window.flags(), baseFlags | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    window.setFlag(Qt::FramelessWindowHint, false);
    QCOMPARE(window.flags(), baseFlags | Qt::WindowStaysOnTopHint);
}

class EventWindow : public QWindow
{
public:
    bool gotBlocked = false;

protected:
    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::WindowBlocked)
            gotBlocked = true;
        return QWindow::event(e);
    }
};

void tst_QWindow::testBlockingWindowShownAfterModalDialog()
{
    EventWindow normalWindow;
    normalWindow.setTitle(QLatin1String(QTest::currentTestFunction()));
    normalWindow.setFramePosition(m_availableTopLeft + QPoint(80, 80));
    normalWindow.resize(m_testWindowSize);
    normalWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&normalWindow));
    QVERIFY(!normalWindow.gotBlocked);

    QWindow dialog;
    dialog.setFramePosition(m_availableTopLeft + QPoint(200, 200));
    dialog.resize(m_testWindowSize);
    dialog.setModality(Qt::ApplicationModal);
    dialog.setFlags(Qt::Dialog);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    QVERIFY(normalWindow.gotBlocked);

    EventWindow normalWindowAfter;
    normalWindowAfter.setFramePosition(m_availableTopLeft + QPoint(80, 80));
    normalWindowAfter.resize(m_testWindowSize);
    QVERIFY(!normalWindowAfter.gotBlocked);
    normalWindowAfter.show();
    QVERIFY(QTest::qWaitForWindowExposed(&normalWindowAfter));
    QVERIFY(normalWindowAfter.gotBlocked);
}

void tst_QWindow::generatedMouseMove()
{
    InputTestWindow w;
    w.setTitle(QLatin1String(QTest::currentTestFunction()));
    w.setGeometry(QRect(m_availableTopLeft + QPoint(100, 100), m_testWindowSize));
    w.setFlags(w.flags() | Qt::FramelessWindowHint); // ### FIXME: QTBUG-63542
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QPoint point(10, 10);
    QPoint step(2, 2);

    QVERIFY(w.mouseMovedCount == 0);
    QTest::mouseMove(&w, point);
    QVERIFY(w.mouseMovedCount == 1);
    // A press event that does not change position should not generate mouse move
    QTest::mousePress(&w, Qt::LeftButton, Qt::KeyboardModifiers(), point);
    QTest::mousePress(&w, Qt::RightButton, Qt::KeyboardModifiers(), point);

    QVERIFY(w.mouseMovedCount == 1);

    // Verify that a move event is generated for a mouse release event that changes position
    point += step;
    QTest::mouseRelease(&w, Qt::LeftButton,Qt::KeyboardModifiers(), point);
    QVERIFY(w.mouseMovedCount == 2);
    QVERIFY(w.buttonStateInGeneratedMove == (Qt::LeftButton | Qt::RightButton));
    point += step;
    QTest::mouseRelease(&w, Qt::RightButton, Qt::KeyboardModifiers(), point);
    QVERIFY(w.mouseMovedCount == 3);
    QVERIFY(w.buttonStateInGeneratedMove == Qt::RightButton);

    // Verify that a move event is generated for a mouse press event that changes position
    point += step;
    QTest::mousePress(&w, Qt::LeftButton, Qt::KeyboardModifiers(), point);
    QVERIFY(w.mouseMovedCount == 4);
    QVERIFY(w.buttonStateInGeneratedMove == Qt::NoButton);
    point += step;
    QTest::mousePress(&w, Qt::RightButton, Qt::KeyboardModifiers(), point);
    QVERIFY(w.mouseMovedCount == 5);
    QVERIFY(w.buttonStateInGeneratedMove == Qt::LeftButton);

    // A release event that does not change position should not generate mouse move
    QTest::mouseRelease(&w, Qt::RightButton, Qt::KeyboardModifiers(), point);
    QTest::mouseRelease(&w, Qt::LeftButton, Qt::KeyboardModifiers(), point);
    QVERIFY(w.mouseMovedCount == 5);
}

void tst_QWindow::keepPendingUpdateRequests()
{
    QRect geometry(m_availableTopLeft + QPoint(80, 80), m_testWindowSize);

    Window window;
    window.setTitle(QLatin1String(QTest::currentTestFunction()));
    window.setGeometry(geometry);
    window.show();
    QCoreApplication::processEvents();
    QTRY_VERIFY(window.isExposed());

    window.requestUpdate();
    window.close();
    window.setVisible(true);

    QPlatformWindow *platformWindow = window.handle();
    QVERIFY(platformWindow);

    QVERIFY(platformWindow->hasPendingUpdateRequest());
    QTRY_VERIFY(!platformWindow->hasPendingUpdateRequest());
}

void tst_QWindow::activateDeactivateEvent()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("QWindow::requestActivate() is not supported.");

    class Window : public QWindow
    {
    public:
        using QWindow::QWindow;

        int activateCount = 0;
        int deactivateCount = 0;
    protected:
        bool event(QEvent *e) override
        {
            switch (e->type()) {
            case QEvent::WindowActivate:
                ++activateCount;
                break;
            case QEvent::WindowDeactivate:
                ++deactivateCount;
                break;
            default:
                break;
            }
            return QWindow::event(e);
        }
    };

    Window w1;
    Window w2;

    w1.show();
    w1.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&w1));
    QCOMPARE(w1.activateCount, 1);
    QCOMPARE(w1.deactivateCount, 0);

    w2.show();
    w2.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&w2));
    QCOMPARE(w1.deactivateCount, 1);
    QCOMPARE(w2.activateCount, 1);
}

// Test that in a slot connected to destroyed() the emitter is
// is no longer a QWindow.
void tst_QWindow::qobject_castOnDestruction()
{
    QWindow window;
    QObject::connect(&window, &QObject::destroyed, [](QObject *object)
    {
        QVERIFY(!qobject_cast<QWindow *>(object));
        QVERIFY(!dynamic_cast<QWindow *>(object));
        QVERIFY(!object->isWindowType());
    });
}

void tst_QWindow::touchToMouseTranslationByPopup()
{
    InputTestWindow window;
    window.setTitle(QLatin1String(QTest::currentTestFunction()));
    window.ignoreTouch = true;
    window.setGeometry(QRect(m_availableTopLeft, m_testWindowSize));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    InputTestWindow popupWindow;
    popupWindow.setGeometry(QRect(m_availableTopLeft + QPoint(20, 20),
                                  QSize(m_testWindowSize.width(), m_testWindowSize.height() / 2)));
    popupWindow.setFlag(Qt::Popup);
    popupWindow.setTransientParent(&window);
    popupWindow.ignoreTouch = true;
    popupWindow.closeOnTap = true;
    popupWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&popupWindow));

    QTest::touchEvent(&popupWindow, touchDevice).press(0, {1, 1}, &window);
    QVERIFY(!popupWindow.isVisible());

    // Omit touchpoint 0: because the popup was closed, touchpoint0.release is not sent.
    const QPoint tp1(50, 1);
    QTest::touchEvent(&window, touchDevice).press(1, tp1, &window);
    QTRY_COMPARE(window.mousePressButton, int(Qt::LeftButton));
    QTest::touchEvent(&window, touchDevice).release(1, tp1, &window);
    QTRY_COMPARE(window.mouseReleaseButton, int(Qt::LeftButton));
}

// Test that windowStateChanged is not emitted on noop change (QTBUG-102478)
void tst_QWindow::stateChangeSignal()
{
    // Test only for Windows, Linux and macOS
#if !defined(Q_OS_LINUX) && !defined(Q_OS_WINDOWS) && !defined(Q_OS_DARWIN)
    QSKIP("Singular windowStateChanged signal emission is guaranteed for Linux, Windows and macOS only.\n"
          "On other operating systems, the signal may be emitted twice.");
#endif
    QWindow w;
    Q_ASSERT(connect (&w, &QWindow::windowStateChanged, [](Qt::WindowState s){qCDebug(lcTests) << "State change to" << s;}));
    QSignalSpy spy(&w, SIGNAL(windowStateChanged(Qt::WindowState)));
    unsigned short signalCount = 0;
    QList<Qt::WindowState> effectiveStates;
    Q_ASSERT(connect(&w, &QWindow::windowStateChanged, [&effectiveStates](Qt::WindowState state)
            { effectiveStates.append(state); }));
    // Part 1:
    // => test signal emission on programmatic state changes
    QCOMPARE(w.windowState(), Qt::WindowNoState);
    // - wait for target state to be set
    // - wait for signal spy to have reached target count
    // - extract state from signal and compare to target
#define CHECK_STATE(State)\
    QTRY_VERIFY(QTest::qWaitFor([&w](){return (w.windowState() == State); }));\
    CHECK_SIGNAL(State)
#define CHECK_SIGNAL(State)\
    QTRY_COMPARE(spy.count(), signalCount);\
    if (signalCount > 0) {\
        QVariantList list = spy.at(signalCount - 1).toList();\
        QCOMPARE(list.count(), 1);\
        bool ok;\
        const int stateInt = list.at(0).toInt(&ok);\
        QVERIFY(ok);\
        const Qt::WindowState newState = static_cast<Qt::WindowState>(stateInt);\
        QCOMPARE(newState, State);\
    }
    // Check initialization
    CHECK_STATE(Qt::WindowNoState);
    // showMaximized after init
    // expected behavior: signal emitted once with state == WindowMaximized
    ++signalCount;
    w.showMaximized();
    CHECK_STATE(Qt::WindowMaximized);
    // setWindowState to normal
    // expected behavior: signal emitted once with state == WindowNoState
    ++signalCount;
    w.setWindowState(Qt::WindowNoState);
    CHECK_STATE(Qt::WindowNoState);
    // redundant setWindowState to normal - except windows, where the no-op is counted
    // expected behavior: No emits.
    // On Windows, a no-op state change causes a no-op resize and repaint, leading to a
    // no-op state change and singal emission.
#ifdef Q_OS_WINDOWS
    ++signalCount;
    ++signalCount;
#endif
    w.setWindowState(Qt::WindowNoState);
    CHECK_STATE(Qt::WindowNoState);
    // setWindowState to minimized
    // expected behavior: signal emitted once with state == WindowMinimized
    ++signalCount;
    w.showMinimized();
    CHECK_STATE(Qt::WindowMinimized);
    // setWindowState to Normal
    // expected behavior: signal emitted once with state == WindowNoState
    ++signalCount;
    w.showNormal();
    CHECK_STATE(Qt::WindowNoState);
    /*
     - Testcase showFullScreen is omitted: Depending on window manager,
     WindowFullScreen can be mapped to WindowMaximized
     - Transition from WindowMinimized to WindowMaximized is omitted:
     WindowNoState to WindowMaximized
     */
    // Part 2:
    // => test signal emission on simulated user interaction
    // To test the code path, inject state change events into the QPA event queue.
    // Test the signal emission only, not the window's actual visible state.

    // Flush pending events and clear
    QCoreApplication::processEvents();
    spy.clear();
    effectiveStates.clear();
    signalCount = 0;
    // Maximize window
    QWindowSystemInterface::handleWindowStateChanged(&w, Qt::WindowMaximized, w.windowState());
    ++signalCount;
    CHECK_SIGNAL(Qt::WindowMaximized);
    // Normalize window
    QWindowSystemInterface::handleWindowStateChanged(&w, Qt::WindowNoState, w.windowState());
    ++signalCount;
    CHECK_SIGNAL(Qt::WindowNoState);
    // Minimize window
    QWindowSystemInterface::handleWindowStateChanged(&w, Qt::WindowMinimized, w.windowState());
    ++signalCount;
    CHECK_SIGNAL(Qt::WindowMinimized);
}

#ifndef QT_NO_CURSOR
void tst_QWindow::enterLeaveOnWindowShowHide_data()
{
    QTest::addColumn<Qt::WindowType>("windowType");
    QTest::addRow("dialog") << Qt::Dialog;
    QTest::addRow("popup") << Qt::Popup;
}

/*!
    Verify that we get enter and leave events if the window under the mouse
    opens and closes a modal dialog or popup. QWindow might get multiple
    events in a row, as the various QPA plugins need to use different techniques
    to synthesize events if the native platform doesn't provide them for us.
*/
void tst_QWindow::enterLeaveOnWindowShowHide()
{
    if (isPlatformWayland())
        QSKIP("Can't set cursor position and qWaitForWindowActive on Wayland");

    if (isPlatformEglFS())
        QSKIP("QCursor::setPos() is not supported on this platform");

    QFETCH(Qt::WindowType, windowType);

    class Window : public QWindow
    {
    public:
        int numEnterEvents = 0;
        int numLeaveEvents = 0;
        QPoint enterPosition;
    protected:
        bool event(QEvent *e) override
        {
            switch (e->type()) {
            case QEvent::Enter:
                ++numEnterEvents;
                enterPosition = static_cast<QEnterEvent*>(e)->position().toPoint();
                break;
            case QEvent::Leave:
                ++numLeaveEvents;
                break;
            default:
                break;
            }
            return QWindow::event(e);
        }
    };

    int expectedEnter = 0;
    int expectedLeave = 0;

    Window window;
    const QRect screenGeometry = window.screen()->availableGeometry();
    const QPoint cursorPos = screenGeometry.topLeft() + QPoint(50, 50);
    window.setGeometry(QRect(cursorPos - QPoint(50, 50), screenGeometry.size() / 4));
    QCursor::setPos(cursorPos);

    if (!QTest::qWaitFor([&]{ return window.geometry().contains(QCursor::pos()); }))
        QSKIP("We can't move the cursor");

    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    ++expectedEnter;
    QTRY_COMPARE_WITH_TIMEOUT(window.numEnterEvents, expectedEnter, 250);
    QCOMPARE(window.enterPosition, window.mapFromGlobal(QCursor::pos()));

    QWindow secondary;
    secondary.setFlag(windowType);
    secondary.setModality(Qt::WindowModal);
    secondary.setTransientParent(&window);
    secondary.setPosition(cursorPos + QPoint(50, 50));
    secondary.show();
    QVERIFY(QTest::qWaitForWindowExposed(&secondary));
    ++expectedLeave;
    QTRY_VERIFY(window.numLeaveEvents >= expectedLeave);
    secondary.close();
    ++expectedEnter;
    QTRY_VERIFY(window.numEnterEvents >= expectedEnter);
    QCOMPARE(window.enterPosition, window.mapFromGlobal(QCursor::pos()));
}
#endif

void tst_QWindow::windowExposedAfterReparent()
{
    QWindow parent;
    QWindow child(&parent);
    child.show();
    parent.show();

    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    QVERIFY(QTest::qWaitForWindowExposed(&child));

    child.setParent(nullptr);
    QCoreApplication::processEvents();
    QVERIFY(QTest::qWaitForWindowExposed(&child));

    child.setParent(&parent);
    QCoreApplication::processEvents();
    QVERIFY(QTest::qWaitForWindowExposed(&child));
}

struct ParentWindow : public QWindow
{
    bool event(QEvent *event) override
    {
        [&]() -> void {
            if (event->type() == QEvent::ChildWindowAdded
             || event->type() == QEvent::ChildWindowRemoved) {
                // We should not receive child events after the window has been destructed
                QVERIFY(this->isWindowType());

                auto *parentWindow = this;
                auto *childEvent = static_cast<QChildWindowEvent*>(event);
                auto *childWindow = childEvent->child();

                if (event->type() == QEvent::ChildWindowAdded) {
                    QVERIFY(childWindow->parent());
                    QVERIFY(parentWindow->isAncestorOf(childWindow));
                    if (childWindow->handle())
                        QVERIFY(childWindow->handle()->parent() == parentWindow->handle());

                } else {
                    QVERIFY(!childWindow->parent());
                    QVERIFY(!parentWindow->isAncestorOf(childWindow));
                    if (childWindow->handle())
                        QVERIFY(childWindow->handle()->parent() != parentWindow->handle());
                }
            }
        }();

        return QWindow::event(event);
    }
};

void tst_QWindow::childEvents()
{
    ParentWindow parent;

    {
        // ChildAdded via constructor
        QWindow constructorChild(&parent);
        if (QTest::currentTestFailed()) return;
        // ChildRemoved via destructor
    }

    if (QTest::currentTestFailed()) return;

    // ChildAdded and ChildRemoved via setParent
    QWindow child;
    child.setParent(&parent);
    if (QTest::currentTestFailed()) return;
    child.setParent(nullptr);
    if (QTest::currentTestFailed()) return;

    parent.create();
    child.create();

    // ChildAdded and ChildRemoved after creation
    child.setParent(&parent);
    if (QTest::currentTestFailed()) return;
    child.setParent(nullptr);
    if (QTest::currentTestFailed()) return;
}

struct ChildWindowPrivate;
struct ChildWindow : public QWindow
{
    ChildWindow(QWindow *parent = nullptr);
};

struct ChildWindowPrivate : public QWindowPrivate
{
    ChildWindowPrivate() : QWindowPrivate()
    {
        receiveParentEvents = true;
    }
};

ChildWindow::ChildWindow(QWindow *parent)
    : QWindow(*new ChildWindowPrivate, parent)
{}

struct ParentEventTester : public QObject
{
    bool eventFilter(QObject *object, QEvent *event) override
    {
        [&]() -> void {
            if (event->type() == QEvent::ParentWindowAboutToChange
             || event->type() == QEvent::ParentWindowChange) {
                // We should not receive parent events after the window has been destructed
                QVERIFY(object->isWindowType());
                auto *window = static_cast<QWindow*>(object);

                if (event->type() == QEvent::ParentWindowAboutToChange) {
                    QVERIFY(window->parent() != nextExpectedParent);
                    if (window->handle()) {
                        QVERIFY(window->handle()->parent() !=
                            (nextExpectedParent ? nextExpectedParent->handle() : nullptr));
                    }
                } else {
                    QVERIFY(window->parent() == nextExpectedParent);
                    if (window->handle()) {
                        QVERIFY(window->handle()->parent() ==
                            (nextExpectedParent ? nextExpectedParent->handle() : nullptr));
                    }
                }
            }
        }();

        return QObject::eventFilter(object, event);
    }

    QWindow *nextExpectedParent = nullptr;
};



void tst_QWindow::parentEvents()
{
    QWindow parent;

    {
        ParentEventTester tester;

        {
            // We can't hook in early enough to get the parent change during
            // QObject construction.
            ChildWindow child(&parent);

            // But we can observe the one during destruction
            child.installEventFilter(&tester);
            tester.nextExpectedParent = nullptr;
        }
    }
    if (QTest::currentTestFailed()) return;

    ParentEventTester tester;
    ChildWindow child;
    child.installEventFilter(&tester);

    tester.nextExpectedParent = &parent;
    child.setParent(&parent);
    if (QTest::currentTestFailed()) return;

    tester.nextExpectedParent = nullptr;
    child.setParent(nullptr);
    if (QTest::currentTestFailed()) return;

    parent.create();
    child.create();

    tester.nextExpectedParent = &parent;
    child.setParent(&parent);
    if (QTest::currentTestFailed()) return;

    tester.nextExpectedParent = nullptr;
    child.setParent(nullptr);
    if (QTest::currentTestFailed()) return;
}

#include <tst_qwindow.moc>
QTEST_MAIN(tst_QWindow)

