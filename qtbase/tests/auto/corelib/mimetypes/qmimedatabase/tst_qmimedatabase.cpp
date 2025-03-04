// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qmimedatabase.h"
#include <qmimedatabase.h>

#include "qstandardpaths.h"

#ifdef Q_OS_UNIX
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/qspan.h>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/private/qduplicatetracker_p.h>

#include <QTest>
#include <QBuffer>
#include <QTemporaryFile>
#if QT_CONFIG(process)
#include <QProcess>
#endif

using namespace Qt::StringLiterals;

static const std::array additionalGlobalMimeFiles = {
    "yast2-metapackage-handler-mimetypes.xml",
    "qml-again.xml",
    "magic-and-hierarchy.xml",
};

static const std::array additionalLocalMimeFiles = {
    "add-extension.xml", // adds *.jnewext to image/jpeg
    "yast2-metapackage-handler-mimetypes.xml",
    "qml-again.xml",
    "text-x-objcsrc.xml",
    "text-plain-subclass.xml",
    "invalid-magic1.xml",
    "invalid-magic2.xml",
    "invalid-magic3.xml",
    "magic-and-hierarchy.xml",
    "circular-inheritance.xml",
    "webm-glob-deleteall.xml",
};

static const auto s_additionalFilesResourcePrefix = ":/tst_qmimedatabase/qmime/"_L1;
static const auto s_resourcePrefix = ":/qt-project.org/qmime/"_L1;
static const auto s_inodeMimetype = "inode/directory"_L1;

void initializeLang()
{
    qputenv("LC_ALL", "");
    qputenv("LANG", "C");
    QCoreApplication::setApplicationName("tst_qmimedatabase"); // temporary directory pattern
}

static inline QString testSuiteWarning()
{

    QString result;
    QTextStream str(&result);
    str << "\nCannot find the shared-mime-info test suite\nin the parent of: "
        << QDir::toNativeSeparators(QDir::currentPath()) << "\n"
           "cd " << QDir::toNativeSeparators(QStringLiteral("tests/auto/corelib/mimetypes/qmimedatabase")) << "\n"
           "wget https://gitlab.freedesktop.org/xdg/shared-mime-info/-/archive/2.2/shared-mime-info-2.2.zip\n"
           "unzip shared-mime-info-2.2.zip\n";
#ifdef Q_OS_WIN
    str << "mkdir testfiles\nxcopy /s shared-mime-info-2.2 s-m-i\n";
#else
    str << "ln -s shared-mime-info-2.2 s-m-i\n";
#endif
    return result;
}

static bool copyResourceFile(const QString &sourceFileName, const QString &targetFileName,
                             QString *errorMessage)
{

    QFile sourceFile(sourceFileName);
    if (!sourceFile.exists()) {
        *errorMessage = QDir::toNativeSeparators(sourceFileName) + QLatin1String(" does not exist.");
        return false;
    }
    if (!sourceFile.copy(targetFileName)) {
        *errorMessage = QLatin1String("Cannot copy ")
            + QDir::toNativeSeparators(sourceFileName) + QLatin1String(" to ")
            + QDir::toNativeSeparators(targetFileName) + QLatin1String(": ")
            + sourceFile.errorString();
        return false;
    }
    // QFile::copy() sets the permissions of the source file which are read-only for
    // resource files. Set write permission to enable deletion of the temporary directory.
    QFile targetFile(targetFileName);
    if (!targetFile.setPermissions(targetFile.permissions() | QFileDevice::WriteUser)) {
        *errorMessage = QLatin1String("Cannot set write permission on ")
            + QDir::toNativeSeparators(targetFileName) + QLatin1String(": ")
            + targetFile.errorString();
        return false;
    }
    return true;
}

// Set LANG before QCoreApplication is created
Q_CONSTRUCTOR_FUNCTION(initializeLang)

static QString seedAndTemplate()
{
    return QDir::tempPath() + "/tst_qmimedatabase-XXXXXX";
}

tst_QMimeDatabase::tst_QMimeDatabase()
    : m_temporaryDir(seedAndTemplate())
{
}

void tst_QMimeDatabase::initTestCase()
{
    QLocale::setDefault(QLocale::c());
    QVERIFY2(m_temporaryDir.isValid(), qPrintable(m_temporaryDir.errorString()));
    QStandardPaths::setTestModeEnabled(true);
    m_localMimeDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/mime";
    if (QDir(m_localMimeDir).exists()) {
        QVERIFY2(QDir(m_localMimeDir).removeRecursively(), qPrintable(m_localMimeDir + ": " + qt_error_string()));
    }

    m_isUsingCacheProvider = useCacheProvider();
    m_hasFreedesktopOrg = useFreeDesktopOrgXml();

#ifdef USE_XDG_DATA_DIRS
    // Create a temporary "global" XDG data dir. It's used
    // 1) to install new global mimetypes later on
    // 2) to run update-mime-database right away when testing the cache provider
    QVERIFY2(m_temporaryDir.isValid(),
             ("Could not create temporary subdir: " + m_temporaryDir.errorString()).toUtf8());
    const QDir here = QDir(m_temporaryDir.path());
    m_globalXdgDir = m_temporaryDir.path() + QStringLiteral("/global");
    const QString globalPackageDir = m_globalXdgDir + QStringLiteral("/mime/packages");
    QVERIFY(here.mkpath(globalPackageDir));

    qputenv("XDG_DATA_DIRS", QFile::encodeName(m_globalXdgDir));
    qDebug() << "\nGlobal XDG_DATA_DIRS: " << m_globalXdgDir;

    if (m_isUsingCacheProvider) {
        const QString xmlFileName = m_hasFreedesktopOrg
                ? (s_additionalFilesResourcePrefix + "/freedesktop.org.xml"_L1)
                : (s_resourcePrefix + "/tika/packages/tika-mimetypes.xml"_L1);
        QVERIFY2(QFileInfo::exists(xmlFileName), qPrintable(xmlFileName));
        const QString xmlTargetFileName =
                globalPackageDir + '/' + QFileInfo(xmlFileName).fileName();
        QString errorMessage;
        QVERIFY2(copyResourceFile(xmlFileName, xmlTargetFileName, &errorMessage),
                 qPrintable(errorMessage));
    }
#endif

    if (m_hasFreedesktopOrg) {
        m_testSuite = QFINDTESTDATA("../s-m-i/tests/mime-detection");
        if (m_testSuite.isEmpty()) {
            qWarning().noquote() << testSuiteWarning();
        }
    }

    initTestCaseInternal();
}

void tst_QMimeDatabase::init()
{
    // clean up local data from previous runs
    QDir(m_localMimeDir).removeRecursively();
}

void tst_QMimeDatabase::cleanupTestCase()
{
    QDir(m_localMimeDir).removeRecursively();
}

