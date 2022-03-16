/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "appxlocalengine.h"
#include "appxengine_p.h"

#include "utils.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLoggingCategory>
#include <QtCore/QStandardPaths>
#include <QtCore/QOperatingSystemVersion>

#include <ShlObj.h>
#include <Shlwapi.h>
#include <wsdevlicensing.h>
#include <AppxPackaging.h>
#include <wrl.h>
#include <windows.applicationmodel.h>
#include <windows.management.deployment.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Management::Deployment;
using namespace ABI::Windows::ApplicationModel;
using namespace ABI::Windows::System;

typedef IAsyncOperationWithProgressCompletedHandler<DeploymentResult *, DeploymentProgress> DeploymentResultHandler;
typedef IAsyncOperationWithProgress<DeploymentResult *, DeploymentProgress> DeploymentOperation;

QT_USE_NAMESPACE

// Set a break handler for gracefully breaking long-running ops
static bool g_ctrlReceived = false;
static bool g_handleCtrl = false;
static BOOL WINAPI ctrlHandler(DWORD type)
{
    switch (type) {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
        g_ctrlReceived = g_handleCtrl;
        return g_handleCtrl;
    case CTRL_BREAK_EVENT:
    case CTRL_SHUTDOWN_EVENT:
    default:
        break;
    }
    return false;
}

QString sidForPackage(const QString &packageFamilyName)
{
    QString sid;
    HKEY regKey;
    LONG result = RegOpenKeyEx(
                HKEY_CLASSES_ROOT,
                L"Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppContainer\\Mappings",
                0, KEY_READ, &regKey);
    if (result != ERROR_SUCCESS) {
        qCWarning(lcWinRtRunner) << "Unable to open registry key:" << qt_error_string(result);
        return sid;
    }

    DWORD index = 0;
    wchar_t subKey[MAX_PATH];
    const wchar_t *packageFamilyNameW = wchar(packageFamilyName);
    forever {
        result = RegEnumKey(regKey, index++, subKey, MAX_PATH);
        if (result != ERROR_SUCCESS)
            break;
        wchar_t moniker[MAX_PATH];
        DWORD monikerSize = MAX_PATH;
        result = RegGetValue(regKey, subKey, L"Moniker", RRF_RT_REG_SZ, NULL, moniker, &monikerSize);
        if (result != ERROR_SUCCESS)
            continue;
        if (lstrcmp(moniker, packageFamilyNameW) == 0) {
            sid = QString::fromWCharArray(subKey);
            break;
        }
    }
    RegCloseKey(regKey);
    return sid;
}

