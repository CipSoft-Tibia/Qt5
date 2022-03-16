/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the ActiveQt framework of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qstringlist.h>
#include <qvector.h>

#include "qaxfactory.h"

#include <string.h>

#include <qt_windows.h>

QT_BEGIN_NAMESPACE

static DWORD *classRegistration = 0;
static DWORD dwThreadID;
static bool qAxActivity = false;
static HANDLE hEventShutdown;

#ifdef QT_DEBUG
static const DWORD dwTimeOut = 1000;
static const DWORD dwPause = 500;
#else
static const DWORD dwTimeOut = 5000; // time for EXE to be idle before shutting down
static const DWORD dwPause = 1000; // time to wait for threads to finish up
#endif

extern HANDLE hEventShutdown;
extern bool qAxActivity;
extern HANDLE qAxInstance;
extern bool qAxIsServer;
extern bool qAxOutProcServer;
extern wchar_t qAxModuleFilename[MAX_PATH];
extern QString qAxInit();
extern void qAxCleanup();
extern HRESULT UpdateRegistry(BOOL bRegister);
extern HRESULT GetClassObject(const GUID &clsid, const GUID &iid, void **ppUnk);
extern ulong qAxLockCount();
extern bool qax_winEventFilter(void *message);

STDAPI DumpIDL(const QString &outfile, const QString &ver);

// Monitors the shutdown event
static DWORD WINAPI MonitorProc(void* /* pv */)
{
    while (1) {
        WaitForSingleObject(hEventShutdown, INFINITE);
        DWORD dwWait=0;
        do {
            qAxActivity = false;
            dwWait = WaitForSingleObject(hEventShutdown, dwTimeOut);
        } while (dwWait == WAIT_OBJECT_0);
        // timed out
        if (!qAxActivity && !qAxLockCount()) // if no activity let's really bail
            break;
    }
    CloseHandle(hEventShutdown);
    PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
    PostQuitMessage(0);

    return 0;
}

// Starts the monitoring thread
static bool StartMonitor()
{
    dwThreadID = GetCurrentThreadId();
    hEventShutdown = CreateEvent(0, false, false, 0);
    if (hEventShutdown == 0)
        return false;
    DWORD dwThreadID;
    HANDLE h = CreateThread(0, 0, MonitorProc, 0, 0, &dwThreadID);
    return (h != NULL);
}

void qax_shutDown()
{
    qAxActivity = true;
    if (hEventShutdown)
        SetEvent(hEventShutdown); // tell monitor that we transitioned to zero
}

/*
    Start the COM server (if necessary).
*/
bool qax_startServer(QAxFactory::ServerType type)
{
    if (qAxIsServer)
        return true;

    const QStringList keys = qAxFactory()->featureList();
    const int keyCount = keys.count();
    if (!keyCount)
        return false;

    if (!qAxFactory()->isService())
        StartMonitor();

    classRegistration = new DWORD[keyCount];
    int object = 0;
    for (object = 0; object < keyCount; ++object) {
        IUnknown* p = 0;
        CLSID clsid = qAxFactory()->classID(keys.at(object));

        // Create a QClassFactory (implemented in qaxserverbase.cpp)
        HRESULT hRes = GetClassObject(clsid, IID_IClassFactory, reinterpret_cast<void **>(&p));
        if (SUCCEEDED(hRes))
            hRes = CoRegisterClassObject(clsid, p, CLSCTX_LOCAL_SERVER,
                type == QAxFactory::MultipleInstances ? REGCLS_MULTIPLEUSE : REGCLS_SINGLEUSE,
                classRegistration+object);
        if (p)
            p->Release();
    }

    qAxIsServer = true;
    return true;
}

/*
    Stop the COM server (if necessary).
*/
bool qax_stopServer()
{
    if (!qAxIsServer || !classRegistration)
        return true;

    qAxIsServer = false;

    const int keyCount = qAxFactory()->featureList().size();
    for (int object = 0; object < keyCount; ++object)
        CoRevokeClassObject(classRegistration[object]);

    delete []classRegistration;
    classRegistration = 0;

    Sleep(dwPause); //wait for any threads to finish

    return true;
}

QT_END_NAMESPACE

#if defined(QT_NEEDS_QMAIN)
int qMain(int, char **);
#define main qMain
#else
extern "C" int main(int, char **);
#endif

