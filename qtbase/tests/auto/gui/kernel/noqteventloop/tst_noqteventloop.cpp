// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

#include <QEvent>
#include <QtTest/QSignalSpy>
#include <QtCore/qthread.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpainter.h>
#include <QtGui/qrasterwindow.h>
#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qtcpsocket.h>
#include <QtNetwork/qlocalserver.h>
#include <QtNetwork/qlocalsocket.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qtimer.h>
#include <QtCore/qwineventnotifier.h>

#include <QtCore/qt_windows.h>

static const int topVerticalMargin = 50;
static const int margin = 10;

class tst_NoQtEventLoop : public QObject
{
    Q_OBJECT

private slots:
    void consumeMouseEvents();
    void consumeSocketEvents();
    void consumeLocalSocketEvents();
    void consumeWinEvents_data();
    void consumeWinEvents();
    void deliverEventsInLivelock();
    void postingWithNoYieldFlag();
};

class Window : public QRasterWindow
{
public:
    explicit Window(QWindow *parentWindow = nullptr) : QRasterWindow(parentWindow)
    {
    }

    void reset()
    {
        m_received.clear();
    }

    bool event(QEvent *event) override
    {
        m_received[event->type()]++;
        return QRasterWindow::event(event);
    }

    int received(QEvent::Type type)
    {
        return m_received.value(type, 0);
    }


    QHash<QEvent::Type, int> m_received;

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.fillRect(QRect(QPoint(0, 0), size()), Qt::yellow);
    }
};

bool g_exit = false;

extern "C" LRESULT QT_WIN_CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_SHOWWINDOW && wParam == 0)
        g_exit = true;
    return DefWindowProc(hwnd, message, wParam, lParam);
}

class TestThread : public QThread
{
    Q_OBJECT
public:
    TestThread(HWND parentWnd, Window *childWindow) : QThread(), m_hwnd(parentWnd), m_childWindow(childWindow) {
        m_screenW = ::GetSystemMetrics(SM_CXSCREEN);
        m_screenH = ::GetSystemMetrics(SM_CYSCREEN);
    }

    enum {
        MouseClick,
        MouseMove
    };

    void mouseInput(int command, const QPoint &p = QPoint())
    {
        INPUT mouseEvent;
        mouseEvent.type = INPUT_MOUSE;
        MOUSEINPUT &mi = mouseEvent.mi;
        mi.mouseData = 0;
        mi.time = 0;
        mi.dwExtraInfo = 0;
        mi.dx = 0;
        mi.dy = 0;
        switch (command) {
        case MouseClick:
            mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            ::SendInput(1, &mouseEvent, sizeof(INPUT));
            ::Sleep(50);
            mi.dwFlags = MOUSEEVENTF_LEFTUP;
            ::SendInput(1, &mouseEvent, sizeof(INPUT));
            break;
        case MouseMove:
            mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
            mi.dx = p.x() * 65536 / m_screenW;
            mi.dy = p.y() * 65536 / m_screenH;
            ::SendInput(1, &mouseEvent, sizeof(INPUT));
            break;
        }
    }

    void mouseClick()
    {
        mouseInput(MouseClick);
    }

    void mouseMove(const QPoint &pt)
    {
        mouseInput(MouseMove, pt);
    }


    void run() override
    {
        struct ScopedCleanup
        {
            /* This is in order to ensure that the window is hidden when returning from run(),
               regardless of the return point (e.g. with QTRY_COMPARE) */
            ScopedCleanup(HWND hwnd) : m_hwnd(hwnd) { }
            ~ScopedCleanup() {
                ::ShowWindow(m_hwnd, SW_HIDE);
            }
            HWND m_hwnd;
        } cleanup(m_hwnd);

        m_testPassed = false;
        POINT pt;
        pt.x = 0;
        pt.y = 0;
        if (!::ClientToScreen(m_hwnd, &pt))
            return;
        m_windowPos = QPoint(pt.x, pt.y);


        // First activate the parent window (which will also activate the child window)
        m_windowPos += QPoint(5,5);
        mouseMove(m_windowPos);
        ::Sleep(150);
        mouseClick();



        // At this point the windows are activated, no further events will be send to the QWindow
        // if we click on the native parent HWND
        m_childWindow->reset();
        ::Sleep(150);
        mouseClick();
        ::Sleep(150);

        QTRY_COMPARE(m_childWindow->received(QEvent::MouseButtonPress) + m_childWindow->received(QEvent::MouseButtonRelease), 0);

        // Now click in the QWindow. The QWindow should receive those events.
        m_windowPos.rx() += margin;
        m_windowPos.ry() += topVerticalMargin;
        mouseMove(m_windowPos);
        ::Sleep(150);
        mouseClick();
        QTRY_COMPARE(m_childWindow->received(QEvent::MouseButtonPress), 1);
        QTRY_COMPARE(m_childWindow->received(QEvent::MouseButtonRelease), 1);

        m_testPassed = true;

        // ScopedCleanup will hide the window here
        // Once the native window is hidden, it will exit the event loop.
    }

