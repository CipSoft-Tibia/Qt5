// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/qtest.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qplugin.h>
#include <private/qfactoryloader_p.h>
#include "plugin1/plugininterface1.h"
#include "plugin2/plugininterface2.h"

#if !QT_CONFIG(library)
Q_IMPORT_PLUGIN(Plugin1)
Q_IMPORT_PLUGIN(Plugin2)
#endif

class tst_QFactoryLoader : public QObject
{
    Q_OBJECT

    QString binFolder;
public slots:
    void initTestCase();

private slots:
    void usingTwoFactoriesFromSameDir();
    void extraSearchPath();
    void staticPlugin_data();
    void staticPlugin();
};

static const char binFolderC[] = "bin";

void tst_QFactoryLoader::initTestCase()
{
    // On Android the plugins are bundled into APK's libs subdir
#ifndef Q_OS_ANDROID
    binFolder = QFINDTESTDATA(binFolderC);
    QVERIFY2(!binFolder.isEmpty(), "Unable to locate 'bin' folder");
#endif
}

void tst_QFactoryLoader::usingTwoFactoriesFromSameDir()
{
#if QT_CONFIG(library) && !defined(Q_OS_ANDROID)
    // set the library path to contain the directory where the 'bin' dir is located
    QCoreApplication::setLibraryPaths( { QFileInfo(binFolder).absolutePath() });
#endif

    const QString suffix = QLatin1Char('/') + QLatin1String(binFolderC);
    QFactoryLoader loader1(PluginInterface1_iid, suffix);

    PluginInterface1 *plugin1 = qobject_cast<PluginInterface1 *>(loader1.instance(0));
    QVERIFY2(plugin1,
             qPrintable(QString::fromLatin1("Cannot load plugin '%1'")
                        .arg(QLatin1String(PluginInterface1_iid))));

    QFactoryLoader loader2(PluginInterface2_iid, suffix);

    PluginInterface2 *plugin2 = qobject_cast<PluginInterface2 *>(loader2.instance(0));
    QVERIFY2(plugin2,
             qPrintable(QString::fromLatin1("Cannot load plugin '%1'")
                        .arg(QLatin1String(PluginInterface2_iid))));

    QCOMPARE(plugin1->pluginName(), QLatin1String("Plugin1 ok"));
    QCOMPARE(plugin2->pluginName(), QLatin1String("Plugin2 ok"));
}

void tst_QFactoryLoader::extraSearchPath()
{
#if defined(Q_OS_ANDROID) && !QT_CONFIG(library)
    QSKIP("Test not applicable in this configuration.");
#else
#ifdef Q_OS_ANDROID
    // On Android the libs are not stored in binFolder, but bundled into
    // APK's libs subdir
    const QStringList androidLibsPaths = QCoreApplication::libraryPaths();
    QCOMPARE(androidLibsPaths.size(), 1);
#endif
    QCoreApplication::setLibraryPaths(QStringList());

#ifndef Q_OS_ANDROID
    QString pluginsPath = QFileInfo(binFolder).absoluteFilePath();
    QFactoryLoader loader1(PluginInterface1_iid, "/nonexistent");
#else
    QString pluginsPath = androidLibsPaths.first();
    // On Android we still need to specify a valid suffix, because it's a part
    // of a file name, not directory structure
    const QString suffix = QLatin1Char('/') + QLatin1String(binFolderC);
    QFactoryLoader loader1(PluginInterface1_iid, suffix);
#endif

    // it shouldn't have scanned anything because we haven't given it a path yet
    QVERIFY(loader1.metaData().isEmpty());

    loader1.setExtraSearchPath(pluginsPath);
    PluginInterface1 *plugin1 = qobject_cast<PluginInterface1 *>(loader1.instance(0));
    QVERIFY2(plugin1,
             qPrintable(QString::fromLatin1("Cannot load plugin '%1'")
                        .arg(QLatin1String(PluginInterface1_iid))));

    QCOMPARE(plugin1->pluginName(), QLatin1String("Plugin1 ok"));

    // check that it forgets that plugin
    loader1.setExtraSearchPath(QString());
    QVERIFY(loader1.metaData().isEmpty());
#endif
}

Q_IMPORT_PLUGIN(StaticPlugin1)
Q_IMPORT_PLUGIN(StaticPlugin2)
constexpr bool IsDebug =
#ifdef QT_NO_DEBUG
        false &&
#endif
        true;

void tst_QFactoryLoader::staticPlugin_data()
{
    QTest::addColumn<QString>("iid");
    auto addRow = [](const char *iid) {
        QTest::addRow("%s", iid) << QString(iid);
    };
    addRow("StaticPlugin1");
    addRow("StaticPlugin2");
}

void tst_QFactoryLoader::staticPlugin()
{
    QFETCH(QString, iid);
    QFactoryLoader loader(iid.toLatin1(), "/irrelevant");
    QFactoryLoader::MetaDataList list = loader.metaData();
    QCOMPARE(list.size(), 1);

    QCborMap map = list.at(0).toCbor();
    QCOMPARE(map[int(QtPluginMetaDataKeys::QtVersion)],
            QT_VERSION_CHECK(QT_VERSION_MAJOR, QT_VERSION_MINOR, 0));
    QCOMPARE(map[int(QtPluginMetaDataKeys::IID)], iid);
    QCOMPARE(map[int(QtPluginMetaDataKeys::ClassName)], iid);
    QCOMPARE(map[int(QtPluginMetaDataKeys::IsDebug)], IsDebug);

    QCborValue metaData = map[int(QtPluginMetaDataKeys::MetaData)];
    QVERIFY(metaData.isMap());
    QCOMPARE(metaData["Keys"], QCborArray{ "Value" });
    QCOMPARE(loader.indexOf("Value"), 0);

    // instantiate
    QObject *instance = loader.instance(0);
    QVERIFY(instance);
    QCOMPARE(instance->metaObject()->className(), iid);
}

QTEST_MAIN(tst_QFactoryLoader)
#include "tst_qfactoryloader.moc"
