/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "utils.h"
#include "qmlutils.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCommandLineOption>
#include <QtCore/QOperatingSystemVersion>
#include <QtCore/QSharedPointer>
#include <QtCore/QVector>

#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h>
#else
#define IMAGE_FILE_MACHINE_ARM64 0xaa64
#endif

#include <algorithm>
#include <iostream>
#include <cstdio>

QT_BEGIN_NAMESPACE

enum QtModule
#if defined(Q_COMPILER_CLASS_ENUM) || defined(Q_CC_MSVC)
    : quint64
#endif
{
    QtBluetoothModule         = 0x0000000000000001,
    QtConcurrentModule        = 0x0000000000000002,
    QtCoreModule              = 0x0000000000000004,
    QtDeclarativeModule       = 0x0000000000000008,
    QtDesignerComponents      = 0x0000000000000010,
    QtDesignerModule          = 0x0000000000000020,
    QtGuiModule               = 0x0000000000000040,
    QtHelpModule              = 0x0000000000000080,
    QtMultimediaModule        = 0x0000000000000100,
    QtMultimediaWidgetsModule = 0x0000000000000200,
    QtMultimediaQuickModule   = 0x0000000000000400,
    QtNetworkModule           = 0x0000000000000800,
    QtNfcModule               = 0x0000000000001000,
    QtOpenGLModule            = 0x0000000000002000,
    QtPositioningModule       = 0x0000000000004000,
    QtPrintSupportModule      = 0x0000000000008000,
    QtQmlModule               = 0x0000000000010000,
    QtQuickModule             = 0x0000000000020000,
    QtQuickParticlesModule    = 0x0000000000040000,
    QtScriptModule            = 0x0000000000080000,
    QtScriptToolsModule       = 0x0000000000100000,
    QtSensorsModule           = 0x0000000000200000,
    QtSerialPortModule        = 0x0000000000400000,
    QtSqlModule               = 0x0000000000800000,
    QtSvgModule               = 0x0000000001000000,
    QtTestModule              = 0x0000000002000000,
    QtWidgetsModule           = 0x0000000004000000,
    QtWinExtrasModule         = 0x0000000008000000,
    QtXmlModule               = 0x0000000010000000,
    QtXmlPatternsModule       = 0x0000000020000000,
    QtWebKitModule            = 0x0000000040000000,
    QtWebKitWidgetsModule     = 0x0000000080000000,
    QtQuickWidgetsModule      = 0x0000000100000000,
    QtWebSocketsModule        = 0x0000000200000000,
    QtEnginioModule           = 0x0000000400000000,
    QtWebEngineCoreModule     = 0x0000000800000000,
    QtWebEngineModule         = 0x0000001000000000,
    QtWebEngineWidgetsModule  = 0x0000002000000000,
    QtQmlToolingModule        = 0x0000004000000000,
    Qt3DCoreModule            = 0x0000008000000000,
    Qt3DRendererModule        = 0x0000010000000000,
    Qt3DQuickModule           = 0x0000020000000000,
    Qt3DQuickRendererModule   = 0x0000040000000000,
    Qt3DInputModule           = 0x0000080000000000,
    QtLocationModule          = 0x0000100000000000,
    QtWebChannelModule        = 0x0000200000000000,
    QtTextToSpeechModule      = 0x0000400000000000,
    QtSerialBusModule         = 0x0000800000000000,
    QtGamePadModule           = 0x0001000000000000,
    Qt3DAnimationModule       = 0x0002000000000000,
    QtWebViewModule           = 0x0004000000000000,
    Qt3DExtrasModule          = 0x0008000000000000
};

struct QtModuleEntry {
    quint64 module;
    const char *option;
    const char *libraryName;
    const char *translation;
};

static QtModuleEntry qtModuleEntries[] = {
    { QtBluetoothModule, "bluetooth", "Qt5Bluetooth", nullptr },
    { QtConcurrentModule, "concurrent", "Qt5Concurrent", "qtbase" },
    { QtCoreModule, "core", "Qt5Core", "qtbase" },
    { QtDeclarativeModule, "declarative", "Qt5Declarative", "qtquick1" },
    { QtDesignerModule, "designer", "Qt5Designer", nullptr },
    { QtDesignerComponents, "designercomponents", "Qt5DesignerComponents", nullptr },
    { QtEnginioModule, "enginio", "Enginio", nullptr },
    { QtGamePadModule, "gamepad", "Qt5Gamepad", nullptr },
    { QtGuiModule, "gui", "Qt5Gui", "qtbase" },
    { QtHelpModule, "qthelp", "Qt5Help", "qt_help" },
    { QtMultimediaModule, "multimedia", "Qt5Multimedia", "qtmultimedia" },
    { QtMultimediaWidgetsModule, "multimediawidgets", "Qt5MultimediaWidgets", "qtmultimedia" },
    { QtMultimediaQuickModule, "multimediaquick", "Qt5MultimediaQuick_p", "qtmultimedia" },
    { QtNetworkModule, "network", "Qt5Network", "qtbase" },
    { QtNfcModule, "nfc", "Qt5Nfc", nullptr },
    { QtOpenGLModule, "opengl", "Qt5OpenGL", nullptr },
    { QtPositioningModule, "positioning", "Qt5Positioning", nullptr },
    { QtPrintSupportModule, "printsupport", "Qt5PrintSupport", nullptr },
    { QtQmlModule, "qml", "Qt5Qml", "qtdeclarative" },
    { QtQmlToolingModule, "qmltooling", "qmltooling", nullptr },
    { QtQuickModule, "quick", "Qt5Quick", "qtdeclarative" },
    { QtQuickParticlesModule, "quickparticles", "Qt5QuickParticles", nullptr },
    { QtQuickWidgetsModule, "quickwidgets", "Qt5QuickWidgets", nullptr },
    { QtScriptModule, "script", "Qt5Script", "qtscript" },
    { QtScriptToolsModule, "scripttools", "Qt5ScriptTools", "qtscript" },
    { QtSensorsModule, "sensors", "Qt5Sensors", nullptr },
    { QtSerialPortModule, "serialport", "Qt5SerialPort", "qtserialport" },
    { QtSqlModule, "sql", "Qt5Sql", "qtbase" },
    { QtSvgModule, "svg", "Qt5Svg", nullptr },
    { QtTestModule, "test", "Qt5Test", "qtbase" },
    { QtWebKitModule, "webkit", "Qt5WebKit", nullptr },
    { QtWebKitWidgetsModule, "webkitwidgets", "Qt5WebKitWidgets", nullptr },
    { QtWebSocketsModule, "websockets", "Qt5WebSockets", nullptr },
    { QtWidgetsModule, "widgets", "Qt5Widgets", "qtbase" },
    { QtWinExtrasModule, "winextras", "Qt5WinExtras", nullptr },
    { QtXmlModule, "xml", "Qt5Xml", "qtbase" },
    { QtXmlPatternsModule, "xmlpatterns", "Qt5XmlPatterns", "qtxmlpatterns" },
    { QtWebEngineCoreModule, "webenginecore", "Qt5WebEngineCore", nullptr },
    { QtWebEngineModule, "webengine", "Qt5WebEngine", "qtwebengine" },
    { QtWebEngineWidgetsModule, "webenginewidgets", "Qt5WebEngineWidgets", nullptr },
    { Qt3DCoreModule, "3dcore", "Qt53DCore", nullptr },
    { Qt3DRendererModule, "3drenderer", "Qt53DRender", nullptr },
    { Qt3DQuickModule, "3dquick", "Qt53DQuick", nullptr },
    { Qt3DQuickRendererModule, "3dquickrenderer", "Qt53DQuickRender", nullptr },
    { Qt3DInputModule, "3dinput", "Qt53DInput", nullptr },
    { Qt3DAnimationModule, "3danimation", "Qt53DAnimation", nullptr },
    { Qt3DExtrasModule, "3dextras", "Qt53DExtras", nullptr },
    { QtLocationModule, "geoservices", "Qt5Location", nullptr },
    { QtWebChannelModule, "webchannel", "Qt5WebChannel", nullptr },
    { QtTextToSpeechModule, "texttospeech", "Qt5TextToSpeech", nullptr },
    { QtSerialBusModule, "serialbus", "Qt5SerialBus", nullptr },
    { QtWebViewModule, "webview", "Qt5WebView", nullptr }
};

static const char webKitProcessC[] = "QtWebProcess";
static const char webEngineProcessC[] = "QtWebEngineProcess";

static inline QString webProcessBinary(const char *binaryName, Platform p)
{
    const QString webProcess = QLatin1String(binaryName);
    return (p & WindowsBased) ? webProcess + QStringLiteral(".exe") : webProcess;
}

static QByteArray formatQtModules(quint64 mask, bool option = false)
{
    QByteArray result;
    for (const auto &qtModule : qtModuleEntries) {
        if (mask & qtModule.module) {
            if (!result.isEmpty())
                result.append(' ');
            result.append(option ? qtModule.option : qtModule.libraryName);
        }
    }
    return result;
}

static Platform platformFromMkSpec(const QString &xSpec)
{
    if (xSpec == QLatin1String("linux-g++"))
        return Unix;
    if (xSpec.startsWith(QLatin1String("win32-")))
        return xSpec.contains(QLatin1String("g++")) ? WindowsDesktopMinGW : WindowsDesktop;
    if (xSpec.startsWith(QLatin1String("winrt-x")))
        return WinRtIntel;
    if (xSpec.startsWith(QLatin1String("winrt-arm")))
        return WinRtArm;
    if (xSpec.startsWith(QLatin1String("wince"))) {
        if (xSpec.contains(QLatin1String("-x86-")))
            return WinCEIntel;
        if (xSpec.contains(QLatin1String("-arm")))
            return WinCEArm;
    }
    return UnknownPlatform;
}

