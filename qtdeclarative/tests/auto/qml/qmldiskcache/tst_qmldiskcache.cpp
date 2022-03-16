/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <qtest.h>

#include <private/qv4compileddata_p.h>
#include <private/qv4compiler_p.h>
#include <private/qv8engine_p.h>
#include <private/qv4engine_p.h>
#include <private/qv4codegen_p.h>
#include <private/qqmlcomponent_p.h>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlFileSelector>
#include <QThread>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QDirIterator>

class tst_qmldiskcache: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void loadLocalAsFallback();
    void regenerateAfterChange();
    void registerImportForImplicitComponent();
    void basicVersionChecks();
    void recompileAfterChange();
    void recompileAfterDirectoryChange();
    void fileSelectors();
    void localAliases();
    void cacheResources();
    void stableOrderOfDependentCompositeTypes();
    void singletonDependency();
    void cppRegisteredSingletonDependency();
    void cacheModuleScripts();

private:
    QDir m_qmlCacheDirectory;
};

// A wrapper around QQmlComponent to ensure the temporary reference counts
// on the type data as a result of the main thread <> loader thread communication
// are dropped. Regular Synchronous loading will leave us with an event posted
// to the gui thread and an extra refcount that will only be dropped after the
// event delivery. A plain sendPostedEvents() however is insufficient because
// we can't be sure that the event is posted after the constructor finished.
class CleanlyLoadingComponent : public QQmlComponent
{
public:
    CleanlyLoadingComponent(QQmlEngine *engine, const QUrl &url)
        : QQmlComponent(engine, url, QQmlComponent::Asynchronous)
    { waitForLoad(); }
    CleanlyLoadingComponent(QQmlEngine *engine, const QString &fileName)
        : QQmlComponent(engine, fileName, QQmlComponent::Asynchronous)
    { waitForLoad(); }

    void waitForLoad()
    {
        QTRY_VERIFY(status() == QQmlComponent::Ready || status() == QQmlComponent::Error);
    }
};

static void waitForFileSystem()
{
    // On macOS with HFS+ the precision of file times is measured in seconds, so to ensure that
    // the newly written file has a modification date newer than an existing cache file, we must
    // wait.
    // Similar effects of lacking precision have been observed on some Linux systems.
    static const bool fsHasSubSecondResolution = []() {
        QDateTime mtime = QFileInfo(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).lastModified();
        // 1:1000 chance of a false negative
        return mtime.toMSecsSinceEpoch() % 1000;
    }();
    if (!fsHasSubSecondResolution)
        QThread::sleep(1);
}

struct TestCompiler
{
    TestCompiler(QQmlEngine *engine)
        : engine(engine)
        , tempDir()
        , currentMapping(nullptr)
    {
        init(tempDir.path());
    }

    void init(const QString &baseDirectory)
    {
        closeMapping();
        testFilePath = baseDirectory + QStringLiteral("/test.qml");
        cacheFilePath = QV4::CompiledData::CompilationUnit::localCacheFilePath(QUrl::fromLocalFile(testFilePath));
        mappedFile.setFileName(cacheFilePath);
    }

    bool compile(const QByteArray &contents)
    {
        closeMapping();
        engine->clearComponentCache();

        waitForFileSystem();

        {
            QFile f(testFilePath);
            if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                lastErrorString = f.errorString();
                return false;
            }
            if (f.write(contents) != contents.size()) {
                lastErrorString = f.errorString();
                return false;
            }
        }

        CleanlyLoadingComponent component(engine, testFilePath);
        if (!component.isReady()) {
            lastErrorString = component.errorString();
            return false;
        }

