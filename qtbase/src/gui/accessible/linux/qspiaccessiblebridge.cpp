// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qspiaccessiblebridge_p.h"

#include <atspi/atspi-constants.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <qstring.h>

#include "atspiadaptor_p.h"

#include "qspidbuscache_p.h"
#include "qspi_constant_mappings_p.h"
#include "dbusconnection_p.h"
#include "qspi_struct_marshallers_p.h"

#if QT_CONFIG(accessibility)
#include "deviceeventcontroller_adaptor.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QSpiAccessibleBridge
    \internal
*/

QSpiAccessibleBridge::QSpiAccessibleBridge()
    : cache(nullptr), dec(nullptr), dbusAdaptor(nullptr)
{
    dbusConnection = new DBusConnection();
    connect(dbusConnection, SIGNAL(enabledChanged(bool)), this, SLOT(enabledChanged(bool)));
    // Now that we have connected the signal, make sure we didn't miss a change,
    // e.g. when running as root or when AT_SPI_BUS_ADDRESS is set by hand.
    // But do that only on next loop, once dbus is really settled.
    QTimer::singleShot(
        0, this, [this]{
            if (dbusConnection->isEnabled() && dbusConnection->connection().isConnected())
                enabledChanged(true);
        });
}

void QSpiAccessibleBridge::enabledChanged(bool enabled)
{
    setActive(enabled);
    updateStatus();
}

QSpiAccessibleBridge::~QSpiAccessibleBridge()
{
    delete dbusConnection;
} // Qt currently doesn't delete plugins.

QDBusConnection QSpiAccessibleBridge::dBusConnection() const
{
    return dbusConnection->connection();
}

void QSpiAccessibleBridge::updateStatus()
{
    // create the adaptor to handle everything if we are in enabled state
    if (!dbusAdaptor && isActive()) {
        qSpiInitializeStructTypes();
        initializeConstantMappings();

        cache = new QSpiDBusCache(dbusConnection->connection(), this);
        dec = new DeviceEventControllerAdaptor(this);

        dbusConnection->connection().registerObject(ATSPI_DBUS_PATH_DEC ""_L1, this, QDBusConnection::ExportAdaptors);

        dbusAdaptor = new AtSpiAdaptor(dbusConnection, this);
        dbusConnection->connection().registerVirtualObject(QSPI_OBJECT_PATH_ACCESSIBLE ""_L1, dbusAdaptor, QDBusConnection::SubPath);
        dbusAdaptor->registerApplication();
    }
}

void QSpiAccessibleBridge::notifyAccessibilityUpdate(QAccessibleEvent *event)
{
    if (!dbusAdaptor)
        return;
    if (isActive() && event->accessibleInterface())
        dbusAdaptor->notify(event);
}

struct RoleMapping {
    QAccessible::Role role;
    AtspiRole spiRole;
    const char *name;
};