// Helpers for exclusive options, "-foo", "--no-foo"
enum ExlusiveOptionValue {
    OptionAuto,
    OptionEnabled,
    OptionDisabled
};

static ExlusiveOptionValue parseExclusiveOptions(const QCommandLineParser *parser,
                                                 const QCommandLineOption &enableOption,
                                                 const QCommandLineOption &disableOption)
{
    const bool enabled = parser->isSet(enableOption);
    const bool disabled = parser->isSet(disableOption);
    if (enabled) {
        if (disabled) {
            std::wcerr << "Warning: both -" << enableOption.names().first()
                << " and -" << disableOption.names().first() << " were specified, defaulting to -"
                << enableOption.names().first() << ".\n";
        }
        return OptionEnabled;
    }
    return disabled ? OptionDisabled : OptionAuto;
}

static ExlusiveOptionValue optWebKit2 = OptionAuto;

struct Options {
    enum DebugDetection {
        DebugDetectionAuto,
        DebugDetectionForceDebug,
        DebugDetectionForceRelease
    };

    enum AngleDetection {
        AngleDetectionAuto,
        AngleDetectionForceOn,
        AngleDetectionForceOff
    };

    bool plugins = true;
    bool libraries = true;
    bool quickImports = true;
    bool translations = true;
    bool systemD3dCompiler = true;
    bool compilerRunTime = false;
    AngleDetection angleDetection = AngleDetectionAuto;
    bool softwareRasterizer = true;
    Platform platform = WindowsDesktop;
    quint64 additionalLibraries = 0;
    quint64 disabledLibraries = 0;
    unsigned updateFileFlags = 0;
    QStringList qmlDirectories; // Project's QML files.
    QString directory;
    QString translationsDirectory; // Translations target directory
    QString libraryDirectory;
    QString pluginDirectory;
    QStringList binaries;
    JsonOutput *json = nullptr;
    ListOption list = ListNone;
    DebugDetection debugDetection = DebugDetectionAuto;
    bool deployPdb = false;
    bool dryRun = false;
    bool patchQt = true;

    inline bool isWinRt() const {
        return platform == WinRtArm || platform == WinRtIntel;
    }
};

// Return binary from folder
static inline QString findBinary(const QString &directory, Platform platform)
{
    const QStringList nameFilters = (platform & WindowsBased) ?
        QStringList(QStringLiteral("*.exe")) : QStringList();
    const QFileInfoList &binaries =
        QDir(QDir::cleanPath(directory)).entryInfoList(nameFilters, QDir::Files | QDir::Executable);
    for (const QFileInfo &binaryFi : binaries) {
        const QString binary = binaryFi.fileName();
        if (!binary.contains(QLatin1String(webKitProcessC), Qt::CaseInsensitive)
            && !binary.contains(QLatin1String(webEngineProcessC), Qt::CaseInsensitive)) {
            return binaryFi.absoluteFilePath();
        }
    }
    return QString();
}

static QString msgFileDoesNotExist(const QString & file)
{
    return QLatin1Char('"') + QDir::toNativeSeparators(file)
        + QStringLiteral("\" does not exist.");
}

enum CommandLineParseFlag {
    CommandLineParseError = 0x1,
    CommandLineParseHelpRequested = 0x2
};

