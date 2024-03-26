// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qlist.h>

#include <qtest.h>

#include <QtQml/QQmlComponent>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitemgrabresult.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtQuick/private/qquickimage_p_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>
#include <QtQuickTemplates2/private/qquickicon_p.h>
#include <QtQuickControls2Impl/private/qquickiconimage_p.h>
#include <QtQuickControls2Impl/private/qquickiconlabel_p.h>
#include <QtQuickControls2Impl/private/qquickiconlabel_p_p.h>

using namespace QQuickVisualTestUtils;

class tst_qquickiconlabel : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickiconlabel();

private slots:
    void display_data();
    void display();
    void spacingWithOneDelegate_data();
    void spacingWithOneDelegate();
    void emptyIconSource();
    void colorChanges();
    void iconSourceContext();
};

tst_qquickiconlabel::tst_qquickiconlabel()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qquickiconlabel::display_data()
{
    QTest::addColumn<QList<QQuickIconLabel::Display> >("displayTypes");
    QTest::addColumn<bool>("mirrored");
    QTest::addColumn<qreal>("labelWidth");
    QTest::addColumn<qreal>("labelHeight");
    QTest::addColumn<qreal>("spacing");

    typedef QList<QQuickIconLabel::Display> DisplayVector;
    QQuickIconLabel::Display IconOnly = QQuickIconLabel::IconOnly;
    QQuickIconLabel::Display TextOnly = QQuickIconLabel::TextOnly;
    QQuickIconLabel::Display TextUnderIcon = QQuickIconLabel::TextUnderIcon;
    QQuickIconLabel::Display TextBesideIcon = QQuickIconLabel::TextBesideIcon;

    QTest::addRow("IconOnly") << (DisplayVector() << IconOnly) << false << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextOnly") << (DisplayVector() << TextOnly) << false << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextUnderIcon") << (DisplayVector() << TextUnderIcon) << false << -1.0 << -1.0 << 10.0;
    QTest::addRow("TextBesideIcon") << (DisplayVector() << TextBesideIcon) << false << -1.0 << -1.0 << 10.0;
    QTest::addRow("IconOnly, spacing=10") << (DisplayVector() << IconOnly) << false << -1.0 << -1.0 << 10.0;
    QTest::addRow("TextOnly, spacing=10") << (DisplayVector() << TextOnly) << false << -1.0 << -1.0 << 10.0;
    QTest::addRow("TextUnderIcon, spacing=10") << (DisplayVector() << TextUnderIcon) << false << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextUnderIcon => IconOnly => TextUnderIcon")
        << (DisplayVector() << TextUnderIcon << IconOnly << TextUnderIcon) << false << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextUnderIcon => IconOnly => TextUnderIcon, labelWidth=400")
        << (DisplayVector() << TextUnderIcon << IconOnly << TextUnderIcon) << false << 400.0 << -1.0 << 0.0;
    QTest::addRow("TextUnderIcon => TextOnly => TextUnderIcon")
        << (DisplayVector() << TextUnderIcon << TextOnly << TextUnderIcon) << false << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextUnderIcon => TextOnly => TextUnderIcon, labelWidth=400")
        << (DisplayVector() << TextUnderIcon << TextOnly << TextUnderIcon) << false << 400.0 << -1.0 << 0.0;
    QTest::addRow("TextBesideIcon, spacing=10") << (DisplayVector() << TextBesideIcon) << false << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextBesideIcon => IconOnly => TextBesideIcon")
        << (DisplayVector() << TextBesideIcon << IconOnly << TextBesideIcon) << false << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextBesideIcon => IconOnly => TextBesideIcon, labelWidth=400")
        << (DisplayVector() << TextBesideIcon << IconOnly << TextBesideIcon) << false << 400.0 << -1.0 << 0.0;
    QTest::addRow("TextBesideIcon => TextOnly => TextBesideIcon")
        << (DisplayVector() << TextBesideIcon << TextOnly << TextBesideIcon) << false << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextBesideIcon => TextOnly => TextBesideIcon, labelWidth=400")
        << (DisplayVector() << TextBesideIcon << TextOnly << TextBesideIcon) << false << 400.0 << -1.0 << 0.0;
    QTest::addRow("IconOnly, mirrored") << (DisplayVector() << IconOnly) << true << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextOnly, mirrored") << (DisplayVector() << TextOnly) << true << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextUnderIcon, mirrored") << (DisplayVector() << TextUnderIcon) << true << -1.0 << -1.0 << 0.0;
    QTest::addRow("TextBesideIcon, mirrored") << (DisplayVector() << TextBesideIcon) << true << -1.0 << -1.0 << 0.0;
}