class OutputDebugMonitor
{
public:
    OutputDebugMonitor()
        : runLock(CreateEvent(NULL, FALSE, FALSE, NULL)), thread(0)
    {
    }
    ~OutputDebugMonitor()
    {
        if (runLock) {
            SetEvent(runLock);
            CloseHandle(runLock);
        }
        if (thread) {
            WaitForSingleObject(thread, INFINITE);
            CloseHandle(thread);
        }
    }
    void start(const QString &packageFamilyName)
    {
        if (thread) {
            qCWarning(lcWinRtRunner) << "OutputDebugMonitor is already running.";
            return;
        }

        package = packageFamilyName;

        thread = CreateThread(NULL, 0, &monitor, this, NULL, NULL);
        if (!thread) {
            qCWarning(lcWinRtRunner) << "Unable to create thread for app debugging:"
                                     << qt_error_string(GetLastError());
            return;
        }

        return;
    }
private:
    static DWORD __stdcall monitor(LPVOID param)
    {
        OutputDebugMonitor *that = static_cast<OutputDebugMonitor *>(param);

        const QString handleBase = QStringLiteral("Local\\AppContainerNamedObjects\\")
                + sidForPackage(that->package);
        const QString eventName = handleBase + QStringLiteral("\\qdebug-event");
        const QString ackEventName = handleBase + QStringLiteral("\\qdebug-event-ack");
        const QString shmemName = handleBase + QStringLiteral("\\qdebug-shmem");

        HANDLE event = CreateEvent(NULL, FALSE, FALSE, reinterpret_cast<LPCWSTR>(eventName.utf16()));
        if (!event) {
            qCWarning(lcWinRtRunner) << "Unable to open shared event for app debugging:"
                                     << qt_error_string(GetLastError());
            return 1;
        }

        HANDLE ackEvent = CreateEvent(NULL, FALSE, FALSE, reinterpret_cast<LPCWSTR>(ackEventName.utf16()));
        if (!ackEvent) {
            qCWarning(lcWinRtRunner) << "Unable to open shared acknowledge event for app debugging:"
                                     << qt_error_string(GetLastError());
            return 1;
        }

        HANDLE shmem = 0;
        DWORD ret = 0;
        quint64 size = 4096;
        const quint32 resizeMessageType = QtInfoMsg + 1;
        HANDLE handles[] = { that->runLock, event };
        forever {
            DWORD result = WaitForMultipleObjects(2, handles, FALSE, INFINITE);

            // runLock set; exit thread
            if (result == WAIT_OBJECT_0)
                break;

            if (result == WAIT_OBJECT_0 + 1) {
                if (!shmem) {
                    shmem = OpenFileMapping(GENERIC_READ, FALSE,
                                            reinterpret_cast<LPCWSTR>(shmemName.utf16()));
                    if (!shmem) {
                        qCWarning(lcWinRtRunner) << "Unable to open shared memory for app debugging:"
                                                 << qt_error_string(GetLastError());
                        ret = 1;
                        break;
                    }
                }

                const quint32 *data = reinterpret_cast<const quint32 *>(
                            MapViewOfFile(shmem, FILE_MAP_READ, 0, 0, size));
                if (!data) {
                    qCWarning(lcWinRtRunner) << "Unable to map view of shared memory for app debugging:"
                                             << qt_error_string(GetLastError());
                    ret = 1;
                    break;
                }
                const quint32 type = data[0];
                // resize message received; Resize shared memory
                if (type == resizeMessageType) {
                    size = (data[2] << 8) + data[1];
                    if (!UnmapViewOfFile(data)) {
                        qCWarning(lcWinRtRunner) << "Unable to unmap view of shared memory for app debugging:"
                                                 << qt_error_string(GetLastError());
                        ret = 1;
                        break;
                    }
                    if (shmem) {
                        if (!CloseHandle(shmem)) {
                            qCWarning(lcWinRtRunner) << "Unable to close shared memory handle:"
                                                     << qt_error_string(GetLastError());
                            ret = 1;
                            break;
                        }
                        shmem = 0;
                    }
                    SetEvent(ackEvent);
                    continue;
                }

                // debug event set; print message
                QtMsgType messageType = static_cast<QtMsgType>(type);
                QString message = QString::fromWCharArray(
                            reinterpret_cast<const wchar_t *>(data + 1));
                UnmapViewOfFile(data);
                SetEvent(ackEvent);
                switch (messageType) {
                default:
                case QtDebugMsg:
                    qCDebug(lcWinRtRunnerApp, qPrintable(message));
                    break;
                case QtWarningMsg:
                    qCWarning(lcWinRtRunnerApp, qPrintable(message));
                    break;
                case QtCriticalMsg:
                case QtFatalMsg:
                    qCCritical(lcWinRtRunnerApp, qPrintable(message));
                    break;
                }
                continue;
            }

            // An error occurred; exit thread
            qCWarning(lcWinRtRunner) << "Debug output monitor error:"
                                     << qt_error_string(GetLastError());
            ret = 1;
            break;
        }
        if (shmem)
            CloseHandle(shmem);
        if (event)
            CloseHandle(event);
        if (ackEvent)
            CloseHandle(ackEvent);
        return ret;
    }
    HANDLE runLock;
    HANDLE thread;
    QString package;
};
Q_GLOBAL_STATIC(OutputDebugMonitor, debugMonitor)