        return true;
    }

    const QV4::CompiledData::Unit *mapUnit()
    {
        if (!mappedFile.open(QIODevice::ReadOnly)) {
            lastErrorString = mappedFile.errorString();
            return nullptr;
        }

        currentMapping = mappedFile.map(/*offset*/0, mappedFile.size());
        if (!currentMapping) {
            lastErrorString = mappedFile.errorString();
            return nullptr;
        }
        QV4::CompiledData::Unit *unitPtr;
        memcpy(&unitPtr, &currentMapping, sizeof(unitPtr));
        return unitPtr;
    }

    typedef void (*HeaderTweakFunction)(QV4::CompiledData::Unit *header);
    bool tweakHeader(HeaderTweakFunction function)
    {
        closeMapping();

        QFile f(cacheFilePath);
        if (!f.open(QIODevice::ReadWrite))
            return false;
        QV4::CompiledData::Unit header;
        if (f.read(reinterpret_cast<char *>(&header), sizeof(header)) != sizeof(header))
            return false;
        function(&header);
        f.seek(0);
        return f.write(reinterpret_cast<const char *>(&header), sizeof(header)) == sizeof(header);
    }

    bool verify()
    {
        QQmlRefPointer<QV4::CompiledData::CompilationUnit> unit = QV4::Compiler::Codegen::createUnitForLoading();
        return unit->loadFromDisk(QUrl::fromLocalFile(testFilePath), QFileInfo(testFilePath).lastModified(), &lastErrorString);
    }

    void closeMapping()
    {
        if (currentMapping) {
            mappedFile.unmap(currentMapping);
            currentMapping = nullptr;
        }
        mappedFile.close();
    }

    void clearCache()
    {
        closeMapping();
        QFile::remove(cacheFilePath);
    }

    QQmlEngine *engine;
    const QTemporaryDir tempDir;
    QString testFilePath;
    QString cacheFilePath;
    QString lastErrorString;
    QFile mappedFile;
    uchar *currentMapping;
};

void tst_qmldiskcache::initTestCase()
{
    qputenv("QML_FORCE_DISK_CACHE", "1");
    QStandardPaths::setTestModeEnabled(true);

    const QString cacheDirectory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    m_qmlCacheDirectory.setPath(cacheDirectory + QLatin1String("/qmlcache"));
    if (m_qmlCacheDirectory.exists())
        QVERIFY(m_qmlCacheDirectory.removeRecursively());
    QVERIFY(QDir::root().mkpath(m_qmlCacheDirectory.absolutePath()));
}

void tst_qmldiskcache::cleanupTestCase()
{
    m_qmlCacheDirectory.removeRecursively();
}

void tst_qmldiskcache::loadLocalAsFallback()
{
    QQmlEngine engine;
    TestCompiler testCompiler(&engine);

    QVERIFY(testCompiler.tempDir.isValid());

    const QByteArray contents = QByteArrayLiteral("import QtQml 2.0\n"
                                                  "QtObject {\n"
                                                   "    property string blah: Qt.platform;\n"
                                                   "}");

    QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));

    // Create an invalid side-by-side .qmlc
    {
        QFile f(testCompiler.tempDir.path() + "/test.qmlc");
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QV4::CompiledData::Unit unit = {};
        memcpy(unit.magic, QV4::CompiledData::magic_str, sizeof(unit.magic));
        unit.version = QV4_DATA_STRUCTURE_VERSION;
        unit.qtVersion = QT_VERSION;
        unit.sourceTimeStamp = testCompiler.mappedFile.fileTime(QFile::FileModificationTime).toMSecsSinceEpoch();
        unit.unitSize = ~0U;    // make the size a silly number
        // write something to the library hash that should cause it not to be loaded
        memset(unit.libraryVersionHash, 'z', sizeof(unit.libraryVersionHash));
        memset(unit.md5Checksum, 0, sizeof(unit.md5Checksum));

        // leave the other fields unset, since they don't matter

        f.write(reinterpret_cast<const char *>(&unit), sizeof(unit));
    }

    QQmlRefPointer<QV4::CompiledData::CompilationUnit> unit = QV4::Compiler::Codegen::createUnitForLoading();
    bool loaded = unit->loadFromDisk(QUrl::fromLocalFile(testCompiler.testFilePath), QFileInfo(testCompiler.testFilePath).lastModified(),
                                     &testCompiler.lastErrorString);
    QVERIFY2(loaded, qPrintable(testCompiler.lastErrorString));
    QCOMPARE(unit->objectCount(), 1);
}