void tst_qquickiconlabel::display()
{
    QFETCH(QList<QQuickIconLabel::Display>, displayTypes);
    QFETCH(bool, mirrored);
    QFETCH(qreal, labelWidth);
    QFETCH(qreal, labelHeight);
    QFETCH(qreal, spacing);

    QQuickView view(testFileUrl("iconlabel.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem *rootItem = view.rootObject();
    QVERIFY(rootItem);

    QQuickIconLabel *label = rootItem->findChild<QQuickIconLabel *>();
    QVERIFY(label);
    QCOMPARE(label->spacing(), 0.0);
    QCOMPARE(label->display(), QQuickIconLabel::TextBesideIcon);
    QCOMPARE(label->isMirrored(), false);

    // Setting labelWidth allows us to test the issue where the icon's
    // width was not updated after switching between different display types.
    if (!qFuzzyCompare(labelWidth, -1)) {
        label->setWidth(labelWidth);
        QCOMPARE(label->width(), labelWidth);
    }
    if (!qFuzzyCompare(labelHeight, -1)) {
        label->setHeight(labelHeight);
        QCOMPARE(label->height(), labelHeight);
    }

    label->setMirrored(mirrored);
    QCOMPARE(label->isMirrored(), mirrored);

    label->setSpacing(spacing);
    QCOMPARE(label->spacing(), spacing);

    const qreal horizontalPadding = label->leftPadding() + label->rightPadding();
    const qreal verticalPadding = label->topPadding() + label->bottomPadding();

    // Test that the icon and text are correctly positioned and sized after
    // setting several different display types in succession.
    for (QQuickIconLabel::Display displayType : std::as_const(displayTypes)) {
        label->setDisplay(displayType);
        QCOMPARE(label->display(), displayType);

        QQuickIconImage *icon = label->findChild<QQuickIconImage *>();
        QQuickText *text = label->findChild<QQuickText *>();

        const qreal horizontalCenter = label->width() / 2;
        const qreal verticalCenter = label->height() / 2;

        switch (displayType) {
        case QQuickIconLabel::IconOnly:
            QVERIFY(icon);
            QVERIFY(!text);
            QCOMPARE(icon->x(), horizontalCenter - icon->width() / 2);
            QCOMPARE(icon->y(), verticalCenter - icon->height() / 2);
            QCOMPARE(icon->width(), icon->implicitWidth());
            QCOMPARE(icon->height(), icon->implicitHeight());
            QCOMPARE(label->implicitWidth(), icon->implicitWidth() + horizontalPadding);
            QCOMPARE(label->implicitHeight(), icon->implicitHeight() + verticalPadding);
            break;
        case QQuickIconLabel::TextOnly:
            QVERIFY(!icon);
            QVERIFY(text);
            QCOMPARE(text->x(), horizontalCenter - text->width() / 2);
            QCOMPARE(text->y(), verticalCenter - text->height() / 2);
            QCOMPARE(text->width(), text->implicitWidth());
            QCOMPARE(text->height(), text->implicitHeight());
            QCOMPARE(label->implicitWidth(), text->implicitWidth() + horizontalPadding);
            QCOMPARE(label->implicitHeight(), text->implicitHeight() + verticalPadding);
            break;
        case QQuickIconLabel::TextUnderIcon: {
            QVERIFY(icon);
            QVERIFY(text);
            const qreal combinedHeight = icon->height() + label->spacing() + text->height();
            const qreal contentY = verticalCenter - combinedHeight / 2;
            QCOMPARE(icon->x(), horizontalCenter - icon->width() / 2);
            QCOMPARE(icon->y(), contentY);
            QCOMPARE(icon->width(), icon->implicitWidth());
            QCOMPARE(icon->height(), icon->implicitHeight());
            QCOMPARE(text->x(), horizontalCenter - text->width() / 2);
            QCOMPARE(text->y(), contentY + icon->height() + label->spacing());
            QCOMPARE(text->width(), text->implicitWidth());
            QCOMPARE(text->height(), text->implicitHeight());
            QCOMPARE(label->implicitWidth(), qMax(icon->implicitWidth(), text->implicitWidth()) + horizontalPadding);
            QCOMPARE(label->implicitHeight(), combinedHeight + verticalPadding);
            break;
        }
        case QQuickIconLabel::TextBesideIcon:
        default:
            QVERIFY(icon);
            QVERIFY(text);
            const qreal combinedWidth = icon->width() + label->spacing() + text->width();
            const qreal contentX = horizontalCenter - combinedWidth / 2;
            QCOMPARE(icon->x(), contentX + (label->isMirrored() ? text->width() + label->spacing() : 0));
            QCOMPARE(icon->y(), verticalCenter - icon->height() / 2);
            QCOMPARE(icon->width(), icon->implicitWidth());
            QCOMPARE(icon->height(), icon->implicitHeight());
            QCOMPARE(text->x(), contentX + (label->isMirrored() ? 0 : icon->width() + label->spacing()));
            QCOMPARE(text->y(), verticalCenter - text->height() / 2);
            QCOMPARE(text->width(), text->implicitWidth());
            QCOMPARE(text->height(), text->implicitHeight());
            QCOMPARE(label->implicitWidth(), combinedWidth + horizontalPadding);
            QCOMPARE(label->implicitHeight(), qMax(icon->implicitHeight(), text->implicitHeight()) + verticalPadding);
            break;
        }

        if (text)
            QCOMPARE(label->baselineOffset(), text->y() + text->baselineOffset());
        else
            QCOMPARE(label->baselineOffset(), 0);
    }
}

void tst_qquickiconlabel::spacingWithOneDelegate_data()
{
    QTest::addColumn<QString>("qmlFileName");

    QTest::addRow("spacingWithOnlyIcon") << QStringLiteral("spacingWithOnlyIcon.qml");
    QTest::addRow("spacingWithOnlyText") << QStringLiteral("spacingWithOnlyText.qml");
}

void tst_qquickiconlabel::spacingWithOneDelegate()
{
    QFETCH(QString, qmlFileName);

    QQuickView view(testFileUrl(qmlFileName));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem *rootItem = view.rootObject();
    QVERIFY(rootItem);

    QQuickIconLabel *label = rootItem->findChild<QQuickIconLabel *>();
    QVERIFY(label);
    QQuickItem *delegate = nullptr;
    if (!label->icon().isEmpty()) {
        QVERIFY(!label->findChild<QQuickText *>());
        delegate = label->findChild<QQuickIconImage *>();
    } else {
        QVERIFY(!label->findChild<QQuickIconImage *>());
        delegate = label->findChild<QQuickText *>();
    }

    QVERIFY(delegate);
    QCOMPARE(delegate->x(), 0.0);
    QCOMPARE(delegate->width(), label->width());
}

void tst_qquickiconlabel::emptyIconSource()
{
    QQuickView view(testFileUrl("iconlabel.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem *rootItem = view.rootObject();
    QVERIFY(rootItem);

    QQuickIconLabel *label = rootItem->findChild<QQuickIconLabel *>();
    QVERIFY(label);
    QCOMPARE(label->spacing(), 0.0);
    QCOMPARE(label->display(), QQuickIconLabel::TextBesideIcon);
    QCOMPARE(label->isMirrored(), false);

    QQuickItem *icon = label->findChild<QQuickIconImage *>();
    QVERIFY(icon);

    QQuickItem *text = label->findChild<QQuickText *>();
    QVERIFY(text);
    qreal horizontalCenter = label->width() / 2;
    const qreal combinedWidth = icon->width() + text->width();
    const qreal contentX = horizontalCenter - combinedWidth / 2;
    // The text should be positioned next to an item.
    QCOMPARE(text->x(), contentX + icon->width() + label->spacing());

    // Now give the label an explicit width large enough so that implicit size
    // changes in its children don't affect its implicit size.
    label->setWidth(label->implicitWidth() + 200);
    label->setHeight(label->implicitWidth() + 100);
    QVERIFY(icon->property("source").isValid());
    label->setIcon(QQuickIcon());
    QVERIFY(!label->findChild<QQuickIconImage *>());
    horizontalCenter = label->width() / 2;
    QCOMPARE(text->x(), horizontalCenter - text->width() / 2);
}

void tst_qquickiconlabel::colorChanges()
{
    QQuickView view(testFileUrl("colorChanges.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem *rootItem = view.rootObject();
    QVERIFY(rootItem);

    QQuickIconLabel *label = rootItem->findChild<QQuickIconLabel *>();
    QVERIFY(label);
    QCOMPARE(label->spacing(), 0.0);
    QCOMPARE(label->display(), QQuickIconLabel::TextBesideIcon);
    QCOMPARE(label->isMirrored(), false);

    QSharedPointer<QQuickItemGrabResult> grabResult = label->grabToImage();
    QTRY_VERIFY(!grabResult->image().isNull());
    const QImage enabledImageGrab = grabResult->image();

    // The color should change to "red" when the item is disabled.
    rootItem->setEnabled(false);

    grabResult = label->grabToImage();
    QTRY_VERIFY(!grabResult->image().isNull());
    QVERIFY(grabResult->image() != enabledImageGrab);
}

void tst_qquickiconlabel::iconSourceContext()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("iconSourceContext.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> o(component.create());
    QQuickItem *root = qobject_cast<QQuickItem *>(o.data());
    QVERIFY(root);

    for (QQuickItem *child : root->childItems()) {
        QQuickImage *image = qobject_cast<QQuickImage *>(child);
        if (!image) {
            QQuickIconLabel *label = nullptr;
            if (QQuickAbstractButton *button = qobject_cast<QQuickAbstractButton *>(child)) {
                label = qobject_cast<QQuickIconLabel *>(button->contentItem());
            } else {
                label = qobject_cast<QQuickIconLabel *>(child);
            }
            if (label) {
                QQuickIconLabelPrivate *labelPrivate = static_cast<QQuickIconLabelPrivate *>(
                            QQuickItemPrivate::get(label));
                image = labelPrivate->image;
            }
        }

        QVERIFY(image);
        QQuickImagePrivate *imagePrivate
                = static_cast<QQuickImagePrivate *>(QQuickItemPrivate::get(image));
        QCOMPARE(imagePrivate->pix.url(), testFileUrl("a.png"));
    }

}

QTEST_MAIN(tst_qquickiconlabel)

#include "tst_qquickiconlabel.moc"
