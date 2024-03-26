// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qlogicaspect.h"
#include "qlogicaspect_p.h"

#include <Qt3DLogic/qframeaction.h>
#include <Qt3DCore/qnode.h>
#include <QtCore/QThread>
#include <QtGui/QWindow>

#include <Qt3DLogic/private/executor_p.h>
#include <Qt3DLogic/private/handler_p.h>
#include <Qt3DLogic/private/manager_p.h>
#include <Qt3DCore/private/qchangearbiter_p.h>
#include <Qt3DCore/private/qscene_p.h>
#include <Qt3DCore/private/qservicelocator_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt3DCore;

namespace Qt3DLogic {

/*!
 \class Qt3DLogic::QLogicAspect
 \inherits Qt3DCore::QAbstractAspect
 \inmodule Qt3DLogic
 \brief Responsible for handling frame synchronization jobs.
 \since 5.7
*/

QLogicAspectPrivate::QLogicAspectPrivate()
    : QAbstractAspectPrivate()
    , m_time(0)
    , m_manager(new Logic::Manager)
    , m_executor(new Logic::Executor)
    , m_callbackJob(new Logic::CallbackJob)
{
    m_callbackJob->setManager(m_manager.data());
    m_manager->setExecutor(m_executor.data());
}

void QLogicAspectPrivate::onEngineAboutToShutdown()
{
    m_executor->setScene(nullptr);
}

void QLogicAspectPrivate::registerBackendTypes()
{
    Q_Q(QLogicAspect);
    q->registerBackendType<QFrameAction>(QBackendNodeMapperPtr(new Logic::HandlerFunctor(m_manager.data())));
}

/*!
  Constructs a new Qt3DLogic::QLogicAspect instance with \a parent.
*/
QLogicAspect::QLogicAspect(QObject *parent)
    : QLogicAspect(*new QLogicAspectPrivate(), parent) {}

/*! \internal */
QLogicAspect::QLogicAspect(QLogicAspectPrivate &dd, QObject *parent)
    : QAbstractAspect(dd, parent)
{
    Q_D(QLogicAspect);
    setObjectName(QStringLiteral("Logic Aspect"));
    d->registerBackendTypes();
    d_func()->m_manager->setLogicAspect(this);
}

/*! \internal */
QLogicAspect::~QLogicAspect()
{
}

/*! \internal */
std::vector<QAspectJobPtr> QLogicAspect::jobsToExecute(qint64 time)
{
    Q_D(QLogicAspect);
    const qint64 deltaTime = time - d->m_time;
    const float dt = static_cast<float>(deltaTime) / 1.0e9;
    d->m_manager->setDeltaTime(dt);
    d->m_time = time;

    // Create jobs that will get executed by the threadpool
    if (d->m_manager->hasFrameActions())
        return {d->m_callbackJob};

    return {};
}

/*! \internal */
void QLogicAspect::onRegistered()
{
}

/*! \internal */
void QLogicAspect::onEngineStartup()
{
    Q_D(QLogicAspect);
    d->m_executor->setScene(d->m_arbiter->scene());
}

} // namespace Qt3DLogic

QT_END_NAMESPACE

QT3D_REGISTER_NAMESPACED_ASPECT("logic", QT_PREPEND_NAMESPACE(Qt3DLogic), QLogicAspect)

#include "moc_qlogicaspect.cpp"