void tst_qmldiskcache::regenerateAfterChange()
{
    QQmlEngine engine;
    TestCompiler testCompiler(&engine);

    QVERIFY(testCompiler.tempDir.isValid());

    const QByteArray contents = QByteArrayLiteral("import QtQml 2.0\n"
                                                  "QtObject {\n"
                                                   "    property string blah: Qt.platform;\n"
                                                   "}");

    QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));

    {
        const QV4::CompiledData::Unit *testUnit = testCompiler.mapUnit();
        QVERIFY2(testUnit, qPrintable(testCompiler.lastErrorString));

        const QV4::CompiledData::QmlUnit *qmlUnit = testUnit->qmlUnit();

        QCOMPARE(quint32(qmlUnit->nObjects), quint32(1));

        const QV4::CompiledData::Object *obj = qmlUnit->objectAt(0);
        QCOMPARE(quint32(obj->nBindings), quint32(1));
        QCOMPARE(quint32(obj->bindingTable()->type), quint32(QV4::CompiledData::Binding::Type_Script));
        QCOMPARE(quint32(obj->bindingTable()->value.compiledScriptIndex), quint32(0));

        QCOMPARE(quint32(testUnit->functionTableSize), quint32(1));

        const QV4::CompiledData::Function *bindingFunction = testUnit->functionAt(0);
        QCOMPARE(testUnit->stringAtInternal(bindingFunction->nameIndex), QString("expression for blah")); // check if we have the correct function
        QVERIFY(bindingFunction->codeSize > 0);
        QVERIFY(bindingFunction->codeOffset < testUnit->unitSize);
    }

    {
        const QByteArray newContents = QByteArrayLiteral("import QtQml 2.0\n"
                                                         "QtObject {\n"
                                                         "    property string blah: Qt.platform;\n"
                                                         "    property int secondProperty: 42;\n"
                                                         "}");

        QVERIFY2(testCompiler.compile(newContents), qPrintable(testCompiler.lastErrorString));
        const QV4::CompiledData::Unit *testUnit = testCompiler.mapUnit();
        QVERIFY2(testUnit, qPrintable(testCompiler.lastErrorString));

        const QV4::CompiledData::QmlUnit *qmlUnit = testUnit->qmlUnit();

        QCOMPARE(quint32(qmlUnit->nObjects), quint32(1));

        const QV4::CompiledData::Object *obj = qmlUnit->objectAt(0);
        QCOMPARE(quint32(obj->nBindings), quint32(2));
        QCOMPARE(quint32(obj->bindingTable()->type), quint32(QV4::CompiledData::Binding::Type_Number));
        QCOMPARE(obj->bindingTable()->valueAsNumber(reinterpret_cast<const QV4::Value *>(testUnit->constants())), double(42));

        QCOMPARE(quint32(testUnit->functionTableSize), quint32(1));

        const QV4::CompiledData::Function *bindingFunction = testUnit->functionAt(0);
        QCOMPARE(testUnit->stringAtInternal(bindingFunction->nameIndex), QString("expression for blah")); // check if we have the correct function
        QVERIFY(bindingFunction->codeSize > 0);
        QVERIFY(bindingFunction->codeOffset < testUnit->unitSize);
    }
}

void tst_qmldiskcache::registerImportForImplicitComponent()
{
    QQmlEngine engine;

    TestCompiler testCompiler(&engine);
    QVERIFY(testCompiler.tempDir.isValid());

    const QByteArray contents = QByteArrayLiteral("import QtQuick 2.0\n"
                                                  "Loader {\n"
                                                   "    sourceComponent: Item {}\n"
                                                   "}");

    QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));
    {
        const QV4::CompiledData::Unit *testUnit = testCompiler.mapUnit();
        QVERIFY2(testUnit, qPrintable(testCompiler.lastErrorString));

        const QV4::CompiledData::QmlUnit *qmlUnit = testUnit->qmlUnit();
        QCOMPARE(quint32(qmlUnit->nImports), quint32(2));
        QCOMPARE(testUnit->stringAtInternal(qmlUnit->importAt(0)->uriIndex), QStringLiteral("QtQuick"));

        QQmlType componentType = QQmlMetaType::qmlType(&QQmlComponent::staticMetaObject);

        QCOMPARE(testUnit->stringAtInternal(qmlUnit->importAt(1)->uriIndex), QString(componentType.module()));
        QCOMPARE(testUnit->stringAtInternal(qmlUnit->importAt(1)->qualifierIndex), QStringLiteral("QmlInternals"));

        QCOMPARE(quint32(qmlUnit->nObjects), quint32(3));

        const QV4::CompiledData::Object *obj = qmlUnit->objectAt(0);
        QCOMPARE(quint32(obj->nBindings), quint32(1));
        QCOMPARE(quint32(obj->bindingTable()->type), quint32(QV4::CompiledData::Binding::Type_Object));

        const QV4::CompiledData::Object *implicitComponent = qmlUnit->objectAt(obj->bindingTable()->value.objectIndex);
        QCOMPARE(testUnit->stringAtInternal(implicitComponent->inheritedTypeNameIndex), QStringLiteral("QmlInternals.") + componentType.elementName());
    }
}