static inline int parseArguments(const QStringList &arguments, QCommandLineParser *parser,
                                 Options *options, QString *errorMessage)
{
    using CommandLineOptionPtr = QSharedPointer<QCommandLineOption>;
    using OptionPtrVector = QVector<CommandLineOptionPtr>;

    parser->setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser->setApplicationDescription(QStringLiteral("Qt Deploy Tool ") + QLatin1String(QT_VERSION_STR)
        + QLatin1String("\n\nThe simplest way to use windeployqt is to add the bin directory of your Qt\n"
        "installation (e.g. <QT_DIR\\bin>) to the PATH variable and then run:\n  windeployqt <path-to-app-binary>\n"
        "If ICU, ANGLE, etc. are not in the bin directory, they need to be in the PATH\nvariable. "
        "If your application uses Qt Quick, run:\n  windeployqt --qmldir <path-to-app-qml-files> <path-to-app-binary>"));
    const QCommandLineOption helpOption = parser->addHelpOption();
    parser->addVersionOption();

    QCommandLineOption dirOption(QStringLiteral("dir"),
                                 QStringLiteral("Use directory instead of binary directory."),
                                 QStringLiteral("directory"));
    parser->addOption(dirOption);

    QCommandLineOption libDirOption(QStringLiteral("libdir"),
                                    QStringLiteral("Copy libraries to path."),
                                    QStringLiteral("path"));
    parser->addOption(libDirOption);

    QCommandLineOption pluginDirOption(QStringLiteral("plugindir"),
                                       QStringLiteral("Copy plugins to path."),
                                       QStringLiteral("path"));
    parser->addOption(pluginDirOption);

    QCommandLineOption debugOption(QStringLiteral("debug"),
                                   QStringLiteral("Assume debug binaries."));
    parser->addOption(debugOption);
    QCommandLineOption releaseOption(QStringLiteral("release"),
                                   QStringLiteral("Assume release binaries."));
    parser->addOption(releaseOption);
    QCommandLineOption releaseWithDebugInfoOption(QStringLiteral("release-with-debug-info"),
                                                  QStringLiteral("Assume release binaries with debug information."));
    releaseWithDebugInfoOption.setFlags(QCommandLineOption::HiddenFromHelp); // Deprecated by improved debug detection.
    parser->addOption(releaseWithDebugInfoOption);

    QCommandLineOption deployPdbOption(QStringLiteral("pdb"),
                                       QStringLiteral("Deploy .pdb files (MSVC)."));
    parser->addOption(deployPdbOption);

    QCommandLineOption forceOption(QStringLiteral("force"),
                                    QStringLiteral("Force updating files."));
    parser->addOption(forceOption);

    QCommandLineOption dryRunOption(QStringLiteral("dry-run"),
                                    QStringLiteral("Simulation mode. Behave normally, but do not copy/update any files."));
    parser->addOption(dryRunOption);

    QCommandLineOption noPatchQtOption(QStringLiteral("no-patchqt"),
                                       QStringLiteral("Do not patch the Qt5Core library."));
    parser->addOption(noPatchQtOption);

    QCommandLineOption noPluginsOption(QStringLiteral("no-plugins"),
                                       QStringLiteral("Skip plugin deployment."));
    parser->addOption(noPluginsOption);

    QCommandLineOption noLibraryOption(QStringLiteral("no-libraries"),
                                       QStringLiteral("Skip library deployment."));
    parser->addOption(noLibraryOption);

    QCommandLineOption qmlDirOption(QStringLiteral("qmldir"),
                                    QStringLiteral("Scan for QML-imports starting from directory."),
                                    QStringLiteral("directory"));
    parser->addOption(qmlDirOption);

    QCommandLineOption noQuickImportOption(QStringLiteral("no-quick-import"),
                                           QStringLiteral("Skip deployment of Qt Quick imports."));
    parser->addOption(noQuickImportOption);

    QCommandLineOption noTranslationOption(QStringLiteral("no-translations"),
                                           QStringLiteral("Skip deployment of translations."));
    parser->addOption(noTranslationOption);

    QCommandLineOption noSystemD3DCompilerOption(QStringLiteral("no-system-d3d-compiler"),
                                                 QStringLiteral("Skip deployment of the system D3D compiler."));
    parser->addOption(noSystemD3DCompilerOption);


    QCommandLineOption compilerRunTimeOption(QStringLiteral("compiler-runtime"),
                                             QStringLiteral("Deploy compiler runtime (Desktop only)."));
    parser->addOption(compilerRunTimeOption);

    QCommandLineOption noCompilerRunTimeOption(QStringLiteral("no-compiler-runtime"),
                                             QStringLiteral("Do not deploy compiler runtime (Desktop only)."));
    parser->addOption(noCompilerRunTimeOption);

    QCommandLineOption webKitOption(QStringLiteral("webkit2"),
                                    QStringLiteral("Deployment of WebKit2 (web process)."));
    parser->addOption(webKitOption);

    QCommandLineOption noWebKitOption(QStringLiteral("no-webkit2"),
                                      QStringLiteral("Skip deployment of WebKit2."));
    parser->addOption(noWebKitOption);

    QCommandLineOption jsonOption(QStringLiteral("json"),
                                  QStringLiteral("Print to stdout in JSON format."));
    parser->addOption(jsonOption);

    QCommandLineOption angleOption(QStringLiteral("angle"),
                                   QStringLiteral("Force deployment of ANGLE."));
    parser->addOption(angleOption);

    QCommandLineOption noAngleOption(QStringLiteral("no-angle"),
                                     QStringLiteral("Disable deployment of ANGLE."));
    parser->addOption(noAngleOption);

    QCommandLineOption suppressSoftwareRasterizerOption(QStringLiteral("no-opengl-sw"),
                                                        QStringLiteral("Do not deploy the software rasterizer library."));
    parser->addOption(suppressSoftwareRasterizerOption);

    QCommandLineOption listOption(QStringLiteral("list"),
                                  QLatin1String("Print only the names of the files copied.\n"
                                                "Available options:\n"
                                                "  source:   absolute path of the source files\n"
                                                "  target:   absolute path of the target files\n"
                                                "  relative: paths of the target files, relative\n"
                                                "            to the target directory\n"
                                                "  mapping:  outputs the source and the relative\n"
                                                "            target, suitable for use within an\n"
                                                "            Appx mapping file"),
                                  QStringLiteral("option"));
    parser->addOption(listOption);

    QCommandLineOption verboseOption(QStringLiteral("verbose"),
                                     QStringLiteral("Verbose level (0-2)."),
                                     QStringLiteral("level"));
    parser->addOption(verboseOption);

    parser->addPositionalArgument(QStringLiteral("[files]"),
                                  QStringLiteral("Binaries or directory containing the binary."));

    OptionPtrVector enabledModuleOptions;
    OptionPtrVector disabledModuleOptions;
    const int qtModulesCount = int(sizeof(qtModuleEntries) / sizeof(QtModuleEntry));
    enabledModuleOptions.reserve(qtModulesCount);
    disabledModuleOptions.reserve(qtModulesCount);
    for (int i = 0; i < qtModulesCount; ++i) {
        const QString option = QLatin1String(qtModuleEntries[i].option);
        const QString name = QLatin1String(qtModuleEntries[i].libraryName);
        const QString enabledDescription = QStringLiteral("Add ") + name + QStringLiteral(" module.");
        CommandLineOptionPtr enabledOption(new QCommandLineOption(option, enabledDescription));
        parser->addOption(*enabledOption.data());
        enabledModuleOptions.append(enabledOption);
        const QString disabledDescription = QStringLiteral("Remove ") + name + QStringLiteral(" module.");
        CommandLineOptionPtr disabledOption(new QCommandLineOption(QStringLiteral("no-") + option,
                                                                   disabledDescription));
        disabledModuleOptions.append(disabledOption);
        parser->addOption(*disabledOption.data());
    }

    const bool success = parser->parse(arguments);
    if (parser->isSet(helpOption))
        return CommandLineParseHelpRequested;
    if (!success) {
        *errorMessage = parser->errorText();
        return CommandLineParseError;
    }

    options->libraryDirectory = parser->value(libDirOption);
    options->pluginDirectory = parser->value(pluginDirOption);
    options->plugins = !parser->isSet(noPluginsOption);
    options->libraries = !parser->isSet(noLibraryOption);
    options->translations = !parser->isSet(noTranslationOption);
    options->systemD3dCompiler = !parser->isSet(noSystemD3DCompilerOption);
    options->quickImports = !parser->isSet(noQuickImportOption);

    if (parser->isSet(compilerRunTimeOption))
        options->compilerRunTime = true;
    else if (parser->isSet(noCompilerRunTimeOption))
        options->compilerRunTime = false;

    if (options->compilerRunTime && options->platform != WindowsDesktopMinGW && options->platform != WindowsDesktop) {
        *errorMessage = QStringLiteral("Deployment of the compiler runtime is implemented for Desktop only.");
        return CommandLineParseError;
    }

    if (parser->isSet(releaseWithDebugInfoOption))
        std::wcerr << "Warning: " << releaseWithDebugInfoOption.names().first() << " is obsolete.";

    switch (parseExclusiveOptions(parser, debugOption, releaseOption)) {
    case OptionAuto:
        break;
    case OptionEnabled:
        options->debugDetection = Options::DebugDetectionForceDebug;
        break;
    case OptionDisabled:
        options->debugDetection = Options::DebugDetectionForceRelease;
        break;
    }

    if (parser->isSet(deployPdbOption)) {
        if ((options->platform & WindowsBased) && !(options->platform & MinGW))
            options->deployPdb = true;
        else
            std::wcerr << "Warning: --" << deployPdbOption.names().first() << " is not supported on this platform.\n";
    }

    switch (parseExclusiveOptions(parser, angleOption, noAngleOption)) {
    case OptionAuto:
        break;
    case OptionEnabled:
        options->angleDetection = Options::AngleDetectionForceOn;
        break;
    case OptionDisabled:
        options->angleDetection = Options::AngleDetectionForceOff;
        break;
    }

    if (parser->isSet(suppressSoftwareRasterizerOption))
        options->softwareRasterizer = false;

    optWebKit2 = parseExclusiveOptions(parser, webKitOption, noWebKitOption);

    if (parser->isSet(forceOption))
        options->updateFileFlags |= ForceUpdateFile;
    if (parser->isSet(dryRunOption)) {
        options->dryRun = true;
        options->updateFileFlags |= SkipUpdateFile;
    }

    options->patchQt = !parser->isSet(noPatchQtOption);

    for (int i = 0; i < qtModulesCount; ++i) {
        if (parser->isSet(*enabledModuleOptions.at(i)))
            options->additionalLibraries |= qtModuleEntries[i].module;
        if (parser->isSet(*disabledModuleOptions.at(i)))
            options->disabledLibraries |= qtModuleEntries[i].module;
    }

    // Add some dependencies
    if (options->additionalLibraries & QtQuickModule)
        options->additionalLibraries |= QtQmlModule;
    if (options->additionalLibraries & QtDesignerComponents)
        options->additionalLibraries |= QtDesignerModule;

    if (parser->isSet(listOption)) {
        const QString value = parser->value(listOption);
        if (value == QStringLiteral("source")) {
            options->list = ListSource;
        } else if (value == QStringLiteral("target")) {
            options->list = ListTarget;
        } else if (value == QStringLiteral("relative")) {
            options->list = ListRelative;
        } else if (value == QStringLiteral("mapping")) {
            options->list = ListMapping;
        } else {
            *errorMessage = QStringLiteral("Please specify a valid option for -list (source, target, relative, mapping).");
            return CommandLineParseError;
        }
    }

    if (parser->isSet(jsonOption) || options->list) {
        optVerboseLevel = 0;
        options->json = new JsonOutput;
    } else {
        if (parser->isSet(verboseOption)) {
            bool ok;
            const QString value = parser->value(verboseOption);
            optVerboseLevel = value.toInt(&ok);
            if (!ok || optVerboseLevel < 0) {
                *errorMessage = QStringLiteral("Invalid value \"%1\" passed for verbose level.").arg(value);
                return CommandLineParseError;
            }
        }
    }

    const QStringList posArgs = parser->positionalArguments();
    if (posArgs.isEmpty()) {
        *errorMessage = QStringLiteral("Please specify the binary or folder.");
        return CommandLineParseError | CommandLineParseHelpRequested;
    }

    if (parser->isSet(dirOption))
        options->directory = parser->value(dirOption);

    if (parser->isSet(qmlDirOption))
        options->qmlDirectories = parser->values(qmlDirOption);

    const QString &file = posArgs.front();
    const QFileInfo fi(QDir::cleanPath(file));
    if (!fi.exists()) {
        *errorMessage = msgFileDoesNotExist(file);
        return CommandLineParseError;
    }

    if (!options->directory.isEmpty() && !fi.isFile()) { // -dir was specified - expecting file.
        *errorMessage = QLatin1Char('"') + file + QStringLiteral("\" is not an executable file.");
        return CommandLineParseError;
    }

    if (fi.isFile()) {
        options->binaries.append(fi.absoluteFilePath());
        if (options->directory.isEmpty())
            options->directory = fi.absolutePath();
    } else {
        const QString binary = findBinary(fi.absoluteFilePath(), options->platform);
        if (binary.isEmpty()) {
            *errorMessage = QStringLiteral("Unable to find binary in \"") + file + QLatin1Char('"');
            return CommandLineParseError;
        }
        options->directory = fi.absoluteFilePath();
        options->binaries.append(binary);
    } // directory.

    // Remaining files or plugin directories
    for (int i = 1; i < posArgs.size(); ++i) {
        const QFileInfo fi(QDir::cleanPath(posArgs.at(i)));
        const QString path = fi.absoluteFilePath();
        if (!fi.exists()) {
            *errorMessage = msgFileDoesNotExist(path);
            return CommandLineParseError;
        }
        if (fi.isDir()) {
            const QStringList libraries =
                findSharedLibraries(QDir(path), options->platform, MatchDebugOrRelease, QString());
            for (const QString &library : libraries)
                options->binaries.append(path + QLatin1Char('/') + library);
        } else {
            options->binaries.append(path);
        }
    }
    options->translationsDirectory = options->directory + QLatin1String("/translations");
    return 0;
}

// Simple line wrapping at 80 character boundaries.
static inline QString lineBreak(QString s)
{
    for (int i = 80; i < s.size(); i += 80) {
        const int lastBlank = s.lastIndexOf(QLatin1Char(' '), i);
        if (lastBlank >= 0) {
            s[lastBlank] = QLatin1Char('\n');
            i = lastBlank + 1;
        }
    }
    return s;
}

static inline QString helpText(const QCommandLineParser &p)
{
    QString result = p.helpText();
    // Replace the default-generated text which is too long by a short summary
    // explaining how to enable single libraries.
    const int moduleStart = result.indexOf(QLatin1String("\n  --bluetooth"));
    const int argumentsStart = result.lastIndexOf(QLatin1String("\nArguments:"));
    if (moduleStart >= argumentsStart)
        return result;
    QString moduleHelp = QLatin1String(
        "\n\nQt libraries can be added by passing their name (-xml) or removed by passing\n"
        "the name prepended by --no- (--no-xml). Available libraries:\n");
    moduleHelp += lineBreak(QString::fromLatin1(formatQtModules(0xFFFFFFFFFFFFFFFFull, true)));
    moduleHelp += QLatin1Char('\n');
    result.replace(moduleStart, argumentsStart - moduleStart, moduleHelp);
    return result;
}

