// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QT3DINPUT_QINPUTASPECT_P_H
#define QT3DINPUT_QINPUTASPECT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <Qt3DCore/private/qabstractaspect_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DInput {

class QInputAspect;

namespace Input {
class InputHandler;
class KeyboardMouseGenericDeviceIntegration;
}

class QInputAspectPrivate : public Qt3DCore::QAbstractAspectPrivate
{
public:
    QInputAspectPrivate();
    void loadInputDevicePlugins();

    Q_DECLARE_PUBLIC(QInputAspect)
    QScopedPointer<Input::InputHandler> m_inputHandler;
    QScopedPointer<Input::KeyboardMouseGenericDeviceIntegration> m_keyboardMouseIntegration;
    qint64 m_time;
};

} // namespace Qt3DInput

QT_END_NAMESPACE

#endif // QT3DINPUT_QINPUTASPECT_P_H