class AppxLocalEnginePrivate : public AppxEnginePrivate
{
public:
    ComPtr<IPackageManager> packageManager;
    ComPtr<IApplicationActivationManager> appLauncher;
    ComPtr<IPackageDebugSettings> packageDebug;

    Qt::HANDLE loopbackServerProcessHandle = INVALID_HANDLE_VALUE;

    void retrieveInstalledPackages();
};

static bool getManifestFile(const QString &fileName, QString *manifest = 0)
{
    if (!QFile::exists(fileName)) {
        qCWarning(lcWinRtRunner) << fileName << "does not exist.";
        return false;
    }

    // If it looks like an appx manifest, we're done
    if (fileName.endsWith(QStringLiteral("AppxManifest.xml"))) {

        if (manifest)
            *manifest = fileName;
        return true;
    }

    // If it looks like an executable, check that manifest is next to it
    if (fileName.endsWith(QStringLiteral(".exe"))) {
        QDir appDir = QFileInfo(fileName).absoluteDir();
        QString manifestFileName = appDir.absoluteFilePath(QStringLiteral("AppxManifest.xml"));
        if (!QFile::exists(manifestFileName)) {
            qCWarning(lcWinRtRunner) << manifestFileName << "does not exist.";
            return false;
        }

        if (manifest)
            *manifest = manifestFileName;
        return true;
    }

    // TODO: handle already-built package as well

    qCWarning(lcWinRtRunner) << "Appx: unable to determine manifest for" << fileName << ".";
    return false;
}

bool AppxLocalEngine::canHandle(Runner *runner)
{
    return getManifestFile(runner->app());
}

RunnerEngine *AppxLocalEngine::create(Runner *runner)
{
    QScopedPointer<AppxLocalEngine> engine(new AppxLocalEngine(runner));
    if (engine->d_ptr->hasFatalError)
        return 0;

    return engine.take();
}

QStringList AppxLocalEngine::deviceNames()
{
    qCDebug(lcWinRtRunner) << __FUNCTION__;
    return QStringList(QStringLiteral("local"));
}



static ProcessorArchitecture toProcessorArchitecture(APPX_PACKAGE_ARCHITECTURE appxArch)
{
    switch (appxArch) {
    case APPX_PACKAGE_ARCHITECTURE_X86:
        return ProcessorArchitecture_X86;
    case APPX_PACKAGE_ARCHITECTURE_ARM:
        return ProcessorArchitecture_Arm;
    case APPX_PACKAGE_ARCHITECTURE_X64:
        return ProcessorArchitecture_X64;
    case APPX_PACKAGE_ARCHITECTURE_NEUTRAL:
        // fall-through intended
    default:
        return ProcessorArchitecture_Neutral;
    }
}

AppxLocalEngine::AppxLocalEngine(Runner *runner)
    : AppxEngine(runner, new AppxLocalEnginePrivate)
{
    Q_D(AppxLocalEngine);
    if (d->hasFatalError)
        return;
    d->hasFatalError = true;

    HRESULT hr = RoActivateInstance(HString::MakeReference(RuntimeClass_Windows_Management_Deployment_PackageManager).Get(),
                                    &d->packageManager);
    RETURN_VOID_IF_FAILED("Failed to instantiate package manager");

    hr = CoCreateInstance(CLSID_ApplicationActivationManager, nullptr, CLSCTX_INPROC_SERVER,
                          IID_IApplicationActivationManager, &d->appLauncher);
    RETURN_VOID_IF_FAILED("Failed to instantiate application activation manager");

    hr = CoCreateInstance(CLSID_PackageDebugSettings, nullptr, CLSCTX_INPROC_SERVER,
                          IID_IPackageDebugSettings, &d->packageDebug);
    RETURN_VOID_IF_FAILED("Failed to instantiate package debug settings");

    d->retrieveInstalledPackages();

    // Set a break handler for gracefully exiting from long-running operations
    SetConsoleCtrlHandler(&ctrlHandler, true);
    d->hasFatalError = false;
}

