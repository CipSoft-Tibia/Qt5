// Copyright (C) 2020 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QT3DLOGIC_LOGIC_CALLBACKJOB_P_H
#define QT3DLOGIC_LOGIC_CALLBACKJOB_P_H

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

#include <Qt3DCore/qaspectjob.h>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DLogic {
namespace Logic {

class Manager;

class CallbackJob : public Qt3DCore::QAspectJob
{
public:
    CallbackJob();
    void setManager(Manager *manager);

    void run() override;
    bool isRequired() override;
    void postFrame(Qt3DCore::QAspectEngine *aspectEngine) override;

private:
    Manager *m_logicManager;
};

} // namespace Logic
} // namespace Qt3DLogic

QT_END_NAMESPACE

#endif // QT3DLOGIC_LOGIC_CALLBACKJOB_P_H