void tst_QMimeDatabase::mimeTypeForName()
{
    QMimeDatabase db;
    QMimeType s0 = db.mimeTypeForName(QString::fromLatin1("application/x-zerosize"));
    QVERIFY(s0.isValid());
    QCOMPARE(s0.name(), QString::fromLatin1("application/x-zerosize"));
    QCOMPARE(s0.comment(), QString::fromLatin1("Empty document"));

    QMimeType s0Again = db.mimeTypeForName(QString::fromLatin1("application/x-zerosize"));
    QCOMPARE(s0Again.name(), s0.name());

    QMimeType s1 = db.mimeTypeForName(QString::fromLatin1("text/plain"));
    QVERIFY(s1.isValid());
    QCOMPARE(s1.name(), QString::fromLatin1("text/plain"));
    //qDebug("Comment is %s", qPrintable(s1.comment()));

    QMimeType cbor = db.mimeTypeForName(QString::fromLatin1("application/cbor"));
    QVERIFY(cbor.isValid());

    // Test <comment> parsing with application/rdf+xml which has the english comment after the other ones
    QMimeType rdf = db.mimeTypeForName(QString::fromLatin1("application/rdf+xml"));
    QVERIFY(rdf.isValid());
    QVERIFY(rdf.comment() == QLatin1String("RDF file")
            || rdf.comment() == QLatin1String("XML syntax for RDF graphs") /*tika*/);

    QMimeType bzip2 = db.mimeTypeForName(QString::fromLatin1("application/x-bzip2"));
    QVERIFY(bzip2.isValid());
    QVERIFY(bzip2.comment() == QLatin1String("Bzip2 archive")
            || bzip2.comment() == QLatin1String("Bzip 2 UNIX Compressed File") /*tika*/);

    QMimeType defaultMime = db.mimeTypeForName(QString::fromLatin1("application/octet-stream"));
    QVERIFY(defaultMime.isValid());
    QVERIFY(defaultMime.isDefault());

    QMimeType doesNotExist = db.mimeTypeForName(QString::fromLatin1("foobar/x-doesnot-exist"));
    QVERIFY(!doesNotExist.isValid());
    QCOMPARE(doesNotExist.comment(), QString());
    QCOMPARE(doesNotExist.aliases(), QStringList());

#ifdef Q_OS_LINUX
    if (m_hasFreedesktopOrg) {
        QString exePath = QStandardPaths::findExecutable(QLatin1String("ls"));
        if (exePath.isEmpty())
            qWarning() << "ls not found";
        else {
            const QString executableType = QString::fromLatin1("application/x-executable");
            const QString sharedLibType = QString::fromLatin1("application/x-sharedlib");
            QVERIFY(db.mimeTypeForFile(exePath).name() == executableType
                    || db.mimeTypeForFile(exePath).name() == sharedLibType);
        }
    }
#endif

}

void tst_QMimeDatabase::mimeTypeForFileName_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("expectedMimeType");

    QTest::newRow("text") << "textfile.txt" << "text/plain";
    QTest::newRow("case-insensitive search") << "textfile.TxT" << "text/plain";

    // Needs shared-mime-info > 0.91. Earlier versions wrote .Z to the mime.cache file...
    //QTest::newRow("case-insensitive match on a non-lowercase glob") << "foo.z" << "application/x-compress";

    QTest::newRow("case-sensitive uppercase match") << "textfile.C" << "text/x-c++src";
    QTest::newRow("case-sensitive lowercase match") << "textfile.c" << "text/x-csrc";
    QTest::newRow("case-sensitive long-extension match") << "foo.PS.gz" << "application/x-gzpostscript";
    QTest::newRow("case-sensitive-only-match-core") << "core" << "application/x-core";
    QTest::newRow("case-sensitive-only-match-Core") << "Core" << "application/octet-stream"; // #198477

    QTest::newRow("desktop file") << "foo.desktop"
                                  << "application/x-desktop";
    QTest::newRow("double-extension file") << "foo.tar.bz2"
                                           << "application/x-bzip2-compressed-tar";
    QTest::newRow("single-extension file") << "foo.bz2"
                                           << "application/x-bzip2";
    QTest::newRow(".doc should assume msword") << "somefile.doc" << "application/msword"; // #204139
    QTest::newRow("glob that uses [] syntax, 1") << "Makefile" << "text/x-makefile";
    QTest::newRow("glob that uses [] syntax, 2") << "makefile" << "text/x-makefile";
    if (m_hasFreedesktopOrg) {
        QTest::newRow("glob that ends with *, no extension") << "README"
                                                             << "text/x-readme";
        QTest::newRow("glob that ends with *, extension") << "README.foo"
                                                          << "text/x-readme";
        QTest::newRow("glob that ends with *, also matches *.txt. Higher weight wins.")
                << "README.txt"
                << "text/plain";
        QTest::newRow("glob that ends with *, also matches *.nfo. Higher weight wins.")
                << "README.nfo"
                << "text/x-nfo";
        // fdo bug 15436, needs shared-mime-info >= 0.40 (and this tests the globs2-parsing code).
        QTest::newRow("glob that ends with *, also matches *.pdf. *.pdf has higher weight")
                << "README.pdf"
                << "application/pdf";
    }
    QTest::newRow("directory") << "/" << "inode/directory";
    QTest::newRow("resource-directory") << ":/files/" << "inode/directory";
    QTest::newRow("doesn't exist, no extension") << "IDontExist" << "application/octet-stream";
    QTest::newRow("doesn't exist but has known extension") << "IDontExist.txt" << "text/plain";
    QTest::newRow("empty") << "" << "application/octet-stream";
}

static inline QByteArray msgMimeTypeForFileNameFailed(const QList<QMimeType> &actual,
                                                      const QString &expected)
{
    QByteArray result = "Actual (";
    for (const QMimeType &m : actual) {
        result += m.name().toLocal8Bit();
        result +=  ' ';
    }
    result +=  ") , expected: ";
    result +=  expected.toLocal8Bit();
    return result;
}

void tst_QMimeDatabase::mimeTypeForFileName()
{
    QFETCH(QString, fileName);
    QFETCH(QString, expectedMimeType);
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(fileName, QMimeDatabase::MatchExtension);
    QVERIFY(mime.isValid());
    QCOMPARE(mime.name(), expectedMimeType);

    const QList<QMimeType> mimes = db.mimeTypesForFileName(fileName);
    if (expectedMimeType == "application/octet-stream") {
        QVERIFY(mimes.isEmpty());
    } else {
        QVERIFY2(!mimes.isEmpty(), msgMimeTypeForFileNameFailed(mimes, expectedMimeType).constData());
        QVERIFY2(mimes.size() == 1, msgMimeTypeForFileNameFailed(mimes, expectedMimeType).constData());
        QCOMPARE(mimes.first().name(), expectedMimeType);
    }
}

void tst_QMimeDatabase::mimeTypesForFileName_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QStringList>("expectedMimeTypes");

    QTest::newRow("txt, 1 hit") << "foo.txt" << (QStringList() << "text/plain");
    QTest::newRow("txtfoobar, 0 hit") << "foo.foobar" << QStringList();
    QTest::newRow("m, 2 hits") << "foo.m" << (QStringList() << "text/x-matlab" << "text/x-objcsrc");
    if (m_hasFreedesktopOrg)
        QTest::newRow("sub, 3 hits") << "foo.sub"
                                     << (QStringList() << "text/x-microdvd"
                                                       << "text/x-mpsub"
                                                       << "text/x-subviewer");
    QTest::newRow("non_ascii") << QString::fromUtf8("AİİA.pdf") << (QStringList() << "application/pdf");
}

static QStringList mimeTypeNames(const QList<QMimeType> &mimes)
{
    QStringList mimeNames;
    mimeNames.reserve(mimes.size());
    for (const auto &mime : mimes)
        mimeNames.append(mime.name());
    return mimeNames;
}

void tst_QMimeDatabase::mimeTypesForFileName()
{
    QFETCH(QString, fileName);
    QFETCH(QStringList, expectedMimeTypes);
    QMimeDatabase db;
    QList<QMimeType> mimes = db.mimeTypesForFileName(fileName);
    QStringList mimeNames = mimeTypeNames(mimes);
    QCOMPARE(mimeNames, expectedMimeTypes);
}