AppxLocalEngine::~AppxLocalEngine()
{
}

bool AppxLocalEngine::installPackage(IAppxManifestReader *reader, const QString &filePath)
{
    Q_D(const AppxLocalEngine);
    qCDebug(lcWinRtRunner).nospace().noquote()
        << __FUNCTION__ << " \"" << QDir::toNativeSeparators(filePath) << '"';

    HRESULT hr;
    if (reader) {
        ComPtr<IAppxManifestPackageId> packageId;
        hr = reader->GetPackageId(&packageId);
        RETURN_FALSE_IF_FAILED("Failed to get package ID from reader.");

        LPWSTR name;
        hr = packageId->GetName(&name);
        RETURN_FALSE_IF_FAILED("Failed to get package name from package ID.");

        const QString packageName = QString::fromWCharArray(name);
        CoTaskMemFree(name);
        if (d->installedPackages.contains(packageName)) {
            qCDebug(lcWinRtRunner) << "The package" << packageName << "is already installed.";
            return true;
        }
    }

    const QString nativeFilePath = QDir::toNativeSeparators(QFileInfo(filePath).absoluteFilePath());
    const bool addInsteadOfRegister = nativeFilePath.endsWith(QStringLiteral(".appx"),
                                                              Qt::CaseInsensitive);
    ComPtr<IUriRuntimeClass> uri;
    hr = d->uriFactory->CreateUri(hStringFromQString(nativeFilePath), &uri);
    RETURN_FALSE_IF_FAILED("Failed to create an URI for the package");

    ComPtr<DeploymentOperation> deploymentOperation;
    if (addInsteadOfRegister) {
        hr = d->packageManager->AddPackageAsync(uri.Get(), NULL, DeploymentOptions_None,
                                                &deploymentOperation);
        RETURN_FALSE_IF_FAILED("Failed to add package");
    } else {
        hr = d->packageManager->RegisterPackageAsync(uri.Get(), 0,
                                                     DeploymentOptions_DevelopmentMode,
                                                     &deploymentOperation);
        RETURN_FALSE_IF_FAILED("Failed to start package registration");
    }

    HANDLE ev = CreateEvent(NULL, FALSE, FALSE, NULL);
    hr = deploymentOperation->put_Completed(Callback<DeploymentResultHandler>([ev](DeploymentOperation *, AsyncStatus) {
                                        SetEvent(ev);
                                        return S_OK;
                                    }).Get());
    RETURN_FALSE_IF_FAILED("Could not register deployment completed callback.");
    DWORD ret = WaitForSingleObjectEx(ev, 60000, FALSE);
    CloseHandle(ev);
    if (ret != WAIT_OBJECT_0) {
        if (ret == WAIT_TIMEOUT)
            qCWarning(lcWinRtRunner) << "Deployment did not finish within 15 seconds.";
        else
            qCWarning(lcWinRtRunner) << "Deployment finished event was not triggered.";
        return false;
    }

    ComPtr<IDeploymentResult> results;
    hr = deploymentOperation->GetResults(&results);
    RETURN_FALSE_IF_FAILED("Failed to retrieve package registration results.");

    HRESULT errorCode;
    hr = results->get_ExtendedErrorCode(&errorCode);
    RETURN_FALSE_IF_FAILED("Failed to retrieve extended error code.");

    if (FAILED(errorCode)) {
        if (HRESULT_CODE(errorCode) == ERROR_INSTALL_PREREQUISITE_FAILED) {
            qCWarning(lcWinRtRunner) << "Unable to register package: A requirement for installation was not met. "
                "Check that your Windows version matches TargetDeviceFamily's MinVersion set in your AppxManifest.xml.";
        } else {
            HString errorText;
            if (SUCCEEDED(results->get_ErrorText(errorText.GetAddressOf()))) {
                qCWarning(lcWinRtRunner) << "Unable to register package:"
                    << QString::fromWCharArray(errorText.GetRawBuffer(NULL));
            }
        }
        if (HRESULT_CODE(errorCode) == ERROR_INSTALL_POLICY_FAILURE) {
            // The user's license has expired. Give them the opportunity to renew it.
            FILETIME expiration;
            hr = AcquireDeveloperLicense(GetForegroundWindow(), &expiration);
            RETURN_FALSE_IF_FAILED("Unable to renew developer license");
            return install(false);
        }
        return false;
    }

    return SUCCEEDED(hr);
}

