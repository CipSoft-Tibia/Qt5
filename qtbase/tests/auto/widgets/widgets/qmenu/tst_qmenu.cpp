// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QtTest/private/qtesthelpers_p.h>
#include <qapplication.h>
#include <private/qguiapplication_p.h>
#include <QPushButton>
#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QToolButton>
#include <QStatusBar>
#include <QListWidget>
#include <QWidgetAction>
#include <QScreen>
#include <QSpinBox>
#include <QSignalSpy>
#include <qdialog.h>

#include <qmenu.h>
#include <qstyle.h>
#include <QStyleHints>
#include <QTimer>
#include <qdebug.h>
#include <QToolTip>
#include <qpa/qplatformtheme.h>
#include <qpa/qplatformintegration.h>

#include <QtWidgets/private/qapplication_p.h>
#include <QtWidgets/private/qmenu_p.h>

using namespace QTestPrivate;

Q_DECLARE_METATYPE(Qt::Key);
Q_DECLARE_METATYPE(Qt::KeyboardModifiers);

struct MenuMetrics {
    int fw;
    int hmargin;
    int vmargin;
    int tearOffHeight;

    MenuMetrics(const QMenu *menu) {
        fw = menu->style()->pixelMetric(QStyle::PM_MenuPanelWidth, nullptr, menu);
        hmargin = menu->style()->pixelMetric(QStyle::PM_MenuHMargin, nullptr, menu);
        vmargin = menu->style()->pixelMetric(QStyle::PM_MenuVMargin, nullptr, menu);
        tearOffHeight = menu->style()->pixelMetric(QStyle::PM_MenuTearoffHeight, nullptr, menu);
    }
};

class tst_QMenu : public QObject
{
    Q_OBJECT

public:
    tst_QMenu();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
private slots:
    void getSetCheck();
    void addActionsAndClear();
    void addActionsConnect();

    void keyboardNavigation_data();
    void keyboardNavigation();
    void focus();
    void overrideMenuAction();
    void statusTip();
    void widgetActionFocus();
    void mouseActivation();
    void tearOff();
    void submenuTearOffDontClose();
    void layoutDirection();

    void task208001_stylesheet();
    void activeSubMenuPosition();
    void activeSubMenuPositionExec();
    void task242454_sizeHint();
    void task176201_clear();
    void task250673_activeMultiColumnSubMenuPosition();
    void task256918_setFont();
    void menuSizeHint();
    void task258920_mouseBorder();
    void setFixedWidth();
    void deleteActionInTriggered();
    void pushButtonPopulateOnAboutToShow();
    void QTBUG7907_submenus_autoselect();
    void QTBUG7411_submenus_activate();
    void QTBUG30595_rtl_submenu();
    void QTBUG20403_nested_popup_on_shortcut_trigger();
    void QTBUG47515_widgetActionEnterLeave();
    void QTBUG_89082_actionTipsHide();
    void QTBUG8122_widgetActionCrashOnClose();
    void widgetActionTriggerClosesMenu();
    void transientParent();

    void QTBUG_10735_crashWithDialog();
#ifdef Q_OS_MAC
    void QTBUG_37933_ampersands_data();
    void QTBUG_37933_ampersands();
#else
    void click_while_dismissing_submenu();
#endif
    void QTBUG_56917_wideMenuSize();
    void QTBUG_56917_wideMenuScreenNumber();
    void QTBUG_56917_wideSubmenuScreenNumber();
    void menuSize_Scrolling_data();
    void menuSize_Scrolling();
    void tearOffMenuNotDisplayed();
    void QTBUG_61039_menu_shortcuts();
    void screenOrientationChangedCloseMenu();
    void deleteWhenTriggered();

    void nestedTearOffDetached();

protected slots:
    void onActivated(QAction*);
    void onHighlighted(QAction*);
    void onStatusMessageChanged(const QString &);
    void onStatusTipTimer();
    void deleteAction(QAction *a) { delete a; }
private:
    void createActions();
    QMenu *menus[2], *lastMenu;
    enum { num_builtins = 10 };
    QAction *activated, *highlighted, *builtins[num_builtins];
    QString statustip;
    bool m_onStatusTipTimerExecuted;
};

// Testing get/set functions
void tst_QMenu::getSetCheck()
{
    QMenu obj1;
    // QAction * QMenu::defaultAction()
    // void QMenu::setDefaultAction(QAction *)
    QAction *var1 = new QAction(0);
    obj1.setDefaultAction(var1);
    QCOMPARE(var1, obj1.defaultAction());
    obj1.setDefaultAction((QAction *)0);
    QCOMPARE((QAction *)0, obj1.defaultAction());
    delete var1;

    // QAction * QMenu::activeAction()
    // void QMenu::setActiveAction(QAction *)
    QAction *var2 = new QAction(0);
    obj1.setActiveAction(var2);
    QCOMPARE(var2, obj1.activeAction());
    obj1.setActiveAction((QAction *)0);
    QCOMPARE((QAction *)0, obj1.activeAction());
    delete var2;
}

tst_QMenu::tst_QMenu()
    : m_onStatusTipTimerExecuted(false)
{
    QApplication::setEffectEnabled(Qt::UI_FadeTooltip, false);
    QApplication::setEffectEnabled(Qt::UI_AnimateMenu, false);
    QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, false);
}

void tst_QMenu::initTestCase()
{
    for (int i = 0; i < num_builtins; i++)
        builtins[i] = 0;
    for (int i = 0; i < 2; i++) {
        menus[i] = new QMenu;
        QObject::connect(menus[i], SIGNAL(triggered(QAction*)), this, SLOT(onActivated(QAction*)));
        QObject::connect(menus[i], SIGNAL(hovered(QAction*)), this, SLOT(onHighlighted(QAction*)));
    }
}

void tst_QMenu::cleanupTestCase()
{
    for (int i = 0; i < 2; i++)
        menus[i]->clear();
    for (int i = 0; i < num_builtins; i++) {
        bool menuAction = false;
        for (int j = 0; j < 2; ++j)
            if (menus[j]->menuAction() == builtins[i])
                menuAction = true;
        if (!menuAction)
            delete builtins[i];
    }
    delete menus[0];
    delete menus[1];
}

void tst_QMenu::init()
{
    activated = highlighted = 0;
    lastMenu = 0;
    m_onStatusTipTimerExecuted = false;
}

void tst_QMenu::createActions()
{
    if (!builtins[0])
        builtins[0] = new QAction("New", 0);
    menus[0]->addAction(builtins[0]);

    if (!builtins[1]) {
        builtins[1] = new QAction(0);
        builtins[1]->setSeparator(true);
    }
    menus[0]->addAction(builtins[1]);

    if (!builtins[2]) {
        builtins[2] = menus[1]->menuAction();
        builtins[2]->setText("&Open..");
        builtins[8] = new QAction("Close", 0);
        menus[1]->addAction(builtins[8]);
        builtins[9] = new QAction("Quit", 0);
        menus[1]->addAction(builtins[9]);
    }
    menus[0]->addAction(builtins[2]);

    if (!builtins[3])
        builtins[3] = new QAction("Open &as..", 0);
    menus[0]->addAction(builtins[3]);

    if (!builtins[4]) {
        builtins[4] = new QAction("Save", 0);
        builtins[4]->setEnabled(false);
    }
    menus[0]->addAction(builtins[4]);

    if (!builtins[5])
        builtins[5] = new QAction("Sa&ve as..", 0);
    menus[0]->addAction(builtins[5]);

    if (!builtins[6]) {
        builtins[6] = new QAction(0);
        builtins[6]->setSeparator(true);
    }
    menus[0]->addAction(builtins[6]);

    if (!builtins[7])
        builtins[7] = new QAction("Prin&t", 0);
    menus[0]->addAction(builtins[7]);
}

void tst_QMenu::onHighlighted(QAction *action)
{
    highlighted = action;
    lastMenu = qobject_cast<QMenu*>(sender());
}

void tst_QMenu::onActivated(QAction *action)
{
    activated = action;
    lastMenu = qobject_cast<QMenu*>(sender());
}

void tst_QMenu::onStatusMessageChanged(const QString &s)
{
    statustip=s;
}

void tst_QMenu::addActionsAndClear()
{
    QCOMPARE(menus[0]->actions().size(), 0);
    createActions();
    QCOMPARE(menus[0]->actions().size(), 8);
    menus[0]->clear();
    QCOMPARE(menus[0]->actions().size(), 0);
}

static void testFunction0() {}
static void testFunction1(bool) {}

template <typename T>
struct ImplicitlyConvertibleTo {
    T t;
    operator const T() const { return t; }
    operator T() { return t; }
};

