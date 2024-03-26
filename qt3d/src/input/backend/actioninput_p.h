// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QT3DINPUT_INPUT_ACTIONINPUT_H
#define QT3DINPUT_INPUT_ACTIONINPUT_H

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

#include <Qt3DCore/qnodeid.h>

#include <Qt3DInput/private/abstractactioninput_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DInput {

namespace Input {

class Q_AUTOTEST_EXPORT ActionInput : public AbstractActionInput
{
public:
    ActionInput();
    void cleanup();

    inline QList<int> buttons() const { return m_buttons; }
    inline Qt3DCore::QNodeId sourceDevice() const { return m_sourceDevice; }
    void syncFromFrontEnd(const Qt3DCore::QNode *frontEnd, bool firstTime) override;

    bool process(InputHandler *inputHandler, qint64 currentTime) override;

private:
    QList<int> m_buttons;
    Qt3DCore::QNodeId m_sourceDevice;
};

} // namespace Input

} // namespace Qt3DInput

QT_END_NAMESPACE

#endif // QT3DINPUT_INPUT_ACTIONINPUT_H