bool AppxLocalEngine::parseExitCode()
{
    Q_D(AppxLocalEngine);
    const QString exitFileName(QStringLiteral("exitCode.tmp"));
    bool ok = false;
    QFile exitCodeFile(devicePath(QString::number(pid()).append(QStringLiteral(".pid"))));
    if (exitCodeFile.open(QIODevice::ReadOnly)) {
        d->exitCode = exitCodeFile.readAll().toInt(&ok);
        exitCodeFile.close();
        exitCodeFile.remove();
    }
    if (!ok && !GetExitCodeProcess(d->processHandle, &d->exitCode)) {
        d->exitCode = UINT_MAX;
        qCWarning(lcWinRtRunner).nospace() << "Failed to obtain process exit code.";
        qCDebug(lcWinRtRunner, "GetLastError: 0x%x", GetLastError());
        return false;
    }
    return true;
}

bool AppxLocalEngine::install(bool removeFirst)
{
    Q_D(const AppxLocalEngine);
    qCDebug(lcWinRtRunner) << __FUNCTION__;

    ComPtr<IPackage> packageInformation;
    HRESULT hr = d->packageManager->FindPackageByUserSecurityIdPackageFullName(
                NULL, hStringFromQString(d->packageFullName), &packageInformation);
    if (SUCCEEDED(hr) && packageInformation) {
        qCWarning(lcWinRtRunner) << "Package already installed.";
        if (removeFirst)
            remove();
        else
            return true;
    }

    return installDependencies() && installPackage(nullptr, d->manifest);
}

bool AppxLocalEngine::remove()
{
    Q_D(const AppxLocalEngine);
    qCDebug(lcWinRtRunner) << __FUNCTION__;

    // ### TODO: use RemovePackageWithOptions to preserve previous state when re-installing
    ComPtr<DeploymentOperation> deploymentOperation;
    HRESULT hr = d->packageManager->RemovePackageAsync(hStringFromQString(d->packageFullName), &deploymentOperation);
    RETURN_FALSE_IF_FAILED("Unable to start package removal");

    HANDLE ev = CreateEvent(NULL, FALSE, FALSE, NULL);
    hr = deploymentOperation->put_Completed(Callback<DeploymentResultHandler>([ev](DeploymentOperation *, AsyncStatus) {
                                        SetEvent(ev);
                                        return S_OK;
                                    }).Get());
    RETURN_FALSE_IF_FAILED("Could not register deployment completed callback.");
    DWORD ret = WaitForSingleObjectEx(ev, 60000, FALSE);
    CloseHandle(ev);
    if (ret != WAIT_OBJECT_0) {
        if (ret == WAIT_TIMEOUT)
            qCWarning(lcWinRtRunner) << "Deployment did not finish within 15 seconds.";
        else
            qCWarning(lcWinRtRunner) << "Deployment finished event was not triggered.";
        return false;
    }

    ComPtr<IAsyncInfo> asyncInfo;
    hr = deploymentOperation.As(&asyncInfo);
    RETURN_FALSE_IF_FAILED("Failed to cast deployment operation.");

    AsyncStatus status;
    hr = asyncInfo->get_Status(&status);
    RETURN_FALSE_IF_FAILED("Failed to retrieve deployment operation's status.");

    if (status != Completed) {
        qCWarning(lcWinRtRunner) << "Unable to remove package.";
        return false;
    }

    return true;
}