void tst_QMimeDatabase::inheritance()
{
    QMimeDatabase db;

    // All file-like mimetypes inherit from octet-stream
    const QMimeType wordperfect = db.mimeTypeForName(QString::fromLatin1("application/vnd.wordperfect"));
    QVERIFY(wordperfect.isValid());
    QCOMPARE(wordperfect.parentMimeTypes().join(QString::fromLatin1(",")), QString::fromLatin1("application/octet-stream"));
    QVERIFY(wordperfect.inherits(QLatin1String("application/octet-stream")));

    if (m_hasFreedesktopOrg) {
        QVERIFY(db.mimeTypeForName(QString::fromLatin1("image/svg+xml-compressed"))
                        .inherits(QLatin1String("application/x-gzip")));

        // Check that msword derives from ole-storage
        const QMimeType msword = db.mimeTypeForName(QString::fromLatin1("application/msword"));
        QVERIFY(msword.isValid());
        const QMimeType olestorage =
                db.mimeTypeForName(QString::fromLatin1("application/x-ole-storage"));
        QVERIFY(olestorage.isValid());
        QVERIFY(msword.inherits(olestorage.name()));
        QVERIFY(msword.inherits(QLatin1String("application/octet-stream")));
    }

    const QMimeType directory = db.mimeTypeForName(s_inodeMimetype);
    QVERIFY(directory.isValid());
    QCOMPARE(directory.parentMimeTypes().size(), 0);
    QVERIFY(!directory.inherits(QLatin1String("application/octet-stream")));

    // Check that text/x-patch knows that it inherits from text/plain (it says so explicitly)
    const QMimeType plain = db.mimeTypeForName(QString::fromLatin1("text/plain"));
    const QMimeType derived = db.mimeTypeForName(QString::fromLatin1("text/x-patch"));
    QVERIFY(derived.isValid());
    QCOMPARE(derived.parentMimeTypes().join(QString::fromLatin1(",")), plain.name());
    QVERIFY(derived.inherits(QLatin1String("text/plain")));
    QVERIFY(derived.inherits(QLatin1String("application/octet-stream")));

    // Check that application/x-shellscript inherits from application/x-executable
    // (Otherwise KRun cannot start shellscripts...)
    // This is a test for multiple inheritance...
    const QMimeType shellscript = db.mimeTypeForName(QString::fromLatin1("application/x-shellscript"));
    QVERIFY(shellscript.isValid());
    QVERIFY(shellscript.inherits(QLatin1String("text/plain")));
    QVERIFY(shellscript.inherits(QLatin1String("application/x-executable")));
    const QStringList shellParents = shellscript.parentMimeTypes();
    QVERIFY(shellParents.contains(QLatin1String("text/plain")));
    QVERIFY(shellParents.contains(QLatin1String("application/x-executable")));
    QCOMPARE(shellParents.size(), 2); // only the above two
    const QStringList allShellAncestors = shellscript.allAncestors();
    QVERIFY(allShellAncestors.contains(QLatin1String("text/plain")));
    QVERIFY(allShellAncestors.contains(QLatin1String("application/x-executable")));
    QVERIFY(allShellAncestors.contains(QLatin1String("application/octet-stream")));
    // Must be least-specific last, i.e. breadth first.
    QCOMPARE(allShellAncestors.last(), QString::fromLatin1("application/octet-stream"));

    const QStringList allSvgAncestors = db.mimeTypeForName(QString::fromLatin1("image/svg+xml")).allAncestors();
    QCOMPARE(allSvgAncestors, QStringList() << QLatin1String("application/xml") << QLatin1String("text/plain") << QLatin1String("application/octet-stream"));

    // Check that text/x-mrml knows that it inherits from text/plain (implicitly)
    const QMimeType mrml = db.mimeTypeForName(QString::fromLatin1(
            m_hasFreedesktopOrg ? "text/x-mrml" : "text/vnd.trolltech.linguist"));
    QVERIFY(mrml.isValid());
    QVERIFY(mrml.inherits(QLatin1String("text/plain")));
    QVERIFY(mrml.inherits(QLatin1String("application/octet-stream")));

    if (m_hasFreedesktopOrg) {
        // Check that msword-template inherits msword
        const QMimeType mswordTemplate =
                db.mimeTypeForName(QString::fromLatin1("application/msword-template"));
        QVERIFY(mswordTemplate.isValid());
        QVERIFY(mswordTemplate.inherits(QLatin1String("application/msword")));

        // Check that buggy type definitions that have circular inheritance don't cause an infinite
        // loop, especially when resolving a conflict between the file's name and its contents
        const QMimeType ecmascript =
                db.mimeTypeForName(QString::fromLatin1("application/ecmascript"));
        QVERIFY(ecmascript.allAncestors().contains("text/plain"));
        const QMimeType javascript = db.mimeTypeForFileNameAndData("xml.js", "<?xml?>");
        QVERIFY(javascript.inherits(QString::fromLatin1("text/javascript")));
    }
}

void tst_QMimeDatabase::aliases()
{
    QMimeDatabase db;

    const QMimeType canonical = db.mimeTypeForName(QString::fromLatin1("application/xml"));
    QVERIFY(canonical.isValid());

    QMimeType resolvedAlias = db.mimeTypeForName(QString::fromLatin1("text/xml"));
    QVERIFY(resolvedAlias.isValid());
    QCOMPARE(resolvedAlias.name(), QString::fromLatin1("application/xml"));

    QVERIFY(resolvedAlias.inherits(QLatin1String("application/xml")));
    QVERIFY(canonical.inherits(QLatin1String("text/xml")));

    // Test for kde bug 197346: does nspluginscan see that audio/mp3 already exists?
    bool mustWriteMimeType = !db.mimeTypeForName(QString::fromLatin1("audio/mp3")).isValid();
    QVERIFY(!mustWriteMimeType);
}

void tst_QMimeDatabase::listAliases_data()
{
    QTest::addColumn<QString>("inputMime");
    QTest::addColumn<QString>("expectedAliases");

    if (m_hasFreedesktopOrg) {
        QTest::newRow("csv") << "text/csv"
                             << "text/x-csv,text/x-comma-separated-values";
        QTest::newRow("xml") << "application/xml"
                             << "text/xml";
        QTest::newRow("xml2") << "text/xml" /* gets resolved to application/xml */ << "text/xml";
    } else {
        QTest::newRow("csv") << "text/csv"
                             << "";
        QTest::newRow("xml") << "application/xml"
                             << "text/xml,application/x-xml";
        QTest::newRow("xml2") << "text/xml" /* gets resolved to application/xml */
                              << "text/xml,application/x-xml";
    }
    QTest::newRow("no_mime") << "message/news" << "";
}

void tst_QMimeDatabase::listAliases()
{
    QFETCH(QString, inputMime);
    QFETCH(QString, expectedAliases);
    QMimeDatabase db;
    QStringList expectedAliasesList = expectedAliases.split(',', Qt::SkipEmptyParts);
    expectedAliasesList.sort();
    QMimeType mime = db.mimeTypeForName(inputMime);
    QVERIFY(mime.isValid());
    QStringList aliasList = mime.aliases();
    aliasList.sort();
    QCOMPARE(aliasList, expectedAliasesList);
}

void tst_QMimeDatabase::icons()
{
    QMimeDatabase db;
    QMimeType directory = db.mimeTypeForFile(QString::fromLatin1("/"));
    QCOMPARE(directory.name(), s_inodeMimetype);
    QCOMPARE(directory.iconName(), QString::fromLatin1("inode-directory"));
    QCOMPARE(directory.genericIconName(), QString::fromLatin1("folder"));

    QMimeType pub = db.mimeTypeForFile(QString::fromLatin1("foo.epub"), QMimeDatabase::MatchExtension);
    QCOMPARE(pub.name(), QString::fromLatin1("application/epub+zip"));
    QCOMPARE(pub.iconName(), QString::fromLatin1("application-epub+zip"));
    if (m_hasFreedesktopOrg)
        QCOMPARE(pub.genericIconName(), QString::fromLatin1("x-office-document"));
}

void tst_QMimeDatabase::comment()
{
    if (!m_hasFreedesktopOrg)
        QSKIP("Translations not yet available for tika mimetypes");

    struct RestoreLocale
    {
        ~RestoreLocale() { QLocale::setDefault(QLocale::c()); }
    } restoreLocale;

    QLocale::setDefault(QLocale("de"));
    QMimeDatabase db;
    QMimeType directory = db.mimeTypeForName(s_inodeMimetype);
    QCOMPARE(directory.comment(), QStringLiteral("Ordner"));
    QLocale::setDefault(QLocale("fr"));
    // Missing in s-m-i 2.3 due to case changes
    // QCOMPARE(directory.comment(), QStringLiteral("dossier"));
    QMimeType cpp = db.mimeTypeForName("text/x-c++src");
    QCOMPARE(cpp.comment(), QStringLiteral("code source C++"));
}