static RoleMapping map[] = {
    //: Role of an accessible object - the object is in an invalid state or could not be constructed
    { QAccessible::NoRole, ATSPI_ROLE_INVALID, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "invalid role") },
    //: Role of an accessible object
    { QAccessible::TitleBar, ATSPI_ROLE_TEXT, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "title bar") },
    //: Role of an accessible object
    { QAccessible::MenuBar, ATSPI_ROLE_MENU_BAR, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "menu bar") },
    //: Role of an accessible object
    { QAccessible::ScrollBar, ATSPI_ROLE_SCROLL_BAR, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "scroll bar") },
    //: Role of an accessible object - the grip is usually used for resizing another object
    { QAccessible::Grip, ATSPI_ROLE_UNKNOWN, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "grip") },
    //: Role of an accessible object
    { QAccessible::Sound, ATSPI_ROLE_UNKNOWN, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "sound") },
    //: Role of an accessible object
    { QAccessible::Cursor, ATSPI_ROLE_UNKNOWN, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "cursor") },
    //: Role of an accessible object
    { QAccessible::Caret, ATSPI_ROLE_UNKNOWN, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "text caret") },
    //: Role of an accessible object
    { QAccessible::AlertMessage, ATSPI_ROLE_ALERT, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "alert message") },
    //: Role of an accessible object: a window with frame and title
    { QAccessible::Window, ATSPI_ROLE_FRAME, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "frame") },
    //: Role of an accessible object
    { QAccessible::Client, ATSPI_ROLE_FILLER, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "filler") },
    //: Role of an accessible object
    { QAccessible::PopupMenu, ATSPI_ROLE_POPUP_MENU, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "popup menu") },
    //: Role of an accessible object
    { QAccessible::MenuItem, ATSPI_ROLE_MENU_ITEM, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "menu item") },
    //: Role of an accessible object
    { QAccessible::ToolTip, ATSPI_ROLE_TOOL_TIP, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "tool tip") },
    //: Role of an accessible object
    { QAccessible::Application, ATSPI_ROLE_APPLICATION, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "application") },
    //: Role of an accessible object
    { QAccessible::Document, ATSPI_ROLE_DOCUMENT_FRAME, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "document") },
    //: Role of an accessible object
    { QAccessible::Pane, ATSPI_ROLE_PANEL, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "panel") },
    //: Role of an accessible object
    { QAccessible::Chart, ATSPI_ROLE_CHART, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "chart") },
    //: Role of an accessible object
    { QAccessible::Dialog, ATSPI_ROLE_DIALOG, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "dialog") },
    //: Role of an accessible object
    { QAccessible::Border, ATSPI_ROLE_PANEL, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "panel") },
    //: Role of an accessible object
    { QAccessible::Grouping, ATSPI_ROLE_PANEL, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "panel") },
    //: Role of an accessible object
    { QAccessible::Separator, ATSPI_ROLE_SEPARATOR, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "separator") },
    //: Role of an accessible object
    { QAccessible::ToolBar, ATSPI_ROLE_TOOL_BAR, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "tool bar") },
    //: Role of an accessible object
    { QAccessible::StatusBar, ATSPI_ROLE_STATUS_BAR, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "status bar") },
    //: Role of an accessible object
    { QAccessible::Table, ATSPI_ROLE_TABLE, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "table") },
    //: Role of an accessible object - part of a table
    { QAccessible::ColumnHeader, ATSPI_ROLE_TABLE_COLUMN_HEADER, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "column header") },
    //: Role of an accessible object - part of a table
    { QAccessible::RowHeader, ATSPI_ROLE_TABLE_ROW_HEADER, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "row header") },
    //: Role of an accessible object - part of a table
    { QAccessible::Column, ATSPI_ROLE_TABLE_CELL, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "column") },
    //: Role of an accessible object - part of a table
    { QAccessible::Row, ATSPI_ROLE_TABLE_ROW, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "row") },
    //: Role of an accessible object - part of a table
    { QAccessible::Cell, ATSPI_ROLE_TABLE_CELL, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "cell") },
    //: Role of an accessible object
    { QAccessible::Link, ATSPI_ROLE_LINK, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "link") },
    //: Role of an accessible object
    { QAccessible::HelpBalloon, ATSPI_ROLE_DIALOG, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "help balloon") },
    //: Role of an accessible object - a helper dialog
    { QAccessible::Assistant, ATSPI_ROLE_DIALOG, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "assistant") },
    //: Role of an accessible object
    { QAccessible::List, ATSPI_ROLE_LIST, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "list") },
    //: Role of an accessible object
    { QAccessible::ListItem, ATSPI_ROLE_LIST_ITEM, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "list item") },
    //: Role of an accessible object
    { QAccessible::Tree, ATSPI_ROLE_TREE, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "tree") },
    //: Role of an accessible object
    { QAccessible::TreeItem, ATSPI_ROLE_TABLE_CELL, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "tree item") },
    //: Role of an accessible object
    { QAccessible::PageTab, ATSPI_ROLE_PAGE_TAB, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "page tab") },
    //: Role of an accessible object
    { QAccessible::PropertyPage, ATSPI_ROLE_PAGE_TAB, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "property page") },
    //: Role of an accessible object
    { QAccessible::Indicator, ATSPI_ROLE_UNKNOWN, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "indicator") },
    //: Role of an accessible object
    { QAccessible::Graphic, ATSPI_ROLE_IMAGE, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "graphic") },
    //: Role of an accessible object
    { QAccessible::StaticText, ATSPI_ROLE_LABEL, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "label") },
    //: Role of an accessible object
    { QAccessible::EditableText, ATSPI_ROLE_TEXT, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "text") },
    //: Role of an accessible object
    { QAccessible::PushButton, ATSPI_ROLE_PUSH_BUTTON, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "push button") },
    //: Role of an accessible object
    { QAccessible::CheckBox, ATSPI_ROLE_CHECK_BOX, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "check box") },
    //: Role of an accessible object
    { QAccessible::RadioButton, ATSPI_ROLE_RADIO_BUTTON, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "radio button") },
    //: Role of an accessible object
    { QAccessible::ComboBox, ATSPI_ROLE_COMBO_BOX, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "combo box") },
    //: Role of an accessible object
    { QAccessible::ProgressBar, ATSPI_ROLE_PROGRESS_BAR, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "progress bar") },
    //: Role of an accessible object
    { QAccessible::Dial, ATSPI_ROLE_DIAL, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "dial") },
    //: Role of an accessible object
    { QAccessible::HotkeyField, ATSPI_ROLE_TEXT, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "hotkey field") },
    //: Role of an accessible object
    { QAccessible::Slider, ATSPI_ROLE_SLIDER, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "slider") },
    //: Role of an accessible object
    { QAccessible::SpinBox, ATSPI_ROLE_SPIN_BUTTON, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "spin box") },
    //: Role of an accessible object
    { QAccessible::Canvas, ATSPI_ROLE_CANVAS, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "canvas") },
    //: Role of an accessible object
    { QAccessible::Animation, ATSPI_ROLE_ANIMATION, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "animation") },
    //: Role of an accessible object
    { QAccessible::Equation, ATSPI_ROLE_TEXT, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "equation") },
    //: Role of an accessible object
    { QAccessible::ButtonDropDown, ATSPI_ROLE_PUSH_BUTTON, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "button with drop down") },
    //: Role of an accessible object
