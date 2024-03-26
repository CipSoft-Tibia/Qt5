// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qbaselinetest.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDirIterator>
#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtGui/QImage>

#include <algorithm>

QString blockify(const QByteArray& s)
{
    const char* indent = "\n | ";
    QByteArray block = s.trimmed();
    block.replace('\n', indent);
    block.prepend(indent);
    block.append('\n');
    return block;
}

//#define TEXTLESS_TEST

class tst_Scenegraph : public QObject
{
    Q_OBJECT

public:
    tst_Scenegraph();

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();
#ifdef TEXTLESS_TEST
    void testNoTextRendering_data();
    void testNoTextRendering();
#endif
    void testRendering_data();
    void testRendering();

private:
    void setupTestSuite(const QByteArray& filter = QByteArray());
    void runTest(const QStringList& extraArgs = QStringList());
    bool renderAndGrab(const QString& qmlFile, const QStringList& extraArgs, QImage *screenshot, QString *errMsg);
    quint16 checksumFileOrDir(const QString &path);

    QString testSuitePath;
    QString grabberPath;
    int consecutiveErrors;   // Not test failures (image mismatches), but system failures (so no image at all)
    bool aborted;            // This run given up because of too many system failures
};


tst_Scenegraph::tst_Scenegraph()
    : consecutiveErrors(0), aborted(false)
{
}


void tst_Scenegraph::initTestCase()
{
    QString dataDir = QFINDTESTDATA("../data/.");
    if (dataDir.isEmpty())
        dataDir = QStringLiteral("data");
    QFileInfo fi(dataDir);
    if (!fi.exists() || !fi.isDir() || !fi.isReadable())
        QSKIP("Test suite data directory missing or unreadable: " + fi.canonicalFilePath().toLatin1());
    testSuitePath = fi.canonicalFilePath();

#if defined(Q_OS_WIN)
    grabberPath = QFINDTESTDATA("qmlscenegrabber.exe");
#elif defined(Q_OS_DARWIN)
    grabberPath = QFINDTESTDATA("qmlscenegrabber.app/Contents/MacOS/qmlscenegrabber");
#else
    grabberPath = QFINDTESTDATA("qmlscenegrabber");
#endif
    if (grabberPath.isEmpty())
        grabberPath = QCoreApplication::applicationDirPath() + "/qmlscenegrabber";

    const char *backendVarName = "QT_QUICK_BACKEND";
    const QString backend = qEnvironmentVariable(backendVarName, QString::fromLatin1("default"));
    QBaselineTest::addClientProperty(QString::fromLatin1(backendVarName), backend);

    QString stack = backend;
    if (stack != QLatin1String("software")) {
#if defined(Q_OS_WIN)
        const char *defaultRhiBackend = "d3d11";
#elif defined(Q_OS_DARWIN)
        const char *defaultRhiBackend = "metal";
#else
        const char *defaultRhiBackend = "opengl";
#endif
        const QString rhiBackend = qEnvironmentVariable("QSG_RHI_BACKEND", QString::fromLatin1(defaultRhiBackend));
        stack = QString::fromLatin1("RHI_%1").arg(rhiBackend);
    }
    QBaselineTest::addClientProperty(QString::fromLatin1("GraphicsStack"), stack);

    QByteArray msg;
    if (!QBaselineTest::connectToBaselineServer(&msg))
        QSKIP(msg);
}

void tst_Scenegraph::init()
{
    // This gets called for every row. QSKIP if current item is blacklisted on the baseline server:
    QBASELINE_SKIP_IF_BLACKLISTED;
}

void tst_Scenegraph::cleanup()
{
    // Allow subsystems time to settle
    if (!aborted)
        QTest::qWait(20);
}

#ifdef TEXTLESS_TEST
void tst_Scenegraph::testNoTextRendering_data()
{
    setupTestSuite("text/");
    consecutiveErrors = 0;
    aborted = false;
}


void tst_Scenegraph::testNoTextRendering()
{
    runTest(QStringList() << "-notext");
}
#endif

void tst_Scenegraph::testRendering_data()
{
    setupTestSuite();
    consecutiveErrors = 0;
    aborted = false;
}


void tst_Scenegraph::testRendering()
{
    runTest();
}


