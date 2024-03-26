// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <qsslkey.h>
#include <qsslsocket.h>
#include <QScopeGuard>
#include <qsslconfiguration.h>
#include <qsslellipticcurve.h>

#include <QtNetwork/qhostaddress.h>
#include <QtNetwork/qnetworkproxy.h>

#include <QtCore/qstring.h>
#include <QtCore/qdebug.h>
#include <QtCore/qlist.h>

#ifdef QT_BUILD_INTERNAL
    #if QT_CONFIG(ssl)
        #include "private/qsslkey_p.h"
        #define TEST_CRYPTO
    #endif
    #ifndef QT_NO_OPENSSL
        #include "../shared/qopenssl_symbols.h"
    #endif
#endif

#if QT_CONFIG(ssl)
#include <QtNetwork/qsslsocket.h>
#endif // QT_CONFIG(ssl)

#include <algorithm>

class tst_QSslKey : public QObject
{
    Q_OBJECT

    struct KeyInfo {
        QFileInfo fileInfo;
        QSsl::KeyAlgorithm algorithm;
        QSsl::KeyType type;
        int length;
        QSsl::EncodingFormat format;
        KeyInfo(
            const QFileInfo &fileInfo, QSsl::KeyAlgorithm algorithm, QSsl::KeyType type,
            int length, QSsl::EncodingFormat format)
            : fileInfo(fileInfo), algorithm(algorithm), type(type), length(length)
            , format(format) {}
    };

    QList<KeyInfo> keyInfoList;

    void createPlainTestRows(bool pemOnly = false);
public:
    tst_QSslKey();

public slots:
    void initTestCase();

#if QT_CONFIG(ssl)

private slots:
    void emptyConstructor();
    void constructor_data();
    void constructor();
#ifndef QT_NO_OPENSSL
    void constructorHandle_data();
    void constructorHandle();
#endif
    void copyAndAssign_data();
    void copyAndAssign();
    void equalsOperator();
    void length_data();
    void length();
    void toPemOrDer_data();
    void toPemOrDer();
    void toEncryptedPemOrDer_data();
    void toEncryptedPemOrDer();

    void passphraseChecks_data();
    void passphraseChecks();
    void noPassphraseChecks();
#ifdef TEST_CRYPTO
    void encrypt_data();
    void encrypt();
#endif

#endif // ssl
private:
    QString testDataDir;

    bool fileContainsUnsupportedEllipticCurve(const QString &fileName) const;
    bool algorithmsSupported(const QString &fileName) const;
    QVector<QString> unsupportedCurves;

    bool isOpenSsl = false;
    bool isOpenSslResolved = false;
    bool isSecureTransport = false;
    bool isSchannel = false;
};

tst_QSslKey::tst_QSslKey()
{
#if QT_CONFIG(ssl)
    const QString expectedCurves[] = {
        // See how we generate them in keys/genkey.sh.
        QStringLiteral("secp224r1"),
        QStringLiteral("prime256v1"),
        QStringLiteral("secp384r1"),
        QStringLiteral("brainpoolP256r1"),
        QStringLiteral("brainpoolP384r1"),
        QStringLiteral("brainpoolP512r1")
    };
    const auto supportedCurves = QSslConfiguration::supportedEllipticCurves();

    for (const auto &requestedEc : expectedCurves) {
        auto pos = std::find_if(supportedCurves.begin(), supportedCurves.end(),
                     [&requestedEc](const auto &supported) {
            return requestedEc == supported.shortName();
        });
        if (pos == supportedCurves.end()) {
            qWarning() << "EC with the name:" << requestedEc
                       << "is not supported by your build of OpenSSL and will not be tested.";
            unsupportedCurves.push_back(requestedEc);
        }
    }
    // Alas, we don't use network-private (and why?).
    const auto backendName = QSslSocket::activeBackend();
    isOpenSsl = backendName == QStringLiteral("openssl");

    if (isOpenSsl) {
#if !defined(QT_NO_OPENSSL) && defined(QT_BUILD_INTERNAL)
        isOpenSslResolved = qt_auto_test_resolve_OpenSSL_symbols();
#else
        isOpenSslResolved = false; // not 'unused variable' anymore.
#endif
    } else {
        isSecureTransport = backendName == QStringLiteral("securetransport");
    }

    if (!isOpenSsl && !isSecureTransport)
        isSchannel = backendName == QStringLiteral("schannel");
#else
    unsupportedCurves = {}; // not unsued anymore.
#endif
}

