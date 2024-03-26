// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtSerialBus/qcanbusframe.h>

#include <QtCore/qdatastream.h>
#include <QtTest/qtest.h>

class tst_QCanBusFrame : public QObject
{
    Q_OBJECT
public:
    explicit tst_QCanBusFrame();

private slots:
    void constructors();
    void id();
    void payload();
    void timeStamp();
    void bitRateSwitch();
    void errorStateIndicator();
    void localEcho();

    void tst_isValid_data();
    void tst_isValid();
    void tst_isValidSize_data();
    void tst_isValidSize();

    void tst_toString_data();
    void tst_toString();

    void streaming_data();
    void streaming();

    void tst_error();
};

tst_QCanBusFrame::tst_QCanBusFrame()
{
}

void tst_QCanBusFrame::constructors()
{
    QCanBusFrame frame1;
    QCanBusFrame frame2(123, "tst");
    QCanBusFrame frame3(123456, "tst");
    QCanBusFrame frame4(1234, "tst tst tst");
    QCanBusFrame::TimeStamp timeStamp1;
    QCanBusFrame::TimeStamp timeStamp2(5, 5);

    QVERIFY(frame1.payload().isEmpty());
    QVERIFY(!frame1.frameId());
    QVERIFY(!frame1.hasFlexibleDataRateFormat());
    QVERIFY(!frame1.hasExtendedFrameFormat());
    QCOMPARE(frame1.frameType(), QCanBusFrame::DataFrame);

    QVERIFY(!frame2.payload().isEmpty());
    QVERIFY(frame2.frameId());
    QVERIFY(!frame2.hasFlexibleDataRateFormat());
    QVERIFY(!frame2.hasExtendedFrameFormat());
    QCOMPARE(frame2.frameType(), QCanBusFrame::DataFrame);

    QVERIFY(!frame3.payload().isEmpty());
    QVERIFY(frame3.frameId());
    QVERIFY(!frame3.hasFlexibleDataRateFormat());
    QVERIFY(frame3.hasExtendedFrameFormat());
    QCOMPARE(frame3.frameType(), QCanBusFrame::DataFrame);

    QVERIFY(!frame4.payload().isEmpty());
    QVERIFY(frame4.frameId());
    QVERIFY(frame4.hasFlexibleDataRateFormat());
    QVERIFY(!frame4.hasExtendedFrameFormat());
    QCOMPARE(frame4.frameType(), QCanBusFrame::DataFrame);

    QVERIFY(!timeStamp1.seconds());
    QVERIFY(!timeStamp1.microSeconds());

    QVERIFY(timeStamp2.seconds());
    QVERIFY(timeStamp2.microSeconds());
}

void tst_QCanBusFrame::id()
{
    QCanBusFrame frame;
    QVERIFY(!frame.frameId());
    frame.setExtendedFrameFormat(false);
    frame.setFrameId(2047u);
    QCOMPARE(frame.frameId(), 2047u);
    QVERIFY(frame.isValid());
    QVERIFY(!frame.hasExtendedFrameFormat());
    // id > 2^11 -> extended format
    frame.setExtendedFrameFormat(false);
    frame.setFrameId(2048u);
    QCOMPARE(frame.frameId(), 2048u);
    QVERIFY(frame.isValid());
    QVERIFY(frame.hasExtendedFrameFormat());
    // id < 2^11 -> no extended format
    frame.setExtendedFrameFormat(false);
    frame.setFrameId(512u);
    QCOMPARE(frame.frameId(), 512u);
    QVERIFY(frame.isValid());
    QVERIFY(!frame.hasExtendedFrameFormat());
    // id < 2^11 -> keep extended format
    frame.setExtendedFrameFormat(true);
    frame.setFrameId(512u);
    QCOMPARE(frame.frameId(), 512u);
    QVERIFY(frame.isValid());
    QVERIFY(frame.hasExtendedFrameFormat());
    // id >= 2^29 -> invalid
    frame.setExtendedFrameFormat(false);
    frame.setFrameId(536870912u);
    QCOMPARE(frame.frameId(), 0u);
    QVERIFY(!frame.isValid());
    QVERIFY(!frame.hasExtendedFrameFormat());
}

