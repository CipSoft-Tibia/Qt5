// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtWidgets/QApplication>
#include <QtWidgets/QDial>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsPixmapItem>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QVBoxLayout>

#include <QtGui/QPainterPath>
#include <QtGui/QScreen>

#include <QTest>
#include <QTestEventLoop>
#include <QRandomGenerator>
#include <QSignalSpy>
#include <QMimeData>

#include <QtCore/QDebug>
#include <QtCore/QLoggingCategory>

#include <private/qgraphicsscene_p.h>
#include <private/qgraphicssceneindex_p.h>
#include <math.h>
#include "../../../gui/painting/qpathclipper/pathcompare.h"
#include "../../../shared/platforminputcontext.h"
#include <private/qinputmethod_p.h>

#include <QtWidgets/private/qapplication_p.h>

#if defined(Q_OS_WIN)
#include <QtCore/qt_windows.h>
#define Q_CHECK_PAINTEVENTS \
    if (::SwitchDesktop(::GetThreadDesktop(::GetCurrentThreadId())) == 0) \
        QSKIP("The Graphics View doesn't get the paint events");
#else
#define Q_CHECK_PAINTEVENTS
#endif

Q_DECLARE_METATYPE(Qt::FocusReason)
Q_DECLARE_METATYPE(QPainterPath)
Q_DECLARE_METATYPE(Qt::AspectRatioMode)
Q_DECLARE_METATYPE(Qt::ItemSelectionMode)
Q_DECLARE_METATYPE(QGraphicsItem::GraphicsItemFlags)

static const int randomX[] = {276, 40, 250, 864, -56, 426, 855, 825, 184, 955, -798, -804, 773,
                              282, 489, 686, 780, -220, 50, 749, -856, -205, 81, 492, -819, 518,
                              895, 57, -559, 788, -965, 68, -442, -247, -339, -648, 292, 891,
                              -865, 462, 864, 673, 640, 523, 194, 500, -727, 307, -243, 320,
                              -545, 415, 448, 341, -619, 652, 892, -16, -14, -659, -101, -934,
                              532, 356, 824, 132, 160, 130, 104, 886, -179, -174, 543, -644, 60,
                              -470, -354, -728, 689, 682, -587, -694, -221, -741, 37, 372, -289,
                              741, -300, 858, -320, 729, -602, -956, -544, -403, 203, 398, 284,
                              -972, -572, -946, 81, 51, -403, -580, 867, 546, 565, -580, -484,
                              659, 982, -518, -976, 423, -800, 659, -297, 712, 938, -19, -16,
                              824, -252, 197, 321, -837, 824, 136, 226, -980, -909, -826, -479,
                              -835, -503, -828, -901, -810, -641, -548, -179, 194, 749, -296, 539,
                              -37, -599, -235, 121, 35, -230, -915, 789, 764, -622, -382, -90, -701,
                              676, -407, 998, 267, 913, 817, -748, -370, -162, -797, 19, -556, 933,
                              -670, -101, -765, -941, -17, 360, 31, 960, 509, 933, -35, 974, -924,
                              -734, 589, 963, 724, 794, 843, 16, -272, -811, 721, 99, -122, 216,
                              -404, 158, 787, -443, -437, -337, 383, -342, 538, -641, 791, 637,
                              -848, 397, 820, 109, 11, 45, 809, 591, 933, 961, 625, -140, -592,
                              -694, -969, 317, 293, 777, -18, -282, 835, -455, -708, -407, -204,
                              748, 347, -501, -545, 292, -362, 176, 546, -573, -38, -854, -395,
                              560, -624, -940, -971, 66, -910, 782, 985};

static const int randomY[] = {603, 70, -318, 843, 450, -637, 199, -527, 407, 964, -54, 620, -207,
                              -736, -700, -476, -706, -142, 837, 621, 522, -98, 232, 292, -267, 900,
                              615, -356, -415, 783, 290, 462, -857, -314, 677, 36, 772, 424, -72,
                              -121, 547, -533, 537, -656, 289, 508, 914, 601, 434, 588, -779, -714,
                              -368, 628, -276, 432, -1, -929, 638, -36, 253, -922, -943, 979, -34,
                              -268, -193, 601, 686, -330, 165, 98, 75, -691, -605, 617, 773, 617,
                              619, 238, -42, -405, 17, 384, -472, -846, 520, 110, 591, -217, 936,
                              -373, 731, 734, 810, 961, 881, 939, 379, -905, -137, 437, 298, 688,
                              -71, -204, 573, -120, -821, 489, -722, -926, 529, -113, -243, 543,
                              868, -301, -781, -549, -842, -489, -80, -910, -928, 51, -91, 324,
                              204, -92, 867, 723, 248, 709, -357, 591, -365, -379, 266, -649, -95,
                              205, 551, 355, -631, 79, -186, 795, -7, -225, 46, -410, 665, -874,
                              -618, 845, -548, 443, 471, -644, 606, -607, 59, -619, 288, -244, 529,
                              690, 349, -738, -611, -879, -642, 801, -178, 823, -748, -552, -247,
                              -223, -408, 651, -62, 949, -795, 171, -107, -210, -207, -842, -86,
                              436, 528, 366, -178, 245, -695, 665, 613, -948, 667, -620, -979, -949,
                              905, 181, -412, -467, -437, -774, 750, -10, 54, 205, -674, -290, -924,
                              -361, -463, 912, -702, 622, -542, 220, 115, 832, 451, -38, -952, -230,
                              -588, 864, 234, 225, -303, 493, 246, 153, 338, -378, 377, -819, 140, 136,
                              467, -849, -326, -533, 166, 252, -994, -699, 904, -566, 621, -752};

Q_LOGGING_CATEGORY(lcTests, "qt.widgets.tests")

class HoverItem : public QGraphicsRectItem
{
public:
    HoverItem()
        : QGraphicsRectItem(QRectF(-10, -10, 20, 20))
    { setAcceptHoverEvents(true); }

    bool isHovered = false;

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *) override
    {
        isHovered = (option->state & QStyle::State_MouseOver);

        painter->setOpacity(0.75);
        painter->setPen(Qt::NoPen);
        painter->setBrush(Qt::darkGray);
        painter->drawRoundedRect(boundingRect().adjusted(3, 3, -3, -3), 25, 25, Qt::RelativeSize);
        painter->setPen(Qt::black);
        if (isHovered) {
            painter->setBrush(QColor(Qt::blue).lighter(120));
        } else {
            painter->setBrush(Qt::gray);
        }
        painter->drawRoundedRect(boundingRect().adjusted(0, 0, -5, -5), 25, 25, Qt::RelativeSize);
    }
};

class EventSpy : public QGraphicsWidget
{
    Q_OBJECT
public:
    EventSpy(QObject *watched, QEvent::Type type)
        : spied(type)
    {
        watched->installEventFilter(this);
    }

    EventSpy(QGraphicsScene *scene, QGraphicsItem *watched, QEvent::Type type)
        : spied(type)
    {
        scene->addItem(this);
        watched->installSceneEventFilter(this);
    }

    int count() const { return _count; }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        Q_UNUSED(watched);
        if (event->type() == spied)
            ++_count;
        return false;
    }

    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override
    {
        Q_UNUSED(watched);
        if (event->type() == spied)
            ++_count;
        return false;
    }

    int _count = 0;
    const QEvent::Type spied;
};

class tst_QGraphicsScene : public QObject
{
    Q_OBJECT
public:
    tst_QGraphicsScene();

public slots:
    void cleanup();

private slots:
    void construction();
    void sceneRect();
    void itemIndexMethod();
    void bspTreeDepth();
    void itemsBoundingRect_data();
    void itemsBoundingRect();
    void items();
    void items_QPointF_data();
    void items_QPointF();
    void items_QRectF();
    void items_QRectF_2_data();
    void items_QRectF_2();
    void items_QPolygonF();
    void items_QPolygonF_2();
    void items_QPainterPath();
    void items_QPainterPath_2();
    void selectionChanged();
    void selectionChanged2();
    void addItem();
    void addEllipse();
    void addLine();
    void addPath();
    void addPixmap();
    void addRect();
    void addText();
    void removeItem();
    void clear();
    void focusItem();
    void focusItemLostFocus();
    void setFocusItem();
    void setFocusItem_inactive();
    void mouseGrabberItem();
    void hoverEvents_siblings();
    void hoverEvents_parentChild();
    void createItemGroup();
    void mouseEventPropagation();
    void mouseEventPropagation_ignore();
    void mouseEventPropagation_focus();
    void mouseEventPropagation_doubleclick();
    void mouseEventPropagation_mouseMove();
#if QT_CONFIG(draganddrop)
    void dragAndDrop_simple();
    void dragAndDrop_disabledOrInvisible();
    void dragAndDrop_propagate();
#endif
    void render_data();
    void render();
    void renderItemsWithNegativeWidthOrHeight();
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent();
    void contextMenuEvent_ItemIgnoresTransformations();
#endif
    void update();
    void update2();
    void views();
    void testEvent();
    void eventsToDisabledItems();
    void exposedRect();
    void tabFocus_emptyScene();
    void tabFocus_sceneWithFocusableItems();
    void tabFocus_sceneWithFocusWidgets();
    void tabFocus_sceneWithNestedFocusWidgets();
    void style();
    void sorting();
    void insertionOrder();
    void changedSignal_data();
    void changedSignal();
    void stickyFocus_data();
    void stickyFocus();
    void sendEvent();
    void inputMethod_data();
    void inputMethod();
    void dispatchHoverOnPress();
    void initialFocus_data();
    void initialFocus();
    void polishItems();
    void polishItems2();
    void isActive();
    void siblingIndexAlwaysValid();
    void removeFullyTransparentItem();
    void zeroScale();
    void focusItemChangedSignal();
    void minimumRenderSize();
    void focusOnTouch();
    void clearSelection();

    // task specific tests below me
    void task139710_bspTreeCrash();
    void task139782_containsItemBoundingRect();
    void task176178_itemIndexMethodBreaksSceneRect();
    void task160653_selectionChanged();
    void task250680_childClip();
    void taskQTBUG_5904_crashWithDeviceCoordinateCache();
    void taskQT657_paintIntoCacheWithTransparentParts();
    void taskQTBUG_7863_paintIntoCacheWithTransparentParts();
    void taskQT_3674_doNotCrash();
    void taskQTBUG_15977_renderWithDeviceCoordinateCache();
    void taskQTBUG_16401_focusItem();
    void taskQTBUG_42915_focusNextPrevChild();
    void taskQTBUG_85088_previewTextfailWhenLostFocus();

    void deleteItemsOnChange();

private:
    QRect m_availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    QSize m_testSize;
};

tst_QGraphicsScene::tst_QGraphicsScene()
{
    const int testSize = qMax(200, m_availableGeometry.width() / 10);
    m_testSize.setWidth(testSize);
    m_testSize.setHeight(testSize);
}

void tst_QGraphicsScene::cleanup()
{
    // ensure not even skipped tests with custom input context leave it dangling
    QInputMethodPrivate *inputMethodPrivate =
        QInputMethodPrivate::get(QGuiApplication::inputMethod());
    inputMethodPrivate->testContext = nullptr;
    QTRY_VERIFY(QApplication::topLevelWidgets().isEmpty());
}

void tst_QGraphicsScene::construction()
{
    QGraphicsScene scene;
    QCOMPARE(scene.itemsBoundingRect(), QRectF());
    QVERIFY(scene.items().isEmpty());
    QVERIFY(scene.items(QPointF()).isEmpty());
    QVERIFY(scene.items(QRectF()).isEmpty());
    QVERIFY(scene.items(QPolygonF()).isEmpty());
    QVERIFY(scene.items(QPainterPath()).isEmpty());
    QTest::ignoreMessage(QtWarningMsg, "QGraphicsScene::collidingItems: cannot find collisions for null item");
    QVERIFY(scene.collidingItems(nullptr).isEmpty());
    QVERIFY(scene.items(QPointF()).isEmpty());
    QVERIFY(scene.selectedItems().isEmpty());
    QVERIFY(!scene.focusItem());
}

static inline const QGraphicsItem *itemAt(const QGraphicsScene &scene, qreal x, qreal y)
{
    return scene.items(QPointF(x, y)).value(0, nullptr);
}

void tst_QGraphicsScene::sceneRect()
{
    QGraphicsScene scene;
    QSignalSpy sceneRectChanged(&scene, &QGraphicsScene::sceneRectChanged);
    QCOMPARE(scene.sceneRect(), QRectF());
    QCOMPARE(sceneRectChanged.size(), 0);

    QGraphicsRectItem *item = scene.addRect(QRectF(0, 0, 10, 10));
    item->setPen(QPen(Qt::black, 0));
    item->setPos(-5, -5);
    QCOMPARE(sceneRectChanged.size(), 0);

    QCOMPARE(itemAt(scene, 0, 0), item);
    QVERIFY(scene.items(QPointF(10, 10)).isEmpty());
    QCOMPARE(sceneRectChanged.size(), 0);
    QCOMPARE(scene.sceneRect(), QRectF(-5, -5, 10, 10));
    QCOMPARE(sceneRectChanged.size(), 1);
    QCOMPARE(sceneRectChanged.last().at(0).toRectF(), scene.sceneRect());

    item->setPos(0, 0);
    QCOMPARE(scene.sceneRect(), QRectF(-5, -5, 15, 15));
    QCOMPARE(sceneRectChanged.size(), 2);
    QCOMPARE(sceneRectChanged.last().at(0).toRectF(), scene.sceneRect());

    scene.setSceneRect(-100, -100, 10, 10);
    QCOMPARE(sceneRectChanged.size(), 3);
    QCOMPARE(sceneRectChanged.last().at(0).toRectF(), scene.sceneRect());

    QCOMPARE(itemAt(scene, 0, 0), item);
    QVERIFY(scene.items(QPointF(10, 10)).isEmpty());
    QCOMPARE(scene.sceneRect(), QRectF(-100, -100, 10, 10));
    item->setPos(10, 10);
    QCOMPARE(scene.sceneRect(), QRectF(-100, -100, 10, 10));
    QCOMPARE(sceneRectChanged.size(), 3);
    QCOMPARE(sceneRectChanged.last().at(0).toRectF(), scene.sceneRect());

    scene.setSceneRect(QRectF());

    QCOMPARE(itemAt(scene, 10, 10), item);
    QVERIFY(scene.items(QPointF(20, 20)).isEmpty());
    QCOMPARE(sceneRectChanged.size(), 4);
    QCOMPARE(scene.sceneRect(), QRectF(-5, -5, 25, 25));
    QCOMPARE(sceneRectChanged.size(), 5);
    QCOMPARE(sceneRectChanged.last().at(0).toRectF(), scene.sceneRect());
}

void tst_QGraphicsScene::itemIndexMethod()
{
    QGraphicsScene scene;
    QCOMPARE(scene.itemIndexMethod(), QGraphicsScene::BspTreeIndex);

#ifdef Q_PROCESSOR_ARM
    const int minY = -500;
    const int maxY = 500;
    const int minX = -500;
    const int maxX = 500;
#else
    const int minY = -1000;
    const int maxY = 2000;
    const int minX = -1000;
    const int maxX = 2000;
#endif

    QList<QGraphicsItem *> items;
    for (int y = minY; y < maxY; y += 100) {
        for (int x = minX; x < maxX; x += 100) {
            QGraphicsItem *item = scene.addRect(QRectF(0, 0, 10, 10));
            item->setPos(x, y);
            QCOMPARE(itemAt(scene, x, y), item);
            items << item;
        }
    }

    int n = 0;
    for (int y = minY; y < maxY; y += 100) {
        for (int x = minX; x < maxX; x += 100)
            QCOMPARE(itemAt(scene, x, y), items.at(n++));
    }

    scene.setItemIndexMethod(QGraphicsScene::NoIndex);
    QCOMPARE(scene.itemIndexMethod(), QGraphicsScene::NoIndex);

    n = 0;
    for (int y = minY; y < maxY; y += 100) {
        for (int x = minX; x < maxX; x += 100)
            QCOMPARE(itemAt(scene, x, y), items.at(n++));
    }

    scene.setItemIndexMethod(QGraphicsScene::BspTreeIndex);
    QCOMPARE(scene.itemIndexMethod(), QGraphicsScene::BspTreeIndex);

    n = 0;
    for (int y = minY; y < maxY; y += 100) {
        for (int x = minX; x < maxX; x += 100)
            QCOMPARE(itemAt(scene, x, y), items.at(n++));
    }
}

void tst_QGraphicsScene::bspTreeDepth()
{
    QGraphicsScene scene;
    QCOMPARE(scene.itemIndexMethod(), QGraphicsScene::BspTreeIndex);
    QCOMPARE(scene.bspTreeDepth(), 0);
    scene.setBspTreeDepth(1);
    QCOMPARE(scene.bspTreeDepth(), 1);
    QTest::ignoreMessage(QtWarningMsg, "QGraphicsScene::setBspTreeDepth: invalid depth -1 ignored; must be >= 0");
    scene.setBspTreeDepth(-1);
    QCOMPARE(scene.bspTreeDepth(), 1);
}

void tst_QGraphicsScene::items()
{
#ifdef Q_PROCESSOR_ARM
    const int minY = -500;
    const int maxY = 500;
    const int minX = -500;
    const int maxX = 500;
#else
    const int minY = -1000;
    const int maxY = 2000;
    const int minX = -1000;
    const int maxX = 2000;
#endif

    {
        QGraphicsScene scene;

        QList<QGraphicsItem *> items;
        for (int y = minY; y < maxY; y += 100) {
            for (int x = minX; x < maxX; x += 100)
                items << scene.addRect(QRectF(0, 0, 10, 10));
        }
        QCOMPARE(scene.items().size(), items.size());
        itemAt(scene, 0, 0); // trigger indexing

        scene.removeItem(items.at(5));
        delete items.at(5);
        QVERIFY(!scene.items().contains(nullptr));
        delete items.at(7);
        QVERIFY(!scene.items().contains(nullptr));
    }
    {
        QGraphicsScene scene;
        QGraphicsLineItem *l1 = scene.addLine(-5, 0, 5, 0);
        l1->setPen(QPen(Qt::black, 0));
        QGraphicsLineItem *l2 = scene.addLine(0, -5, 0, 5);
        l2->setPen(QPen(Qt::black, 0));
        QVERIFY(!l1->sceneBoundingRect().intersects(l2->sceneBoundingRect()));
        QVERIFY(!l2->sceneBoundingRect().intersects(l1->sceneBoundingRect()));
        QList<QGraphicsItem *> items;
        items<<l1<<l2;
        QCOMPARE(scene.items().size(), items.size());
        QVERIFY(scene.items(QRectF(-1, -1, 2, 2)).contains(l1));
        QVERIFY(scene.items(QRectF(-1, -1, 2, 2)).contains(l2));
    }
}

void tst_QGraphicsScene::itemsBoundingRect_data()
{
    QTest::addColumn<QList<QRectF> >("rects");
    QTest::addColumn<QTransform>("transform");
    QTest::addColumn<QRectF>("boundingRect");

    QTransform transformation;
    transformation.translate(50, -50);
    transformation.scale(2, 2);
    transformation.rotate(90);

    QTest::newRow("none")
        << QList<QRectF>()
        << QTransform()
        << QRectF();
    QTest::newRow("{{0, 0, 10, 10}}")
        << (QList<QRectF>() << QRectF(0, 0, 10, 10))
        << QTransform()
        << QRectF(0, 0, 10, 10);
    QTest::newRow("{{-10, -10, 10, 10}}")
        << (QList<QRectF>() << QRectF(-10, -10, 10, 10))
        << QTransform()
        << QRectF(-10, -10, 10, 10);
    QTest::newRow("{{-1000, -1000, 1, 1}, {-10, -10, 10, 10}}")
        << (QList<QRectF>() << QRectF(-1000, -1000, 1, 1) << QRectF(-10, -10, 10, 10))
        << QTransform()
        << QRectF(-1000, -1000, 1000, 1000);
    QTest::newRow("transformed {{0, 0, 10, 10}}")
        << (QList<QRectF>() << QRectF(0, 0, 10, 10))
        << transformation
        << QRectF(30, -50, 20, 20);
    QTest::newRow("transformed {{-10, -10, 10, 10}}")
        << (QList<QRectF>() << QRectF(-10, -10, 10, 10))
        << transformation
        << QRectF(50, -70, 20, 20);
    QTest::newRow("transformed {{-1000, -1000, 1, 1}, {-10, -10, 10, 10}}")
        << (QList<QRectF>() << QRectF(-1000, -1000, 1, 1) << QRectF(-10, -10, 10, 10))
        << transformation
        << QRectF(50, -2050, 2000, 2000);

    QList<QRectF> all;
    for (int i = 0; i < 256; ++i)
        all << QRectF(randomX[i], randomY[i], 10, 10);
    QTest::newRow("all")
        << all
        << QTransform()
        << QRectF(-980, -994, 1988, 1983);
    QTest::newRow("transformed all")
        << all
        << transformation
        << QRectF(-1928, -2010, 3966, 3976);
}

void tst_QGraphicsScene::itemsBoundingRect()
{
    QFETCH(QList<QRectF>, rects);
    QFETCH(QTransform, transform);
    QFETCH(QRectF, boundingRect);

    QGraphicsScene scene;

    for (const auto &rect : std::as_const(rects)) {
        QPainterPath path;
        path.addRect(rect);
        QGraphicsPathItem *item = scene.addPath(path);
        item->setPen(QPen(Qt::black, 0));
        item->setTransform(transform);
    }

    QCOMPARE(scene.itemsBoundingRect(), boundingRect);
}

void tst_QGraphicsScene::items_QPointF_data()
{
    QTest::addColumn<QList<QRectF> >("items");
    QTest::addColumn<QPointF>("point");
    QTest::addColumn<QList<int> >("itemsAtPoint");

    QTest::newRow("empty")
        << QList<QRectF>()
        << QPointF()
        << QList<int>();
    QTest::newRow("1")
        << (QList<QRectF>() << QRectF(0, 0, 10, 10))
        << QPointF(0, 0)
        << (QList<int>() << 0);
    QTest::newRow("2")
        << (QList<QRectF>() << QRectF(0, 0, 10, 10))
        << QPointF(5, 5)
        << (QList<int>() << 0);
    QTest::newRow("3")
        << (QList<QRectF>() << QRectF(0, 0, 10, 10))
        << QPointF(9.9, 9.9)
        << (QList<int>() << 0);
    QTest::newRow("3.5")
        << (QList<QRectF>() << QRectF(0, 0, 10, 10))
        << QPointF(10, 10)
        << QList<int>();
    QTest::newRow("4")
        << (QList<QRectF>() << QRectF(0, 0, 10, 10) << QRectF(9.9, 9.9, 10, 10))
        << QPointF(9.9, 9.9)
        << (QList<int>() << 1 << 0);
    QTest::newRow("4.5")
        << (QList<QRectF>() << QRectF(0, 0, 10, 10) << QRectF(10, 10, 10, 10))
        << QPointF(10, 10)
        << (QList<int>() << 1);
    QTest::newRow("5")
        << (QList<QRectF>() << QRectF(5, 5, 10, 10) << QRectF(10, 10, 10, 10))
        << QPointF(10, 10)
        << (QList<int>() << 1 << 0);
    QTest::newRow("6")
        << (QList<QRectF>() << QRectF(5, 5, 10, 10) << QRectF(10, 10, 10, 10) << QRectF(0, 0, 20, 30))
        << QPointF(10, 10)
        << (QList<int>() << 2 << 1 << 0);
}

void tst_QGraphicsScene::items_QPointF()
{
    QFETCH(QList<QRectF>, items);
    QFETCH(QPointF, point);
    QFETCH(QList<int>, itemsAtPoint);

    QGraphicsScene scene;

    int n = 0;
    QList<QGraphicsItem *> addedItems;
    for (const auto &rect : std::as_const(items)) {
        QPainterPath path;
        path.addRect(0, 0, rect.width(), rect.height());

        QGraphicsPathItem *item = scene.addPath(path);
        item->setPen(QPen(Qt::black, 0));
        item->setZValue(n++);
        item->setPos(rect.topLeft());
        addedItems << item;
    }

    QList<int> itemIndexes;
    const auto &actualItemsAtPoint = scene.items(point);
    for (QGraphicsItem *item : actualItemsAtPoint)
        itemIndexes << addedItems.indexOf(item);

    QCOMPARE(itemIndexes, itemsAtPoint);
}

void tst_QGraphicsScene::items_QRectF()
{
    QGraphicsScene scene;
    QGraphicsItem *item1 = scene.addRect(QRectF(-10, -10, 10, 10));
    QGraphicsItem *item2 = scene.addRect(QRectF(10, -10, 10, 10));
    QGraphicsItem *item3 = scene.addRect(QRectF(10, 10, 10, 10));
    QGraphicsItem *item4 = scene.addRect(QRectF(-10, 10, 10, 10));

    item1->setZValue(0);
    item2->setZValue(1);
    item3->setZValue(2);
    item4->setZValue(3);

    QCOMPARE(scene.items(QRectF(-10, -10, 10, 10)), {item1});
    QCOMPARE(scene.items(QRectF(10, -10, 10, 10)), {item2});
    QCOMPARE(scene.items(QRectF(10, 10, 10, 10)), {item3});
    QCOMPARE(scene.items(QRectF(-10, 10, 10, 10)), {item4});
    QCOMPARE(scene.items(QRectF(-10, -10, 1, 1)), {item1});
    QCOMPARE(scene.items(QRectF(10, -10, 1, 1)), {item2});
    QCOMPARE(scene.items(QRectF(10, 10, 1, 1)), {item3});
    QCOMPARE(scene.items(QRectF(-10, 10, 1, 1)), {item4});

    QCOMPARE(scene.items(QRectF(-10, -10, 40, 10)), QList<QGraphicsItem *>() << item2 << item1);
    QCOMPARE(scene.items(QRectF(-10, 10, 40, 10)), QList<QGraphicsItem *>() << item4 << item3);

    item1->setZValue(3);
    item2->setZValue(2);
    item3->setZValue(1);
    item4->setZValue(0);

    QCOMPARE(scene.items(QRectF(-10, -10, 40, 10)), QList<QGraphicsItem *>() << item1 << item2);
    QCOMPARE(scene.items(QRectF(-10, 10, 40, 10)), QList<QGraphicsItem *>() << item3 << item4);
}