void tst_QMenu::addActionsConnect()
{
    // almost exhaustive check of addAction() overloads:
    // (text), (icon, text), (icon, text, shortcut), (text, shortcut)
    // each with a good sample of ways to QObject::connect() to
    // QAction::triggered(bool)
    QMenu menu;

    // don't just pass QString etc - that'd be too easy (think QStringBuilder)
    ImplicitlyConvertibleTo<QString> text = {QLatin1String("bla")};
    ImplicitlyConvertibleTo<QIcon> icon;

    const auto check = [&](auto &...args) { // don't need to perfectly-forward, only lvalues passed
        menu.addAction(args...);

        menu.addAction(args..., &menu, SLOT(deleteLater()));
        menu.addAction(args..., &menu, &QMenu::deleteLater);
        menu.addAction(args..., testFunction0);
        menu.addAction(args..., &menu, testFunction0);
        menu.addAction(args..., testFunction1);
        menu.addAction(args..., &menu, testFunction1);
        menu.addAction(args..., [&](bool b) { menu.setEnabled(b); });
        menu.addAction(args..., &menu, [&](bool b) { menu.setEnabled(b); });

        menu.addAction(args..., &menu, SLOT(deleteLater()), Qt::QueuedConnection);
        menu.addAction(args..., &menu, &QMenu::deleteLater, Qt::QueuedConnection);
        // doesn't exist: menu.addAction(args..., testFunction0, Qt::QueuedConnection);
        menu.addAction(args..., &menu, testFunction0, Qt::QueuedConnection);
        // doesn't exist: menu.addAction(args..., testFunction1, Qt::QueuedConnection);
        menu.addAction(args..., &menu, testFunction1, Qt::QueuedConnection);
        // doesn't exist: menu.addAction(args..., [&](bool b) { menu.setEnabled(b); }, Qt::QueuedConnection);
        menu.addAction(args..., &menu, [&](bool b) { menu.setEnabled(b); }, Qt::QueuedConnection);
    };
    const auto check1 = [&](auto &arg, auto &...args) {
        check(arg, args...);
        check(std::as_const(arg), args...);
    };
    const auto check2 = [&](auto &arg1, auto &arg2, auto &...args) {
        check1(arg1, arg2, args...);
        check1(arg1, std::as_const(arg2), args...);
    };
    [[maybe_unused]]
    const auto check3 = [&](auto &arg1, auto &arg2, auto &arg3) {
        check2(arg1, arg2, arg3);
        check2(arg1, arg2, std::as_const(arg3));
    };

    check1(text);
    check2(icon, text);
#ifndef QT_NO_SHORTCUT
    ImplicitlyConvertibleTo<QKeySequence> keySequence = {Qt::CTRL | Qt::Key_C};
    check2(text, keySequence);
    check3(icon, text, keySequence);
#if QT_DEPRECATED_SINCE(6, 4)
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    menu.addAction(text, &menu, SLOT(deleteLater()), keySequence);
    menu.addAction(text, &menu, &QMenu::deleteLater, keySequence);
    menu.addAction(text, testFunction0, keySequence);
    menu.addAction(text, &menu, testFunction0, keySequence);
    menu.addAction(text, testFunction1, keySequence);
    menu.addAction(text, &menu, testFunction1, keySequence);
    menu.addAction(icon, text, &menu, SLOT(deleteLater()), keySequence);
    menu.addAction(icon, text, &menu, &QMenu::deleteLater, keySequence);
    menu.addAction(icon, text, testFunction0, keySequence);
    menu.addAction(icon, text, &menu, testFunction0, keySequence);
    menu.addAction(icon, text, testFunction1, keySequence);
    menu.addAction(icon, text, &menu, testFunction1, keySequence);
    QT_WARNING_POP
#endif
#endif // !QT_NO_SHORTCUT
}

void tst_QMenu::mouseActivation()
{
    QWidget topLevel;
    topLevel.resize(300, 200);
    centerOnScreen(&topLevel);
    QMenu menu(&topLevel);
    topLevel.show();
    menu.addAction("Menu Action");
    menu.move(topLevel.geometry().topRight() + QPoint(50, 0));
    menu.show();
    QTest::mouseClick(&menu, Qt::LeftButton, {}, menu.rect().center(), 300);
    QVERIFY(!menu.isVisible());

    //context menus can always be accessed with right click except on windows
    menu.show();
    QTest::mouseClick(&menu, Qt::RightButton, {}, menu.rect().center(), 300);
    QVERIFY(!menu.isVisible());

#ifdef Q_OS_WIN
    //on windows normal mainwindow menus Can only be accessed with left mouse button
    QMenuBar menubar;
    QMenu submenu("Menu");
    submenu.addAction("action");
    QAction *action = menubar.addMenu(&submenu);
    menubar.move(topLevel.geometry().topRight() + QPoint(300, 0));
    menubar.show();


    QTest::mouseClick(&menubar, Qt::LeftButton, {}, menubar.actionGeometry(action).center(), 300);
    QVERIFY(submenu.isVisible());
    QTest::mouseClick(&submenu, Qt::LeftButton, {}, QPoint(5, 5), 300);
    QVERIFY(!submenu.isVisible());

    QTest::mouseClick(&menubar, Qt::LeftButton, {}, menubar.actionGeometry(action).center(), 300);
    QVERIFY(submenu.isVisible());
    QTest::mouseClick(&submenu, Qt::RightButton, {}, QPoint(5, 5), 300);
    QVERIFY(submenu.isVisible());
#endif
}

void tst_QMenu::keyboardNavigation_data()
{
    QTest::addColumn<Qt::Key>("key");
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers");
    QTest::addColumn<int>("expected_action");
    QTest::addColumn<int>("expected_menu");
    QTest::addColumn<bool>("init");
    QTest::addColumn<bool>("expected_activated");
    QTest::addColumn<bool>("expected_highlighted");

    //test up and down (order is important here)
    QTest::newRow("data0") << Qt::Key(Qt::Key_Down) << Qt::KeyboardModifiers(Qt::NoModifier) << 0 << 0 << true << false << true;
    QTest::newRow("data1") << Qt::Key(Qt::Key_Down) << Qt::KeyboardModifiers(Qt::NoModifier) << 2 << 0 << false << false << true; //skips the separator
    QTest::newRow("data2") << Qt::Key(Qt::Key_Down) << Qt::KeyboardModifiers(Qt::NoModifier) << 3 << 0 << false << false << true;

    if (QApplication::style()->styleHint(QStyle::SH_Menu_AllowActiveAndDisabled))
        QTest::newRow("data3_noMac") << Qt::Key(Qt::Key_Down) << Qt::KeyboardModifiers(Qt::NoModifier) << 4 << 0 << false << false << true;
    else
        QTest::newRow("data3_Mac") << Qt::Key(Qt::Key_Down) << Qt::KeyboardModifiers(Qt::NoModifier) << 5 << 0 << false << false << true;
    QTest::newRow("data4") << Qt::Key(Qt::Key_Up) << Qt::KeyboardModifiers(Qt::NoModifier) << 3 << 0 << false << false << true;
    QTest::newRow("data5") << Qt::Key(Qt::Key_Up) << Qt::KeyboardModifiers(Qt::NoModifier) << 2 << 0 << false << false << true;
    QTest::newRow("data6") << Qt::Key(Qt::Key_Right) << Qt::KeyboardModifiers(Qt::NoModifier) << 8 << 1 << false << false << true;
    QTest::newRow("data7") << Qt::Key(Qt::Key_Down) << Qt::KeyboardModifiers(Qt::NoModifier) << 9 << 1 << false << false << true;
    QTest::newRow("data8") << Qt::Key(Qt::Key_Escape) << Qt::KeyboardModifiers(Qt::NoModifier) << 2 << 0 << false << false << false;
    QTest::newRow("data9") << Qt::Key(Qt::Key_Down) << Qt::KeyboardModifiers(Qt::NoModifier) << 3 << 0 << false << false<< true;
    QTest::newRow("data10") << Qt::Key(Qt::Key_Return) << Qt::KeyboardModifiers(Qt::NoModifier) << 3 << 0 << false << true << false;

    if (QGuiApplication::platformName().compare(QLatin1String("xcb"), Qt::CaseInsensitive)) {
        // Test shortcuts.
        QTest::newRow("shortcut0") << Qt::Key(Qt::Key_V) << Qt::KeyboardModifiers(Qt::AltModifier) << 5 << 0 << true << true << false;
    }
}

void tst_QMenu::keyboardNavigation()
{
    QFETCH(Qt::Key, key);
    QFETCH(Qt::KeyboardModifiers, modifiers);
    QFETCH(int, expected_action);
    QFETCH(int, expected_menu);
    QFETCH(bool, init);
    QFETCH(bool, expected_activated);
    QFETCH(bool, expected_highlighted);

    if (init) {
        lastMenu = menus[0];
        lastMenu->clear();
        createActions();
        lastMenu->popup(QPoint(0, 0));
    }

    QTest::keyClick(lastMenu, key, modifiers);
    if (expected_activated) {
#ifdef Q_OS_MAC
        QEXPECT_FAIL("shortcut0", "Shortcut navication fails, see QTBUG-23684", Continue);
#endif
        QCOMPARE(activated, builtins[expected_action]);
#ifndef Q_OS_MAC
        QEXPECT_FAIL("shortcut0", "QTBUG-22449: QMenu doesn't remove highlight if a menu item is activated by a shortcut", Abort);
#endif
        QCOMPARE(menus[expected_menu]->activeAction(), nullptr);
    } else {
        QCOMPARE(menus[expected_menu]->activeAction(), builtins[expected_action]);
    }

    if (expected_highlighted)
        QCOMPARE(menus[expected_menu]->activeAction(), highlighted);
    else
        QCOMPARE(highlighted, nullptr);
}

#ifdef Q_OS_MAC
QT_BEGIN_NAMESPACE
extern bool qt_tab_all_widgets(); // from qapplication.cpp
QT_END_NAMESPACE
#endif

void tst_QMenu::focus()
{
    QMenu menu;
    menu.addAction("One");
    menu.addAction("Two");
    menu.addAction("Three");

#ifdef Q_OS_MAC
    if (!qt_tab_all_widgets())
        QSKIP("Computer is currently set up to NOT tab to all widgets,"
             " this test assumes you can tab to all widgets");
#endif

    QWidget window;
    window.resize(300, 200);
    QPushButton button("Push me", &window);
    centerOnScreen(&window);
    window.show();
    QApplicationPrivate::setActiveWindow(&window);

    QVERIFY(button.hasFocus());
    QCOMPARE(QApplication::focusWidget(), (QWidget *)&button);
    QCOMPARE(QApplication::activeWindow(), &window);
    menu.move(window.geometry().topRight() + QPoint(50, 0));
    menu.show();
    QVERIFY(button.hasFocus());
    QCOMPARE(QApplication::focusWidget(), (QWidget *)&button);
    QCOMPARE(QApplication::activeWindow(), &window);
    menu.hide();
    QVERIFY(button.hasFocus());
    QCOMPARE(QApplication::focusWidget(), (QWidget *)&button);
    QCOMPARE(QApplication::activeWindow(), &window);
}

void tst_QMenu::overrideMenuAction()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    //test the override menu action by first creating an action to which we set its menu
    QMainWindow w;
    w.resize(300, 200);
    w.menuBar()->setNativeMenuBar(false);
    centerOnScreen(&w);

    QAction *aFileMenu = new QAction("&File", &w);
    w.menuBar()->addAction(aFileMenu);

    QMenu *m = new QMenu(&w);
    QAction *menuaction = m->menuAction();
    connect(m, SIGNAL(triggered(QAction*)), SLOT(onActivated(QAction*)));
    aFileMenu->setMenu(m); //this sets the override menu action for the QMenu
    QCOMPARE(m->menuAction(), aFileMenu);

    // On Mac and Windows CE, we need to create native key events to test menu
    // action activation, so skip this part of the test.