void tst_qmldiskcache::basicVersionChecks()
{
    QQmlEngine engine;

    TestCompiler testCompiler(&engine);
    QVERIFY(testCompiler.tempDir.isValid());

    const QByteArray contents = QByteArrayLiteral("import QtQml 2.0\n"
                                                  "QtObject {\n"
                                                   "    property string blah: Qt.platform;\n"
                                                   "}");

    {
        testCompiler.clearCache();
        QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));
        QVERIFY2(testCompiler.verify(), qPrintable(testCompiler.lastErrorString));
    }

    {
        testCompiler.clearCache();
        QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));

        testCompiler.tweakHeader([](QV4::CompiledData::Unit *header) {
            header->qtVersion = 0;
        });

        QVERIFY(!testCompiler.verify());
        QCOMPARE(testCompiler.lastErrorString, QString::fromUtf8("Qt version mismatch. Found 0 expected %1").arg(QT_VERSION, 0, 16));
        testCompiler.clearCache();
    }

    {
        testCompiler.clearCache();
        QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));

        testCompiler.tweakHeader([](QV4::CompiledData::Unit *header) {
            header->version = 0;
        });

        QVERIFY(!testCompiler.verify());
        QCOMPARE(testCompiler.lastErrorString, QString::fromUtf8("V4 data structure version mismatch. Found 0 expected %1").arg(QV4_DATA_STRUCTURE_VERSION, 0, 16));
    }
}

class TypeVersion1 : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged);
public:


    int m_value = 0;
    int value() const { return m_value; }
    void setValue(int v) { m_value = v; emit valueChanged(); }

signals:
    void valueChanged();
};

// Same as TypeVersion1 except the property type changed!
class TypeVersion2 : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged);
public:


    QString m_value;
    QString value() const { return m_value; }
    void setValue(QString v) { m_value = v; emit valueChanged(); }

signals:
    void valueChanged();
};

void tst_qmldiskcache::recompileAfterChange()
{
    QQmlEngine engine;

    TestCompiler testCompiler(&engine);
    QVERIFY(testCompiler.tempDir.isValid());

    const QByteArray contents = QByteArrayLiteral("import TypeTest 1.0\n"
                                                  "TypeThatWillChange {\n"
                                                   "}");

    qmlRegisterType<TypeVersion1>("TypeTest", 1, 0, "TypeThatWillChange");

    {
        testCompiler.clearCache();
        QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));
        QVERIFY2(testCompiler.verify(), qPrintable(testCompiler.lastErrorString));
    }

    QDateTime initialCacheTimeStamp = QFileInfo(testCompiler.cacheFilePath).lastModified();

    {
        CleanlyLoadingComponent component(&engine, testCompiler.testFilePath);
        QScopedPointer<TypeVersion1> obj(qobject_cast<TypeVersion1*>(component.create()));
        QVERIFY(!obj.isNull());
        QCOMPARE(QFileInfo(testCompiler.cacheFilePath).lastModified(), initialCacheTimeStamp);
    }

    engine.clearComponentCache();

    {
        CleanlyLoadingComponent component(&engine, testCompiler.testFilePath);
        QScopedPointer<TypeVersion1> obj(qobject_cast<TypeVersion1*>(component.create()));
        QVERIFY(!obj.isNull());
        QCOMPARE(QFileInfo(testCompiler.cacheFilePath).lastModified(), initialCacheTimeStamp);
    }

    engine.clearComponentCache();
    qmlClearTypeRegistrations();
    qmlRegisterType<TypeVersion2>("TypeTest", 1, 0, "TypeThatWillChange");

    waitForFileSystem();

    {
        CleanlyLoadingComponent component(&engine, testCompiler.testFilePath);
        QScopedPointer<TypeVersion2> obj(qobject_cast<TypeVersion2*>(component.create()));
        QVERIFY(!obj.isNull());
        QVERIFY(QFileInfo(testCompiler.cacheFilePath).lastModified() > initialCacheTimeStamp);
    }
}