void tst_QGraphicsScene::items_QRectF_2_data()
{
    QTest::addColumn<QRectF>("ellipseRect");
    QTest::addColumn<QRectF>("sceneRect");
    QTest::addColumn<Qt::ItemSelectionMode>("selectionMode");
    QTest::addColumn<bool>("contained");
    QTest::addColumn<bool>("containedRotated");

    // None of the rects contain the ellipse's shape nor bounding rect
    QTest::newRow("1") << QRectF(0, 0, 100, 100) << QRectF(1, 1, 10, 10) << Qt::ContainsItemShape << false << false;
    QTest::newRow("2") << QRectF(0, 0, 100, 100) << QRectF(1, 89, 10, 10) << Qt::ContainsItemShape << false << false;
    QTest::newRow("3") << QRectF(0, 0, 100, 100) << QRectF(89, 1, 10, 10) << Qt::ContainsItemShape << false << false;
    QTest::newRow("4") << QRectF(0, 0, 100, 100) << QRectF(89, 89, 10, 10) << Qt::ContainsItemShape << false << false;
    QTest::newRow("5") << QRectF(0, 0, 100, 100) << QRectF(1, 1, 10, 10) << Qt::ContainsItemBoundingRect << false << false;
    QTest::newRow("6") << QRectF(0, 0, 100, 100) << QRectF(1, 89, 10, 10) << Qt::ContainsItemBoundingRect << false << false;
    QTest::newRow("7") << QRectF(0, 0, 100, 100) << QRectF(89, 1, 10, 10) << Qt::ContainsItemBoundingRect << false << false;
    QTest::newRow("8") << QRectF(0, 0, 100, 100) << QRectF(89, 89, 10, 10) << Qt::ContainsItemBoundingRect << false << false;
    QTest::newRow("9") << QRectF(0, 0, 100, 100) << QRectF(0, 0, 50, 50) << Qt::ContainsItemShape << false << false;
    QTest::newRow("10") << QRectF(0, 0, 100, 100) << QRectF(0, 50, 50, 50) << Qt::ContainsItemShape << false << false;
    QTest::newRow("11") << QRectF(0, 0, 100, 100) << QRectF(50, 0, 50, 50) << Qt::ContainsItemShape << false << false;
    QTest::newRow("12") << QRectF(0, 0, 100, 100) << QRectF(50, 50, 50, 50) << Qt::ContainsItemShape << false << false;
    QTest::newRow("13") << QRectF(0, 0, 100, 100) << QRectF(0, 0, 50, 50) << Qt::ContainsItemBoundingRect << false << false;
    QTest::newRow("14") << QRectF(0, 0, 100, 100) << QRectF(0, 50, 50, 50) << Qt::ContainsItemBoundingRect << false << false;
    QTest::newRow("15") << QRectF(0, 0, 100, 100) << QRectF(50, 0, 50, 50) << Qt::ContainsItemBoundingRect << false << false;
    QTest::newRow("16") << QRectF(0, 0, 100, 100) << QRectF(50, 50, 50, 50) << Qt::ContainsItemBoundingRect << false << false;
    QTest::newRow("17") << QRectF(0, 0, 100, 100) << QRectF(-50, -50, 100, 100) << Qt::ContainsItemShape << false << false;
    QTest::newRow("18") << QRectF(0, 0, 100, 100) << QRectF(0, -50, 100, 100) << Qt::ContainsItemShape << false << false;
    QTest::newRow("19") << QRectF(0, 0, 100, 100) << QRectF(-50, 0, 100, 100) << Qt::ContainsItemShape << false << false;
    QTest::newRow("20") << QRectF(0, 0, 100, 100) << QRectF(0, 0, 100, 100) << Qt::ContainsItemShape << false << false;
    QTest::newRow("21") << QRectF(0, 0, 100, 100) << QRectF(-50, -50, 100, 100) << Qt::ContainsItemBoundingRect << false << false;
    QTest::newRow("22") << QRectF(0, 0, 100, 100) << QRectF(0, -50, 100, 100) << Qt::ContainsItemBoundingRect << false << false;
    QTest::newRow("23") << QRectF(0, 0, 100, 100) << QRectF(-50, 0, 100, 100) << Qt::ContainsItemBoundingRect << false << false;

    // The rect is the same as the ellipse's bounding rect
    QTest::newRow("24") << QRectF(0, 0, 100, 100) << QRectF(0, 0, 100, 100) << Qt::ContainsItemBoundingRect << false << false;

    // None intersects with the item's shape, but they all intersects with the
    // item's bounding rect.
    QTest::newRow("25") << QRectF(0, 0, 100, 100) << QRectF(1, 1, 10, 10) << Qt::IntersectsItemShape << false << false;
    QTest::newRow("26") << QRectF(0, 0, 100, 100) << QRectF(1, 89, 10, 10) << Qt::IntersectsItemShape << false << true;
    QTest::newRow("27") << QRectF(0, 0, 100, 100) << QRectF(89, 1, 10, 10) << Qt::IntersectsItemShape << false << false;
    QTest::newRow("28") << QRectF(0, 0, 100, 100) << QRectF(89, 89, 10, 10) << Qt::IntersectsItemShape << false << false;
    QTest::newRow("29") << QRectF(0, 0, 100, 100) << QRectF(1, 1, 10, 10) << Qt::IntersectsItemBoundingRect << true << true;
    QTest::newRow("30") << QRectF(0, 0, 100, 100) << QRectF(1, 89, 10, 10) << Qt::IntersectsItemBoundingRect << true << true;
    QTest::newRow("31") << QRectF(0, 0, 100, 100) << QRectF(89, 1, 10, 10) << Qt::IntersectsItemBoundingRect << true << false;
    QTest::newRow("32") << QRectF(0, 0, 100, 100) << QRectF(89, 89, 10, 10) << Qt::IntersectsItemBoundingRect << true << false;

    // This rect does not contain the shape nor the bounding rect
    QTest::newRow("33") << QRectF(0, 0, 100, 100) << QRectF(5, 5, 90, 90) << Qt::ContainsItemShape << false << false;
    QTest::newRow("34") << QRectF(0, 0, 100, 100) << QRectF(5, 5, 90, 90) << Qt::ContainsItemBoundingRect << false << false;

    // It will, however, intersect with both
    QTest::newRow("35") << QRectF(0, 0, 100, 100) << QRectF(5, 5, 90, 90) << Qt::IntersectsItemShape << true << true;
    QTest::newRow("36") << QRectF(0, 0, 100, 100) << QRectF(5, 5, 90, 90) << Qt::IntersectsItemBoundingRect << true << true;

    // A rect that contains the whole ellipse will both contain and intersect
    // with both the ellipse's shape and bounding rect.
    QTest::newRow("37") << QRectF(0, 0, 100, 100) << QRectF(-5, -5, 110, 110) << Qt::IntersectsItemBoundingRect << true << true;
    QTest::newRow("38") << QRectF(0, 0, 100, 100) << QRectF(-5, -5, 110, 110) << Qt::IntersectsItemShape << true << true;
    QTest::newRow("39") << QRectF(0, 0, 100, 100) << QRectF(-5, -5, 110, 110) << Qt::ContainsItemBoundingRect << true << false;
    QTest::newRow("40") << QRectF(0, 0, 100, 100) << QRectF(-5, -5, 110, 110) << Qt::ContainsItemShape << true << false;

    // A rect that is fully contained within the ellipse will intersect only
    QTest::newRow("41") << QRectF(0, 0, 100, 100) << QRectF(40, 40, 20, 20) << Qt::ContainsItemShape << false << false;
    QTest::newRow("42") << QRectF(0, 0, 100, 100) << QRectF(40, 40, 20, 20) << Qt::ContainsItemBoundingRect << false << false;
    QTest::newRow("43") << QRectF(0, 0, 100, 100) << QRectF(40, 40, 20, 20) << Qt::IntersectsItemShape << true << true;
    QTest::newRow("44") << QRectF(0, 0, 100, 100) << QRectF(40, 40, 20, 20) << Qt::IntersectsItemBoundingRect << true << true;
}

void tst_QGraphicsScene::items_QRectF_2()
{
    QFETCH(QRectF, ellipseRect);
    QFETCH(QRectF, sceneRect);
    QFETCH(Qt::ItemSelectionMode, selectionMode);
    QFETCH(bool, contained);
    QFETCH(bool, containedRotated);

    QGraphicsScene scene;
    QGraphicsItem *item = scene.addEllipse(ellipseRect);

    QCOMPARE(!scene.items(sceneRect, selectionMode).isEmpty(), contained);
    item->setTransform(QTransform().rotate(45), true);
    QCOMPARE(!scene.items(sceneRect, selectionMode).isEmpty(), containedRotated);
}

void tst_QGraphicsScene::items_QPolygonF()
{
    QGraphicsScene scene;
    QGraphicsItem *item1 = scene.addRect(QRectF(-10, -10, 10, 10));
    QGraphicsItem *item2 = scene.addRect(QRectF(10, -10, 10, 10));
    QGraphicsItem *item3 = scene.addRect(QRectF(10, 10, 10, 10));
    QGraphicsItem *item4 = scene.addRect(QRectF(-10, 10, 10, 10));

    item1->setZValue(0);
    item2->setZValue(1);
    item3->setZValue(2);
    item4->setZValue(3);

    QPolygonF poly1(item1->boundingRect());
    QPolygonF poly2(item2->boundingRect());
    QPolygonF poly3(item3->boundingRect());
    QPolygonF poly4(item4->boundingRect());

    QCOMPARE(scene.items(poly1), {item1});
    QCOMPARE(scene.items(poly2), {item2});
    QCOMPARE(scene.items(poly3), {item3});
    QCOMPARE(scene.items(poly4), {item4});

    poly1 = QPolygonF(QRectF(-10, -10, 1, 1));
    poly2 = QPolygonF(QRectF(10, -10, 1, 1));
    poly3 = QPolygonF(QRectF(10, 10, 1, 1));
    poly4 = QPolygonF(QRectF(-10, 10, 1, 1));

    QCOMPARE(scene.items(poly1), {item1});
    QCOMPARE(scene.items(poly2), {item2});
    QCOMPARE(scene.items(poly3), {item3});
    QCOMPARE(scene.items(poly4), {item4});

    poly1 = QPolygonF(QRectF(-10, -10, 40, 10));
    poly2 = QPolygonF(QRectF(-10, 10, 40, 10));

    QCOMPARE(scene.items(poly1), QList<QGraphicsItem *>() << item2 << item1);
    QCOMPARE(scene.items(poly2), QList<QGraphicsItem *>() << item4 << item3);

    item1->setZValue(3);
    item2->setZValue(2);
    item3->setZValue(1);
    item4->setZValue(0);

    QCOMPARE(scene.items(poly1), QList<QGraphicsItem *>() << item1 << item2);
    QCOMPARE(scene.items(poly2), QList<QGraphicsItem *>() << item3 << item4);
}

void tst_QGraphicsScene::items_QPolygonF_2()
{
    QGraphicsScene scene;
    QGraphicsItem *ellipse = scene.addEllipse(QRectF(0, 0, 100, 100));

    // None of the rects contain the ellipse's shape nor bounding rect
    QVERIFY(scene.items(QPolygonF(QRectF(1, 1, 10, 10)), Qt::ContainsItemShape).isEmpty());
    QVERIFY(scene.items(QPolygonF(QRectF(1, 89, 10, 10)), Qt::ContainsItemShape).isEmpty());
    QVERIFY(scene.items(QPolygonF(QRectF(89, 1, 10, 10)), Qt::ContainsItemShape).isEmpty());
    QVERIFY(scene.items(QPolygonF(QRectF(89, 89, 10, 10)), Qt::ContainsItemShape).isEmpty());
    QVERIFY(scene.items(QPolygonF(QRectF(1, 1, 10, 10)), Qt::ContainsItemBoundingRect).isEmpty());
    QVERIFY(scene.items(QPolygonF(QRectF(1, 89, 10, 10)), Qt::ContainsItemBoundingRect).isEmpty());
    QVERIFY(scene.items(QPolygonF(QRectF(89, 1, 10, 10)), Qt::ContainsItemBoundingRect).isEmpty());
    QVERIFY(scene.items(QPolygonF(QRectF(89, 89, 10, 10)), Qt::ContainsItemBoundingRect).isEmpty());

    // None intersects with the item's shape, but they all intersects with the
    // item's bounding rect.
    QVERIFY(scene.items(QPolygonF(QRectF(1, 1, 10, 10)), Qt::IntersectsItemShape).isEmpty());
    QVERIFY(scene.items(QPolygonF(QRectF(1, 89, 10, 10)), Qt::IntersectsItemShape).isEmpty());
    QVERIFY(scene.items(QPolygonF(QRectF(89, 1, 10, 10)), Qt::IntersectsItemShape).isEmpty());
    QVERIFY(scene.items(QPolygonF(QRectF(89, 89, 10, 10)), Qt::IntersectsItemShape).isEmpty());
    QCOMPARE(scene.items(QPolygonF(QRectF(1, 1, 10, 10)), Qt::IntersectsItemBoundingRect).first(), ellipse);
    QCOMPARE(scene.items(QPolygonF(QRectF(1, 89, 10, 10)), Qt::IntersectsItemBoundingRect).first(), ellipse);
    QCOMPARE(scene.items(QPolygonF(QRectF(89, 1, 10, 10)), Qt::IntersectsItemBoundingRect).first(), ellipse);
    QCOMPARE(scene.items(QPolygonF(QRectF(89, 89, 10, 10)), Qt::IntersectsItemBoundingRect).first(), ellipse);

    // This rect does not contain the shape nor the bounding rect
    QVERIFY(scene.items(QPolygonF(QRectF(5, 5, 90, 90)), Qt::ContainsItemShape).isEmpty());
    QVERIFY(scene.items(QPolygonF(QRectF(5, 5, 90, 90)), Qt::ContainsItemBoundingRect).isEmpty());

    // It will, however, intersect with both
    QCOMPARE(scene.items(QPolygonF(QRectF(5, 5, 90, 90)), Qt::IntersectsItemShape).first(), ellipse);
    QCOMPARE(scene.items(QPolygonF(QRectF(5, 5, 90, 90)), Qt::IntersectsItemBoundingRect).first(), ellipse);

    // A rect that contains the whole ellipse will both contain and intersect
    // with both the ellipse's shape and bounding rect.
    QCOMPARE(scene.items(QPolygonF(QRectF(-5, -5, 110, 110)), Qt::IntersectsItemShape).first(), ellipse);
    QCOMPARE(scene.items(QPolygonF(QRectF(-5, -5, 110, 110)), Qt::ContainsItemShape).first(), ellipse);
    QCOMPARE(scene.items(QPolygonF(QRectF(-5, -5, 110, 110)), Qt::IntersectsItemBoundingRect).first(), ellipse);
    QCOMPARE(scene.items(QPolygonF(QRectF(-5, -5, 110, 110)), Qt::ContainsItemBoundingRect).first(), ellipse);
}

void tst_QGraphicsScene::items_QPainterPath()
{
    QGraphicsScene scene;
    QGraphicsItem *item1 = scene.addRect(QRectF(-10, -10, 10, 10));
    QGraphicsItem *item2 = scene.addRect(QRectF(10, -10, 10, 10));
    QGraphicsItem *item3 = scene.addRect(QRectF(10, 10, 10, 10));
    QGraphicsItem *item4 = scene.addRect(QRectF(-10, 10, 10, 10));

    item1->setZValue(0);
    item2->setZValue(1);
    item3->setZValue(2);
    item4->setZValue(3);

    QPainterPath path1; path1.addEllipse(item1->boundingRect());
    QPainterPath path2; path2.addEllipse(item2->boundingRect());
    QPainterPath path3; path3.addEllipse(item3->boundingRect());
    QPainterPath path4; path4.addEllipse(item4->boundingRect());

    QCOMPARE(scene.items(path1), {item1});
    QCOMPARE(scene.items(path2), {item2});
    QCOMPARE(scene.items(path3), {item3});
    QCOMPARE(scene.items(path4), {item4});

    path1 = QPainterPath(); path1.addEllipse(QRectF(-10, -10, 1, 1));
    path2 = QPainterPath(); path2.addEllipse(QRectF(10, -10, 1, 1));
    path3 = QPainterPath(); path3.addEllipse(QRectF(10, 10, 1, 1));
    path4 = QPainterPath(); path4.addEllipse(QRectF(-10, 10, 1, 1));

    QCOMPARE(scene.items(path1), {item1});
    QCOMPARE(scene.items(path2), {item2});
    QCOMPARE(scene.items(path3), {item3});
    QCOMPARE(scene.items(path4), {item4});

    path1 = QPainterPath(); path1.addRect(QRectF(-10, -10, 40, 10));
    path2 = QPainterPath(); path2.addRect(QRectF(-10, 10, 40, 10));

    QCOMPARE(scene.items(path1), QList<QGraphicsItem *>() << item2 << item1);
    QCOMPARE(scene.items(path2), QList<QGraphicsItem *>() << item4 << item3);

    item1->setZValue(3);
    item2->setZValue(2);
    item3->setZValue(1);
    item4->setZValue(0);

    QCOMPARE(scene.items(path1), QList<QGraphicsItem *>() << item1 << item2);
    QCOMPARE(scene.items(path2), QList<QGraphicsItem *>() << item3 << item4);
}

void tst_QGraphicsScene::items_QPainterPath_2()
{
    QGraphicsScene scene;
    QGraphicsItem *ellipse = scene.addEllipse(QRectF(0, 0, 100, 100));

    QPainterPath p1; p1.addRect(QRectF(1, 1, 10, 10));
    QPainterPath p2; p2.addRect(QRectF(1, 89, 10, 10));
    QPainterPath p3; p3.addRect(QRectF(89, 1, 10, 10));
    QPainterPath p4; p4.addRect(QRectF(89, 89, 10, 10));

    // None of the rects contain the ellipse's shape nor bounding rect
    QVERIFY(scene.items(p1, Qt::ContainsItemShape).isEmpty());
    QVERIFY(scene.items(p2, Qt::ContainsItemShape).isEmpty());
    QVERIFY(scene.items(p3, Qt::ContainsItemShape).isEmpty());
    QVERIFY(scene.items(p4, Qt::ContainsItemShape).isEmpty());
    QVERIFY(scene.items(p1, Qt::ContainsItemBoundingRect).isEmpty());
    QVERIFY(scene.items(p2, Qt::ContainsItemBoundingRect).isEmpty());
    QVERIFY(scene.items(p3, Qt::ContainsItemBoundingRect).isEmpty());
    QVERIFY(scene.items(p4, Qt::ContainsItemBoundingRect).isEmpty());

    // None intersects with the item's shape, but they all intersects with the
    // item's bounding rect.
    QVERIFY(scene.items(p1, Qt::IntersectsItemShape).isEmpty());
    QVERIFY(scene.items(p2, Qt::IntersectsItemShape).isEmpty());
    QVERIFY(scene.items(p3, Qt::IntersectsItemShape).isEmpty());
    QVERIFY(scene.items(p4, Qt::IntersectsItemShape).isEmpty());
    QCOMPARE(scene.items(p1, Qt::IntersectsItemBoundingRect).first(), ellipse);
    QCOMPARE(scene.items(p2, Qt::IntersectsItemBoundingRect).first(), ellipse);
    QCOMPARE(scene.items(p3, Qt::IntersectsItemBoundingRect).first(), ellipse);
    QCOMPARE(scene.items(p4, Qt::IntersectsItemBoundingRect).first(), ellipse);

    QPainterPath p5;
    p5.addRect(QRectF(5, 5, 90, 90));

    // This rect does not contain the shape nor the bounding rect
    QVERIFY(scene.items(p5, Qt::ContainsItemShape).isEmpty());
    QVERIFY(scene.items(p5, Qt::ContainsItemBoundingRect).isEmpty());

    // It will, however, intersect with both
    QCOMPARE(scene.items(p5, Qt::IntersectsItemShape).first(), ellipse);
    QCOMPARE(scene.items(p5, Qt::IntersectsItemBoundingRect).first(), ellipse);

    QPainterPath p6;
    p6.addRect(QRectF(-5, -5, 110, 110));

    // A rect that contains the whole ellipse will both contain and intersect
    // with both the ellipse's shape and bounding rect.
    QCOMPARE(scene.items(p6, Qt::IntersectsItemShape).first(), ellipse);
    QCOMPARE(scene.items(p6, Qt::ContainsItemShape).first(), ellipse);
    QCOMPARE(scene.items(p6, Qt::IntersectsItemBoundingRect).first(), ellipse);
    QCOMPARE(scene.items(p6, Qt::ContainsItemBoundingRect).first(), ellipse);
}

class CustomView : public QGraphicsView
{
public:
    using QGraphicsView::QGraphicsView;

    int repaints = 0;
protected:
    void paintEvent(QPaintEvent *event) override
    {
        ++repaints;
        QGraphicsView::paintEvent(event);
    }
};

void tst_QGraphicsScene::selectionChanged()
{
    QGraphicsScene scene(0, 0, 1000, 1000);
    QSignalSpy spy(&scene, &QGraphicsScene::selectionChanged);
    QCOMPARE(spy.size(), 0);

    QPainterPath path;
    path.addRect(scene.sceneRect());
    QCOMPARE(scene.selectionArea(), QPainterPath());
    scene.setSelectionArea(path);
    QCOMPARE(scene.selectionArea(), path);
    QCOMPARE(spy.size(), 0); // selection didn't change
    QVERIFY(scene.selectedItems().isEmpty());

    QGraphicsItem *rect = scene.addRect(QRectF(0, 0, 100, 100));
    QCOMPARE(spy.size(), 0); // selection didn't change

    rect->setSelected(true);
    QVERIFY(!rect->isSelected());
    QCOMPARE(spy.size(), 0); // selection didn't change, item isn't selectable

    rect->setFlag(QGraphicsItem::ItemIsSelectable);
    rect->setSelected(true);
    QVERIFY(rect->isSelected());
    QCOMPARE(spy.size(), 1); // selection changed
    QCOMPARE(scene.selectedItems(), {rect});

    rect->setSelected(false);
    QVERIFY(!rect->isSelected());
    QCOMPARE(spy.size(), 2); // selection changed
    QVERIFY(scene.selectedItems().isEmpty());

    QGraphicsEllipseItem *parentItem = new QGraphicsEllipseItem(QRectF(0, 0, 100, 100));
    QGraphicsEllipseItem *childItem = new QGraphicsEllipseItem(QRectF(0, 0, 100, 100), parentItem);
    QGraphicsEllipseItem *grandChildItem = new QGraphicsEllipseItem(QRectF(0, 0, 100, 100), childItem);
    grandChildItem->setFlag(QGraphicsItem::ItemIsSelectable);
    grandChildItem->setSelected(true);
    grandChildItem->setSelected(false);
    grandChildItem->setSelected(true);
    scene.addItem(parentItem);

    QCOMPARE(spy.size(), 3); // the grandchild was added, so the selection changed once

    scene.removeItem(parentItem);
    QCOMPARE(spy.size(), 4); // the grandchild was removed, so the selection changed

    rect->setSelected(true);
    QCOMPARE(spy.size(), 5); // the rect was reselected, so the selection changed

    scene.clearSelection();
    QCOMPARE(spy.size(), 6); // the scene selection was cleared

    rect->setSelected(true);
    QCOMPARE(spy.size(), 7); // the rect was reselected, so the selection changed

    rect->setFlag(QGraphicsItem::ItemIsSelectable, false);
    QCOMPARE(spy.size(), 8); // the rect was unselected, so the selection changed

    rect->setSelected(true);
    QCOMPARE(spy.size(), 8); // the rect is not longer selectable, so the selection does not change


    rect->setFlag(QGraphicsItem::ItemIsSelectable, true);
    rect->setSelected(true);
    QCOMPARE(spy.size(), 9); // the rect is again selectable, so the selection changed

    delete rect;
    QCOMPARE(spy.size(), 10); // a selected item was deleted; selection changed
}

void tst_QGraphicsScene::selectionChanged2()
{
    QGraphicsScene scene;
    QSignalSpy spy(&scene, &QGraphicsScene::selectionChanged);

    QGraphicsItem *item1 = scene.addRect(0, 0, 100, 100);
    QGraphicsItem *item2 = scene.addRect(100, 100, 100, 100);
    item1->setFlag(QGraphicsItem::ItemIsSelectable);
    item2->setFlag(QGraphicsItem::ItemIsSelectable);

    QCOMPARE(spy.size(), 0);
    {
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMousePress);
        event.setScenePos(QPointF(50, 50));
        event.setButton(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &event);
    }
    {
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseRelease);
        event.setScenePos(QPointF(50, 50));
        event.setButton(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &event);
    }
    QVERIFY(item1->isSelected());
    QVERIFY(!item2->isSelected());
    QCOMPARE(spy.size(), 1);
    {
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMousePress);
        event.setScenePos(QPointF(150, 150));
        event.setButton(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &event);
    }
    {
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseRelease);
        event.setScenePos(QPointF(150, 150));
        event.setButton(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &event);
    }
    QVERIFY(!item1->isSelected());
    QVERIFY(item2->isSelected());
    QCOMPARE(spy.size(), 2);
    {
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMousePress);
        event.setScenePos(QPointF(50, 50));
        event.setButton(Qt::LeftButton);
        event.setModifiers(Qt::ControlModifier);
        QCoreApplication::sendEvent(&scene, &event);
    }
    QVERIFY(!item1->isSelected());
    QVERIFY(item2->isSelected());
    QCOMPARE(spy.size(), 2);
    {
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseRelease);
        event.setScenePos(QPointF(50, 50));
        event.setButton(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &event);
    }
    QVERIFY(item1->isSelected());
    QVERIFY(!item2->isSelected());
    QCOMPARE(spy.size(), 3);
}