static inline QStringList commandLineArguments()
{
    QStringList result;
    int size;
    if (wchar_t **argv = CommandLineToArgvW(GetCommandLine(), &size)) {
        result.reserve(size);
        wchar_t **argvEnd = argv + size;
        for (wchar_t **a = argv; a < argvEnd; ++a)
            result.append(QString::fromWCharArray(*a));
        LocalFree(argv);
    }
    return result;
}

static inline bool matchesOption(const QString &arg, const char *option)
{
    return (arg.startsWith(QLatin1Char('/')) || arg.startsWith(QLatin1Char('-')))
        && arg.rightRef(arg.size() - 1).compare(QLatin1String(option), Qt::CaseInsensitive) == 0;
}

namespace {
struct Arg {
    explicit Arg(const QStringList &args)
    {
        argv.reserve(args.size() + 1);
        for (const QString &a : args)
            argv.append(_strdup(a.toLocal8Bit().constData()));
        argv.append(nullptr);
    }

    ~Arg()
    {
        for (int i = 0, last = argv.size() - 1; i < last; ++i)
            free(argv.at(i));
    }

    QVector<char *> argv;
};
} // namespace

EXTERN_C int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /* hPrevInstance */, LPSTR, int /* nShowCmd */)
{
    QT_USE_NAMESPACE

    qAxOutProcServer = true;
    GetModuleFileName(0, qAxModuleFilename, MAX_PATH);
    qAxInstance = hInstance;

    const QStringList cmds = commandLineArguments();
    QStringList unprocessed;
    unprocessed.reserve(cmds.size());

    int nRet = 0;
    bool run = true;
    bool runServer = false;
    for (int i = 0; i < cmds.count(); ++i) {
        const QString &cmd = cmds.at(i);
        if (matchesOption(cmd, "activex") || matchesOption(cmd, "embedding")) {
            runServer = true;
        } else if (matchesOption(cmd, "unregserver")) {
            nRet = UpdateRegistry(false);
            run = false;
            break;
        } else if (matchesOption(cmd, "regserver")) {
            nRet = UpdateRegistry(true);
            run = false;
            break;
        } else if (matchesOption(cmd, "dumpidl")) {
            ++i;
            if (i < cmds.count()) {
                const QString &outfile = cmds.at(i);
                ++i;
                QString version;
                if (i < cmds.count() && matchesOption(cmds.at(i), "version")) {
                    ++i;
                    if (i < cmds.count())
                        version = cmds.at(i);
                    else
                        version = QStringLiteral("1.0");
                }

                nRet = DumpIDL(outfile, version);
            } else {
                qWarning("Wrong commandline syntax: <app> -dumpidl <idl file> [-version <x.y.z>]");
            }
            run = false;
            break;
        } else {
            unprocessed.append(cmds.at(i));
        }
    }

    if (run) {
        if (SUCCEEDED(CoInitializeEx(0, COINIT_APARTMENTTHREADED))) {
#ifdef Q_CC_MINGW
            // define GlobalOptions class ID locally for MinGW, since it's missing from the distribution
            static const CLSID CLSID_GlobalOptions =
                { 0x0000034B, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
#endif
            // Disable C++ & SEH exception handling by the COM runtime for out-of-process COM controls.
            // Done to prevent silent crashes and enable crash dump generation.
            IGlobalOptions *globalOptions = nullptr;
            if (SUCCEEDED(CoCreateInstance(CLSID_GlobalOptions, NULL, CLSCTX_INPROC_SERVER,
                                           IID_IGlobalOptions, reinterpret_cast<void **>(&globalOptions)))) {
                globalOptions->Set(COMGLB_EXCEPTION_HANDLING, COMGLB_EXCEPTION_DONOT_HANDLE_ANY);
                globalOptions->Release();
                globalOptions = nullptr;
            }

            {
                Arg args(unprocessed);
                qAxInit();
                if (runServer)
                    QAxFactory::startServer();
                nRet = ::main(args.argv.size() - 1, args.argv.data());
                QAxFactory::stopServer();
                qAxCleanup();
            }
            CoUninitialize();
        } else {
            qErrnoWarning("CoInitializeEx() failed.");
            nRet = -1;
        }
    }

    return nRet;
}