// In here we do the tests that need some content in a temporary file.
// This could also be added to shared-mime-info's testsuite...
void tst_QMimeDatabase::mimeTypeForFileWithContent()
{
    QMimeDatabase db;
    QMimeType mime;

    // Test a real PDF file.
    // If we find x-matlab because it starts with '%' then we are not ordering by priority.
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString tempFileName = tempFile.fileName();
    tempFile.write("%PDF-");
    tempFile.close();
    mime = db.mimeTypeForFile(tempFileName);
    QCOMPARE(mime.name(), QString::fromLatin1("application/pdf"));
    QFile file(tempFileName);
    mime = db.mimeTypeForData(&file); // QIODevice ctor
    QCOMPARE(mime.name(), QString::fromLatin1("application/pdf"));
    // by name only, we cannot find the mimetype
    mime = db.mimeTypeForFile(tempFileName, QMimeDatabase::MatchExtension);
    QVERIFY(mime.isValid());
    QVERIFY(mime.isDefault());

    // Test the case where the extension doesn't match the contents: extension wins
    {
        QTemporaryFile txtTempFile(QDir::tempPath() + QLatin1String("/tst_QMimeDatabase_XXXXXX.txt"));
        QVERIFY(txtTempFile.open());
        txtTempFile.write("%PDF-");
        QString txtTempFileName = txtTempFile.fileName();
        txtTempFile.close();
        mime = db.mimeTypeForFile(txtTempFileName);
        QCOMPARE(mime.name(), QString::fromLatin1("text/plain"));
        // fast mode finds the same
        mime = db.mimeTypeForFile(txtTempFileName, QMimeDatabase::MatchExtension);
        QCOMPARE(mime.name(), QString::fromLatin1("text/plain"));
    }

    // Now the case where extension differs from contents, but contents has >80 magic rule
    // XDG spec says: contents wins. But we can't sniff all files...
    if (m_hasFreedesktopOrg) {
        QTemporaryFile txtTempFile(QDir::tempPath() + QLatin1String("/tst_QMimeDatabase_XXXXXX.txt"));
        QVERIFY(txtTempFile.open());
        txtTempFile.write("<smil");
        QString txtTempFileName = txtTempFile.fileName();
        txtTempFile.close();
        mime = db.mimeTypeForFile(txtTempFileName);
        QCOMPARE(mime.name(), QString::fromLatin1("text/plain"));
        mime = db.mimeTypeForFile(txtTempFileName, QMimeDatabase::MatchContent);
        QCOMPARE(mime.name(), QString::fromLatin1("application/smil+xml"));
    }

    // Test what happens with Qt resources (file engines in general)
    {
        QFile rccFile(":/files/test.txt");

        mime = db.mimeTypeForFile(rccFile.fileName());
        QCOMPARE(mime.name(), "text/plain"_L1);

        QVERIFY(rccFile.open(QIODevice::ReadOnly));
        mime = db.mimeTypeForData(&rccFile);
        QCOMPARE(mime.name(), "text/x-qml"_L1);
        QVERIFY(rccFile.isOpen());

        mime = db.mimeTypeForFile(rccFile.fileName(), QMimeDatabase::MatchContent);
        QCOMPARE(mime.name(), "text/x-qml"_L1);
    }

    // Directories
    {
        mime = db.mimeTypeForFile("/");
        QCOMPARE(mime.name(), "inode/directory"_L1);

        QString dirName = QDir::tempPath();
        if (!dirName.endsWith(u'/'))
            dirName += u'/';
        mime = db.mimeTypeForFile(dirName);
        QCOMPARE(mime.name(), "inode/directory"_L1);

        while (dirName.endsWith(u'/'))
            dirName.chop(1);
        mime = db.mimeTypeForFile(dirName);
        QCOMPARE(mime.name(), "inode/directory"_L1);

        mime = db.mimeTypeForFile(":/files");
        QCOMPARE(mime.name(), "inode/directory"_L1);
    }

    // Test what happens with an incorrect path
    mime = db.mimeTypeForFile(QString::fromLatin1("file:///etc/passwd" /* incorrect code, use a path instead */));
    QVERIFY(mime.isDefault());

    // findByData when the device cannot be opened (e.g. a directory)
    QFile dir("/");
    mime = db.mimeTypeForData(&dir);
    QVERIFY(mime.isDefault());
}

void tst_QMimeDatabase::mimeTypeForUrl()
{
    QMimeDatabase db;
    QVERIFY(db.mimeTypeForUrl(QUrl::fromEncoded("http://foo/bar.png")).isDefault()); // HTTP can't know before downloading
    QCOMPARE(db.mimeTypeForUrl(QUrl::fromEncoded("ftp://foo/bar.png")).name(), QString::fromLatin1("image/png"));
    QCOMPARE(db.mimeTypeForUrl(QUrl::fromEncoded("ftp://foo/bar")).name(), QString::fromLatin1("application/octet-stream")); // unknown extension
    QCOMPARE(db.mimeTypeForUrl(QUrl("mailto:something@example.com")).name(), QString::fromLatin1("application/octet-stream")); // unknown
    QCOMPARE(db.mimeTypeForUrl(QUrl("mailto:something@polish.pl")).name(), QString::fromLatin1("application/octet-stream")); // unknown, NOT perl ;)
}

void tst_QMimeDatabase::mimeTypeForData_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("expectedMimeTypeName");

    QTest::newRow("tnef data, needs smi >= 0.20") << QByteArray("\x78\x9f\x3e\x22") << "application/vnd.ms-tnef";
    QTest::newRow("PDF magic") << QByteArray("%PDF-") << "application/pdf";
    QTest::newRow("PHP, High-priority rule")
            << QByteArray("<?php") << (m_hasFreedesktopOrg ? "application/x-php" : "text/x-php");
    if (m_hasFreedesktopOrg)
        QTest::newRow("diff\\t") << QByteArray("diff\t") << "text/x-patch";
    else
        QTest::newRow("diff_space") << QByteArray("diff ") << "text/x-diff";
    QTest::newRow("unknown") << QByteArray("\001abc?}") << "application/octet-stream";
}

void tst_QMimeDatabase::mimeTypeForData()
{
    QFETCH(QByteArray, data);
    QFETCH(QString, expectedMimeTypeName);

    QMimeDatabase db;
    QCOMPARE(db.mimeTypeForData(data).name(), expectedMimeTypeName);
    QBuffer buffer(&data);
    QCOMPARE(db.mimeTypeForData(&buffer).name(), expectedMimeTypeName);
    QVERIFY(!buffer.isOpen()); // initial state was restored

    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QCOMPARE(db.mimeTypeForData(&buffer).name(), expectedMimeTypeName);
    QVERIFY(buffer.isOpen());
    QCOMPARE(buffer.pos(), qint64(0));
}

void tst_QMimeDatabase::mimeTypeForFileNameAndData_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("expectedMimeTypeName");

    QTest::newRow("plain text, no extension") << QString::fromLatin1("textfile") << QByteArray("Hello world") << "text/plain";
    QTest::newRow("plain text, unknown extension") << QString::fromLatin1("textfile.foo") << QByteArray("Hello world") << "text/plain";
    // Needs kde/mimetypes.xml
    //QTest::newRow("plain text, doc extension") << QString::fromLatin1("textfile.doc") << QByteArray("Hello world") << "text/plain";

    // If you get powerpoint instead, then you're hit by https://bugs.freedesktop.org/show_bug.cgi?id=435,
    // upgrade to shared-mime-info >= 0.22
    const QByteArray oleData("\320\317\021\340\241\261\032\341"); // same as \xD0\xCF\x11\xE0 \xA1\xB1\x1A\xE1
    if (m_hasFreedesktopOrg)
        QTest::newRow("msword file, unknown extension")
                << QString::fromLatin1("mswordfile") << oleData << "application/x-ole-storage";
    QTest::newRow("excel file, found by extension") << QString::fromLatin1("excelfile.xls") << oleData << "application/vnd.ms-excel";
    QTest::newRow("text.xls, found by extension, user is in control") << QString::fromLatin1("text.xls") << oleData << "application/vnd.ms-excel";
}