bool tst_QSslKey::fileContainsUnsupportedEllipticCurve(const QString &fileName) const
{
    for (const auto &name : unsupportedCurves) {
        if (fileName.contains(name))
            return true;
    }
    return false;
}

bool tst_QSslKey::algorithmsSupported(const QString &fileName) const
{
#if QT_CONFIG(ssl)
    if (isSchannel && fileName.contains("RC2-64")) // Schannel treats RC2 as 128 bit
        return false;

    if (isSchannel || isSecureTransport) {
        // No AES support in the generic back-end, PKCS#12 algorithms not supported either.
        return !(fileName.contains(QRegularExpression("-aes\\d\\d\\d-")) || fileName.contains("pkcs8-pkcs12"));
    }

    if (!isOpenSsl || QSslSocket::sslLibraryVersionNumber() >> 28 < 3)
        return true;

    // OpenSSL v3 first introduced the notion of 'providers'. Many algorithms
    // were moved into the 'legacy' provider. While they are still supported in theory,
    // the 'legacy' provider is NOT loaded by default and we are not loading it either.
    // Thus, some of the keys we are using in tst_QSslKey would fail the test. We
    // have to filter them out.
    const auto name = fileName.toLower();
    if (name.contains("-des."))
        return false;

    return !name.contains("-rc2-") && !name.contains("-rc4-");
#else
    Q_UNUSED(fileName);
    return false;
#endif // QT_CONFIG(ssl)
}


void tst_QSslKey::initTestCase()
{
    testDataDir = QFileInfo(QFINDTESTDATA("rsa-without-passphrase.pem")).absolutePath();
    if (testDataDir.isEmpty())
        testDataDir = QCoreApplication::applicationDirPath();
    if (!testDataDir.endsWith(QLatin1String("/")))
        testDataDir += QLatin1String("/");

    QDir dir(testDataDir + "keys");
    const QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files | QDir::Readable);
    QRegularExpression rx(QLatin1String("^(rsa|dsa|dh|ec)-(pub|pri)-(\\d+)-?[\\w-]*\\.(pem|der)$"));
    for (const QFileInfo &fileInfo : fileInfoList) {
        if (fileContainsUnsupportedEllipticCurve(fileInfo.fileName()))
            continue;
        auto match = rx.match(fileInfo.fileName());
        if (match.hasMatch()) {
            keyInfoList << KeyInfo(
                fileInfo,
                match.captured(1) == QLatin1String("rsa") ? QSsl::Rsa :
                match.captured(1) == QLatin1String("dsa") ? QSsl::Dsa :
                match.captured(1) == QLatin1String("dh") ? QSsl::Dh : QSsl::Ec,
                match.captured(2) == QLatin1String("pub") ? QSsl::PublicKey : QSsl::PrivateKey,
                match.captured(3).toInt(),
                match.captured(4) == QLatin1String("pem") ? QSsl::Pem : QSsl::Der);
        }
    }
}

#if QT_CONFIG(ssl)

static QByteArray readFile(const QString &absFilePath)
{
    QFile file(absFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("failed to open file");
        return QByteArray();
    }
    return file.readAll();
}

void tst_QSslKey::emptyConstructor()
{
    if (!QSslSocket::supportsSsl())
        return;

    QSslKey key;
    QVERIFY(key.isNull());
    QVERIFY(key.length() < 0);

    QSslKey key2;
    QCOMPARE(key, key2);
}