void tst_QCanBusFrame::payload()
{
    QCanBusFrame frame;
    QVERIFY(frame.payload().isEmpty());
    frame.setPayload("test");
    QCOMPARE(frame.payload().data(), "test");
    QVERIFY(!frame.hasFlexibleDataRateFormat());
    // setting long payload should automatically set hasFlexibleDataRateFormat()
    frame.setPayload("testtesttest");
    QCOMPARE(frame.payload().data(), "testtesttest");
    QVERIFY(frame.hasFlexibleDataRateFormat());
    // setting short payload should not change hasFlexibleDataRateFormat()
    frame.setPayload("test");
    QCOMPARE(frame.payload().data(), "test");
    QVERIFY(frame.hasFlexibleDataRateFormat());
}

void tst_QCanBusFrame::timeStamp()
{
    QCanBusFrame frame;
    QCanBusFrame::TimeStamp timeStamp = frame.timeStamp();
    QVERIFY(!timeStamp.seconds());
    QVERIFY(!timeStamp.microSeconds());

    // fromMicroSeconds: no microsecond overflow
    timeStamp = QCanBusFrame::TimeStamp::fromMicroSeconds(999999);
    QCOMPARE(timeStamp.seconds(), 0);
    QCOMPARE(timeStamp.microSeconds(), 999999);

    // fromMicroSeconds: microsecond overflow
    timeStamp = QCanBusFrame::TimeStamp::fromMicroSeconds(1000000);
    QCOMPARE(timeStamp.seconds(), 1);
    QCOMPARE(timeStamp.microSeconds(), 0);

    // fromMicroSeconds: microsecond overflow
    timeStamp = QCanBusFrame::TimeStamp::fromMicroSeconds(2000001);
    QCOMPARE(timeStamp.seconds(), 2);
    QCOMPARE(timeStamp.microSeconds(), 1);
}

void tst_QCanBusFrame::bitRateSwitch()
{
     QCanBusFrame frame(QCanBusFrame::DataFrame);
     QVERIFY(!frame.hasBitrateSwitch());

     // set CAN FD does not set BRS
     frame.setFlexibleDataRateFormat(true);
     QVERIFY(frame.hasFlexibleDataRateFormat());
     QVERIFY(!frame.hasBitrateSwitch());

     // set BRS keeps CAN FD
     frame.setBitrateSwitch(true);
     QVERIFY(frame.hasFlexibleDataRateFormat());
     QVERIFY(frame.hasBitrateSwitch());

     // clear BRS keeps CAN FD
     frame.setBitrateSwitch(false);
     QVERIFY(frame.hasFlexibleDataRateFormat());
     QVERIFY(!frame.hasBitrateSwitch());

     // clear CAN FD
     frame.setFlexibleDataRateFormat(false);
     QVERIFY(!frame.hasFlexibleDataRateFormat());
     QVERIFY(!frame.hasBitrateSwitch());

     // set BRS sets CAN FD
     frame.setBitrateSwitch(true);
     QVERIFY(frame.hasFlexibleDataRateFormat());
     QVERIFY(frame.hasBitrateSwitch());

     // clear CAN FD clears BRS
     frame.setFlexibleDataRateFormat(false);
     QVERIFY(!frame.hasFlexibleDataRateFormat());
     QVERIFY(!frame.hasBitrateSwitch());

     // default constructed CAN FD frame has no BRS
     const QCanBusFrame frame2(0x123, QByteArray(10, 0x55));
     QVERIFY(frame2.hasFlexibleDataRateFormat());
     QVERIFY(!frame2.hasBitrateSwitch());
}

void tst_QCanBusFrame::errorStateIndicator()
{
    QCanBusFrame frame(QCanBusFrame::DataFrame);
    QVERIFY(!frame.hasErrorStateIndicator());

    // set CAN FD does not set ESI
    frame.setFlexibleDataRateFormat(true);
    QVERIFY(frame.hasFlexibleDataRateFormat());
    QVERIFY(!frame.hasErrorStateIndicator());

    // set ESI keeps CAN FD
    frame.setErrorStateIndicator(true);
    QVERIFY(frame.hasFlexibleDataRateFormat());
    QVERIFY(frame.hasErrorStateIndicator());

    // clear ESI keeps CAN FD
    frame.setErrorStateIndicator(false);
    QVERIFY(frame.hasFlexibleDataRateFormat());
    QVERIFY(!frame.hasErrorStateIndicator());

    // clear CAN FD
    frame.setFlexibleDataRateFormat(false);
    QVERIFY(!frame.hasFlexibleDataRateFormat());
    QVERIFY(!frame.hasErrorStateIndicator());

    // set ESI sets CAN FD
    frame.setErrorStateIndicator(true);
    QVERIFY(frame.hasFlexibleDataRateFormat());
    QVERIFY(frame.hasErrorStateIndicator());

    // clear CAN FD clears ESI
    frame.setFlexibleDataRateFormat(false);
    QVERIFY(!frame.hasFlexibleDataRateFormat());
    QVERIFY(!frame.hasErrorStateIndicator());

    // default constructed CAN FD frame has no ESI
    const QCanBusFrame frame2(0x123, QByteArray(10, 0x55));
    QVERIFY(frame2.hasFlexibleDataRateFormat());
    QVERIFY(!frame2.hasErrorStateIndicator());
}

