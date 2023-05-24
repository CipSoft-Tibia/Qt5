// Copyright (C) 2017 Witekio.
// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest>
#include <QCoreApplication>

#include <QtCoap/qcoapreply.h>
#include <private/qcoapreply_p.h>
#include <private/qcoapnamespace_p.h>

class tst_QCoapReply : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void updateReply_data();
    void updateReply();
    void requestData();
    void abortRequest();
};

void tst_QCoapReply::updateReply_data()
{
    QTest::addColumn<QByteArray>("payload");
    QTest::addColumn<QtCoap::ResponseCode>("responseCode");
    QTest::addColumn<QtCoap::Error>("error");

    QTest::newRow("success")
            << QByteArray("Some data")
            << QtCoap::ResponseCode::Content
            << QtCoap::Error::Ok;
    QTest::newRow("content error")
            << QByteArray("Error")
            << QtCoap::ResponseCode::BadRequest
            << QtCoap::Error::Ok;
    QTest::newRow("finished error")
            << QByteArray("Error")
            << QtCoap::ResponseCode::Content
            << QtCoap::Error::BadRequest;
    QTest::newRow("content & finished errors")
            << QByteArray("2Errors")
            << QtCoap::ResponseCode::BadGateway
            << QtCoap::Error::BadRequest;
}

void tst_QCoapReply::updateReply()
{
    QFETCH(QByteArray, payload);
    QFETCH(QtCoap::ResponseCode, responseCode);
    QFETCH(QtCoap::Error, error);

    const QByteArray token = "\xAF\x01\xC2";
    const quint16 id = 645;

    QScopedPointer<QCoapReply> reply(QCoapReplyPrivate::createCoapReply(QCoapRequest()));
    QCoapMessage message;
    message.setToken(token);
    message.setMessageId(id);
    message.setPayload(payload);

    QSignalSpy spyReplyFinished(reply.data(), &QCoapReply::finished);
    QSignalSpy spyReplyNotified(reply.data(), &QCoapReply::notified);
    QSignalSpy spyReplyError(reply.data(), &QCoapReply::error);
    QSignalSpy spyReplyAborted(reply.data(), &QCoapReply::aborted);

    QMetaObject::invokeMethod(reply.data(), "_q_setContent",
                              Q_ARG(QHostAddress, QHostAddress()),
                              Q_ARG(QCoapMessage, message),
                              Q_ARG(QtCoap::ResponseCode, responseCode));
    QMetaObject::invokeMethod(reply.data(), "_q_setFinished",
                              Q_ARG(QtCoap::Error, error));

    QCOMPARE(spyReplyFinished.size(), 1);
    QCOMPARE(spyReplyNotified.size(), 0);
    QCOMPARE(spyReplyAborted.size(), 0);
    if (error != QtCoap::Error::Ok || QtCoap::isError(responseCode)) {
        QVERIFY(spyReplyError.size() > 0);
        QCOMPARE(reply->isSuccessful(), false);
    } else {
        QCOMPARE(spyReplyError.size(), 0);
        QCOMPARE(reply->isSuccessful(), true);
    }

    QCOMPARE(reply->readAll(), payload);
    QCOMPARE(reply->readAll(), QByteArray());
    QCOMPARE(reply->responseCode(), responseCode);
    QCOMPARE(reply->message().token(), token);
    QCOMPARE(reply->message().messageId(), id);
}

void tst_QCoapReply::requestData()
{
    QScopedPointer<QCoapReply> reply(QCoapReplyPrivate::createCoapReply(QCoapRequest()));
    QMetaObject::invokeMethod(reply.data(), "_q_setRunning",
                              Q_ARG(QCoapToken, "token"),
                              Q_ARG(QCoapMessageId, 543));

    QCOMPARE(reply->request().token(), QByteArray("token"));
    QCOMPARE(reply->request().messageId(), 543);
}

void tst_QCoapReply::abortRequest()
{
    QScopedPointer<QCoapReply> reply(QCoapReplyPrivate::createCoapReply(QCoapRequest()));
    QMetaObject::invokeMethod(reply.data(), "_q_setRunning",
                              Q_ARG(QCoapToken, "token"),
                              Q_ARG(QCoapMessageId, 543));

    QSignalSpy spyAborted(reply.data(), &QCoapReply::aborted);
    QSignalSpy spyFinished(reply.data(), &QCoapReply::finished);
    reply->abortRequest();

    QTRY_COMPARE_WITH_TIMEOUT(spyAborted.size(), 1, 1000);
    QList<QVariant> arguments = spyAborted.takeFirst();
    QTRY_COMPARE_WITH_TIMEOUT(spyFinished.size(), 1, 1000);
    QVERIFY(arguments.at(0).toByteArray() == "token");
}

QTEST_MAIN(tst_QCoapReply)

#include "tst_qcoapreply.moc"