Q_DECLARE_METATYPE(QSsl::KeyAlgorithm)
Q_DECLARE_METATYPE(QSsl::KeyType)
Q_DECLARE_METATYPE(QSsl::EncodingFormat)

void tst_QSslKey::createPlainTestRows(bool pemOnly)
{
    QTest::addColumn<QString>("absFilePath");
    QTest::addColumn<QSsl::KeyAlgorithm>("algorithm");
    QTest::addColumn<QSsl::KeyType>("type");
    QTest::addColumn<int>("length");
    QTest::addColumn<QSsl::EncodingFormat>("format");
    foreach (KeyInfo keyInfo, keyInfoList) {
        if (pemOnly && keyInfo.format != QSsl::EncodingFormat::Pem)
            continue;

        if (!algorithmsSupported(keyInfo.fileInfo.fileName()))
            continue;

        QTest::newRow(keyInfo.fileInfo.fileName().toLatin1())
            << keyInfo.fileInfo.absoluteFilePath() << keyInfo.algorithm << keyInfo.type
            << keyInfo.length << keyInfo.format;
    }
}

void tst_QSslKey::constructor_data()
{
    createPlainTestRows();
}

void tst_QSslKey::constructor()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH(QString, absFilePath);
    QFETCH(QSsl::KeyAlgorithm, algorithm);
    QFETCH(QSsl::KeyType, type);
    QFETCH(QSsl::EncodingFormat, format);

    QByteArray encoded = readFile(absFilePath);
    QByteArray passphrase;
    if (QByteArray(QTest::currentDataTag()).contains("-pkcs8-"))
        passphrase = QByteArray("1234");
    QSslKey key(encoded, algorithm, format, type, passphrase);
    QVERIFY(!key.isNull());
}

#ifndef QT_NO_OPENSSL

void tst_QSslKey::constructorHandle_data()
{
    createPlainTestRows(true);
}

void tst_QSslKey::constructorHandle()
{
#ifndef QT_BUILD_INTERNAL
    QSKIP("This test requires -developer-build.");
#else
    if (!isOpenSslResolved)
        return;

    QFETCH(QString, absFilePath);
    QFETCH(QSsl::KeyAlgorithm, algorithm);
    QFETCH(QSsl::KeyType, type);
    QFETCH(int, length);

    QByteArray pem = readFile(absFilePath);
    auto func = (type == QSsl::KeyType::PublicKey
                 ? q_PEM_read_bio_PUBKEY
                 : q_PEM_read_bio_PrivateKey);

    QByteArray passphrase;
    if (QByteArray(QTest::currentDataTag()).contains("-pkcs8-"))
        passphrase = "1234";

    BIO* bio = q_BIO_new(q_BIO_s_mem());
    q_BIO_write(bio, pem.constData(), pem.size());
    EVP_PKEY *origin = func(bio, nullptr, nullptr, static_cast<void *>(passphrase.data()));
    Q_ASSERT(origin);
    q_EVP_PKEY_up_ref(origin);
    QSslKey key(origin, type);
    q_BIO_free(bio);

    EVP_PKEY *handle = q_EVP_PKEY_new();
    switch (algorithm) {
    case QSsl::Rsa:
        q_EVP_PKEY_set1_RSA(handle, static_cast<RSA *>(key.handle()));
        break;
    case QSsl::Dsa:
        q_EVP_PKEY_set1_DSA(handle, static_cast<DSA *>(key.handle()));
        break;
    case QSsl::Dh:
        q_EVP_PKEY_set1_DH(handle, static_cast<DH *>(key.handle()));
        break;
#ifndef OPENSSL_NO_EC
    case QSsl::Ec:
        q_EVP_PKEY_set1_EC_KEY(handle, static_cast<EC_KEY *>(key.handle()));
        break;
#endif
    default:
        break;
    }

    auto cleanup = qScopeGuard([origin, handle] {
        q_EVP_PKEY_free(origin);
        q_EVP_PKEY_free(handle);
    });

    QVERIFY(!key.isNull());
    QCOMPARE(key.algorithm(), algorithm);
    QCOMPARE(key.type(), type);
    QCOMPARE(key.length(), length);
    QCOMPARE(q_EVP_PKEY_cmp(origin, handle), 1);
#endif
}