void tst_QCanBusFrame::localEcho()
{
    QCanBusFrame frame(QCanBusFrame::DataFrame);
    QVERIFY(!frame.hasLocalEcho());

    frame.setLocalEcho(true);
    QVERIFY(frame.hasLocalEcho());

    frame.setLocalEcho(false);
    QVERIFY(!frame.hasLocalEcho());

    const QCanBusFrame frame2(0x123, QByteArray());
    QVERIFY(!frame2.hasLocalEcho());
}

void tst_QCanBusFrame::tst_isValid_data()
{
    QTest::addColumn<QCanBusFrame::FrameType>("frameType");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<QByteArray>("payload");
    QTest::addColumn<uint>("id");
    QTest::addColumn<bool>("extended");
    QTest::addColumn<bool>("flexibleData");

    QTest::newRow("invalid frame")
                 << QCanBusFrame::InvalidFrame << false
                 << QByteArray() << 0u << false << false;
    QTest::newRow("data frame")
                 << QCanBusFrame::DataFrame << true
                 << QByteArray() << 0u << false << false;
    QTest::newRow("error frame")
                 << QCanBusFrame::ErrorFrame << true
                 << QByteArray() << 0u << false << false;
    QTest::newRow("remote request frame")
                 << QCanBusFrame::RemoteRequestFrame << true
                 << QByteArray() << 0u << false << false;
    QTest::newRow("remote request CAN FD frame")
                 << QCanBusFrame::RemoteRequestFrame << false
                 << QByteArray() << 0u << false << true;
    QTest::newRow("unknown frame")
                 << QCanBusFrame::UnknownFrame << true
                 << QByteArray() << 0u << false << false;
    QTest::newRow("data frame CAN max payload")
                 << QCanBusFrame::DataFrame << true
                 << QByteArray(8, 0) << 0u << false << false;
    QTest::newRow("data frame CAN FD max payload")
                 << QCanBusFrame::DataFrame << true
                 << QByteArray(64, 0) << 0u << false << true;
    QTest::newRow("data frame CAN too much payload")
                 << QCanBusFrame::DataFrame << false
                 << QByteArray(65, 0) << 0u << false << false;
    QTest::newRow("data frame short id")
                 << QCanBusFrame::DataFrame << true
                 << QByteArray(8, 0) << (1u << 11) - 1 << false << false;
    QTest::newRow("data frame long id")
                 << QCanBusFrame::DataFrame << true
                 << QByteArray(8, 0) << (1u << 11) << true << false;
    QTest::newRow("data frame bad long id")
                 << QCanBusFrame::DataFrame << false
                 << QByteArray(8, 0) << (1u << 11) << false << false;
    QTest::newRow("data frame CAN too long payload")
                 << QCanBusFrame::DataFrame << false
                 << QByteArray(9, 0) << 512u << false << false;
    QTest::newRow("data frame CAN FD long payload")
                 << QCanBusFrame::DataFrame << true
                 << QByteArray(12, 0) << 512u << false << true;
    QTest::newRow("data frame CAN FD too long payload")
                 << QCanBusFrame::DataFrame << false
                 << QByteArray(65, 0) << 512u << false << true;
 }