void tst_QGraphicsScene::addItem()
{
    Q_CHECK_PAINTEVENTS
    {
        // 1) Create item, then scene, then add item
        QGraphicsItem *path = new QGraphicsEllipseItem(QRectF(-10, -10, 20, 20));
        QGraphicsScene scene;

        CustomView view;
        view.setWindowTitle(QTest::currentTestFunction());
        view.setScene(&scene);
        view.resize(m_testSize);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        QCoreApplication::processEvents();
        view.repaints = 0;

        scene.addItem(path);

        // Adding an item should always issue a repaint.
        QTRY_VERIFY(view.repaints > 0);
        view.repaints = 0;

        QCOMPARE(itemAt(scene, 0, 0), path);

        QGraphicsItem *path2 = new QGraphicsEllipseItem(QRectF(-10, -10, 20, 20));
        path2->setPos(100, 100);

        QCOMPARE(itemAt(scene, 0, 0), path);
        QVERIFY(scene.items(QPointF(100, 100)).isEmpty());
        scene.addItem(path2);

        // Adding an item should always issue a repaint.
        QTRY_VERIFY(view.repaints > 0);

        QCOMPARE(itemAt(scene, 100, 100), path2);
    }
    {
        // 2) Create scene, then item, then add item
        QGraphicsScene scene;
        QGraphicsItem *path = new QGraphicsEllipseItem(QRectF(-10, -10, 20, 20));
        scene.addItem(path);

        QGraphicsItem *path2 = new QGraphicsEllipseItem(QRectF(-10, -10, 20, 20));
        path2->setPos(100, 100);
        scene.addItem(path2);

        QCOMPARE(itemAt(scene, 0, 0), path);
        QCOMPARE(itemAt(scene, 100, 100), path2);
    }
}

void tst_QGraphicsScene::addEllipse()
{
    QGraphicsScene scene;
    QGraphicsEllipseItem *ellipse = scene.addEllipse(QRectF(-10, -10, 20, 20),
                                                     QPen(Qt::red), QBrush(Qt::blue));
    QCOMPARE(ellipse->pos(), QPointF());
    QCOMPARE(ellipse->pen(), QPen(Qt::red));
    QCOMPARE(ellipse->brush(), QBrush(Qt::blue));
    QCOMPARE(ellipse->rect(), QRectF(-10, -10, 20, 20));
    QCOMPARE(itemAt(scene, 0, 0), ellipse);
    QVERIFY(scene.items(QPointF(-10, -10)).isEmpty());
    QCOMPARE(itemAt(scene, -9.9, 0), ellipse);
    QVERIFY(scene.items(QPointF(-10, 10)).isEmpty());
    QCOMPARE(itemAt(scene, 0, -9.9), ellipse);
    QCOMPARE(itemAt(scene, 0, 9.9), ellipse);
    QVERIFY(scene.items(QPointF(10, -10)).isEmpty());
    QCOMPARE(itemAt(scene, 9.9, 0), ellipse);
    QVERIFY(scene.items(QPointF(10, 10)).isEmpty());
}

void tst_QGraphicsScene::addLine()
{
    QGraphicsScene scene;
    QPen pen(Qt::red);
    pen.setWidthF(1.0);
    QGraphicsLineItem *line = scene.addLine(QLineF(-10, -10, 20, 20),
                                            pen);
    QCOMPARE(line->pos(), QPointF());
    QCOMPARE(line->pen(), pen);
    QCOMPARE(line->line(), QLineF(-10, -10, 20, 20));
    QCOMPARE(itemAt(scene, 0, 0), line);
    QCOMPARE(itemAt(scene, -10, -10), line);
    QVERIFY(scene.items(QPointF(-9.9, 0)).isEmpty());
    QVERIFY(scene.items(QPointF(-10, 10)).isEmpty());
    QVERIFY(scene.items(QPointF(0, -9.9)).isEmpty());
    QVERIFY(scene.items(QPointF(0, 9.9)).isEmpty());
    QVERIFY(scene.items(QPointF(10, -10)).isEmpty());
    QVERIFY(scene.items(QPointF(9.9, 0)).isEmpty());
    QCOMPARE(itemAt(scene, 10, 10), line);
}

void tst_QGraphicsScene::addPath()
{
    QGraphicsScene scene;
    QPainterPath p;
    p.addEllipse(QRectF(-10, -10, 20, 20));
    p.addEllipse(QRectF(-10, 20, 20, 20));

    QGraphicsPathItem *path = scene.addPath(p, QPen(Qt::red), QBrush(Qt::blue));
    QCOMPARE(path->pos(), QPointF());
    QCOMPARE(path->pen(), QPen(Qt::red));
    QCOMPARE(path->path(), p);
    QCOMPARE(path->brush(), QBrush(Qt::blue));

    path->setPen(QPen(Qt::red, 0));

    QCOMPARE(itemAt(scene, 0, 0), path);
    QCOMPARE(itemAt(scene, -9.9, 0), path);
    QCOMPARE(itemAt(scene, 9.9, 0), path);
    QCOMPARE(itemAt(scene, 0, -9.9), path);
    QCOMPARE(itemAt(scene, 0, 9.9), path);
    QCOMPARE(itemAt(scene, 0, 30), path);
    QCOMPARE(itemAt(scene, -9.9, 30), path);
    QCOMPARE(itemAt(scene, 9.9, 30), path);
    QCOMPARE(itemAt(scene, 0, 20.1), path);
    QCOMPARE(itemAt(scene, 0, 39.9), path);
    QVERIFY(scene.items(QPointF(-10, -10)).isEmpty());
    QVERIFY(scene.items(QPointF(10, -10)).isEmpty());
    QVERIFY(scene.items(QPointF(-10, 10)).isEmpty());
    QVERIFY(scene.items(QPointF(10, 10)).isEmpty());
    QVERIFY(scene.items(QPointF(-10, 20)).isEmpty());
    QVERIFY(scene.items(QPointF(10, 20)).isEmpty());
    if (sizeof(qreal) != sizeof(double))
        qWarning("Skipping test because of rounding errors when qreal != double");
    else
        QVERIFY(scene.items(QPointF(-10, 30)).isEmpty());
    QVERIFY(scene.items(QPointF(10.1, 30)).isEmpty());
}

void tst_QGraphicsScene::addPixmap()
{
    QGraphicsScene scene;
    QPixmap pix(":/Ash_European.jpg");
    QGraphicsPixmapItem *pixmap = scene.addPixmap(pix);

    QCOMPARE(pixmap->pos(), QPointF());
    QCOMPARE(pixmap->pixmap(), pix);
    QCOMPARE(itemAt(scene, 0, 0), pixmap);
    QCOMPARE(itemAt(scene, pix.width() - 1, 0), pixmap);
    QCOMPARE(itemAt(scene, 0, pix.height() - 1), pixmap);
    QCOMPARE(itemAt(scene, pix.width() - 1, pix.height() - 1), pixmap);

    QVERIFY(scene.items(QPointF(-1, -1)).isEmpty());
    QVERIFY(scene.items(QPointF(pix.width() - 1, -1)).isEmpty());
    QVERIFY(scene.items(QPointF(-1, pix.height() - 1)).isEmpty());
    QVERIFY(scene.items(QPointF(pix.width(), pix.height())).isEmpty());
    QVERIFY(scene.items(QPointF(0, pix.height())).isEmpty());
    QVERIFY(scene.items(QPointF(pix.width(), 0)).isEmpty());
}

void tst_QGraphicsScene::addRect()
{
    QGraphicsScene scene;
    QGraphicsRectItem *rect = scene.addRect(QRectF(-10, -10, 20, 20),
                                            QPen(Qt::red), QBrush(Qt::blue));
    QCOMPARE(rect->pos(), QPointF());
    QCOMPARE(rect->pen(), QPen(Qt::red));
    QCOMPARE(rect->brush(), QBrush(Qt::blue));
    QCOMPARE(rect->rect(), QRectF(-10, -10, 20, 20));

    rect->setPen(QPen(Qt::red, 0));

    QCOMPARE(itemAt(scene, 0, 0),rect);
    QCOMPARE(itemAt(scene, -10, -10), rect);
    QCOMPARE(itemAt(scene, -9.9, 0), rect);
    QVERIFY(scene.items(QPointF(-10, 10)).isEmpty());
    QCOMPARE(itemAt(scene, 0, -9.9), rect);
    QCOMPARE(itemAt(scene, 0, 9.9), rect);
    QVERIFY(scene.items(QPointF(10, -10)).isEmpty());
    QCOMPARE(itemAt(scene, 9.9, 0), rect);
    QVERIFY(scene.items(QPointF(10, 10)).isEmpty());
}

void tst_QGraphicsScene::addText()
{
    QGraphicsScene scene;
    QGraphicsTextItem *text = scene.addText("Qt", QFont());
    QCOMPARE(text->pos(), QPointF());
    QCOMPARE(text->toPlainText(), QString("Qt"));
    QCOMPARE(text->font(), QFont());
}

void tst_QGraphicsScene::removeItem()
{
#if defined(Q_OS_ANDROID)
    QSKIP("No mouse cursor support");
#endif
    QGraphicsScene scene;
    QGraphicsItem *item = scene.addRect(QRectF(0, 0, 10, 10));
    QCOMPARE(itemAt(scene, 0, 0), item); // forces indexing
    scene.removeItem(item);
    QVERIFY(scene.items(QPointF(0, 0)).isEmpty());
    delete item;

    QGraphicsItem *item2 = scene.addRect(QRectF(0, 0, 10, 10));
    item2->setFlag(QGraphicsItem::ItemIsSelectable);
    QCOMPARE(itemAt(scene, 0, 0), item2);

    // Removing a selected item
    QVERIFY(scene.selectedItems().isEmpty());
    item2->setSelected(true);
    QVERIFY(scene.selectedItems().contains(item2));
    scene.removeItem(item2);
    QVERIFY(scene.selectedItems().isEmpty());

    // Check that we are in a state that can receive paint events
    // (i.e., not logged out on Windows).
    Q_CHECK_PAINTEVENTS

    // Removing a hovered item
    HoverItem *hoverItem = new HoverItem;
    scene.addItem(hoverItem);
    scene.setSceneRect(-50, -50, 100, 100);

    QGraphicsView view(&scene);
    view.setWindowTitle(QTest::currentTestFunction());
    view.setFixedSize(150, 150);
    view.show();
    QApplicationPrivate::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTest::mouseMove(view.windowHandle(), view.mapFromScene(hoverItem->scenePos() + QPointF(20, 20)));
    QTRY_VERIFY(!hoverItem->isHovered);

    QTest::mouseMove(view.windowHandle(), view.mapFromScene(hoverItem->scenePos()));
    QTRY_VERIFY(hoverItem->isHovered);

    scene.removeItem(hoverItem);
    hoverItem->setAcceptHoverEvents(false);
    scene.addItem(hoverItem);
    QTRY_VERIFY(!hoverItem->isHovered);
}

void tst_QGraphicsScene::focusItem()
{
    QGraphicsScene scene;
    QEvent activate(QEvent::WindowActivate);
    QApplication::sendEvent(&scene, &activate);

    QVERIFY(!scene.focusItem());
    QGraphicsItem *item = scene.addText("Qt");
    QVERIFY(!scene.focusItem());
    item->setFocus();
    QVERIFY(!scene.focusItem());
    item->setFlag(QGraphicsItem::ItemIsFocusable);
    QVERIFY(!scene.focusItem());
    item->setFocus();
    QCOMPARE(scene.focusItem(), item);

    QFocusEvent focusOut(QEvent::FocusOut);
    QApplication::sendEvent(&scene, &focusOut);

    QVERIFY(!scene.focusItem());

    QFocusEvent focusIn(QEvent::FocusIn);
    QApplication::sendEvent(&scene, &focusIn);
    QCOMPARE(scene.focusItem(), item);

    QGraphicsItem *item2 = scene.addText("Qt");
    item2->setFlag(QGraphicsItem::ItemIsFocusable);
    QCOMPARE(scene.focusItem(), item);

    item2->setFocus();
    QCOMPARE(scene.focusItem(), item2);
    item->setFocus();
    QCOMPARE(scene.focusItem(), item);

    item2->setFocus();
    QCOMPARE(scene.focusItem(), item2);
    QApplication::sendEvent(&scene, &focusOut);
    QVERIFY(!scene.hasFocus());
    QVERIFY(!scene.focusItem());
    QApplication::sendEvent(&scene, &focusIn);
    QCOMPARE(scene.focusItem(), item2);

    QApplication::sendEvent(&scene, &focusOut);

    QVERIFY(!scene.focusItem());
    scene.removeItem(item2);
    delete item2;

    QApplication::sendEvent(&scene, &focusIn);
    QVERIFY(!scene.focusItem());
}

class FocusItem : public QGraphicsTextItem
{
protected:
    void focusOutEvent(QFocusEvent *) override
    {
        QVERIFY(!scene()->focusItem());
    }
};

void tst_QGraphicsScene::focusItemLostFocus()
{
    QGraphicsScene scene;
    QEvent activate(QEvent::WindowActivate);
    QApplication::sendEvent(&scene, &activate);

    FocusItem *item = new FocusItem;
    item->setTextInteractionFlags(Qt::TextEditorInteraction);
    scene.addItem(item);

    item->setFocus();
    QCOMPARE(scene.focusItem(), item);
    item->clearFocus();
}

class ClearTestItem : public QGraphicsRectItem
{
public:
    using QGraphicsRectItem::QGraphicsRectItem;
    ~ClearTestItem() { qDeleteAll(items); }
    QList<QGraphicsItem *> items;
};

void tst_QGraphicsScene::clear()
{
    QGraphicsScene scene;
    scene.clear();
    QVERIFY(scene.items().isEmpty());
    scene.addRect(0, 0, 100, 100)->setPen(QPen(Qt::black, 0));
    QCOMPARE(scene.sceneRect(), QRectF(0, 0, 100, 100));
    scene.clear();
    QVERIFY(scene.items().isEmpty());
    QCOMPARE(scene.sceneRect(), QRectF(0, 0, 100, 100));

    ClearTestItem *firstItem = new ClearTestItem;
    QGraphicsItem *secondItem = new QGraphicsRectItem;
    firstItem->items += secondItem;

    scene.setItemIndexMethod(QGraphicsScene::NoIndex);
    scene.addItem(firstItem);
    scene.addItem(secondItem);
    QCOMPARE(scene.items().at(0), firstItem);
    QCOMPARE(scene.items().at(1), secondItem);

    ClearTestItem *thirdItem = new ClearTestItem(firstItem);
    QGraphicsItem *forthItem = new QGraphicsRectItem(firstItem);
    thirdItem->items += forthItem;

    // must not crash even if firstItem deletes secondItem
    scene.clear();
    QVERIFY(scene.items().isEmpty());
}

void tst_QGraphicsScene::setFocusItem()
{
    QGraphicsScene scene;
    QEvent activate(QEvent::WindowActivate);
    QApplication::sendEvent(&scene, &activate);

    QGraphicsItem *item = scene.addText("Qt");
    QVERIFY(!scene.focusItem());
    QVERIFY(!scene.hasFocus());
    scene.setFocusItem(item);
    QVERIFY(!scene.hasFocus());
    QVERIFY(!scene.focusItem());
    item->setFlag(QGraphicsItem::ItemIsFocusable);

    for (int i = 0; i < 3; ++i) {
        scene.setFocusItem(item);
        QVERIFY(scene.hasFocus());
        QCOMPARE(scene.focusItem(), item);
        QVERIFY(item->hasFocus());
    }

    QGraphicsItem *item2 = scene.addText("Qt");
    item2->setFlag(QGraphicsItem::ItemIsFocusable);

    scene.setFocusItem(item2);
    QVERIFY(!item->hasFocus());
    QVERIFY(item2->hasFocus());

    scene.setFocusItem(item);
    QVERIFY(item->hasFocus());
    QVERIFY(!item2->hasFocus());

    scene.clearFocus();
    QVERIFY(!item->hasFocus());
    QVERIFY(!item2->hasFocus());

    scene.setFocus();
    QVERIFY(item->hasFocus());
    QVERIFY(!item2->hasFocus());

    scene.setFocusItem(nullptr);
    QVERIFY(!item->hasFocus());
    QVERIFY(!item2->hasFocus());

    scene.setFocus();
    QVERIFY(!item->hasFocus());
    QVERIFY(!item2->hasFocus());
}

void tst_QGraphicsScene::setFocusItem_inactive()
{
    QGraphicsScene scene;
    QGraphicsItem *item = scene.addText("Qt");
    QVERIFY(!scene.focusItem());
    QVERIFY(!scene.hasFocus());
    scene.setFocusItem(item);
    QVERIFY(!scene.hasFocus());
    QVERIFY(!scene.focusItem());
    item->setFlag(QGraphicsItem::ItemIsFocusable);

    for (int i = 0; i < 3; ++i) {
        scene.setFocusItem(item);
        QCOMPARE(scene.focusItem(), item);
        QVERIFY(!item->hasFocus());
    }

}


void tst_QGraphicsScene::mouseGrabberItem()
{
    QGraphicsScene scene;
    QVERIFY(!scene.mouseGrabberItem());

    QGraphicsItem *item = scene.addRect(QRectF(-10, -10, 20, 20));
    item->setFlag(QGraphicsItem::ItemIsMovable);
    item->setZValue(1);

    QGraphicsItem *item2 = scene.addRect(QRectF(-10, -10, 20, 20));
    item2->setFlag(QGraphicsItem::ItemIsMovable);
    item2->setZValue(0);

    for (int i = 0; i < 3; ++i) {
        item->setPos(0, 0);
        item2->setPos(0, 0);
        item->setZValue((i & 1) ? 0 : 1);
        item2->setZValue((i & 1) ? 1 : 0);
        QGraphicsItem *topMostItem = (i & 1) ? item2 : item;

        QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
        pressEvent.setButton(Qt::LeftButton);
        pressEvent.setScenePos(QPointF(0, 0));
        pressEvent.setScreenPos(QPoint(100, 100));

        QApplication::sendEvent(&scene, &pressEvent);
        QCOMPARE(scene.mouseGrabberItem(), topMostItem);

        for (int i = 0; i < 1000; ++i) {
            QGraphicsSceneMouseEvent moveEvent(QEvent::GraphicsSceneMouseMove);
            moveEvent.setButtons(Qt::LeftButton);
            moveEvent.setScenePos(QPointF(i * 10, i * 10));
            moveEvent.setScreenPos(QPoint(100 + i * 10, 100 + i * 10));
            QApplication::sendEvent(&scene, &moveEvent);
            QCOMPARE(scene.mouseGrabberItem(), topMostItem);

            // Geometrical changes should not affect the mouse grabber.
            item->setZValue(QRandomGenerator::global()->bounded(500));
            item2->setZValue(QRandomGenerator::global()->bounded(500));
            item->setPos(QRandomGenerator::global()->bounded(50000), QRandomGenerator::global()->bounded(50000));
            item2->setPos(QRandomGenerator::global()->bounded(50000), QRandomGenerator::global()->bounded(50000));
        }

        QGraphicsSceneMouseEvent releaseEvent(QEvent::GraphicsSceneMouseRelease);
        releaseEvent.setScenePos(QPointF(10000, 10000));
        releaseEvent.setScreenPos(QPoint(1000000, 1000000));
        QApplication::sendEvent(&scene, &releaseEvent);
        QVERIFY(!scene.mouseGrabberItem());
    }

    // Structural change: deleting the mouse grabber
    item->setPos(0, 0);
    item->setZValue(1);
    item2->setPos(0, 0);
    item2->setZValue(0);
    QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
    pressEvent.setButton(Qt::LeftButton);
    pressEvent.setScenePos(QPointF(0, 0));
    pressEvent.setScreenPos(QPoint(100, 100));

    QGraphicsSceneMouseEvent moveEvent(QEvent::GraphicsSceneMouseMove);
    moveEvent.setButtons(Qt::LeftButton);
    moveEvent.setScenePos(QPointF(0, 0));
    moveEvent.setScreenPos(QPoint(100, 100));

    QApplication::sendEvent(&scene, &pressEvent);
    QApplication::sendEvent(&scene, &moveEvent);
    QCOMPARE(scene.mouseGrabberItem(), item);
    item->setVisible(false);
    QCOMPARE(scene.mouseGrabberItem(), nullptr);
    QApplication::sendEvent(&scene, &pressEvent);
    QCOMPARE(scene.mouseGrabberItem(), item2);
    item2->setVisible(false);
    QCOMPARE(scene.mouseGrabberItem(), nullptr);
    QApplication::sendEvent(&scene, &moveEvent);
    QCOMPARE(scene.mouseGrabberItem(), nullptr);
    item2->setVisible(true);
    QApplication::sendEvent(&scene, &moveEvent);
    QCOMPARE(scene.mouseGrabberItem(), nullptr);
    QApplication::sendEvent(&scene, &pressEvent);
    QApplication::sendEvent(&scene, &moveEvent);
    QCOMPARE(scene.mouseGrabberItem(), item2);
    scene.removeItem(item2);
    delete item2;
    QCOMPARE(scene.mouseGrabberItem(), nullptr);
}

void tst_QGraphicsScene::hoverEvents_siblings()
{
    Q_CHECK_PAINTEVENTS

    QGraphicsScene scene;
    QGraphicsItem *lastItem = nullptr;
    QList<HoverItem *> items;
    for (int i = 0; i < 15; ++i) {
        auto item = new HoverItem;
        scene.addItem(item);
        items << item;
        if (lastItem)
            item->setPos(lastItem->pos() + QPointF(sin(i / 3.0) * 17, cos(i / 3.0) * 17));
        item->setZValue(i);
        lastItem = item;
    }

    QGraphicsView view(&scene);
    view.setWindowTitle(QTest::currentTestFunction());
    view.setRenderHint(QPainter::Antialiasing, true);
    view.setMinimumSize(400, 300);
    view.rotate(10);
    view.scale(1.7, 1.7);
    view.show();
    QApplicationPrivate::setActiveWindow(&view);
    view.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QCursor::setPos(view.mapToGlobal(QPoint(-5, -5)));

    QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseMove);
    mouseEvent.setScenePos(QPointF(-1000, -1000));
    QApplication::sendEvent(&scene, &mouseEvent);

    QTest::qWait(50);

    for (int j = 1; j >= 0; --j) {
        int i = j ? 0 : 14;
        forever {
            if (j)
                QVERIFY(!items.at(i)->isHovered);
            else
                QVERIFY(!items.at(i)->isHovered);
            QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseMove);
            mouseEvent.setScenePos(items.at(i)->mapToScene(0, 0));
            QApplication::sendEvent(&scene, &mouseEvent);

            QCoreApplication::processEvents(); // this posts updates from the scene to the view
            QCoreApplication::processEvents(); // which trigger a repaint here

            QTRY_VERIFY(items.at(i)->isHovered);
            if (j && i > 0)
                QVERIFY(!items.at(i - 1)->isHovered);
            if (!j && i < 14)
                QVERIFY(!items.at(i + 1)->isHovered);
            i += j ? 1 : -1;
            if ((j && i == 15) || (!j && i == -1))
                break;
        }

        QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseMove);
        mouseEvent.setScenePos(QPointF(-1000, -1000));
        QApplication::sendEvent(&scene, &mouseEvent);

        QCoreApplication::processEvents(); // this posts updates from the scene to the view
        QCoreApplication::processEvents(); // which trigger a repaint here
    }
}

void tst_QGraphicsScene::hoverEvents_parentChild()
{
    Q_CHECK_PAINTEVENTS

    QGraphicsScene scene;
    QGraphicsItem *lastItem = nullptr;
    QList<HoverItem *> items;
    for (int i = 0; i < 15; ++i) {
        auto item = new HoverItem;
        scene.addItem(item);
        items << item;
        if (lastItem) {
            item->setParentItem(lastItem);
            item->setPos(sin(i / 3.0) * 17, cos(i / 3.0) * 17);
        }
        lastItem = item;
    }

    QGraphicsView view(&scene);
    view.setWindowTitle(QTest::currentTestFunction());
    view.setRenderHint(QPainter::Antialiasing, true);
    view.setMinimumSize(400, 300);
    view.rotate(10);
    view.scale(1.7, 1.7);
    view.show();
    QApplicationPrivate::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseMove);
    mouseEvent.setScenePos(QPointF(-1000, -1000));
    QApplication::sendEvent(&scene, &mouseEvent);

    for (int j = 1; j >= 0; --j) {
        int i = j ? 0 : 14;
        forever {
            if (j) {
                QVERIFY(!items.at(i)->isHovered);
            } else {
                if (i == 14)
                    QVERIFY(!items.at(13)->isHovered);
            }
            mouseEvent.setScenePos(items.at(i)->mapToScene(0, 0));
            QApplication::sendEvent(&scene, &mouseEvent);

            QCoreApplication::processEvents(); // this posts updates from the scene to the view
            QCoreApplication::processEvents(); // which trigger a repaint here

            QTRY_VERIFY(items.at(i)->isHovered);
            if (i < 14)
                QVERIFY(!items.at(i + 1)->isHovered);
            i += j ? 1 : -1;
            if ((j && i == 15) || (!j && i == -1))
                break;
        }

        mouseEvent.setScenePos(QPointF(-1000, -1000));
        QApplication::sendEvent(&scene, &mouseEvent);

        QCoreApplication::processEvents(); // this posts updates from the scene to the view
        QCoreApplication::processEvents(); // which trigger a repaint here
    }
}

