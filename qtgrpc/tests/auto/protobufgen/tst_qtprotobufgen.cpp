// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QString>
#include <QLibraryInfo>
#include <QProcess>
#include <QCryptographicHash>
#include <qtprotobuftypes.h>

#define XSTR(x) DEFSTR(x)
#define DEFSTR(x) #x

using namespace Qt::StringLiterals;

const QLatin1StringView cppExtension(".qpb.cpp");
const QLatin1StringView headerExtension(".qpb.h");
const QLatin1StringView cppRegistrationsExtension("_protobuftyperegistrations.cpp");
const QLatin1StringView protocGenQtprotobufKey(" --plugin=protoc-gen-qtprotobuf=");
const QLatin1StringView optKey(" --qtprotobuf_opt=");
const QLatin1StringView outputKey(" --qtprotobuf_out=");
const QLatin1StringView includeKey(" -I");
#ifdef ALLOW_PROTO3_OPTIONAL
const QLatin1StringView allow_proto3_optional(" --experimental_allow_proto3_optional");
#else
const QLatin1StringView allow_proto3_optional("");
#endif // ALLOW_PROTO3_OPTIONAL
#ifndef PROTOC_EXECUTABLE
#  error PROTOC_EXECUTABLE definition must be set and point to the valid protoc executable
#endif
const QLatin1StringView protocolBufferCompiler(XSTR(PROTOC_EXECUTABLE));
#if defined(Q_OS_WIN)
const QLatin1StringView qtprotobufgen("/qtprotobufgen.exe");
#else
const QLatin1StringView qtprotobufgen("/qtprotobufgen");
#endif

QByteArray msgProcessStartFailed(const QProcess &p)
{
    const QString result = QLatin1StringView("Could not start \"")
            + QDir::toNativeSeparators(p.program()) + QLatin1StringView("\": ")
            + p.errorString();
    return result.toLocal8Bit();
}

QByteArray msgProcessTimeout(const QProcess &p)
{
    return '"' + QDir::toNativeSeparators(p.program()).toLocal8Bit()
            + "\" timed out.";
}

QByteArray msgProcessCrashed(QProcess &p)
{
    return '"' + QDir::toNativeSeparators(p.program()).toLocal8Bit()
            + "\" crashed.\n" + p.readAllStandardError();
}

QByteArray msgProcessFailed(QProcess &p)
{
    return '"' + QDir::toNativeSeparators(p.program()).toLocal8Bit()
            + "\" returned " + QByteArray::number(p.exitCode()) + ":\n"
            + p.readAllStandardError();
}

QByteArray hash(const QByteArray &fileData)
{
    return QCryptographicHash::hash(fileData, QCryptographicHash::Sha1);
}

QByteArrayList splitToLines(const QByteArray &data)
{
    return data.split('\n');
}

// Return size diff and first NOT equal line;
QByteArray doCompare(const QByteArrayList &actual, const QByteArrayList &expected)
{
    QByteArray ba;

    if (actual.size() != expected.size()) {
        ba.append(QString("Length count different: actual: %1, expected: %2")
                  .arg(actual.size()).arg(expected.size()).toUtf8());
    }

    for (int i = 0, n = expected.size(); i != n; ++i) {
        QString expectedLine = expected.at(i);
        if (expectedLine != actual.at(i)) {
            ba.append("\n<<<<<< ACTUAL\n" + actual.at(i)
                      + "\n======\n" + expectedLine.toUtf8()
                      + "\n>>>>>> EXPECTED\n"
                      );
            break;
        }
    }
    return ba;
}

QByteArray msgCannotReadFile(const QFile &file)
{
    const QString result = QLatin1StringView("Could not read file: ")
            + QDir::toNativeSeparators(file.fileName())
            + QLatin1StringView(": ") + file.errorString();
    return result.toLocal8Bit();
}