void tst_QCanBusFrame::tst_isValid()
{
    QFETCH(QCanBusFrame::FrameType, frameType);
    QFETCH(bool, isValid);
    QFETCH(QByteArray, payload);
    QFETCH(uint, id);
    QFETCH(bool, extended);
    QFETCH(bool, flexibleData);

    QCanBusFrame frame(frameType);
    frame.setPayload(payload);
    frame.setFrameId(id);
    frame.setExtendedFrameFormat(extended);
    frame.setFlexibleDataRateFormat(flexibleData);
    QCOMPARE(frame.isValid(), isValid);
    QCOMPARE(frame.frameType(), frameType);
    QCOMPARE(frame.payload(), payload);
    QCOMPARE(frame.frameId(), id);
    QCOMPARE(frame.hasExtendedFrameFormat(), extended);
    QCOMPARE(frame.hasFlexibleDataRateFormat(), flexibleData);

    frame.setFrameType(QCanBusFrame::InvalidFrame);
    QCOMPARE(frame.isValid(), false);
    QCOMPARE(QCanBusFrame::InvalidFrame, frame.frameType());
}

void tst_QCanBusFrame::tst_isValidSize_data()
{
    QTest::addColumn<int>("payloadLength");
    QTest::addColumn<bool>("isFlexibleDataRate");
    QTest::addColumn<bool>("isValid");

    QTest::newRow("2.0-0") << 0  << false << true;
    QTest::newRow("2.0-8") << 8  << false << true;
    QTest::newRow("2.0-9") << 9  << false << false;

    QTest::newRow("FD-0")  << 0  << true << true;
    QTest::newRow("FD-8")  << 8  << true << true;
    QTest::newRow("FD-9")  << 9  << true << false;
    QTest::newRow("FD-11") << 11 << true << false;
    QTest::newRow("FD-12") << 12 << true << true;
    QTest::newRow("FD-13") << 13 << true << false;
    QTest::newRow("FD-16") << 16 << true << true;

    QTest::newRow("FD-20") << 20 << true << true;
    QTest::newRow("FD-24") << 24 << true << true;
    QTest::newRow("FD-32") << 32 << true << true;
    QTest::newRow("FD-48") << 48 << true << true;

    QTest::newRow("FD-63") << 63 << true << false;
    QTest::newRow("FD-64") << 64 << true << true;
    QTest::newRow("FD-65") << 65 << true << false;
}

void tst_QCanBusFrame::tst_isValidSize()
{
    QFETCH(int,  payloadLength);
    QFETCH(bool, isFlexibleDataRate);
    QFETCH(bool, isValid);

    QCanBusFrame frame(0, QByteArray(payloadLength, ' '));
    frame.setFlexibleDataRateFormat(isFlexibleDataRate);

    QCOMPARE(frame.isValid(), isValid);
}

void tst_QCanBusFrame::tst_toString_data()
{
    QTest::addColumn<QCanBusFrame::FrameType>("frameType");
    QTest::addColumn<QCanBusFrame::FrameId>("id");
    QTest::addColumn<bool>("extended");
    QTest::addColumn<QByteArray>("payload");
    QTest::addColumn<QString>("expected");

    QTest::newRow("invalid frame")
            << QCanBusFrame::InvalidFrame << 0x0u << false
            << QByteArray()
            << "(Invalid)";
    QTest::newRow("error frame")
            << QCanBusFrame::ErrorFrame << 0x0u << false
            << QByteArray()
            << "(Error)";
    QTest::newRow("unknown frame")
            << QCanBusFrame::UnknownFrame << 0x0u << false
            << QByteArray()
            << "(Unknown)";
    QTest::newRow("remote request frame")
            << QCanBusFrame::RemoteRequestFrame << 0x123u << false
            << QByteArray::fromHex("01") // fake data to get a DLC > 0
            << QString("     123   [1]  Remote Request");
    QTest::newRow("data frame min std id")
            << QCanBusFrame::DataFrame << 0x0u << false
            << QByteArray()
            << QString("     000   [0]");
    QTest::newRow("data frame max std id")
            << QCanBusFrame::DataFrame << 0x7FFu << false
            << QByteArray()
            << QString("     7FF   [0]");
    QTest::newRow("data frame min ext id")
            << QCanBusFrame::DataFrame << 0x0u << true
            << QByteArray()
            << QString("00000000   [0]");
    QTest::newRow("data frame max ext id")
            << QCanBusFrame::DataFrame << 0x1FFFFFFFu << true
            << QByteArray()
            << QString("1FFFFFFF   [0]");
    QTest::newRow("data frame minimal size")
            << QCanBusFrame::DataFrame << 0x7FFu << false
            << QByteArray::fromHex("01")
            << QString("     7FF   [1]  01");
    QTest::newRow("data frame maximal size")
            << QCanBusFrame::DataFrame << 0x1FFFFFFFu << true
            << QByteArray::fromHex("0123456789ABCDEF")
            << QString("1FFFFFFF   [8]  01 23 45 67 89 AB CD EF");
    QTest::newRow("short data frame FD")
            << QCanBusFrame::DataFrame << 0x123u << false
            << QByteArray::fromHex("001122334455667788")
            << QString("     123  [09]  00 11 22 33 44 55 66 77 88");
    QTest::newRow("long data frame FD")
            << QCanBusFrame::DataFrame << 0x123u << false
            << QByteArray::fromHex("00112233445566778899")
            << QString("     123  [10]  00 11 22 33 44 55 66 77 88 99");
}

