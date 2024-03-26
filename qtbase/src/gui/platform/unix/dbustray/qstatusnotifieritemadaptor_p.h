// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/*
    This file was originally created by qdbusxml2cpp version 0.8
    Command line was:
    qdbusxml2cpp -a statusnotifieritem ../../3rdparty/dbus-ifaces/org.kde.StatusNotifierItem.xml

    However it is maintained manually.

    It is also not part of the public API. This header file may change from
    version to version without notice, or even be removed.
*/

#ifndef QSTATUSNOTIFIERITEMADAPTER_P_H
#define QSTATUSNOTIFIERITEMADAPTER_P_H

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

#include <private/qtguiglobal_p.h>

QT_REQUIRE_CONFIG(systemtrayicon);

#include <QObject>
#include <QDBusAbstractAdaptor>

#include <private/qdbustraytypes_p.h>

QT_BEGIN_NAMESPACE
class QDBusTrayIcon;

/*
    Adaptor class for interface org.kde.StatusNotifierItem
    see http://www.freedesktop.org/wiki/Specifications/StatusNotifierItem/
    (also http://www.notmart.org/misc/statusnotifieritem/)
*/
class QStatusNotifierItemAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.StatusNotifierItem")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.kde.StatusNotifierItem\">\n"
"    <property access=\"read\" type=\"s\" name=\"Category\"/>\n"
"    <property access=\"read\" type=\"s\" name=\"Id\"/>\n"
"    <property access=\"read\" type=\"s\" name=\"Title\"/>\n"
"    <property access=\"read\" type=\"s\" name=\"Status\"/>\n"
"    <property access=\"read\" type=\"i\" name=\"WindowId\"/>\n"
"    <property access=\"read\" type=\"s\" name=\"IconThemePath\"/>\n"
"    <property access=\"read\" type=\"o\" name=\"Menu\"/>\n"
"    <property access=\"read\" type=\"b\" name=\"ItemIsMenu\"/>\n"
"    <property access=\"read\" type=\"s\" name=\"IconName\"/>\n"
"    <property access=\"read\" type=\"a(iiay)\" name=\"IconPixmap\">\n"
"      <annotation value=\"QXdgDBusImageVector\" name=\"org.qtproject.QtDBus.QtTypeName\"/>\n"
"    </property>\n"
"    <property access=\"read\" type=\"s\" name=\"OverlayIconName\"/>\n"
"    <property access=\"read\" type=\"a(iiay)\" name=\"OverlayIconPixmap\">\n"
"      <annotation value=\"QXdgDBusImageVector\" name=\"org.qtproject.QtDBus.QtTypeName\"/>\n"
"    </property>\n"
"    <property access=\"read\" type=\"s\" name=\"AttentionIconName\"/>\n"
"    <property access=\"read\" type=\"a(iiay)\" name=\"AttentionIconPixmap\">\n"
"      <annotation value=\"QXdgDBusImageVector\" name=\"org.qtproject.QtDBus.QtTypeName\"/>\n"
"    </property>\n"
"    <property access=\"read\" type=\"s\" name=\"AttentionMovieName\"/>\n"
"    <property access=\"read\" type=\"(sa(iiay)ss)\" name=\"ToolTip\">\n"
"      <annotation value=\"QXdgDBusToolTipStruct\" name=\"org.qtproject.QtDBus.QtTypeName\"/>\n"
"    </property>\n"
"    <method name=\"ProvideXdgActivationToken\">\n"
"        <arg name=\"token\" type=\"s\" direction=\"in\"/>\n"
"    </method>\n"
"    <method name=\"ContextMenu\">\n"
"      <arg direction=\"in\" type=\"i\" name=\"x\"/>\n"
"      <arg direction=\"in\" type=\"i\" name=\"y\"/>\n"
"    </method>\n"
"    <method name=\"Activate\">\n"
"      <arg direction=\"in\" type=\"i\" name=\"x\"/>\n"
"      <arg direction=\"in\" type=\"i\" name=\"y\"/>\n"
"    </method>\n"
"    <method name=\"SecondaryActivate\">\n"
"      <arg direction=\"in\" type=\"i\" name=\"x\"/>\n"
"      <arg direction=\"in\" type=\"i\" name=\"y\"/>\n"
"    </method>\n"
"    <method name=\"Scroll\">\n"
"      <arg direction=\"in\" type=\"i\" name=\"delta\"/>\n"
"      <arg direction=\"in\" type=\"s\" name=\"orientation\"/>\n"
"    </method>\n"
"    <signal name=\"NewTitle\"/>\n"
"    <signal name=\"NewIcon\"/>\n"
"    <signal name=\"NewAttentionIcon\"/>\n"
"    <signal name=\"NewOverlayIcon\"/>\n"
"    <signal name=\"NewMenu\"/>\n"
"    <signal name=\"NewToolTip\"/>\n"
"    <signal name=\"NewStatus\">\n"
"      <arg type=\"s\" name=\"status\"/>\n"
"    </signal>\n"
"  </interface>\n"
        "")
public:
    QStatusNotifierItemAdaptor(QDBusTrayIcon *parent);
    virtual ~QStatusNotifierItemAdaptor();

public: // PROPERTIES
    Q_PROPERTY(QString AttentionIconName READ attentionIconName)
    QString attentionIconName() const;

    Q_PROPERTY(QXdgDBusImageVector AttentionIconPixmap READ attentionIconPixmap)
    QXdgDBusImageVector attentionIconPixmap() const;

    Q_PROPERTY(QString AttentionMovieName READ attentionMovieName)
    QString attentionMovieName() const;

    Q_PROPERTY(QString Category READ category)
    QString category() const;

    Q_PROPERTY(QString IconName READ iconName)
    QString iconName() const;

    Q_PROPERTY(QXdgDBusImageVector IconPixmap READ iconPixmap)
    QXdgDBusImageVector iconPixmap() const;

    Q_PROPERTY(QString Id READ id)
    QString id() const;

    Q_PROPERTY(bool ItemIsMenu READ itemIsMenu)
    bool itemIsMenu() const;

    Q_PROPERTY(QDBusObjectPath Menu READ menu)
    QDBusObjectPath menu() const;

    Q_PROPERTY(QString OverlayIconName READ overlayIconName)
    QString overlayIconName() const;

    Q_PROPERTY(QXdgDBusImageVector OverlayIconPixmap READ overlayIconPixmap)
    QXdgDBusImageVector overlayIconPixmap() const;

    Q_PROPERTY(QString Status READ status)
    QString status() const;

    Q_PROPERTY(QString Title READ title)
    QString title() const;

    Q_PROPERTY(QXdgDBusToolTipStruct ToolTip READ toolTip)
    QXdgDBusToolTipStruct toolTip() const;

public Q_SLOTS: // METHODS
    void Activate(int x, int y);
    void ContextMenu(int x, int y);
    void ProvideXdgActivationToken(const QString &token);
    void Scroll(int delta, const QString &orientation);
    void SecondaryActivate(int x, int y);
Q_SIGNALS: // SIGNALS
    void NewAttentionIcon();
    void NewIcon();
    void NewOverlayIcon();
    void NewMenu();
    void NewStatus(const QString &status);
    void NewTitle();
    void NewToolTip();

private:
    QDBusTrayIcon *m_trayIcon;
};

QT_END_NAMESPACE

#endif // QSTATUSNOTIFIERITEMADAPTER_P_H