void compareTwoFiles(const QString &expectedFileName, const QString &actualFileName)
{
    QFile expectedResultFile(expectedFileName);
    QFile generatedFile(actualFileName);

    QVERIFY2(expectedResultFile.exists(), qPrintable(expectedResultFile.fileName()));
    QVERIFY2(generatedFile.exists(), qPrintable(expectedResultFile.fileName()));

    QVERIFY2(expectedResultFile.open(QIODevice::ReadOnly | QIODevice::Text),
             msgCannotReadFile(expectedResultFile).constData());
    QVERIFY2(generatedFile.open(QIODevice::ReadOnly | QIODevice::Text),
             msgCannotReadFile(generatedFile).constData());

    QByteArray expectedData = expectedResultFile.readAll();
    QByteArray generatedData = generatedFile.readAll();

    expectedResultFile.close();
    generatedFile.close();

    if (hash(expectedData).toHex() != hash(generatedData).toHex()) {
        const QString diff = doCompare(splitToLines(generatedData),
                                       splitToLines(expectedData));
        QCOMPARE_GT(diff.size(), 0); // Hashes can only differ if content does.
        QFAIL(qPrintable(diff));
    }
    // Ensure we do see a failure, even in the unlikely case of a hash collision:
    QVERIFY(generatedData == expectedData);
}

bool containsString(const QStringList &list, const QString &comment)
{
    return std::any_of(list.cbegin(), list.cend(),
                       [&comment](const auto &it) {return it == comment;});
}

void cleanFolder(const QString &folderName)
{
    QDir dir(folderName);
    dir.removeRecursively();
}

bool protocolCompilerAvailableToRun()
{
    QProcess protoc;
    protoc.startCommand(protocolBufferCompiler + " --version");

    if (!protoc.waitForStarted())
        return false;

    if (!protoc.waitForFinished()) {
        protoc.kill();
        return false;
    }
    return protoc.exitStatus() == QProcess::NormalExit;
}

class tst_qtprotobufgen : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    //! Test qt_add_protobuf() cmake function
    void cmakeGeneratedFile_data();
    void cmakeGeneratedFile();

    //! Test command-line call of qtprotobufgen
    void cmdLineGeneratedFile_data();
    void cmdLineGeneratedFile();
    void cmdLineGeneratedNoOptions_data();
    void cmdLineGeneratedNoOptions();
    void cmdLineInvalidExportMacro_data();
    void cmdLineInvalidExportMacro();

    void cleanupTestCase();

private:
    QString m_protobufgen;
    QString m_cmakeGenerated;
    QString m_commandLineGenerated;
    QString m_expectedResult;
    QString m_protoFiles;
};

void tst_qtprotobufgen::initTestCase()
{
    m_protobufgen = QLibraryInfo::path(QLibraryInfo::LibraryExecutablesPath) + qtprotobufgen;

    m_cmakeGenerated = QFINDTESTDATA("qt_protobuf_generated");
    QVERIFY(!m_cmakeGenerated.isEmpty());

    m_expectedResult = QFINDTESTDATA("data/expected_result");
    QVERIFY(!m_expectedResult.isEmpty());

    m_protoFiles = QFINDTESTDATA("../shared/data/proto/");
    QVERIFY(!m_protoFiles.isEmpty());

    QDir testOutputBaseDir(QCoreApplication::applicationDirPath());
    testOutputBaseDir.mkdir(QLatin1StringView("cmd_line_generation"));
    QLatin1StringView folders[] = {
        "comments"_L1,       "extra-namespace"_L1, "fieldenum"_L1,           "folder"_L1,
        "qml-no-package"_L1, "no-options"_L1,      "invalid_export_macro"_L1
    };
    for (QLatin1StringView folder : folders)
        testOutputBaseDir.mkdir("cmd_line_generation/"_L1 + folder);

    m_commandLineGenerated = testOutputBaseDir.absolutePath() +
                             QLatin1StringView("/cmd_line_generation");
    QVERIFY(!m_commandLineGenerated.isEmpty());
#ifdef Q_OS_MACOS
    if (!protocolCompilerAvailableToRun())
        QSKIP("Protocol buffer compiler is not provisioned for macOS ARM VMs: QTBUG-109130");
#else
    QVERIFY(protocolCompilerAvailableToRun());
#endif
}

