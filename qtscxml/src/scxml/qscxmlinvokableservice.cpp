// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qscxmlglobals_p.h"
#include "qscxmlinvokableservice_p.h"
#include "qscxmlstatemachine_p.h"

QT_BEGIN_NAMESPACE

/*!
 * \class QScxmlInvokableService
 * \brief The QScxmlInvokableService class is the base class for services called
 * from state machines.
 * \since 5.8
 * \inmodule QtScxml
 *
 * The services are called from state machines via the mechanism described in
 * \l {SCXML Specification - 6.4 <invoke>}. This class represents an actual
 * instance of an invoked service.
 */

/*!
 * \class QScxmlInvokableServiceFactory
 * \brief The QScxmlInvokableServiceFactory class creates invokable service
 * instances.
 * \since 5.8
 * \inmodule QtScxml
 *
 * Each service instance represents
 * an \c <invoke> element in the SCXML document. Each time the service is
 * actually invoked, a new instance of QScxmlInvokableService is created.
 */

/*!
  \property QScxmlInvokableServiceFactory::invokeInfo

  \brief The QScxmlExecutableContent::InvokeInfo passed to the constructor.
 */

/*!
  \property QScxmlInvokableServiceFactory::names

  \brief The names passed to the constructor.
 */

/*!
  \property QScxmlInvokableServiceFactory::parameters

  \brief The parameters passed to the constructor.
 */

/*!
 * \class QScxmlStaticScxmlServiceFactory
 * \brief The QScxmlStaticScxmlServiceFactory class creates SCXML service
 * instances from precompiled documents.
 * \since 5.8
 * \inmodule QtScxml
 *
 * A factory for instantiating SCXML state machines from files known at compile
 * time, that is, files specified via the \c src attribute in \c <invoke>.
 */

/*!
 * \class QScxmlDynamicScxmlServiceFactory
 * \brief The QScxmlDynamicScxmlServiceFactory class creates SCXML service
 * instances from documents loaded at runtime.
 * \since 5.8
 * \inmodule QtScxml
 *
 * Dynamically resolved services are used when loading \l{SCXML Specification}
 * {SCXML} content from files that a
 * parent state machine requests at runtime, via the \c srcexpr attribute in
 * the \c <invoke> element.
 */

/*!
 * \property QScxmlInvokableService::parentStateMachine
 *
 * \brief The SCXML state machine that invoked the service.
 */

/*!
 * \property QScxmlInvokableService::id
 *
 * \brief The ID of the invokable service.
 *
 * The ID is specified by the \c id attribute of the \c <invoke> element.
 */

/*!
 * \property QScxmlInvokableService::name
 *
 * \brief The name of the service being invoked.
 */

/*!
 * \fn QScxmlInvokableService::postEvent(QScxmlEvent *event)
 *
 * Sends an \a event to the service.
 */

/*!
 * \fn QScxmlInvokableService::start()
 *
 * Starts the invokable service. Returns \c true on success, or \c false if the
 * invocation fails.
 */

/*!
 * \fn QScxmlInvokableServiceFactory::invoke(QScxmlStateMachine *parentStateMachine)
 *
 * Invokes the service with the parameters given in the constructor, passing
 * \a parentStateMachine as the parent. Returns the new invokable service.
 */

QScxmlInvokableServicePrivate::QScxmlInvokableServicePrivate(QScxmlStateMachine *parentStateMachine)
    : parentStateMachine(parentStateMachine)
{
    static int metaType = qRegisterMetaType<QScxmlInvokableService *>();
    Q_UNUSED(metaType);
}

QScxmlInvokableServiceFactoryPrivate::QScxmlInvokableServiceFactoryPrivate(
        const QScxmlExecutableContent::InvokeInfo &invokeInfo,
        const QList<QScxmlExecutableContent::StringId> &namelist,
        const QList<QScxmlExecutableContent::ParameterInfo> &parameters)
    : invokeInfo(invokeInfo)
    , names(namelist)
    , parameters(parameters)
{}

QScxmlStaticScxmlServiceFactoryPrivate::QScxmlStaticScxmlServiceFactoryPrivate(
        const QMetaObject *metaObject,
        const QScxmlExecutableContent::InvokeInfo &invokeInfo,
        const QList<QScxmlExecutableContent::StringId> &names,
        const QList<QScxmlExecutableContent::ParameterInfo> &parameters)
    : QScxmlInvokableServiceFactoryPrivate(invokeInfo, names, parameters), metaObject(metaObject)
{
}

QScxmlInvokableService::QScxmlInvokableService(QScxmlStateMachine *parentStateMachine,
                                               QScxmlInvokableServiceFactory *factory) :
    QObject(*(new QScxmlInvokableServicePrivate(parentStateMachine)), factory)
{
}