void tst_Scenegraph::setupTestSuite(const QByteArray& filter)
{
    QTest::addColumn<QString>("qmlFile");
    int numItems = 0;

    QStringList ignoreItems;
    QFile ignoreFile(testSuitePath + "/Ignore");
    if (ignoreFile.open(QIODevice::ReadOnly)) {
        while (!ignoreFile.atEnd()) {
            QByteArray line = ignoreFile.readLine().trimmed();
            if (!line.isEmpty() && !line.startsWith('#'))
                ignoreItems += line;
        }
    }

    QStringList itemFiles;
    QDirIterator it(testSuitePath, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString fp = it.next();
        if (fp.endsWith(".qml")) {
            QString itemName = fp.mid(testSuitePath.size() + 1);
            if (!ignoreItems.contains(itemName) && (filter.isEmpty() || !itemName.startsWith(filter)))
                itemFiles.append(it.filePath());
        }
    }

    std::sort(itemFiles.begin(), itemFiles.end());
    for (const QString &filePath : std::as_const(itemFiles)) {
        QByteArray itemName = filePath.mid(testSuitePath.size() + 1).toLatin1();
        QBaselineTest::newRow(itemName, checksumFileOrDir(filePath)) << filePath;
        numItems++;
    }

    if (!numItems)
        QSKIP("No .qml test files found in " + testSuitePath.toLatin1());
}


void tst_Scenegraph::runTest(const QStringList& extraArgs)
{
    // qDebug() << "Rendering" << QTest::currentDataTag();

    if (aborted)
        QSKIP("System too unstable.");

    QFETCH(QString, qmlFile);

    QImage screenShot;
    QString errorMessage;
    if (renderAndGrab(qmlFile, extraArgs, &screenShot, &errorMessage)) {
        consecutiveErrors = 0;
    }
    else {
        if (++consecutiveErrors >= 3 && QBaselineTest::shouldAbortIfUnstable())
            aborted = true;                   // Just give up if screen grabbing fails 3 times in a row
        QFAIL(qPrintable("QuickView grabbing failed: " + errorMessage));
    }

    QBASELINE_TEST(screenShot);
}


bool tst_Scenegraph::renderAndGrab(const QString& qmlFile, const QStringList& extraArgs, QImage *screenshot, QString *errMsg)
{
    bool usePipe = true;  // Whether to transport the grabbed image using temp. file or pipe. TBD: cmdline option
    QProcess grabber;
    grabber.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    QStringList args = extraArgs;
    QString tmpfile = usePipe ? QString("-") : QString("/tmp/qmlscenegrabber-%1-out.ppm").arg(QCoreApplication::applicationPid());
    args << qmlFile << "-o" << tmpfile;
    grabber.start(grabberPath, args, QIODevice::ReadOnly);
    grabber.waitForFinished(17000);         //### hardcoded, must be larger than the scene timeout in qmlscenegrabber
    if (grabber.state() != QProcess::NotRunning) {
        grabber.terminate();
        grabber.waitForFinished(3000);
    }
    QImage img;
    bool res = usePipe ? img.load(&grabber, "ppm") : img.load(tmpfile);
    if (!res || img.isNull()) {
        if (errMsg) {
            QString s("Failed to grab screen. qmlscenegrabber exitcode: %1. Process error: %2. Stderr:%3");
            *errMsg = s.arg(grabber.exitCode()).arg(grabber.errorString()).arg(blockify(grabber.readAllStandardError()));
        }
        if (!usePipe)
            QFile::remove(tmpfile);
        return false;
    }
    if (screenshot)
        *screenshot = img;
    if (!usePipe)
        QFile::remove(tmpfile);
    return true;
}


quint16 tst_Scenegraph::checksumFileOrDir(const QString &path)
{
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isReadable())
        return 0;
    if (fi.isFile()) {
        QFile f(path);
        f.open(QIODevice::ReadOnly);
        QByteArray contents = f.readAll();
        return qChecksum(contents);
    }
    if (fi.isDir()) {
        static const QStringList nameFilters = QStringList() << "*.qml" << "*.cpp" << "*.png" << "*.jpg";
        quint16 cs = 0;
        const auto entryList = QDir(fi.filePath()).entryList(nameFilters,
                                                             QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
        for (const QString &item : entryList)
            cs ^= checksumFileOrDir(path + QLatin1Char('/') + item);
        return cs;
    }
    return 0;
}


#define main _realmain
QTEST_MAIN(tst_Scenegraph)
#undef main

int main(int argc, char *argv[])
{
    QBaselineTest::handleCmdLineArgs(&argc, &argv);
    return _realmain(argc, argv);
}

#include "tst_baseline_scenegraph.moc"