#if QT_CONFIG(shortcut) && !defined(Q_OS_DARWIN)
    QAction *aQuit = new QAction("Quit", &w);
    aQuit->setShortcut(QKeySequence("Ctrl+X"));
    m->addAction(aQuit);

    w.show();
    QApplicationPrivate::setActiveWindow(&w);
    w.setFocus();
    QVERIFY(QTest::qWaitForWindowActive(&w));
    QVERIFY(w.hasFocus());

    //test of the action inside the menu
    QTest::keyClick(&w, Qt::Key_X, Qt::ControlModifier);
    QTRY_COMPARE(activated, aQuit);

    //test if the menu still pops out
    QTest::keyClick(&w, Qt::Key_F, Qt::AltModifier);
    QTRY_VERIFY(m->isVisible());
#endif // QT_CONFIG(shortcut) && !Q_OS_DARWIN

    delete aFileMenu;

    //after the deletion of the override menu action,
    //the menu should have its default menu action back
    QCOMPARE(m->menuAction(), menuaction);
}

void tst_QMenu::statusTip()
{
    //check that the statustip of actions inserted into the menu are displayed
    QMainWindow w;
    w.resize(300, 200);
    centerOnScreen(&w);
    connect(w.statusBar(), SIGNAL(messageChanged(QString)), SLOT(onStatusMessageChanged(QString)));; //creates the status bar
    QToolBar tb;
    QAction a("main action", &tb);
    a.setStatusTip("main action");
    QMenu m(&tb);
    QAction subact("sub action", &m);
    subact.setStatusTip("sub action");
    m.addAction(&subact);
    a.setMenu(&m);
    tb.addAction(&a);

    w.addToolBar(&tb);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QRect rect1 = tb.actionGeometry(&a);
    QToolButton *btn = qobject_cast<QToolButton*>(tb.childAt(rect1.center()));

    QVERIFY(btn != nullptr);

    //because showMenu calls QMenu::exec, we need to use a singleshot
    //to continue the test
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, &tst_QMenu::onStatusTipTimer);
    timer.setInterval(200);
    timer.start();
    btn->showMenu();
    QVERIFY(m_onStatusTipTimerExecuted);
    QVERIFY(statustip.isEmpty());
}

//2nd part of the test
void tst_QMenu::onStatusTipTimer()
{
    QMenu *menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
    QVERIFY(menu != 0);
    QVERIFY(menu->isVisible());
    QTest::keyClick(menu, Qt::Key_Down);

    //we store the statustip to press escape in any case
    //otherwise, if the test fails it blocks (never gets out of QMenu::exec
    const QString st=statustip;

    menu->close(); //goes out of the menu

    QCOMPARE(st, QString("sub action"));
    QVERIFY(!menu->isVisible());
    m_onStatusTipTimerExecuted = true;
}

void tst_QMenu::widgetActionFocus()
{
    //test if the focus is correctly handled with a QWidgetAction
    QMenu m;
    QListWidget *l = new QListWidget(&m);
    for (int i = 1; i<3 ; i++)
        l->addItem(QStringLiteral("item" ) + QString::number(i));
    QWidgetAction *wa = new QWidgetAction(&m);
    wa->setDefaultWidget(l);
    m.addAction(wa);
    m.setActiveAction(wa);
    l->setFocus(); //to ensure it has primarily the focus
    QAction *menuitem1=m.addAction("menuitem1");
    QAction *menuitem2=m.addAction("menuitem2");

    m.popup(QPoint());

    QVERIFY(m.isVisible());
    QVERIFY(l->hasFocus());
    QVERIFY(l->currentItem());
    QCOMPARE(l->currentItem()->text(), QString("item1"));

    QTest::keyClick(QApplication::focusWidget(), Qt::Key_Down);
    QVERIFY(l->currentItem());
    QCOMPARE(l->currentItem()->text(), QString("item2"));

    QTest::keyClick(QApplication::focusWidget(), Qt::Key_Down);
    QVERIFY(m.hasFocus());
    QCOMPARE(m.activeAction(), menuitem1);

    QTest::keyClick(QApplication::focusWidget(), Qt::Key_Down);
    QVERIFY(m.hasFocus());
    QCOMPARE(m.activeAction(), menuitem2);

    QTest::keyClick(QApplication::focusWidget(), Qt::Key_Up);
    QVERIFY(m.hasFocus());
    QCOMPARE(m.activeAction(), menuitem1);

    QTest::keyClick(QApplication::focusWidget(), Qt::Key_Up);
    QVERIFY(l->hasFocus());
    QCOMPARE(m.activeAction(), (QAction *)wa);
}

static QMenu *getTornOffMenu()
{
    for (QWidget *w : QApplication::allWidgets()) {
        if (w->isVisible() && w->inherits("QTornOffMenu"))
            return static_cast<QMenu *>(w);
    }
    return nullptr;
}

void tst_QMenu::tearOff()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QWidget widget;
    QScopedPointer<QMenu> menu(new QMenu(&widget));
    QVERIFY(!menu->isTearOffEnabled()); //default value
    menu->setTearOffEnabled(true);
    menu->setTitle(QLatin1String("Same &Menu"));
    menu->addAction("aaa");
    menu->addAction("bbb");
    QVERIFY(menu->isTearOffEnabled());

    widget.resize(300, 200);
    centerOnScreen(&widget);
    widget.show();
    widget.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&widget));
    menu->popup(widget.geometry().topRight() + QPoint(50, 0));
    QVERIFY(QTest::qWaitForWindowActive(menu.data()));
    QVERIFY(!menu->isTearOffMenuVisible());

    MenuMetrics mm(menu.data());
    const int tearOffOffset = mm.fw + mm.vmargin + mm.tearOffHeight / 2;

    QTest::mouseClick(menu.data(), Qt::LeftButton, {}, QPoint(10, tearOffOffset), 10);
    QTRY_VERIFY(menu->isTearOffMenuVisible());
    QPointer<QMenu> torn = getTornOffMenu();
    QVERIFY(torn);
    QVERIFY(torn->isVisible());

    // Check menu title
    const QString cleanTitle = QPlatformTheme::removeMnemonics(menu->title()).trimmed();
    QCOMPARE(torn->windowTitle(), cleanTitle);

    // Change menu title and check again
    menu->setTitle(QLatin1String("Sample &Menu"));
    const QString newCleanTitle = QPlatformTheme::removeMnemonics(menu->title()).trimmed();
    QCOMPARE(torn->windowTitle(), newCleanTitle);

    // Clear menu title and check again
    menu->setTitle(QString());
    QCOMPARE(torn->windowTitle(), QString());

    menu->hideTearOffMenu();
    QVERIFY(!menu->isTearOffMenuVisible());
    QVERIFY(!torn->isVisible());

#ifndef QT_NO_CURSOR
    // Test under-mouse positioning
    menu->showTearOffMenu();
    torn = getTornOffMenu();
    QVERIFY(torn);
    QVERIFY(torn->isVisible());
    QVERIFY(menu->isTearOffMenuVisible());
    // Some platforms include the window title bar in its geometry.
    QTRY_COMPARE(torn->windowHandle()->position(), QCursor::pos());

    menu->hideTearOffMenu();
    QVERIFY(!menu->isTearOffMenuVisible());
    QVERIFY(!torn->isVisible());

    // Test custom positioning
    const QPoint &pos = QCursor::pos() / 2 + QPoint(10, 10);
    menu->showTearOffMenu(pos);
    torn = getTornOffMenu();
    QVERIFY(torn);
    QVERIFY(torn->isVisible());
    QVERIFY(menu->isTearOffMenuVisible());
    // Some platforms include the window title bar in its geometry.
    QTRY_COMPARE(torn->windowHandle()->position(), pos);
#endif // QT_NO_CURSOR
}

void tst_QMenu::submenuTearOffDontClose()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QWidget widget;
    QMenu *menu = new QMenu(&widget);
    QVERIFY(!menu->isTearOffEnabled()); //default value
    menu->setTearOffEnabled(true);
    QVERIFY(menu->isTearOffEnabled());
    QMenu *submenu = new QMenu(&widget);
    submenu->addAction("aaa");
    submenu->addAction("bbb");
    QVERIFY(!submenu->isTearOffEnabled()); //default value
    submenu->setTearOffEnabled(true);
    QVERIFY(submenu->isTearOffEnabled());
    menu->addMenu(submenu);

    widget.resize(300, 200);
    centerOnScreen(&widget);
    widget.show();
    widget.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&widget));
    // Show parent menu
    menu->popup(widget.geometry().topRight() + QPoint(50, 0));
    QVERIFY(QTest::qWaitForWindowActive(menu));
    // Then its submenu
    const QRect submenuRect = menu->actionGeometry(menu->actions().at(0));
    const QPoint submenuPos(submenuRect.topLeft() + QPoint(3, 3));
    // Move then click to avoid the submenu moves from causing it to close
    QTest::mouseMove(menu, submenuPos, 100);
    QTest::mouseClick(menu, Qt::LeftButton, {}, submenuPos, 100);
    QVERIFY(QTest::qWaitFor([&]() { return submenu->window()->windowHandle(); }));
    QVERIFY(QTest::qWaitForWindowActive(submenu));
    // Make sure we enter the submenu frame directly on the tear-off area
    QTest::mouseMove(submenu, QPoint(10, 3), 100);
    if (submenu->style()->styleHint(QStyle::SH_Menu_SubMenuDontStartSloppyOnLeave)) {
        qWarning("Sloppy menu timer disabled by the style: %s", qPrintable(QApplication::style()->objectName()));
        // Submenu must get the enter event
        QTRY_VERIFY(submenu->underMouse());
    } else {
        const int closeTimeout = submenu->style()->styleHint(QStyle::SH_Menu_SubMenuSloppyCloseTimeout);
        QTest::qWait(closeTimeout + 100);
        // Menu must not disappear and it must get the enter event
        QVERIFY(submenu->isVisible());
        QVERIFY(submenu->underMouse());
    }
}