    bool passed() const { return m_testPassed; }

private:
    int m_screenW;
    int m_screenH;
    bool m_testPassed;
    HWND m_hwnd;
    Window *m_childWindow;
    QPoint m_windowPos;

};


void tst_NoQtEventLoop::consumeMouseEvents()
{
    int argc = 1;
    char *argv[] = {const_cast<char*>("test")};
    // ensure scaling is off since the child window is positioned using QWindow API.
    // FIXME: Position by taking the screen DPR into account instead
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");
    QGuiApplication app(argc, argv);
    QString clsName(QStringLiteral("tst_NoQtEventLoop_WINDOW"));
    const HINSTANCE appInstance = (HINSTANCE)GetModuleHandle(0);
    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_DBLCLKS | CS_OWNDC; // CS_SAVEBITS
    wc.lpfnWndProc  = wndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = appInstance;
    wc.hIcon = 0;
    wc.hIconSm = 0;
    wc.hCursor = 0;
    wc.hbrBackground = ::GetSysColorBrush(COLOR_BTNFACE /*COLOR_WINDOW*/);
    wc.lpszMenuName = 0;
    wc.lpszClassName = (wchar_t*)clsName.utf16();

    ATOM atom = ::RegisterClassEx(&wc);
    QVERIFY2(atom, "RegisterClassEx failed");

    DWORD dwExStyle = WS_EX_APPWINDOW;
    DWORD dwStyle = WS_CAPTION | WS_TABSTOP | WS_VISIBLE;

    const int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);
    const int width = screenWidth / 4;
    const int height = screenHeight / 4;

    HWND mainWnd =
        ::CreateWindowEx(dwExStyle, reinterpret_cast<const wchar_t*>(clsName.utf16()),
                         TEXT("tst_NoQtEventLoop"), dwStyle,
                         (screenWidth - width) / 2, (screenHeight - height) / 2 , width, height,
                         0, NULL, appInstance, NULL);
    QVERIFY2(mainWnd, "CreateWindowEx failed");

    ::ShowWindow(mainWnd, SW_SHOW);

    ::SetWindowPos(mainWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    QWindow *mainWindow = QWindow::fromWinId(reinterpret_cast<WId>(mainWnd));
    Window *childWindow = new Window(mainWindow);
    childWindow->setGeometry(margin, topVerticalMargin,
                             width - 2 * margin, height - margin - topVerticalMargin);
    childWindow->show();

    TestThread *testThread = new TestThread(mainWnd, childWindow);
    connect(testThread, SIGNAL(finished()), testThread, SLOT(deleteLater()));
    testThread->start();

    // Our own message loop...
    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0) > 0) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
        if (g_exit)
            break;
    }
    delete mainWindow;

    QCOMPARE(testThread->passed(), true);

}

void tst_NoQtEventLoop::consumeSocketEvents()
{
    int argc = 1;
    char *argv[] = { const_cast<char *>("test"), 0 };
    QGuiApplication app(argc, argv);
    QTcpServer server;
    QTcpSocket client;

    QVERIFY(server.listen(QHostAddress::LocalHost));
    client.connectToHost(server.serverAddress(), server.serverPort());
    QVERIFY(client.waitForConnected());

    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    // Exec own message loop
    MSG msg;
    forever {
        if (elapsedTimer.hasExpired(3000) || server.hasPendingConnections())
            break;

        if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    QVERIFY(server.hasPendingConnections());
}

void tst_NoQtEventLoop::consumeLocalSocketEvents()
{
    int argc = 1;
    char *argv[] = { const_cast<char *>("test"), 0 };
    QGuiApplication app(argc, argv);
    QLocalServer server;
    QLocalSocket client;
    QSignalSpy readyReadSpy(&client, &QIODevice::readyRead);

    QVERIFY(server.listen("consumeLocalSocketEvents"));
    client.connectToServer("consumeLocalSocketEvents");
    QVERIFY(client.waitForConnected(200));
    QVERIFY(server.waitForNewConnection(200));
    QLocalSocket *clientSocket = server.nextPendingConnection();
    QVERIFY(clientSocket);
    QSignalSpy bytesWrittenSpy(clientSocket, &QIODevice::bytesWritten);
    server.close();

    bool timeExpired = false;
    QTimer::singleShot(3000, Qt::CoarseTimer, [&timeExpired]() {
        timeExpired = true;
    });
    QVERIFY(clientSocket->putChar(0));

    // Exec own message loop
    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);

        if (timeExpired || readyReadSpy.count() != 0)
            break;
    }
    QVERIFY(!timeExpired);
    QCOMPARE(bytesWrittenSpy.count(), 1);
    QCOMPARE(readyReadSpy.count(), 1);
}