void tst_qtprotobufgen::cmakeGeneratedFile_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("folder");
    QTest::addColumn<QString>("extension");

    const QLatin1StringView extensions[] = {cppExtension,
                                            headerExtension,
                                            cppRegistrationsExtension};

    for (const auto extension : extensions) {
        QTest::addRow("repeatednonpackedmessages%s", extension.data())
                << "repeatednonpackedmessages"
                << "/packed/"
                << QString(extension);

        QTest::addRow("annotation%s", extension.data())
                << "annotation"
                << "/comments/"
                << QString(extension);

        QTest::addRow("basicmessages%s", extension.data())
                << "basicmessages"
                << "/folder/qtprotobufnamespace/tests/"
                << QString(extension);

        QTest::addRow("mapmessages%s", extension.data())
                << "mapmessages"
                << "/folder/qtprotobufnamespace/tests/"
                << QString(extension);

        QTest::addRow("oneofmessages%s", extension.data())
                << "oneofmessages"
                << "/folder/qtprotobufnamespace/tests/"
                << QString(extension);

        QTest::addRow("optional%s", extension.data())
                << "optional"
                << "/folder/qtprotobufnamespace/optional/tests/"
                << QString(extension);

        QTest::addRow("repeatedmessages%s", extension.data())
                << "repeatedmessages"
                << "/folder/qtprotobufnamespace/tests/"
                << QString(extension);

        QTest::addRow("fieldindexrange%s", extension.data())
                << "fieldindexrange"
                << "/fieldenum/"
                << QString(extension);

        QTest::addRow("extranamespace%s", extension.data())
                << "extranamespace"
                << "/extra-namespace/"
                << QString(extension);

        QTest::addRow("custom-exports/basicmessages%s", extension.data())
            << "basicmessages"
            << "/custom-exports/"
            << QString(extension);

        QTest::addRow("no-exports/basicmessages%s", extension.data())
            << "basicmessages"
            << "/no-exports/"
            << QString(extension);
#ifdef HAVE_QML
        QTest::addRow("nopackage%s", extension.data())
                << "nopackage"
                << "/qml-no-package/"
                << QString(extension);
#endif
    }

    //Check the generating of cpp export files
    QTest::addRow("cpp-exports")
        << "tst_qtprotobufgen_gen_exports.qpb.h"
        << "/folder/"
        << QString();

    QTest::addRow("custom-cpp-exports")
        << "tst_qtprotobufgen_custom_exports_gen_exports.qpb.h"
        << "/custom-exports/"
        << QString();


#ifdef HAVE_QML
    const QLatin1StringView qmlExtensions[]
            = {cppExtension,
               headerExtension,
               cppRegistrationsExtension};

    for (const auto extension : qmlExtensions) {
        QTest::addRow("enummessages%s with QML option", extension.data())
                << "enummessages"
                << "/qmlgen/"
                << QString(extension);

        QTest::addRow("basicmessages%s with QML option", extension.data())
                << "basicmessages"
                << "/qmlgen/"
                << QString(extension);
        QTest::addRow("oneofmessages%s with QML option", extension.data())
                << "oneofmessages"
                << "/qmlgen/"
                << QString(extension);
    }
#endif
}

void tst_qtprotobufgen::cmakeGeneratedFile()
{
    QFETCH(QString, fileName);
    QFETCH(QString, folder);
    QFETCH(QString, extension);

    const QString filePath = folder + fileName + extension;
    compareTwoFiles(m_expectedResult + filePath, m_cmakeGenerated + filePath);
}