void tst_QMenu::layoutDirection()
{
    QMainWindow win;
    win.setLayoutDirection(Qt::RightToLeft);
    win.resize(300, 200);
    centerOnScreen(&win);

    QMenu menu(&win);
    menu.addAction("foo");
    menu.move(win.geometry().topRight() + QPoint(50, 0));
    menu.show();
    QVERIFY(QTest::qWaitForWindowExposed(&menu));
    QCOMPARE(menu.layoutDirection(), Qt::RightToLeft);
    menu.close();

    menu.setParent(0);
    menu.move(win.geometry().topRight() + QPoint(50, 0));
    menu.show();
    QVERIFY(QTest::qWaitForWindowExposed(&menu));
    QCOMPARE(menu.layoutDirection(), QApplication::layoutDirection());
    menu.close();

    //now the menubar
    QAction *action = win.menuBar()->addMenu(&menu);
    win.menuBar()->setActiveAction(action);
    QVERIFY(QTest::qWaitForWindowExposed(&menu));
    QCOMPARE(menu.layoutDirection(), Qt::RightToLeft);
}

void tst_QMenu::task208001_stylesheet()
{
    //test if it crash
    QMainWindow main;
    main.setStyleSheet("QMenu [title =\"File\"] { color: red;}");
    main.menuBar()->addMenu("File");
}

void tst_QMenu::activeSubMenuPosition()
{
    QPushButton lab("subMenuPosition test");

    QMenu *sub = new QMenu("Submenu", &lab);
    sub->addAction("Sub-Item1");
    QAction *subAction = sub->addAction("Sub-Item2");

    QMenu *main = new QMenu("Menu-Title", &lab);
    (void)main->addAction("Item 1");
    QAction *menuAction = main->addMenu(sub);
    (void)main->addAction("Item 3");
    (void)main->addAction("Item 4");

    main->setActiveAction(menuAction);
    sub->setActiveAction(subAction);
    main->popup(QPoint(200,200));

    QVERIFY(main->isVisible());
    QCOMPARE(main->activeAction(), menuAction);
    QVERIFY(sub->isVisible());
    QVERIFY(sub->pos() != QPoint(0,0));
    // well, it's enough to check the pos is not (0,0) but it's more safe
    // to check that submenu is to the right of the main menu too.
    QVERIFY(sub->pos().x() > main->pos().x());
    QCOMPARE(sub->activeAction(), subAction);
}

// QTBUG-49588, QTBUG-48396: activeSubMenuPositionExec() is the same as
// activeSubMenuPosition(), but uses QMenu::exec(), which produces a different
// sequence of events. Verify that the sub menu is positioned to the right of the
// main menu.
class SubMenuPositionExecMenu : public QMenu
{
    Q_OBJECT
public:
    SubMenuPositionExecMenu() : QMenu("Menu-Title"), m_timerId(-1), m_timerTick(0)
    {
        addAction("Item 1");
        m_subMenu = addMenu("Submenu");
        m_subAction = m_subMenu->addAction("Sub-Item1");
        setActiveAction(m_subMenu->menuAction());
    }

protected:
    void showEvent(QShowEvent *e) override
    {
        QVERIFY(m_subMenu->isVisible());
        QVERIFY2(m_subMenu->x() > x(),
                 (QByteArray::number(m_subMenu->x()) + ' ' + QByteArray::number(x())).constData());
        m_timerId = startTimer(50);
        QMenu::showEvent(e);
    }

    void timerEvent(QTimerEvent *e) override
    {
        if (e->timerId() == m_timerId) {
            switch (m_timerTick++) {
            case 0:
                m_subMenu->close();
                break;
            case 1:
                close();
                break;
            }
        }
    }

private:
    int m_timerId;
    int m_timerTick;
    QMenu *m_subMenu;
    QAction *m_subAction;
};

void tst_QMenu::activeSubMenuPositionExec()
{
#ifdef Q_OS_ANDROID
    // QTBUG-87424
    QSKIP("Android: This hangs. Figure out why.");
#endif
    SubMenuPositionExecMenu menu;
    menu.exec(QGuiApplication::primaryScreen()->availableGeometry().center());
}

void tst_QMenu::task242454_sizeHint()
{
#ifdef Q_OS_ANDROID
    // QTBUG-87424
    QSKIP("Android: This hangs. Figure out why.");
#endif
    QMenu menu;
    QString s = QLatin1String("foo\nfoo\nfoo\nfoo");
    menu.addAction(s);
    QVERIFY(menu.sizeHint().width() > menu.fontMetrics().boundingRect(QRect(), Qt::TextSingleLine, s).width());
}

class Menu : public QMenu
{
    Q_OBJECT
public slots:
    void clear()
    {
        QMenu::clear();
    }
};

void tst_QMenu::task176201_clear()
{
    //this test used to crash
    Menu menu;
    QAction *action = menu.addAction("test");
    menu.connect(action, SIGNAL(triggered()), SLOT(clear()));
    menu.popup(QPoint());
    QTest::mouseClick(&menu, Qt::LeftButton, {}, menu.rect().center());
}

void tst_QMenu::task250673_activeMultiColumnSubMenuPosition()
{
    class MyMenu : public QMenu
    {
    public:
        int columnCount() const { return QMenu::columnCount(); }
    };

    QMenu sub;

    if (sub.style()->styleHint(QStyle::SH_Menu_Scrollable, 0, &sub)) {
        //the style prevents the menus from getting columns
        QSKIP("the style doesn't support multiple columns, it makes the menu scrollable");
    }

    sub.addAction("Sub-Item1");
    QAction *subAction = sub.addAction("Sub-Item2");

    MyMenu main;
    main.addAction("Item 1");
    QAction *menuAction = main.addMenu(&sub);
    main.popup(QPoint(200,200));

    uint i = 2;
    while (main.columnCount() < 2) {
        main.addAction(QLatin1String("Item ") + QString::number(i));
        ++i;
        QVERIFY(i<1000);
    }
    main.setActiveAction(menuAction);
    sub.setActiveAction(subAction);

    QVERIFY(main.isVisible());
    QCOMPARE(main.activeAction(), menuAction);
    QVERIFY(sub.isVisible());
    QVERIFY(sub.pos().x() > main.pos().x());

    const int subMenuOffset = main.style()->pixelMetric(QStyle::PM_SubMenuOverlap, 0, &main);
    QVERIFY((sub.geometry().left() - subMenuOffset + 5) < main.geometry().right());
}

void tst_QMenu::task256918_setFont()
{
    QMenu menu;
    QAction *action = menu.addAction("foo");
    QFont f;
    f.setPointSize(30);
    action->setFont(f);
    centerOnScreen(&menu, QSize(120, 40));
    menu.show(); //ensures that the actiongeometry are calculated
    QVERIFY(menu.actionGeometry(action).height() > f.pointSize());
}

void tst_QMenu::menuSizeHint()
{
    QMenu menu;
    //this is a list of arbitrary strings so that we check the geometry
    for (auto str : {"trer", "ezrfgtgvqd", "sdgzgzerzerzer", "eerzertz", "er"})
        menu.addAction(str);

    const QMargins cm = menu.contentsMargins();
    const int panelWidth = menu.style()->pixelMetric(QStyle::PM_MenuPanelWidth, 0, &menu);
    const int hmargin = menu.style()->pixelMetric(QStyle::PM_MenuHMargin, 0, &menu),
    vmargin = menu.style()->pixelMetric(QStyle::PM_MenuVMargin, 0, &menu);

    int maxWidth =0;
    QRect result;
    for (QAction *action : menu.actions()) {
        maxWidth = qMax(maxWidth, menu.actionGeometry(action).width());
        result |= menu.actionGeometry(action);
        QCOMPARE(result.x(), cm.left() + hmargin + panelWidth);
        QCOMPARE(result.y(), cm.top() + vmargin + panelWidth);
    }

    QStyleOption opt(0);
    opt.rect = menu.rect();
    opt.state = QStyle::State_None;

    QSize resSize = QSize(result.x(), result.y()) + result.size() + QSize(hmargin + cm.right() + panelWidth, vmargin + cm.top() + panelWidth);

    resSize = menu.style()->sizeFromContents(QStyle::CT_Menu, &opt, resSize, &menu);

    QCOMPARE(resSize, menu.sizeHint());
}

class Menu258920 : public QMenu
{
    Q_OBJECT
public slots:
    void paintEvent(QPaintEvent *e) override
    {
        QMenu::paintEvent(e);
        painted = true;
    }

public:
    bool painted;
};

// Mouse move related signals for Windows Mobile unavailable
void tst_QMenu::task258920_mouseBorder()
{
    const QRect screenGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    Menu258920 menu;
    QCursor::setPos(screenGeometry.topLeft());
    if (!QTest::qWaitFor([screenGeometry]{ return QCursor::pos() == screenGeometry.topLeft(); }))
        QSKIP("Can't move cursor out of the way");
    // For styles which inherit from QWindowsStyle, styleHint(QStyle::SH_Menu_MouseTracking) is true.
    menu.setMouseTracking(true);
    QAction *action = menu.addAction("test");

    const QPoint center = screenGeometry.center();
    menu.popup(center);
    QVERIFY(QTest::qWaitForWindowExposed(&menu));
    QRect actionRect = menu.actionGeometry(action);
    const QPoint actionCenter = actionRect.center();
    QTest::mouseMove(&menu, actionCenter - QPoint(-10, 0));
    QTest::mouseMove(&menu, actionCenter);
    QTest::mouseMove(&menu, actionCenter + QPoint(10, 0));
    QTRY_COMPARE(action, menu.activeAction());
    menu.painted = false;
    QTest::mouseMove(&menu, QPoint(actionRect.center().x(), actionRect.bottom() + 1));
    QTRY_COMPARE(static_cast<QAction*>(0), menu.activeAction());
    QTRY_VERIFY(menu.painted);
}

void tst_QMenu::setFixedWidth()
{
    QMenu menu;
    menu.addAction("action");
    menu.setFixedWidth(300);
    //the sizehint should reflect the minimumwidth because the action will try to
    //get as much space as possible
    QCOMPARE(menu.sizeHint().width(), menu.minimumWidth());
}

void tst_QMenu::deleteActionInTriggered()
{
    // should not crash
    QMenu m;
    QObject::connect(&m, SIGNAL(triggered(QAction*)), this, SLOT(deleteAction(QAction*)));
    QPointer<QAction> a = m.addAction("action");
    a.data()->trigger();
    QVERIFY(!a);
}

class PopulateOnAboutToShowTestMenu : public QMenu {
    Q_OBJECT
public:
    explicit PopulateOnAboutToShowTestMenu(QWidget *parent = nullptr);

public slots:
    void populateMenu();
};

PopulateOnAboutToShowTestMenu::PopulateOnAboutToShowTestMenu(QWidget *parent) : QMenu(parent)
{
    connect(this, SIGNAL(aboutToShow()), this, SLOT(populateMenu()));
}

