// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKAPPLICATIONWINDOW_P_H
#define QQUICKAPPLICATIONWINDOW_P_H

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

#include <QtQuick/private/qquickwindowmodule_p.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>
#include <QtGui/qfont.h>
#include <QtGui/qpalette.h>
#include <QtCore/qlocale.h>

QT_BEGIN_NAMESPACE

class QQuickApplicationWindowPrivate;
class QQuickApplicationWindowAttached;
class QQuickApplicationWindowAttachedPrivate;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickApplicationWindow : public QQuickWindowQmlImpl
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *background READ background WRITE setBackground NOTIFY backgroundChanged FINAL)
    Q_PROPERTY(QQuickItem *contentItem READ contentItem CONSTANT FINAL)
    Q_PRIVATE_PROPERTY(QQuickApplicationWindow::d_func(), QQmlListProperty<QObject> contentData READ contentData FINAL)
    Q_PROPERTY(QQuickItem *activeFocusControl READ activeFocusControl NOTIFY activeFocusControlChanged FINAL)
    Q_PROPERTY(QQuickItem *header READ header WRITE setHeader NOTIFY headerChanged FINAL)
    Q_PROPERTY(QQuickItem *footer READ footer WRITE setFooter NOTIFY footerChanged FINAL)
    Q_PROPERTY(QFont font READ font WRITE setFont RESET resetFont NOTIFY fontChanged FINAL)
    Q_PROPERTY(QLocale locale READ locale WRITE setLocale RESET resetLocale NOTIFY localeChanged FINAL)
    // 2.3 (Qt 5.10)
    Q_PROPERTY(QQuickItem *menuBar READ menuBar WRITE setMenuBar NOTIFY menuBarChanged FINAL REVISION(2, 3))
    // 2.14 (Qt 6)
    Q_PRIVATE_PROPERTY(QQuickApplicationWindow::d_func(), QQuickPalette *palette READ palette WRITE setPalette RESET resetPalette NOTIFY paletteChanged REVISION(2, 3))
    Q_CLASSINFO("DeferredPropertyNames", "background")
    Q_CLASSINFO("DefaultProperty", "contentData")
    QML_NAMED_ELEMENT(ApplicationWindow)
    QML_ADDED_IN_VERSION(2, 0)
    QML_ATTACHED(QQuickApplicationWindowAttached)

public:
    explicit QQuickApplicationWindow(QWindow *parent = nullptr);
    ~QQuickApplicationWindow();

    static QQuickApplicationWindowAttached *qmlAttachedProperties(QObject *object);

    QQuickItem *background() const;
    void setBackground(QQuickItem *background);

    QQuickItem *contentItem() const;

    QQuickItem *activeFocusControl() const;

    QQuickItem *header() const;
    void setHeader(QQuickItem *header);

    QQuickItem *footer() const;
    void setFooter(QQuickItem *footer);

    QFont font() const;
    void setFont(const QFont &font);
    void resetFont();

    QLocale locale() const;
    void setLocale(const QLocale &locale);
    void resetLocale();

    QQuickItem *menuBar() const;
    void setMenuBar(QQuickItem *menuBar);

Q_SIGNALS:
    void backgroundChanged();
    void activeFocusControlChanged();
    void headerChanged();
    void footerChanged();
    void fontChanged();
    void localeChanged();
    Q_REVISION(2, 3) void menuBarChanged();

protected:
    bool isComponentComplete() const;
    void classBegin() override;
    void componentComplete() override;
    void resizeEvent(QResizeEvent *event) override;

private:
    Q_DISABLE_COPY(QQuickApplicationWindow)
    Q_DECLARE_PRIVATE(QQuickApplicationWindow)
    Q_PRIVATE_SLOT(d_func(), void _q_updateActiveFocus())
};

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickApplicationWindowAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickApplicationWindow *window READ window NOTIFY windowChanged FINAL)
    Q_PROPERTY(QQuickItem *contentItem READ contentItem NOTIFY contentItemChanged FINAL)
    Q_PROPERTY(QQuickItem *activeFocusControl READ activeFocusControl NOTIFY activeFocusControlChanged FINAL)
    Q_PROPERTY(QQuickItem *header READ header NOTIFY headerChanged FINAL)
    Q_PROPERTY(QQuickItem *footer READ footer NOTIFY footerChanged FINAL)
    Q_PROPERTY(QQuickItem *menuBar READ menuBar NOTIFY menuBarChanged FINAL) // REVISION(2, 3)

public:
    explicit QQuickApplicationWindowAttached(QObject *parent = nullptr);

    QQuickApplicationWindow *window() const;
    QQuickItem *contentItem() const;
    QQuickItem *activeFocusControl() const;
    QQuickItem *header() const;
    QQuickItem *footer() const;
    QQuickItem *menuBar() const;

Q_SIGNALS:
    void windowChanged();
    void contentItemChanged();
    void activeFocusControlChanged();
    void headerChanged();
    void footerChanged();
    // 2.3 (Qt 5.10)
    /*Q_REVISION(2, 3)*/ void menuBarChanged();

private:
    Q_DISABLE_COPY(QQuickApplicationWindowAttached)
    Q_DECLARE_PRIVATE(QQuickApplicationWindowAttached)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickApplicationWindow)

#endif // QQUICKAPPLICATIONWINDOW_P_H