static inline bool isQtModule(const QString &libName)
{
    // Match Standard modules named Qt5XX.dll
    if (libName.size() < 3 || !libName.startsWith(QLatin1String("Qt"), Qt::CaseInsensitive))
        return false;
    const QChar version = libName.at(2);
    return version.isDigit() && (version.toLatin1() - '0') == QT_VERSION_MAJOR;
}

// Helper for recursively finding all dependent Qt libraries.
static bool findDependentQtLibraries(const QString &qtBinDir, const QString &binary, Platform platform,
                                     QString *errorMessage, QStringList *result,
                                     unsigned *wordSize = nullptr, bool *isDebug = nullptr,
                                     unsigned short *machineArch = nullptr,
                                     int *directDependencyCount = nullptr, int recursionDepth = 0)
{
    QStringList dependentLibs;
    if (directDependencyCount)
        *directDependencyCount = 0;
    if (!readExecutable(binary, platform, errorMessage, &dependentLibs, wordSize, isDebug, machineArch)) {
        errorMessage->prepend(QLatin1String("Unable to find dependent libraries of ") +
                              QDir::toNativeSeparators(binary) + QLatin1String(" :"));
        return false;
    }
    // Filter out the Qt libraries. Note that depends.exe finds libs from optDirectory if we
    // are run the 2nd time (updating). We want to check against the Qt bin dir libraries
    const int start = result->size();
    for (const QString &lib : qAsConst(dependentLibs)) {
        if (isQtModule(lib)) {
            const QString path = normalizeFileName(qtBinDir + QLatin1Char('/') + QFileInfo(lib).fileName());
            if (!result->contains(path))
                result->append(path);
        }
    }
    const int end = result->size();
    if (directDependencyCount)
        *directDependencyCount = end - start;
    // Recurse
    for (int i = start; i < end; ++i)
        if (!findDependentQtLibraries(qtBinDir, result->at(i), platform, errorMessage, result,
                                      nullptr, nullptr, nullptr, nullptr, recursionDepth + 1))
            return false;
    return true;
}

// Base class to filter debug/release Windows DLLs for functions to be passed to updateFile().
// Tries to pre-filter by namefilter and does check via PE.
class DllDirectoryFileEntryFunction {
public:
    explicit DllDirectoryFileEntryFunction(Platform platform,
                                           DebugMatchMode debugMatchMode, const QString &prefix = QString()) :
        m_platform(platform), m_debugMatchMode(debugMatchMode), m_prefix(prefix) {}

    QStringList operator()(const QDir &dir) const
        { return findSharedLibraries(dir, m_platform, m_debugMatchMode, m_prefix); }

private:
    const Platform m_platform;
    const DebugMatchMode m_debugMatchMode;
    const QString m_prefix;
};

static QString pdbFileName(QString libraryFileName)
{
    const int lastDot = libraryFileName.lastIndexOf(QLatin1Char('.')) + 1;
    if (lastDot <= 0)
        return QString();
    libraryFileName.replace(lastDot, libraryFileName.size() - lastDot, QLatin1String("pdb"));
    return libraryFileName;
}
static inline QStringList qmlCacheFileFilters()
{
    return QStringList() << QStringLiteral("*.jsc") << QStringLiteral("*.qmlc");
}

// File entry filter function for updateFile() that returns a list of files for
// QML import trees: DLLs (matching debgug) and .qml/,js, etc.
class QmlDirectoryFileEntryFunction {
public:
    enum Flags {
        DeployPdb = 0x1,
        SkipSources = 0x2
    };

    explicit QmlDirectoryFileEntryFunction(Platform platform, DebugMatchMode debugMatchMode, unsigned flags)
        : m_flags(flags), m_qmlNameFilter(QmlDirectoryFileEntryFunction::qmlNameFilters(flags))
        , m_dllFilter(platform, debugMatchMode)
    {}

    QStringList operator()(const QDir &dir) const
    {
        QStringList result;
        const QStringList &libraries = m_dllFilter(dir);
        for (const QString &library : libraries) {
            result.append(library);
            if (m_flags & DeployPdb) {
                const QString pdb = pdbFileName(library);
                if (QFileInfo(dir.absoluteFilePath(pdb)).isFile())
                    result.append(pdb);
            }
        }
        result.append(m_qmlNameFilter(dir));
        return result;
    }

private:
    static inline QStringList qmlNameFilters(unsigned flags)
    {
        QStringList result;
        result << QStringLiteral("qmldir") << QStringLiteral("*.qmltypes")
           << QStringLiteral("*.frag") << QStringLiteral("*.vert") // Shaders
           << QStringLiteral("*.ttf");
        if (!(flags & SkipSources)) {
            result << QStringLiteral("*.js") << QStringLiteral("*.qml") << QStringLiteral("*.png");
            result.append(qmlCacheFileFilters());
        }
        return result;
    }

    const unsigned m_flags;
    NameFilterFileEntryFunction m_qmlNameFilter;
    DllDirectoryFileEntryFunction m_dllFilter;
};

struct PluginModuleMapping
{
    const char *directoryName;
    quint64 module;
};

static const PluginModuleMapping pluginModuleMappings[] =
{
    {"qml1tooling", QtDeclarativeModule},
    {"gamepads", QtGamePadModule},
    {"accessible", QtGuiModule},
    {"iconengines", QtGuiModule},
    {"imageformats", QtGuiModule},
    {"platforms", QtGuiModule},
    {"platforminputcontexts", QtGuiModule},
    {"virtualkeyboard", QtGuiModule},
    {"geoservices", QtLocationModule},
    {"audio", QtMultimediaModule},
    {"mediaservice", QtMultimediaModule},
    {"playlistformats", QtMultimediaModule},
    {"bearer", QtNetworkModule},
    {"position", QtPositioningModule},
    {"printsupport", QtPrintSupportModule},
    {"scenegraph", QtQuickModule},
    {"qmltooling", QtQuickModule | QtQmlToolingModule},
    {"sensors", QtSensorsModule},
    {"sensorgestures", QtSensorsModule},
    {"canbus", QtSerialBusModule},
    {"sqldrivers", QtSqlModule},
    {"texttospeech", QtTextToSpeechModule},
    {"qtwebengine", QtWebEngineModule | QtWebEngineCoreModule | QtWebEngineWidgetsModule},
    {"styles", QtWidgetsModule},
    {"sceneparsers", Qt3DRendererModule},
    {"renderplugins", Qt3DRendererModule},
    {"geometryloaders", Qt3DRendererModule},
    {"webview", QtWebViewModule}
};

static inline quint64 qtModuleForPlugin(const QString &subDirName)
{
    const auto end = std::end(pluginModuleMappings);
    const auto result =
        std::find_if(std::begin(pluginModuleMappings), end,
                     [&subDirName] (const PluginModuleMapping &m) { return subDirName == QLatin1String(m.directoryName); });
    return result != end ? result->module : 0; // "designer"
}

static quint64 qtModule(QString module, const QString &infix)
{
    // Match needle 'path/Qt5Core<infix><d>.dll' or 'path/libQt5Core<infix>.so.5.0'
    const int lastSlashPos = module.lastIndexOf(QLatin1Char('/'));
    if (lastSlashPos > 0)
        module.remove(0, lastSlashPos + 1);
    if (module.startsWith(QLatin1String("lib")))
        module.remove(0, 3);
    int endPos = infix.isEmpty() ? -1 : module.lastIndexOf(infix);
    if (endPos == -1)
        endPos = module.indexOf(QLatin1Char('.')); // strip suffixes '.so.5.0'.
    if (endPos > 0)
        module.truncate(endPos);
    // That should leave us with 'Qt5Core<d>'.
    for (const auto &qtModule : qtModuleEntries) {
        const QLatin1String libraryName(qtModule.libraryName);
        if (module == libraryName
            || (module.size() == libraryName.size() + 1 && module.startsWith(libraryName))) {
            return qtModule.module;
        }
    }
    return 0;
}