void tst_qmldiskcache::recompileAfterDirectoryChange()
{
    QQmlEngine engine;
    TestCompiler testCompiler(&engine);

    QVERIFY(testCompiler.tempDir.isValid());

    QVERIFY(QDir(testCompiler.tempDir.path()).mkdir("source1"));
    testCompiler.init(testCompiler.tempDir.path() + QLatin1String("/source1"));

    {
        const QByteArray contents = QByteArrayLiteral("import QtQml 2.0\n"
                                                      "QtObject {\n"
                                                       "    property int blah: 42;\n"
                                                       "}");

        testCompiler.clearCache();
        QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));
        QVERIFY2(testCompiler.verify(), qPrintable(testCompiler.lastErrorString));
        const QV4::CompiledData::Unit *unit = testCompiler.mapUnit();
        QVERIFY(unit->sourceFileIndex != 0);
        const QString expectedPath = QUrl::fromLocalFile(testCompiler.testFilePath).toString();
        QCOMPARE(unit->stringAtInternal(unit->sourceFileIndex), expectedPath);
        testCompiler.closeMapping();
    }

    const QDateTime initialCacheTimeStamp = QFileInfo(testCompiler.cacheFilePath).lastModified();

    QDir(testCompiler.tempDir.path()).rename(QStringLiteral("source1"), QStringLiteral("source2"));
    waitForFileSystem();

    testCompiler.init(testCompiler.tempDir.path() + QLatin1String("/source2"));

    {
        CleanlyLoadingComponent component(&engine, testCompiler.testFilePath);
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("blah").toInt(), 42);
    }

    QFile cacheFile(testCompiler.cacheFilePath);
    QVERIFY2(cacheFile.exists(), qPrintable(cacheFile.fileName()));
    QVERIFY(QFileInfo(testCompiler.cacheFilePath).lastModified() > initialCacheTimeStamp);
}

void tst_qmldiskcache::fileSelectors()
{
    QQmlEngine engine;

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString testFilePath = tempDir.path() + "/test.qml";
    {
        QFile f(testFilePath);
        QVERIFY2(f.open(QIODevice::WriteOnly), qPrintable(f.errorString()));
        f.write(QByteArrayLiteral("import QtQml 2.0\nQtObject { property int value: 42 }"));
    }

    const QString selector = QStringLiteral("testSelector");
    const QString selectorPath = tempDir.path() + "/+" + selector;
    const QString selectedTestFilePath = selectorPath + "/test.qml";
    {
        QVERIFY(QDir::root().mkpath(selectorPath));
        QFile f(selectorPath + "/test.qml");
        QVERIFY2(f.open(QIODevice::WriteOnly), qPrintable(f.errorString()));
        f.write(QByteArrayLiteral("import QtQml 2.0\nQtObject { property int value: 100 }"));
    }

    {
        QQmlComponent component(&engine, testFilePath);
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("value").toInt(), 42);

        QFile cacheFile(QV4::CompiledData::CompilationUnit::localCacheFilePath(QUrl::fromLocalFile(testFilePath)));
        QVERIFY2(cacheFile.exists(), qPrintable(cacheFile.fileName()));
    }

    QQmlFileSelector qmlSelector(&engine);
    qmlSelector.setExtraSelectors(QStringList() << selector);

    engine.clearComponentCache();

    {
        QQmlComponent component(&engine, testFilePath);
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("value").toInt(), 100);

        QFile cacheFile(QV4::CompiledData::CompilationUnit::localCacheFilePath(QUrl::fromLocalFile(selectedTestFilePath)));
        QVERIFY2(cacheFile.exists(), qPrintable(cacheFile.fileName()));
    }
}

void tst_qmldiskcache::localAliases()
{
    QQmlEngine engine;

    TestCompiler testCompiler(&engine);
    QVERIFY(testCompiler.tempDir.isValid());

    const QByteArray contents = QByteArrayLiteral("import QtQml 2.0\n"
                                                  "QtObject {\n"
                                                  "    id: root\n"
                                                  "    property int prop: 100\n"
                                                  "    property alias dummy1: root.prop\n"
                                                  "    property alias dummy2: root.prop\n"
                                                  "    property alias dummy3: root.prop\n"
                                                  "    property alias dummy4: root.prop\n"
                                                  "    property alias dummy5: root.prop\n"
                                                  "    property alias foo: root.prop\n"
                                                  "    property alias bar: root.foo\n"
                                                   "}");

    {
        testCompiler.clearCache();
        QVERIFY2(testCompiler.compile(contents), qPrintable(testCompiler.lastErrorString));
        QVERIFY2(testCompiler.verify(), qPrintable(testCompiler.lastErrorString));
    }

    {
        CleanlyLoadingComponent component(&engine, testCompiler.testFilePath);
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("bar").toInt(), 100);
    }

    engine.clearComponentCache();

    {
        CleanlyLoadingComponent component(&engine, testCompiler.testFilePath);
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("bar").toInt(), 100);
    }
}