void tst_NoQtEventLoop::consumeWinEvents_data()
{
    QTest::addColumn<bool>("peeking");
    QTest::addColumn<bool>("nestedLoops");
    QTest::addColumn<bool>("nativeMainLoop");

    QTest::newRow("PeekMessage, single loop") << true << false << true;
    QTest::newRow("GetMessage, single loop") << false << false << true;

    QTest::newRow("PeekMessage, nested loops") << true << true << true;
    QTest::newRow("GetMessage, nested loops") << false << true << true;
    QTest::newRow("PeekMessage, nested loops, Qt main loop") << true << true << false;
    QTest::newRow("GetMessage, nested loops, Qt main loop") << false << true << false;
}

void tst_NoQtEventLoop::consumeWinEvents()
{
    QFETCH(bool, peeking);
    QFETCH(bool, nestedLoops);
    QFETCH(bool, nativeMainLoop);
    int argc = 1;
    char *argv[] = { const_cast<char *>("test"), 0 };
    QGuiApplication app(argc, argv);

    HANDLE winEvent = ::CreateEvent(0, FALSE, FALSE, 0);
    QVERIFY(winEvent != NULL);
    bool notifierActivated = false;
    QWinEventNotifier notifier(winEvent);
    connect(&notifier, &QWinEventNotifier::activated, [&notifierActivated]() {
        notifierActivated = true;
    });

    bool timeExpired = false;
    QTimer expiredTimer;
    connect(&expiredTimer, &QTimer::timeout, [&timeExpired]() {
        timeExpired = true;
    });
    expiredTimer.start(3000);

    auto runNativeEventLoop = [&timeExpired, &notifierActivated, peeking]() -> void {
        MSG msg;
        while (!(timeExpired || notifierActivated)) {
            if (peeking) {
                if (!::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                    QThread::msleep(100);
                    continue;
                }
            } else {
                if (::GetMessage(&msg, NULL, 0, 0) <= 0)
                    break;
            }

            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    };

    QTimer::singleShot(0, [winEvent, nestedLoops, runNativeEventLoop]() {
        ::SetEvent(winEvent);
        if (nestedLoops)
            runNativeEventLoop();
    });

    // Exec main message loop
    if (nativeMainLoop) {
        runNativeEventLoop();
    } else {
        QEventLoop loop;
        connect(&notifier, &QWinEventNotifier::activated,
                &loop, &QEventLoop::quit);
        connect(&expiredTimer, &QTimer::timeout,
                &loop, &QEventLoop::quit);

        loop.exec();
    }

    QVERIFY(notifierActivated);
    QVERIFY(!timeExpired);
}

void tst_NoQtEventLoop::deliverEventsInLivelock()
{
    int argc = 1;
    char *argv[] = { const_cast<char *>("test"), 0 };
    QGuiApplication app(argc, argv);

    QTimer livelockTimer;
    livelockTimer.start(0);
    QTimer::singleShot(100, Qt::CoarseTimer, &livelockTimer, &QTimer::stop);

    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    // Exec own message loop
    MSG msg;
    forever {
        if (elapsedTimer.hasExpired(3000) || !livelockTimer.isActive())
            break;

        if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    QVERIFY(!livelockTimer.isActive());
}

void tst_NoQtEventLoop::postingWithNoYieldFlag()
{
    int argc = 1;
    char *argv[] = { const_cast<char *>("test"), 0 };
    QGuiApplication app(argc, argv);

    bool signalReceived = false;
    // Post a message to the queue
    QMetaObject::invokeMethod(this, [&signalReceived]() {
        signalReceived = true;
    }, Qt::QueuedConnection);

    // Post some system messages
    QWindow mainWindow;
    mainWindow.show();

    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    // Exec own message loop
    MSG msg;
    forever {
        if (elapsedTimer.hasExpired(3000) || signalReceived)
            break;

        if (!::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE | PM_NOYIELD)) {
            QThread::msleep(100);
            continue;
        }

        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    QVERIFY(signalReceived);
}

#include <tst_noqteventloop.moc>

QTEST_APPLESS_MAIN(tst_NoQtEventLoop)