void tst_QGraphicsScene::createItemGroup()
{
    QGraphicsScene scene;

    QList<QGraphicsItem *> children1;
    children1 << scene.addRect(QRectF(-10, -10, 20, 20));
    children1 << scene.addRect(QRectF(-10, -10, 20, 20));
    children1 << scene.addRect(QRectF(-10, -10, 20, 20));
    children1 << scene.addRect(QRectF(-10, -10, 20, 20));

    QList<QGraphicsItem *> children2;
    children2 << scene.addRect(QRectF(-10, -10, 20, 20));
    children2 << scene.addRect(QRectF(-10, -10, 20, 20));
    children2 << scene.addRect(QRectF(-10, -10, 20, 20));
    children2 << scene.addRect(QRectF(-10, -10, 20, 20));

    QList<QGraphicsItem *> children3;
    children3 << scene.addRect(QRectF(-10, -10, 20, 20));
    children3 << scene.addRect(QRectF(-10, -10, 20, 20));
    children3 << scene.addRect(QRectF(-10, -10, 20, 20));
    children3 << scene.addRect(QRectF(-10, -10, 20, 20));

    // All items in children1 are children of parent1
    QGraphicsItem *parent1 = scene.addRect(QRectF(-10, -10, 20, 20));
    for (QGraphicsItem *item : std::as_const(children1))
        item->setParentItem(parent1);

    QGraphicsItemGroup *group = scene.createItemGroup(children1);
    QCOMPARE(group->parentItem(), parent1);
    QCOMPARE(children1.first()->parentItem(), group);
    scene.destroyItemGroup(group);
    QCOMPARE(children1.first()->parentItem(), parent1);
    group = scene.createItemGroup(children1);
    QCOMPARE(group->parentItem(), parent1);
    QCOMPARE(children1.first()->parentItem(), group);
    scene.destroyItemGroup(group);
    QCOMPARE(children1.first()->parentItem(), parent1);

    // All items in children2 are children of parent2
    QGraphicsItem *parent2 = scene.addRect(QRectF(-10, -10, 20, 20));
    for (QGraphicsItem *item : std::as_const(children2))
        item->setParentItem(parent2);

    // Now make parent2 a child of parent1, so all children2 are also children
    // of parent1.
    parent2->setParentItem(parent1);

    // The children2 group should still have parent2 as their common ancestor.
    group = scene.createItemGroup(children2);
    QCOMPARE(group->parentItem(), parent2);
    QCOMPARE(children2.first()->parentItem(), group);
    scene.destroyItemGroup(group);
    QCOMPARE(children2.first()->parentItem(), parent2);

    // But the set of both children2 and children1 share only parent1.
    group = scene.createItemGroup(children2 + children1);
    QCOMPARE(group->parentItem(), parent1);
    QCOMPARE(children1.first()->parentItem(), group);
    QCOMPARE(children2.first()->parentItem(), group);
    scene.destroyItemGroup(group);
    QCOMPARE(children1.first()->parentItem(), parent1);
    QCOMPARE(children2.first()->parentItem(), parent1);

    // Fixup the parent-child chain
    for (QGraphicsItem *item : std::as_const(children2))
        item->setParentItem(parent2);

    // These share no common parent
    group = scene.createItemGroup(children3);
    QCOMPARE(group->parentItem(), nullptr);
    scene.destroyItemGroup(group);

    // Make children3 children of parent3
    QGraphicsItem *parent3 = scene.addRect(QRectF(-10, -10, 20, 20));
    for (QGraphicsItem *item : std::as_const(children3))
        item->setParentItem(parent3);

    // These should have parent3 as a parent
    group = scene.createItemGroup(children3);
    QCOMPARE(group->parentItem(), parent3);
    scene.destroyItemGroup(group);

    // Now make them all children of parent1
    parent3->setParentItem(parent1);

    group = scene.createItemGroup(children3);
    QCOMPARE(group->parentItem(), parent3);
    scene.destroyItemGroup(group);

    group = scene.createItemGroup(children2);
    QCOMPARE(group->parentItem(), parent2);
    scene.destroyItemGroup(group);

    group = scene.createItemGroup(children1);
    QCOMPARE(group->parentItem(), parent1);
    scene.destroyItemGroup(group);

    QGraphicsItemGroup *emptyGroup = scene.createItemGroup(QList<QGraphicsItem *>());
    QVERIFY(emptyGroup->childItems().isEmpty());
    QVERIFY(!emptyGroup->parentItem());
    QCOMPARE(emptyGroup->scene(), &scene);
}

class EventTester : public QGraphicsEllipseItem
{
public:
    EventTester() : QGraphicsEllipseItem(QRectF(-10, -10, 20, 20))
    { }

    bool ignoreMouse = false;
    QList<QEvent::Type> eventTypes;

protected:
    bool sceneEvent(QEvent *event) override
    {
        eventTypes << QEvent::Type(event->type());
        switch (event->type()) {
        case QEvent::GraphicsSceneMousePress:
        case QEvent::GraphicsSceneMouseMove:
        case QEvent::GraphicsSceneMouseRelease:
            if (ignoreMouse) {
                event->ignore();
                return true;
            }
            break;
        default:
            break;
        }

        return QGraphicsEllipseItem::sceneEvent(event);
    }
};

void tst_QGraphicsScene::mouseEventPropagation()
{
    EventTester *a = new EventTester;
    EventTester *b = new EventTester;
    EventTester *c = new EventTester;
    EventTester *d = new EventTester;
    b->setParentItem(a);
    c->setParentItem(b);
    d->setParentItem(c);

    a->setFlag(QGraphicsItem::ItemIsMovable);
    b->setFlag(QGraphicsItem::ItemIsMovable);
    c->setFlag(QGraphicsItem::ItemIsMovable);
    d->setFlag(QGraphicsItem::ItemIsMovable);

    a->setData(0, "A");
    b->setData(0, "B");
    c->setData(0, "C");
    d->setData(0, "D");

    // scene -> a -> b -> c -> d
    QGraphicsScene scene;
    QEvent activate(QEvent::WindowActivate);
    QApplication::sendEvent(&scene, &activate);

    scene.addItem(a);

    // Prepare some events
    QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
    pressEvent.setButton(Qt::LeftButton);
    pressEvent.setScenePos(QPointF(0, 0));
    QGraphicsSceneMouseEvent moveEvent(QEvent::GraphicsSceneMouseMove);
    moveEvent.setButton(Qt::LeftButton);
    moveEvent.setScenePos(QPointF(0, 0));
    QGraphicsSceneMouseEvent releaseEvent(QEvent::GraphicsSceneMouseRelease);
    releaseEvent.setButton(Qt::LeftButton);
    releaseEvent.setScenePos(QPointF(0, 0));

    // Send a press
    QApplication::sendEvent(&scene, &pressEvent);
    QCOMPARE(d->eventTypes.size(), 2);
    QCOMPARE(d->eventTypes.at(0), QEvent::GrabMouse);
    QCOMPARE(d->eventTypes.at(1), QEvent::GraphicsSceneMousePress);
    QCOMPARE(c->eventTypes.size(), 0);
    QCOMPARE(b->eventTypes.size(), 0);
    QCOMPARE(a->eventTypes.size(), 0);
    QCOMPARE(scene.mouseGrabberItem(), d);

    // Send a move
    QApplication::sendEvent(&scene, &moveEvent);
    QCOMPARE(d->eventTypes.size(), 3);
    QCOMPARE(d->eventTypes.at(2), QEvent::GraphicsSceneMouseMove);
    QCOMPARE(c->eventTypes.size(), 0);
    QCOMPARE(b->eventTypes.size(), 0);
    QCOMPARE(a->eventTypes.size(), 0);
    QCOMPARE(scene.mouseGrabberItem(), d);

    // Send a release
    QApplication::sendEvent(&scene, &releaseEvent);
    QCOMPARE(d->eventTypes.size(), 5);
    QCOMPARE(d->eventTypes.at(3), QEvent::GraphicsSceneMouseRelease);
    QCOMPARE(d->eventTypes.at(4), QEvent::UngrabMouse);
    QCOMPARE(c->eventTypes.size(), 0);
    QCOMPARE(b->eventTypes.size(), 0);
    QCOMPARE(a->eventTypes.size(), 0);
    QCOMPARE(scene.mouseGrabberItem(), nullptr);

    d->setAcceptedMouseButtons(Qt::RightButton);

    // Send a press
    QApplication::sendEvent(&scene, &pressEvent);
    QCOMPARE(d->eventTypes.size(), 5);
    QCOMPARE(c->eventTypes.size(), 2);
    QCOMPARE(c->eventTypes.at(0), QEvent::GrabMouse);
    QCOMPARE(c->eventTypes.at(1), QEvent::GraphicsSceneMousePress);
    QCOMPARE(b->eventTypes.size(), 0);
    QCOMPARE(a->eventTypes.size(), 0);
    QCOMPARE(scene.mouseGrabberItem(), c);

    // Send another press, with a button that isn't actually accepted
    QApplication::sendEvent(&scene, &pressEvent);
    pressEvent.setButton(Qt::RightButton);
    QCOMPARE(d->eventTypes.size(), 5);
    QCOMPARE(c->eventTypes.size(), 3);
    QCOMPARE(c->eventTypes.at(2), QEvent::GraphicsSceneMousePress);
    QCOMPARE(b->eventTypes.size(), 0);
    QCOMPARE(a->eventTypes.size(), 0);
    QCOMPARE(scene.mouseGrabberItem(), c);

    // Send a move
    QApplication::sendEvent(&scene, &moveEvent);
    QCOMPARE(d->eventTypes.size(), 5);
    QCOMPARE(c->eventTypes.size(), 4);
    QCOMPARE(c->eventTypes.at(3), QEvent::GraphicsSceneMouseMove);
    QCOMPARE(b->eventTypes.size(), 0);
    QCOMPARE(a->eventTypes.size(), 0);
    QCOMPARE(scene.mouseGrabberItem(), c);

    // Send a release
    QApplication::sendEvent(&scene, &releaseEvent);
    QCOMPARE(d->eventTypes.size(), 5);
    QCOMPARE(c->eventTypes.size(), 6);
    QCOMPARE(c->eventTypes.at(4), QEvent::GraphicsSceneMouseRelease);
    QCOMPARE(c->eventTypes.at(5), QEvent::UngrabMouse);
    QCOMPARE(b->eventTypes.size(), 0);
    QCOMPARE(a->eventTypes.size(), 0);
    QCOMPARE(scene.mouseGrabberItem(), nullptr);

    // Disabled items eat events. c should not get this.
    d->setEnabled(false);
    d->setAcceptedMouseButtons(Qt::RightButton);

    // Send a right press. This disappears in d.
    QApplication::sendEvent(&scene, &pressEvent);
    QCOMPARE(d->eventTypes.size(), 5);
    QCOMPARE(c->eventTypes.size(), 6);
    QCOMPARE(b->eventTypes.size(), 0);
    QCOMPARE(a->eventTypes.size(), 0);
    QCOMPARE(scene.mouseGrabberItem(), nullptr);

    // Send a left press. This goes to c.
    pressEvent.setButton(Qt::LeftButton);
    QApplication::sendEvent(&scene, &pressEvent);
    QCOMPARE(d->eventTypes.size(), 5);
    QCOMPARE(c->eventTypes.size(), 8);
    QCOMPARE(c->eventTypes.at(6), QEvent::GrabMouse);
    QCOMPARE(c->eventTypes.at(7), QEvent::GraphicsSceneMousePress);
    QCOMPARE(b->eventTypes.size(), 0);
    QCOMPARE(a->eventTypes.size(), 0);
    QCOMPARE(scene.mouseGrabberItem(), c);

    // Clicking outside the items removes the mouse grabber
}

void tst_QGraphicsScene::mouseEventPropagation_ignore()
{
    EventTester *a = new EventTester;
    EventTester *b = new EventTester;
    EventTester *c = new EventTester;
    EventTester *d = new EventTester;
    b->setParentItem(a);
    c->setParentItem(b);
    d->setParentItem(c);

    a->setFlags(QGraphicsItem::ItemIsMovable);
    b->setFlags(QGraphicsItem::ItemIsMovable);
    c->setFlags(QGraphicsItem::ItemIsMovable);
    d->setFlags(QGraphicsItem::ItemIsMovable);

    // scene -> a -> b -> c -> d
    QGraphicsScene scene;
    scene.addItem(a);

    // Prepare some events
    QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
    pressEvent.setButton(Qt::LeftButton);
    pressEvent.setScenePos(QPointF(0, 0));

    b->ignoreMouse = true;
    c->ignoreMouse = true;
    d->ignoreMouse = true;

    QApplication::sendEvent(&scene, &pressEvent);
    QCOMPARE(a->eventTypes.size(), 2);
    QCOMPARE(a->eventTypes.at(0), QEvent::GrabMouse);
    QCOMPARE(a->eventTypes.at(1), QEvent::GraphicsSceneMousePress);
    QCOMPARE(b->eventTypes.size(), 3);
    QCOMPARE(b->eventTypes.at(0), QEvent::GrabMouse);
    QCOMPARE(b->eventTypes.at(1), QEvent::GraphicsSceneMousePress);
    QCOMPARE(b->eventTypes.at(2), QEvent::UngrabMouse);
    QCOMPARE(c->eventTypes.size(), 3);
    QCOMPARE(c->eventTypes.at(0), QEvent::GrabMouse);
    QCOMPARE(c->eventTypes.at(1), QEvent::GraphicsSceneMousePress);
    QCOMPARE(c->eventTypes.at(2), QEvent::UngrabMouse);
    QCOMPARE(d->eventTypes.size(), 3);
    QCOMPARE(d->eventTypes.at(0), QEvent::GrabMouse);
    QCOMPARE(d->eventTypes.at(1), QEvent::GraphicsSceneMousePress);
    QCOMPARE(d->eventTypes.at(2), QEvent::UngrabMouse);
    QCOMPARE(scene.mouseGrabberItem(), a);

    a->ignoreMouse = true;

    QApplication::sendEvent(&scene, &pressEvent);
    QCOMPARE(a->eventTypes.size(), 3);
    QCOMPARE(a->eventTypes.at(2), QEvent::GraphicsSceneMousePress);
    QCOMPARE(b->eventTypes.size(), 3);
    QCOMPARE(c->eventTypes.size(), 3);
    QCOMPARE(d->eventTypes.size(), 3);

    QVERIFY(!pressEvent.isAccepted());
}

void tst_QGraphicsScene::mouseEventPropagation_focus()
{
    EventTester *a = new EventTester;
    EventTester *b = new EventTester;
    EventTester *c = new EventTester;
    EventTester *d = new EventTester;
    b->setParentItem(a);
    c->setParentItem(b);
    d->setParentItem(c);

    a->setFlag(QGraphicsItem::ItemIsMovable);
    b->setFlag(QGraphicsItem::ItemIsMovable);
    c->setFlag(QGraphicsItem::ItemIsMovable);
    d->setFlag(QGraphicsItem::ItemIsMovable);

    // scene -> a -> b -> c -> d
    QGraphicsScene scene;
    QEvent activate(QEvent::WindowActivate);
    QApplication::sendEvent(&scene, &activate);

    scene.addItem(a);

    // Prepare some events
    QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
    pressEvent.setButton(Qt::LeftButton);
    pressEvent.setScenePos(QPointF(0, 0));

    a->setFlag(QGraphicsItem::ItemIsFocusable);
    QVERIFY(!a->hasFocus());

    QApplication::sendEvent(&scene, &pressEvent);

    QVERIFY(a->hasFocus());
    QCOMPARE(a->eventTypes.size(), 1);
    QCOMPARE(a->eventTypes.first(), QEvent::FocusIn);
    QCOMPARE(d->eventTypes.size(), 2);
    QCOMPARE(d->eventTypes.at(0), QEvent::GrabMouse);
    QCOMPARE(d->eventTypes.at(1), QEvent::GraphicsSceneMousePress);
}

void tst_QGraphicsScene::mouseEventPropagation_doubleclick()
{
    EventTester *a = new EventTester;
    EventTester *b = new EventTester;
    a->setFlags(QGraphicsItem::ItemIsMovable);
    b->setFlags(QGraphicsItem::ItemIsMovable);

    a->setPos(-50, 0);
    b->setPos(50, 0);

    QGraphicsScene scene;
    scene.addItem(a);
    scene.addItem(b);

    // Prepare some events
    QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
    pressEvent.setButton(Qt::LeftButton);
    pressEvent.setScenePos(QPointF(0, 0));
    QGraphicsSceneMouseEvent doubleClickEvent(QEvent::GraphicsSceneMouseDoubleClick);
    doubleClickEvent.setButton(Qt::LeftButton);
    doubleClickEvent.setScenePos(QPointF(0, 0));
    QGraphicsSceneMouseEvent releaseEvent(QEvent::GraphicsSceneMouseRelease);
    releaseEvent.setButton(Qt::LeftButton);
    releaseEvent.setScenePos(QPointF(0, 0));

    // Send press to A
    pressEvent.setScenePos(a->mapToScene(0, 0));
    QApplication::sendEvent(&scene, &pressEvent);
    QCOMPARE(a->eventTypes.size(), 2);
    QCOMPARE(a->eventTypes.at(0), QEvent::GrabMouse);
    QCOMPARE(a->eventTypes.at(1), QEvent::GraphicsSceneMousePress);

    // Send release to A
    releaseEvent.setScenePos(a->mapToScene(0, 0));
    QApplication::sendEvent(&scene, &releaseEvent);
    QCOMPARE(a->eventTypes.size(), 4);
    QCOMPARE(a->eventTypes.at(2), QEvent::GraphicsSceneMouseRelease);
    QCOMPARE(a->eventTypes.at(3), QEvent::UngrabMouse);

    // Send doubleclick to B
    doubleClickEvent.setScenePos(b->mapToScene(0, 0));
    QApplication::sendEvent(&scene, &doubleClickEvent);
    QCOMPARE(a->eventTypes.size(), 4);
    QCOMPARE(b->eventTypes.size(), 2);
    QCOMPARE(b->eventTypes.at(0), QEvent::GrabMouse);
    QCOMPARE(b->eventTypes.at(1), QEvent::GraphicsSceneMousePress);

    // Send release to B
    releaseEvent.setScenePos(b->mapToScene(0, 0));
    QApplication::sendEvent(&scene, &releaseEvent);
    QCOMPARE(a->eventTypes.size(), 4);
    QCOMPARE(b->eventTypes.size(), 4);
    QCOMPARE(b->eventTypes.at(2), QEvent::GraphicsSceneMouseRelease);
    QCOMPARE(b->eventTypes.at(3), QEvent::UngrabMouse);
}

class Scene : public QGraphicsScene
{
public:
    QList<QPointF> mouseMovePoints;

protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override
    {
        mouseMovePoints << event->scenePos();
    }
};

void tst_QGraphicsScene::mouseEventPropagation_mouseMove()
{
    Scene scene;
    scene.addRect(QRectF(5, 0, 12, 12));
    scene.addRect(QRectF(15, 0, 12, 12))->setAcceptHoverEvents(true);
    for (int i = 0; i < 30; ++i) {
        QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseMove);
        event.setScenePos(QPointF(i, 5));
        QApplication::sendEvent(&scene, &event);
    }

    QCOMPARE(scene.mouseMovePoints.size(), 30);
    for (int i = 0; i < 30; ++i)
        QCOMPARE(scene.mouseMovePoints.at(i), QPointF(i, 5));
}

class DndTester : public QGraphicsEllipseItem
{
public:
    using QGraphicsEllipseItem::QGraphicsEllipseItem;

    ~DndTester()
    {
        delete lastEvent;
    }

    QGraphicsSceneDragDropEvent *lastEvent = nullptr;
    QList<QEvent::Type> eventList;
    bool ignoresDragEnter = false;
    bool ignoresDragMove = false;

protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override
    {
        storeLastEvent(event);
        event->setAccepted(!ignoresDragEnter);
        if (!ignoresDragEnter)
            event->setDropAction(Qt::IgnoreAction);
        eventList << event->type();
    }

    void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override
    {
        storeLastEvent(event);
        event->setAccepted(!ignoresDragMove);
        eventList << event->type();
    }

    void dragLeaveEvent(QGraphicsSceneDragDropEvent *event) override
    {
        storeLastEvent(event);
        eventList << event->type();
    }

    void dropEvent(QGraphicsSceneDragDropEvent *event) override
    {
        storeLastEvent(event);
        eventList << event->type();
    }

private:
    void storeLastEvent(QGraphicsSceneDragDropEvent *event)
    {
        delete lastEvent;
        lastEvent = new QGraphicsSceneDragDropEvent(event->type());
        lastEvent->setScenePos(event->scenePos());
        lastEvent->setScreenPos(event->screenPos());
        lastEvent->setButtons(event->buttons());
        lastEvent->setModifiers(event->modifiers());
        lastEvent->setPossibleActions(event->possibleActions());
        lastEvent->setProposedAction(event->proposedAction());
        lastEvent->setDropAction(event->dropAction());
        lastEvent->setMimeData(event->mimeData());
        lastEvent->setWidget(event->widget());
        lastEvent->setSource(event->source());
    }
};

