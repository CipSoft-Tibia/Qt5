// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <QString>
#include <QProcess>
#include <QCryptographicHash>
#include <qtprotobuftypes.h>

#define XSTR(x) DEFSTR(x)
#define DEFSTR(x) #x

using namespace Qt::StringLiterals;
const QLatin1StringView cppProtobufGenExtension(".qpb.cpp");
const QLatin1StringView headerProtobufGenExtension(".qpb.h");
const QLatin1StringView cppExtension("_client.grpc.qpb.cpp");
const QLatin1StringView headerExtension("_client.grpc.qpb.h");
const QLatin1StringView grpcGenQtprotobufKey(" --plugin=protoc-gen-qtgrpc=");
const QLatin1StringView optKey(" --qtgrpc_opt=");
const QLatin1StringView outputKey(" --qtgrpc_out=");
const QLatin1StringView includeKey(" -I");
#ifndef PROTOC_EXECUTABLE
#  error PROTOC_EXECUTABLE definition must be set and point to the valid protoc executable
#endif
const QLatin1StringView protocolBufferCompiler(XSTR(PROTOC_EXECUTABLE));
#if defined(Q_OS_WIN)
const QLatin1StringView qtgrpcgen("/qtgrpcgen.exe");
#else
const QLatin1StringView qtgrpcgen("/qtgrpcgen");
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

class tst_qtgrpcgen : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    //! Test qt_add_grpc() cmake function
    void cmakeGeneratedFile_data();
    void cmakeGeneratedFile();

    //! Test command-line call of qtgrpcgen
    void cmdLineGeneratedFile_data();
    void cmdLineGeneratedFile();

    void cleanupTestCase();

private:
    QString m_grpcgen;
    QString m_cmakeGenerated;
    QString m_qmlCmakeGenerated;
    QString m_commandLineGenerated;
    QString m_expectedResult;
    QString m_protoFiles;
};

void tst_qtgrpcgen::initTestCase()
{
    m_grpcgen = QLibraryInfo::path(QLibraryInfo::LibraryExecutablesPath) + qtgrpcgen;

    m_cmakeGenerated = QFINDTESTDATA("qt_grpc_generated");
    QVERIFY(!m_cmakeGenerated.isEmpty());

#ifdef HAVE_QML
    m_qmlCmakeGenerated = QFINDTESTDATA("qt_grpc_generated_qml");
    QVERIFY(!m_qmlCmakeGenerated.isEmpty());
#endif

    m_expectedResult = QFINDTESTDATA("data/expected_result");
    QVERIFY(!m_expectedResult.isEmpty());

    m_protoFiles = QFINDTESTDATA("../shared/data/proto/");
    QVERIFY(!m_protoFiles.isEmpty());

    QDir testOutputBaseDir(QCoreApplication::applicationDirPath());
    testOutputBaseDir.mkdir(QLatin1StringView("cmd_line_generation"));
    QLatin1StringView folders[] = {"comments"_L1, "extra-namespace"_L1,
                                   "fieldenum"_L1, "folder"_L1, "no-options"_L1};
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

void tst_qtgrpcgen::cmakeGeneratedFile_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("folder");
    QTest::addColumn<QString>("extension");
    QTest::addColumn<QString>("cmakeGenerationFolder");

    const QLatin1StringView protobufExtensions[] = { cppProtobufGenExtension,
                                                     headerProtobufGenExtension };

    const QLatin1StringView grpcExtensions[] = { cppExtension, headerExtension };

    for (const auto extension : grpcExtensions) {
        QTest::addRow("testservice%s", extension.data())
                << "testservice"
                << "/folder/qtgrpc/tests/"
                << QString(extension)
                << m_cmakeGenerated;

        QTest::addRow("separate/grpc/testservice%s", extension.data())
            << "testservice"
            << "/separate/grpc/qtgrpc/tests/" << QString(extension) << m_cmakeGenerated;
    }

    for (const auto extension : protobufExtensions) {
        QTest::addRow("testservice%s", extension.data())
            << "testservice"
            << "/folder/qtgrpc/tests/" << QString(extension) << m_cmakeGenerated;

        QTest::addRow("separate/protobuf/testservice%s", extension.data())
            << "testservice"
            << "/separate/protobuf/qtgrpc/tests/" << QString(extension) << m_cmakeGenerated;
    }

    QTest::addRow("tst_qtgrpcgen_client_grpc_only_exports.qpb.h")
        << "tst_qtgrpcgen_client_grpc_only_exports.qpb.h"
        << "/separate/grpc/" << QString() << m_cmakeGenerated;

#ifdef HAVE_QML
    const QLatin1StringView qmlExtensions[] = { cppExtension,
                                                headerExtension };

    for (const auto extension : qmlExtensions) {
        QTest::addRow("qmltestservice%s", extension.data())
                << "qmltestservice"
                << "/qml/"
                << QString(extension)
                << m_qmlCmakeGenerated;
    }
#endif
}