void tst_QCanBusFrame::tst_toString()
{
    QFETCH(QCanBusFrame::FrameType, frameType);
    QFETCH(QCanBusFrame::FrameId, id);
    QFETCH(bool, extended);
    QFETCH(QByteArray, payload);
    QFETCH(QString, expected);
    QCanBusFrame frame;
    frame.setFrameType(frameType);
    frame.setFrameId(id);
    frame.setExtendedFrameFormat(extended);
    frame.setPayload(payload);

    const QString result = frame.toString();

    QCOMPARE(result, expected);
}

void tst_QCanBusFrame::streaming_data()
{
    QTest::addColumn<QCanBusFrame::FrameId>("frameId");
    QTest::addColumn<QByteArray>("payload");
    QTest::addColumn<qint64>("seconds");
    QTest::addColumn<qint64>("microSeconds");
    QTest::addColumn<bool>("isExtended");
    QTest::addColumn<bool>("isFlexibleDataRate");
    QTest::addColumn<bool>("isBitrateSwitch");
    QTest::addColumn<bool>("isErrorStateIndicator");
    QTest::addColumn<bool>("isLocalEcho");
    QTest::addColumn<QCanBusFrame::FrameType>("frameType");


    QTest::newRow("emptyFrame") << QCanBusFrame::FrameId(0) << QByteArray()
                                << qint64(0) << qint64(0)
                                << false << false << false << false << false
                                << QCanBusFrame::DataFrame;
    QTest::newRow("fullFrame1") << QCanBusFrame::FrameId(123) << QByteArray("abcde1")
                               << qint64(456) << qint64(784)
                               << true << false << false << false << false
                               << QCanBusFrame::DataFrame;
    QTest::newRow("fullFrame2") << QCanBusFrame::FrameId(123) << QByteArray("abcde2")
                               << qint64(457) << qint64(785)
                               << false << false << false << false << false
                               << QCanBusFrame::DataFrame;
    QTest::newRow("fullFrameFD") << QCanBusFrame::FrameId(123) << QByteArray("abcdfd")
                                << qint64(457) << qint64(785)
                                << false << true << false << false << false
                                << QCanBusFrame::DataFrame;
    QTest::newRow("fullFrameBRS") << QCanBusFrame::FrameId(123) << QByteArray("abcdfd")
                                << qint64(457) << qint64(785)
                                << false << true << true << false << false
                                << QCanBusFrame::DataFrame;
    QTest::newRow("fullFrameESI") << QCanBusFrame::FrameId(123) << QByteArray("abcdfd")
                                  << qint64(457) << qint64(785)
                                  << false << true << false << true << false
                                  << QCanBusFrame::DataFrame;
    QTest::newRow("echoFrame") << QCanBusFrame::FrameId(123) << QByteArray("abcde7")
                               << qint64(888) << qint64(777)
                               << false << false << false << false << true
                               << QCanBusFrame::DataFrame;
    QTest::newRow("fullFrame3") << QCanBusFrame::FrameId(123) << QByteArray("abcde3")
                               << qint64(458) << qint64(786)
                               << true << false << false << false << false
                               << QCanBusFrame::RemoteRequestFrame;
    QTest::newRow("fullFrame4") << QCanBusFrame::FrameId(123) << QByteArray("abcde4")
                               << qint64(459) << qint64(787)
                               << false << false << false << false << false
                               << QCanBusFrame::RemoteRequestFrame;
    QTest::newRow("fullFrame5") << QCanBusFrame::FrameId(123) << QByteArray("abcde5")
                               << qint64(460) << qint64(789)
                               << true << false << false << false << false
                               << QCanBusFrame::ErrorFrame;
    QTest::newRow("fullFrame6") << QCanBusFrame::FrameId(123) << QByteArray("abcde6")
                               << qint64(453) << qint64(788)
                               << false << false << false << false << false
                               << QCanBusFrame::ErrorFrame;
}