QStringList findQtPlugins(quint64 *usedQtModules, quint64 disabledQtModules,
                          const QString &qtPluginsDirName, const QString &libraryLocation,
                          const QString &infix,
                          DebugMatchMode debugMatchModeIn, Platform platform, QString *platformPlugin)
{
    QString errorMessage;
    if (qtPluginsDirName.isEmpty())
        return QStringList();
    QDir pluginsDir(qtPluginsDirName);
    QStringList result;
    const QFileInfoList &pluginDirs = pluginsDir.entryInfoList(QStringList(QLatin1String("*")), QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &subDirFi : pluginDirs) {
        const QString subDirName = subDirFi.fileName();
        const quint64 module = qtModuleForPlugin(subDirName);
        if (module & *usedQtModules) {
            const DebugMatchMode debugMatchMode = (module & QtWebEngineCoreModule)
                ? MatchDebugOrRelease // QTBUG-44331: Debug detection does not work for webengine, deploy all.
                : debugMatchModeIn;
            QDir subDir(subDirFi.absoluteFilePath());
            // Filter out disabled plugins
            if (disabledQtModules & QtQmlToolingModule && subDirName == QLatin1String("qmltooling"))
                continue;
            // Filter for platform or any.
            QString filter;
            const bool isPlatformPlugin = subDirName == QLatin1String("platforms");
            if (isPlatformPlugin) {
                switch (platform) {
                case WindowsDesktop:
                case WindowsDesktopMinGW:
                case WinCEIntel:
                case WinCEArm:
                    filter = QStringLiteral("qwindows");
                    break;
                case WinRtIntel:
                case WinRtArm:
                    filter = QStringLiteral("qwinrt");
                    break;
                case Unix:
                    filter = QStringLiteral("libqxcb");
                    break;
                case UnknownPlatform:
                    break;
                }
            } else {
                filter  = QLatin1String("*");
            }
            const QStringList plugins = findSharedLibraries(subDir, platform, debugMatchMode, filter);
            for (const QString &plugin : plugins) {
                const QString pluginPath = subDir.absoluteFilePath(plugin);
                if (isPlatformPlugin)
                    *platformPlugin = pluginPath;
                QStringList dependentQtLibs;
                quint64 neededModules = 0;
                if (findDependentQtLibraries(libraryLocation, pluginPath, platform, &errorMessage, &dependentQtLibs)) {
                    for (int d = 0; d < dependentQtLibs.size(); ++ d)
                        neededModules |= qtModule(dependentQtLibs.at(d), infix);
                } else {
                    std::wcerr << "Warning: Cannot determine dependencies of "
                        << QDir::toNativeSeparators(pluginPath) << ": " << errorMessage << '\n';
                }
                if (const quint64 missingModules = neededModules & disabledQtModules) {
                    if (optVerboseLevel) {
                        std::wcout << "Skipping plugin " << plugin
                            << " due to disabled dependencies ("
                            << formatQtModules(missingModules).constData() << ").\n";
                    }
                } else {
                    if (const quint64 missingModules = (neededModules & ~*usedQtModules)) {
                        *usedQtModules |= missingModules;
                        if (optVerboseLevel)
                            std::wcout << "Adding " << formatQtModules(missingModules).constData() << " for " << plugin << '\n';
                    }
                    result.append(pluginPath);
                }
            } // for filter
        } // type matches
    } // for plugin folder
    return result;
}

static QStringList translationNameFilters(quint64 modules, const QString &prefix)
{
    QStringList result;
    for (const auto &qtModule : qtModuleEntries) {
        if ((qtModule.module & modules) && qtModule.translation) {
            const QString name = QLatin1String(qtModule.translation) +
                                 QLatin1Char('_') +  prefix + QStringLiteral(".qm");
            if (!result.contains(name))
                result.push_back(name);
        }
    }
    return result;
}

static bool deployTranslations(const QString &sourcePath, quint64 usedQtModules,
                               const QString &target, const Options &options,
                               QString *errorMessage)
{
    // Find available languages prefixes by checking on qtbase.
    QStringList prefixes;
    QDir sourceDir(sourcePath);
    const QStringList qmFilter = QStringList(QStringLiteral("qtbase_*.qm"));
    const QFileInfoList &qmFiles = sourceDir.entryInfoList(qmFilter);
    for (const QFileInfo &qmFi : qmFiles) {
        QString qmFile = qmFi.baseName();
        qmFile.remove(0, 7);
        prefixes.push_back(qmFile);
    }
    if (prefixes.isEmpty()) {
        std::wcerr << "Warning: Could not find any translations in "
                   << QDir::toNativeSeparators(sourcePath) << " (developer build?)\n.";
        return true;
    }
    // Run lconvert to concatenate all files into a single named "qt_<prefix>.qm" in the application folder
    // Use QT_INSTALL_TRANSLATIONS as working directory to keep the command line short.
    const QString absoluteTarget = QFileInfo(target).absoluteFilePath();
    const QString binary = QStringLiteral("lconvert");
    QStringList arguments;
    for (const QString &prefix : qAsConst(prefixes)) {
        arguments.clear();
        const QString targetFile = QStringLiteral("qt_") + prefix + QStringLiteral(".qm");
        arguments.append(QStringLiteral("-o"));
        const QString targetFilePath = absoluteTarget + QLatin1Char('/') + targetFile;
        if (options.json)
            options.json->addFile(sourcePath +  QLatin1Char('/') + targetFile, absoluteTarget);
        arguments.append(QDir::toNativeSeparators(targetFilePath));
        const QFileInfoList &langQmFiles = sourceDir.entryInfoList(translationNameFilters(usedQtModules, prefix));
        for (const QFileInfo &langQmFileFi : langQmFiles) {
            // winrt relies on a proper list of deployed files. We cannot cheat an mention files we do not ship here.
            if (options.json && !options.isWinRt()) {
                options.json->addFile(langQmFileFi.absoluteFilePath(),
                                      absoluteTarget);
            }
            arguments.append(langQmFileFi.fileName());
        }
        if (optVerboseLevel)
            std::wcout << "Creating " << targetFile << "...\n";
        unsigned long exitCode;
        if ((options.updateFileFlags & SkipUpdateFile) == 0
            && (!runProcess(binary, arguments, sourcePath, &exitCode, nullptr, nullptr, errorMessage)
                || exitCode)) {
            return false;
        }
    } // for prefixes.
    return true;
}

struct DeployResult
{
    operator bool() const { return success; }

    bool success = false;
    bool isDebug = false;
    quint64 directlyUsedQtLibraries = 0;
    quint64 usedQtLibraries = 0;
    quint64 deployedQtLibraries = 0;
};

static QString libraryPath(const QString &libraryLocation, const char *name,
                           const QString &qtLibInfix, Platform platform, bool debug)
{
    QString result = libraryLocation + QLatin1Char('/');
    if (platform & WindowsBased) {
        result += QLatin1String(name);
        result += qtLibInfix;
        if (debug)
            result += QLatin1Char('d');
    } else if (platform & UnixBased) {
        result += QStringLiteral("lib");
        result += QLatin1String(name);
        result += qtLibInfix;
    }
    result += sharedLibrarySuffix(platform);
    return result;
}

static QString vcDebugRedistDir() { return QStringLiteral("Debug_NonRedist"); }

static QString vcRedistDir()
{
    const char vcDirVar[] = "VCINSTALLDIR";
    const QChar slash(QLatin1Char('/'));
    QString vcRedistDirName = QDir::cleanPath(QFile::decodeName(qgetenv(vcDirVar)));
    if (vcRedistDirName.isEmpty()) {
        std::wcerr << "Warning: Cannot find Visual Studio installation directory, " << vcDirVar
            << " is not set.\n";
        return QString();
    }
    if (!vcRedistDirName.endsWith(slash))
        vcRedistDirName.append(slash);
    vcRedistDirName.append(QStringLiteral("redist"));
    if (!QFileInfo(vcRedistDirName).isDir()) {
        std::wcerr << "Warning: Cannot find Visual Studio redist directory, "
            << QDir::toNativeSeparators(vcRedistDirName).toStdWString() << ".\n";
        return QString();
    }
    const QString vc2017RedistDirName = vcRedistDirName + QStringLiteral("/MSVC");
    if (!QFileInfo(vc2017RedistDirName).isDir())
        return vcRedistDirName; // pre 2017
    // Look in reverse order for folder containing the debug redist folder
    const QFileInfoList subDirs =
        QDir(vc2017RedistDirName).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::Reversed);
    const bool isWindows10 = QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10;
    for (const QFileInfo &f : subDirs) {
        QString path = f.absoluteFilePath();
        if (QFileInfo(path + slash + vcDebugRedistDir()).isDir())
            return path;
        if (isWindows10) {
            path += QStringLiteral("/onecore");
            if (QFileInfo(path + slash + vcDebugRedistDir()).isDir())
                return path;
        }
    }
    std::wcerr << "Warning: Cannot find Visual Studio redist directory under "
        << QDir::toNativeSeparators(vc2017RedistDirName).toStdWString() << ".\n";
    return QString();
}

static QStringList compilerRunTimeLibs(Platform platform, bool isDebug, unsigned short machineArch)
{
    QStringList result;
    switch (platform) {
    case WindowsDesktopMinGW: { // MinGW: Add runtime libraries
        static const char *minGwRuntimes[] = {"*gcc_", "*stdc++", "*winpthread"};
        const QString gcc = findInPath(QStringLiteral("g++.exe"));
        if (gcc.isEmpty()) {
            std::wcerr << "Warning: Cannot find GCC installation directory. g++.exe must be in the path.\n";
            break;
        }
        const QString binPath = QFileInfo(gcc).absolutePath();
        QStringList filters;
        const QString suffix = QLatin1Char('*') + sharedLibrarySuffix(platform);
        for (auto minGwRuntime : minGwRuntimes)
            filters.append(QLatin1String(minGwRuntime) + suffix);
        const QFileInfoList &dlls = QDir(binPath).entryInfoList(filters, QDir::Files);
        for (const QFileInfo &dllFi : dlls)
                result.append(dllFi.absoluteFilePath());
    }
        break;
#ifdef Q_OS_WIN
    case WindowsDesktop: { // MSVC/Desktop: Add redistributable packages.
        QString vcRedistDirName = vcRedistDir();
        if (vcRedistDirName.isEmpty())
             break;
        QStringList redistFiles;
        QDir vcRedistDir(vcRedistDirName);
        const QString machineArchString = getArchString(machineArch);
        if (isDebug) {
            // Append DLLs from Debug_NonRedist\x??\Microsoft.VC<version>.DebugCRT.
            if (vcRedistDir.cd(vcDebugRedistDir()) && vcRedistDir.cd(machineArchString)) {
                const QStringList names = vcRedistDir.entryList(QStringList(QStringLiteral("Microsoft.VC*.DebugCRT")), QDir::Dirs);
                if (!names.isEmpty() && vcRedistDir.cd(names.first())) {
                    const QFileInfoList &dlls = vcRedistDir.entryInfoList(QStringList(QLatin1String("*.dll")));
                    for (const QFileInfo &dll : dlls)
                        redistFiles.append(dll.absoluteFilePath());
                }
            }
        } else { // release: Bundle vcredist<>.exe
            QString releaseRedistDir = vcRedistDirName;
            const QStringList countryCodes = vcRedistDir.entryList(QStringList(QStringLiteral("[0-9]*")), QDir::Dirs);
            if (!countryCodes.isEmpty()) // Pre MSVC2017
                releaseRedistDir += QLatin1Char('/') + countryCodes.constFirst();
            QFileInfo fi(releaseRedistDir + QLatin1Char('/') + QStringLiteral("vc_redist.")
                         + machineArchString + QStringLiteral(".exe"));
            if (!fi.isFile()) { // Pre MSVC2017/15.5
                fi.setFile(releaseRedistDir + QLatin1Char('/') + QStringLiteral("vcredist_")
                           + machineArchString + QStringLiteral(".exe"));
            }
            if (fi.isFile())
                redistFiles.append(fi.absoluteFilePath());
        }
        if (redistFiles.isEmpty()) {
            std::wcerr << "Warning: Cannot find Visual Studio " << (isDebug ? "debug" : "release")
                       << " redistributable files in " << QDir::toNativeSeparators(vcRedistDirName).toStdWString() << ".\n";
            break;
        }
        result.append(redistFiles);
    }
        break;
#endif // Q_OS_WIN
    default:
        break;
    }
    return result;
}

