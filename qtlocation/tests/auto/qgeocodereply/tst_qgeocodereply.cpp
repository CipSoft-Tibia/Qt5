// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tst_qgeocodereply.h"

QT_USE_NAMESPACE

void tst_QGeoCodeReply::initTestCase()
{

    reply = new SubGeocodeReply();
}

void tst_QGeoCodeReply::cleanupTestCase()
{

    delete reply;
    delete qgeolocation;
}

void tst_QGeoCodeReply::init()
{
    qRegisterMetaType<QGeoCodeReply::Error>();
    signalerror = new QSignalSpy(reply, SIGNAL(errorOccurred(QGeoCodeReply::Error,QString)));
    signalfinished = new QSignalSpy(reply, SIGNAL(finished()));
}

void tst_QGeoCodeReply::cleanup()
{
    delete signalerror;
    delete signalfinished;
}

void tst_QGeoCodeReply::constructor()
{
    QVERIFY(!reply->isFinished());

    QCOMPARE(reply->limit(),-1);
    QCOMPARE(reply->offset(),0);
    QCOMPARE(reply->error(),QGeoCodeReply::NoError);

    QVERIFY( signalerror->isValid() );
    QVERIFY( signalfinished->isValid() );

    QCOMPARE(signalerror->count(),0);
    QCOMPARE(signalfinished->count(),0);
}

void tst_QGeoCodeReply::constructor_error()
{
    QFETCH(QGeoCodeReply::Error,error);
    QFETCH(QString,msg);

    QVERIFY( signalerror->isValid() );
    QVERIFY( signalfinished->isValid() );

    QGeoCodeReply *qgeocodereplycopy = new QGeoCodeReply (error,msg,0);

    QCOMPARE(signalerror->count(),0);
    QCOMPARE(signalfinished->count(),0);

    QCOMPARE (qgeocodereplycopy->error(),error);
    QCOMPARE (qgeocodereplycopy->errorString(),msg);

    delete qgeocodereplycopy;
}

void tst_QGeoCodeReply::constructor_error_data()
{
    QTest::addColumn<QGeoCodeReply::Error>("error");
    QTest::addColumn<QString>("msg");

    QTest::newRow("error1") << QGeoCodeReply::NoError << "No error.";
    QTest::newRow("error2") << QGeoCodeReply::EngineNotSetError << "Engine Not Set Error.";
    QTest::newRow("error3") << QGeoCodeReply::CommunicationError << "Communication Error.";
    QTest::newRow("error4") << QGeoCodeReply::ParseError << "Parse Error.";
    QTest::newRow("error5") << QGeoCodeReply::UnsupportedOptionError << "Unsupported Option Error.";
    QTest::newRow("error6") << QGeoCodeReply::UnknownError << "Unknown Error.";

}

void tst_QGeoCodeReply::destructor()
{
    QGeoCodeReply *qgeocodereplycopy;
    QFETCH(QGeoCodeReply::Error,error);
    QFETCH(QString,msg);

    qgeocodereplycopy = new QGeoCodeReply (error,msg,0);
    delete qgeocodereplycopy;
}

void tst_QGeoCodeReply::destructor_data()
{
    tst_QGeoCodeReply::constructor_error_data();
}

void tst_QGeoCodeReply::abort()
{
    QVERIFY( signalerror->isValid() );
    QVERIFY( signalfinished->isValid() );

    QCOMPARE(signalerror->count(),0);
    QCOMPARE (signalfinished->count(),0);

    reply->callSetFinished(true);
    reply->abort();

    QCOMPARE(signalerror->count(),0);
    QCOMPARE (signalfinished->count(),1);

    reply->abort();
    reply->callSetFinished(false);
    reply->abort();

    QCOMPARE(signalerror->count(),0);
    QCOMPARE (signalfinished->count(),2);
}

void tst_QGeoCodeReply::error()
{
    QFETCH(QGeoCodeReply::Error,error);
    QFETCH(QString,msg);

    QVERIFY( signalerror->isValid() );
    QVERIFY( signalfinished->isValid() );
    QCOMPARE(signalerror->count(),0);

    reply->callSetError(error,msg);

   QCOMPARE(signalerror->count(),1);
   QCOMPARE(signalfinished->count(),1);
   QCOMPARE(reply->errorString(),msg);
   QCOMPARE(reply->error(),error);


}