void PopulateOnAboutToShowTestMenu::populateMenu()
{
    // just adds 3 dummy actions and a separator.
    addAction("Foo");
    addAction("Bar");
    addAction("FooBar");
    addSeparator();
}

static inline QByteArray msgGeometryIntersects(const QRect &r1, const QRect &r2)
{
    QString result;
    QDebug(&result) << r1 << "intersects" << r2;
    return result.toLocal8Bit();
}

void tst_QMenu::pushButtonPopulateOnAboutToShow()
{
#ifdef Q_OS_MACOS
    QSKIP("Popup menus may partially overlap the button on macOS, and that's okey");
#endif

    QPushButton b("Test PushButton");
    b.setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);

    QMenu *buttonMenu= new PopulateOnAboutToShowTestMenu(&b);
    b.setMenu(buttonMenu);
    const QScreen *scr = QGuiApplication::screenAt(b.pos());
    b.show();
    const QRect screen = scr->geometry();

    QRect desiredGeometry = b.geometry();
    desiredGeometry.moveTopLeft(QPoint(screen.x() + 10, screen.bottom() - b.height() - 5));

    b.setGeometry(desiredGeometry);
    QVERIFY(QTest::qWaitForWindowExposed(&b));

    if (b.geometry() != desiredGeometry) {
        // We are trying to put the button very close to the edge of the screen,
        // explicitly to test behavior when the popup menu goes off the screen.
        // However a modern window manager is quite likely to reject this requested geometry
        // (kwin in kde4 does, for example, since the button would probably appear behind
        // or partially behind the taskbar).
        // Your best bet is to run this test _without_ a WM.
        QSKIP("Your window manager won't allow a window against the bottom of the screen");
    }

    QTest::mouseClick(&b, Qt::LeftButton, Qt::NoModifier, b.rect().center());
    QVERIFY(QTest::qWaitForWindowExposed(buttonMenu));
    QTest::qWait(300);
    buttonMenu->hide();
    QVERIFY2(!buttonMenu->geometry().intersects(b.geometry()), msgGeometryIntersects(buttonMenu->geometry(), b.geometry()));

    // note: we're assuming that, if we previously got the desired geometry, we'll get it here too
    b.move(10, screen.bottom()-buttonMenu->height()-5);
    QTest::mouseClick(&b, Qt::LeftButton, Qt::NoModifier, b.rect().center());
    QVERIFY(QTest::qWaitForWindowExposed(buttonMenu));
    QTest::qWait(300);
    buttonMenu->hide();
    QVERIFY2(!buttonMenu->geometry().intersects(b.geometry()), msgGeometryIntersects(buttonMenu->geometry(), b.geometry()));
}

void tst_QMenu::QTBUG7907_submenus_autoselect()
{
    QMenu menu("Test Menu");
    QMenu set1("Setting1");
    QMenu set2("Setting2");
    QMenu subset("Subsetting");
    subset.addAction("Values");
    set1.addMenu(&subset);
    menu.addMenu(&set1);
    menu.addMenu(&set2);
    centerOnScreen(&menu, QSize(120, 100));
    menu.show();
    QVERIFY(QTest::qWaitForWindowExposed(&menu));
    QTest::mouseClick(&menu, Qt::LeftButton, Qt::NoModifier, QPoint(5,5) );
    QVERIFY(!subset.isVisible());
}

void tst_QMenu::QTBUG7411_submenus_activate()
{
    QMenu menu("Test Menu");
    QAction *act = menu.addAction("foo");
    QMenu sub1("&sub1");
    sub1.addAction("foo");
    sub1.setTitle("&sub1");
    QAction *act1 = menu.addMenu(&sub1);
    centerOnScreen(&menu, QSize(120, 100));
    menu.show();
    QVERIFY(QTest::qWaitForWindowExposed(&menu));
    menu.setActiveAction(act);
    QTest::keyPress(&menu, Qt::Key_Down);
    QCOMPARE(menu.activeAction(), act1);
    QVERIFY(!sub1.isVisible());
    QTest::keyPress(&menu, Qt::Key_S);
    QTRY_VERIFY(sub1.isVisible());
}

static bool isPlatformWayland()
{
    return !QGuiApplication::platformName().compare(QLatin1String("wayland"), Qt::CaseInsensitive);
}

void tst_QMenu::QTBUG30595_rtl_submenu()
{
    if (isPlatformWayland())
        QSKIP("Creating xdg_popups on Wayland requires real input events. Positions would be off.");

    QMenu menu("Test Menu");
    menu.setLayoutDirection(Qt::RightToLeft);
    QMenu sub("&sub");
    sub.addAction("bar");
    sub.setTitle("&sub");
    menu.addMenu(&sub);
    centerOnScreen(&menu, QSize(120, 40));
    menu.show();
    QVERIFY(QTest::qWaitForWindowExposed(&menu));
    QTest::mouseClick(&menu, Qt::LeftButton, Qt::NoModifier, QPoint(5,5) );
    QTRY_VERIFY(sub.isVisible());
    QVERIFY2(sub.pos().x() < menu.pos().x(), QByteArray::number(sub.pos().x()) + QByteArrayLiteral(" not less than ") + QByteArray::number(menu.pos().x()));
}

void tst_QMenu::QTBUG20403_nested_popup_on_shortcut_trigger()
{
    QMenu menu("Test Menu");
    QMenu sub1("&sub1");
    QMenu subsub1("&subsub1");
    subsub1.addAction("foo");
    sub1.addMenu(&subsub1);
    menu.addMenu(&sub1);
    centerOnScreen(&menu, QSize(120, 100));
    menu.show();
    QVERIFY(QTest::qWaitForWindowExposed(&menu));
    QTest::keyPress(&menu, Qt::Key_S);
    QTest::qWait(100); // 20ms delay with previous behavior
    QTRY_VERIFY(sub1.isVisible());
    QVERIFY(!subsub1.isVisible());
}

#ifndef Q_OS_MACOS
void tst_QMenu::click_while_dismissing_submenu()
{
    if (isPlatformWayland())
        QSKIP("Wayland: Creating (grabbing) popups requires real mouse events.");

    QMenu menu("Test Menu");
    QAction *action = menu.addAction("action");
    QMenu sub("&sub");
    sub.addAction("subaction");
    menu.addMenu(&sub);
    centerOnScreen(&menu, QSize(120, 100));
    menu.show();
    QSignalSpy spy(action, &QAction::triggered);
    QSignalSpy menuShownSpy(&sub, &QMenu::aboutToShow);
    QSignalSpy menuHiddenSpy(&sub, &QMenu::aboutToHide);
    QVERIFY(QTest::qWaitForWindowExposed(&menu));
    QWindow *menuWindow = menu.windowHandle();
    QVERIFY(menuWindow);
    //go over the submenu, press, move and release over the top level action
    //this opens the submenu, move two times to emulate user interaction (d->motions > 0 in QMenu)
    QTest::mouseMove(menuWindow, menu.rect().center() + QPoint(0,2));
    QTest::mouseMove(menuWindow, menu.rect().center() + QPoint(1,3), 60);
    QVERIFY(menuShownSpy.wait());
    QVERIFY(sub.isVisible());
    QVERIFY(QTest::qWaitForWindowExposed(&sub));
    //press over the submenu entry
    QTest::mousePress(menuWindow, Qt::LeftButton, {}, menu.rect().center() + QPoint(0, 2), 300);
    //move over the main action
    QTest::mouseMove(menuWindow, menu.rect().center() - QPoint(0,2));
    QVERIFY(menuHiddenSpy.wait());
    //the submenu must have been hidden for the bug to be triggered
    QVERIFY(!sub.isVisible());
    QTest::mouseRelease(menuWindow, Qt::LeftButton, {}, menu.rect().center() - QPoint(0, 2), 300);
    QCOMPARE(spy.size(), 1);
}
#endif

class MyWidget : public QWidget
{
public:
    MyWidget(QWidget *parent) :
        QWidget(parent),
        move(0), enter(0), leave(0)
    {
        setMinimumSize(100, 100);
        setMouseTracking(true);
    }

    bool event(QEvent *e) override
    {
        switch (e->type()) {
        case QEvent::MouseMove:
            ++move;
            break;
        case QEvent::Enter:
            ++enter;
            break;
        case QEvent::Leave:
            ++leave;
            break;
        default:
            break;
        }
        return QWidget::event(e);
    }

    int move, enter, leave;
};

