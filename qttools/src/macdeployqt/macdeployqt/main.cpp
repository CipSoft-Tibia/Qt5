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
#include <QCoreApplication>
#include <QDir>

#include "../shared/shared.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QString appBundlePath;
    if (argc > 1)
        appBundlePath = QString::fromLocal8Bit(argv[1]);

    if (argc < 2 || appBundlePath.startsWith("-")) {
        qDebug() << "Usage: macdeployqt app-bundle [options]";
        qDebug() << "";
        qDebug() << "Options:";
        qDebug() << "   -verbose=<0-3>     : 0 = no output, 1 = error/warning (default), 2 = normal, 3 = debug";
        qDebug() << "   -no-plugins        : Skip plugin deployment";
        qDebug() << "   -dmg               : Create a .dmg disk image";
        qDebug() << "   -no-strip          : Don't run 'strip' on the binaries";
        qDebug() << "   -use-debug-libs    : Deploy with debug versions of frameworks and plugins (implies -no-strip)";
        qDebug() << "   -executable=<path> : Let the given executable use the deployed frameworks too";
        qDebug() << "   -qmldir=<path>     : Scan for QML imports in the given path";
        qDebug() << "   -always-overwrite  : Copy files even if the target file exists";
        qDebug() << "   -codesign=<ident>  : Run codesign with the given identity on all executables";
        qDebug() << "   -appstore-compliant: Skip deployment of components that use private API";
        qDebug() << "   -libpath=<path>    : Add the given path to the library search path";
        qDebug() << "   -fs=<filesystem>   : Set the filesystem used for the .dmg disk image (defaults to HFS+)";
        qDebug() << "";
        qDebug() << "macdeployqt takes an application bundle as input and makes it";
        qDebug() << "self-contained by copying in the Qt frameworks and plugins that";
        qDebug() << "the application uses.";
        qDebug() << "";
        qDebug() << "Plugins related to a framework are copied in with the";
        qDebug() << "framework. The accessibility, image formats, and text codec";
        qDebug() << "plugins are always copied, unless \"-no-plugins\" is specified.";
        qDebug() << "";
        qDebug() << "Qt plugins may use private API and will cause the app to be";
        qDebug() << "rejected from the Mac App store. MacDeployQt will print a warning";
        qDebug() << "when known incompatible plugins are deployed. Use -appstore-compliant ";
        qDebug() << "to skip these plugins. Currently two SQL plugins are known to";
        qDebug() << "be incompatible: qsqlodbc and qsqlpsql.";
        qDebug() << "";
        qDebug() << "See the \"Deploying Applications on OS X\" topic in the";
        qDebug() << "documentation for more information about deployment on OS X.";

        return 1;
    }

    appBundlePath = QDir::cleanPath(appBundlePath);

    if (QDir().exists(appBundlePath) == false) {
        qDebug() << "Error: Could not find app bundle" << appBundlePath;
        return 1;
    }

    bool plugins = true;
    bool dmg = false;
    QByteArray filesystem("HFS+");
    bool useDebugLibs = false;
    extern bool runStripEnabled;
    extern bool alwaysOwerwriteEnabled;
    extern QStringList librarySearchPath;
    QStringList additionalExecutables;
    bool qmldirArgumentUsed = false;
    QStringList qmlDirs;
    extern bool runCodesign;
    extern QString codesignIdentiy;
    extern bool appstoreCompliant;
    extern bool deployFramework;

    for (int i = 2; i < argc; ++i) {
        QByteArray argument = QByteArray(argv[i]);
        if (argument == QByteArray("-no-plugins")) {
            LogDebug() << "Argument found:" << argument;
            plugins = false;
        } else if (argument == QByteArray("-dmg")) {
            LogDebug() << "Argument found:" << argument;
            dmg = true;
        } else if (argument == QByteArray("-no-strip")) {
            LogDebug() << "Argument found:" << argument;
            runStripEnabled = false;
        } else if (argument == QByteArray("-use-debug-libs")) {
            LogDebug() << "Argument found:" << argument;
            useDebugLibs = true;
            runStripEnabled = false;
        } else if (argument.startsWith(QByteArray("-verbose"))) {
            LogDebug() << "Argument found:" << argument;
            int index = argument.indexOf("=");
            bool ok = false;
            int number = argument.mid(index+1).toInt(&ok);
            if (!ok)
                LogError() << "Could not parse verbose level";
            else
                logLevel = number;
        } else if (argument.startsWith(QByteArray("-executable"))) {
            LogDebug() << "Argument found:" << argument;
            int index = argument.indexOf('=');
            if (index == -1)
                LogError() << "Missing executable path";
            else
                additionalExecutables << argument.mid(index+1);
        } else if (argument.startsWith(QByteArray("-qmldir"))) {
            LogDebug() << "Argument found:" << argument;
            qmldirArgumentUsed = true;
            int index = argument.indexOf('=');
            if (index == -1)
                LogError() << "Missing qml directory path";
            else
                qmlDirs << argument.mid(index+1);
        } else if (argument.startsWith(QByteArray("-libpath"))) {
            LogDebug() << "Argument found:" << argument;
            int index = argument.indexOf('=');
            if (index == -1)
                LogError() << "Missing library search path";
            else
                librarySearchPath << argument.mid(index+1);
        } else if (argument == QByteArray("-always-overwrite")) {
            LogDebug() << "Argument found:" << argument;
            alwaysOwerwriteEnabled = true;
        } else if (argument.startsWith(QByteArray("-codesign"))) {
            LogDebug() << "Argument found:" << argument;
            int index = argument.indexOf("=");
            if (index < 0 || index >= argument.size()) {
                LogError() << "Missing code signing identity";
            } else {
                runCodesign = true;
                codesignIdentiy = argument.mid(index+1);
            }
        } else if (argument == QByteArray("-appstore-compliant")) {
            LogDebug() << "Argument found:" << argument;
            appstoreCompliant = true;

        // Undocumented option, may not work as intented
        } else if (argument == QByteArray("-deploy-framework")) {
            LogDebug() << "Argument found:" << argument;
            deployFramework = true;

        } else if (argument.startsWith(QByteArray("-fs"))) {
            LogDebug() << "Argument found:" << argument;
            int index = argument.indexOf('=');
            if (index == -1)
                LogError() << "Missing filesystem type";
            else
                filesystem = argument.mid(index+1);
        } else if (argument.startsWith("-")) {
            LogError() << "Unknown argument" << argument << "\n";
            return 1;
        }
     }

    DeploymentInfo deploymentInfo = deployQtFrameworks(appBundlePath, additionalExecutables, useDebugLibs);

    if (deploymentInfo.isDebug)
        useDebugLibs = true;

    if (deployFramework && deploymentInfo.isFramework)
        fixupFramework(appBundlePath);

    // Convenience: Look for .qml files in the current directoty if no -qmldir specified.
    if (qmlDirs.isEmpty()) {
        QDir dir;
        if (!dir.entryList(QStringList() << QStringLiteral("*.qml")).isEmpty()) {
            qmlDirs += QStringLiteral(".");
        }
    }

    if (!qmlDirs.isEmpty()) {
        bool ok = deployQmlImports(appBundlePath, deploymentInfo, qmlDirs);
        if (!ok && qmldirArgumentUsed)
            return 1; // exit if the user explicitly asked for qml import deployment

        // Update deploymentInfo.deployedFrameworks - the QML imports
        // may have brought in extra frameworks as dependencies.
        deploymentInfo.deployedFrameworks += findAppFrameworkNames(appBundlePath);
        deploymentInfo.deployedFrameworks = deploymentInfo.deployedFrameworks.toSet().toList();
    }

    if (plugins && !deploymentInfo.qtPath.isEmpty()) {
        deploymentInfo.pluginPath = deploymentInfo.qtPath + "/plugins";
        LogNormal();
        deployPlugins(appBundlePath, deploymentInfo, useDebugLibs);
        createQtConf(appBundlePath);
    }

    if (runStripEnabled)
        stripAppBinary(appBundlePath);

    if (runCodesign)
        codesign(codesignIdentiy, appBundlePath);

    if (dmg) {
        LogNormal();
        createDiskImage(appBundlePath, filesystem);
    }

    return 0;
}