void tst_qtprotobufgen::cmdLineGeneratedFile_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("generatingOption");
    QTest::addColumn<QString>("folder");
    QTest::addColumn<QString>("extension");
    QTest::addColumn<QString>("generatedFolderStructure");
    QTest::addColumn<QString>("exportMacro");

    const QLatin1StringView extensions[]
            = {cppExtension, headerExtension, cppRegistrationsExtension};

    for (const auto extension : extensions) {
        QTest::addRow("basicmessages%s", extension.data())
                << "basicmessages"
                << "GENERATE_PACKAGE_SUBFOLDERS"
                << "/folder/"
                << QString(extension)
                << "qtprotobufnamespace/tests/"
                << "EXPORT_MACRO=TST_QTPROTOBUFGEN_GEN";

        QTest::addRow("mapmessages%s", extension.data())
                << "mapmessages"
                << "GENERATE_PACKAGE_SUBFOLDERS"
                << "/folder/"
                << QString(extension)
                << "qtprotobufnamespace/tests/"
                << "EXPORT_MACRO=TST_QTPROTOBUFGEN_GEN";

        QTest::addRow("oneofmessages%s", extension.data())
                << "oneofmessages"
                << "GENERATE_PACKAGE_SUBFOLDERS"
                << "/folder/"
                << QString(extension)
                << "qtprotobufnamespace/tests/"
                << "EXPORT_MACRO=TST_QTPROTOBUFGEN_GEN";

        QTest::addRow("optional%s", extension.data())
                << "optional"
                << "GENERATE_PACKAGE_SUBFOLDERS"
                << "/folder/"
                << QString(extension)
                << "qtprotobufnamespace/optional/tests/"
                << "EXPORT_MACRO=TST_QTPROTOBUFGEN_GEN";

        QTest::addRow("repeatedmessages%s", extension.data())
                << "repeatedmessages"
                << "GENERATE_PACKAGE_SUBFOLDERS"
                << "/folder/"
                << QString(extension)
                << "qtprotobufnamespace/tests/"
                << "EXPORT_MACRO=TST_QTPROTOBUFGEN_GEN";

        QTest::addRow("annotation%s", extension.data())
                << "annotation"
                << "COPY_COMMENTS"
                << "/comments/"
                << QString(extension)
                << "" << "";

        QTest::addRow("fieldindexrange%s", extension.data())
                << "fieldindexrange"
                << ""
                << "/fieldenum/"
                << QString(extension)
                << "" << "";

        QTest::addRow("extranamespace%s", extension.data())
                << "extranamespace"
                << "EXTRA_NAMESPACE=MyTopLevelNamespace"
                << "/extra-namespace/"
                << QString(extension)
                << "" << "";
#ifdef HAVE_QML
        QTest::addRow("nopackage%s", extension.data())
                << "nopackage"
                << "QML_URI=nopackage.uri.test;EXPORT_MACRO=TST_QTPROTOBUFGEN_NOPACKAGE_QML_GEN"
                << "/qml-no-package/"
                << QString(extension)
                << "" << "";
#endif
    }
}

void tst_qtprotobufgen::cmdLineGeneratedFile()
{
    QFETCH(QString, fileName);
    QFETCH(QString, generatingOption);
    QFETCH(QString, folder);
    QFETCH(QString, extension);
    QFETCH(QString, generatedFolderStructure);
    QFETCH(QString, exportMacro);

    QProcess process;
    process.setWorkingDirectory(m_commandLineGenerated);
    /* Call command:
         protoc --plugin=protoc-gen-qtprotobuf=<path/to/bin/>qtprotobufgen \
         --qtprotobuf_opt=<option> \
         --qtprotobuf_out=<output_dir> [-I/extra/proto/include/path] <protofile>.proto */
    if (exportMacro.isEmpty()) {
        process.startCommand(protocolBufferCompiler + QString(" ")
                             + protocGenQtprotobufKey + m_protobufgen
                             + optKey + generatingOption
                             + outputKey + m_commandLineGenerated + folder
                             + includeKey + m_protoFiles
                             + " " + fileName + ".proto" + allow_proto3_optional);
    } else {
        process.startCommand(protocolBufferCompiler + QString(" ")
                             + protocGenQtprotobufKey + m_protobufgen
                             + optKey + generatingOption + ";" + exportMacro
                             + outputKey + m_commandLineGenerated + folder
                             + includeKey + m_protoFiles
                             + " " + fileName + ".proto" + allow_proto3_optional);
    }

    QVERIFY2(process.waitForStarted(), msgProcessStartFailed(process).constData());
    if (!process.waitForFinished()) {
        process.kill();
        QFAIL(msgProcessTimeout(process).constData());
    }
    QVERIFY2(process.exitStatus() == QProcess::NormalExit, msgProcessCrashed(process).constData());
    QVERIFY2(process.exitCode() == 0, msgProcessFailed(process).constData());

    const QString filePath = folder + generatedFolderStructure + fileName  + extension;
    compareTwoFiles(m_expectedResult + filePath, m_commandLineGenerated + filePath);
}