void tst_QCanBusFrame::streaming()
{
    QFETCH(QCanBusFrame::FrameId, frameId);
    QFETCH(QByteArray, payload);
    QFETCH(qint64, seconds);
    QFETCH(qint64, microSeconds);
    QFETCH(bool, isExtended);
    QFETCH(bool, isFlexibleDataRate);
    QFETCH(bool, isBitrateSwitch);
    QFETCH(bool, isErrorStateIndicator);
    QFETCH(bool, isLocalEcho);
    QFETCH(QCanBusFrame::FrameType, frameType);

    QCanBusFrame originalFrame(frameId, payload);
    const QCanBusFrame::TimeStamp originalStamp(seconds, microSeconds);
    originalFrame.setTimeStamp(originalStamp);

    originalFrame.setExtendedFrameFormat(isExtended);
    originalFrame.setFlexibleDataRateFormat(isFlexibleDataRate);
    originalFrame.setBitrateSwitch(isBitrateSwitch);
    originalFrame.setErrorStateIndicator(isErrorStateIndicator);
    originalFrame.setLocalEcho(isLocalEcho);
    originalFrame.setFrameType(frameType);

    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);
    out << originalFrame;

    QDataStream in(buffer);
    QCanBusFrame restoredFrame;
    in >> restoredFrame;
    const QCanBusFrame::TimeStamp restoredStamp(restoredFrame.timeStamp());

    QCOMPARE(restoredFrame.frameId(), originalFrame.frameId());
    QCOMPARE(restoredFrame.payload(), originalFrame.payload());

    QCOMPARE(restoredStamp.seconds(), originalStamp.seconds());
    QCOMPARE(restoredStamp.microSeconds(), originalStamp.microSeconds());

    QCOMPARE(restoredFrame.frameType(), originalFrame.frameType());
    QCOMPARE(restoredFrame.hasExtendedFrameFormat(),
             originalFrame.hasExtendedFrameFormat());
    QCOMPARE(restoredFrame.hasFlexibleDataRateFormat(),
             originalFrame.hasFlexibleDataRateFormat());
    QCOMPARE(restoredFrame.hasBitrateSwitch(),
             originalFrame.hasBitrateSwitch());
    QCOMPARE(restoredFrame.hasErrorStateIndicator(),
             originalFrame.hasErrorStateIndicator());
    QCOMPARE(restoredFrame.hasLocalEcho(),
             originalFrame.hasLocalEcho());
}

void tst_QCanBusFrame::tst_error()
{
    QCanBusFrame frame(1, QByteArray());
    QCOMPARE(frame.frameType(), QCanBusFrame::DataFrame);
    QCOMPARE(frame.frameId(), 1u);
    QCOMPARE(frame.error(), QCanBusFrame::NoError);

    //set error -> should be ignored since still DataFrame
    frame.setError(QCanBusFrame::AnyError);
    QCOMPARE(frame.frameType(), QCanBusFrame::DataFrame);
    QCOMPARE(frame.frameId(), 1u);
    QCOMPARE(frame.error(), QCanBusFrame::NoError);

    frame.setFrameType(QCanBusFrame::ErrorFrame);
    QCOMPARE(frame.frameType(), QCanBusFrame::ErrorFrame);
    QCOMPARE(frame.frameId(), 0u); //id of Error frame always 0
    QCOMPARE(frame.error(), QCanBusFrame::TransmissionTimeoutError);

    frame.setError(QCanBusFrame::FrameErrors(QCanBusFrame::ControllerError | QCanBusFrame::ProtocolViolationError));
    QCOMPARE(frame.frameType(), QCanBusFrame::ErrorFrame);
    QCOMPARE(frame.frameId(), 0u); //id of Error frame always 0
    QCOMPARE(frame.error(),
             QCanBusFrame::ControllerError|QCanBusFrame::ProtocolViolationError);

    frame.setFrameType(QCanBusFrame::RemoteRequestFrame);
    QCOMPARE(frame.frameType(), QCanBusFrame::RemoteRequestFrame);
    QCOMPARE(frame.frameId(), uint(QCanBusFrame::ControllerError | QCanBusFrame::ProtocolViolationError));
    QCOMPARE(frame.error(), QCanBusFrame::NoError);
}

QTEST_MAIN(tst_QCanBusFrame)

#include "tst_qcanbusframe.moc"