void tst_QMenu::QTBUG47515_widgetActionEnterLeave()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");
    if (QGuiApplication::platformName() == QLatin1String("cocoa"))
        QSKIP("This test is meaningless on macOS, for additional info see QTBUG-63031");

    const QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    QRect geometry(QPoint(), availableGeometry.size() / 3);
    geometry.moveCenter(availableGeometry.center());
    QPoint pointOutsideMenu = geometry.bottomRight() - QPoint(5, 5);

    QMainWindow topLevel;
    topLevel.setGeometry(geometry);

    QMenuBar *menuBar = topLevel.menuBar();
    menuBar->setNativeMenuBar(false);
    QMenu *menu = menuBar->addMenu("Menu1");
    QMenu *submenu = menu->addMenu("Menu2");

    QWidgetAction *menuAction = new QWidgetAction(menu);
    MyWidget *w1 = new MyWidget(menu);
    menuAction->setDefaultWidget(w1);

    QWidgetAction *submenuAction = new QWidgetAction(submenu);
    MyWidget *w2 = new MyWidget(submenu);
    submenuAction->setDefaultWidget(w2);

    QAction *nextMenuAct = menu->addMenu(submenu);

    menu->addAction(menuAction);
    submenu->addAction(submenuAction);

    topLevel.show();
    topLevel.setWindowTitle(QTest::currentTestFunction());
    QVERIFY(QTest::qWaitForWindowActive(&topLevel));
    QWindow *topLevelWindow = topLevel.windowHandle();
    QVERIFY(topLevelWindow);

    // Root menu: Click on menu bar to open menu1
    {
        const QPoint menuActionPos = menuBar->mapTo(&topLevel, menuBar->actionGeometry(menu->menuAction()).center());
        QTest::mouseClick(topLevelWindow, Qt::LeftButton, Qt::KeyboardModifiers(), menuActionPos);
        QVERIFY(QTest::qWaitForWindowExposed(menu));

        w1->enter = 0;
        w1->leave = 0;
        QPoint w1Center = topLevel.mapFromGlobal(w1->mapToGlobal(w1->rect().center()));
        QTest::mouseMove(topLevelWindow, w1Center);
        QVERIFY(w1->isVisible());
        QTRY_COMPARE(w1->leave, 0);
        QTRY_COMPARE(w1->enter, 1);

        // Check whether leave event is not delivered on mouse move
        w1->move = 0;
        QTest::mouseMove(topLevelWindow, w1Center + QPoint(1, 1));
        QTRY_COMPARE(w1->move, 1);
        QTRY_COMPARE(w1->leave, 0);
        QTRY_COMPARE(w1->enter, 1);

        QTest::mouseMove(topLevelWindow, topLevel.mapFromGlobal(pointOutsideMenu));
        QTRY_COMPARE(w1->leave, 1);
        QTRY_COMPARE(w1->enter, 1);
    }

    // Submenu
    {
        menu->setActiveAction(nextMenuAct);
        QVERIFY(QTest::qWaitForWindowExposed(submenu));

        QPoint w2Center = topLevel.mapFromGlobal(w2->mapToGlobal(w2->rect().center()));
        QTest::mouseMove(topLevelWindow, w2Center);

        QVERIFY(w2->isVisible());
        QTRY_COMPARE(w2->leave, 0);
        QTRY_COMPARE(w2->enter, 1);

        // Check whether leave event is not delivered on mouse move
        w2->move = 0;
        QTest::mouseMove(topLevelWindow, w2Center + QPoint(1, 1));
        QTRY_COMPARE(w2->move, 1);
        QTRY_COMPARE(w2->leave, 0);
        QTRY_COMPARE(w2->enter, 1);

        QTest::mouseMove(topLevelWindow, topLevel.mapFromGlobal(pointOutsideMenu));
        QTRY_COMPARE(w2->leave, 1);
        QTRY_COMPARE(w2->enter, 1);
    }
}

void tst_QMenu::QTBUG_89082_actionTipsHide()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QWidget widget;
    QMenu *menu = new QMenu(&widget);
    menu->addAction("aaa");
    menu->addAction("bbb");
    menu->addAction("ccc");
    menu->addAction("ddd");
    menu->addAction("eee");
    menu->addAction("fff");
    menu->setToolTipsVisible(true);

    auto menuActTip = menu->actions().first();
    QString tipFullName = "actionTip-this-is-a-long-action-and-show-the-full-name-by-tip";
    QFontMetrics m_fm = QFontMetrics(QAction().font());
    const QString &&elidedName = m_fm.elidedText(tipFullName, Qt::ElideRight, 50);
    menuActTip->setText(elidedName);
    if (elidedName != tipFullName)
        menuActTip->setToolTip(tipFullName);

    widget.resize(300, 200);
    centerOnScreen(&widget);
    widget.show();
#if QT_CONFIG(cursor)
    QCursor::setPos(widget.screen()->availableGeometry().topLeft() + QPoint(10, 10));
#endif
    widget.activateWindow();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    menu->popup(widget.geometry().topRight() + QPoint(50, 0));
    QVERIFY(QTest::qWaitForWindowExposed(menu));
    auto menuWindow = menu->windowHandle();
    QVERIFY(menuWindow != nullptr);

    auto actionZero = menu->actions().at(0);
    auto actionOne = menu->actions().at(1);
    auto actionFive = menu->actions().at(5);
    const QRect submenuRect0 = menu->actionGeometry(actionZero);
    const QPoint submenuPos0(submenuRect0.topLeft() + QPoint(3, 3));

    const QRect submenuRect1 = menu->actionGeometry(actionOne);
    const QPoint submenuPos1(submenuRect1.topLeft() + QPoint(3, 3));

    const QRect submenuRect5 = menu->actionGeometry(actionFive);
    const QPoint submenuPos5(submenuRect5.topLeft() + QPoint(10, 3));

    QTest::mouseMove(menuWindow, submenuPos1);
    QTest::mouseMove(menuWindow, submenuPos0); //show the tip
    QTRY_COMPARE_WITH_TIMEOUT(QToolTip::text(), tipFullName, 1000);

    //Move to the fifth action without prompting
    QTest::mouseMove(menuWindow, submenuPos5);
    //The previous tip was hidden, but now is a new tip to get text, So there should be no content
    QTRY_COMPARE_WITH_TIMEOUT(QToolTip::text(), QString(), 1000);
}

void tst_QMenu::QTBUG8122_widgetActionCrashOnClose()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");
    if (QGuiApplication::platformName() == QLatin1String("cocoa"))
        QSKIP("See QTBUG-63031");

    const QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    QRect geometry(QPoint(), availableGeometry.size() / 3);
    geometry.moveCenter(availableGeometry.center());
    QPoint pointOutsideMenu = geometry.bottomRight() - QPoint(5, 5);

    QMainWindow topLevel;
    topLevel.setGeometry(geometry);

    auto menuBar = topLevel.menuBar();
    auto menu = menuBar->addMenu("Menu");
    auto wAct = new QWidgetAction(menu);
    auto spinBox1 = new QSpinBox(menu);
    wAct->setDefaultWidget(spinBox1);
    menu->addAction(wAct);
    auto subMenu = menu->addMenu("Submenu");
    auto nextMenuAct = menu->addMenu(subMenu);
    auto wAct2 = new QWidgetAction(menu);
    auto spinBox2 = new QSpinBox(menu);
    wAct2->setDefaultWidget(spinBox2);
    subMenu->addAction(wAct2);
    QObject::connect(spinBox2, &QSpinBox::editingFinished, menu, &QMenu::hide);

    topLevel.show();
    topLevel.setWindowTitle(QTest::currentTestFunction());
    QVERIFY(QTest::qWaitForWindowActive(&topLevel));
    QWindow *topLevelWindow = topLevel.windowHandle();
    QVERIFY(topLevelWindow);

    const QPoint menuActionPos = menuBar->mapTo(&topLevel, menuBar->actionGeometry(menu->menuAction()).center());
    QTest::mouseClick(topLevelWindow, Qt::LeftButton, Qt::KeyboardModifiers(), menuActionPos);
    QVERIFY(QTest::qWaitForWindowExposed(menu));

    QPoint w1Center = topLevel.mapFromGlobal(spinBox1->mapToGlobal(spinBox1->rect().center()));
    QTest::mouseClick(topLevelWindow, Qt::LeftButton, Qt::KeyboardModifiers(), w1Center);
    menu->setActiveAction(nextMenuAct);
    QVERIFY(QTest::qWaitForWindowExposed(subMenu));

    QPoint w2Center = topLevel.mapFromGlobal(spinBox2->mapToGlobal(spinBox2->rect().center()));
    QTest::mouseClick(topLevelWindow, Qt::LeftButton, Qt::KeyboardModifiers(), w2Center);
    QTest::mouseMove(topLevelWindow, topLevel.mapFromGlobal(pointOutsideMenu));
    QTRY_VERIFY(menu->isHidden());
}

/*!
    Test that a QWidgetAction that fires closes the menus that it is in.
*/
void tst_QMenu::widgetActionTriggerClosesMenu()
{
    class ButtonAction : public QWidgetAction
    {
    public:
        ButtonAction()
        : QWidgetAction(nullptr)
        {}

        void click()
        {
            if (pushButton)
                pushButton->click();
        }

    protected:
        QWidget *createWidget(QWidget *parent) override
        {
            QPushButton *button = new QPushButton(QLatin1String("Button"), parent);
            connect(button, &QPushButton::clicked, this, &QAction::trigger);

            if (!pushButton)
                pushButton = button;
            return button;
        }

    private:
        QPointer<QPushButton> pushButton;
    };

    QMenu menu;
    QMenu submenu;

    int menuTriggeredCount = 0;
    int menuAboutToHideCount = 0;
    QAction *actionTriggered = nullptr;

    connect(&menu, &QMenu::triggered, this, [&](QAction *action){
        ++menuTriggeredCount;
        actionTriggered = action;
    });
    connect (&menu, &QMenu::aboutToHide, this, [&](){
        ++menuAboutToHideCount;
    });

    QAction regularAction(QLatin1String("Action"));
    ButtonAction widgetAction;

    submenu.addAction(&regularAction);
    submenu.addAction(&widgetAction);

    menu.addMenu(&submenu);
    menu.addAction(&regularAction);
    menu.addAction(&widgetAction);

    menu.popup(QPoint(200,200));
    submenu.popup(QPoint(250,250));
    if (!QTest::qWaitForWindowExposed(&menu) || !QTest::qWaitForWindowExposed(&submenu))
        QSKIP("Failed to show menus, aborting test");

    regularAction.trigger();
    QVERIFY(menu.isVisible());
    QVERIFY(submenu.isVisible());
    QCOMPARE(menuTriggeredCount, 1);
    QCOMPARE(actionTriggered, &regularAction);
    menuTriggeredCount = 0;
    actionTriggered = nullptr;

    widgetAction.click();
    QVERIFY(!menu.isVisible());
    QVERIFY(!submenu.isVisible());
    QCOMPARE(menuTriggeredCount, 1);
    QCOMPARE(menuAboutToHideCount, 1);
    QCOMPARE(actionTriggered, &widgetAction);
}

void tst_QMenu::transientParent()
{
    QMainWindow window;
    window.resize(480, 320);
    window.menuBar()->setNativeMenuBar(false);
    centerOnScreen(&window);

    QMenu *fileMenu = new QMenu("&File");
    QAction *exitAct = new QAction("Exit");
    fileMenu->addAction(exitAct);

    QMenu *editMenu = new QMenu("&Edit");
    QAction *undoAct = new QAction("Undo");
    editMenu->addAction(undoAct);

    QMenuBar *menuBar = new QMenuBar;
    menuBar->addMenu(fileMenu);
    menuBar->addMenu(editMenu);
    window.setMenuBar(menuBar);

    // On Mac, we need to create native key events to test menu
    // action activation, so skip this part of the test.
#if QT_CONFIG(shortcut) && !defined(Q_OS_DARWIN)
    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QWindow *topLevel = window.windowHandle();
    QVERIFY(topLevel);

    QApplicationPrivate::setActiveWindow(&window);
    window.setFocus();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QVERIFY(window.hasFocus());

    QTest::keyPress(&window, Qt::Key_F, Qt::AltModifier);
    QTRY_VERIFY(QTest::qWaitForWindowExposed(fileMenu));
    if (fileMenu->isWindow() && fileMenu->window() && fileMenu->window()->windowHandle())
        QVERIFY(fileMenu->window()->windowHandle()->transientParent());
    QTest::keyRelease(fileMenu, Qt::Key_F, Qt::AltModifier);

    QTest::keyPress(fileMenu, Qt::Key_E, Qt::AltModifier);
    QTRY_VERIFY(QTest::qWaitForWindowExposed(editMenu));
    if (editMenu->isWindow() && editMenu->window() && editMenu->window()->windowHandle())
        QVERIFY(editMenu->window()->windowHandle()->transientParent());
    QTest::keyRelease(editMenu, Qt::Key_E, Qt::AltModifier);
#endif // QT_CONFIG(shortcut) && !Q_OS_DARWIN

}

