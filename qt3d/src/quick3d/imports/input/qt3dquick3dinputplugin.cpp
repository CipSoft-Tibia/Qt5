// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qt3dquick3dinputplugin.h"

#include <Qt3DInput/qabstractphysicaldevice.h>
#include <Qt3DInput/qaction.h>
#include <Qt3DInput/qactioninput.h>
#include <Qt3DInput/qanalogaxisinput.h>
#include <Qt3DInput/qaxis.h>
#include <Qt3DInput/qaxisaccumulator.h>
#include <Qt3DInput/qaxissetting.h>
#include <Qt3DInput/qbuttonaxisinput.h>
#include <Qt3DInput/qinputchord.h>
#include <Qt3DInput/qinputsequence.h>
#include <Qt3DInput/qinputsettings.h>
#include <Qt3DInput/qkeyboarddevice.h>
#include <Qt3DInput/qkeyboardhandler.h>
#include <Qt3DInput/qkeyevent.h>
#include <Qt3DInput/qlogicaldevice.h>
#include <Qt3DInput/qmousedevice.h>
#include <Qt3DInput/qmouseevent.h>
#include <Qt3DInput/qmousehandler.h>
#include <QtQml>

#include <Qt3DInput/private/qgenericinputdevice_p.h>
#include <Qt3DQuickInput/private/quick3daction_p.h>
#include <Qt3DQuickInput/private/quick3daxis_p.h>
#include <Qt3DQuickInput/private/quick3dinputchord_p.h>
#include <Qt3DQuickInput/private/quick3dinputsequence_p.h>
#include <Qt3DQuickInput/private/quick3dlogicaldevice_p.h>
#include <Qt3DQuickInput/private/quick3dphysicaldevice_p.h>

#ifdef HAVE_QGAMEPAD
# include <Qt3DInput/private/qgamepadinput_p.h>
#endif

QT_BEGIN_NAMESPACE

void Qt3DQuick3DInputPlugin::registerTypes(const char *uri)
{
    qmlRegisterUncreatableType<Qt3DInput::QKeyEvent>(uri, 2, 0, "KeyEvent", QStringLiteral("Events cannot be created"));
    qmlRegisterType<Qt3DInput::QKeyboardDevice>(uri, 2, 0, "KeyboardDevice");
    qmlRegisterType<Qt3DInput::QKeyboardHandler>(uri, 2, 0, "KeyboardHandler");
    qmlRegisterType<Qt3DInput::QInputSettings>(uri, 2, 0, "InputSettings");

    qmlRegisterUncreatableType<Qt3DInput::QMouseEvent>(uri, 2, 0, "MouseEvent", QStringLiteral("Events cannot be created"));
#if QT_CONFIG(wheelevent)
    qmlRegisterUncreatableType<Qt3DInput::QWheelEvent>(uri, 2, 0, "WheelEvent", QStringLiteral("Events cannot be created"));
#endif
    qmlRegisterType<Qt3DInput::QMouseHandler>(uri, 2, 0, "MouseHandler");
    qmlRegisterType<Qt3DInput::QMouseDevice>(uri, 2, 0, "MouseDevice");
    qmlRegisterType<Qt3DInput::QMouseDevice, 15>(uri, 2, 15, "MouseDevice");

    qmlRegisterExtendedType<Qt3DInput::QLogicalDevice, Qt3DInput::Input::Quick::Quick3DLogicalDevice>(uri, 2, 0, "LogicalDevice");
    qmlRegisterUncreatableType<Qt3DInput::QAbstractActionInput>(uri, 2, 0, "AbstractActionInput", QStringLiteral("AbstractActionInput is abstract"));
    qmlRegisterType<Qt3DInput::QActionInput>(uri, 2, 0, "ActionInput");
    qmlRegisterUncreatableType<Qt3DInput::QAbstractAxisInput>(uri, 2, 0, "AbstractAxisInput", QStringLiteral("AbstractAxisInput is abstract"));
    qmlRegisterType<Qt3DInput::QAxisSetting>(uri, 2, 0, "AxisSetting");
    qmlRegisterType<Qt3DInput::QAnalogAxisInput>(uri, 2, 0, "AnalogAxisInput");
    qmlRegisterType<Qt3DInput::QButtonAxisInput>(uri, 2, 0, "ButtonAxisInput");
    qmlRegisterExtendedType<Qt3DInput::QAxis, Qt3DInput::Input::Quick::Quick3DAxis>(uri, 2, 0, "Axis");
    qmlRegisterExtendedType<Qt3DInput::QAction, Qt3DInput::Input::Quick::Quick3DAction>(uri, 2, 0, "Action");
    qmlRegisterExtendedType<Qt3DInput::QInputSequence, Qt3DInput::Input::Quick::Quick3DInputSequence>(uri, 2, 0, "InputSequence");
    qmlRegisterExtendedType<Qt3DInput::QInputChord, Qt3DInput::Input::Quick::Quick3DInputChord>(uri, 2, 0, "InputChord");
    qmlRegisterExtendedUncreatableType<Qt3DInput::QAbstractPhysicalDevice, Qt3DInput::Input::Quick::Quick3DPhysicalDevice>(uri, 2, 0, "QAbstractPhysicalDevice", QStringLiteral("QAbstractPhysicalDevice is abstract"));
    qmlRegisterType<Qt3DInput::QAxisAccumulator>(uri, 2, 1, "AxisAccumulator");

#ifdef HAVE_QGAMEPAD
    qmlRegisterType<Qt3DInput::QGamepadInput>(uri, 2, 0, "GamepadInput");
#endif

    // The minor version used to be the current Qt 5 minor. For compatibility it is the last
    // Qt 5 release.
    qmlRegisterModule(uri, 2, 15);
}

QT_END_NAMESPACE

#include "moc_qt3dquick3dinputplugin.cpp"