#endif // !QT_NO_OPENSSL

void tst_QSslKey::copyAndAssign_data()
{
    createPlainTestRows();
}

void tst_QSslKey::copyAndAssign()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH(QString, absFilePath);
    QFETCH(QSsl::KeyAlgorithm, algorithm);
    QFETCH(QSsl::KeyType, type);
    QFETCH(QSsl::EncodingFormat, format);

    QByteArray encoded = readFile(absFilePath);
    QByteArray passphrase;
    if (QByteArray(QTest::currentDataTag()).contains("-pkcs8-"))
        passphrase = QByteArray("1234");
    QSslKey key(encoded, algorithm, format, type, passphrase);

    QSslKey copied(key);
    QCOMPARE(key, copied);
    QCOMPARE(key.algorithm(), copied.algorithm());
    QCOMPARE(key.type(), copied.type());
    QCOMPARE(key.length(), copied.length());
    QCOMPARE(key.toPem(), copied.toPem());
    QCOMPARE(key.toDer(), copied.toDer());

    QSslKey assigned = key;
    QCOMPARE(key, assigned);
    QCOMPARE(key.algorithm(), assigned.algorithm());
    QCOMPARE(key.type(), assigned.type());
    QCOMPARE(key.length(), assigned.length());
    QCOMPARE(key.toPem(), assigned.toPem());
    QCOMPARE(key.toDer(), assigned.toDer());
}

void tst_QSslKey::equalsOperator()
{
    // ### unimplemented
}

void tst_QSslKey::length_data()
{
    createPlainTestRows();
}

void tst_QSslKey::length()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH(QString, absFilePath);
    QFETCH(QSsl::KeyAlgorithm, algorithm);
    QFETCH(QSsl::KeyType, type);
    QFETCH(int, length);
    QFETCH(QSsl::EncodingFormat, format);

    QByteArray encoded = readFile(absFilePath);
    QByteArray passphrase;
    if (QByteArray(QTest::currentDataTag()).contains("-pkcs8-"))
        passphrase = QByteArray("1234");
    QSslKey key(encoded, algorithm, format, type, passphrase);
    QVERIFY(!key.isNull());
    QCOMPARE(key.length(), length);
}

void tst_QSslKey::toPemOrDer_data()
{
    createPlainTestRows();
}

void tst_QSslKey::toPemOrDer()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH(QString, absFilePath);
    QFETCH(QSsl::KeyAlgorithm, algorithm);
    QFETCH(QSsl::KeyType, type);
    QFETCH(QSsl::EncodingFormat, format);

    QByteArray dataTag = QByteArray(QTest::currentDataTag());
    if (dataTag.contains("-pkcs8-")) // these are encrypted
        QSKIP("Encrypted PKCS#8 keys gets decrypted when loaded. So we can't compare it to the encrypted version.");

    if (dataTag.contains("pkcs8")) {
        if (isOpenSsl)
            QSKIP("OpenSSL converts PKCS#8 keys to other formats, invalidating comparisons.");
        else if (dataTag.contains("rsa"))
            QSKIP("PKCS#8 RSA keys are changed into a different format in the generic back-end, meaning the comparison fails.");
    }

    QByteArray encoded = readFile(absFilePath);
    QSslKey key(encoded, algorithm, format, type);
    QVERIFY(!key.isNull());
    if (format == QSsl::Pem)
        encoded.replace('\r', "");
    QCOMPARE(format == QSsl::Pem ? key.toPem() : key.toDer(), encoded);
}

