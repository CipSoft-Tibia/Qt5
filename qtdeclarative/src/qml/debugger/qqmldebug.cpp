// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldebug.h"
#include "qqmldebugconnector_p.h"
#include "qqmldebugserviceinterfaces_p.h"

#include <private/qqmlengine_p.h>
#include <private/qv4compileddata_p.h>

#include <atomic>
#include <cstdio>

QT_REQUIRE_CONFIG(qml_debug);

QT_BEGIN_NAMESPACE

#if __cplusplus >= 202002L
# define Q_ATOMIC_FLAG_INIT {}
#else
# define Q_ATOMIC_FLAG_INIT ATOMIC_FLAG_INIT // deprecated in C++20
#endif

Q_CONSTINIT static std::atomic_flag s_printedWarning = Q_ATOMIC_FLAG_INIT;

void QQmlDebuggingEnabler::enableDebugging(bool printWarning)
{
    if (printWarning && !s_printedWarning.test_and_set(std::memory_order_relaxed))
        fprintf(stderr, "QML debugging is enabled. Only use this in a safe environment.\n");
    QQmlEnginePrivate::qml_debugging_enabled.store(true, std::memory_order_relaxed);
}

#if QT_DEPRECATED_SINCE(6, 4)
QQmlDebuggingEnabler::QQmlDebuggingEnabler(bool printWarning)
{
    enableDebugging(printWarning);
};
#endif // QT_DEPRECATED_SINCE(6, 4)

/*!
 * Retrieves the plugin keys of the debugger services provided by default. The debugger services
 * enable a debug client to use a Qml/JavaScript debugger, in order to set breakpoints, pause
 * execution, evaluate expressions and similar debugging tasks.
 * \return List of plugin keys of default debugger services.
 */
QStringList QQmlDebuggingEnabler::debuggerServices()
{
    return {QV4DebugService::s_key, QQmlEngineDebugService::s_key, QDebugMessageService::s_key};
}

/*!
 * Retrieves the plugin keys of the inspector services provided by default. The inspector services
 * enable a debug client to use a visual inspector tool for Qt Quick.
 * \return List of plugin keys of default inspector services.
 */
QStringList QQmlDebuggingEnabler::inspectorServices()
{
    return {QQmlInspectorService::s_key};
}

/*!
 * Retrieves the names of the profiler services provided by default. The profiler services enable a
 * debug client to use a profiler and track the time taken by various QML and JavaScript constructs,
 * as well as the QtQuick SceneGraph.
 * \return List of plugin keys of default profiler services.
 */
QStringList QQmlDebuggingEnabler::profilerServices()
{
    return {QQmlProfilerService::s_key, QQmlEngineControlService::s_key, QDebugMessageService::s_key};
}

/*!
 * Retrieves the plugin keys of the debug services designed to be used with a native debugger. The
 * native debugger will communicate with these services by directly reading and writing the
 * application's memory.
 * \return List of plugin keys of debug services designed to be used with a native debugger.
 */
QStringList QQmlDebuggingEnabler::nativeDebuggerServices()
{
    return {QQmlNativeDebugService::s_key};
}

/*!
 * Restricts the services available from the debug connector. The connector will scan plugins in the
 * "qmltooling" subdirectory of the default plugin path. If this function is not called before the
 * debug connector is enabled, all services found that way will be available to any client. If this
 * function is called, only the services with plugin keys given in \a services will be available.
 *
 * Use this method to disable debugger and inspector services when profiling to get better
 * performance and more realistic profiles. The debugger service will put any JavaScript engine it
 * connects to into interpreted mode, disabling the JIT compiler.
 *
 * \sa debuggerServices(), profilerServices(), inspectorServices()
 */
void QQmlDebuggingEnabler::setServices(const QStringList &services)
{
    QQmlDebugConnector::setServices(services);
}

/*!
 * \enum QQmlDebuggingEnabler::StartMode
 *
 * Defines the debug connector's start behavior. You can interrupt QML engines starting while a
 * debug client is connecting, in order to set breakpoints in or profile startup code.
 *
 * \value DoNotWaitForClient Run any QML engines as usual while the debug services are connecting.
 * \value WaitForClient      If a QML engine starts while the debug services are connecting,
 *                           interrupt it until they are done.
 */

/*!
 * Enables debugging for QML engines created after calling this function. The debug connector will
 * listen on \a port at \a hostName and block the QML engine until it receives a connection if
 * \a mode is \c WaitForClient. If \a mode is not specified it won't block and if \a hostName is not
 * specified it will listen on all available interfaces. You can only start one debug connector at a
 * time. A debug connector may have already been started if the -qmljsdebugger= command line
 * argument was given. This method returns \c true if a new debug connector was successfully
 * started, or \c false otherwise.
 */
bool QQmlDebuggingEnabler::startTcpDebugServer(int port, StartMode mode, const QString &hostName)
{
    QVariantHash configuration;
    configuration[QLatin1String("portFrom")] = configuration[QLatin1String("portTo")] = port;
    configuration[QLatin1String("block")] = (mode == WaitForClient);
    configuration[QLatin1String("hostAddress")] = hostName;
    return startDebugConnector(QLatin1String("QQmlDebugServer"), configuration);
}

/*!
 * \since 5.6
 *
 * Enables debugging for QML engines created after calling this function. The debug connector will
 * connect to a debugger waiting on a local socket at the given \a socketFileName and block the QML
 * engine until the connection is established if \a mode is \c WaitForClient. If \a mode is not
 * specified it will not block. You can only start one debug connector at a time. A debug connector
 * may have already been started if the -qmljsdebugger= command line argument was given. This method
 * returns \c true if a new debug connector was successfully started, or \c false otherwise.
 */
bool QQmlDebuggingEnabler::connectToLocalDebugger(const QString &socketFileName, StartMode mode)
{
    QVariantHash configuration;
    configuration[QLatin1String("fileName")] = socketFileName;
    configuration[QLatin1String("block")] = (mode == WaitForClient);
    return startDebugConnector(QLatin1String("QQmlDebugServer"), configuration);
}

/*!
 * \since 5.7
 *
 * Enables debugging for QML engines created after calling this function. A debug connector plugin
 * specified by \a pluginName will be loaded and started using the given \a configuration. Supported
 * configuration entries and their semantics depend on the plugin being loaded. You can only start
 * one debug connector at a time. A debug connector may have already been started if the
 * -qmljsdebugger= command line argument was given. This method returns \c true if a new debug
 * connector was successfully started, or \c false otherwise.
 */
bool QQmlDebuggingEnabler::startDebugConnector(const QString &pluginName,
                                               const QVariantHash &configuration)
{
    QQmlDebugConnector::setPluginKey(pluginName);
    QQmlDebugConnector *connector = QQmlDebugConnector::instance();
    return connector ? connector->open(configuration) : false;
}

enum { HookCount = 4 };

// Only add to the end, and bump version if you do.
quintptr Q_QML_EXPORT qtDeclarativeHookData[] = {
    // Version of this Array. Bump if you add to end.
    2,

    // Number of entries in this array.
    HookCount,

    // TypeInformationVersion, an integral value, bumped whenever private
    // object sizes or member offsets that are used in Qt Creator's
    // data structure "pretty printing" change.
    3,

    // Version of the cache data.
    QV4_DATA_STRUCTURE_VERSION
};

Q_STATIC_ASSERT(HookCount == sizeof(qtDeclarativeHookData) / sizeof(qtDeclarativeHookData[0]));

QT_END_NAMESPACE