#if ATSPI_ROLE_COUNT > 130
    { QAccessible::ButtonMenu, ATSPI_ROLE_PUSH_BUTTON_MENU, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "button menu") },
#else
    { QAccessible::ButtonMenu, ATSPI_ROLE_PUSH_BUTTON, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "button menu") },
#endif
    //: Role of an accessible object - a button that expands a grid.
    { QAccessible::ButtonDropGrid, ATSPI_ROLE_PUSH_BUTTON, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "button with drop down grid") },
    //: Role of an accessible object - blank space between other objects.
    { QAccessible::Whitespace, ATSPI_ROLE_FILLER, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "space") },
    //: Role of an accessible object
    { QAccessible::PageTabList, ATSPI_ROLE_PAGE_TAB_LIST, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "page tab list") },
    //: Role of an accessible object
    { QAccessible::Clock, ATSPI_ROLE_UNKNOWN, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "clock") },
    //: Role of an accessible object
    { QAccessible::Splitter, ATSPI_ROLE_SPLIT_PANE, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "splitter") },
    //: Role of an accessible object
    { QAccessible::LayeredPane, ATSPI_ROLE_LAYERED_PANE, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "layered pane") },
    //: Role of an accessible object
    { QAccessible::WebDocument, ATSPI_ROLE_DOCUMENT_WEB, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "web document") },
    //: Role of an accessible object
    { QAccessible::Paragraph, ATSPI_ROLE_PARAGRAPH, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "paragraph") },
    //: Role of an accessible object
    { QAccessible::Section, ATSPI_ROLE_SECTION, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "section") },
    //: Role of an accessible object
    { QAccessible::ColorChooser, ATSPI_ROLE_COLOR_CHOOSER, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "color chooser") },
    //: Role of an accessible object
    { QAccessible::Footer, ATSPI_ROLE_FOOTER, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "footer") },
    //: Role of an accessible object
    { QAccessible::Form, ATSPI_ROLE_FORM, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "form") },
    //: Role of an accessible object
    { QAccessible::Heading, ATSPI_ROLE_HEADING, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "heading") },
    //: Role of an accessible object
    { QAccessible::Note, ATSPI_ROLE_COMMENT, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "note") },
    //: Role of an accessible object
    { QAccessible::ComplementaryContent, ATSPI_ROLE_SECTION, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "complementary content") },
    //: Role of an accessible object
    { QAccessible::Terminal, ATSPI_ROLE_TERMINAL, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "terminal") },
    //: Role of an accessible object
    { QAccessible::Desktop, ATSPI_ROLE_DESKTOP_FRAME, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "desktop") },
    //: Role of an accessible object
    { QAccessible::Notification, ATSPI_ROLE_NOTIFICATION, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "notification") },
    //: Role of an accessible object
    { QAccessible::UserRole, ATSPI_ROLE_UNKNOWN, QT_TRANSLATE_NOOP("QSpiAccessibleBridge", "unknown") }
};

void QSpiAccessibleBridge::initializeConstantMappings()
{
    for (uint i = 0; i < sizeof(map) / sizeof(RoleMapping); ++i)
        m_spiRoleMapping.insert(map[i].role, RoleNames(map[i].spiRole, QLatin1StringView(map[i].name), tr(map[i].name)));

    // -1 because we have button duplicated, as PushButton and Button.
    Q_ASSERT_X(m_spiRoleMapping.size() ==
               QAccessible::staticMetaObject.enumerator(
                   QAccessible::staticMetaObject.indexOfEnumerator("Role")).keyCount() - 1,
               "", "Handle all QAccessible::Role members in qSpiRoleMapping");
}

QSpiAccessibleBridge *QSpiAccessibleBridge::instance()
{
    if (auto integration = QGuiApplicationPrivate::platformIntegration()) {
        if (auto accessibility = integration->accessibility())
            return static_cast<QSpiAccessibleBridge *>(accessibility);
    }
    return nullptr;
}

RoleNames QSpiAccessibleBridge::namesForRole(QAccessible::Role role)
{
    auto brigde = QSpiAccessibleBridge::instance();
    return brigde ? brigde->spiRoleNames().value(role) : RoleNames();
}

QT_END_NAMESPACE

#include "moc_qspiaccessiblebridge_p.cpp"
#endif // QT_CONFIG(accessibility)