bool AppxLocalEngine::start()
{
    Q_D(AppxLocalEngine);
    qCDebug(lcWinRtRunner) << __FUNCTION__;

    const QString launchArguments =
            (d->runner->arguments() << QStringLiteral("-qdevel")).join(QLatin1Char(' '));
    DWORD pid;
    const QString activationId = d->packageFamilyName + QStringLiteral("!App");
    HRESULT hr = d->appLauncher->ActivateApplication(wchar(activationId),
                                                     wchar(launchArguments), AO_NONE, &pid);
    RETURN_FALSE_IF_FAILED("Failed to activate application");
    d->pid = qint64(pid);
    CloseHandle(d->processHandle);
    d->processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, true, pid);

    return true;
}

bool AppxLocalEngine::enableDebugging(const QString &debuggerExecutable, const QString &debuggerArguments)
{
    Q_D(AppxLocalEngine);

    const QString &debuggerCommand = debuggerExecutable + QLatin1Char(' ') + debuggerArguments;
    HRESULT hr = d->packageDebug->EnableDebugging(wchar(d->packageFullName),
                                                  wchar(debuggerCommand),
                                                  NULL);
    RETURN_FALSE_IF_FAILED("Failed to enable debugging for application");
    return true;
}

bool AppxLocalEngine::disableDebugging()
{
    Q_D(AppxLocalEngine);

    HRESULT hr = d->packageDebug->DisableDebugging(wchar(d->packageFullName));
    RETURN_FALSE_IF_FAILED("Failed to disable debugging for application");

    return true;
}

bool AppxLocalEngine::setLoopbackExemptClientEnabled(bool enabled)
{
    Q_D(AppxLocalEngine);
    qCDebug(lcWinRtRunner) << __FUNCTION__ << enabled;

    if (!enabled) {
        PACKAGE_EXECUTION_STATE state;
        HRESULT hr = d->packageDebug->GetPackageExecutionState(wchar(d->packageFullName), &state);
        RETURN_FALSE_IF_FAILED("Failed to get package execution state");
        if (state != PES_TERMINATED && state != PES_UNKNOWN) {
            qCWarning(lcWinRtRunner) << "Cannot unregister loopback exemption for running program."
                                     << "Please use checknetisolation.exe to check/clean up the exemption list.";
            return false;
        }
    }

    QByteArray stdOut;
    QByteArray stdErr;
    unsigned long exitCode = 0;
    QString errorMessage;
    QStringList arguments;
    const QString binary = QStringLiteral("checknetisolation.exe");
    arguments << QStringLiteral("LoopbackExempt")
              << (enabled ? QStringLiteral("-a") : QStringLiteral("-d"))
              << QStringLiteral("-p=") + sidForPackage(d->packageFamilyName);
    if (!runProcess(binary, arguments, QString(), &exitCode, &stdOut, &stdErr, &errorMessage)) {
        qCWarning(lcWinRtRunner) << "Could not run" << binary;
        return false;
    }
    if (exitCode) {
        if (errorMessage.isEmpty()) {
            errorMessage = binary + QStringLiteral(" returned ") + QString::number(exitCode)
                        + QStringLiteral(": ") + QString::fromLocal8Bit(stdErr);
        }
        qCWarning(lcWinRtRunner) << errorMessage;
        return false;
    }
    return true;
}