#if QT_CONFIG(draganddrop)
void tst_QGraphicsScene::dragAndDrop_simple()
{
    DndTester *item = new DndTester(QRectF(-10, -10, 20, 20));

    QGraphicsScene scene;
    scene.addItem(item);

    QGraphicsView view(&scene);
    view.setFixedSize(100, 100);

    QMimeData mimeData;

    // Initial drag enter for the scene
    QDragEnterEvent dragEnter(QPoint(0, 0), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
    QApplication::sendEvent(view.viewport(), &dragEnter);
    QVERIFY(dragEnter.isAccepted());
    QCOMPARE(dragEnter.dropAction(), Qt::CopyAction);

    {
        // Move outside the item
        QDragMoveEvent dragMove(QPoint(0, 0), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
        QApplication::sendEvent(view.viewport(), &dragMove);
        QVERIFY(!dragMove.isAccepted());
        QCOMPARE(dragMove.dropAction(), Qt::CopyAction);
    }
    {
        // Move inside the item without setAcceptDrops
        QDragMoveEvent dragMove(view.mapFromScene(item->scenePos()), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
        QApplication::sendEvent(view.viewport(), &dragMove);
        QVERIFY(!dragMove.isAccepted());
        QCOMPARE(dragMove.dropAction(), Qt::CopyAction);
        QCOMPARE(item->eventList.size(), 0);
    }
    item->setAcceptDrops(true);
    {
        // Move inside the item with setAcceptDrops
        QDragMoveEvent dragMove(view.mapFromScene(item->scenePos()), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
        QApplication::sendEvent(view.viewport(), &dragMove);
        QVERIFY(dragMove.isAccepted());
        QCOMPARE(dragMove.dropAction(), Qt::IgnoreAction);
        QCOMPARE(item->eventList.size(), 2);
        QCOMPARE(item->eventList.at(0), QEvent::GraphicsSceneDragEnter);
        QCOMPARE(item->eventList.at(1), QEvent::GraphicsSceneDragMove);
        QCOMPARE(item->lastEvent->screenPos(), view.mapToGlobal(dragMove.position().toPoint()));
        QCOMPARE(item->lastEvent->scenePos(), view.mapToScene(dragMove.position().toPoint()));
        QVERIFY(item->lastEvent->isAccepted());
        QCOMPARE(item->lastEvent->dropAction(), Qt::IgnoreAction);
    }
    {
        // Another move inside the item
        QDragMoveEvent dragMove(view.mapFromScene(item->mapToScene(5, 5)), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
        QApplication::sendEvent(view.viewport(), &dragMove);
        QVERIFY(dragMove.isAccepted());
        QCOMPARE(dragMove.dropAction(), Qt::IgnoreAction);
        QCOMPARE(item->eventList.size(), 3);
        QCOMPARE(item->eventList.at(2), QEvent::GraphicsSceneDragMove);
        QCOMPARE(item->lastEvent->screenPos(), view.mapToGlobal(dragMove.position().toPoint()));
        QCOMPARE(item->lastEvent->scenePos(), view.mapToScene(dragMove.position().toPoint()));
        QVERIFY(item->lastEvent->isAccepted());
        QCOMPARE(item->lastEvent->dropAction(), Qt::IgnoreAction);
    }
    {
        // Move outside the item
        QDragMoveEvent dragMove(QPoint(0, 0), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
        QApplication::sendEvent(view.viewport(), &dragMove);
        QVERIFY(!dragMove.isAccepted());
        QCOMPARE(dragMove.dropAction(), Qt::CopyAction);
        QCOMPARE(item->eventList.size(), 4);
        QCOMPARE(item->eventList.at(3), QEvent::GraphicsSceneDragLeave);
        QCOMPARE(item->lastEvent->screenPos(), view.mapToGlobal(dragMove.position().toPoint()));
        QCOMPARE(item->lastEvent->scenePos(), view.mapToScene(dragMove.position().toPoint()));
        QVERIFY(item->lastEvent->isAccepted());
        QCOMPARE(item->lastEvent->dropAction(), Qt::CopyAction);
    }
    {
        // Move inside the item again
        QDragMoveEvent dragMove(view.mapFromScene(item->scenePos()), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
        QApplication::sendEvent(view.viewport(), &dragMove);
        QVERIFY(dragMove.isAccepted());
        QCOMPARE(dragMove.dropAction(), Qt::IgnoreAction);
        QCOMPARE(item->eventList.size(), 6);
        QCOMPARE(item->eventList.at(4), QEvent::GraphicsSceneDragEnter);
        QCOMPARE(item->eventList.at(5), QEvent::GraphicsSceneDragMove);
        QCOMPARE(item->lastEvent->screenPos(), view.mapToGlobal(dragMove.position().toPoint()));
        QCOMPARE(item->lastEvent->scenePos(), view.mapToScene(dragMove.position().toPoint()));
        QVERIFY(item->lastEvent->isAccepted());
        QCOMPARE(item->lastEvent->dropAction(), Qt::IgnoreAction);
    }
    {
        // Drop inside the item
        QDropEvent drop(view.mapFromScene(item->scenePos()), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
        QApplication::sendEvent(view.viewport(), &drop);
        QVERIFY(drop.isAccepted());
        QCOMPARE(drop.dropAction(), Qt::CopyAction);
        QCOMPARE(item->eventList.size(), 7);
        QCOMPARE(item->eventList.at(6), QEvent::GraphicsSceneDrop);
        QCOMPARE(item->lastEvent->screenPos(), view.mapToGlobal(drop.position().toPoint()));
        QCOMPARE(item->lastEvent->scenePos(), view.mapToScene(drop.position().toPoint()));
        QVERIFY(item->lastEvent->isAccepted());
        QCOMPARE(item->lastEvent->dropAction(), Qt::CopyAction);
    }
}

void tst_QGraphicsScene::dragAndDrop_disabledOrInvisible()
{
    DndTester *item = new DndTester(QRectF(-10, -10, 20, 20));
    item->setAcceptDrops(true);

    QGraphicsScene scene;
    scene.addItem(item);

    QGraphicsView view(&scene);
    view.setFixedSize(100, 100);

    QMimeData mimeData;

    // Initial drag enter for the scene
    QDragEnterEvent dragEnter(QPoint(0, 0), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
    QApplication::sendEvent(view.viewport(), &dragEnter);
    QVERIFY(dragEnter.isAccepted());
    QCOMPARE(dragEnter.dropAction(), Qt::CopyAction);
    {
        // Move inside the item
        QDragMoveEvent dragMove(view.mapFromScene(item->scenePos()), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
        QApplication::sendEvent(view.viewport(), &dragMove);
        QVERIFY(dragMove.isAccepted());
        QCOMPARE(dragMove.dropAction(), Qt::IgnoreAction);
        QCOMPARE(item->eventList.size(), 2);
        QCOMPARE(item->eventList.at(0), QEvent::GraphicsSceneDragEnter);
        QCOMPARE(item->eventList.at(1), QEvent::GraphicsSceneDragMove);
    }
    {
        // Move outside the item
        QDragMoveEvent dragMove(QPoint(0, 0), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
        QApplication::sendEvent(view.viewport(), &dragMove);
        QVERIFY(!dragMove.isAccepted());
        QCOMPARE(dragMove.dropAction(), Qt::CopyAction);
        QCOMPARE(item->eventList.size(), 3);
        QCOMPARE(item->eventList.at(2), QEvent::GraphicsSceneDragLeave);
    }

    // Now disable the item
    item->setEnabled(false);
    QVERIFY(!item->isEnabled());
    QVERIFY(item->isVisible());

    {
        // Move inside the item
        QDragMoveEvent dragMove(view.mapFromScene(item->scenePos()), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
        QApplication::sendEvent(view.viewport(), &dragMove);
        QVERIFY(!dragMove.isAccepted());
        QCOMPARE(dragMove.dropAction(), Qt::CopyAction);
        QCOMPARE(item->eventList.size(), 3);
        QCOMPARE(item->eventList.at(2), QEvent::GraphicsSceneDragLeave);
    }

    // Reenable it, and make it invisible
    item->setEnabled(true);
    item->setVisible(false);
    QVERIFY(item->isEnabled());
    QVERIFY(!item->isVisible());

    {
        // Move inside the item
        QDragMoveEvent dragMove(view.mapFromScene(item->scenePos()), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
        QApplication::sendEvent(view.viewport(), &dragMove);
        QVERIFY(!dragMove.isAccepted());
        QCOMPARE(dragMove.dropAction(), Qt::CopyAction);
        QCOMPARE(item->eventList.size(), 3);
        QCOMPARE(item->eventList.at(2), QEvent::GraphicsSceneDragLeave);
    }

    // Dummy drop event to keep the Mac from crashing.
    QDropEvent dropEvent(QPoint(0, 0), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
    QApplication::sendEvent(view.viewport(), &dropEvent);
}

void tst_QGraphicsScene::dragAndDrop_propagate()
{
    DndTester *item1 = new DndTester(QRectF(-10, -10, 20, 20));
    DndTester *item2 = new DndTester(QRectF(0, 0, 20, 20));
    item1->setAcceptDrops(true);
    item2->setAcceptDrops(true);
    item2->ignoresDragMove = true;
    item2->ignoresDragEnter = false;
    item2->setZValue(1);

    item1->setData(0, "item1");
    item2->setData(0, "item2");

    QGraphicsScene scene;
    scene.addItem(item1);
    scene.addItem(item2);

    QGraphicsView view(&scene);
    view.setFixedSize(100, 100);

    QMimeData mimeData;

    // Initial drag enter for the scene
    QDragEnterEvent dragEnter(QPoint(0, 0), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
    QApplication::sendEvent(view.viewport(), &dragEnter);
    QVERIFY(dragEnter.isAccepted());
    QCOMPARE(dragEnter.dropAction(), Qt::CopyAction);

    {
        // Move outside the items
        QDragMoveEvent dragMove(QPoint(0, 0), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
        QApplication::sendEvent(view.viewport(), &dragMove);
        QVERIFY(!dragMove.isAccepted());
        QCOMPARE(dragMove.dropAction(), Qt::CopyAction);
        QVERIFY(item1->eventList.isEmpty());
        QVERIFY(item2->eventList.isEmpty());
    }
    {
        // Move inside item1
        QDragMoveEvent dragMove(view.mapFromScene(-5, -5), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
        QApplication::sendEvent(view.viewport(), &dragMove);
        QVERIFY(dragMove.isAccepted());
        QCOMPARE(dragMove.dropAction(), Qt::IgnoreAction);
        QCOMPARE(item1->eventList.size(), 2);
        QCOMPARE(item1->eventList.at(0), QEvent::GraphicsSceneDragEnter);
        QCOMPARE(item1->eventList.at(1), QEvent::GraphicsSceneDragMove);
    }

    {
        // Move into the intersection item1-item2
        QDragMoveEvent dragMove(view.mapFromScene(5, 5), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
        QApplication::sendEvent(view.viewport(), &dragMove);
        QVERIFY(!dragMove.isAccepted());    // move does not propagate, (ignoresDragMove = true)
        QCOMPARE(item1->eventList.size(), 3);
        QCOMPARE(item1->eventList.at(2), QEvent::GraphicsSceneDragLeave);
        QCOMPARE(item2->eventList.size(), 2);
        QCOMPARE(item2->eventList.at(0), QEvent::GraphicsSceneDragEnter);
        QCOMPARE(item2->eventList.at(1), QEvent::GraphicsSceneDragMove);
    }
    {
        // Move into the item2
        QDragMoveEvent dragMove(view.mapFromScene(15, 15), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
        QApplication::sendEvent(view.viewport(), &dragMove);
        QVERIFY(!dragMove.isAccepted());
        QCOMPARE(dragMove.dropAction(), Qt::CopyAction);
        QCOMPARE(item1->eventList.size(), 3);
        QCOMPARE(item2->eventList.size(), 3);
        QCOMPARE(item2->eventList.at(2), QEvent::GraphicsSceneDragMove);
    }
    {
        // Move inside item1
        QDragMoveEvent dragMove(view.mapFromScene(-5, -5), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
        QApplication::sendEvent(view.viewport(), &dragMove);
        QVERIFY(dragMove.isAccepted());
        QCOMPARE(dragMove.dropAction(), Qt::IgnoreAction);
        QCOMPARE(item1->eventList.size(), 5);
        QCOMPARE(item1->eventList.at(3), QEvent::GraphicsSceneDragEnter);
        QCOMPARE(item1->eventList.at(4), QEvent::GraphicsSceneDragMove);
        QCOMPARE(item2->eventList.size(), 4);
        QCOMPARE(item2->eventList.at(3), QEvent::GraphicsSceneDragLeave);
    }

    {
        item2->ignoresDragEnter = true;
        // Move into the intersection item1-item2
        QDragMoveEvent dragMove(view.mapFromScene(5, 5), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
        QApplication::sendEvent(view.viewport(), &dragMove);
        QVERIFY(dragMove.isAccepted());    // dragEnter propagates down to item1, which then accepts the move event.
        QCOMPARE(dragMove.dropAction(), Qt::IgnoreAction);
        QCOMPARE(item1->eventList.size(), 6);
        QCOMPARE(item1->eventList.at(5), QEvent::GraphicsSceneDragMove);
        QCOMPARE(item2->eventList.size(), 5);
        QCOMPARE(item2->eventList.at(4), QEvent::GraphicsSceneDragEnter);
    }

    {
        item2->ignoresDragEnter = false;
        // Drop on the intersection item1-item2
        QDropEvent drop(view.mapFromScene(5, 5), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
        QApplication::sendEvent(view.viewport(), &drop);
        QVERIFY(drop.isAccepted());
        QCOMPARE(drop.dropAction(), Qt::CopyAction);

        QCOMPARE(item1->eventList.size(), 7);
        QCOMPARE(item1->eventList.at(6), QEvent::GraphicsSceneDrop);
        QCOMPARE(item2->eventList.size(), 5);
    }

    // Dummy drop event to keep the Mac from crashing.
    QDropEvent dropEvent(QPoint(0, 0), Qt::CopyAction, &mimeData, Qt::LeftButton, {});
    QApplication::sendEvent(view.viewport(), &dropEvent);
}
#endif

void tst_QGraphicsScene::render_data()
{
    QTest::addColumn<QRectF>("targetRect");
    QTest::addColumn<QRectF>("sourceRect");
    QTest::addColumn<Qt::AspectRatioMode>("aspectRatioMode");
    QTest::addColumn<QTransform>("transform");
    QTest::addColumn<QPainterPath>("clip");

    QPainterPath clip_rect;
    clip_rect.addRect(50, 100, 200, 150);

    QPainterPath clip_ellipse;
    clip_ellipse.addEllipse(100,50,150,200);

    QTest::newRow("all-all-untransformed") << QRectF() << QRectF()
                                           << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("all-topleft-untransformed") << QRectF(0, 0, 150, 150)
                                               << QRectF() << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("all-topright-untransformed") << QRectF(150, 0, 150, 150)
                                                << QRectF() << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("all-bottomleft-untransformed") << QRectF(0, 150, 150, 150)
                                                  << QRectF() << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("all-bottomright-untransformed") << QRectF(150, 150, 150, 150)
                                                   << QRectF() << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("topleft-all-untransformed") << QRectF() << QRectF(-10, -10, 10, 10)
                                               << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("topright-all-untransformed") << QRectF() << QRectF(0, -10, 10, 10)
                                                << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("bottomleft-all-untransformed") << QRectF() << QRectF(-10, 0, 10, 10)
                                                  << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("bottomright-all-untransformed") << QRectF() << QRectF(0, 0, 10, 10)
                                                   << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("topleft-topleft-untransformed") << QRectF(0, 0, 150, 150) << QRectF(-10, -10, 10, 10)
                                                   << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("topright-topleft-untransformed") << QRectF(150, 0, 150, 150) << QRectF(-10, -10, 10, 10)
                                                    << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("bottomleft-topleft-untransformed") << QRectF(0, 150, 150, 150) << QRectF(-10, -10, 10, 10)
                                                      << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("bottomright-topleft-untransformed") << QRectF(150, 150, 150, 150) << QRectF(-10, -10, 10, 10)
                                                       << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("top-topleft-untransformed") << QRectF(0, 0, 300, 150) << QRectF(-10, -10, 10, 10)
                                               << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("bottom-topleft-untransformed") << QRectF(0, 150, 300, 150) << QRectF(-10, -10, 10, 10)
                                                  << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("left-topleft-untransformed") << QRectF(0, 0, 150, 300) << QRectF(-10, -10, 10, 10)
                                                << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("right-topleft-untransformed") << QRectF(150, 0, 150, 300) << QRectF(-10, -10, 10, 10)
                                                 << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("top-bottomright-untransformed") << QRectF(0, 0, 300, 150) << QRectF(0, 0, 10, 10)
                                                   << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("bottom-bottomright-untransformed") << QRectF(0, 150, 300, 150) << QRectF(0, 0, 10, 10)
                                                      << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("left-bottomright-untransformed") << QRectF(0, 0, 150, 300) << QRectF(0, 0, 10, 10)
                                                    << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("right-bottomright-untransformed") << QRectF(150, 0, 150, 300) << QRectF(0, 0, 10, 10)
                                                     << Qt::IgnoreAspectRatio << QTransform() << QPainterPath();
    QTest::newRow("all-all-45-deg-right") << QRectF() << QRectF()
                                          << Qt::IgnoreAspectRatio << QTransform().rotate(-45) << QPainterPath();
    QTest::newRow("all-all-45-deg-left") << QRectF() << QRectF()
                                         << Qt::IgnoreAspectRatio << QTransform().rotate(45) << QPainterPath();
    QTest::newRow("all-all-scale-2x") << QRectF() << QRectF()
                                      << Qt::IgnoreAspectRatio << QTransform::fromScale(2, 2) << QPainterPath();
    QTest::newRow("all-all-translate-50-0") << QRectF() << QRectF()
                                            << Qt::IgnoreAspectRatio << QTransform::fromTranslate(50, 0) << QPainterPath();
    QTest::newRow("all-all-translate-0-50") << QRectF() << QRectF()
                                            << Qt::IgnoreAspectRatio << QTransform::fromTranslate(0, 50) << QPainterPath();
    QTest::newRow("all-all-untransformed-clip-rect") << QRectF() << QRectF()
                                           << Qt::IgnoreAspectRatio << QTransform() << clip_rect;
    QTest::newRow("all-all-untransformed-clip-ellipse") << QRectF() << QRectF()
                                           << Qt::IgnoreAspectRatio << QTransform() << clip_ellipse;
}

void tst_QGraphicsScene::render()
{
    QFETCH(QRectF, targetRect);
    QFETCH(QRectF, sourceRect);
    QFETCH(Qt::AspectRatioMode, aspectRatioMode);
    QFETCH(QTransform, transform);
    QFETCH(QPainterPath, clip);

    QPixmap pix(30, 30);
    pix.fill(Qt::blue);

    QGraphicsView view;
    view.setWindowTitle(QTest::currentTestFunction());
    view.resize(m_testSize);
    QGraphicsScene scene(&view);
    scene.addEllipse(QRectF(-10, -10, 20, 20), QPen(Qt::black, 0), QBrush(Qt::white));
    scene.addEllipse(QRectF(-2, -7, 4, 4), QPen(Qt::black, 0), QBrush(Qt::yellow))->setZValue(1);
    QGraphicsPixmapItem *item = scene.addPixmap(pix);
    item->setZValue(2);
    item->setOffset(QPointF(3, 3));
    view.show();

    scene.setSceneRect(scene.itemsBoundingRect());

    QImage bigImage(300, 300, QImage::Format_RGB32);
    bigImage.fill(0);
    QPainter painter(&bigImage);
    painter.setPen(Qt::lightGray);
    for (int i = 0; i <= 300; i += 25) {
        painter.drawLine(0, i, 300, i);
        painter.drawLine(i, 0, i, 300);
    }
    painter.setPen(QPen(Qt::darkGray, 2));
    painter.drawLine(0, 150, 300, 150);
    painter.drawLine(150, 0, 150, 300);
    painter.setTransform(transform);
    if (!clip.isEmpty()) painter.setClipPath(clip);
    scene.render(&painter, targetRect, sourceRect, aspectRatioMode);
    painter.end();

    QString fileName = QFINDTESTDATA(QString("/testData/render/%1.png").arg(QTest::currentDataTag()));
    QImage original(fileName);
    QVERIFY(!original.isNull());

    // Compare
    int wrongPixels = 0;
    for (int y = 0; y < original.height(); ++y) {
        for (int x = 0; x < original.width(); ++x) {
            if (bigImage.pixel(x, y) != original.pixel(x, y))
                ++wrongPixels;
        }
    }

    // This is a pixmap compare test - because of rounding errors on diverse
    // platforms, and especially because tests are compiled in release mode,
    // we set a 95% acceptance threshold for comparing images. This number may
    // have to be adjusted if this test fails.
    qreal threshold = 0.95;
    qreal similarity = (1 - (wrongPixels / qreal(original.width() * original.height())));
    if (similarity < threshold) {
#if 1
        // fail
        QLabel *expectedLabel = new QLabel;
        expectedLabel->setPixmap(QPixmap::fromImage(original));

        QLabel *newLabel = new QLabel;
        newLabel->setPixmap(QPixmap::fromImage(bigImage));

        QGridLayout *gridLayout = new QGridLayout;
        gridLayout->addWidget(new QLabel(tr("MISMATCH: %1").arg(QTest::currentDataTag())), 0, 0, 1, 2);
        gridLayout->addWidget(new QLabel(tr("Current")), 1, 0);
        gridLayout->addWidget(new QLabel(tr("Expected")), 1, 1);
        gridLayout->addWidget(expectedLabel, 2, 1);
        gridLayout->addWidget(newLabel, 2, 0);

        QWidget widget;
        widget.setWindowTitle(QTest::currentTestFunction());
        widget.setLayout(gridLayout);
        widget.show();

        QTestEventLoop::instance().enterLoop(1);

        QFAIL("Images are not identical.");
#else
        // generate
        qDebug() << "Updating" << QTest::currentDataTag() << ":" << bigImage.save(fileName, "png");
#endif
    }
}

void tst_QGraphicsScene::renderItemsWithNegativeWidthOrHeight()
{
#ifdef Q_OS_ANDROID
    QSKIP("Test only works on platforms with resizable windows");
#endif
    QGraphicsScene scene(0, 0, m_testSize.width(), m_testSize.height());

    // Add item with negative width.
    QGraphicsRectItem *item1 = new QGraphicsRectItem(0, 0, -m_testSize.width(), 50);
    item1->setBrush(Qt::red);
    item1->setPos(m_testSize.width(), 50);
    scene.addItem(item1);

    // Add item with negative height.
    QGraphicsRectItem *item2 = new QGraphicsRectItem(0, 0, 50, -m_testSize.height());
    item2->setBrush(Qt::blue);
    item2->setPos(50, m_testSize.height());
    scene.addItem(item2);

    QGraphicsView view(&scene);
    view.setWindowTitle(QTest::currentTestFunction());
    view.setFrameStyle(QFrame::NoFrame);
    view.show();
    QTRY_COMPARE(view.viewport()->size(), m_testSize);

    QImage expected(view.viewport()->size(), QImage::Format_RGB32);
    view.viewport()->render(&expected);

    // Make sure the scene background is the same as the viewport background.
    scene.setBackgroundBrush(view.viewport()->palette().brush(view.viewport()->backgroundRole()));
    QImage actual(m_testSize, QImage::Format_RGB32);
    QPainter painter(&actual);
    scene.render(&painter);
    painter.end();

    QCOMPARE(actual, expected);
}

#ifndef QT_NO_CONTEXTMENU
void tst_QGraphicsScene::contextMenuEvent()
{
    QGraphicsScene scene;
    QEvent activate(QEvent::WindowActivate);
    QApplication::sendEvent(&scene, &activate);

    EventTester *item = new EventTester;
    scene.addItem(item);
    item->setFlag(QGraphicsItem::ItemIsFocusable);
    item->setFocus();

    QVERIFY(item->hasFocus());
    QVERIFY(scene.hasFocus());

    QGraphicsView view(&scene);
    view.setWindowTitle(QTest::currentTestFunction());
    view.resize(m_testSize);
    view.show();
    view.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    view.centerOn(item);

    {
        QContextMenuEvent event(QContextMenuEvent::Keyboard, view.viewport()->rect().center(),
                                view.mapToGlobal(view.viewport()->rect().center()));
        QApplication::sendEvent(view.viewport(), &event);
        QCOMPARE(item->eventTypes.last(), QEvent::GraphicsSceneContextMenu);
    }
}

class ContextMenuItem : public QGraphicsRectItem
{
public:
    ContextMenuItem(const QSize &s) : QGraphicsRectItem(0, 0, s.width(), s.height())
    { setBrush(Qt::red); }

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *) override
    { /* just accept */ }
};

void tst_QGraphicsScene::contextMenuEvent_ItemIgnoresTransformations()
{
#ifdef Q_OS_ANDROID
    QSKIP("Test fails on some Android devices (QTBUG-44430)");
#endif

    QGraphicsScene scene(0, 0, m_testSize.width(), m_testSize.height());
    const QSize itemSize = m_testSize / 2;
    ContextMenuItem *item = new ContextMenuItem(itemSize);
    item->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene.addItem(item);

    QWidget topLevel;
    topLevel.setWindowTitle(QTest::currentTestFunction());
    topLevel.resize(m_testSize);
    QGraphicsView view(&scene, &topLevel);
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));



    {
        QPoint pos(itemSize.width() / 2, itemSize.height() / 2);
        QContextMenuEvent event(QContextMenuEvent::Keyboard, pos, view.viewport()->mapToGlobal(pos));
        event.ignore();
        QApplication::sendEvent(view.viewport(), &event);
        QVERIFY(event.isAccepted());
    }
    {
        QPoint pos(itemSize.width() * 3 / 2, itemSize.height() * 3 / 2);
        QContextMenuEvent event(QContextMenuEvent::Keyboard, pos, view.viewport()->mapToGlobal(pos));
        event.ignore();
        QApplication::sendEvent(view.viewport(), &event);
        QVERIFY(!event.isAccepted());
    }
    view.scale(1.5, 1.5);
    {
        QPoint pos(itemSize.width() / 4, itemSize.height() / 4);
        QContextMenuEvent event(QContextMenuEvent::Keyboard, pos, view.viewport()->mapToGlobal(pos));
        event.ignore();
        QApplication::sendEvent(view.viewport(), &event);
        QVERIFY(event.isAccepted());
    }
    {
        QPoint pos(itemSize.width() / 2 + 5, itemSize.height() / 2 + 5);
        QContextMenuEvent event(QContextMenuEvent::Keyboard, pos, view.viewport()->mapToGlobal(pos));
        event.ignore();
        QApplication::sendEvent(view.viewport(), &event);
        QVERIFY(!event.isAccepted());
    }
}
#endif // QT_NO_CONTEXTMENU

void tst_QGraphicsScene::update()
{
    QGraphicsScene scene;

    QGraphicsRectItem *rect = new QGraphicsRectItem(0, 0, 100, 100);
    rect->setPen(QPen(Qt::black, 0));
    scene.addItem(rect);
    QCoreApplication::processEvents();
    rect->setPos(-100, -100);

    // This function forces indexing
    itemAt(scene, 0, 0);

    qRegisterMetaType<QList<QRectF> >("QList<QRectF>");
    QSignalSpy spy(&scene, &QGraphicsScene::changed);

    // We update the scene.
    scene.update();

    // This function forces a purge, which will post an update signal
    itemAt(scene, 0, 0);

    // This will process the pending update
    QCoreApplication::processEvents();

    // Check that the update region is correct
    QCOMPARE(spy.size(), 1);
    QRectF region;
    const auto &rects = qvariant_cast<QList<QRectF> >(spy.at(0).at(0));
    for (const auto &rectF : rects)
        region |= rectF;
    QCOMPARE(region, QRectF(-100, -100, 200, 200));
}

void tst_QGraphicsScene::update2()
{
    QGraphicsScene scene;
    scene.setSceneRect(-200, -200, 200, 200);
    CustomView view;
    view.setWindowTitle(QTest::currentTestFunction());
    view.resize(m_testSize);
    view.setScene(&scene);
    view.show();
    QApplicationPrivate::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTRY_VERIFY(view.repaints >= 1);
    view.repaints = 0;

    // Make sure QGraphicsScene::update only requires one event-loop iteration
    // before the view is updated.
    scene.update();
    QCoreApplication::processEvents();
    QTRY_COMPARE(view.repaints, 1);
    view.repaints = 0;

    // The same for partial scene updates.
    scene.update(QRectF(-100, -100, 100, 100));
    QCoreApplication::processEvents();
    QCOMPARE(view.repaints, 1);
}

void tst_QGraphicsScene::views()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);

    QCOMPARE(scene.views().size(), 1);
    QCOMPARE(scene.views().at(0), &view);

    QGraphicsView view1(&scene);
    QCOMPARE(scene.views().size(), 2);
    QVERIFY(scene.views().contains(&view1));

    view.setScene(nullptr);
    QCOMPARE(scene.views().size(), 1);
    QCOMPARE(scene.views().at(0), &view1);

    QGraphicsView *view2 = new QGraphicsView(&scene);
    QCOMPARE(scene.views().size(), 2);
    QCOMPARE(scene.views().at(0), &view1);
    QCOMPARE(scene.views().at(1), view2);

    delete view2;

    QCOMPARE(scene.views().size(), 1);
    QCOMPARE(scene.views().at(0), &view1);
}

class CustomScene : public QGraphicsScene
{
public:
    CustomScene()
    { startTimer(10); }

    bool gotTimerEvent = false;
protected:
    void timerEvent(QTimerEvent *) override
    {
        gotTimerEvent = true;
    }
};

void tst_QGraphicsScene::testEvent()
{
    // Test that QGraphicsScene properly propagates events to QObject.
    CustomScene scene;
    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(scene.gotTimerEvent);
}

class DisabledItemTester : public QGraphicsRectItem
{
public:
    DisabledItemTester(const QRectF &rect, QGraphicsItem *parent = nullptr)
        : QGraphicsRectItem(rect, parent)
    { }

    QList<QEvent::Type> receivedSceneEvents;
    QList<QEvent::Type> receivedSceneEventFilters;

protected:
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override
    {
        receivedSceneEventFilters << event->type();
        return QGraphicsRectItem::sceneEventFilter(watched, event);
    }

    bool sceneEvent(QEvent *event) override
    {
        receivedSceneEvents << event->type();
        return QGraphicsRectItem::sceneEvent(event);
    }
};

void tst_QGraphicsScene::eventsToDisabledItems()
{
    QGraphicsScene scene;

    DisabledItemTester *item1 = new DisabledItemTester(QRectF(-50, -50, 100, 100));
    DisabledItemTester *item2 = new DisabledItemTester(QRectF(-50, -50, 100, 100));
    item1->setZValue(1); // on top

    scene.addItem(item1);
    scene.addItem(item2);

    item1->installSceneEventFilter(item2);

    QVERIFY(item1->receivedSceneEvents.isEmpty());
    QVERIFY(item2->receivedSceneEvents.isEmpty());
    QVERIFY(item1->receivedSceneEventFilters.isEmpty());
    QVERIFY(item2->receivedSceneEventFilters.isEmpty());

    QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMousePress);
    event.setButton(Qt::LeftButton);
    QApplication::sendEvent(&scene, &event);

    // First item2 receives a scene event filter. Then item1 receives the
    // actual event. Finally the event propagates to item2. So both items
    // should have received the event, and item1 also got the filter.
    QCOMPARE(item1->receivedSceneEvents.size(), 3);
    QCOMPARE(item2->receivedSceneEvents.size(), 3);
    QCOMPARE(item1->receivedSceneEventFilters.size(), 0);
    QCOMPARE(item2->receivedSceneEventFilters.size(), 3);

    item1->receivedSceneEvents.clear();
    item1->receivedSceneEventFilters.clear();
    item2->receivedSceneEvents.clear();
    item2->receivedSceneEventFilters.clear();

    item1->setEnabled(false); // disable the topmost item, eat mouse events

    event.setButton(Qt::LeftButton);
    event.setAccepted(false);
    QApplication::sendEvent(&scene, &event);

    // Check that only item1 received anything - it only got the filter.
    QCOMPARE(item1->receivedSceneEvents.size(), 0);
    QCOMPARE(item2->receivedSceneEvents.size(), 0);
    QCOMPARE(item1->receivedSceneEventFilters.size(), 0);
    QCOMPARE(item2->receivedSceneEventFilters.size(), 3);
}

class ExposedPixmapItem : public QGraphicsPixmapItem
{
public:
    using QGraphicsPixmapItem::QGraphicsPixmapItem;

    void paint(QPainter *, const QStyleOptionGraphicsItem *option, QWidget *) override
    {
        exposed = option->exposedRect;
    }

    QRectF exposed;
};

void tst_QGraphicsScene::exposedRect()
{
    ExposedPixmapItem *item = new ExposedPixmapItem;
    item->setPixmap(QPixmap(":/Ash_European.jpg"));
    QGraphicsScene scene;
    scene.addItem(item);

    QCOMPARE(item->exposed, QRectF());

    QImage image(100, 100, QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&image);

    scene.render(&painter);
    QCOMPARE(item->exposed, item->boundingRect());

    painter.rotate(180);
    painter.translate(100, 100);

    scene.render(&painter);
    QCOMPARE(item->exposed, item->boundingRect());
}

void tst_QGraphicsScene::tabFocus_emptyScene()
{
    QGraphicsScene scene;
    QDial *dial1 = new QDial;
    QGraphicsView *view = new QGraphicsView(&scene);
    QDial *dial2 = new QDial;

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(dial1);
    layout->addWidget(view);
    layout->addWidget(dial2);

    QWidget widget;
    widget.setLayout(layout);
    widget.setWindowTitle(QTest::currentTestFunction());
    widget.show();
    QApplicationPrivate::setActiveWindow(&widget);
    widget.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&widget));

    dial1->setFocus();
    QVERIFY(dial1->hasFocus());
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QVERIFY(!dial1->hasFocus());
    QVERIFY(view->hasFocus());
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QVERIFY(!view->hasFocus());
    QVERIFY(dial2->hasFocus());
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QVERIFY(!dial2->hasFocus());
    QVERIFY(view->hasFocus());
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QVERIFY(dial1->hasFocus());
    QVERIFY(!dial2->hasFocus());
}

void tst_QGraphicsScene::tabFocus_sceneWithFocusableItems()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QGraphicsScene scene;
    QGraphicsTextItem *item = scene.addText("Qt rocks!");
    item->setTabChangesFocus(true);
    item->setTextInteractionFlags(Qt::TextEditorInteraction);
    QVERIFY(item->flags() & QGraphicsItem::ItemIsFocusable);
    item->setFocus();
    item->clearFocus();

    QGraphicsTextItem *item2 = scene.addText("Qt rocks!");
    item2->setTabChangesFocus(true);
    item2->setTextInteractionFlags(Qt::TextEditorInteraction);
    item2->setPos(0, item->boundingRect().bottom());
    QVERIFY(item2->flags() & QGraphicsItem::ItemIsFocusable);

    QDial *dial1 = new QDial;
    QGraphicsView *view = new QGraphicsView(&scene);
    QDial *dial2 = new QDial;

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(dial1);
    layout->addWidget(view);
    layout->addWidget(dial2);

    QWidget widget;
    widget.setWindowTitle(QTest::currentTestFunction());
    widget.setLayout(layout);
    widget.show();
    QApplicationPrivate::setActiveWindow(&widget);
    widget.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&widget));

    dial1->setFocus();
    QTRY_VERIFY(dial1->hasFocus());
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QTRY_VERIFY(view->hasFocus());
    QVERIFY(view->viewport()->hasFocus());
    QVERIFY(scene.hasFocus());
    QVERIFY(item->hasFocus());

    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QTRY_VERIFY(dial2->hasFocus());

    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QTRY_VERIFY(view->hasFocus());
    QTRY_VERIFY(view->viewport()->hasFocus());
    QTRY_VERIFY(scene.hasFocus());
    QTRY_VERIFY(item->hasFocus());

    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QTRY_VERIFY(dial1->hasFocus());

    // If the item requests input focus, it can only ensure that the scene
    // sets focus on itself, but the scene cannot request focus from any view.
    item->setFocus();
    QApplication::processEvents();
    QTRY_VERIFY(!view->hasFocus());
    QVERIFY(!view->viewport()->hasFocus());
    QTRY_VERIFY(scene.hasFocus());
    QVERIFY(item->hasFocus());

    view->setFocus();
    QApplication::processEvents();
    QTRY_VERIFY(view->hasFocus());
    QTRY_VERIFY(view->viewport()->hasFocus());
    QTRY_VERIFY(scene.hasFocus());
    QTRY_VERIFY(item->hasFocus());

    // Check that everyone loses focus when the widget is hidden.
    widget.hide();
    QTRY_VERIFY(!view->hasFocus());
    QVERIFY(!view->viewport()->hasFocus());
    QVERIFY(!scene.hasFocus());
    QVERIFY(!item->hasFocus());
    QCOMPARE(scene.focusItem(), item);

    // Check that the correct item regains focus.
    widget.show();
    QApplicationPrivate::setActiveWindow(&widget);
    widget.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&widget));
    QVERIFY(view->hasFocus());
    QTRY_VERIFY(scene.isActive());
    QVERIFY(view->viewport()->hasFocus());
    QVERIFY(scene.hasFocus());
    QCOMPARE(scene.focusItem(), item);
    QVERIFY(item->hasFocus());
}

class FocusWidget : public QGraphicsWidget
{
    Q_OBJECT
public:
    FocusWidget(QGraphicsItem *parent = nullptr) : QGraphicsWidget(parent)
    {
        setFocusPolicy(Qt::StrongFocus);
        resize(100, 100);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *) override
    {
        if (option->state & QStyle::State_HasFocus) {
            painter->fillRect(rect(), Qt::blue);
        }
        painter->setBrush(Qt::green);
        painter->drawEllipse(rect());
        if (option->state & QStyle::State_HasFocus) {
            painter->setPen(QPen(Qt::black, 1, Qt::DashLine));
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(rect().adjusted(5, 5, -5, -5));
        }
    }

    int tabs = 0;
    int backTabs = 0;

protected:
    bool sceneEvent(QEvent *event) override
    {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *k = static_cast<QKeyEvent *>(event);
            if (k->key() == Qt::Key_Tab)
                ++tabs;
            if (k->key() == Qt::Key_Backtab)
                ++backTabs;
        }
        return QGraphicsWidget::sceneEvent(event);
    }

    void focusInEvent(QFocusEvent *) override
    { update(); }
    void focusOutEvent(QFocusEvent *) override
    { update(); }
};

void tst_QGraphicsScene::tabFocus_sceneWithFocusWidgets()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QGraphicsScene scene;

    FocusWidget *widget1 = new FocusWidget;
    FocusWidget *widget2 = new FocusWidget;
    widget2->setPos(widget1->boundingRect().right(), 0);
    scene.addItem(widget1);
    scene.addItem(widget2);

    QDial *dial1 = new QDial;
    QGraphicsView *view = new QGraphicsView(&scene);
    view->setRenderHint(QPainter::Antialiasing);
    QDial *dial2 = new QDial;

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(dial1);
    layout->addWidget(view);
    layout->addWidget(dial2);

    QWidget widget;
    widget.setWindowTitle(QTest::currentTestFunction());
    widget.setLayout(layout);
    widget.show();
    QApplicationPrivate::setActiveWindow(&widget);
    widget.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&widget));

    dial1->setFocus();
    QTRY_VERIFY(dial1->hasFocus());

    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QTRY_VERIFY(view->hasFocus());
    QTRY_VERIFY(view->viewport()->hasFocus());
    QTRY_VERIFY(scene.hasFocus());
    QCOMPARE(widget1->tabs, 0);
    QVERIFY(widget1->hasFocus());
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QTRY_COMPARE(widget1->tabs, 1);
    QTRY_VERIFY(widget2->hasFocus());
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QTRY_COMPARE(widget2->tabs, 1);
    QTRY_VERIFY(dial2->hasFocus());
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QTRY_VERIFY(widget2->hasFocus());
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QTRY_COMPARE(widget2->backTabs, 1);
    QTRY_VERIFY(widget1->hasFocus());
    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QTRY_COMPARE(widget1->backTabs, 1);
    QTRY_VERIFY(dial1->hasFocus());

    widget1->setFocus();
    view->viewport()->setFocus();
    widget.hide();
    QTest::qWait(15);
    widget.show();
    QApplicationPrivate::setActiveWindow(&widget);
    widget.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&widget));
    QTRY_VERIFY(widget1->hasFocus());
}

void tst_QGraphicsScene::tabFocus_sceneWithNestedFocusWidgets()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QGraphicsScene scene;

    FocusWidget *widget1 = new FocusWidget;
    FocusWidget *widget1_1 = new FocusWidget;
    FocusWidget *widget1_2 = new FocusWidget;
    widget1_1->setParentItem(widget1);
    const QTransform scale(QTransform::fromScale(0.5, 0.5));
    widget1_1->setTransform(scale, true);
    widget1_1->setPos(0, widget1->boundingRect().height() / 2);
    widget1_2->setParentItem(widget1);
    widget1_2->setTransform(scale, true);
    widget1_2->setPos(widget1->boundingRect().width() / 2, widget1->boundingRect().height() / 2);

    FocusWidget *widget2 = new FocusWidget;
    widget2->setPos(widget1->boundingRect().right(), 0);

    widget1->setData(0, "widget1");
    widget1_1->setData(0, "widget1_1");
    widget1_2->setData(0, "widget1_2");
    widget2->setData(0, "widget2");

    scene.addItem(widget1);
    scene.addItem(widget2);

    QDial *dial1 = new QDial;
    QGraphicsView *view = new QGraphicsView(&scene);
    view->setRenderHint(QPainter::Antialiasing);
    QDial *dial2 = new QDial;

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(dial1);
    layout->addWidget(view);
    layout->addWidget(dial2);

    QWidget widget;
    widget.setWindowTitle(QTest::currentTestFunction());
    widget.setLayout(layout);
    widget.show();
    QApplicationPrivate::setActiveWindow(&widget);
    widget.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&widget));

    dial1->setFocus();
    QTRY_VERIFY(dial1->hasFocus());

    EventSpy focusInSpy_1(widget1, QEvent::FocusIn);
    EventSpy focusOutSpy_1(widget1, QEvent::FocusOut);
    EventSpy focusInSpy_1_1(widget1_1, QEvent::FocusIn);
    EventSpy focusOutSpy_1_1(widget1_1, QEvent::FocusOut);
    EventSpy focusInSpy_1_2(widget1_2, QEvent::FocusIn);
    EventSpy focusOutSpy_1_2(widget1_2, QEvent::FocusOut);
    EventSpy focusInSpy_2(widget2, QEvent::FocusIn);
    EventSpy focusOutSpy_2(widget2, QEvent::FocusOut);

    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QTRY_VERIFY(widget1->hasFocus());
    QCOMPARE(focusInSpy_1.count(), 1);

    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QTRY_VERIFY(!widget1->hasFocus());
    QVERIFY(widget1_1->hasFocus());
    QCOMPARE(focusOutSpy_1.count(), 1);
    QCOMPARE(focusInSpy_1_1.count(), 1);

    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QTRY_VERIFY(!widget1_1->hasFocus());
    QVERIFY(widget1_2->hasFocus());
    QCOMPARE(focusOutSpy_1_1.count(), 1);
    QCOMPARE(focusInSpy_1_2.count(), 1);

    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QTRY_VERIFY(!widget1_2->hasFocus());
    QVERIFY(widget2->hasFocus());
    QCOMPARE(focusOutSpy_1_2.count(), 1);
    QCOMPARE(focusInSpy_2.count(), 1);

    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Tab);
    QApplication::processEvents();
    QTRY_VERIFY(!widget2->hasFocus());
    QVERIFY(dial2->hasFocus());
    QCOMPARE(focusOutSpy_2.count(), 1);

    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QTRY_VERIFY(widget2->hasFocus());
    QCOMPARE(focusInSpy_2.count(), 2);

    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QTRY_VERIFY(!widget2->hasFocus());
    QTRY_VERIFY(widget1_2->hasFocus());
    QCOMPARE(focusOutSpy_2.count(), 2);
    QCOMPARE(focusInSpy_1_2.count(), 2);

    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QTRY_VERIFY(!widget1_2->hasFocus());
    QTRY_VERIFY(widget1_1->hasFocus());
    QCOMPARE(focusOutSpy_1_2.count(), 2);
    QCOMPARE(focusInSpy_1_1.count(), 2);

    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QTRY_VERIFY(!widget1_1->hasFocus());
    QTRY_VERIFY(widget1->hasFocus());
    QCOMPARE(focusOutSpy_1_1.count(), 2);
    QCOMPARE(focusInSpy_1.count(), 2);

    QTest::keyPress(QApplication::focusWidget(), Qt::Key_Backtab);
    QApplication::processEvents();
    QTRY_VERIFY(!widget1->hasFocus());
    QTRY_VERIFY(dial1->hasFocus());
    QCOMPARE(focusOutSpy_1.count(), 2);

    widget1->setFocus();
    view->viewport()->setFocus();
    widget.hide();
    QTest::qWait(12);
    widget.show();
    QApplicationPrivate::setActiveWindow(&widget);
    widget.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&widget));
    QTRY_VERIFY(widget1->hasFocus());
}