void tst_qmldiskcache::cacheResources()
{
    const QSet<QString> existingFiles =
        m_qmlCacheDirectory.entryList(QDir::Files | QDir::NoDotAndDotDot).toSet();

    QQmlEngine engine;

    {
        CleanlyLoadingComponent component(&engine, QUrl("qrc:/test.qml"));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("value").toInt(), 20);
    }

    const QSet<QString> entries =
        m_qmlCacheDirectory.entryList(QDir::NoDotAndDotDot | QDir::Files).toSet().subtract(existingFiles);
    QCOMPARE(entries.count(), 1);

    QDateTime cacheFileTimeStamp;

    {
        QFile cacheFile(m_qmlCacheDirectory.absoluteFilePath(*entries.cbegin()));
        QVERIFY2(cacheFile.open(QIODevice::ReadOnly), qPrintable(cacheFile.errorString()));
        QV4::CompiledData::Unit unit;
        QVERIFY(cacheFile.read(reinterpret_cast<char *>(&unit), sizeof(unit)) == sizeof(unit));

        cacheFileTimeStamp = QFileInfo(cacheFile.fileName()).lastModified();

        QDateTime referenceTimeStamp = QFileInfo(":/test.qml").lastModified();
        if (!referenceTimeStamp.isValid())
            referenceTimeStamp = QFileInfo(QCoreApplication::applicationFilePath()).lastModified();
        QCOMPARE(qint64(unit.sourceTimeStamp), referenceTimeStamp.toMSecsSinceEpoch());
    }

    waitForFileSystem();

    {
        CleanlyLoadingComponent component(&engine, QUrl("qrc:///test.qml"));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("value").toInt(), 20);
    }

    {
        const QSet<QString> entries =
            m_qmlCacheDirectory.entryList(QDir::NoDotAndDotDot | QDir::Files).toSet().subtract(existingFiles);
        QCOMPARE(entries.count(), 1);

        QCOMPARE(QFileInfo(m_qmlCacheDirectory.absoluteFilePath(*entries.cbegin())).lastModified().toMSecsSinceEpoch(),
                           cacheFileTimeStamp.toMSecsSinceEpoch());
    }
}