static inline int qtVersion(const QMap<QString, QString> &qmakeVariables)
{
    const QString versionString = qmakeVariables.value(QStringLiteral("QT_VERSION"));
    const QChar dot = QLatin1Char('.');
    const int majorVersion = versionString.section(dot, 0, 0).toInt();
    const int minorVersion = versionString.section(dot, 1, 1).toInt();
    const int patchVersion = versionString.section(dot, 2, 2).toInt();
    return (majorVersion << 16) | (minorVersion << 8) | patchVersion;
}

// Determine the Qt lib infix from the library path of "Qt5Core<qtblibinfix>[d].dll".
static inline QString qtlibInfixFromCoreLibName(const QString &path, bool isDebug, Platform platform)
{
    const int startPos = path.lastIndexOf(QLatin1Char('/')) + 8;
    int endPos = path.lastIndexOf(QLatin1Char('.'));
    if (isDebug && (platform & WindowsBased))
        endPos--;
    return endPos > startPos ? path.mid(startPos, endPos - startPos) : QString();
}

// Deploy a library along with its .pdb debug info file (MSVC) should it exist.
static bool updateLibrary(const QString &sourceFileName, const QString &targetDirectory,
                          const Options &options, QString *errorMessage)
{

    if (!updateFile(sourceFileName, targetDirectory, options.updateFileFlags, options.json, errorMessage))
        return false;
    if (options.deployPdb) {
        const QFileInfo pdb(pdbFileName(sourceFileName));
        if (pdb.isFile())
            return updateFile(pdb.absoluteFilePath(), targetDirectory, options.updateFileFlags, nullptr, errorMessage);
    }
    return true;
}

