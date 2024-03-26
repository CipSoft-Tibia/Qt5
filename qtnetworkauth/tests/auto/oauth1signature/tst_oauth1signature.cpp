// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore>
#include <QtTest>

#include <QtNetworkAuth/qoauth1signature.h>

Q_DECLARE_METATYPE(QOAuth1Signature::HttpRequestMethod)

class tst_OAuth1Signature : public QObject
{
    Q_OBJECT

public:
    QOAuth1Signature createTwitterSignature();

private Q_SLOTS:
    void twitterSignatureExample();
    void copyAndModify();

    void signatures_data();
    void signatures();
};

const auto oauthVersion = QStringLiteral("oauth_version");
const auto oauthConsumerKey = QStringLiteral("oauth_consumer_key");
const auto oauthNonce = QStringLiteral("oauth_nonce");
const auto oauthSignatureMethod = QStringLiteral("oauth_signature_method");
const auto oauthTimestamp = QStringLiteral("oauth_timestamp");
const auto oauthToken = QStringLiteral("oauth_token");

QOAuth1Signature tst_OAuth1Signature::createTwitterSignature()
{
    // Example from https://dev.twitter.com/oauth/overview/creating-signatures

    const QUrl url("https://api.twitter.com/1/statuses/update.json?include_entities=true");
    QOAuth1Signature signature(url,
                               QStringLiteral("kAcSOqF21Fu85e7zjz7ZN2U4ZRhfV3WpwPAoE3Z7kBw"),
                               QStringLiteral("LswwdoUaIvS8ltyTt5jkRh4J50vUPVVHtR2YPi5kE"),
                               QOAuth1Signature::HttpRequestMethod::Post);
    const QString body = QUrl::fromPercentEncoding("status=Hello%20Ladies%20%2b%20Gentlemen%2c%20a"
                                                   "%20signed%20OAuth%20request%21");

    signature.insert(oauthConsumerKey,
                     QStringLiteral("xvz1evFS4wEEPTGEFPHBog"));
    signature.insert(oauthNonce,
                     QStringLiteral("kYjzVBB8Y0ZFabxSWbWovY3uYSQ2pTgmZeNu2VS4cg"));
    signature.insert(oauthSignatureMethod,
                     QStringLiteral("HMAC-SHA1"));
    signature.insert(oauthTimestamp, QStringLiteral("1318622958"));
    signature.insert(oauthToken,
                     QStringLiteral("370773112-GmHxMAgYyLbNEtIKZeRNFsMKPR9EyMZeS9weJAEb"));
    signature.insert(oauthVersion, QStringLiteral("1.0"));
    signature.addRequestBody(QUrlQuery(body));
    return signature;
}

void tst_OAuth1Signature::twitterSignatureExample()
{
    const QOAuth1Signature signature = createTwitterSignature();
    QByteArray signatureData = signature.hmacSha1();
    QCOMPARE(signatureData.toBase64(), QByteArray("tnnArxj06cWHq44gCs1OSKk/jLY="));
}

void tst_OAuth1Signature::copyAndModify()
{
    const QOAuth1Signature signature = createTwitterSignature();
    QOAuth1Signature copy = signature;
    QCOMPARE(signature.hmacSha1(), copy.hmacSha1());
    copy.insert(QStringLiteral("signature"), QStringLiteral("modified"));
    QVERIFY(signature.hmacSha1() != copy.hmacSha1());
}

