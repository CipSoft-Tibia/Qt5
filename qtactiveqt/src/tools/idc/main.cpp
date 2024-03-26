// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QDir>
#include <QFile>
#include <QDir>
#include <QScopedArrayPointer>
#include <qt_windows.h>
#include <io.h>

QT_BEGIN_NAMESPACE

static QString quotePath(const QString &s)
{
    if (!s.startsWith(QLatin1Char('\"')) && s.contains(QLatin1Char(' ')))
        return QLatin1Char('\"') + s + QLatin1Char('\"');
    return s;
}

static bool hasFileExtension(const QString &filePath, const QString &extension)
{
    return filePath.endsWith(extension, Qt::CaseInsensitive);
}

static bool hasExeExtension(const QString &filePath)
{
    return hasFileExtension(filePath, QStringLiteral(".exe"));
}

static bool hasDllExtension(const QString &filePath)
{
    return hasFileExtension(filePath, QStringLiteral(".dll"));
}

// Prepend the Qt binary directory to PATH.
static bool prependPath()
{
    enum { maxEnvironmentSize = 32767 };
    wchar_t buffer[maxEnvironmentSize];
    if (!GetModuleFileName(nullptr, buffer, maxEnvironmentSize))
        return false;
    wchar_t *ptr = wcsrchr(buffer, L'\\');
    if (!ptr)
        return false;
    *ptr++ = L';';
    const wchar_t pathVariable[] = L"PATH";
    return GetEnvironmentVariable(pathVariable, ptr, DWORD(maxEnvironmentSize - (ptr - buffer))) != 0
        && SetEnvironmentVariable(pathVariable, buffer) == TRUE;
}

static QString errorString(DWORD errorCode)
{
    wchar_t *resultW = nullptr;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                  nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  reinterpret_cast<LPWSTR>(&resultW), 0, nullptr);
    const QString result = QString::fromWCharArray(resultW);
    LocalFree(resultW);
    return result;
}