void tst_qmldiskcache::stableOrderOfDependentCompositeTypes()
{
    QQmlEngine engine;

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    {
        const QString depFilePath = tempDir.path() + "/FirstDependentType.qml";
        QFile f(depFilePath);
        QVERIFY2(f.open(QIODevice::WriteOnly), qPrintable(f.errorString()));
        f.write(QByteArrayLiteral("import QtQml 2.0\nQtObject { property int value: 42 }"));
    }

    {
        const QString depFilePath = tempDir.path() + "/SecondDependentType.qml";
        QFile f(depFilePath);
        QVERIFY2(f.open(QIODevice::WriteOnly), qPrintable(f.errorString()));
        f.write(QByteArrayLiteral("import QtQml 2.0\nQtObject { property int value: 100 }"));
    }

    const QString testFilePath = tempDir.path() + "/main.qml";
    {
        QFile f(testFilePath);
        QVERIFY2(f.open(QIODevice::WriteOnly), qPrintable(f.errorString()));
        f.write(QByteArrayLiteral("import QtQml 2.0\nQtObject {\n"
                                  "    property QtObject dep1: FirstDependentType{}\n"
                                  "    property QtObject dep2: SecondDependentType{}\n"
                                  "    property int value: dep1.value + dep2.value\n"
                                  "}"));
    }

    QByteArray firstDependentTypeClassName;
    QByteArray secondDependentTypeClassName;

    {
        CleanlyLoadingComponent component(&engine, QUrl::fromLocalFile(testFilePath));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("value").toInt(), 142);

        firstDependentTypeClassName = qvariant_cast<QObject *>(obj->property("dep1"))->metaObject()->className();
        secondDependentTypeClassName = qvariant_cast<QObject *>(obj->property("dep2"))->metaObject()->className();
    }

    QVERIFY(firstDependentTypeClassName != secondDependentTypeClassName);
    QVERIFY2(firstDependentTypeClassName.contains("QMLTYPE"), firstDependentTypeClassName.constData());
    QVERIFY2(secondDependentTypeClassName.contains("QMLTYPE"), secondDependentTypeClassName.constData());

    const QString testFileCachePath = QV4::CompiledData::CompilationUnit::localCacheFilePath(QUrl::fromLocalFile(testFilePath));
    QVERIFY(QFile::exists(testFileCachePath));
    QDateTime initialCacheTimeStamp = QFileInfo(testFileCachePath).lastModified();

    engine.clearComponentCache();
    waitForFileSystem();

    // Creating the test component a second time should load it from the cache (same time stamp),
    // despite the class names of the dependent composite types differing.
    {
        CleanlyLoadingComponent component(&engine, QUrl::fromLocalFile(testFilePath));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("value").toInt(), 142);

        QVERIFY(qvariant_cast<QObject *>(obj->property("dep1"))->metaObject()->className() != firstDependentTypeClassName);
        QVERIFY(qvariant_cast<QObject *>(obj->property("dep2"))->metaObject()->className() != secondDependentTypeClassName);
    }

    {
        QVERIFY(QFile::exists(testFileCachePath));
        QDateTime newCacheTimeStamp = QFileInfo(testFileCachePath).lastModified();
        QCOMPARE(newCacheTimeStamp, initialCacheTimeStamp);
    }

    // Now change the first dependent QML type and see if we correctly re-generate the
    // caches.
    engine.clearComponentCache();
    waitForFileSystem();
    {
        const QString depFilePath = tempDir.path() + "/FirstDependentType.qml";
        QFile f(depFilePath);
        QVERIFY2(f.open(QIODevice::WriteOnly), qPrintable(f.errorString()));
        f.write(QByteArrayLiteral("import QtQml 2.0\nQtObject { property int value: 40 }"));
    }

    {
        CleanlyLoadingComponent component(&engine, QUrl::fromLocalFile(testFilePath));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("value").toInt(), 140);
    }

    {
        QVERIFY(QFile::exists(testFileCachePath));
        QDateTime newCacheTimeStamp = QFileInfo(testFileCachePath).lastModified();
        QVERIFY2(newCacheTimeStamp > initialCacheTimeStamp, qPrintable(newCacheTimeStamp.toString()));
    }
}

void tst_qmldiskcache::singletonDependency()
{
    QScopedPointer<QQmlEngine> engine(new QQmlEngine);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const auto writeTempFile = [&tempDir](const QString &fileName, const char *contents) {
        QFile f(tempDir.path() + '/' + fileName);
        const bool ok = f.open(QIODevice::WriteOnly | QIODevice::Truncate);
        Q_ASSERT(ok);
        f.write(contents);
        return f.fileName();
    };

    writeTempFile("MySingleton.qml", "import QtQml 2.0\npragma Singleton\nQtObject { property int value: 42 }");
    writeTempFile("qmldir", "singleton MySingleton 1.0 MySingleton.qml");
    const QString testFilePath = writeTempFile("main.qml", "import QtQml 2.0\nimport \".\"\nQtObject {\n"
                                                           "    property int value: MySingleton.value\n"
                                                           "}");

    {
        CleanlyLoadingComponent component(engine.data(), QUrl::fromLocalFile(testFilePath));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("value").toInt(), 42);
    }

    const QString testFileCachePath = QV4::CompiledData::CompilationUnit::localCacheFilePath(QUrl::fromLocalFile(testFilePath));
    QVERIFY(QFile::exists(testFileCachePath));
    QDateTime initialCacheTimeStamp = QFileInfo(testFileCachePath).lastModified();

    engine.reset(new QQmlEngine);
    waitForFileSystem();

    writeTempFile("MySingleton.qml", "import QtQml 2.0\npragma Singleton\nQtObject { property int value: 100 }");
    waitForFileSystem();

    {
        CleanlyLoadingComponent component(engine.data(), QUrl::fromLocalFile(testFilePath));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QCOMPARE(obj->property("value").toInt(), 100);
    }

    {
        QVERIFY(QFile::exists(testFileCachePath));
        QDateTime newCacheTimeStamp = QFileInfo(testFileCachePath).lastModified();
        QVERIFY2(newCacheTimeStamp > initialCacheTimeStamp, qPrintable(newCacheTimeStamp.toString()));
    }
}