void tst_QGraphicsScene::style()
{
    QPointer<QStyle> windowsStyle = QStyleFactory::create("windows");

    QGraphicsScene scene;
    QLineEdit *edit = new QLineEdit;
    QGraphicsProxyWidget *proxy = scene.addWidget(edit);

    EventSpy sceneSpy(&scene, QEvent::StyleChange);
    EventSpy proxySpy(proxy, QEvent::StyleChange);
    EventSpy editSpy(edit, QEvent::StyleChange);

    QCOMPARE(scene.style(), QApplication::style());

    scene.setStyle(windowsStyle);
    QCOMPARE(sceneSpy.count(), 1);
    QCOMPARE(proxySpy.count(), 1);
    QCOMPARE(editSpy.count(), 1);
    QCOMPARE(scene.style(), windowsStyle.data());
    QCOMPARE(proxy->style(), windowsStyle.data());
    QCOMPARE(edit->style(), windowsStyle.data());

    scene.setStyle(nullptr);
    QCOMPARE(sceneSpy.count(), 2);
    QCOMPARE(proxySpy.count(), 2);
    QCOMPARE(editSpy.count(), 2);
    QCOMPARE(scene.style(), QApplication::style());
    QCOMPARE(proxy->style(), QApplication::style());
    QCOMPARE(edit->style(), QApplication::style());
    QVERIFY(!windowsStyle); // deleted
}

void tst_QGraphicsScene::task139710_bspTreeCrash()
{
    // create a scene with 2000 items
    QGraphicsScene scene(0, 0, 1000, 1000);

    for (int i = 0; i < 2; ++i) {
        // trigger delayed item indexing
        QCoreApplication::processEvents();
        scene.setSceneRect(0, 0, 10000, 10000);

        // delete all items in the scene - pointers are now likely to be recycled
        const auto &items = scene.items();
        for (QGraphicsItem *item : items) {
            scene.removeItem(item);
            delete item;
        }

        // add 1000 more items - the BSP tree is now resized
        for (int i = 0; i < 1000; ++i) {
            QGraphicsRectItem *item = scene.addRect(QRectF(0, 0, 200, 200));
            item->setPos(QRandomGenerator::global()->bounded(10000), QRandomGenerator::global()->bounded(10000));
        }

        // trigger delayed item indexing for the first 1000 items
        QCoreApplication::processEvents();

        // add 1000 more items - the BSP tree is now resized
        for (int i = 0; i < 1000; ++i) {
            QGraphicsRectItem *item = scene.addRect(QRectF(0, 0, 200, 200));
            item->setPos(QRandomGenerator::global()->bounded(10000), QRandomGenerator::global()->bounded(10000));
        }

        // get items from the BSP tree and use them. there was junk in the tree
        // the second time this happened.
        const auto &itemsWithin = scene.items(QRectF(0, 0, 1000, 1000));
        for (QGraphicsItem *item : itemsWithin)
            item->moveBy(0, 0);
    }
}

void tst_QGraphicsScene::task139782_containsItemBoundingRect()
{
    // The item in question has a scene bounding rect of (10, 10, 50, 50)
    QGraphicsScene scene(0.0, 0.0, 200.0, 200.0);
    QGraphicsRectItem *item = new QGraphicsRectItem(0.0, 0.0, 50.0, 50.0, nullptr);
    scene.addItem(item);
    item->setPos(10.0, 10.0);

    // The (0, 0, 50, 50) scene rect should not include the item's bounding rect
    QVERIFY(!scene.items(QRectF(0.0, 0.0, 50.0, 50.0), Qt::ContainsItemBoundingRect).contains(item));

    // The (9, 9, 500, 500) scene rect _should_ include the item's bounding rect
    QVERIFY(scene.items(QRectF(9.0, 9.0, 500.0, 500.0), Qt::ContainsItemBoundingRect).contains(item));

    // The (25, 25, 5, 5) scene rect should not include the item's bounding rect
    QVERIFY(!scene.items(QRectF(25.0, 25.0, 5.0, 5.0), Qt::ContainsItemBoundingRect).contains(item));
}

void tst_QGraphicsScene::task176178_itemIndexMethodBreaksSceneRect()
{
    QGraphicsScene scene;
    scene.setItemIndexMethod(QGraphicsScene::NoIndex);
    QGraphicsRectItem *rect = new QGraphicsRectItem;
    rect->setPen(QPen(Qt::black, 0));
    rect->setRect(0,0,100,100);
    scene.addItem(rect);
    QCOMPARE(scene.sceneRect(), rect->rect());
}

void tst_QGraphicsScene::task160653_selectionChanged()
{
    QGraphicsScene scene(0, 0, 100, 100);
    scene.addItem(new QGraphicsRectItem(0, 0, 20, 20));
    scene.addItem(new QGraphicsRectItem(30, 30, 20, 20));
    const auto &items = scene.items();
    for (QGraphicsItem *item : items) {
        item->setFlags(
            item->flags() | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
        item->setSelected(true);
    }
    QVERIFY(scene.items().size() > 1);
    QCOMPARE(scene.items().size(), scene.selectedItems().size());

    QSignalSpy spy(&scene, &QGraphicsScene::selectionChanged);
    QGraphicsView view(&scene);
    view.setWindowTitle(QTest::currentTestFunction());
    view.resize(m_testSize);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QTest::mouseClick(
                view.viewport(), Qt::LeftButton, {}, view.mapFromScene(scene.items().first()->scenePos()));
    QCOMPARE(spy.size(), 1);
}

void tst_QGraphicsScene::task250680_childClip()
{
    QGraphicsRectItem *clipper = new QGraphicsRectItem;
    clipper->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
    clipper->setPen(QPen(Qt::green, 0));
    clipper->setRect(200, 200, 640, 480);

    QGraphicsRectItem *rect = new QGraphicsRectItem(clipper);
    rect->setPen(QPen(Qt::red, 0));
    rect->setBrush(QBrush(QColor(255, 0, 0, 75)));
    rect->setPos(320, 240);
    rect->setRect(-25, -25, 50, 50);

    QGraphicsScene scene;
    scene.addItem(clipper);

    QPainterPath path;
    path.addRect(-25, -25, 50, 50);
    QVERIFY(QPathCompare::comparePaths(rect->clipPath().simplified(), path));

    QCOMPARE(scene.items(QRectF(320, 240, 5, 5)).size(), 2);
    rect->setTransform(QTransform().rotate(45), true);
    QCOMPARE(scene.items(QRectF(320, 240, 5, 5)).size(), 2);
}

void tst_QGraphicsScene::sorting()
{
    QGraphicsScene scene;

    QGraphicsRectItem *t_1 = new QGraphicsRectItem(0, 0, 50, 50);
    QGraphicsRectItem *c_1 = new QGraphicsRectItem(0, 0, 40, 40, t_1);
    QGraphicsRectItem *c_1_1 = new QGraphicsRectItem(0, 0, 30, 30, c_1);
    QGraphicsRectItem *c_1_1_1 = new QGraphicsRectItem(0, 0, 20, 20, c_1_1);
    QGraphicsRectItem *c_1_2 = new QGraphicsRectItem(0, 0, 30, 30, c_1);
    QGraphicsRectItem *c_2 = new QGraphicsRectItem(0, 0, 40, 40, t_1);
    QGraphicsRectItem *c_2_1 = new QGraphicsRectItem(0, 0, 30, 30, c_2);
    QGraphicsRectItem *c_2_1_1 = new QGraphicsRectItem(0, 0, 20, 20, c_2_1);
    QGraphicsRectItem *c_2_2 = new QGraphicsRectItem(0, 0, 30, 30, c_2);
    t_1->setBrush(QColor(QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256)));
    c_1->setBrush(QColor(QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256)));
    c_1_1->setBrush(QColor(QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256)));
    c_1_1_1->setBrush(QColor(QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256)));
    c_1_2->setBrush(QColor(QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256)));
    c_2->setBrush(QColor(QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256)));
    c_2_1->setBrush(QColor(QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256)));
    c_2_1_1->setBrush(QColor(QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256)));
    c_2_2->setBrush(QColor(QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256)));

    c_1->setPos(23, 18);
    c_1_1->setPos(24, 28);
    c_1_1_1->setPos(-16, 16);
    c_1_2->setPos(-16, 28);
    c_1_2->setZValue(1);
    c_2->setPos(-23, 18);
    c_2->setZValue(1);
    c_2_1->setPos(24, 28);
    c_2_1_1->setPos(-16, 16);
    c_2_2->setPos(-16, 28);
    c_2_2->setZValue(1);

    c_1->setFlag(QGraphicsItem::ItemIsMovable);
    c_1_1->setFlag(QGraphicsItem::ItemIsMovable);
    c_1_1_1->setFlag(QGraphicsItem::ItemIsMovable);
    c_1_2->setFlag(QGraphicsItem::ItemIsMovable);
    c_2->setFlag(QGraphicsItem::ItemIsMovable);
    c_2_1->setFlag(QGraphicsItem::ItemIsMovable);
    c_2_1_1->setFlag(QGraphicsItem::ItemIsMovable);
    c_2_2->setFlag(QGraphicsItem::ItemIsMovable);

    t_1->setData(0, "t_1");
    c_1->setData(0, "c_1");
    c_1_1->setData(0, "c_1_1");
    c_1_1_1->setData(0, "c_1_1_1");
    c_1_2->setData(0, "c_1_2");
    c_2->setData(0, "c_2");
    c_2_1->setData(0, "c_2_1");
    c_2_1_1->setData(0, "c_2_1_1");
    c_2_2->setData(0, "c_2_2");

    scene.addItem(t_1);

    const auto &items = scene.items();
    for (QGraphicsItem *item : items)
        item->setFlag(QGraphicsItem::ItemIsSelectable);

    // QGraphicsView view(&scene);
    // view.setDragMode(QGraphicsView::RubberBandDrag);
    // view.show();

    if (lcTests().isDebugEnabled()) {
        qCDebug(lcTests) << "items: {";
        const auto &itemsWithin = scene.items(QRectF(32, 31, 4, 55));
        for (QGraphicsItem *item : itemsWithin)
            qCDebug(lcTests).nospace() << '\t' << item->data(0).toString();
        qCDebug(lcTests) << '}';
    }

    QCOMPARE(scene.items(QRectF(32, 31, 4, 55)),
             QList<QGraphicsItem *>()
             << c_1_2 << c_1_1_1 << c_1 << t_1);
    QCOMPARE(scene.items(QRectF(-53, 47, 136, 3)),
             QList<QGraphicsItem *>()
             << c_2_2 << c_2_1 << c_2 << c_1_2 << c_1_1 << c_1 << t_1);
    QCOMPARE(scene.items(QRectF(-23, 79, 104, 3)),
             QList<QGraphicsItem *>()
             << c_2_1_1 << c_1_1_1);
    QCOMPARE(scene.items(QRectF(-26, -3, 92, 79)),
             QList<QGraphicsItem *>()
             << c_2_2 << c_2_1_1 << c_2_1 << c_2
             << c_1_2 << c_1_1_1 << c_1_1 << c_1
             << t_1);
}