void tst_QMimeDatabase::mimeTypeForFileNameAndData()
{
    QFETCH(QString, name);
    QFETCH(QByteArray, data);
    QFETCH(QString, expectedMimeTypeName);

    QMimeDatabase db;
    QCOMPARE(db.mimeTypeForFileNameAndData(name, data).name(), expectedMimeTypeName);

    QBuffer buffer(&data);
    QCOMPARE(db.mimeTypeForFileNameAndData(name, &buffer).name(), expectedMimeTypeName);
    QVERIFY(!buffer.isOpen()); // initial state was restored

    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QCOMPARE(db.mimeTypeForFileNameAndData(name, &buffer).name(), expectedMimeTypeName);
    QVERIFY(buffer.isOpen());
    QCOMPARE(buffer.pos(), qint64(0));
}

#ifdef Q_OS_UNIX
void tst_QMimeDatabase::mimeTypeForUnixSpecials_data()
{
#ifndef AT_FDCWD
    QSKIP("fdopendir and fstatat are not available");
#else
    if (!m_hasFreedesktopOrg)
        QSKIP("Special devices are not available in tika");

    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("expected");

    static const char * const mimeTypes[] = {
        "inode/blockdevice",
        "inode/chardevice",
        "inode/fifo",
        "inode/socket",
    };
    enum SpecialType {
        FoundBlock  = 0,
        FoundChar   = 1,
        FoundFifo   = 2,
        FoundSocket = 3,
    };
    uint found = 0;
    auto nothingfound = []() {
        QSKIP("No special Unix inode types found!");
    };

    // on a standard Linux system (systemd), /dev/log is a symlink to a socket
    // and /dev/initctl is a symlink to a FIFO
    int devfd = open("/dev", O_RDONLY);
    DIR *devdir = fdopendir(devfd); // takes ownership
    if (!devdir)
        return nothingfound();

    while (struct dirent *ent = readdir(devdir)) {
        struct stat statbuf;
        if (fstatat(devfd, ent->d_name, &statbuf, 0) < 0)
            continue;

        SpecialType type;
        if (S_ISBLK(statbuf.st_mode)) {
            type = FoundBlock;
        } else if (S_ISCHR(statbuf.st_mode)) {
            type = FoundChar;
        } else if (S_ISFIFO(statbuf.st_mode)) {
            type = FoundFifo;
        } else if (S_ISSOCK(statbuf.st_mode)) {
            type = FoundSocket;
        } else {
            if (!S_ISREG(statbuf.st_mode) && !S_ISDIR(statbuf.st_mode))
                qWarning("Could not tell what file type '%s' is: %#o'",
                         ent->d_name, statbuf.st_mode);
            continue;
        }

        if (found & (1U << type))
            continue;       // we've already seen such a type

        const char *mimeType = mimeTypes[type];
        QTest::addRow("%s", mimeType)
                << u"/dev/"_s + QFile::decodeName(ent->d_name) << mimeType;
        found |= (1U << type);
    }
    closedir(devdir);

    if (!found)
        nothingfound();
#endif
}

void tst_QMimeDatabase::mimeTypeForUnixSpecials()
{
    QFETCH(QString, name);
    QFETCH(QString, expected);

    qInfo() << "Testing that" << name << "is" << expected;
    QMimeDatabase db;
    QCOMPARE(db.mimeTypeForFile(name).name(), expected);
}
#endif

void tst_QMimeDatabase::allMimeTypes()
{
    QMimeDatabase db;
    const QList<QMimeType> lst = db.allMimeTypes(); // does NOT include aliases
    QVERIFY(!lst.isEmpty());

    // Hardcoding this is the only way to check both providers find the same number of mimetypes.
    if (m_hasFreedesktopOrg)
        QCOMPARE(lst.size(), 908);
    else
        QCOMPARE(lst.size(), 1641); // interestingly, tika has more mimetypes (but many are empty)

    for (const QMimeType &mime : lst) {
        const QString name = mime.name();
        QVERIFY(!name.isEmpty());
        QCOMPARE(name.count(QLatin1Char('/')), 1);
        const QMimeType lookedupMime = db.mimeTypeForName(name);
        QVERIFY(lookedupMime.isValid());
        QCOMPARE(lookedupMime.name(), name); // if this fails, you have an alias defined as a real mimetype too!
    }
}

void tst_QMimeDatabase::suffixes_data()
{
    QTest::addColumn<QString>("mimeType");
    QTest::addColumn<QString>("patterns");
    QTest::addColumn<QString>("preferredSuffix");

    QTest::newRow("mimetype with a single pattern") << "application/pdf" << "*.pdf" << "pdf";
    QTest::newRow("mimetype-with-multiple-patterns-kpr") << "application/x-kpresenter" << "*.kpr;*.kpt" << "kpr";
    // The preferred suffix for image/jpeg is *.jpg, as per https://bugs.kde.org/show_bug.cgi?id=176737
    QTest::newRow("jpeg") << "image/jpeg"
                          << (m_hasFreedesktopOrg ? "*.jfif;*.jpe;*.jpg;*.jpeg"
                                                  : "*.jfi;*.jfif;*.jif;*.jpe;*.jpeg;*.jpg")
                          << "jpg";
    QTest::newRow("mimetype with many patterns")
            << "application/vnd.wordperfect"
            << (m_hasFreedesktopOrg ? "*.wp;*.wp4;*.wp5;*.wp6;*.wpd;*.wpp"
                                    : "*.w60;*.wp;*.wp5;*.wp6;*.wp61;*.wpd;*.wpt")
            << (m_hasFreedesktopOrg ? "wp" : "wpd");
    QTest::newRow("oasis text mimetype") << "application/vnd.oasis.opendocument.text" << "*.odt" << "odt";
    QTest::newRow("oasis presentation mimetype") << "application/vnd.oasis.opendocument.presentation" << "*.odp" << "odp";
    if (m_hasFreedesktopOrg) { // tika has a very very long list of patterns for text/plain
        QTest::newRow("mimetype-multiple-patterns-text-plain") << "text/plain"
                                                               << "*.asc;*.txt;*,v"
                                                               << "txt";
        QTest::newRow("mimetype with uncommon pattern") << "text/x-readme"
                                                        << "README*" << QString();
    }
    QTest::newRow("mimetype with no patterns")
            << "application/x-zerosize" << QString() << QString();
    if (m_hasFreedesktopOrg) // tika has a long list of patterns for application/octet-stream
        QTest::newRow("default_mimetype") << "application/octet-stream" << QString() << QString();
}

void tst_QMimeDatabase::suffixes()
{
    QFETCH(QString, mimeType);
    QFETCH(QString, patterns);
    QFETCH(QString, preferredSuffix);
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(mimeType);
    QVERIFY(mime.isValid());
    // Sort both lists; order is unreliable since shared-mime-info uses hashes internally.
    QStringList expectedPatterns = patterns.split(QLatin1Char(';'));
    expectedPatterns.sort();
    QStringList mimePatterns = mime.globPatterns();
    mimePatterns.sort();
    QCOMPARE(mimePatterns.join(QLatin1Char(';')), expectedPatterns.join(QLatin1Char(';')));
    QCOMPARE(mime.preferredSuffix(), preferredSuffix);
}

void tst_QMimeDatabase::knownSuffix()
{
    QMimeDatabase db;
    QCOMPARE(db.suffixForFileName(QString::fromLatin1("foo.tar")), QString::fromLatin1("tar"));
    QCOMPARE(db.suffixForFileName(QString::fromLatin1("foo.bz2")), QString::fromLatin1("bz2"));
    QCOMPARE(db.suffixForFileName(QString::fromLatin1("foo.bar.bz2")), QString::fromLatin1("bz2"));
    QCOMPARE(db.suffixForFileName(QString::fromLatin1("foo.tar.bz2")), QString::fromLatin1("tar.bz2"));
    QCOMPARE(db.suffixForFileName(QString::fromLatin1("foo.TAR")), QString::fromLatin1("TAR")); // preserve case
    if (m_hasFreedesktopOrg) {
        QCOMPARE(db.suffixForFileName(QString::fromLatin1("foo.flatpakrepo")),
                 QString::fromLatin1("flatpakrepo"));
        QCOMPARE(db.suffixForFileName(QString::fromLatin1("foo.anim2")),
                 QString()); // the glob is anim[0-9], no way to extract the extension without
                             // expensive regexp capturing
    }
}