QScxmlStateMachine *QScxmlInvokableService::parentStateMachine() const
{
    Q_D(const QScxmlInvokableService);
    return d->parentStateMachine;
}

QScxmlInvokableServiceFactory::QScxmlInvokableServiceFactory(
        const QScxmlExecutableContent::InvokeInfo &invokeInfo,
        const QList<QScxmlExecutableContent::StringId> &names,
        const QList<QScxmlExecutableContent::ParameterInfo> &parameters,
        QObject *parent)
    : QObject(*(new QScxmlInvokableServiceFactoryPrivate(invokeInfo, names, parameters)), parent)
{}

const QScxmlExecutableContent::InvokeInfo &QScxmlInvokableServiceFactory::invokeInfo() const
{
    Q_D(const QScxmlInvokableServiceFactory);
    return d->invokeInfo;
}

const QList<QScxmlExecutableContent::ParameterInfo> &
QScxmlInvokableServiceFactory::parameters() const
{
    Q_D(const QScxmlInvokableServiceFactory);
    return d->parameters;
}

const QList<QScxmlExecutableContent::StringId> &QScxmlInvokableServiceFactory::names() const
{
    Q_D(const QScxmlInvokableServiceFactory);
    return d->names;
}

QString calculateSrcexpr(QScxmlStateMachine *parent, QScxmlExecutableContent::EvaluatorId srcexpr,
                         bool *ok)
{
    Q_ASSERT(ok);
    *ok = true;
    auto dataModel = parent->dataModel();

    if (srcexpr != QScxmlExecutableContent::NoEvaluator) {
        *ok = false;
        auto v = dataModel->evaluateToString(srcexpr, ok);
        if (!*ok)
            return QString();
        return v;
    }

    return QString();
}

QString QScxmlInvokableServicePrivate::calculateId(
        QScxmlStateMachine *parent, const QScxmlExecutableContent::InvokeInfo &invokeInfo,
        bool *ok) const
{
    Q_ASSERT(ok);
    *ok = true;
    auto stateMachine = parent->tableData();

    if (invokeInfo.id != QScxmlExecutableContent::NoString) {
        return stateMachine->string(invokeInfo.id);
    }

    const QString newId = QScxmlStateMachinePrivate::generateSessionId(
                stateMachine->string(invokeInfo.prefix));

    if (invokeInfo.location != QScxmlExecutableContent::NoString) {
        auto idloc = stateMachine->string(invokeInfo.location);
        auto ctxt = stateMachine->string(invokeInfo.context);
        *ok = parent->dataModel()->setScxmlProperty(idloc, newId, ctxt);
        if (!*ok)
            return QString();
    }

    return newId;
}

QVariantMap QScxmlInvokableServicePrivate::calculateData(
        QScxmlStateMachine *parent,
        const QList<QScxmlExecutableContent::ParameterInfo> &parameters,
        const QList<QScxmlExecutableContent::StringId> &names,
        bool *ok) const
{
    Q_ASSERT(ok);

    QVariantMap result;
    auto dataModel = parent->dataModel();
    auto tableData = parent->tableData();

    for (const QScxmlExecutableContent::ParameterInfo &param : parameters) {
        auto name = tableData->string(param.name);

        if (param.expr != QScxmlExecutableContent::NoEvaluator) {
            *ok = false;
            auto v = dataModel->evaluateToVariant(param.expr, ok);
            if (!*ok)
                return QVariantMap();
            result.insert(name, v);
        } else {
            QString loc;
            if (param.location != QScxmlExecutableContent::NoString) {
                loc = tableData->string(param.location);
            }

            if (loc.isEmpty()) {
                // TODO: error message?
                *ok = false;
                return QVariantMap();
            }

            auto v = dataModel->scxmlProperty(loc);
            result.insert(name, v);
        }
    }

    for (QScxmlExecutableContent::StringId locid : names) {
        QString loc;
        if (locid != QScxmlExecutableContent::NoString) {
            loc = tableData->string(locid);
        }
        if (loc.isEmpty()) {
            // TODO: error message?
            *ok = false;
            return QVariantMap();
        }
        if (dataModel->hasScxmlProperty(loc)) {
            auto v = dataModel->scxmlProperty(loc);
            result.insert(loc, v);
        } else {
            *ok = false;
            return QVariantMap();
        }
    }

    return result;
}

QScxmlScxmlService::~QScxmlScxmlService()
{
    delete m_stateMachine;
}

/*!
  Creates a SCXML service wrapping \a stateMachine, invoked from
  \a parentStateMachine, as a child of \a factory.
 */