static bool runWithQtInEnvironment(const QString &cmd)
{
    enum { timeOutMs = 30000 };
    static const bool pathSet = prependPath();
    if (!pathSet)
        return false;

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    STARTUPINFO myInfo;
    GetStartupInfo(&myInfo);
    si.hStdInput = myInfo.hStdInput;
    si.hStdOutput = myInfo.hStdOutput;
    si.hStdError = myInfo.hStdError;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    QScopedArrayPointer<wchar_t> commandLineW(new wchar_t[cmd.size() + 1]);
    cmd.toWCharArray(commandLineW.data());
    commandLineW[cmd.size()] = 0;
    if (!CreateProcessW(nullptr, commandLineW.data(), nullptr, nullptr, /* InheritHandles */ TRUE, 0, nullptr, nullptr, &si, &pi)) {
        fprintf(stderr, "Unable to execute \"%s\": %s\n", qPrintable(cmd),
                qPrintable(errorString(GetLastError())));
        return false;
    }

    DWORD exitCode = 1;
    switch (WaitForSingleObject(pi.hProcess, timeOutMs)) {
    case WAIT_OBJECT_0:
        GetExitCodeProcess(pi.hProcess, &exitCode);
        break;
    case WAIT_TIMEOUT:
        fprintf(stderr, "Timed out after %d ms out waiting for \"%s\".\n",
                int(timeOutMs), qPrintable(cmd));
        TerminateProcess(pi.hProcess, 1);
        break;
    default:
        fprintf(stderr, "Error waiting for \"%s\": %s\n",
                qPrintable(cmd), qPrintable(errorString(GetLastError())));
        TerminateProcess(pi.hProcess, 1);
        break;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    if (exitCode)
        fprintf(stderr, "\"%s\" returned exit code: %lu (0x%lx)\n", qPrintable(cmd), exitCode, exitCode);
    return exitCode == 0;
}

static bool attachTypeLibrary(const QString &applicationName, int resource, const QByteArray &data, QString *errorMessage)
{
    HANDLE hExe = BeginUpdateResource(reinterpret_cast<const wchar_t *>(applicationName.utf16()), false);
    if (hExe == nullptr) {
        if (errorMessage)
            *errorMessage = QString::fromLatin1("Failed to attach type library to binary %1 - could not open file.").arg(applicationName);
        return false;
    }
    if (!UpdateResource(hExe, L"TYPELIB", MAKEINTRESOURCE(resource), 0,
                        const_cast<char *>(data.data()), DWORD(data.size()))) {
        EndUpdateResource(hExe, true);
        if (errorMessage)
            *errorMessage = QString::fromLatin1("Failed to attach type library to binary %1 - could not update file.").arg(applicationName);
        return false;
    }

    if (!EndUpdateResource(hExe,false)) {
        if (errorMessage)
            *errorMessage = QString::fromLatin1("Failed to attach type library to binary %1 - could not write file.").arg(applicationName);
        return false;
    }

    if (errorMessage)
        *errorMessage = QString::fromLatin1("Type library attached to %1.").arg(applicationName);
    return true;
}

// Manually add defines that are missing from pre-VS2012 compilers
#ifndef LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
#  define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR 0x00000100
#endif
#ifndef LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
#  define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS 0x00001000
#endif

static HMODULE loadLibraryQt(const QString &input)
{
    const wchar_t *inputC = reinterpret_cast<const wchar_t *>(input.utf16());
    // Load DLL with the folder containing the DLL temporarily added to the search path when loading dependencies
    const UINT oldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    HMODULE result =
        LoadLibraryEx(inputC, nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    // If that fails, call with flags=0 to get LoadLibrary() behavior (search %PATH%).
    if (!result)
        result = LoadLibraryEx(inputC, nullptr, 0);
    SetErrorMode(oldErrorMode);
    return result;
}

static bool dllInstall(const QString &input, bool doRegister)
{
    HMODULE hdll = loadLibraryQt(input);
    if (!hdll) {
        fprintf(stderr, "Couldn't load library file %s\n", qPrintable(input));
        return false;
    }

    typedef HRESULT(__stdcall* DllInstallProc)(BOOL bInstall, PCWSTR pszCmdLine);
    DllInstallProc DllInstall = reinterpret_cast<DllInstallProc>
            (reinterpret_cast<void *>(GetProcAddress(hdll, "DllInstall")));
    if (!DllInstall) {
        fprintf(stderr, "Library file %s doesn't appear to be a COM library supporting DllInstall\n", qPrintable(input));
        return false;
    }

    return DllInstall(doRegister, L"user") == S_OK;
}

static bool registerServer(const QString &input, bool perUser)
{
    bool ok = false;
    if (hasExeExtension(input)) {
        ok = runWithQtInEnvironment(quotePath(input) + QLatin1String(perUser ? " -regserverperuser" : " -regserver"));
    } else {
        if (perUser) {
            return dllInstall(input, true);
        } else {
            HMODULE hdll = loadLibraryQt(input);
            if (!hdll) {
                fprintf(stderr, "Couldn't load library file %s\n", qPrintable(input));
                return false;
            }

            typedef HRESULT(__stdcall* RegServerProc)();
            RegServerProc DllRegisterServer = reinterpret_cast<RegServerProc>
                    (reinterpret_cast<void *>(GetProcAddress(hdll, "DllRegisterServer")));
            if (!DllRegisterServer) {
                fprintf(stderr, "Library file %s doesn't appear to be a COM library\n", qPrintable(input));
                return false;
            }
            ok = DllRegisterServer() == S_OK;
        }
    }
    return ok;
}

static bool unregisterServer(const QString &input, bool perUser)
{
    bool ok = false;
    if (hasExeExtension(input)) {
        ok = runWithQtInEnvironment(quotePath(input) + QLatin1String(perUser ? " -unregserverperuser" : " -unregserver"));
    } else {
        if (perUser) {
            return dllInstall(input, false);
        } else {
            HMODULE hdll = loadLibraryQt(input);
            if (!hdll) {
                fprintf(stderr, "Couldn't load library file %s\n", qPrintable(input));
                return false;
            }

            typedef HRESULT(__stdcall* RegServerProc)();
            RegServerProc DllUnregisterServer = reinterpret_cast<RegServerProc>
                    (reinterpret_cast<void *>(GetProcAddress(hdll, "DllUnregisterServer")));
            if (!DllUnregisterServer) {
                fprintf(stderr, "Library file %s doesn't appear to be a COM library\n", qPrintable(input));
                return false;
            }
            ok = DllUnregisterServer() == S_OK;
        }
    }
    return ok;
}

static HRESULT dumpIdl(const QString &input, const QString &idlfile, const QString &version)
{
    HRESULT res = E_FAIL;

    if (hasExeExtension(input)) {
        const QString command = quotePath(input) + QLatin1String(" -dumpidl ")
            + quotePath(idlfile) + QLatin1String(" -version ") + version;
        if (runWithQtInEnvironment(command))
            res = S_OK;
    } else {
        HMODULE hdll = loadLibraryQt(input);
        if (!hdll) {
            fprintf(stderr, "Couldn't load library file %s\n", qPrintable(input));
            return 3;
        }
        typedef HRESULT(__stdcall* DumpIDLProc)(const QString&, const QString&);
        DumpIDLProc DumpIDL = reinterpret_cast<DumpIDLProc>
                (reinterpret_cast<void *>(GetProcAddress(hdll, "DumpIDL")));
        if (!DumpIDL) {
            fprintf(stderr, "Couldn't resolve 'DumpIDL' symbol in %s\n", qPrintable(input));
            return 3;
        }
        res = DumpIDL(idlfile, version);
        FreeLibrary(hdll);
    }

    return res;
}

const char usage[] =
"Usage: idc [options] [input_file]\n"
"Interface Description Compiler " QT_VERSION_STR "\n\n"
"Options:\n"
"  /?, -?, /h, -h, /help, -help      Displays this help.\n"
"  /v, -v                            Displays version information.\n"
"  /version, -version <version>      Specify the interface version.\n"
"  /idl, -idl <file>                 Specify the interface definition file.\n"
"  /tlb, -tlb <file>                 Specify the type library file.\n"
"  /regserver, -regserver            Register server.\n"
"  /unregserver, -unregserver        Unregister server.\n\n"
"  /regserverperuser, -regserverperuser     Per-user register server.\n"
"  /unregserverperuser, -unregserverperuser Per-user unregister server.\n\n"
"Examples:\n"
"idc -regserver l.dll                Register the COM server l.dll\n"
"idc -unregserver l.dll              Unregister the COM server l.dll\n"
"idc l.dll -idl l.idl -version 2.3   Writes the IDL of the server dll to the file idl.\n"
"                                    The type library will have version 2.3\n"
"idc l.dll -tlb l.tlb                Replaces the type library in l.dll with l.tlb\n";

enum Mode { RegisterServer, UnregisterServer, RegServerPerUser, UnregServerPerUser, Other };

int runIdc(int argc, char **argv)
{
    QString error;
    QString tlbfile;
    QString idlfile;
    QString input;
    QString version = QLatin1String("1.0");
    Mode mode = Other;

    int i = 1;
    while (i < argc) {
        QString p = QString::fromLocal8Bit(argv[i]).toLower();

        if (p == QLatin1String("/idl") || p == QLatin1String("-idl")) {
            ++i;
            if (i > argc) {
                error = QLatin1String("Missing name for interface definition file!");
                break;
            }
            idlfile = QFile::decodeName(argv[i]).trimmed();
        } else if (p == QLatin1String("/version") || p == QLatin1String("-version")) {
            ++i;
            if (i > argc)
                version = QLatin1String("1.0");
            else
                version = QLatin1String(argv[i]);
        } else if (p == QLatin1String("/tlb") || p == QLatin1String("-tlb")) {
            ++i;
            if (i > argc) {
                error = QLatin1String("Missing name for type library file!");
                break;
            }
            tlbfile = QFile::decodeName(argv[i]).trimmed();
        } else if (p == QLatin1String("/v") || p == QLatin1String("-v")) {
            fprintf(stdout, "Qt Interface Definition Compiler version 1.0 using Qt %s\n", QT_VERSION_STR);
            return 0;
        } else if (p == QLatin1String("/h") || p == QLatin1String("-h")
                   || p == QLatin1String("/?") || p == QLatin1String("-?")
                   || p == QLatin1String("/help") || p == QLatin1String("-help")) {
            fprintf(stdout, "%s\n", usage);
            return 0;
        } else if (p == QLatin1String("/regserver") || p == QLatin1String("-regserver")) {
            mode = RegisterServer;
        } else if (p == QLatin1String("/unregserver") || p == QLatin1String("-unregserver")) {
            mode = UnregisterServer;
        } else if (p == QLatin1String("/regserverperuser") || p == QLatin1String("-regserverperuser")) {
            mode = RegServerPerUser;
        } else if (p == QLatin1String("/unregserverperuser") || p == QLatin1String("-unregserverperuser")) {
            mode = UnregServerPerUser;
        } else if (p[0] == QLatin1Char('/') || p[0] == QLatin1Char('-')) {
            error = QLatin1String("Unknown option \"") + p + QLatin1Char('"');
            break;
        } else {
            input = QFile::decodeName(argv[i]).trimmed();
            // LoadLibraryEx requires a fully qualified path when used together with LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
            input = QFileInfo(input).absoluteFilePath();
            input = QDir::toNativeSeparators(input);
        }
        i++;
    }
    if (!error.isEmpty()) {
        fprintf(stderr, "%s\n", qPrintable(error));
        return 5;
    }
    if (input.isEmpty()) {
        fprintf(stderr, "No input file specified!\n\n%s\n", usage);
        return 1;
    }

    switch (mode) {
    case RegisterServer:
        if (!registerServer(input, false)) {
            fprintf(stderr, "Failed to register server!\n");
            return 1;
        }
        fprintf(stderr, "Server registered successfully!\n");
        return 0;
    case UnregisterServer:
        if (!unregisterServer(input, false)) {
            fprintf(stderr, "Failed to unregister server!\n");
            return 1;
        }
        fprintf(stderr, "Server unregistered successfully!\n");
        return 0;
    case RegServerPerUser:
        if (!registerServer(input, true)) {
            fprintf(stderr, "Failed to register server per user!\n");
            return 1;
        }
        fprintf(stderr, "Server registered successfully per user!\n");
        return 0;
    case UnregServerPerUser:
        if (!unregisterServer(input, true)) {
            fprintf(stderr, "Failed to unregister server per user!\n");
            return 1;
        }
        fprintf(stderr, "Server unregistered successfully per user!\n");
        return 0;
    case Other:
        break;
    }

    if (hasExeExtension(input) && tlbfile.isEmpty() && idlfile.isEmpty()) {
        fprintf(stderr, "No type output file specified!\n");
        return 2;
    }
    if (hasDllExtension(input) && idlfile.isEmpty() && tlbfile.isEmpty()) {
        fprintf(stderr, "No interface definition file and no type library file specified!\n");
        return 3;
    }
    if (!tlbfile.isEmpty()) {
        tlbfile = QDir::toNativeSeparators(tlbfile);
        QFile file(tlbfile);
        if (!file.open(QIODevice::ReadOnly)) {
            fprintf(stderr, "Couldn't open %s for read: %s\n", qPrintable(tlbfile), qPrintable(file.errorString()));
            return 4;
        }
        QString error;
        const bool ok = attachTypeLibrary(input, 1, file.readAll(), &error);
        fprintf(stderr, "%s\n", qPrintable(error));
        return ok ? 0 : 4;
    }
    if (!idlfile.isEmpty()) {
        idlfile = QDir::toNativeSeparators(idlfile);
        fprintf(stderr, "\n\n%s\n\n", qPrintable(idlfile));
        const HRESULT res = dumpIdl(input, idlfile, version);

        switch (res) {
        case S_OK:
            break;
        case E_FAIL:
            fprintf(stderr, "IDL generation failed trying to run program %s!\n", qPrintable(input));
            return res;
        case -1:
            fprintf(stderr, "Couldn't open %s for writing!\n", qPrintable(idlfile));
            return res;
        case 1:
            fprintf(stderr, "Malformed appID value in %s!\n", qPrintable(input));
            return res;
        case 2:
            fprintf(stderr, "Malformed typeLibID value in %s!\n", qPrintable(input));
            return res;
        case 3:
            fprintf(stderr, "Class has no metaobject information (error in %s)!\n", qPrintable(input));
            return res;
        case 4:
            fprintf(stderr, "Malformed classID value in %s!\n", qPrintable(input));
            return res;
        case 5:
            fprintf(stderr, "Malformed interfaceID value in %s!\n", qPrintable(input));
            return res;
        case 6:
            fprintf(stderr, "Malformed eventsID value in %s!\n", qPrintable(input));
            return res;

        default:
            fprintf(stderr, "Unknown error writing IDL from %s\n", qPrintable(input));
            return 7;
        }
    }
    return 0;
}

QT_END_NAMESPACE

int main(int argc, char **argv)
{
    return QT_PREPEND_NAMESPACE(runIdc)(argc, argv);
}