void tst_QMimeDatabase::filterString_data()
{
    QTest::addColumn<QString>("mimeType");
    QTest::addColumn<QString>("expectedFilterString");

    QTest::newRow("single-pattern")
            << "application/pdf"
            << (m_hasFreedesktopOrg ? "PDF document (*.pdf)" : "Portable Document Format (*.pdf)");
    if (m_hasFreedesktopOrg)
        QTest::newRow("multiple-patterns-text-plain") << "text/plain"
                                                      << "Plain text document (*.txt *.asc *,v)";
    else
        QTest::newRow("multiple-patterns-kword") << "application/vnd.kde.kword"
                                                 << "KWord File (*.kwd *.kwt)";
}

void tst_QMimeDatabase::filterString()
{
    QFETCH(QString, mimeType);
    QFETCH(QString, expectedFilterString);

    QMimeDatabase db;
    QCOMPARE(db.mimeTypeForName(mimeType).filterString(), expectedFilterString);
}

void tst_QMimeDatabase::symlinkToFifo() // QTBUG-48529
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_INTEGRITY)
    if (!m_hasFreedesktopOrg)
        QSKIP("Special devices are not available in tika");

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString dir = tempDir.path();
    const QString fifo = dir + "/fifo";
    QCOMPARE(mkfifo(QFile::encodeName(fifo), 0006), 0);

    QMimeDatabase db;
    QCOMPARE(db.mimeTypeForFile(fifo).name(), QString::fromLatin1("inode/fifo"));

    // Now make a symlink to the fifo
    const QString link = dir + "/link";
    QVERIFY(QFile::link(fifo, link));
    QCOMPARE(db.mimeTypeForFile(link).name(), QString::fromLatin1("inode/fifo"));

#else
    QSKIP("This test requires pipes and symlinks");
#endif
}

void tst_QMimeDatabase::findByFileName_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<QString>("mimeTypeName");
    QTest::addColumn<QString>("xFail");

    if (m_testSuite.isEmpty())
        QSKIP("shared-mime-info test suite not available.");

    const QString prefix = m_testSuite + QLatin1Char('/');
    const QString fileName = prefix + QLatin1String("list");
    QFile f(fileName);
    QVERIFY2(f.open(QIODevice::ReadOnly|QIODevice::Text),
             qPrintable(QString::fromLatin1("Cannot open %1: %2").arg(fileName, f.errorString())));

    QByteArray line(1024, Qt::Uninitialized);

    QDuplicateTracker<QString, 800> seen;

    while (!f.atEnd()) {
        const qint64 len = f.readLine(line.data(), 1023);

        if (len <= 2 || line.at(0) == '#')
            continue;

        QString string = QString::fromLatin1(line.constData(), len - 1).trimmed();
        QStringList list = string.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        QVERIFY(list.size() >= 2);

        QString filePath = list.at(0);
        QString mimeTypeType = list.at(1);
        QString xFail;
        if (list.size() >= 3)
            xFail = list.at(2);
        QString rowTag = filePath;
        if (seen.hasSeen(rowTag)) {
            // Two testcases for the same file, e.g.
            // test.ogg audio/ogg oxx
            // test.ogg audio/x-vorbis+ogg x
            rowTag += "_2";
        }

        QTest::newRow(rowTag.toLatin1().constData())
                << QString(prefix + filePath) << mimeTypeType << xFail;
    }
}

void tst_QMimeDatabase::findByFileName()
{
    QFETCH(QString, filePath);
    QFETCH(QString, mimeTypeName);
    QFETCH(QString, xFail);

    QMimeDatabase database;

    //qDebug() << Q_FUNC_INFO << filePath;

    const QMimeType resultMimeType(database.mimeTypeForFile(filePath, QMimeDatabase::MatchExtension));
    if (resultMimeType.isValid()) {
        //qDebug() << Q_FUNC_INFO << "MIME type" << resultMimeType.name() << "has generic icon name" << resultMimeType.genericIconName() << "and icon name" << resultMimeType.iconName();

// Loading icons depend on the icon theme, we can't enable this test
#if 0
        QCOMPARE(resultMimeType.genericIconName(), QIcon::fromTheme(resultMimeType.genericIconName()).name());
        QVERIFY2(!QIcon::fromTheme(resultMimeType.genericIconName()).isNull(), qPrintable(resultMimeType.genericIconName()));
        QVERIFY2(QIcon::hasThemeIcon(resultMimeType.genericIconName()), qPrintable(resultMimeType.genericIconName()));

        QCOMPARE(resultMimeType.iconName(), QIcon::fromTheme(resultMimeType.iconName()).name());
        QVERIFY2(!QIcon::fromTheme(resultMimeType.iconName()).isNull(), qPrintable(resultMimeType.iconName()));
        QVERIFY2(QIcon::hasThemeIcon(resultMimeType.iconName()), qPrintable(resultMimeType.iconName()));
#endif
    }
    const QString resultMimeTypeName = resultMimeType.name();
    //qDebug() << Q_FUNC_INFO << "mimeTypeForFile() returned" << resultMimeTypeName;

    const bool failed = resultMimeTypeName != mimeTypeName;
    const bool shouldFail = (xFail.size() >= 1 && xFail.at(0) == QLatin1Char('x'));
    if (shouldFail != failed) {
        // Results are ambiguous when multiple MIME types have the same glob
        // -> accept the current result if the found MIME type actually
        // matches the file's extension.
        // TODO: a better file format in testfiles/list!
        const QMimeType foundMimeType = database.mimeTypeForName(resultMimeTypeName);
        QVERIFY2(resultMimeType == foundMimeType, qPrintable(resultMimeType.name() + QString::fromLatin1(" vs. ") + foundMimeType.name()));
        if (foundMimeType.isValid()) {
            const QString extension = QFileInfo(filePath).suffix();
            //qDebug() << Q_FUNC_INFO << "globPatterns:" << foundMimeType.globPatterns() << "- extension:" << QString() + "*." + extension;
            if (foundMimeType.globPatterns().contains(QString::fromLatin1("*.") + extension))
                return;
        }
    }
    if (shouldFail) {
        // Expected to fail
        QVERIFY2(resultMimeTypeName != mimeTypeName, qPrintable(resultMimeTypeName));
    } else {
        QCOMPARE(resultMimeTypeName, mimeTypeName);
    }

    // Test QFileInfo overload
    const QMimeType mimeForFileInfo = database.mimeTypeForFile(QFileInfo(filePath), QMimeDatabase::MatchExtension);
    QCOMPARE(mimeForFileInfo.name(), resultMimeTypeName);

    const QString suffix = database.suffixForFileName(filePath);
    QVERIFY2(filePath.endsWith(suffix), qPrintable(filePath + " does not end with " + suffix));
}

void tst_QMimeDatabase::findByData_data()
{
    findByFileName_data();
}

void tst_QMimeDatabase::findByData()
{
    QFETCH(QString, filePath);
    QFETCH(QString, mimeTypeName);
    QFETCH(QString, xFail);

    QMimeDatabase database;
    QFile f(filePath);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QByteArray data = f.read(16384);

    const QString resultMimeTypeName = database.mimeTypeForData(data).name();
    if (xFail.size() >= 2 && xFail.at(1) == QLatin1Char('x')) {
        // Expected to fail
        QVERIFY2(resultMimeTypeName != mimeTypeName, qPrintable(resultMimeTypeName));
    } else {
        QCOMPARE(resultMimeTypeName.toLower(), mimeTypeName.toLower());
    }

    QFileInfo info(filePath);
    QString mimeForInfo = database.mimeTypeForFile(info, QMimeDatabase::MatchContent).name();
    QCOMPARE(mimeForInfo, resultMimeTypeName);
    QString mimeForFile = database.mimeTypeForFile(filePath, QMimeDatabase::MatchContent).name();
    QCOMPARE(mimeForFile, resultMimeTypeName);
}

void tst_QMimeDatabase::findByFile_data()
{
    findByFileName_data();
}