bool AppxLocalEngine::setLoopbackExemptServerEnabled(bool enabled)
{
    Q_D(AppxLocalEngine);
    const QOperatingSystemVersion minimal
        = QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10, 0, 14393);
    if (QOperatingSystemVersion::current() < minimal) {
        qCWarning(lcWinRtRunner) << "Cannot enable loopback exemption for servers. If you want"
                                 << "to use this feature please update to a Windows version >="
                                 << minimal;
        return false;
    }

    if (enabled) {
        QStringList arguments;
        const QString binary = QStringLiteral("checknetisolation.exe");
        arguments << QStringLiteral("LoopbackExempt") << QStringLiteral("-is")
                  << QStringLiteral("-p=") + sidForPackage(d->packageFamilyName);
        if (!runElevatedBackgroundProcess(binary, arguments, &d->loopbackServerProcessHandle)) {
            qCWarning(lcWinRtRunner) << "Could not start" << binary;
            return false;
        }
    } else {
        if (d->loopbackServerProcessHandle != INVALID_HANDLE_VALUE) {
            if (!TerminateProcess(d->loopbackServerProcessHandle, 0)) {
                qCWarning(lcWinRtRunner) << "Could not terminate loopbackexempt debug session";
                return false;
            }
            if (!CloseHandle(d->loopbackServerProcessHandle)) {
                qCWarning(lcWinRtRunner) << "Could not close loopbackexempt debug session process handle";
                return false;
            }
        } else {
            qCWarning(lcWinRtRunner) << "loopbackexempt debug session could not be found";
            return false;
        }
    }
    return true;
}

bool AppxLocalEngine::setLoggingRules(const QByteArray &rules)
{
    qCDebug(lcWinRtRunner) << __FUNCTION__;

    QDir loggingIniDir(devicePath(QLatin1String("QtProject")));
    if (!loggingIniDir.exists() && !loggingIniDir.mkpath(QStringLiteral("."))) {
        qCWarning(lcWinRtRunner) << "Could not create" << loggingIniDir;
        return false;
    }
    QFile loggingIniFile(loggingIniDir.absolutePath().append(QLatin1String("/qtlogging.ini")));
    if (loggingIniFile.exists() && !loggingIniFile.remove()) {
        qCWarning(lcWinRtRunner) << loggingIniFile << "already exists.";
        return false;
    }
    if (!loggingIniFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(lcWinRtRunner) << "Could not open" << loggingIniFile << "for writing.";
        return false;
    }

    QTextStream stream(&loggingIniFile);
    stream << "[Rules]\n" << rules;

    return true;
}


bool AppxLocalEngine::suspend()
{
    Q_D(AppxLocalEngine);
    qCDebug(lcWinRtRunner) << __FUNCTION__;

    HRESULT hr = d->packageDebug->Suspend(wchar(d->packageFullName));
    RETURN_FALSE_IF_FAILED("Failed to suspend application");

    return true;
}

bool AppxLocalEngine::waitForFinished(int secs)
{
    Q_D(AppxLocalEngine);
    qCDebug(lcWinRtRunner) << __FUNCTION__;

    debugMonitor->start(d->packageFamilyName);

    g_handleCtrl = true;
    int time = 0;
    forever {
        PACKAGE_EXECUTION_STATE state;
        HRESULT hr = d->packageDebug->GetPackageExecutionState(wchar(d->packageFullName), &state);
        RETURN_FALSE_IF_FAILED("Failed to get package execution state");
        qCDebug(lcWinRtRunner) << "Current execution state:" << state;
        if (state == PES_TERMINATED || state == PES_UNKNOWN)
            break;

        ++time;
        if ((secs && time > secs) || g_ctrlReceived) {
            g_handleCtrl = false;
            return false;
        }

        Sleep(1000); // Wait one second between checks
        qCDebug(lcWinRtRunner) << "Waiting for app to quit - msecs to go:" << secs - time;
    }
    g_handleCtrl = false;

    if (!GetExitCodeProcess(d->processHandle, &d->exitCode))
        d->exitCode = UINT_MAX;

    return true;
}

bool AppxLocalEngine::stop()
{
    Q_D(AppxLocalEngine);
    qCDebug(lcWinRtRunner) << __FUNCTION__;

    // ### We won't have a process handle if we didn't start the app. We can look it up
    // using a process snapshot, or by calling start if we know the process is already running.
    // For now, simply continue normally, but don't fetch the exit code.
    if (!d->processHandle)
        qCDebug(lcWinRtRunner) << "No handle to the process; the exit code won't be available.";

    if (d->processHandle && !parseExitCode()) {
        return false;
    }

    if (!d->processHandle || d->exitCode == STILL_ACTIVE) {
        HRESULT hr = d->packageDebug->TerminateAllProcesses(wchar(d->packageFullName));
        RETURN_FALSE_IF_FAILED("Failed to terminate package process");

        if (d->processHandle && !GetExitCodeProcess(d->processHandle, &d->exitCode))
            d->exitCode = UINT_MAX;
    }

    return true;
}