QScxmlScxmlService::QScxmlScxmlService(QScxmlStateMachine *stateMachine,
                                       QScxmlStateMachine *parentStateMachine,
                                       QScxmlInvokableServiceFactory *factory)
    : QScxmlInvokableService(parentStateMachine, factory), m_stateMachine(stateMachine)
{
    QScxmlStateMachinePrivate::get(stateMachine)->m_parentStateMachine = parentStateMachine;
}

/*!
 * \reimp
 */
bool QScxmlScxmlService::start()
{
    Q_D(QScxmlInvokableService);
    qCDebug(qscxmlLog) << parentStateMachine() << "preparing to start" << m_stateMachine;

    const QScxmlInvokableServiceFactory *factory
            = qobject_cast<QScxmlInvokableServiceFactory *>(parent());
    Q_ASSERT(factory);

    bool ok = false;
    auto id = d->calculateId(parentStateMachine(), factory->invokeInfo(), &ok);
    if (!ok)
        return false;
    auto data = d->calculateData(parentStateMachine(), factory->parameters(), factory->names(),
                                 &ok);
    if (!ok)
        return false;

    QScxmlStateMachinePrivate::get(m_stateMachine)->m_sessionId = id;
    m_stateMachine->setInitialValues(data);
    if (m_stateMachine->init()) {
        qCDebug(qscxmlLog) << parentStateMachine() << "starting" << m_stateMachine;
        m_stateMachine->start();
        return true;
    }

    qCDebug(qscxmlLog) << parentStateMachine() << "failed to start" << m_stateMachine;
    return false;
}

/*!
  \reimp
 */
QString QScxmlScxmlService::id() const
{
    return m_stateMachine->sessionId();
}

/*!
  \reimp
 */
QString QScxmlScxmlService::name() const
{
    return m_stateMachine->name();
}

/*!
  \reimp
 */
void QScxmlScxmlService::postEvent(QScxmlEvent *event)
{
    QScxmlStateMachinePrivate::get(m_stateMachine)->postEvent(event);
}

QScxmlStateMachine *QScxmlScxmlService::stateMachine() const
{
    return m_stateMachine;
}

/*!
  Creates a factory for dynamically resolved services, passing the attributes of
  the \c <invoke> element as \a invokeInfo, any \c <param> child elements as
  \a parameters, the content of the \c names attribute as \a names, and the
  QObject parent \a parent.
 */
QScxmlDynamicScxmlServiceFactory::QScxmlDynamicScxmlServiceFactory(
        const QScxmlExecutableContent::InvokeInfo &invokeInfo,
        const QList<QScxmlExecutableContent::StringId> &names,
        const QList<QScxmlExecutableContent::ParameterInfo> &parameters,
        QObject *parent)
    : QScxmlInvokableServiceFactory(invokeInfo, names, parameters, parent)
{}

/*!
  \reimp
 */
QScxmlInvokableService *QScxmlDynamicScxmlServiceFactory::invoke(
        QScxmlStateMachine *parentStateMachine)
{
    bool ok = true;
    auto srcexpr = calculateSrcexpr(parentStateMachine, invokeInfo().expr, &ok);
    if (!ok)
        return nullptr;

    return invokeDynamicScxmlService(srcexpr, parentStateMachine, this);
}

QScxmlStaticScxmlServiceFactory::QScxmlStaticScxmlServiceFactory(
        const QMetaObject *metaObject,
        const QScxmlExecutableContent::InvokeInfo &invokeInfo,
        const QList<QScxmlExecutableContent::StringId> &nameList,
        const QList<QScxmlExecutableContent::ParameterInfo> &parameters,
        QObject *parent)
    : QScxmlInvokableServiceFactory(*(new QScxmlStaticScxmlServiceFactoryPrivate(
                                      metaObject, invokeInfo, nameList, parameters)), parent)
{
}

/*!
  \reimp
 */
QScxmlInvokableService *QScxmlStaticScxmlServiceFactory::invoke(
        QScxmlStateMachine *parentStateMachine)
{
    Q_D(const QScxmlStaticScxmlServiceFactory);
    QScxmlStateMachine *instance = qobject_cast<QScxmlStateMachine *>(
                d->metaObject->newInstance(Q_ARG(QObject *, this)));
    return instance ? invokeStaticScxmlService(instance, parentStateMachine, this) : nullptr;
}

QScxmlInvokableServiceFactory::QScxmlInvokableServiceFactory(
        QScxmlInvokableServiceFactoryPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{}

QScxmlScxmlService *invokeStaticScxmlService(QScxmlStateMachine *childStateMachine,
                                             QScxmlStateMachine *parentStateMachine,
                                             QScxmlInvokableServiceFactory *factory)
{
    QScxmlStateMachinePrivate::get(childStateMachine)->setIsInvoked(true);
    return new QScxmlScxmlService(childStateMachine, parentStateMachine, factory);
}

QT_END_NAMESPACE
