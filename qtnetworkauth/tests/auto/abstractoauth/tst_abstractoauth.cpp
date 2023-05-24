// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore>
#include <QtTest>
#include <QtNetwork>

#include <QtNetworkAuth/qabstractoauth.h>

#include <private/qabstractoauth_p.h>

class tst_AbstractOAuth : public QObject
{
    Q_OBJECT

private:
    struct AbstractOAuthPrivate : public QAbstractOAuthPrivate {
        AbstractOAuthPrivate() : QAbstractOAuthPrivate("", QUrl(), QString(), nullptr)
        {}
    };

    struct AbstractOAuth : QAbstractOAuth {
        AbstractOAuth() : QAbstractOAuth(*new AbstractOAuthPrivate(),
                                         nullptr)
        {}

        QNetworkReply *head(const QUrl &, const QVariantMap &) override { return nullptr; }
        QNetworkReply *get(const QUrl &, const QVariantMap &) override { return nullptr; }
        QNetworkReply *post(const QUrl &, const QVariantMap &) override { return nullptr; }
        QNetworkReply *put(const QUrl &, const QVariantMap &) override { return nullptr; }
        QNetworkReply *deleteResource(const QUrl &, const QVariantMap &) override
        {
            return nullptr;
        }
        void grant() override {}

        void prepareRequest(QNetworkRequest *, const QByteArray &,
                            const QByteArray & = QByteArray()) override
        {
        }
    };

private Q_SLOTS:
    void authorizationUrlSignal();
};

void tst_AbstractOAuth::authorizationUrlSignal()
{
    AbstractOAuth obj;
    QUrl expectedValue = QUrl("http://example.net/");
    const QUrl defaultValue = obj.authorizationUrl();
    QVERIFY(expectedValue != defaultValue);
    bool emitted = false;
    connect(&obj, &QAbstractOAuth::authorizationUrlChanged, [&](const QUrl &value) {
        QCOMPARE(expectedValue, value);
        emitted = true;
    });
    obj.setAuthorizationUrl(expectedValue);
    QVERIFY(emitted);
}

QTEST_MAIN(tst_AbstractOAuth)
#include "tst_abstractoauth.moc"