class MyMenu : public QMenu
{
    Q_OBJECT
public:
    MyMenu() : m_currentIndex(0)
    {
        for (int i = 0; i < 2; ++i)
            dialogActions[i] = addAction(QLatin1String("dialog ") + QString::number(i), dialogs + i, SLOT(exec()));
    }

    void activateAction(int index)
    {
        m_currentIndex = index;
        popup(QPoint());
        QVERIFY(QTest::qWaitForWindowExposed(this));
        setActiveAction(dialogActions[index]);
        QTimer::singleShot(500, this, SLOT(checkVisibility()));
        QTest::keyClick(this, Qt::Key_Enter); //activation
    }

public slots:
    void activateLastAction()
    {
        activateAction(1);
    }

    void checkVisibility()
    {
        QTRY_VERIFY(dialogs[m_currentIndex].isVisible());
        if (m_currentIndex == 1) {
            QApplication::closeAllWindows(); //this is the end of the test
        }
    }

private:
    QAction *dialogActions[2];
    QDialog dialogs[2];
    int m_currentIndex;
};

void tst_QMenu::QTBUG_10735_crashWithDialog()
{
    MyMenu menu;

    QTimer::singleShot(1000, &menu, SLOT(activateLastAction()));
    menu.activateAction(0);
}

#ifdef Q_OS_MAC
void tst_QMenu::QTBUG_37933_ampersands_data()
{
    QTest::addColumn<QString>("title");
    QTest::addColumn<QString>("visibleTitle");
    QTest::newRow("simple") << QString("Test") << QString("Test");
    QTest::newRow("ampersand") << QString("&Test") << QString("Test");
    QTest::newRow("double_ampersand") << QString("&Test && more") << QString("Test & more");
    QTest::newRow("ampersand_in_parentheses") << QString("Test(&T) (&&) more") << QString("Test (&) more");
    QTest::newRow("ampersand_in_parentheses_after_space") << QString("Test (&T)") << QString("Test");
    QTest::newRow("ampersand_in_parentheses_after_spaces") << QString("Test  (&T)") << QString("Test");
    QTest::newRow("ampersand_in_parentheses_before_space") << QString("Test(&T) ") << QString("Test ");
    QTest::newRow("only_ampersand_in_parentheses") << QString("(&T)") << QString("");
    QTest::newRow("only_ampersand_in_parentheses_after_space") << QString(" (&T)") << QString("");
    QTest::newRow("parentheses_after_space") << QString(" (Dummy)") << QString(" (Dummy)");
    QTest::newRow("ampersand_after_space") << QString("About &Qt Project") << QString("About Qt Project");
}

void tst_qmenu_QTBUG_37933_ampersands();

void tst_QMenu::QTBUG_37933_ampersands()
{
    // external in .mm file
    tst_qmenu_QTBUG_37933_ampersands();
}
#endif

void tst_QMenu::QTBUG_56917_wideMenuSize()
{
    // menu shouldn't to take on full screen height when menu width is larger than screen width
    QMenu menu;
    QString longString;
    longString.fill(QLatin1Char('Q'), 3000);
    menu.addAction(longString);
    QSize menuSizeHint = menu.sizeHint();
    menu.popup(QPoint());
    QVERIFY(QTest::qWaitForWindowExposed(&menu));
    QVERIFY(menu.isVisible());
    QVERIFY(menu.height() <= menuSizeHint.height());
}

void tst_QMenu::QTBUG_56917_wideMenuScreenNumber()
{
    if (QApplication::styleHints()->showIsFullScreen())
        QSKIP("The platform defaults to windows being fullscreen.");
    // menu must appear on the same screen where show action is triggered
    QString longString;
    longString.fill(QLatin1Char('Q'), 3000);

    const QList<QScreen *> screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        QMenu menu;
        menu.addAction(longString);
        menu.popup(screen->geometry().center());
        QVERIFY(QTest::qWaitForWindowExposed(&menu));
        QVERIFY(menu.isVisible());
        QCOMPARE(QGuiApplication::screenAt(menu.pos()), screen);
    }
}

void tst_QMenu::QTBUG_56917_wideSubmenuScreenNumber()
{
    if (QApplication::styleHints()->showIsFullScreen())
        QSKIP("The platform defaults to windows being fullscreen.");
    // submenu must appear on the same screen where its parent menu is shown
    QString longString;
    longString.fill(QLatin1Char('Q'), 3000);

    const QList<QScreen *> screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        QMenu menu;
        QMenu submenu("Submenu");
        submenu.addAction(longString);
        QAction *action = menu.addMenu(&submenu);
        menu.popup(screen->geometry().center());
        QVERIFY(QTest::qWaitForWindowExposed(&menu));
        QVERIFY(menu.isVisible());
        QTest::mouseClick(&menu, Qt::LeftButton, {}, menu.actionGeometry(action).center());
        QTest::qWait(100);
        QVERIFY(QTest::qWaitForWindowExposed(&submenu));
        QVERIFY(submenu.isVisible());
        QCOMPARE(QGuiApplication::screenAt(submenu.pos()), screen);
    }
}

void tst_QMenu::menuSize_Scrolling_data()
{
    QTest::addColumn<int>("numItems");
    QTest::addColumn<int>("topMargin");
    QTest::addColumn<int>("bottomMargin");
    QTest::addColumn<int>("leftMargin");
    QTest::addColumn<int>("rightMargin");
    QTest::addColumn<int>("topPadding");
    QTest::addColumn<int>("bottomPadding");
    QTest::addColumn<int>("leftPadding");
    QTest::addColumn<int>("rightPadding");
    QTest::addColumn<int>("border");
    QTest::addColumn<bool>("scrollable");
    QTest::addColumn<bool>("tearOff");

    // test data
    // a single column and non-scrollable menu with contents margins + border
    QTest::newRow("data0") << 5 << 2 << 2 << 2 << 2 << 0 << 0 << 0 << 0 << 2 << false << false;
    // a single column and non-scrollable menu with paddings + border
    QTest::newRow("data1") << 5 << 0 << 0 << 0 << 0 << 2 << 2 << 2 << 2 << 2 << false << false;
    // a single column and non-scrollable menu with contents margins + paddings + border
    QTest::newRow("data2") << 5 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << false << false;
    // a single column and non-scrollable menu with contents margins + paddings + border + tear-off
    QTest::newRow("data3") << 5 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << false << true;
    // a multi-column menu with contents margins + border
    QTest::newRow("data4") << 80 << 2 << 2 << 2 << 2 << 0 << 0 << 0 << 0 << 2 << false << false;
    // a multi-column menu with paddings + border
    QTest::newRow("data5") << 80 << 0  << 0 << 0 << 0 << 2 << 2 << 2 << 2 << 2 << false << false;
    // a multi-column menu with contents margins + paddings + border
    QTest::newRow("data6") << 80 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << false << false;
    // a multi-column menu with contents margins + paddings + border + tear-off
    QTest::newRow("data7") << 80 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << false << true;
    // a scrollable menu with contents margins + border
    QTest::newRow("data8") << 80 << 2 << 2 << 2 << 2 << 0 << 0 << 0 << 0 << 2 << true << false;
    // a scrollable menu with paddings + border
    QTest::newRow("data9") << 80 << 0 << 0 << 0 << 0 << 2 << 2 << 2 << 2 << 2 << true << false;
    // a scrollable menu with contents margins + paddings + border
    QTest::newRow("data10") << 80 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << true << false;
    // a scrollable menu with contents margins + paddings + border + tear-off
    QTest::newRow("data11") << 80 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << true << true;
}