void tst_qtprotobufgen::cmdLineGeneratedNoOptions_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("folder");
    QTest::addColumn<QString>("extension");

    const QLatin1StringView extensions[]
            = {cppExtension, headerExtension, cppRegistrationsExtension};

    for (const auto extension : extensions) {
        QTest::addRow("annotation%s", extension.data())
                << "annotation"
                << "/no-options/"
                << QString(extension);

        QTest::addRow("fieldindexrange%s", extension.data())
                << "fieldindexrange"
                << "/no-options/"
                << QString(extension);

        QTest::addRow("extranamespace%s", extension.data())
                << "extranamespace"
                << "/no-options/"
                << QString(extension);

        QTest::addRow("basicmessages%s", extension.data())
                << "basicmessages"
                << "/no-options/"
                << QString(extension);

        QTest::addRow("mapmessages%s", extension.data())
                << "mapmessages"
                << "/no-options/"
                << QString(extension);

        QTest::addRow("oneofmessages%s", extension.data())
                << "oneofmessages"
                << "/no-options/"
                << QString(extension);

        QTest::addRow("optional%s", extension.data())
                << "optional"
                << "/no-options/"
                << QString(extension);

        QTest::addRow("repeatedmessages%s", extension.data())
                << "repeatedmessages"
                << "/no-options/"
                << QString(extension);

        QTest::addRow("repeatednonpackedmessages%s", extension.data())
                << "repeatednonpackedmessages"
                << "/no-options/"
                << QString(extension);

        QTest::addRow("anymessages%s", extension.data())
                << "anymessages"
                << "/no-options/"
                << QString(extension);
    }
}

void tst_qtprotobufgen::cmdLineGeneratedNoOptions()
{
    QFETCH(QString, fileName);
    QFETCH(QString, folder);
    QFETCH(QString, extension);

    QProcess process;
    process.setWorkingDirectory(m_commandLineGenerated);

    /* Call command:
         protoc --plugin=protoc-gen-qtprotobuf=<path/to/bin/>qtprotobufgen \
         --qtprotobuf_out=<output_dir> [-I/extra/proto/include/path] <protofile>.proto */
    process.startCommand(protocolBufferCompiler + QString(" ")
                         + protocGenQtprotobufKey + m_protobufgen
                         + outputKey + m_commandLineGenerated + folder
                         + includeKey + m_protoFiles
                         + " " + fileName + ".proto" + allow_proto3_optional);

    QVERIFY2(process.waitForStarted(), msgProcessStartFailed(process).constData());
    if (!process.waitForFinished()) {
        process.kill();
        QFAIL(msgProcessTimeout(process).constData());
    }
    QVERIFY2(process.exitStatus() == QProcess::NormalExit, msgProcessCrashed(process).constData());
    QVERIFY2(process.exitCode() == 0, msgProcessFailed(process).constData());

    const QString filePath = folder + fileName  + extension;
    compareTwoFiles(m_expectedResult + filePath, m_commandLineGenerated + filePath);
}

void tst_qtprotobufgen::cmdLineInvalidExportMacro_data()
{
    QTest::addColumn<QString>("exportMacro");
    QTest::addColumn<int>("result");

    QTest::addRow("contains_dash") << "TST_QTPROTOBUFGEN-FAIL" << 1;
    QTest::addRow("contains_number_first") << "1Not_ALLoWeD" << 1;
}

void tst_qtprotobufgen::cmdLineInvalidExportMacro()
{
    QFETCH(QString, exportMacro);
    QFETCH(int, result);

    QString folder = "/invalid_export_macro/";
    QString fileName = "basicmessages";
    QString exportMacroCmd = "EXPORT_MACRO=" + exportMacro;

    QProcess process;
    process.setWorkingDirectory(m_commandLineGenerated);
    process.startCommand(protocolBufferCompiler + QString(" ") + protocGenQtprotobufKey
                         + m_protobufgen + optKey + ";" + exportMacroCmd
                         + outputKey + m_commandLineGenerated + folder + includeKey + m_protoFiles
                         + " " + fileName + ".proto" + allow_proto3_optional);
    QVERIFY2(process.waitForStarted(), msgProcessStartFailed(process).constData());
    if (!process.waitForFinished()) {
        process.kill();
        QFAIL(msgProcessTimeout(process).constData());
    }
    QVERIFY2(process.exitStatus() == QProcess::NormalExit, msgProcessCrashed(process).constData());
    QVERIFY2(process.exitCode() == result, msgProcessFailed(process).constData());
}

void tst_qtprotobufgen::cleanupTestCase()
{
    // Leave this function at the bottom. It removes generated content.
    cleanFolder(m_commandLineGenerated);
}

QTEST_MAIN(tst_qtprotobufgen)
#include "tst_qtprotobufgen.moc"
