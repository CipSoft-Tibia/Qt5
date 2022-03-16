/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef WEB_EVENT_FACTORY_H
#define WEB_EVENT_FACTORY_H

#include "content/public/browser/native_web_keyboard_event.h"
#ifndef QT_NO_GESTURES
#include "third_party/blink/public/platform/web_gesture_event.h"
#endif
#include "third_party/blink/public/platform/web_mouse_event.h"
#include "third_party/blink/public/platform/web_mouse_wheel_event.h"

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QEvent;
class QHoverEvent;
class QKeyEvent;
class QMouseEvent;
#ifndef QT_NO_TABLETEVENT
class QTabletEvent;
#endif
class QWheelEvent;
#ifndef QT_NO_GESTURES
class QNativeGestureEvent;
#endif
QT_END_NAMESPACE

class WebEventFactory {

public:
    static blink::WebMouseEvent toWebMouseEvent(QMouseEvent*, double dpiScale);
    static blink::WebMouseEvent toWebMouseEvent(QHoverEvent*, double dpiScale);
#ifndef QT_NO_TABLETEVENT
    static blink::WebMouseEvent toWebMouseEvent(QTabletEvent*, double dpiScale);
#endif
    static blink::WebMouseEvent toWebMouseEvent(QEvent *);
#ifndef QT_NO_GESTURES
    static blink::WebGestureEvent toWebGestureEvent(QNativeGestureEvent *, double dpiScale);
#endif
    static blink::WebMouseWheelEvent toWebWheelEvent(QWheelEvent*, double dpiScale);
    static bool coalesceWebWheelEvent(blink::WebMouseWheelEvent &, QWheelEvent*, double dpiScale);
    static content::NativeWebKeyboardEvent toWebKeyboardEvent(QKeyEvent*);
    static bool getEditCommand(QKeyEvent *event, std::string *editCommand);
};


#endif // WEB_EVENT_FACTORY_H