static DeployResult deploy(const Options &options,
                           const QMap<QString, QString> &qmakeVariables,
                           QString *errorMessage)
{
    DeployResult result;

    const QChar slash = QLatin1Char('/');

    const QString qtBinDir = qmakeVariables.value(QStringLiteral("QT_INSTALL_BINS"));
    const QString libraryLocation = options.platform == Unix ? qmakeVariables.value(QStringLiteral("QT_INSTALL_LIBS")) : qtBinDir;
    const QString infix = qmakeVariables.value(QLatin1String(qmakeInfixKey));
    const int version = qtVersion(qmakeVariables);
    Q_UNUSED(version)

    if (optVerboseLevel > 1)
        std::wcout << "Qt binaries in " << QDir::toNativeSeparators(qtBinDir) << '\n';

    QStringList dependentQtLibs;
    bool detectedDebug;
    unsigned wordSize;
    unsigned short machineArch;
    int directDependencyCount = 0;
    if (!findDependentQtLibraries(libraryLocation, options.binaries.first(), options.platform, errorMessage, &dependentQtLibs, &wordSize,
                                  &detectedDebug, &machineArch, &directDependencyCount)) {
        return result;
    }
    for (int b = 1; b < options.binaries.size(); ++b) {
        if (!findDependentQtLibraries(libraryLocation, options.binaries.at(b), options.platform, errorMessage, &dependentQtLibs,
                                      nullptr, nullptr, nullptr)) {
            return result;
        }
    }

    const bool isDebug = options.debugDetection == Options::DebugDetectionAuto ? detectedDebug: options.debugDetection == Options::DebugDetectionForceDebug;
    result.isDebug = isDebug;
    const DebugMatchMode debugMatchMode = isDebug ? MatchDebug : MatchRelease;

    // Determine application type, check Quick2 is used by looking at the
    // direct dependencies (do not be fooled by QtWebKit depending on it).
    QString qtLibInfix;
    for (int m = 0; m < directDependencyCount; ++m) {
        const quint64 module = qtModule(dependentQtLibs.at(m), infix);
        result.directlyUsedQtLibraries |= module;
        if (module == QtCoreModule)
            qtLibInfix = qtlibInfixFromCoreLibName(dependentQtLibs.at(m), detectedDebug, options.platform);
    }

    const bool usesQml2 = !(options.disabledLibraries & QtQmlModule)
        && ((result.directlyUsedQtLibraries & (QtQmlModule | QtQuickModule | Qt3DQuickModule))
                                || (options.additionalLibraries & QtQmlModule));

    if (optVerboseLevel) {
        std::wcout << QDir::toNativeSeparators(options.binaries.first()) << ' '
                   << wordSize << " bit, " << (isDebug ? "debug" : "release")
                   << " executable";
        if (usesQml2)
            std::wcout << " [QML]";
        std::wcout << '\n';
    }

    if (dependentQtLibs.isEmpty()) {
        *errorMessage = QDir::toNativeSeparators(options.binaries.first()) +  QStringLiteral(" does not seem to be a Qt executable.");
        return result;
    }

    // Some Windows-specific checks: Qt5Core depends on ICU when configured with "-icu". Other than
    // that, Qt5WebKit has a hard dependency on ICU.
    if (options.platform & WindowsBased)  {
        const QStringList qtLibs = dependentQtLibs.filter(QStringLiteral("Qt5Core"), Qt::CaseInsensitive)
            + dependentQtLibs.filter(QStringLiteral("Qt5WebKit"), Qt::CaseInsensitive);
        for (const QString &qtLib : qtLibs) {
            QStringList icuLibs = findDependentLibraries(qtLib, options.platform, errorMessage).filter(QStringLiteral("ICU"), Qt::CaseInsensitive);
            if (!icuLibs.isEmpty()) {
                // Find out the ICU version to add the data library icudtXX.dll, which does not show
                // as a dependency.
                QRegExp numberExpression(QStringLiteral("\\d+"));
                Q_ASSERT(numberExpression.isValid());
                const int index = numberExpression.indexIn(icuLibs.front());
                if (index >= 0)  {
                    const QString icuVersion = icuLibs.front().mid(index, numberExpression.matchedLength());
                    if (optVerboseLevel > 1)
                        std::wcout << "Adding ICU version " << icuVersion << '\n';
                    icuLibs.push_back(QStringLiteral("icudt") + icuVersion + QLatin1String(windowsSharedLibrarySuffix));
                }
                for (const QString &icuLib : qAsConst(icuLibs)) {
                    const QString icuPath = findInPath(icuLib);
                    if (icuPath.isEmpty()) {
                        *errorMessage = QStringLiteral("Unable to locate ICU library ") + icuLib;
                        return result;
                    }
                    dependentQtLibs.push_back(icuPath);
                } // for each icuLib
                break;
            } // !icuLibs.isEmpty()
        } // Qt5Core/Qt5WebKit
    } // Windows

    // Scan Quick2 imports
    QmlImportScanResult qmlScanResult;
    if (options.quickImports && usesQml2) {
        QStringList qmlDirectories = options.qmlDirectories;
        if (qmlDirectories.isEmpty()) {
            const QString qmlDirectory = findQmlDirectory(options.platform, options.directory);
            if (!qmlDirectory.isEmpty())
                qmlDirectories.append(qmlDirectory);
        }
        for (const QString &qmlDirectory : qAsConst(qmlDirectories)) {
            if (optVerboseLevel >= 1)
                std::wcout << "Scanning " << QDir::toNativeSeparators(qmlDirectory) << ":\n";
            const QmlImportScanResult scanResult =
                runQmlImportScanner(qmlDirectory, qmakeVariables.value(QStringLiteral("QT_INSTALL_QML")),
                                    result.directlyUsedQtLibraries & QtWidgetsModule,
                                    options.platform, debugMatchMode, errorMessage);
            if (!scanResult.ok)
                return result;
            qmlScanResult.append(scanResult);
            // Additional dependencies of QML plugins.
            for (const QString &plugin : qAsConst(qmlScanResult.plugins)) {
                if (!findDependentQtLibraries(libraryLocation, plugin, options.platform, errorMessage, &dependentQtLibs, &wordSize, &detectedDebug, &machineArch))
                    return result;
            }
            if (optVerboseLevel >= 1) {
                std::wcout << "QML imports:\n";
                for (const QmlImportScanResult::Module &mod : qAsConst(qmlScanResult.modules)) {
                    std::wcout << "  '" << mod.name << "' "
                               << QDir::toNativeSeparators(mod.sourcePath) << '\n';
                }
                if (optVerboseLevel >= 2) {
                    std::wcout << "QML plugins:\n";
                    for (const QString &p : qAsConst(qmlScanResult.plugins))
                        std::wcout << "  " << QDir::toNativeSeparators(p) << '\n';
                }
            }
        }
    }

    // Find the plugins and check whether ANGLE, D3D are required on the platform plugin.
    QString platformPlugin;
    // Sort apart Qt 5 libraries in the ones that are represented by the
    // QtModule enumeration (and thus controlled by flags) and others.
    QStringList deployedQtLibraries;
    for (int i = 0 ; i < dependentQtLibs.size(); ++i)  {
        if (const quint64 qtm = qtModule(dependentQtLibs.at(i), infix))
            result.usedQtLibraries |= qtm;
        else
            deployedQtLibraries.push_back(dependentQtLibs.at(i)); // Not represented by flag.
    }
    result.deployedQtLibraries = (result.usedQtLibraries | options.additionalLibraries) & ~options.disabledLibraries;

    const QStringList plugins =
        findQtPlugins(&result.deployedQtLibraries,
                      // For non-QML applications, disable QML to prevent it from being pulled in by the qtaccessiblequick plugin.
                      options.disabledLibraries | (usesQml2 ? 0 : (QtQmlModule | QtQuickModule)),
                      qmakeVariables.value(QStringLiteral("QT_INSTALL_PLUGINS")), libraryLocation, infix,
                      debugMatchMode, options.platform, &platformPlugin);

    // Apply options flags and re-add library names.
    QString qtGuiLibrary;
    for (const auto &qtModule : qtModuleEntries) {
        if (result.deployedQtLibraries & qtModule.module) {
            const QString library = libraryPath(libraryLocation, qtModule.libraryName, qtLibInfix, options.platform, isDebug);
            deployedQtLibraries.append(library);
            if (qtModule.module == QtGuiModule)
                qtGuiLibrary = library;
        }
    }

    if (optVerboseLevel >= 1) {
        std::wcout << "Direct dependencies: " << formatQtModules(result.directlyUsedQtLibraries).constData()
                   << "\nAll dependencies   : " << formatQtModules(result.usedQtLibraries).constData()
                   << "\nTo be deployed     : " << formatQtModules(result.deployedQtLibraries).constData() << '\n';
    }

    if (optVerboseLevel > 1)
        std::wcout << "Plugins: " << plugins.join(QLatin1Char(',')) << '\n';

    if ((result.deployedQtLibraries & QtGuiModule) && platformPlugin.isEmpty()) {
        *errorMessage =QStringLiteral("Unable to find the platform plugin.");
        return result;
    }

    // Check for ANGLE on the Qt5Gui library.
    if ((options.platform & WindowsBased) && options.platform != WinCEIntel
        && options.platform != WinCEArm && !qtGuiLibrary.isEmpty())  {
        QString libGlesName = QStringLiteral("libGLESV2");
        if (isDebug)
            libGlesName += QLatin1Char('d');
        libGlesName += QLatin1String(windowsSharedLibrarySuffix);
        QString libCombinedQtAngleName = QStringLiteral("QtANGLE");
        if (isDebug)
            libCombinedQtAngleName += QLatin1Char('d');
        libCombinedQtAngleName += QLatin1String(windowsSharedLibrarySuffix);
        const QStringList guiLibraries = findDependentLibraries(qtGuiLibrary, options.platform, errorMessage);
        const bool dependsOnAngle = !guiLibraries.filter(libGlesName, Qt::CaseInsensitive).isEmpty();
        const bool dependsOnCombinedAngle = !guiLibraries.filter(libCombinedQtAngleName, Qt::CaseInsensitive).isEmpty();
        const bool dependsOnOpenGl = !guiLibraries.filter(QStringLiteral("opengl32"), Qt::CaseInsensitive).isEmpty();
        if (options.angleDetection != Options::AngleDetectionForceOff
            && (dependsOnAngle || dependsOnCombinedAngle || !dependsOnOpenGl || options.angleDetection == Options::AngleDetectionForceOn)) {
            const QString combinedAngleFullPath = qtBinDir + slash + libCombinedQtAngleName;
            if (QFileInfo::exists(combinedAngleFullPath)) {
                deployedQtLibraries.append(combinedAngleFullPath);
            } else {
                const QString libGlesFullPath = qtBinDir + slash + libGlesName;
                deployedQtLibraries.append(libGlesFullPath);
                QString libEglFullPath = qtBinDir + slash + QStringLiteral("libEGL");
                if (isDebug)
                    libEglFullPath += QLatin1Char('d');
                libEglFullPath += QLatin1String(windowsSharedLibrarySuffix);
                deployedQtLibraries.append(libEglFullPath);
            }
            // Find the system D3d Compiler matching the D3D library.
            // Any arm64 OS will be new enough to be shipped with the D3DCompiler inbox.
            if (options.systemD3dCompiler && !options.isWinRt() && machineArch != IMAGE_FILE_MACHINE_ARM64) {
                const QString d3dCompiler = findD3dCompiler(options.platform, qtBinDir, wordSize);
                if (d3dCompiler.isEmpty()) {
                    std::wcerr << "Warning: Cannot find any version of the d3dcompiler DLL.\n";
                } else {
                    deployedQtLibraries.push_back(d3dCompiler);
                }
            }
        } // deployAngle
        if (options.softwareRasterizer && !dependsOnOpenGl) {
            const QFileInfo softwareRasterizer(qtBinDir + slash + QStringLiteral("opengl32sw") + QLatin1String(windowsSharedLibrarySuffix));
            if (softwareRasterizer.isFile())
                deployedQtLibraries.append(softwareRasterizer.absoluteFilePath());
        }
    } // Windows

    // We need to copy ucrtbased.dll on WinRT as this library is not part of
    // the c runtime package. VS 2015 does the same when deploying to a device
    // or creating an appx.
    if (isDebug && options.platform == WinRtArm
             && qmakeVariables.value(QStringLiteral("QMAKE_XSPEC")).endsWith(QLatin1String("msvc2015"))) {
        const QString extensionPath = QString::fromLocal8Bit(qgetenv("ExtensionSdkDir"));
        const QString ucrtVersion = QString::fromLocal8Bit(qgetenv("UCRTVersion"));
        if (extensionPath.isEmpty() || ucrtVersion.isEmpty()) {
            std::wcerr << "Warning: Cannot find ucrtbased.dll as either "
                << "ExtensionSdkDir or UCRTVersion is not set in "
                << "your environment.\n";
        } else {
            const QString ucrtbasedLib = extensionPath
                    + QStringLiteral("/Microsoft.UniversalCRT.Debug/")
                    + ucrtVersion
                    + QStringLiteral("/Redist/Debug/arm/ucrtbased.dll");
            const QFileInfo ucrtPath(ucrtbasedLib);
            if (ucrtPath.exists() && ucrtPath.isFile()) {
                deployedQtLibraries.append(ucrtPath.absoluteFilePath());
            } else {
                std::wcerr << "Warning: Cannot find ucrtbased.dll at "
                           << QDir::toNativeSeparators(ucrtbasedLib)
                           << " or it is not a file.\n";
            }
        }
    }

    // Update libraries
    if (options.libraries) {
        const QString targetPath = options.libraryDirectory.isEmpty() ?
            options.directory : options.libraryDirectory;
        QStringList libraries = deployedQtLibraries;
        if (options.compilerRunTime)
            libraries.append(compilerRunTimeLibs(options.platform, isDebug, machineArch));
        for (const QString &qtLib : qAsConst(libraries)) {
            if (!updateLibrary(qtLib, targetPath, options, errorMessage))
                return result;
        }

        if (options.patchQt  && !options.dryRun && !options.isWinRt()) {
            const QString qt5CoreName = QFileInfo(libraryPath(libraryLocation, "Qt5Core", qtLibInfix,
                                                              options.platform, isDebug)).fileName();

            if (!patchQtCore(targetPath + QLatin1Char('/') + qt5CoreName, errorMessage))
                return result;
        }
    } // optLibraries

    // Update plugins
    if (options.plugins) {
        const QString targetPath = options.pluginDirectory.isEmpty() ?
            options.directory : options.pluginDirectory;
        QDir dir(targetPath);
        if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
            *errorMessage = QLatin1String("Cannot create ") +
                            QDir::toNativeSeparators(dir.absolutePath()) +  QLatin1Char('.');
            return result;
        }
        for (const QString &plugin : plugins) {
            const QString targetDirName = plugin.section(slash, -2, -2);
            const QString targetPath = dir.absoluteFilePath(targetDirName);
            if (!dir.exists(targetDirName)) {
                if (optVerboseLevel)
                    std::wcout << "Creating directory " << targetPath << ".\n";
                if (!(options.updateFileFlags & SkipUpdateFile) && !dir.mkdir(targetDirName)) {
                    *errorMessage = QStringLiteral("Cannot create ") + targetDirName +  QLatin1Char('.');
                    return result;
                }
            }
            if (!updateLibrary(plugin, targetPath, options, errorMessage))
                return result;
        }
    } // optPlugins

    // Update Quick imports
    const bool usesQuick1 = result.deployedQtLibraries & QtDeclarativeModule;
    // Do not be fooled by QtWebKit.dll depending on Quick into always installing Quick imports
    // for WebKit1-applications. Check direct dependency only.
    if (options.quickImports && (usesQuick1 || usesQml2)) {
        if (usesQml2) {
            for (const QmlImportScanResult::Module &module : qAsConst(qmlScanResult.modules)) {
                const QString installPath = module.installPath(options.directory);
                if (optVerboseLevel > 1)
                    std::wcout << "Installing: '" << module.name
                               << "' from " << module.sourcePath << " to "
                               << QDir::toNativeSeparators(installPath) << '\n';
                if (installPath != options.directory && !createDirectory(installPath, errorMessage))
                    return result;
                unsigned updateFileFlags = options.updateFileFlags | SkipQmlDesignerSpecificsDirectories;
                unsigned qmlDirectoryFileFlags = 0;
                if (options.deployPdb)
                    qmlDirectoryFileFlags |= QmlDirectoryFileEntryFunction::DeployPdb;
                if (!updateFile(module.sourcePath, QmlDirectoryFileEntryFunction(options.platform, debugMatchMode, qmlDirectoryFileFlags),
                                installPath, updateFileFlags, options.json, errorMessage)) {
                    return result;
                }
            }
        } // Quick 2
        if (usesQuick1) {
            const QString quick1ImportPath = qmakeVariables.value(QStringLiteral("QT_INSTALL_IMPORTS"));
            const QmlDirectoryFileEntryFunction qmlFileEntryFunction(options.platform, debugMatchMode, options.deployPdb ? QmlDirectoryFileEntryFunction::DeployPdb : 0);
            QStringList quick1Imports(QStringLiteral("Qt"));
            if (result.deployedQtLibraries & QtWebKitModule)
                quick1Imports << QStringLiteral("QtWebKit");
            for (const QString &quick1Import : qAsConst(quick1Imports)) {
                const QString sourceFile = quick1ImportPath + slash + quick1Import;
                if (!updateFile(sourceFile, qmlFileEntryFunction, options.directory, options.updateFileFlags, options.json, errorMessage))
                    return result;
            }
        } // Quick 1
    } // optQuickImports

    if (options.translations) {
        if (!options.dryRun && !createDirectory(options.translationsDirectory, errorMessage))
            return result;
        if (!deployTranslations(qmakeVariables.value(QStringLiteral("QT_INSTALL_TRANSLATIONS")),
                                result.deployedQtLibraries, options.translationsDirectory,
                                options, errorMessage)) {
            return result;
        }
    }

    result.success = true;
    return result;
}