void tst_QSslKey::toEncryptedPemOrDer_data()
{
    QTest::addColumn<QString>("absFilePath");
    QTest::addColumn<QSsl::KeyAlgorithm>("algorithm");
    QTest::addColumn<QSsl::KeyType>("type");
    QTest::addColumn<QSsl::EncodingFormat>("format");
    QTest::addColumn<QString>("password");

    QStringList passwords;
    passwords << " " << "foobar" << "foo bar"
              << "aAzZ`1234567890-=~!@#$%^&*()_+[]{}\\|;:'\",.<>/?"; // ### add more (?)
    foreach (KeyInfo keyInfo, keyInfoList) {
        if (keyInfo.fileInfo.fileName().contains("pkcs8"))
            continue; // pkcs8 keys are encrypted in a different way than the other keys
        foreach (QString password, passwords) {
            const QByteArray testName = keyInfo.fileInfo.fileName().toLatin1()
            + '-' + (keyInfo.algorithm == QSsl::Rsa ? "RSA" :
                                                      (keyInfo.algorithm == QSsl::Dsa ? "DSA" : "EC"))
            + '-' + (keyInfo.type == QSsl::PrivateKey ? "PrivateKey" : "PublicKey")
            + '-' + (keyInfo.format == QSsl::Pem ? "PEM" : "DER")
            + password.toLatin1();
            QTest::newRow(testName.constData())
                << keyInfo.fileInfo.absoluteFilePath() << keyInfo.algorithm << keyInfo.type
                << keyInfo.format << password;
        }
    }
}

void tst_QSslKey::toEncryptedPemOrDer()
{
    if (!QSslSocket::supportsSsl())
        return;

    QFETCH(QString, absFilePath);
    QFETCH(QSsl::KeyAlgorithm, algorithm);
    QFETCH(QSsl::KeyType, type);
    QFETCH(QSsl::EncodingFormat, format);
    QFETCH(QString, password);

    QByteArray plain = readFile(absFilePath);
    QSslKey key(plain, algorithm, format, type);
    QVERIFY(!key.isNull());

    QByteArray pwBytes(password.toLatin1());

    if (type == QSsl::PrivateKey) {
        QByteArray encryptedPem = key.toPem(pwBytes);
        QVERIFY(!encryptedPem.isEmpty());
        QSslKey keyPem(encryptedPem, algorithm, QSsl::Pem, type, pwBytes);
        QVERIFY(!keyPem.isNull());
        QCOMPARE(keyPem, key);
        QCOMPARE(keyPem.toPem(), key.toPem());
    } else {
        // verify that public keys are never encrypted by toPem()
        QByteArray encryptedPem = key.toPem(pwBytes);
        QVERIFY(!encryptedPem.isEmpty());
        QByteArray plainPem = key.toPem();
        QVERIFY(!plainPem.isEmpty());
        QCOMPARE(encryptedPem, plainPem);
    }

    if (type == QSsl::PrivateKey) {
        // verify that private keys are never "encrypted" by toDer() and
        // instead an empty string is returned, see QTBUG-41038.
        QByteArray encryptedDer = key.toDer(pwBytes);
        QVERIFY(encryptedDer.isEmpty());
    } else {
        // verify that public keys are never encrypted by toDer()
        QByteArray encryptedDer = key.toDer(pwBytes);
        QVERIFY(!encryptedDer.isEmpty());
        QByteArray plainDer = key.toDer();
        QVERIFY(!plainDer.isEmpty());
        QCOMPARE(encryptedDer, plainDer);
    }

    // ### add a test to verify that public keys are _decrypted_ correctly (by the ctor)
}