void tst_QGeoCodeReply::error_data()
{
    QTest::addColumn<QGeoCodeReply::Error>("error");
    QTest::addColumn<QString>("msg");

    QTest::newRow("error1") << QGeoCodeReply::NoError << "No error.";
    QTest::newRow("error2") << QGeoCodeReply::EngineNotSetError << "Engine Not Set Error.";
    QTest::newRow("error3") << QGeoCodeReply::CommunicationError << "Communication Error.";
    QTest::newRow("error4") << QGeoCodeReply::ParseError << "Parse Error.";
    QTest::newRow("error5") << QGeoCodeReply::UnsupportedOptionError << "Unsupported Option Error.";
    QTest::newRow("error6") << QGeoCodeReply::UnknownError << "Unknown Error.";
}

void tst_QGeoCodeReply::finished()
{
    QVERIFY( signalerror->isValid() );
    QVERIFY( signalfinished->isValid() );

    QCOMPARE(signalerror->count(),0);
    QCOMPARE (signalfinished->count(),0);

    reply->callSetFinished(true);
    QVERIFY(reply->isFinished());
    QCOMPARE(signalerror->count(),0);
    QCOMPARE (signalfinished->count(),1);

    reply->callSetFinished(false);

    QVERIFY(!reply->isFinished());
    QCOMPARE(signalerror->count(),0);
    QCOMPARE (signalfinished->count(),1);

    reply->callSetFinished(true);

    QVERIFY(reply->isFinished());
    QCOMPARE(signalerror->count(),0);
    QCOMPARE (signalfinished->count(),2);
}



void tst_QGeoCodeReply::limit()
{
    int limit =30;
    reply->callSetLimit(limit);
    QCOMPARE(reply->limit(),limit);
}

void tst_QGeoCodeReply::offset()
{
    int offset = 2;
    reply->callSetOffset(offset);
    QCOMPARE(reply->offset(),offset);
}

void tst_QGeoCodeReply::locations()
{
    QList <QGeoLocation> geolocations;
    geolocations = reply->locations();

    QCOMPARE(geolocations.size(),0);

    QGeoAddress *qgeoaddress = new QGeoAddress ();
    qgeoaddress->setCity("Berlin");

    QGeoCoordinate *qgeocoordinate = new QGeoCoordinate (12.12 , 54.43);

    qgeolocation = new QGeoLocation ();
    qgeolocation->setAddress(*qgeoaddress);
    qgeolocation->setCoordinate(*qgeocoordinate);

    reply->callAddLocation(*qgeolocation);

    geolocations = reply->locations();
    QCOMPARE(geolocations.size(),1);
    QCOMPARE(geolocations.at(0),*qgeolocation);

    QGeoLocation *qgeolocationcopy = new QGeoLocation (*qgeolocation);

    QList <QGeoLocation> qgeolocations;
    qgeolocations.append(*qgeolocation);
    qgeolocations.append(*qgeolocationcopy);

    reply->callSetLocations(qgeolocations);

    geolocations = reply->locations();

    QCOMPARE(geolocations.size(),qgeolocations.size());
    for (int i = 0 ; i < geolocations.size(); i++)
    {
        QCOMPARE(geolocations.at(i),qgeolocations.at(i));
    }

    delete qgeoaddress;
    delete qgeocoordinate;
    delete qgeolocationcopy;
}

void tst_QGeoCodeReply::viewport()
{
    QGeoCoordinate *qgeocoordinate = new QGeoCoordinate (12.12 , 54.43);

    qgeoboundingbox = new QGeoRectangle (*qgeocoordinate, 0.5 , 0.5);

    reply->callSetViewport(*qgeoboundingbox);

    QCOMPARE (reply->viewport(), static_cast<const QGeoShape &>(*qgeoboundingbox));

    delete qgeocoordinate;
    delete qgeoboundingbox;
}

QTEST_MAIN(tst_QGeoCodeReply);