static bool deployWebProcess(const QMap<QString, QString> &qmakeVariables,
                             const char *binaryName,
                             const Options &sourceOptions, QString *errorMessage)
{
    // Copy the web process and its dependencies
    const QString webProcess = webProcessBinary(binaryName, sourceOptions.platform);
    const QString webProcessSource = qmakeVariables.value(QStringLiteral("QT_INSTALL_LIBEXECS")) +
                                     QLatin1Char('/') + webProcess;
    if (!updateFile(webProcessSource, sourceOptions.directory, sourceOptions.updateFileFlags, sourceOptions.json, errorMessage))
        return false;
    Options options(sourceOptions);
    options.binaries.append(options.directory + QLatin1Char('/') + webProcess);
    options.quickImports = false;
    options.translations = false;
    return deploy(options, qmakeVariables, errorMessage);
}

static bool deployWebEngineCore(const QMap<QString, QString> &qmakeVariables,
                                const Options &options, bool isDebug, QString *errorMessage)
{
    static const char *installDataFiles[] = {"icudtl.dat",
                                             "qtwebengine_devtools_resources.pak",
                                             "qtwebengine_resources.pak",
                                             "qtwebengine_resources_100p.pak",
                                             "qtwebengine_resources_200p.pak"};
    QByteArray webEngineProcessName(webEngineProcessC);
    if (isDebug)
        webEngineProcessName.append('d');
    if (optVerboseLevel)
        std::wcout << "Deploying: " << webEngineProcessName.constData() << "...\n";
    if (!deployWebProcess(qmakeVariables, webEngineProcessName, options, errorMessage))
        return false;
    const QString resourcesSubDir = QStringLiteral("/resources");
    const QString resourcesSourceDir
        = qmakeVariables.value(QStringLiteral("QT_INSTALL_DATA")) + resourcesSubDir
            + QLatin1Char('/');
    const QString resourcesTargetDir(options.directory + resourcesSubDir);
    if (!createDirectory(resourcesTargetDir, errorMessage))
        return false;
    for (auto installDataFile : installDataFiles) {
        if (!updateFile(resourcesSourceDir + QLatin1String(installDataFile),
                        resourcesTargetDir, options.updateFileFlags, options.json, errorMessage)) {
            return false;
        }
    }
    const QFileInfo translations(qmakeVariables.value(QStringLiteral("QT_INSTALL_TRANSLATIONS"))
                                 + QStringLiteral("/qtwebengine_locales"));
    if (!translations.isDir()) {
        std::wcerr << "Warning: Cannot find the translation files of the QtWebEngine module at "
            << QDir::toNativeSeparators(translations.absoluteFilePath()) << ".\n";
        return true;
    }
    if (options.translations) {
        // Copy the whole translations directory.
        return createDirectory(options.translationsDirectory, errorMessage)
                && updateFile(translations.absoluteFilePath(), options.translationsDirectory,
                              options.updateFileFlags, options.json, errorMessage);
    }
    // Translations have been turned off, but QtWebEngine needs at least one.
    const QFileInfo enUSpak(translations.filePath() + QStringLiteral("/en-US.pak"));
    if (!enUSpak.exists()) {
        std::wcerr << "Warning: Cannot find "
                   << QDir::toNativeSeparators(enUSpak.absoluteFilePath()) << ".\n";
        return true;
    }
    const QString webEngineTranslationsDir = options.translationsDirectory + QLatin1Char('/')
            + translations.fileName();
    if (!createDirectory(webEngineTranslationsDir, errorMessage))
        return false;
    return updateFile(enUSpak.absoluteFilePath(), webEngineTranslationsDir,
                      options.updateFileFlags, options.json, errorMessage);
}

int main(int argc, char **argv)
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationVersion(QLatin1String(QT_VERSION_STR));

    const QByteArray qtBinPath = QFile::encodeName(QDir::toNativeSeparators(QCoreApplication::applicationDirPath()));
    QByteArray path = qgetenv("PATH");
    if (!path.contains(qtBinPath)) { // QTBUG-39177, ensure Qt is in the path so that qt.conf is taken into account.
        path += ';';
        path += qtBinPath;
        qputenv("PATH", path);
    }

    Options options;
    QString errorMessage;
    const QMap<QString, QString> qmakeVariables = queryQMakeAll(&errorMessage);
    const QString xSpec = qmakeVariables.value(QStringLiteral("QMAKE_XSPEC"));
    options.platform = platformFromMkSpec(xSpec);
    if (options.platform == WindowsDesktopMinGW || options.platform == WindowsDesktop)
        options.compilerRunTime = true;

    {   // Command line
        QCommandLineParser parser;
        QString errorMessage;
        const int result = parseArguments(QCoreApplication::arguments(), &parser, &options, &errorMessage);
        if (result & CommandLineParseError)
            std::wcerr << errorMessage << "\n\n";
        if (result & CommandLineParseHelpRequested)
            std::fputs(qPrintable(helpText(parser)), stdout);
        if (result & CommandLineParseError)
            return 1;
        if (result & CommandLineParseHelpRequested)
            return 0;
    }

    if (qmakeVariables.isEmpty() || xSpec.isEmpty() || !qmakeVariables.contains(QStringLiteral("QT_INSTALL_BINS"))) {
        std::wcerr << "Unable to query qmake: " << errorMessage << '\n';
        return 1;
    }

    if (options.platform == UnknownPlatform) {
        std::wcerr << "Unsupported platform " << xSpec << '\n';
        return 1;
    }

    // Create directories
    if (!createDirectory(options.directory, &errorMessage)) {
        std::wcerr << errorMessage << '\n';
        return 1;
    }
    if (!options.libraryDirectory.isEmpty() && options.libraryDirectory != options.directory
        && !createDirectory(options.libraryDirectory, &errorMessage)) {
        std::wcerr << errorMessage << '\n';
        return 1;
    }

    if (optWebKit2 == OptionEnabled)
        options.additionalLibraries |= QtWebKitModule;


    const DeployResult result = deploy(options, qmakeVariables, &errorMessage);
    if (!result) {
        std::wcerr << errorMessage << '\n';
        return 1;
    }

    if ((optWebKit2 != OptionDisabled)
        && (optWebKit2 == OptionEnabled
            || ((result.deployedQtLibraries & QtWebKitModule)
                && (result.directlyUsedQtLibraries & QtQuickModule)))) {
        if (optVerboseLevel)
            std::wcout << "Deploying: " << webKitProcessC << "...\n";
        if (!deployWebProcess(qmakeVariables, webKitProcessC, options, &errorMessage)) {
            std::wcerr << errorMessage << '\n';
            return 1;
        }
    }

    if (result.deployedQtLibraries & QtWebEngineCoreModule) {
        if (!deployWebEngineCore(qmakeVariables, options, result.isDebug, &errorMessage)) {
            std::wcerr << errorMessage << '\n';
            return 1;
        }
    }

    if (options.json) {
        if (options.list)
            std::fputs(options.json->toList(options.list, options.directory).constData(), stdout);
        else
            std::fputs(options.json->toJson().constData(), stdout);
        delete options.json;
        options.json = nullptr;
    }

    return 0;
}

QT_END_NAMESPACE