QString AppxLocalEngine::devicePath(const QString &relativePath) const
{
    Q_D(const AppxLocalEngine);
    qCDebug(lcWinRtRunner) << __FUNCTION__;

    // Return a path safe for passing to the application
    QDir localAppDataPath(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    const QString path = localAppDataPath.absoluteFilePath(
                QStringLiteral("Packages/") + d->packageFamilyName
                + QStringLiteral("/LocalState/") + relativePath);
    return QDir::toNativeSeparators(path);
}

bool AppxLocalEngine::sendFile(const QString &localFile, const QString &deviceFile)
{
    qCDebug(lcWinRtRunner) << __FUNCTION__;

    // Both files are local, just use QFile
    QFile source(localFile);

    // Remove the destination, or copy will fail
    if (QFileInfo(source) != QFileInfo(deviceFile))
        QFile::remove(deviceFile);

    bool result = source.copy(deviceFile);
    if (!result) {
        qCWarning(lcWinRtRunner).nospace().noquote()
            << "Unable to sendFile: " << source.errorString();
    }

    return result;
}

bool AppxLocalEngine::receiveFile(const QString &deviceFile, const QString &localFile)
{
    qCDebug(lcWinRtRunner) << __FUNCTION__;

    // Both files are local, so just reverse the sendFile arguments
    return sendFile(deviceFile, localFile);
}

QString AppxLocalEngine::extensionSdkPath() const
{
    const QByteArray extensionSdkDirRaw = qgetenv("ExtensionSdkDir");
    if (extensionSdkDirRaw.isEmpty()) {
        qCWarning(lcWinRtRunner) << "The environment variable ExtensionSdkDir is not set.";
        return QString();
    }
    return QString::fromLocal8Bit(extensionSdkDirRaw);
}

void AppxLocalEnginePrivate::retrieveInstalledPackages()
{
    qCDebug(lcWinRtRunner) << __FUNCTION__;

    ComPtr<ABI::Windows::Foundation::Collections::IIterable<Package*>> packages;
    HRESULT hr = packageManager->FindPackagesByUserSecurityId(NULL, &packages);
    RETURN_VOID_IF_FAILED("Failed to find packages");

    ComPtr<ABI::Windows::Foundation::Collections::IIterator<Package*>> pkgit;
    hr = packages->First(&pkgit);
    RETURN_VOID_IF_FAILED("Failed to get package iterator");

    boolean hasCurrent;
    hr = pkgit->get_HasCurrent(&hasCurrent);
    while (SUCCEEDED(hr) && hasCurrent) {
        ComPtr<IPackage> pkg;
        hr = pkgit->get_Current(&pkg);
        RETURN_VOID_IF_FAILED("Failed to get current package");

        ComPtr<IPackageId> pkgId;
        hr = pkg->get_Id(&pkgId);
        RETURN_VOID_IF_FAILED("Failed to get package id");

        HString name;
        hr = pkgId->get_Name(name.GetAddressOf());
        RETURN_VOID_IF_FAILED("Failed retrieve package name");

        ProcessorArchitecture architecture;
        if (packageArchitecture == ProcessorArchitecture_Neutral) {
            architecture = packageArchitecture;
        } else {
            hr = pkgId->get_Architecture(&architecture);
            RETURN_VOID_IF_FAILED("Failed to retrieve package architecture");
        }

        const QString pkgName = QStringFromHString(name.Get());
        qCDebug(lcWinRtRunner) << "found installed package" << pkgName;

        if (architecture == packageArchitecture)
            installedPackages.insert(pkgName);

        hr = pkgit->MoveNext(&hasCurrent);
    }
}