// Note: this test fails on "testcompress.z" when using a shared-mime-info older than 1.0.
// This because of commit 0f9a506069c in shared-mime-info, which fixed the writing of
// case-insensitive patterns into mime.cache.
void tst_QMimeDatabase::findByFile()
{
    QFETCH(QString, filePath);
    QFETCH(QString, mimeTypeName);
    QFETCH(QString, xFail);

    QMimeDatabase database;
    const QString resultMimeTypeName = database.mimeTypeForFile(filePath).name();
    //qDebug() << Q_FUNC_INFO << filePath << "->" << resultMimeTypeName;
    if (xFail.size() >= 3 && xFail.at(2) == QLatin1Char('x')) {
        // Expected to fail
        QVERIFY2(resultMimeTypeName != mimeTypeName, qPrintable(resultMimeTypeName));
    } else {
        QCOMPARE(resultMimeTypeName.toLower(), mimeTypeName.toLower());
    }

    // Test QFileInfo overload
    const QMimeType mimeForFileInfo = database.mimeTypeForFile(QFileInfo(filePath));
    QCOMPARE(mimeForFileInfo.name(), resultMimeTypeName);
}


void tst_QMimeDatabase::fromThreads()
{
    QThreadPool tp;
    tp.setMaxThreadCount(20);
    // Note that data-based tests cannot be used here (QTest::fetchData asserts).
    Q_UNUSED(QtConcurrent::run(&tp, &tst_QMimeDatabase::mimeTypeForName, this));
    Q_UNUSED(QtConcurrent::run(&tp, &tst_QMimeDatabase::aliases, this));
    Q_UNUSED(QtConcurrent::run(&tp, &tst_QMimeDatabase::allMimeTypes, this));
    Q_UNUSED(QtConcurrent::run(&tp, &tst_QMimeDatabase::icons, this));
    Q_UNUSED(QtConcurrent::run(&tp, &tst_QMimeDatabase::inheritance, this));
    Q_UNUSED(QtConcurrent::run(&tp, &tst_QMimeDatabase::knownSuffix, this));
    Q_UNUSED(QtConcurrent::run(&tp, &tst_QMimeDatabase::mimeTypeForFileWithContent, this));
    Q_UNUSED(QtConcurrent::run(&tp, &tst_QMimeDatabase::allMimeTypes, this)); // a second time
    QVERIFY(tp.waitForDone(60000));
}

#if QT_CONFIG(process)

enum {
    UpdateMimeDatabaseTimeout = 4 * 60 * 1000 // 4min
};

static bool runUpdateMimeDatabase(const QString &path) // TODO make it a QMimeDatabase method?
{
    const QString umdCommand = QString::fromLatin1("update-mime-database");
    const QString umd = QStandardPaths::findExecutable(umdCommand);
    if (umd.isEmpty()) {
        qWarning("%s does not exist.", qPrintable(umdCommand));
        return false;
    }
#if QT_CONFIG(process)
    QElapsedTimer timer;
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels); // silence output
    qDebug().noquote() << "runUpdateMimeDatabase: running" << umd << path << "...";
    timer.start();
    proc.start(umd, QStringList(path));
    if (!proc.waitForStarted()) {
        qWarning("Cannot start %s: %s",
                 qPrintable(umd), qPrintable(proc.errorString()));
        return false;
    }
    const bool success = proc.waitForFinished(UpdateMimeDatabaseTimeout);
    qDebug().noquote() << "runUpdateMimeDatabase: done,"
        << success << timer.elapsed() << "ms";
#endif
    return true;
}

static bool waitAndRunUpdateMimeDatabase(const QString &path)
{
    QFileInfo mimeCacheInfo(path + QString::fromLatin1("/mime.cache"));
    if (mimeCacheInfo.exists()) {
        // Wait until the beginning of the next second
        while (mimeCacheInfo.lastModified(QTimeZone::UTC).secsTo(QDateTime::currentDateTimeUtc()) == 0) {
            QTest::qSleep(200);
        }
    }
    return runUpdateMimeDatabase(path);
}
#endif // QT_CONFIG(process)

static void checkHasMimeType(const QString &mimeType)
{
    QMimeDatabase db;
    QVERIFY(db.mimeTypeForName(mimeType).isValid());

    bool found = false;
    const auto all = db.allMimeTypes();
    for (const QMimeType &mt : all) {
        if (mt.name() == mimeType) {
            found = true;
            break;
        }
    }
    QVERIFY(found);
}

static void ignoreInvalidMimetypeWarnings(const QString &mimeDir)
{
    const QByteArray basePath = QFile::encodeName(mimeDir) + "/packages/";
    QTest::ignoreMessage(QtWarningMsg, ("QMimeDatabase: Error parsing " + basePath + "invalid-magic1.xml\nInvalid magic rule value \"foo\"").constData());
    QTest::ignoreMessage(QtWarningMsg, ("QMimeDatabase: Error parsing " + basePath + "invalid-magic2.xml\nInvalid magic rule mask \"ffff\"").constData());
    QTest::ignoreMessage(QtWarningMsg, ("QMimeDatabase: Error parsing " + basePath + "invalid-magic3.xml\nInvalid magic rule mask size \"0xffff\"").constData());
}

QT_BEGIN_NAMESPACE
extern Q_CORE_EXPORT int qmime_secondsBetweenChecks; // see qmimeprovider.cpp
QT_END_NAMESPACE

void copyFiles(const QSpan<const char *const> &additionalMimeFiles, const QString &destDir)
{
    const QString notFoundErrorMessage = QString::fromLatin1("Cannot find '%1'");
    for (const char *mimeFile : additionalMimeFiles) {
        const QString resourceFilePath = s_additionalFilesResourcePrefix + QLatin1String(mimeFile);
        QVERIFY2(QFile::exists(resourceFilePath),
                 qPrintable(notFoundErrorMessage.arg(resourceFilePath)));

        const QString destFile = destDir + QLatin1String(mimeFile);
        QFile::remove(destFile);
        QString errorMessage;
        QVERIFY2(copyResourceFile(resourceFilePath, destFile, &errorMessage),
                 qPrintable(errorMessage));
    }
}

void deleteFiles(const QSpan<const char *const> &additionalMimeFiles, const QString &destDir)
{
    for (const char *mimeFile : additionalMimeFiles) {
        const QString destFile = destDir + QLatin1String(mimeFile);
        QFile::remove(destFile);
    }
}

void tst_QMimeDatabase::installNewGlobalMimeType()
{
#if !defined(USE_XDG_DATA_DIRS)
    QSKIP("This test requires XDG_DATA_DIRS");
#endif

#if !QT_CONFIG(process)
    QSKIP("This test requires QProcess support");
#else
    qmime_secondsBetweenChecks = 0;

    QMimeDatabase db;
    QVERIFY(!db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());

    const QString mimeDir = m_globalXdgDir + QLatin1String("/mime");
    const QString destDir = mimeDir + QLatin1String("/packages/");
    if (!QFileInfo(destDir).isDir())
        QVERIFY(QDir(m_globalXdgDir).mkpath(destDir));

    copyFiles(additionalGlobalMimeFiles, destDir);
    QVERIFY(!QTest::currentTestFailed());
    if (m_isUsingCacheProvider && !waitAndRunUpdateMimeDatabase(mimeDir))
        QSKIP("shared-mime-info not found, skipping mime.cache test");

    QCOMPARE(db.mimeTypeForFile(QLatin1String("foo.ymu"), QMimeDatabase::MatchExtension).name(),
             QString::fromLatin1("text/x-SuSE-ymu"));
    QVERIFY(db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());
    checkHasMimeType("text/x-suse-ymp");

    // Test that a double-definition of a mimetype doesn't lead to sniffing ("conflicting globs").
    const QString qmlTestFile = s_additionalFilesResourcePrefix + "test.qml"_L1;
    QVERIFY2(!qmlTestFile.isEmpty(),
             qPrintable(QString::fromLatin1("Cannot find '%1' starting from '%2'").
                        arg("test.qml", QDir::currentPath())));
    QCOMPARE(db.mimeTypeForFile(qmlTestFile).name(),
             QString::fromLatin1("text/x-qml"));

    const QString fooTestFile = s_additionalFilesResourcePrefix + "magic-and-hierarchy.foo"_L1;
    QCOMPARE(db.mimeTypeForFile(fooTestFile).name(), QString::fromLatin1("application/foo"));

    const QString fooTestFile2 = s_additionalFilesResourcePrefix + "magic-and-hierarchy2.foo"_L1;
    QCOMPARE(db.mimeTypeForFile(fooTestFile2).name(), QString::fromLatin1("application/vnd.qnx.bar-descriptor"));

    // Test if we can use the default comment
    {
        struct RestoreLocale
        {
            ~RestoreLocale() { QLocale::setDefault(QLocale::c()); }
        } restoreLocale;

        QLocale::setDefault(QLocale("zh_CN"));
        QMimeType suseymp = db.mimeTypeForName("text/x-suse-ymp");
        QVERIFY(suseymp.isValid());
        QCOMPARE(suseymp.comment(),
                 QString::fromLatin1("YaST Meta Package"));
    }

    // Now test removing the mimetype definitions again
    deleteFiles(additionalGlobalMimeFiles, destDir);
    if (m_isUsingCacheProvider && !waitAndRunUpdateMimeDatabase(mimeDir))
        QSKIP("shared-mime-info not found, skipping mime.cache test");
    QCOMPARE(db.mimeTypeForFile(QLatin1String("foo.ymu"), QMimeDatabase::MatchExtension).name(),
             QString::fromLatin1("application/octet-stream"));
    QVERIFY(!db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());
#endif // QT_CONFIG(process)
}