void tst_QSslKey::passphraseChecks_data()
{
    if (!QSslSocket::supportsSsl())
        QSKIP("This test requires a working TLS library");

    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QByteArray>("passphrase");

    const QByteArray pass("123");
    const QByteArray aesPass("1234");

    if (!isOpenSsl || QSslSocket::sslLibraryVersionNumber() >> 28 < 3) {
        // DES and RC2 are not provided by default in OpenSSL v3.
        // This part is for either non-OpenSSL build, or OpenSSL v < 3.x.
        QTest::newRow("DES") << QString(testDataDir + "rsa-with-passphrase-des.pem") << pass;
        QTest::newRow("RC2") << QString(testDataDir + "rsa-with-passphrase-rc2.pem") << pass;
    }

    QTest::newRow("3DES") << QString(testDataDir + "rsa-with-passphrase-3des.pem") << pass;

#if defined(QT_NO_OPENSSL) || !defined(OPENSSL_NO_AES)
    QTest::newRow("AES128") << QString(testDataDir + "rsa-with-passphrase-aes128.pem") << aesPass;
    QTest::newRow("AES192") << QString(testDataDir + "rsa-with-passphrase-aes192.pem") << aesPass;
    QTest::newRow("AES256") << QString(testDataDir + "rsa-with-passphrase-aes256.pem") << aesPass;
#endif // Generic backend || OpenSSL built with AES
}

void tst_QSslKey::passphraseChecks()
{
    QFETCH(QString, fileName);
    QFETCH(QByteArray, passphrase);

    QFile keyFile(fileName);
    QVERIFY(keyFile.exists());
    {
        if (!keyFile.isOpen())
            keyFile.open(QIODevice::ReadOnly);
        else
            keyFile.reset();
        QSslKey key(&keyFile,QSsl::Rsa,QSsl::Pem, QSsl::PrivateKey);
        QVERIFY(key.isNull()); // null passphrase => should not be able to decode key
    }
    {
        if (!keyFile.isOpen())
            keyFile.open(QIODevice::ReadOnly);
        else
            keyFile.reset();
        QSslKey key(&keyFile,QSsl::Rsa,QSsl::Pem, QSsl::PrivateKey, "");
        QVERIFY(key.isNull()); // empty passphrase => should not be able to decode key
    }
    {
        if (!keyFile.isOpen())
            keyFile.open(QIODevice::ReadOnly);
        else
            keyFile.reset();
        QSslKey key(&keyFile,QSsl::Rsa,QSsl::Pem, QSsl::PrivateKey, "WRONG!");
        QVERIFY(key.isNull()); // wrong passphrase => should not be able to decode key
    }
    {
        if (!keyFile.isOpen())
            keyFile.open(QIODevice::ReadOnly);
        else
            keyFile.reset();
        QSslKey key(&keyFile,QSsl::Rsa,QSsl::Pem, QSsl::PrivateKey, passphrase);
        QVERIFY(!key.isNull()); // correct passphrase
    }
}

void tst_QSslKey::noPassphraseChecks()
{
    if (!QSslSocket::supportsSsl())
        QSKIP("This test requires a working TLS library");

    // be sure and check a key without passphrase too
    QString fileName(testDataDir + "rsa-without-passphrase.pem");
    QFile keyFile(fileName);
    {
        if (!keyFile.isOpen())
            keyFile.open(QIODevice::ReadOnly);
        else
            keyFile.reset();
        QSslKey key(&keyFile,QSsl::Rsa,QSsl::Pem, QSsl::PrivateKey);
        QVERIFY(!key.isNull()); // null passphrase => should be able to decode key
    }
    {
        if (!keyFile.isOpen())
            keyFile.open(QIODevice::ReadOnly);
        else
            keyFile.reset();
        QSslKey key(&keyFile,QSsl::Rsa,QSsl::Pem, QSsl::PrivateKey, "");
        QVERIFY(!key.isNull()); // empty passphrase => should be able to decode key
    }
    {
        if (!keyFile.isOpen())
            keyFile.open(QIODevice::ReadOnly);
        else
            keyFile.reset();
        QSslKey key(&keyFile,QSsl::Rsa,QSsl::Pem, QSsl::PrivateKey, "xxx");
        QVERIFY(!key.isNull()); // passphrase given but key is not encrypted anyway => should work
    }
}

#ifdef TEST_CRYPTO
Q_DECLARE_METATYPE(QSslKeyPrivate::Cipher)