void tst_QMenu::menuSize_Scrolling()
{
    class TestMenu : public QMenu
    {
    public:
        struct ContentsMargins
        {
            ContentsMargins(int l, int t, int r, int b)
                : left(l), top(t), right(r), bottom(b) {}
            int left;
            int top;
            int right;
            int bottom;
        };

        struct MenuPaddings
        {
            MenuPaddings(int l, int t, int r, int b)
                : left(l), top(t), right(r), bottom(b) {}
            int left;
            int top;
            int right;
            int bottom;
        };

        TestMenu(int numItems, const ContentsMargins &margins, const MenuPaddings &paddings,
                 int border, bool scrollable, bool tearOff)
            : QMenu("Test Menu"),
              m_numItems(numItems),
              m_scrollable(scrollable),
              m_tearOff(tearOff)
        {
            init(margins, paddings, border);
        }

        ~TestMenu() {}

    private:
        void showEvent(QShowEvent *e) override
        {
            QVERIFY(actions().size() == m_numItems);

            int hmargin = style()->pixelMetric(QStyle::PM_MenuHMargin, nullptr, this);
            int fw = style()->pixelMetric(QStyle::PM_MenuPanelWidth, nullptr, this);
            const QMargins cm = contentsMargins();
            QRect lastItem = actionGeometry(actions().at(actions().size() - 1));
            QSize s = size();
            if (!QGuiApplication::platformName().compare(QLatin1String("minimal"), Qt::CaseInsensitive)
                || !QGuiApplication::platformName().compare(QLatin1String("offscreen"), Qt::CaseInsensitive)) {
                qWarning("Skipping test on minimal/offscreen platforms - QTBUG-73522");
                QMenu::showEvent(e);
                return;
            }

            QCOMPARE( s.width(), lastItem.right() + fw + hmargin + cm.right() + 1);
            QMenu::showEvent(e);
        }

        void init(const ContentsMargins &margins, const MenuPaddings &paddings, int border)
        {
            setLayoutDirection(Qt::LeftToRight);

            setTearOffEnabled(m_tearOff);
            setContentsMargins(margins.left, margins.top, margins.right, margins.bottom);
            QString cssStyle("QMenu {menu-scrollable: ");
            cssStyle += (m_scrollable ? QString::number(1) : QString::number(0));
            cssStyle += "; border: ";
            cssStyle += QString::number(border);
            cssStyle += "px solid black; padding: ";
            cssStyle += QString::number(paddings.top);
            cssStyle += "px ";
            cssStyle += QString::number(paddings.right);
            cssStyle += "px ";
            cssStyle += QString::number(paddings.bottom);
            cssStyle += "px ";
            cssStyle += QString::number(paddings.left);
            cssStyle += "px;}";
            setStyleSheet(cssStyle);
            for (int i = 1; i <= m_numItems; i++)
                addAction("MenuItem " + QString::number(i));
        }

    private:
        int m_numItems;
        bool m_scrollable;
        bool m_tearOff;
    };

    QFETCH(int, numItems);
    QFETCH(int, topMargin);
    QFETCH(int, bottomMargin);
    QFETCH(int, leftMargin);
    QFETCH(int, rightMargin);
    QFETCH(int, topPadding);
    QFETCH(int, bottomPadding);
    QFETCH(int, leftPadding);
    QFETCH(int, rightPadding);
    QFETCH(int, border);
    QFETCH(bool, scrollable);
    QFETCH(bool, tearOff);

    qApp->setAttribute(Qt::AA_DontUseNativeMenuBar);

    TestMenu::ContentsMargins margins(leftMargin, topMargin, rightMargin, bottomMargin);
    TestMenu::MenuPaddings paddings(leftPadding, topPadding, rightPadding, bottomPadding);
    TestMenu menu(numItems, margins, paddings, border, scrollable, tearOff);
    menu.popup(QPoint(0,0));
    centerOnScreen(&menu);
    QVERIFY(QTest::qWaitForWindowExposed(&menu));

    QList<QAction *> actions = menu.actions();
    QCOMPARE(actions.size(), numItems);

    MenuMetrics mm(&menu);
    QTest::keyClick(&menu, Qt::Key_Home);
    QTRY_COMPARE(menu.actionGeometry(actions.first()).y(), mm.fw + mm.vmargin + topMargin + (tearOff ? mm.tearOffHeight : 0));
    QCOMPARE(menu.actionGeometry(actions.first()).x(), mm.fw + mm.hmargin + leftMargin);

    if (!scrollable)
        return;

    QTest::keyClick(&menu, Qt::Key_End);
    QTRY_COMPARE(menu.actionGeometry(actions.last()).right(),
                 menu.width() - mm.fw - mm.hmargin - leftMargin - 1);
    QCOMPARE(menu.actionGeometry(actions.last()).bottom(),
             menu.height() - mm.fw - mm.vmargin - bottomMargin - 1);
}

void tst_QMenu::tearOffMenuNotDisplayed()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");
    QWidget widget;
    QScopedPointer<QMenu> menu(new QMenu(&widget));
    menu->setTearOffEnabled(true);
    QVERIFY(menu->isTearOffEnabled());

    menu->setStyleSheet("QMenu { menu-scrollable: 1 }");
    for (int i = 0; i < 80; i++)
        menu->addAction(QString::number(i));

    widget.resize(300, 200);
    centerOnScreen(&widget);
    widget.show();
    widget.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&widget));
    menu->popup(widget.geometry().topRight() + QPoint(50, 0));
    QVERIFY(QTest::qWaitForWindowActive(menu.data()));
    QVERIFY(!menu->isTearOffMenuVisible());

    MenuMetrics mm(menu.data());
    const int tearOffOffset = mm.fw + mm.vmargin + mm.tearOffHeight / 2;

    QTest::mouseClick(menu.data(), Qt::LeftButton, {}, QPoint(10, tearOffOffset), 10);
    QTRY_VERIFY(menu->isTearOffMenuVisible());
    QPointer<QMenu> torn = getTornOffMenu();
    QVERIFY(torn);
    QVERIFY(torn->isVisible());
    QVERIFY(torn->minimumWidth() >=0 && torn->minimumWidth() < QWIDGETSIZE_MAX);

    menu->hideTearOffMenu();
    QVERIFY(!menu->isTearOffMenuVisible());
    QVERIFY(!torn->isVisible());
}

void tst_QMenu::QTBUG_61039_menu_shortcuts()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QAction *actionKamen = new QAction("Action Kamen");
#if QT_CONFIG(shortcut)
    actionKamen->setShortcut(QKeySequence(QLatin1String("K")));
#endif
    QAction *actionJoe = new QAction("Action Joe");
#if QT_CONFIG(shortcut)
    actionJoe->setShortcut(QKeySequence(QLatin1String("Ctrl+J")));
#endif

    QMenu menu;
    menu.addAction(actionKamen);
    menu.addAction(actionJoe);
    QVERIFY(!menu.platformMenu());

    QWidget widget;
    widget.addAction(menu.menuAction());
    widget.show();
    QVERIFY(QTest::qWaitForWindowActive(&widget));

    QSignalSpy actionKamenSpy(actionKamen, &QAction::triggered);
    QTest::keyClick(&widget, Qt::Key_K);
    QTRY_COMPARE(actionKamenSpy.size(), 1);

    QSignalSpy actionJoeSpy(actionJoe, &QAction::triggered);
    QTest::keyClick(&widget, Qt::Key_J, Qt::ControlModifier);
    QTRY_COMPARE(actionJoeSpy.size(), 1);
}

void tst_QMenu::screenOrientationChangedCloseMenu()
{
     QMenu menu;
     menu.addAction("action1");
     menu.show();

     QTRY_COMPARE(menu.isVisible(),true);

     Qt::ScreenOrientation orientation = menu.screen()->orientation() == Qt::PortraitOrientation ? Qt::LandscapeOrientation : Qt::PortraitOrientation;
     QScreenOrientationChangeEvent event(menu.screen(), orientation);
     QCoreApplication::sendEvent(QCoreApplication::instance(), &event);

     QTRY_COMPARE(menu.isVisible(),false);
}

/*
    Verify that deleting the menu in a slot connected to an
    action's triggered signal doesn't crash.
    QTBUG-106718
*/
void tst_QMenu::deleteWhenTriggered()
{
    QPointer<QMenu> menu = new QMenu;
    QAction *action = menu->addAction("Action", [&menu]{
        delete menu;
    });
    menu->popup(QGuiApplication::primaryScreen()->availableGeometry().center());
    menu->setActiveAction(action);
    QTest::keyClick(menu, Qt::Key_Return);
    QTRY_VERIFY(!menu);
}

/*
    QMenu uses the caused-stack to create the parent/child relationship
    for tear-off menus. Since QTornOffMenu set the DeleteOnClose flag, closing a
    tear-off in the parent chain will result in a null-pointer in the caused-stack.
    Verify that we don't crash when traversing the chain, as reported in QTBUG-112217.

    The test has to open the submenus by hovering of the menu action, otherwise
    the caused-stack remains empty and the issue doesn't reproduce. Due to QMenu's
    timing and "sloppiness", we need to move the mouse within the action, with some
    waiting and event processing in between to trigger the opening of the submenu.
    If this fails we skip, as we then can't test what we are trying to test.
*/
void tst_QMenu::nestedTearOffDetached()
{
    // Since QTornOffMenu is not declared in qmenuprivate.h we can't access the
    // object even through QMenuPrivate. So use an event filter to watch out for
    // a QTornOffMenu showing.
    class TearOffWatcher : public QObject
    {
    public:
        QMenu *tornOffMenu = nullptr;
    protected:
        bool eventFilter(QObject *receiver, QEvent *event) override
        {
            if (event->type() == QEvent::Show && receiver->inherits("QTornOffMenu"))
                tornOffMenu = qobject_cast<QMenu *>(receiver);
            return QObject::eventFilter(receiver, event);
        }
    } watcher;
    qApp->installEventFilter(&watcher);

    QWidget widget;
    QMenu *menu = new QMenu("Context", &widget);

    MenuMetrics mm(menu);
    const int tearOffOffset = mm.fw + mm.vmargin + mm.tearOffHeight / 2;

    QMenu *subMenu = menu->addMenu("SubMenu");
    menu->setTearOffEnabled(true);
    QMenu *subSubMenu = subMenu->addMenu("SubSubMenu");
    subMenu->setTearOffEnabled(true);
    subSubMenu->addAction("Action!");
    subSubMenu->setTearOffEnabled(true);

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    // open and tear off context menu
    menu->popup(widget.geometry().center());
    QTest::mouseClick(menu, Qt::LeftButton, {}, QPoint(menu->width() / 2, tearOffOffset));

    QMenu *menuTorn = watcher.tornOffMenu;
    watcher.tornOffMenu = nullptr;
    QVERIFY(menuTorn);
    QVERIFY(QTest::qWaitForWindowExposed(menuTorn));

    // open second menu and tear-off
    QTest::mouseMove(menuTorn, menuTorn->actionGeometry(subMenu->menuAction()).topLeft());
    QTest::qWait(100);
    QTest::mouseMove(menuTorn, menuTorn->actionGeometry(subMenu->menuAction()).center());
    if (!QTest::qWaitFor([subMenu]{ return subMenu->isVisible(); }))
        QSKIP("Menu failed to show, skipping test");

    QTest::mouseClick(subMenu, Qt::LeftButton, {}, QPoint(subMenu->width() / 2, tearOffOffset));
    menuTorn = watcher.tornOffMenu;
    QVERIFY(menuTorn);
    QVERIFY(QTest::qWaitForWindowExposed(menuTorn));
    // close the top level tear off
    menu->hideTearOffMenu();
    // open third menu and tear-off
    QTest::mouseMove(menuTorn, menuTorn->actionGeometry(subSubMenu->menuAction()).topLeft());
    QTest::qWait(100);
    QTest::mouseMove(menuTorn, menuTorn->actionGeometry(subSubMenu->menuAction()).center());
    QTRY_VERIFY(subSubMenu->isVisible());
    QTest::mouseClick(subSubMenu, Qt::LeftButton, {}, QPoint(subSubMenu->width() / 2, tearOffOffset));
}

QTEST_MAIN(tst_QMenu)
#include "tst_qmenu.moc"
