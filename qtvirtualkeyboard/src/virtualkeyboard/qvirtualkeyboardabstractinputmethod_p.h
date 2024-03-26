// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QVIRTUALKEYBOARDABSTRACTINPUTMETHOD_P_H
#define QVIRTUALKEYBOARDABSTRACTINPUTMETHOD_P_H

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

#include <QtVirtualKeyboard/qvirtualkeyboard_global.h>
#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QVirtualKeyboardInputEngine;

class Q_VIRTUALKEYBOARD_EXPORT QVirtualKeyboardAbstractInputMethodPrivate : public QObjectPrivate
{
public:
    QVirtualKeyboardAbstractInputMethodPrivate();

    QVirtualKeyboardInputEngine *inputEngine;
};

QT_END_NAMESPACE

#endif // QVIRTUALKEYBOARDABSTRACTINPUTMETHOD_P_H