void tst_qtgrpcgen::cmakeGeneratedFile()
{
    QFETCH(QString, fileName);
    QFETCH(QString, folder);
    QFETCH(QString, extension);
    QFETCH(QString, cmakeGenerationFolder);

    QFile expectedResultFile(m_expectedResult + folder + fileName + extension);
    QFile generatedFile(cmakeGenerationFolder + folder + fileName + extension);

    QVERIFY2(expectedResultFile.exists(), qPrintable(expectedResultFile.fileName()));
    QVERIFY(generatedFile.exists());

    QVERIFY2(expectedResultFile.open(QIODevice::ReadOnly | QIODevice::Text),
             msgCannotReadFile(expectedResultFile).constData());
    QVERIFY2(generatedFile.open(QIODevice::ReadOnly | QIODevice::Text),
             msgCannotReadFile(generatedFile).constData());

    QByteArray expectedData = expectedResultFile.readAll();
    QByteArray generatedData = generatedFile.readAll();

    expectedResultFile.close();
    generatedFile.close();

    if (hash(expectedData).toHex() != hash(generatedData).toHex())
    {
        const QString diff = doCompare(splitToLines(generatedData),
                                       splitToLines(expectedData));
        QCOMPARE_GT(diff.size(), 0); // Hashes can only differ if content does.
        QFAIL(qPrintable(diff));
    }
    // Ensure we do see a failure, even in the unlikely case of a hash collision:
    QVERIFY(generatedData == expectedData);
}

void tst_qtgrpcgen::cmdLineGeneratedFile_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("folder");
    QTest::addColumn<QString>("extension");

    const QLatin1StringView extensions[] = { cppExtension, headerExtension };

    for (const auto extension : extensions) {
        QTest::addRow("testservice%s", extension.data())
                << "testservice"
                << "/no-options/"
                << QString(extension);
        QTest::addRow("testserivcenomessages%s", extension.data())
                << "testserivcenomessages"
                << "/no-options/"
                << QString(extension);
    }
}

void tst_qtgrpcgen::cmdLineGeneratedFile()
{
    QFETCH(QString, fileName);
    QFETCH(QString, folder);
    QFETCH(QString, extension);

    QProcess process;
    process.setWorkingDirectory(m_commandLineGenerated);

    /* Call command:
         protoc --plugin=protoc-gen-qtgrpc=<path/to/bin/>qtgrpcgen \
         --qtgrpc_opt=<option> \
         --qtgrpc_out=<output_dir> [-I/extra/proto/include/path] <protofile>.proto */
    process.startCommand(protocolBufferCompiler + QString(" ")
                         + grpcGenQtprotobufKey + m_grpcgen
                         + optKey + outputKey
                         + m_commandLineGenerated + folder
                         + includeKey + m_protoFiles
                         + " " + fileName + ".proto");

    QVERIFY2(process.waitForStarted(), msgProcessStartFailed(process).constData());
    if (!process.waitForFinished()) {
        process.kill();
        QFAIL(msgProcessTimeout(process).constData());
    }
    QVERIFY2(process.exitStatus() == QProcess::NormalExit, msgProcessCrashed(process).constData());
    QVERIFY2(process.exitCode() == 0, msgProcessFailed(process).constData());

    QFile expectedResultFile(m_expectedResult + folder + fileName  + extension);
    QFile generatedFile(m_commandLineGenerated + folder + fileName + extension);

    QVERIFY(generatedFile.exists());
    QVERIFY(expectedResultFile.exists());

    QVERIFY2(expectedResultFile.open(QIODevice::ReadOnly | QIODevice::Text),
             msgCannotReadFile(expectedResultFile).constData());
    QVERIFY2(generatedFile.open(QIODevice::ReadOnly | QIODevice::Text),
             msgCannotReadFile(generatedFile).constData());

    QByteArray expectedData = expectedResultFile.readAll();
    QByteArray generatedData = generatedFile.readAll();

    expectedResultFile.close();
    generatedFile.close();

    if (hash(expectedData).toHex() != hash(generatedData).toHex())
    {
        const QString diff = doCompare(splitToLines(generatedData),
                                       splitToLines(expectedData));
        QCOMPARE_GT(diff.size(), 0); // Hashes can only differ if content does.
        QFAIL(qPrintable(diff));
    }
    // Ensure we do see a failure, even in the unlikely case of a hash collision:
    QVERIFY(generatedData == expectedData);
}

void tst_qtgrpcgen::cleanupTestCase()
{
    // Leave this function at the bottom. It removes generated content.
    cleanFolder(m_commandLineGenerated);
}

QTEST_MAIN(tst_qtgrpcgen)
#include "tst_qtgrpcgen.moc"