void tst_qmldiskcache::cppRegisteredSingletonDependency()
{
    qmlClearTypeRegistrations();
    QScopedPointer<QQmlEngine> engine(new QQmlEngine);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const auto writeTempFile = [&tempDir](const QString &fileName, const char *contents) {
        QFile f(tempDir.path() + '/' + fileName);
        const bool ok = f.open(QIODevice::WriteOnly | QIODevice::Truncate);
        Q_ASSERT(ok);
        f.write(contents);
        return f.fileName();
    };

    writeTempFile("MySingleton.qml", "import QtQml 2.0\npragma Singleton\nQtObject { property int value: 42 }");

    qmlRegisterSingletonType(QUrl::fromLocalFile(tempDir.path() + QLatin1String("/MySingleton.qml")), "CppRegisteredSingletonDependency", 1, 0, "Singly");

    const QString testFilePath = writeTempFile("main.qml", "import QtQml 2.0\nimport CppRegisteredSingletonDependency 1.0\nQtObject {\n"
                                                           "    function getValue() { return Singly.value; }\n"
                                                           "}");

    {
        CleanlyLoadingComponent component(engine.data(), QUrl::fromLocalFile(testFilePath));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QVariant value;
        QVERIFY(QMetaObject::invokeMethod(obj.data(), "getValue", Q_RETURN_ARG(QVariant, value)));
        QCOMPARE(value.toInt(), 42);
    }

    const QString testFileCachePath = QV4::CompiledData::CompilationUnit::localCacheFilePath(QUrl::fromLocalFile(testFilePath));
    QVERIFY(QFile::exists(testFileCachePath));
    QDateTime initialCacheTimeStamp = QFileInfo(testFileCachePath).lastModified();

    engine.reset(new QQmlEngine);
    waitForFileSystem();

    writeTempFile("MySingleton.qml", "import QtQml 2.0\npragma Singleton\nQtObject { property int value: 100 }");
    waitForFileSystem();

    {
        CleanlyLoadingComponent component(engine.data(), QUrl::fromLocalFile(testFilePath));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());

        {
            QVERIFY(QFile::exists(testFileCachePath));
            QDateTime newCacheTimeStamp = QFileInfo(testFileCachePath).lastModified();
            QVERIFY2(newCacheTimeStamp > initialCacheTimeStamp, qPrintable(newCacheTimeStamp.toString()));
        }

        QVariant value;
        QVERIFY(QMetaObject::invokeMethod(obj.data(), "getValue", Q_RETURN_ARG(QVariant, value)));
        QCOMPARE(value.toInt(), 100);
    }
}

void tst_qmldiskcache::cacheModuleScripts()
{
    const QSet<QString> existingFiles =
        m_qmlCacheDirectory.entryList(QDir::Files | QDir::NoDotAndDotDot).toSet();

    QQmlEngine engine;

    {
        CleanlyLoadingComponent component(&engine, QUrl("qrc:/importmodule.qml"));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        QVERIFY(obj->property("ok").toBool());

        auto componentPrivate = QQmlComponentPrivate::get(&component);
        QVERIFY(componentPrivate);
        auto compilationUnit = componentPrivate->compilationUnit->dependentScripts.first()->compilationUnit();
        QVERIFY(compilationUnit);
        auto unitData = compilationUnit->unitData();
        QVERIFY(unitData);
        QVERIFY(unitData->flags & QV4::CompiledData::Unit::StaticData);
        QVERIFY(unitData->flags & QV4::CompiledData::Unit::IsESModule);
        QVERIFY(!compilationUnit->backingFile.isNull());
    }

    const QSet<QString> entries =
         m_qmlCacheDirectory.entryList(QStringList("*.mjsc")).toSet().subtract(existingFiles);

    QCOMPARE(entries.count(), 1);

    QDateTime cacheFileTimeStamp;

    {
        QFile cacheFile(m_qmlCacheDirectory.absoluteFilePath(*entries.cbegin()));
        QVERIFY2(cacheFile.open(QIODevice::ReadOnly), qPrintable(cacheFile.errorString()));
        QV4::CompiledData::Unit unit;
        QVERIFY(cacheFile.read(reinterpret_cast<char *>(&unit), sizeof(unit)) == sizeof(unit));

        QVERIFY(unit.flags & QV4::CompiledData::Unit::IsESModule);
    }
}

QTEST_MAIN(tst_qmldiskcache)

#include "tst_qmldiskcache.moc"