void tst_QSslKey::encrypt_data()
{
    using QTlsPrivate::Cipher;

    QTest::addColumn<Cipher>("cipher");
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("plainText");
    QTest::addColumn<QByteArray>("cipherText");
    QTest::addColumn<QByteArray>("iv");

    QByteArray iv("abcdefgh");
#if OPENSSL_VERSION_MAJOR < 3
    // Either non-OpenSSL build, or OpenSSL v < 3
    // (with DES and other legacy algorithms available by default)
    QTest::newRow("DES-CBC, length 0")
        << Cipher::DesCbc << QByteArray("01234567")
        << QByteArray()
        << QByteArray::fromHex("956585228BAF9B1F")
        << iv;
    QTest::newRow("DES-CBC, length 1")
        << Cipher::DesCbc << QByteArray("01234567")
        << QByteArray(1, 'a')
        << QByteArray::fromHex("E6880AF202BA3C12")
        << iv;
    QTest::newRow("DES-CBC, length 2")
        << Cipher::DesCbc << QByteArray("01234567")
        << QByteArray(2, 'a')
        << QByteArray::fromHex("A82492386EED6026")
        << iv;
    QTest::newRow("DES-CBC, length 3")
        << Cipher::DesCbc << QByteArray("01234567")
        << QByteArray(3, 'a')
        << QByteArray::fromHex("90B76D5B79519CBA")
        << iv;
    QTest::newRow("DES-CBC, length 4")
        << Cipher::DesCbc << QByteArray("01234567")
        << QByteArray(4, 'a')
        << QByteArray::fromHex("63E3DD6FED87052A")
        << iv;
    QTest::newRow("DES-CBC, length 5")
        << Cipher::DesCbc << QByteArray("01234567")
        << QByteArray(5, 'a')
        << QByteArray::fromHex("03ACDB0EACBDFA94")
        << iv;
    QTest::newRow("DES-CBC, length 6")
        << Cipher::DesCbc << QByteArray("01234567")
        << QByteArray(6, 'a')
        << QByteArray::fromHex("7D95024E42A3A88A")
        << iv;
    QTest::newRow("DES-CBC, length 7")
        << Cipher::DesCbc << QByteArray("01234567")
        << QByteArray(7, 'a')
        << QByteArray::fromHex("5003436B8A8E42E9")
        << iv;
    QTest::newRow("DES-CBC, length 8")
        << Cipher::DesCbc << QByteArray("01234567")
        << QByteArray(8, 'a')
        << QByteArray::fromHex("E4C1F054BF5521C0A4A0FD4A2BC6C1B1")
        << iv;

    QTest::newRow("DES-EDE3-CBC, length 0")
        << Cipher::DesEde3Cbc << QByteArray("0123456789abcdefghijklmn")
        << QByteArray()
        << QByteArray::fromHex("3B2B4CD0B0FD495F")
        << iv;
    QTest::newRow("DES-EDE3-CBC, length 8")
        << Cipher::DesEde3Cbc << QByteArray("0123456789abcdefghijklmn")
        << QByteArray(8, 'a')
        << QByteArray::fromHex("F2A5A87763C54A72A3224103D90CDB03")
        << iv;

    QTest::newRow("RC2-40-CBC, length 0")
        << Cipher::Rc2Cbc << QByteArray("01234")
        << QByteArray()
        << QByteArray::fromHex("6D05D52392FF6E7A")
        << iv;
    QTest::newRow("RC2-40-CBC, length 8")
        << Cipher::Rc2Cbc << QByteArray("01234")
        << QByteArray(8, 'a')
        << QByteArray::fromHex("75768E64C5749072A5D168F3AFEB0005")
        << iv;

    QTest::newRow("RC2-64-CBC, length 0")
        << Cipher::Rc2Cbc << QByteArray("01234567")
        << QByteArray()
        << QByteArray::fromHex("ADAE6BF70F420130")
        << iv;
    QTest::newRow("RC2-64-CBC, length 8")
        << Cipher::Rc2Cbc << QByteArray("01234567")
        << QByteArray(8, 'a')
        << QByteArray::fromHex("C7BF5C80AFBE9FBEFBBB9FD935F6D0DF")
        << iv;

    QTest::newRow("RC2-128-CBC, length 0")
        << Cipher::Rc2Cbc << QByteArray("012345679abcdefg")
        << QByteArray()
        << QByteArray::fromHex("1E965D483A13C8FB")
        << iv;
    QTest::newRow("RC2-128-CBC, length 8")
        << Cipher::Rc2Cbc << QByteArray("012345679abcdefg")
        << QByteArray(8, 'a')
        << QByteArray::fromHex("5AEC1A5B295660B02613454232F7DECE")
        << iv;
#endif // OPENSSL_VERSION_MAJOR

#if defined(QT_NO_OPENSSL) || !defined(OPENSSL_NO_AES)
    // AES needs a longer IV
    iv = QByteArray("abcdefghijklmnop");
    QTest::newRow("AES-128-CBC, length 0")
        << Cipher::Aes128Cbc << QByteArray("012345679abcdefg")
        << QByteArray()
        << QByteArray::fromHex("28DE1A9AA26601C30DD2527407121D1A")
        << iv;
    QTest::newRow("AES-128-CBC, length 8")
        << Cipher::Aes128Cbc << QByteArray("012345679abcdefg")
        << QByteArray(8, 'a')
        << QByteArray::fromHex("08E880B1BA916F061C1E801D7F44D0EC")
        << iv;

    QTest::newRow("AES-192-CBC, length 0")
        << Cipher::Aes192Cbc << QByteArray("0123456789abcdefghijklmn")
        << QByteArray()
        << QByteArray::fromHex("E169E0E205CDC2BA895B7CF6097673B1")
        << iv;
    QTest::newRow("AES-192-CBC, length 8")
        << Cipher::Aes192Cbc << QByteArray("0123456789abcdefghijklmn")
        << QByteArray(8, 'a')
        << QByteArray::fromHex("3A227D6A3A13237316D30AA17FF9B0A7")
        << iv;

    QTest::newRow("AES-256-CBC, length 0")
        << Cipher::Aes256Cbc << QByteArray("0123456789abcdefghijklmnopqrstuv")
        << QByteArray()
        << QByteArray::fromHex("4BAACAA0D22199C97DE206C465B7B14A")
        << iv;
    QTest::newRow("AES-256-CBC, length 8")
        << Cipher::Aes256Cbc << QByteArray("0123456789abcdefghijklmnopqrstuv")
        << QByteArray(8, 'a')
        << QByteArray::fromHex("879C8C25EC135CDF0B14490A0A7C2F67")
        << iv;
#endif // Generic backend || OpenSSL built with AES
}

void tst_QSslKey::encrypt()
{
    QFETCH(QSslKeyPrivate::Cipher, cipher);
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, plainText);
    QFETCH(QByteArray, cipherText);
    QFETCH(QByteArray, iv);

    if (isSchannel) {
        QEXPECT_FAIL("RC2-40-CBC, length 0", "Schannel treats RC2 as 128-bit", Abort);
        QEXPECT_FAIL("RC2-40-CBC, length 8", "Schannel treats RC2 as 128-bit", Abort);
        QEXPECT_FAIL("RC2-64-CBC, length 0", "Schannel treats RC2 as 128-bit", Abort);
        QEXPECT_FAIL("RC2-64-CBC, length 8", "Schannel treats RC2 as 128-bit", Abort);
    }

    QByteArray encrypted = QSslKeyPrivate::encrypt(cipher, plainText, key, iv);
    QCOMPARE(encrypted, cipherText);

    QByteArray decrypted = QSslKeyPrivate::decrypt(cipher, cipherText, key, iv);
    QCOMPARE(decrypted, plainText);
}
#endif // TEST_CRYPTO

#endif // ssl

QTEST_MAIN(tst_QSslKey)
#include "tst_qsslkey.moc"