void tst_QMimeDatabase::installNewLocalMimeType_data()
{
    QTest::addColumn<bool>("useLocalBinaryCache");

    // Test mixing the providers:
    // * m_isUsingCacheProvider is about the global directory.
    // ** when true, we'll test both for the local directory.
    // ** when false, we can't, because QT_NO_MIME_CACHE is set, so it's XML+XML only

#if QT_CONFIG(process)
    if (m_isUsingCacheProvider)
        QTest::newRow("with_binary_cache") << true;
#endif
    QTest::newRow("without_binary_cache") << false;
}

void tst_QMimeDatabase::installNewLocalMimeType()
{
    QFETCH(bool, useLocalBinaryCache);

    qmime_secondsBetweenChecks = 0;

    QMimeDatabase db;

    // Check that we're starting clean
    QVERIFY(!db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());
    QVERIFY(!db.mimeTypeForName(QLatin1String("text/invalid-magic1")).isValid());

    const QString destDir = m_localMimeDir + QLatin1String("/packages/");
    QVERIFY(QDir().mkpath(destDir));

    copyFiles(additionalLocalMimeFiles, destDir);
    QVERIFY(!QTest::currentTestFailed());
    if (useLocalBinaryCache && !waitAndRunUpdateMimeDatabase(m_localMimeDir)) {
        const QString skipWarning = QStringLiteral("shared-mime-info not found, skipping mime.cache test (")
                                    + QDir::toNativeSeparators(m_localMimeDir) + QLatin1Char(')');
        QSKIP(qPrintable(skipWarning));
    }

    if (!useLocalBinaryCache)
        ignoreInvalidMimetypeWarnings(m_localMimeDir);

    QVERIFY(db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());

    // These mimetypes have invalid magic, but still do exist.
    QVERIFY(db.mimeTypeForName(QLatin1String("text/invalid-magic1")).isValid());
    QVERIFY(db.mimeTypeForName(QLatin1String("text/invalid-magic2")).isValid());
    QVERIFY(db.mimeTypeForName(QLatin1String("text/invalid-magic3")).isValid());

    QCOMPARE(db.mimeTypeForFile(QLatin1String("foo.ymu"), QMimeDatabase::MatchExtension).name(),
             QString::fromLatin1("text/x-SuSE-ymu"));
    QVERIFY(db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());
    QCOMPARE(db.mimeTypeForName(QLatin1String("text/x-SuSE-ymu")).comment(), QString("URL of a YaST Meta Package"));
    checkHasMimeType("text/x-suse-ymp");

    { // QTBUG-85436
        QMimeType objcsrc = db.mimeTypeForName(QStringLiteral("text/x-objcsrc"));
        QVERIFY(objcsrc.isValid());
        QCOMPARE(objcsrc.globPatterns(), QStringList());
    }
    QCOMPARE(db.mimeTypeForFile(QLatin1String("foo.txt"), QMimeDatabase::MatchExtension).name(),
             QString::fromLatin1("text/plain"));

    // Test that a double-definition of a mimetype doesn't lead to sniffing ("conflicting globs").
    const QString qmlTestFile = s_additionalFilesResourcePrefix + "test.qml"_L1;
    QVERIFY2(!qmlTestFile.isEmpty(),
             qPrintable(QString::fromLatin1("Cannot find '%1' starting from '%2'").
                        arg("test.qml", QDir::currentPath())));
    QCOMPARE(db.mimeTypeForFile(qmlTestFile).name(),
             QString::fromLatin1("text/x-qml"));

    { // QTBUG-101755
        QList<QMimeType> mimes = db.mimeTypesForFileName(u"foo.webm"_s);
        // "*.webm" glob pattern is deleted with "glob-deleteall"
        QVERIFY2(mimes.isEmpty(), qPrintable(mimeTypeNames(mimes).join(u',')));
        mimes = db.mimeTypesForFileName(u"foo.videowebm"_s);
        QCOMPARE(mimes.size(), 1);
        QCOMPARE(mimes.at(0).globPatterns(), QStringList{ "*.videowebm" });
        // Custom "*.videowebm" pattern is used instead
        QCOMPARE(mimes.at(0).name(), u"video/webm");
    }

    // QTBUG-116905: globPatterns() should merge all locations
    // add-extension.xml adds *.jnewext
    const auto expectedJpegPatterns = m_hasFreedesktopOrg
            ? QStringList{ "*.jpg", "*.jpeg", "*.jpe", "*.jfif", "*.jnewext" }
            : QStringList{ "*.jpg", "*.jpeg", "*.jpe", "*.jif", "*.jfif", "*.jfi", "*.jnewext" };
    QCOMPARE(db.mimeTypeForName(QStringLiteral("image/jpeg")).globPatterns(), expectedJpegPatterns);

    // Now that we have two directories with mime definitions, check that everything still works
    inheritance();
    if (QTest::currentTestFailed())
        return;

    aliases();
    if (QTest::currentTestFailed())
        return;

    icons();
    if (QTest::currentTestFailed())
        return;

    comment();
    if (QTest::currentTestFailed())
        return;

    mimeTypeForFileWithContent();
    if (QTest::currentTestFailed())
        return;

    mimeTypeForName();
    if (QTest::currentTestFailed())
        return;

    // Now test removing local mimetypes
    for (int i = 1 ; i <= 3 ; ++i)
        QFile::remove(destDir + QStringLiteral("invalid-magic%1.xml").arg(i));
    if (useLocalBinaryCache && !waitAndRunUpdateMimeDatabase(m_localMimeDir))
        QSKIP("shared-mime-info not found, skipping mime.cache test");
    QVERIFY(!db.mimeTypeForName(QLatin1String("text/invalid-magic1")).isValid()); // deleted
    QVERIFY(db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid()); // still present

    // The user deletes the cache -> the XML provider makes things still work
    QFile::remove(m_localMimeDir + QString::fromLatin1("/mime.cache"));
    QVERIFY(!db.mimeTypeForName(QLatin1String("text/invalid-magic1")).isValid()); // deleted
    QVERIFY(db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid()); // still present

    // Finally, the user deletes the whole local dir
    QVERIFY2(QDir(m_localMimeDir).removeRecursively(), qPrintable(m_localMimeDir + ": " + qt_error_string()));
    QCOMPARE(db.mimeTypeForFile(QLatin1String("foo.ymu"), QMimeDatabase::MatchExtension).name(),
             QString::fromLatin1("application/octet-stream"));
    QVERIFY(!db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());
}

QTEST_GUILESS_MAIN(tst_QMimeDatabase)