void tst_QGraphicsScene::insertionOrder()
{
    QGraphicsScene scene;
    const int numItems = 5;
    QList<QGraphicsItem*> items;

    for (int i = 0; i < numItems; ++i) {
        QGraphicsRectItem* item = new QGraphicsRectItem(i * 20, i * 20, 200, 200);
        item->setData(0, i);
        items.append(item);
        scene.addItem(item);
    }

    {
        QList<QGraphicsItem*> itemList = scene.items();
        QCOMPARE(itemList.size(), numItems);
        for (int i = 0; i < itemList.size(); ++i) {
            QCOMPARE(numItems-1-i, itemList.at(i)->data(0).toInt());
        }
    }

    for (int i = 0; i < items.size(); ++i)
    {
        scene.removeItem(items.at(i));
        scene.addItem(items.at(i));
    }

    {
        QList<QGraphicsItem*> itemList = scene.items();
        QCOMPARE(itemList.size(), numItems);
        for (int i = 0; i < itemList.size(); ++i) {
            QCOMPARE(numItems-1-i, itemList.at(i)->data(0).toInt());
        }
    }
}

class ChangedListener : public QObject
{
    Q_OBJECT
public:
    QList<QList<QRectF> > changes;

public slots:
    void changed(const QList<QRectF> &dirty)
    {
        changes << dirty;
    }
};

void tst_QGraphicsScene::changedSignal_data()
{
    QTest::addColumn<bool>("withView");

    QTest::newRow("without view") << false;
    QTest::newRow("with view") << true;
}

void tst_QGraphicsScene::changedSignal()
{
    QFETCH(bool, withView);
    QGraphicsScene scene;
    ChangedListener cl;
    connect(&scene, &QGraphicsScene::changed, &cl, &ChangedListener::changed);

    QScopedPointer<QGraphicsView> view;
    if (withView)
        view.reset(new QGraphicsView(&scene));

    QGraphicsRectItem *rect = new QGraphicsRectItem(0, 0, 10, 10);
    rect->setPen(QPen(Qt::black, 0));
    scene.addItem(rect);

    QCOMPARE(cl.changes.size(), 0);
    QTRY_COMPARE(cl.changes.size(), 1);
    QCOMPARE(cl.changes.at(0).size(), 1);
    QCOMPARE(cl.changes.at(0).first(), QRectF(0, 0, 10, 10));

    rect->setPos(20, 0);

    QCOMPARE(cl.changes.size(), 1);
    QCoreApplication::processEvents();
    QCOMPARE(cl.changes.size(), 2);
    QCOMPARE(cl.changes.at(1).size(), 2);

    QCOMPARE(scene.sceneRect(), QRectF(0, 0, 30, 10));
}

void tst_QGraphicsScene::stickyFocus_data()
{
    QTest::addColumn<bool>("sticky");
    QTest::newRow("sticky") << true;
    QTest::newRow("not sticky") << false;
}

void tst_QGraphicsScene::stickyFocus()
{
    QFETCH(bool, sticky);

    QGraphicsScene scene;
    QEvent activate(QEvent::WindowActivate);
    QApplication::sendEvent(&scene, &activate);

    QGraphicsTextItem *text = scene.addText("Hei");
    text->setTextInteractionFlags(Qt::TextEditorInteraction);
    text->setFocus();

    scene.setStickyFocus(sticky);

    QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMousePress);
    event.setScenePos(QPointF(-10, -10)); // outside item
    event.setButton(Qt::LeftButton);
    QCoreApplication::sendEvent(&scene, &event);

    QCOMPARE(text->hasFocus(), sticky);
}

void tst_QGraphicsScene::sendEvent()
{
    QGraphicsScene scene;
    QGraphicsTextItem *item = scene.addText(QString());
    EventSpy *spy = new EventSpy(&scene, item, QEvent::User);
    QCOMPARE(spy->count(), 0);
    QEvent event(QEvent::User);
    scene.sendEvent(item, &event);
    QCOMPARE(spy->count(), 1);
}

void tst_QGraphicsScene::inputMethod_data()
{
    QTest::addColumn<QGraphicsItem::GraphicsItemFlags>("flags");
    QTest::addColumn<bool>("callFocusItem");
    QTest::newRow("0") << QGraphicsItem::GraphicsItemFlags() << false;
    QTest::newRow("1") << QGraphicsItem::GraphicsItemFlags(QGraphicsItem::ItemAcceptsInputMethod) << false;
    QTest::newRow("2") << QGraphicsItem::GraphicsItemFlags(QGraphicsItem::ItemIsFocusable) << false;
    QTest::newRow("3") <<
        (QGraphicsItem::ItemAcceptsInputMethod|QGraphicsItem::ItemIsFocusable) << true;
}

class InputMethodTester : public QGraphicsRectItem
{
    void inputMethodEvent(QInputMethodEvent *) override { ++eventCalls; }
    QVariant inputMethodQuery(Qt::InputMethodQuery) const override
    {
        ++queryCalls;
        return QVariant();
    }

public:
    int eventCalls = 0;
    mutable int queryCalls = 0;
};

void tst_QGraphicsScene::inputMethod()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    PlatformInputContext inputContext;
    QInputMethodPrivate *inputMethodPrivate =
        QInputMethodPrivate::get(QGuiApplication::inputMethod());
    inputMethodPrivate->testContext = &inputContext;

    QFETCH(QGraphicsItem::GraphicsItemFlags, flags);
    QFETCH(bool, callFocusItem);

    InputMethodTester *item = new InputMethodTester;
    item->setFlags(flags);

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.resize(m_testSize);
    view.show();
    view.setWindowTitle(QTest::currentTestFunction());
    QApplicationPrivate::setActiveWindow(&view);
    view.setFocus();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(QApplication::activeWindow(), &view);

    inputContext.m_resetCallCount = 0;
    inputContext.m_commitCallCount = 0;
    scene.addItem(item);
    QInputMethodEvent event;

    scene.setFocusItem(item);
    QCOMPARE(!!(item->flags() & QGraphicsItem::ItemIsFocusable), scene.focusItem() == item);
    QCOMPARE(inputContext.m_resetCallCount, 0);

    item->eventCalls = 0;
    QCoreApplication::sendEvent(&scene, &event);
    QCOMPARE(item->eventCalls, callFocusItem ? 1 : 0);

    item->queryCalls = 0;
    scene.inputMethodQuery(Qt::InputMethodQuery(0));
    QCOMPARE(item->queryCalls, callFocusItem ? 1 : 0);

    scene.setFocusItem(nullptr);
    // the input context is reset twice, once because an item has lost focus and again because
    // the Qt::WA_InputMethodEnabled flag is cleared because no item has focus.
    QCOMPARE(inputContext.m_resetCallCount + inputContext.m_commitCallCount, callFocusItem ? 2 : 0);
    QCOMPARE(item->queryCalls, callFocusItem ? 1 : 0); // verify that value is unaffected

    item->eventCalls = 0;
    QCoreApplication::sendEvent(&scene, &event);
    QCOMPARE(item->eventCalls, callFocusItem ? 1 : 0);

    item->queryCalls = 0;
    scene.inputMethodQuery(Qt::InputMethodQuery(0));
    QCOMPARE(item->queryCalls, 0);
}

void tst_QGraphicsScene::dispatchHoverOnPress()
{
    QGraphicsScene scene;
    EventTester *tester1 = new EventTester;
    tester1->setAcceptHoverEvents(true);
    EventTester *tester2 = new EventTester;
    tester2->setAcceptHoverEvents(true);
    tester2->setPos(30, 30);
    scene.addItem(tester1);
    scene.addItem(tester2);

    tester1->eventTypes.clear();
    tester2->eventTypes.clear();

    const QEvent::Type initialExpected[] =
        {QEvent::GraphicsSceneHoverEnter, QEvent::GraphicsSceneHoverMove, QEvent::GrabMouse,
         QEvent::GraphicsSceneMousePress, QEvent::UngrabMouse};

    const QEvent::Type repeatExpected[] =
        {QEvent::GraphicsSceneHoverMove, QEvent::GrabMouse, QEvent::GraphicsSceneMousePress,
         QEvent::UngrabMouse};

    {
        QGraphicsSceneMouseEvent me(QEvent::GraphicsSceneMousePress);
        me.setButton(Qt::LeftButton);
        me.setButtons(Qt::LeftButton);
        QGraphicsSceneMouseEvent me2(QEvent::GraphicsSceneMouseRelease);
        me2.setButton(Qt::LeftButton);
        QCoreApplication::sendEvent(&scene, &me);
        QCoreApplication::sendEvent(&scene, &me2);
        QCOMPARE(tester1->eventTypes, initialExpected);
        tester1->eventTypes.clear();
        QCoreApplication::sendEvent(&scene, &me);
        QCoreApplication::sendEvent(&scene, &me2);
        QCOMPARE(tester1->eventTypes, repeatExpected);
    }
    {
        QGraphicsSceneMouseEvent me(QEvent::GraphicsSceneMousePress);
        me.setScenePos(QPointF(30, 30));
        me.setButton(Qt::LeftButton);
        me.setButtons(Qt::LeftButton);
        QGraphicsSceneMouseEvent me2(QEvent::GraphicsSceneMouseRelease);
        me2.setScenePos(QPointF(30, 30));
        me2.setButton(Qt::LeftButton);
        tester1->eventTypes.clear();
        QCoreApplication::sendEvent(&scene, &me);
        QCoreApplication::sendEvent(&scene, &me2);
        qCDebug(lcTests) << tester1->eventTypes;
        QCOMPARE(tester1->eventTypes, {QEvent::GraphicsSceneHoverLeave});
        QCOMPARE(tester2->eventTypes, initialExpected);
        tester2->eventTypes.clear();
        QCoreApplication::sendEvent(&scene, &me);
        QCoreApplication::sendEvent(&scene, &me2);
        QCOMPARE(tester2->eventTypes, repeatExpected);
    }
}

void tst_QGraphicsScene::initialFocus_data()
{
    QTest::addColumn<bool>("activeScene");
    QTest::addColumn<bool>("explicitSetFocus");
    QTest::addColumn<bool>("isPanel");
    QTest::addColumn<bool>("shouldHaveFocus");

    QTest::newRow("inactive scene, normal item") << false << false << false << false;
    QTest::newRow("inactive scene, panel item") << false << false << true << true;
    QTest::newRow("inactive scene, normal item, explicit focus") << false << true << false << true;
    QTest::newRow("inactive scene, panel, explicit focus") << false << true << true << true;
    QTest::newRow("active scene, normal item") << true << false << false << false;
    QTest::newRow("active scene, panel item") << true << false << true << true;
    QTest::newRow("active scene, normal item, explicit focus") << true << true << false << true;
    QTest::newRow("active scene, panel, explicit focus") << true << true << true << true;
}

void tst_QGraphicsScene::initialFocus()
{
    QFETCH(bool, activeScene);
    QFETCH(bool, explicitSetFocus);
    QFETCH(bool, isPanel);
    QFETCH(bool, shouldHaveFocus);

    QGraphicsRectItem *rect = new QGraphicsRectItem;
    rect->setFlag(QGraphicsItem::ItemIsFocusable);
    QVERIFY(!rect->hasFocus());

    if (isPanel)
        rect->setFlag(QGraphicsItem::ItemIsPanel);

    // Setting focus on an item before adding to the scene will ensure
    // it gets focus when the scene is activated.
    if (explicitSetFocus)
        rect->setFocus();

    QGraphicsScene scene;
    QVERIFY(!scene.isActive());

    if (activeScene) {
        QEvent windowActivate(QEvent::WindowActivate);
        QCoreApplication::sendEvent(&scene, &windowActivate);
        scene.setFocus();
    }

    scene.addItem(rect);

    if (!activeScene) {
        QEvent windowActivate(QEvent::WindowActivate);
        QCoreApplication::sendEvent(&scene, &windowActivate);
        scene.setFocus();
    }

    QCOMPARE(rect->hasFocus(), shouldHaveFocus);
}

class PolishItem : public QGraphicsTextItem
{
public:
    using QGraphicsTextItem::QGraphicsTextItem;

    bool polished = false;
    bool deleteChildrenInPolish = true;
    bool addChildrenInPolish = false;
protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override
    {
        if (change == ItemVisibleChange) {
            polished = true;
            if (deleteChildrenInPolish)
                qDeleteAll(childItems());
            if (addChildrenInPolish) {
                for (int i = 0; i < 10; ++i)
                    new PolishItem(this);
            }
        }
        return QGraphicsItem::itemChange(change, value);
    }
};

void tst_QGraphicsScene::polishItems()
{
    QGraphicsScene scene;
    PolishItem *parent = new PolishItem;
    scene.addItem(parent);
    PolishItem *child = new PolishItem(parent);
    Q_UNUSED(child);
    // test that QGraphicsScenePrivate::_q_polishItems() doesn't crash
    QMetaObject::invokeMethod(&scene,"_q_polishItems");
}

void tst_QGraphicsScene::polishItems2()
{
    QGraphicsScene scene;
    PolishItem *item = new PolishItem;
    item->addChildrenInPolish = true;
    item->deleteChildrenInPolish = true;
    // These children should be deleted in the polish.
    for (int i = 0; i < 20; ++i)
        new PolishItem(item);
    scene.addItem(item);

    // Wait for the polish event to be delivered.
    QVERIFY(!item->polished);
    QCoreApplication::sendPostedEvents(&scene, QEvent::MetaCall);
    QVERIFY(item->polished);

    // We deleted the children we added above, but we also
    // added 10 new children. These should be polished in the next
    // event loop iteration.
    const QList<QGraphicsItem *> children = item->childItems();
    QCOMPARE(children.size(), 10);
    for (QGraphicsItem *child : children)
        QVERIFY(!static_cast<PolishItem *>(child)->polished);

    QCoreApplication::sendPostedEvents(&scene, QEvent::MetaCall);
    for (QGraphicsItem *child : children)
        QVERIFY(static_cast<PolishItem *>(child)->polished);
}

void tst_QGraphicsScene::isActive()
{
#ifdef Q_OS_ANDROID
    QSKIP("Fails on Android (QTBUG-44430)");
#endif
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QGraphicsScene scene1;
    QVERIFY(!scene1.isActive());
    QGraphicsScene scene2;
    QVERIFY(!scene2.isActive());

    {
        QWidget toplevel1;
        toplevel1.setWindowTitle(QTest::currentTestFunction());
        toplevel1.resize(m_testSize);
        QHBoxLayout *layout = new QHBoxLayout;
        toplevel1.setLayout(layout);
        QGraphicsView *view1 = new QGraphicsView(&scene1);
        QGraphicsView *view2 = new QGraphicsView(&scene2);
        layout->addWidget(view1);
        layout->addWidget(view2);

        QVERIFY(!scene1.isActive());
        QVERIFY(!scene2.isActive());

        view1->setVisible(false);

        toplevel1.show();
        QApplicationPrivate::setActiveWindow(&toplevel1);
        QVERIFY(QTest::qWaitForWindowActive(&toplevel1));
        QCOMPARE(QApplication::activeWindow(), &toplevel1);

        QVERIFY(!scene1.isActive()); //it is hidden;
        QVERIFY(scene2.isActive());
        QVERIFY(!scene1.hasFocus());
        QVERIFY(scene2.hasFocus());

        view1->show();
        QVERIFY(scene1.isActive());
        QVERIFY(scene2.isActive());
        QVERIFY(!scene1.hasFocus());
        QVERIFY(scene2.hasFocus());

        view2->hide();

        QVERIFY(scene1.isActive());
        QVERIFY(!scene2.isActive());
        QVERIFY(scene1.hasFocus());
        QVERIFY(!scene2.hasFocus());

        toplevel1.hide();
        QTRY_VERIFY(!scene1.isActive());
        QTRY_VERIFY(!scene2.isActive());
        QVERIFY(!scene1.hasFocus());
        QVERIFY(!scene2.hasFocus());

        toplevel1.show();
        QApplicationPrivate::setActiveWindow(&toplevel1);
        QVERIFY(QTest::qWaitForWindowActive(&toplevel1));
        QCOMPARE(QApplication::activeWindow(), &toplevel1);

        QTRY_VERIFY(scene1.isActive());
        QTRY_VERIFY(!scene2.isActive());
        QVERIFY(scene1.hasFocus());
        QVERIFY(!scene2.hasFocus());

        view2->show();
        QVERIFY(scene1.isActive());
        QVERIFY(scene2.isActive());
        QVERIFY(scene1.hasFocus());
        QVERIFY(!scene2.hasFocus());
    }

    QVERIFY(!scene1.isActive());
    QVERIFY(!scene2.isActive());
    QVERIFY(!scene1.hasFocus());
    QVERIFY(!scene2.hasFocus());


    {
        QWidget toplevel2;
        toplevel2.setWindowTitle(QTest::currentTestFunction());
        toplevel2.resize(m_testSize);
        QHBoxLayout *layout = new QHBoxLayout;
        toplevel2.setLayout(layout);
        QGraphicsView *view1 = new QGraphicsView(&scene1);
        QGraphicsView *view2 = new QGraphicsView();
        layout->addWidget(view1);
        layout->addWidget(view2);

        QVERIFY(!scene1.isActive());
        QVERIFY(!scene2.isActive());
        QVERIFY(!scene1.hasFocus());
        QVERIFY(!scene2.hasFocus());

        toplevel2.move(m_availableGeometry.topLeft() + QPoint(50, 50));
        toplevel2.show();
        QApplicationPrivate::setActiveWindow(&toplevel2);
        QVERIFY(QTest::qWaitForWindowActive(&toplevel2));
        QCOMPARE(QApplication::activeWindow(), &toplevel2);

        QTRY_VERIFY(scene1.isActive());
        QVERIFY(!scene2.isActive());
        QVERIFY(scene1.hasFocus());
        QVERIFY(!scene2.hasFocus());

        view2->setScene(&scene2);

        QVERIFY(scene1.isActive());
        QVERIFY(scene2.isActive());
        QVERIFY(scene1.hasFocus());
        QVERIFY(!scene2.hasFocus());

        view1->setScene(&scene2);
        QVERIFY(!scene1.isActive());
        QVERIFY(scene2.isActive());
        QVERIFY(!scene1.hasFocus());
        QVERIFY(scene2.hasFocus());

        view1->hide();
        QVERIFY(!scene1.isActive());
        QVERIFY(scene2.isActive());
        QVERIFY(!scene1.hasFocus());
        QVERIFY(scene2.hasFocus());

        view1->setScene(&scene1);
        QVERIFY(!scene1.isActive());
        QVERIFY(scene2.isActive());
        QVERIFY(!scene1.hasFocus());
        QVERIFY(scene2.hasFocus());

        view1->show();
        QVERIFY(scene1.isActive());
        QVERIFY(scene2.isActive());
        QVERIFY(!scene1.hasFocus());
        QVERIFY(scene2.hasFocus());

        view2->hide();
        QVERIFY(scene1.isActive());
        QVERIFY(!scene2.isActive());
        QVERIFY(scene1.hasFocus());
        QVERIFY(!scene2.hasFocus());

        QGraphicsView topLevelView;
        topLevelView.move(toplevel2.geometry().topRight() + QPoint(100, 50));
        topLevelView.resize(m_testSize);
        topLevelView.show();
        QApplicationPrivate::setActiveWindow(&topLevelView);
        topLevelView.setFocus();
        QVERIFY(QTest::qWaitForWindowActive(&topLevelView));
        QCOMPARE(QApplication::activeWindow(), &topLevelView);

        QVERIFY(!scene1.isActive());
        QVERIFY(!scene2.isActive());
        QVERIFY(!scene1.hasFocus());
        QVERIFY(!scene2.hasFocus());

        topLevelView.setScene(&scene1);
        QVERIFY(scene1.isActive());
        QVERIFY(!scene2.isActive());
        QVERIFY(scene1.hasFocus());
        QVERIFY(!scene2.hasFocus());

        view2->show();
        QVERIFY(scene1.isActive());
        QVERIFY(!scene2.isActive());
        QVERIFY(scene1.hasFocus());
        QVERIFY(!scene2.hasFocus());

        view1->hide();
        QVERIFY(scene1.isActive());
        QVERIFY(!scene2.isActive());
        QVERIFY(scene1.hasFocus());
        QVERIFY(!scene2.hasFocus());

        QApplicationPrivate::setActiveWindow(&toplevel2);
        QVERIFY(QTest::qWaitForWindowActive(&toplevel2));

        QVERIFY(!scene1.isActive());
        QVERIFY(scene2.isActive());
        QVERIFY(!scene1.hasFocus());
        QVERIFY(scene2.hasFocus());
    }

    QVERIFY(!scene1.isActive());
    QVERIFY(!scene2.isActive());
    QVERIFY(!scene1.hasFocus());
    QVERIFY(!scene2.hasFocus());

    {
        QWidget toplevel3;
        toplevel3.resize(m_testSize);
        QHBoxLayout *layout = new QHBoxLayout;
        toplevel3.setLayout(layout);
        QGraphicsView *view1 = new QGraphicsView(&scene1);
        QGraphicsView *view2 = new QGraphicsView(&scene2);
        layout->addWidget(view1);

        QVERIFY(!scene1.isActive());
        QVERIFY(!scene2.isActive());
        QVERIFY(!scene1.hasFocus());
        QVERIFY(!scene2.hasFocus());


        toplevel3.show();
        QApplicationPrivate::setActiveWindow(&toplevel3);
        QVERIFY(QTest::qWaitForWindowActive(&toplevel3));
        QCOMPARE(QApplication::activeWindow(), &toplevel3);

        QVERIFY(scene1.isActive());
        QVERIFY(!scene2.isActive());
        QVERIFY(scene1.hasFocus());
        QVERIFY(!scene2.hasFocus());

        layout->addWidget(view2);
        QApplication::processEvents();
        QVERIFY(scene1.isActive());
        QVERIFY(scene2.isActive());
        QVERIFY(scene1.hasFocus());
        QVERIFY(!scene2.hasFocus());

        view1->setParent(nullptr);
        QVERIFY(!scene1.isActive());
        QVERIFY(scene2.isActive());
        QVERIFY(!scene1.hasFocus());
        QVERIFY(scene2.hasFocus());
        delete view1;
    }

    QVERIFY(!scene1.isActive());
    QVERIFY(!scene2.isActive());
    QVERIFY(!scene1.hasFocus());
    QVERIFY(!scene2.hasFocus());

}

void tst_QGraphicsScene::siblingIndexAlwaysValid()
{
    QGraphicsScene scene;

    QGraphicsWidget *parent = new QGraphicsWidget;
    parent->setZValue(350);
    parent->setGeometry(0, 0, 100, 100);
    QGraphicsWidget *parent2 = new QGraphicsWidget;
    parent2->setGeometry(10, 10, 50, 50);
    QGraphicsWidget *child = new QGraphicsWidget(parent2);
    child->setGeometry(15, 15, 25, 25);
    child->setZValue(150);
    //Both are top level
    scene.addItem(parent);
    scene.addItem(parent2);

    //Then we make the child a top level
    child->setParentItem(nullptr);

    //This is trigerred by a repaint...
    QGraphicsScenePrivate::get(&scene)->index->estimateTopLevelItems(QRectF(), Qt::AscendingOrder);

    delete child;

    //If there are in the list that's bad, we crash...
    QVERIFY(!QGraphicsScenePrivate::get(&scene)->topLevelItems.contains(static_cast<QGraphicsItem *>(child)));

    //Other case
    QGraphicsScene scene2;
    // works with bsp tree index
    scene2.setItemIndexMethod(QGraphicsScene::NoIndex);

    QGraphicsView view2(&scene2);
    view2.setWindowTitle(QTest::currentTestFunction());
    view2.resize(m_testSize);

    // first add the blue rect
    QGraphicsRectItem* const item1 = new QGraphicsRectItem(QRect( 10, 10, 10, 10 ));
    item1->setPen(QPen(Qt::blue, 0));
    item1->setBrush(Qt::blue);
    scene2.addItem(item1);

    // then add the red rect
    QGraphicsRectItem* const item2 = new QGraphicsRectItem(5, 5, 10, 10);
    item2->setPen(QPen(Qt::red, 0));
    item2->setBrush(Qt::red);
    scene2.addItem(item2);

    // now the blue one is visible on top of the red one -> swap them (important for the bug)
    item1->setZValue(1.0);
    item2->setZValue(0.0);

    view2.show();

    // handle events as a real life app would do
    QApplication::processEvents();

    // now delete the red rect
    delete item2;

    // handle events as a real life app would do
     QApplication::processEvents();

     //We should not crash

}

void tst_QGraphicsScene::removeFullyTransparentItem()
{
    QGraphicsScene scene;

    QGraphicsItem *parent = scene.addRect(0, 0, 100, 100);
    parent->setFlag(QGraphicsItem::ItemHasNoContents);

    QGraphicsItem *child = scene.addRect(0, 0, 100, 100);
    child->setParentItem(parent);

    CustomView view;
    view.setWindowTitle(QTest::currentTestFunction());
    view.resize(m_testSize);
    view.setScene(&scene);
    view.show();
    QApplicationPrivate::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCoreApplication::processEvents(); // Process all queued paint events

    // NB! The parent has the ItemHasNoContents flag set, which means
    // the parent itself doesn't generate any update requests, only the
    // child can possibly trigger an update. Also note that the child
    // is removed before processing events.
    view.repaints = 0;
    parent->setOpacity(0);
    QVERIFY(qFuzzyIsNull(child->effectiveOpacity()));
    scene.removeItem(child);
    QVERIFY(!scene.items().contains(child));
    QTRY_VERIFY(view.repaints > 0);

    // Re-add child. There's nothing new to display (child is still
    // effectively hidden), so it shouldn't trigger an update.
    view.repaints = 0;
    child->setParentItem(parent);
    QVERIFY(scene.items().contains(child));
    QVERIFY(qFuzzyIsNull(child->effectiveOpacity()));
    QApplication::processEvents();
    QCOMPARE(view.repaints, 0);

    // Nothing is visible on the screen, removing child item shouldn't trigger an update.
    scene.removeItem(child);
    QApplication::processEvents();
    QCOMPARE(view.repaints, 0);
    delete child;
}

