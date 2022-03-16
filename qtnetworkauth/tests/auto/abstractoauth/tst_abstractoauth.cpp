/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Network Auth module of the Qt Toolkit.
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

#include <QtCore>
#include <QtTest>
#include <QtNetwork>

#include <QtNetworkAuth/qabstractoauth.h>

#include <private/qabstractoauth_p.h>

class tst_AbstractOAuth : public QObject
{
    Q_OBJECT

private:
    struct AbstractOAuth : QAbstractOAuth {
        AbstractOAuth() : QAbstractOAuth(*new QAbstractOAuthPrivate("", QUrl(), QString(), nullptr),
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