void tst_OAuth1Signature::signatures_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<QOAuth1Signature::HttpRequestMethod>("method");
    QTest::addColumn<QByteArray>("customVerb");
    QTest::addColumn<QString>("version");
    QTest::addColumn<QString>("consumerKey");
    QTest::addColumn<QString>("consumerSecret");
    QTest::addColumn<QString>("token");
    QTest::addColumn<QString>("tokenSecret");
    QTest::addColumn<QString>("nonce");
    QTest::addColumn<QString>("timestamp");
    QTest::addColumn<QMultiMap<QString, QVariant>>("parameters");
    QTest::addColumn<QString>("result");

    QTest::newRow("standard") << QUrl("http://example.net")
                              << QOAuth1Signature::HttpRequestMethod::Get
                              << QByteArray()
                              << "1.0"
                              << "key"
                              << "secret"
                              << "accesskey"
                              << "accesssecret"
                              << "468167367"
                              << "1494852816"
                              << QMultiMap<QString, QVariant>()
                              << "mQaARxv7pqJyViuwNGtUfm6QSIQ=";
    QTest::newRow("post") << QUrl("http://example.net")
                          << QOAuth1Signature::HttpRequestMethod::Post
                          << QByteArray()
                          << "1.0"
                          << "key"
                          << "secret"
                          << "accesskey"
                          << "accesssecret"
                          << "468167367"
                          << "1494852816"
                          << QMultiMap<QString, QVariant>()
                          << "L4blJKqYMTSNUEt32rCgDLhxQxM=";
    QTest::newRow("put") << QUrl("http://example.net")
                         << QOAuth1Signature::HttpRequestMethod::Put
                         << QByteArray()
                         << "1.0"
                         << "key"
                         << "secret"
                         << "accesskey"
                         << "accesssecret"
                         << "468167367"
                         << "1494852816"
                         << QMultiMap<QString, QVariant>()
                         << "+eiZ+phNoYnETf6SqI+XSE43JSY=";
    QTest::newRow("delete") << QUrl("http://example.net")
                            << QOAuth1Signature::HttpRequestMethod::Delete
                            << QByteArray()
                            << "1.0"
                            << "key"
                            << "secret"
                            << "accesskey"
                            << "accesssecret"
                            << "468167367"
                            << "1494852816"
                            << QMultiMap<QString, QVariant>()
                            << "enbOVNG7/vGliie2/L44NdccMaw=";
    QTest::newRow("head") << QUrl("http://example.net")
                          << QOAuth1Signature::HttpRequestMethod::Head
                          << QByteArray()
                          << "1.0"
                          << "key"
                          << "secret"
                          << "accesskey"
                          << "accesssecret"
                          << "468167367"
                          << "1494852816"
                          << QMultiMap<QString, QVariant>()
                          << "6v74w0rRsVibJsJ796Nj8cJPqEU=";
    QTest::newRow("no-hmac-key") << QUrl("http://example.net")
                                 << QOAuth1Signature::HttpRequestMethod::Get
                                 << QByteArray()
                                 << "1.0"
                                 << "key"
                                 << QString()
                                 << "accesskey"
                                 << QString()
                                 << "468167367"
                                 << "1494852816"
                                 << QMultiMap<QString, QVariant>()
                                 << "N2qP+LJdLbjalZq71M7oxPdeUjc=";
    QTest::newRow("custom-values") << QUrl("http://example.net")
                                   << QOAuth1Signature::HttpRequestMethod::Get
                                   << QByteArray()
                                   << "1.0"
                                   << "key"
                                   << "secret"
                                   << "accesskey"
                                   << "accesssecret"
                                   << "468167367"
                                   << "1494852816"
                                   << QMultiMap<QString, QVariant> {
                                        { "firstKey", "firstValue" },
                                        { "secondKey", "secondValue" }
                                    }
                                   << "xNXgQaO0LrQMbJZGSfKFUmWwGDw=";
    QTest::newRow("custom-verb-get") << QUrl("http://example.net")
                                     << QOAuth1Signature::HttpRequestMethod::Custom
                                     << QByteArray("GET")
                                     << "1.0"
                                     << "key"
                                     << "secret"
                                     << "accesskey"
                                     << "accesssecret"
                                     << "468167367"
                                     << "1494852816"
                                     << QMultiMap<QString, QVariant>()
                                     << "mQaARxv7pqJyViuwNGtUfm6QSIQ=";
    QTest::newRow("custom-verb-patch") << QUrl("http://example.net")
                                       << QOAuth1Signature::HttpRequestMethod::Custom
                                       << QByteArray("PATCH")
                                       << "1.0"
                                       << "key"
                                       << "secret"
                                       << "accesskey"
                                       << "accesssecret"
                                       << "468167367"
                                       << "1494852816"
                                       << QMultiMap<QString, QVariant>()
                                       << "kcRO68D7IBQWlQvUR/jkhuF8AKM=";
}

void tst_OAuth1Signature::signatures()
{
    // Should match the reference implementation at
    // http://bettiolo.github.io/oauth-reference-page/

    QFETCH(QUrl, url);
    QFETCH(QOAuth1Signature::HttpRequestMethod, method);
    QFETCH(QByteArray, customVerb);
    QFETCH(QString, version);
    QFETCH(QString, consumerKey);
    QFETCH(QString, consumerSecret);
    QFETCH(QString, token);
    QFETCH(QString, tokenSecret);
    QFETCH(QString, nonce);
    QFETCH(QString, timestamp);
    using MultiMap = QMultiMap<QString, QVariant>;
    QFETCH(MultiMap, parameters);
    QFETCH(QString, result);

    parameters.insert(oauthVersion, version);
    parameters.insert(oauthConsumerKey, consumerKey);
    parameters.insert(oauthNonce, nonce);
    parameters.insert(oauthSignatureMethod, "HMAC-SHA1");
    parameters.insert(oauthTimestamp, timestamp);
    parameters.insert(oauthToken, token);

    QOAuth1Signature signature(url, consumerSecret, tokenSecret, method, parameters);
    if (method == QOAuth1Signature::HttpRequestMethod::Custom)
        signature.setCustomMethodString(customVerb);
    const auto signatureData = signature.hmacSha1();
    QCOMPARE(signatureData.toBase64(), result.toLatin1());
}

QTEST_MAIN(tst_OAuth1Signature)
#include "tst_oauth1signature.moc"