void tst_QGraphicsScene::taskQTBUG_5904_crashWithDeviceCoordinateCache()
{
    QGraphicsScene scene;
    QGraphicsRectItem *rectItem = scene.addRect(QRectF(0, 0, 100, 200), QPen(Qt::black), QBrush(Qt::green));

    rectItem->setCacheMode(QGraphicsItem::DeviceCoordinateCache);

    QPixmap pixmap(100,200);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    scene.render(&painter);
    painter.end();
    // No crash, then it passed!
}

void tst_QGraphicsScene::taskQT657_paintIntoCacheWithTransparentParts()
{
    // Test using DeviceCoordinateCache and opaque item
    QScopedPointer<QWidget> w(new QWidget);
    w->setPalette(QColor(0, 0, 255));
    w->setGeometry(0, 0, 50, 50);

    QGraphicsScene *scene = new QGraphicsScene();
    CustomView view;
    view.resize(m_testSize);
    view.setWindowTitle(QTest::currentTestFunction());
    view.setScene(scene);

    QGraphicsProxyWidget *proxy = scene->addWidget(w.data());
    proxy->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    proxy->setTransform(QTransform().rotate(15), true);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.repaints = 0;
    proxy->update(10, 10, 10, 10);
    QTRY_VERIFY(view.repaints > 0);

    QPixmap pix;
    QGraphicsItemPrivate* itemp = QGraphicsItemPrivate::get(proxy);
    QTRY_VERIFY(QPixmapCache::find(itemp->extraItemCache()->deviceData.value(view.viewport()).key, &pix));

    QTransform t = proxy->sceneTransform();
    // Map from scene coordinates to pixmap coordinates.
    // X origin in the pixmap is the most-left point
    // of the item's boundingRect in the scene.
    const int adjust = t.mapRect(proxy->boundingRect().toRect()).left();
    QRect rect = t.mapRect(QRect(10, 10, 10, 10)).adjusted(-adjust, 0, -adjust + 1, 1);
    QPixmap subpix = pix.copy(rect);

    QImage im = subpix.toImage();
    for(int i = 0; i < im.width(); i++) {
        for(int j = 0; j < im.height(); j++)
            QCOMPARE(qAlpha(im.pixel(i, j)), 255);
    }
}

void tst_QGraphicsScene::taskQTBUG_7863_paintIntoCacheWithTransparentParts()
{
    // Test using DeviceCoordinateCache and semi-transparent item
    {
        QGraphicsRectItem *backItem = new QGraphicsRectItem(0, 0, 100, 100);
        backItem->setBrush(QColor(255, 255, 0));
        QGraphicsRectItem *rectItem = new QGraphicsRectItem(0, 0, 50, 50);
        rectItem->setBrush(QColor(0, 0, 255, 125));
        rectItem->setParentItem(backItem);

        QGraphicsScene *scene = new QGraphicsScene();
        CustomView view;
        view.resize(m_testSize);
        view.setWindowTitle(QTest::currentTestFunction());
        view.setScene(scene);

        scene->addItem(backItem);
        rectItem->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
        backItem->setTransform(QTransform().rotate(15), true);

        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        view.repaints = 0;
        rectItem->update(10, 10, 10, 10);
        QTRY_VERIFY(view.repaints > 0);

        QPixmap pix;
        QGraphicsItemPrivate* itemp = QGraphicsItemPrivate::get(rectItem);
        QTRY_VERIFY(QPixmapCache::find(itemp->extraItemCache()->deviceData.value(view.viewport()).key, &pix));

        QTransform t = rectItem->sceneTransform();
        // Map from scene coordinates to pixmap coordinates.
        // X origin in the pixmap is the most-left point
        // of the item's boundingRect in the scene.
        const int adjust = t.mapRect(rectItem->boundingRect().toRect()).left();
        QRect rect = t.mapRect(QRect(10, 10, 10, 10)).adjusted(-adjust, 0, -adjust + 1, 1);
        QPixmap subpix = pix.copy(rect);

        QImage im = subpix.toImage();
        for(int i = 0; i < im.width(); i++) {
            for(int j = 0; j < im.height(); j++) {
                QCOMPARE(qAlpha(im.pixel(i, j)), 125);
            }
        }
    }

    // Test using ItemCoordinateCache and opaque item
    {
        QGraphicsRectItem *rectItem = new QGraphicsRectItem(0, 0, 50, 50);
        rectItem->setBrush(QColor(0, 0, 255));

        QGraphicsScene *scene = new QGraphicsScene();
        CustomView view;
        view.setWindowTitle(QTest::currentTestFunction());
        view.resize(m_testSize);
        view.setScene(scene);

        scene->addItem(rectItem);
        rectItem->setCacheMode(QGraphicsItem::ItemCoordinateCache);
        rectItem->setTransform(QTransform().rotate(15), true);

        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        view.repaints = 0;
        rectItem->update(10, 10, 10, 10);
        QTRY_VERIFY(view.repaints > 0);

        QPixmap pix;
        QGraphicsItemPrivate* itemp = QGraphicsItemPrivate::get(rectItem);
        QTRY_VERIFY(QPixmapCache::find(itemp->extraItemCache()->key, &pix));

        QTransform t = rectItem->sceneTransform();
        // Map from scene coordinates to pixmap coordinates.
        // X origin in the pixmap is the most-left point
        // of the item's boundingRect in the scene.
        const int adjust = t.mapRect(rectItem->boundingRect().toRect()).left();
        QRect rect = t.mapRect(QRect(10, 10, 10, 10)).adjusted(-adjust, 0, -adjust + 1, 1);
        QPixmap subpix = pix.copy(rect);

        QImage im = subpix.toImage();
        for(int i = 0; i < im.width(); i++) {
            for(int j = 0; j < im.height(); j++)
                QCOMPARE(qAlpha(im.pixel(i, j)), 255);
        }
    }

    // Test using ItemCoordinateCache and semi-transparent item
    {
        QGraphicsRectItem *rectItem = new QGraphicsRectItem(0, 0, 50, 50);
        rectItem->setBrush(QColor(0, 0, 255, 125));

        QGraphicsScene *scene = new QGraphicsScene();
        CustomView view;
        view.setWindowTitle(QTest::currentTestFunction());
        view.resize(m_testSize);
        view.setScene(scene);

        scene->addItem(rectItem);
        rectItem->setCacheMode(QGraphicsItem::ItemCoordinateCache);
        rectItem->setTransform(QTransform().rotate(15), true);

        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        view.repaints = 0;
        rectItem->update(10, 10, 10, 10);
        QTRY_VERIFY(view.repaints > 0);

        QPixmap pix;
        QGraphicsItemPrivate* itemp = QGraphicsItemPrivate::get(rectItem);
        QTRY_VERIFY(QPixmapCache::find(itemp->extraItemCache()->key, &pix));

        QTransform t = rectItem->sceneTransform();
        // Map from scene coordinates to pixmap coordinates.
        // X origin in the pixmap is the most-left point
        // of the item's boundingRect in the scene.
        const int adjust = int(t.mapRect(rectItem->boundingRect().toRect()).left());
        QRect rect = t.mapRect(QRect(10, 10, 10, 10)).adjusted(-adjust, 0, -adjust + 1, 1);
        QPixmap subpix = pix.copy(rect);

        QImage im = subpix.toImage();
        for(int i = 0; i < im.width(); i++) {
            for(int j = 0; j < im.height(); j++)
                QCOMPARE(qAlpha(im.pixel(i, j)), 125);
        }
    }
}

void tst_QGraphicsScene::taskQT_3674_doNotCrash()
{
    QGraphicsScene scene;

    QGraphicsView view(&scene);
    view.resize(200, 200);

    QPixmap pixmap(view.size());
    QPainter painter(&pixmap);
    view.render(&painter);
    painter.end();

    scene.addItem(new QGraphicsWidget);
    scene.setBackgroundBrush(Qt::green);

    QApplication::processEvents();
    QApplication::processEvents();
}

void tst_QGraphicsScene::zeroScale()
{
    //should not crash
    QGraphicsScene scene;
    scene.setSceneRect(-100, -100, 100, 100);
    QGraphicsView view(&scene);

    ChangedListener cl;
    connect(&scene, &QGraphicsScene::changed, &cl, &ChangedListener::changed);

    QGraphicsRectItem *rect1 = new QGraphicsRectItem(0, 0, 0.0000001, 0.00000001);
    scene.addItem(rect1);
    rect1->setRotation(82);
    rect1->setScale(0.00000001);

    QApplication::processEvents();
    QTRY_COMPARE(cl.changes.size(), 1);
    QGraphicsRectItem *rect2 = new QGraphicsRectItem(-0.0000001, -0.0000001, 0.0000001, 0.0000001);
    rect2->setScale(0.00000001);
    scene.addItem(rect2);
    rect1->setPos(20,20);
    QApplication::processEvents();
    QTRY_COMPARE(cl.changes.size(), 2);
}

void tst_QGraphicsScene::focusItemChangedSignal()
{
    qRegisterMetaType<QGraphicsItem *>("QGraphicsItem *");
    qRegisterMetaType<Qt::FocusReason>("Qt::FocusReason");

    QGraphicsScene scene;
    QSignalSpy spy(&scene, &QGraphicsScene::focusItemChanged);
    QVERIFY(spy.isValid());
    QCOMPARE(spy.size(), 0);
    scene.setFocus();
    QCOMPARE(spy.size(), 0);
    QEvent activateEvent(QEvent::WindowActivate);
    QCoreApplication::sendEvent(&scene, &activateEvent);
    QCOMPARE(spy.size(), 0);

    QGraphicsRectItem *topLevelItem1 = new QGraphicsRectItem;
    topLevelItem1->setFlag(QGraphicsItem::ItemIsFocusable);
    scene.addItem(topLevelItem1);
    QCOMPARE(spy.size(), 0);
    QVERIFY(!topLevelItem1->hasFocus());

    QGraphicsRectItem *topLevelItem2 = new QGraphicsRectItem;
    topLevelItem2->setFlag(QGraphicsItem::ItemIsFocusable);
    topLevelItem2->setFocus();
    QVERIFY(!topLevelItem2->hasFocus());
    scene.addItem(topLevelItem2);
    QCOMPARE(spy.size(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.size(), 3);
    QCOMPARE(qvariant_cast<QGraphicsItem *>(arguments.at(0)), topLevelItem2);
    QCOMPARE(qvariant_cast<QGraphicsItem *>(arguments.at(1)), nullptr);
    QCOMPARE(qvariant_cast<Qt::FocusReason>(arguments.at(2)), Qt::OtherFocusReason);
    QVERIFY(topLevelItem2->hasFocus());

    scene.clearFocus();
    QCOMPARE(spy.size(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.size(), 3);
    QCOMPARE(qvariant_cast<QGraphicsItem *>(arguments.at(0)), nullptr);
    QCOMPARE(qvariant_cast<QGraphicsItem *>(arguments.at(1)), topLevelItem2);
    QCOMPARE(qvariant_cast<Qt::FocusReason>(arguments.at(2)), Qt::OtherFocusReason);

    scene.setFocus(Qt::MenuBarFocusReason);
    QCOMPARE(spy.size(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.size(), 3);
    QCOMPARE(qvariant_cast<QGraphicsItem *>(arguments.at(0)), topLevelItem2);
    QCOMPARE(qvariant_cast<QGraphicsItem *>(arguments.at(1)), nullptr);
    QCOMPARE(qvariant_cast<Qt::FocusReason>(arguments.at(2)), Qt::MenuBarFocusReason);

    for (int i = 0; i < 3; ++i) {
        topLevelItem1->setFocus(Qt::TabFocusReason);
        arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 3);
        QCOMPARE(qvariant_cast<QGraphicsItem *>(arguments.at(0)), topLevelItem1);
        QCOMPARE(qvariant_cast<QGraphicsItem *>(arguments.at(1)), topLevelItem2);
        QCOMPARE(qvariant_cast<Qt::FocusReason>(arguments.at(2)), Qt::TabFocusReason);

        topLevelItem2->setFocus(Qt::TabFocusReason);
        arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 3);
        QCOMPARE(qvariant_cast<QGraphicsItem *>(arguments.at(0)), topLevelItem2);
        QCOMPARE(qvariant_cast<QGraphicsItem *>(arguments.at(1)), topLevelItem1);
        QCOMPARE(qvariant_cast<Qt::FocusReason>(arguments.at(2)), Qt::TabFocusReason);
    }

    // The following two are unexpected, but fixing this (i.e., losing and gaining focus
    // when the scene activation changes) breaks quite a few tests so leave this fix
    // for some future release. See QTBUG-28346.
    QEvent deactivateEvent(QEvent::WindowDeactivate);
    QCoreApplication::sendEvent(&scene, &deactivateEvent);
    QEXPECT_FAIL("", "QTBUG-28346", Continue);
    QCOMPARE(spy.size(), 1);
    QCoreApplication::sendEvent(&scene, &activateEvent);
    QEXPECT_FAIL("", "QTBUG-28346", Continue);
    QCOMPARE(spy.size(), 1);

    QGraphicsRectItem *panel1 = new QGraphicsRectItem;
    panel1->setFlags(QGraphicsItem::ItemIsPanel | QGraphicsItem::ItemIsFocusable);
    panel1->setFocus();
    scene.addItem(panel1);
    QCOMPARE(spy.size(), 1);
    arguments = spy.takeFirst();
    QCOMPARE(arguments.size(), 3);
    QCOMPARE(qvariant_cast<QGraphicsItem *>(arguments.at(0)), panel1);
    QCOMPARE(qvariant_cast<QGraphicsItem *>(arguments.at(1)), topLevelItem2);
    QCOMPARE(qvariant_cast<Qt::FocusReason>(arguments.at(2)), Qt::ActiveWindowFocusReason);

    QGraphicsRectItem *panel2 = new QGraphicsRectItem;
    panel2->setFlags(QGraphicsItem::ItemIsPanel | QGraphicsItem::ItemIsFocusable);
    scene.addItem(panel2);
    QCOMPARE(spy.size(), 0);

    for (int i = 0; i < 3; ++i) {
        scene.setActivePanel(panel2);
        QCOMPARE(spy.size(), 1);
        arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 3);
        QCOMPARE(qvariant_cast<QGraphicsItem *>(arguments.at(0)), panel2);
        QCOMPARE(qvariant_cast<QGraphicsItem *>(arguments.at(1)), panel1);
        QCOMPARE(qvariant_cast<Qt::FocusReason>(arguments.at(2)), Qt::ActiveWindowFocusReason);

        scene.setActivePanel(panel1);
        QCOMPARE(spy.size(), 1);
        arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 3);
        QCOMPARE(qvariant_cast<QGraphicsItem *>(arguments.at(0)), panel1);
        QCOMPARE(qvariant_cast<QGraphicsItem *>(arguments.at(1)), panel2);
        QCOMPARE(qvariant_cast<Qt::FocusReason>(arguments.at(2)), Qt::ActiveWindowFocusReason);
    }

}

class ItemCountsPaintCalls : public QGraphicsRectItem
{
public:
    using QGraphicsRectItem::QGraphicsRectItem;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override
    {
        QGraphicsRectItem::paint(painter, option, widget);
        ++repaints;
    }
    int repaints = 0;
};

void tst_QGraphicsScene::minimumRenderSize()
{
    Q_CHECK_PAINTEVENTS

    ItemCountsPaintCalls *bigParent = new ItemCountsPaintCalls(QRectF(0,0,100,100));
    ItemCountsPaintCalls *smallChild = new ItemCountsPaintCalls(QRectF(0,0,10,10), bigParent);
    ItemCountsPaintCalls *smallerGrandChild = new ItemCountsPaintCalls(QRectF(0,0,1,1), smallChild);
    QGraphicsScene scene;
    scene.addItem(bigParent);

    CustomView view;
    view.setWindowTitle(QTest::currentTestFunction());
    view.resize(m_testSize);
    view.setScene(&scene);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QCoreApplication::processEvents();

    // Initially, everything should be repainted the same number of times
    int viewRepaints = 0;
    QTRY_VERIFY(view.repaints > viewRepaints);
    viewRepaints = view.repaints;

    QCOMPARE(viewRepaints, bigParent->repaints);
    QCOMPARE(viewRepaints, smallChild->repaints);
    QCOMPARE(viewRepaints, smallerGrandChild->repaints);

    // Setting a minimum render size should cause a repaint
    scene.setMinimumRenderSize(0.5);
    QCoreApplication::processEvents();

    QTRY_VERIFY(view.repaints > viewRepaints);
    viewRepaints = view.repaints;

    QCOMPARE(viewRepaints, bigParent->repaints);
    QCOMPARE(viewRepaints, smallChild->repaints);
    QCOMPARE(viewRepaints, smallerGrandChild->repaints);

    // Scaling should cause a repaint of big items only.
    view.scale(0.1, 0.1);
    QCoreApplication::processEvents();

    QTRY_VERIFY(view.repaints > viewRepaints);
    viewRepaints = view.repaints;

    QCOMPARE(viewRepaints, bigParent->repaints);
    QCOMPARE(viewRepaints, smallChild->repaints);
    QVERIFY(smallChild->repaints > smallerGrandChild->repaints);

    // Scaling further should cause even fewer items to be repainted
    view.scale(0.1, 0.1); // Stacks with previous scale
    QCoreApplication::processEvents();

    QTRY_VERIFY(view.repaints > viewRepaints);
    viewRepaints = view.repaints;

    QCOMPARE(viewRepaints, bigParent->repaints);
    QVERIFY(bigParent->repaints > smallChild->repaints);
    QVERIFY(smallChild->repaints > smallerGrandChild->repaints);
}

void tst_QGraphicsScene::focusOnTouch()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setWindowTitle(QTest::currentTestFunction());
    view.resize(m_testSize);
    scene.setSceneRect(0, 0, 100, 100);
    QGraphicsRectItem *rect = scene.addRect(0, 0, 100, 100);
    rect->setFlag(QGraphicsItem::ItemIsFocusable, true);

    view.show();
    QApplicationPrivate::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QVERIFY(!rect->hasFocus());

    scene.setFocusOnTouch(false);

    QPointingDevice device("fake touchpad", 4321, QInputDevice::DeviceType::TouchPad,
                           QPointingDevice::PointerType::Finger, QInputDevice::Capability::Position, 1, 3);

    auto touchPoints = QList<QEventPoint>() <<
        QEventPoint(0, QEventPoint::State::Pressed, {10, 10}, {});
    QTouchEvent event(QEvent::TouchBegin, &device, Qt::NoModifier, touchPoints);

    QApplication::sendEvent(&scene, &event);

    QVERIFY(!rect->hasFocus());
    scene.setFocusOnTouch(true);

    QApplication::sendEvent(&scene, &event);
    QVERIFY(rect->hasFocus());
}

void tst_QGraphicsScene::clearSelection()
{
    class AlwaysSelectedItem : public QGraphicsRectItem
    {
    public:
        using QGraphicsRectItem::QGraphicsRectItem;
    protected:
        QVariant itemChange(GraphicsItemChange change, const QVariant& value) override
        {
            if (change == ItemSelectedChange)
                return true;
            return QGraphicsRectItem::itemChange(change, value);
        }
    };
    QGraphicsScene scene;
    QSignalSpy spy(&scene, &QGraphicsScene::selectionChanged);

    QGraphicsRectItem *regularRect = new QGraphicsRectItem;
    regularRect->setFlag(QGraphicsItem::ItemIsSelectable);
    regularRect->setRect(0, 0, 50, 50);
    regularRect->setSelected(true);
    AlwaysSelectedItem *selectedRect = new AlwaysSelectedItem;
    selectedRect->setFlag(QGraphicsItem::ItemIsSelectable);
    selectedRect->setRect(50, 50, 50, 50);
    selectedRect->setSelected(true);
    scene.addItem(regularRect);
    scene.addItem(selectedRect);

    QCOMPARE(spy.size(), 2);

    QCOMPARE(scene.selectedItems().size(), 2);
    scene.clearSelection();
    QVERIFY(!regularRect->isSelected());
    QVERIFY(selectedRect->isSelected());
    QCOMPARE(scene.selectedItems().size(), 1);
    QCOMPARE(spy.size(), 3);

    delete regularRect;
    QCOMPARE(spy.size(), 3);

    scene.clearSelection();
    QCOMPARE(spy.size(), 3);

    delete selectedRect;
    QCOMPARE(spy.size(), 4);
}

void tst_QGraphicsScene::taskQTBUG_15977_renderWithDeviceCoordinateCache()
{
    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 100, 100);
    QGraphicsRectItem *rect = scene.addRect(0, 0, 100, 100);
    rect->setPen(Qt::NoPen);
    rect->setBrush(Qt::red);
    rect->setCacheMode(QGraphicsItem::DeviceCoordinateCache);

    QImage image(100, 100, QImage::Format_RGB32);
    QPainter p(&image);
    scene.render(&p);
    p.end();

    QImage expected(100, 100, QImage::Format_RGB32);
    p.begin(&expected);
    p.fillRect(expected.rect(), Qt::red);
    p.end();

    QCOMPARE(image, expected);
}

void tst_QGraphicsScene::taskQTBUG_16401_focusItem()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.resize(m_testSize);
    QGraphicsRectItem *rect = scene.addRect(0, 0, 100, 100);
    rect->setFlag(QGraphicsItem::ItemIsFocusable);

    view.show();
    QApplicationPrivate::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QVERIFY(!scene.focusItem());

    rect->setFocus();
    QCOMPARE(scene.focusItem(), rect);
    QFocusEvent focusOut(QEvent::FocusOut);
    QApplication::sendEvent(&view, &focusOut);
    QVERIFY(!scene.focusItem());
    QFocusEvent focusIn(QEvent::FocusIn);
    QApplication::sendEvent(&view, &focusIn);
    QCOMPARE(scene.focusItem(), rect);

    rect->clearFocus();
    QVERIFY(!scene.focusItem());
    QApplication::sendEvent(&view, &focusOut);
    QVERIFY(!scene.focusItem());
    QApplication::sendEvent(&view, &focusIn);
    QVERIFY(!scene.focusItem());
}

void tst_QGraphicsScene::taskQTBUG_42915_focusNextPrevChild()
{
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setWindowTitle(QTest::currentTestFunction());
    view.resize(m_testSize);
    scene.setSceneRect(1, 1, 198, 198);
    view.setFocus();

    QGraphicsWidget *widget1 = new QGraphicsWidget();
    QGraphicsRectItem *rect1 = new QGraphicsRectItem(-50, -50, 100, 100, widget1);
    rect1->setBrush(Qt::blue);
    scene.addItem(widget1);
    widget1->setPos(100, 100);
    widget1->setFlags(QGraphicsItem::ItemIsPanel);

    QGraphicsWidget *widget2 = new QGraphicsWidget(widget1);
    widget2->setFocusPolicy(Qt::NoFocus);

    view.show();
    QApplicationPrivate::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QTest::keyEvent(QTest::Click, &view, Qt::Key_Tab);
}

void tst_QGraphicsScene::taskQTBUG_85088_previewTextfailWhenLostFocus()
{
    QString str = "simpleTextItem";
    QGraphicsScene scene;
    QGraphicsView view(&scene);

    QGraphicsTextItem *simpleTextItem = new QGraphicsTextItem;
    simpleTextItem->setFlag(QGraphicsTextItem::ItemIsFocusScope);
    simpleTextItem->setTextInteractionFlags(Qt::TextEditorInteraction);
    simpleTextItem->setPlainText(str);
    scene.addItem(simpleTextItem);

    scene.setFocusItem(simpleTextItem);
    view.setGeometry(100, 100, 600, 400);
    view.show();

    QInputMethodEvent inputEvent;
    QString &preedictStr = const_cast<QString &>(inputEvent.preeditString());
    preedictStr = str;
    QApplication::sendEvent(&scene, &inputEvent);
    QCOMPARE(simpleTextItem->toPlainText(), str);

    // focusItem will lose focus
    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPointF(0, 0),
                           view.viewport()->mapToGlobal(QPointF()),
                           Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(view.viewport(), &pressEvent);

    preedictStr.clear();
    inputEvent.setCommitString(str);
    QApplication::sendEvent(&scene, &inputEvent);
    QCOMPARE(simpleTextItem->toPlainText(), str + str);
}

void tst_QGraphicsScene::deleteItemsOnChange()
{
    QGraphicsScene scene;

    class SelectionItem : public QGraphicsRectItem {
    public:
        QRectF boundingRect() const override { return QRectF(); }
    };

    class ChangeItem : public QGraphicsItem
    {
    public:
        ChangeItem()
        {
            setFlag(QGraphicsItem::ItemIsSelectable, true);
            setFlag(QGraphicsItem::ItemIsMovable, true);
        }
        QRectF boundingRect() const override
        {
            return QRectF(0,0,100,100);
        }

    protected:
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
        {
            painter->fillRect(boundingRect().toRect(), isSelected() ? Qt::yellow : Qt::cyan);
        }

        QVariant itemChange(GraphicsItemChange change, const QVariant &value) override
        {
            if (change != QGraphicsItem::ItemSelectedHasChanged)
                return QGraphicsItem::itemChange(change, value);
            if (value.toBool()) {
                selectionRect = new SelectionItem;
                scene()->addItem(selectionRect);
            } else {
                // this recreates the selectedItems QSet inside of QGraphicsScene,
                // invalidating iterators. See QTBUG-101651.
                scene()->selectedItems();
                delete selectionRect;
                selectionRect = nullptr;
            }
            return QGraphicsItem::itemChange(change, value);
        }
    private:
        SelectionItem *selectionRect = nullptr;
    };

    ChangeItem item1;
    item1.setPos(0, 0);
    ChangeItem item2;
    item1.setPos(50, 50);

    scene.addItem(&item1);
    scene.addItem(&item2);

    QGraphicsView view;
    view.setScene(&scene);
    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));

    // this should not crash - see QTBUG-101651
    QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, QPoint(120, 120));
    QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, QPoint(25, 25));
}

QTEST_MAIN(tst_QGraphicsScene)
#include "tst_qgraphicsscene.moc"
