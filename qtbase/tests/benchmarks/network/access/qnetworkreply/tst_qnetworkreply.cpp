// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// This file contains benchmarks for QNetworkReply functions.

#include <QDebug>
#include <qtest.h>
#include <QTest>
#include <QTestEventLoop>
#include <QSemaphore>
#include <QTimer>
#include <QtCore/qrandom.h>
#include <QtCore/QElapsedTimer>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qtcpsocket.h>
#include <QtNetwork/qtcpserver.h>
#include "../../../../auto/network-settings.h"

#ifdef QT_BUILD_INTERNAL
#include <QtNetwork/private/qhostinfo_p.h>
#endif

using namespace std::chrono_literals;

Q_DECLARE_METATYPE(QSharedPointer<char>)

class TimedSender: public QThread
{
    Q_OBJECT
    qint64 totalBytes;
    QSemaphore ready;
    QByteArray dataToSend;
    QTcpSocket *client;
    int timeout;
    int port;
public:
    int transferRate;
    TimedSender(int ms)
        : totalBytes(0), timeout(ms), port(-1), transferRate(-1)
    {
        dataToSend = QByteArray(16*1024, '@');
        start();
        ready.acquire();
    }

    inline int serverPort() const { return port; }

private slots:
    void writeMore()
    {
        while (client->bytesToWrite() < 128 * 1024) {
            writePacket(dataToSend);
        }
    }

protected:
    void run() override
    {
        QTcpServer server;
        server.listen();
        port = server.serverPort();
        ready.release();

        server.waitForNewConnection(-1);
        client = server.nextPendingConnection();

        writeMore();
        connect(client, SIGNAL(bytesWritten(qint64)), SLOT(writeMore()), Qt::DirectConnection);

        QEventLoop eventLoop;
        QTimer::singleShot(timeout, &eventLoop, SLOT(quit()));

        QElapsedTimer timer;
        timer.start();
        eventLoop.exec();
        disconnect(client, SIGNAL(bytesWritten(qint64)), this, 0);

        // wait for the connection to shut down
        client->disconnectFromHost();
        if (!client->waitForDisconnected(10000))
            return;

        transferRate = totalBytes * 1000 / timer.elapsed();
        qDebug() << "TimedSender::run" << "receive rate:" << (transferRate / 1024) << "kB/s in"
                 << timer.elapsed() << "ms";
    }

    void writePacket(const QByteArray &array)
    {
        client->write(array);
        totalBytes += array.size();
    }
};


typedef QSharedPointer<QNetworkReply> QNetworkReplyPtr;

class DataReader: public QObject
{
    Q_OBJECT
public:
    qint64 totalBytes;
    QByteArray data;
    QIODevice *device;
    bool accumulate;
    DataReader(const QNetworkReplyPtr &dev, bool acc = true) : totalBytes(0), device(dev.data()), accumulate(acc)
    {
        connect(device, SIGNAL(readyRead()), SLOT(doRead()));
    }
    DataReader(QIODevice *dev, bool acc = true) : totalBytes(0), device(dev), accumulate(acc)
    {
        connect(device, SIGNAL(readyRead()), SLOT(doRead()));
    }

public slots:
    void doRead()
    {
        QByteArray buffer;
        buffer.resize(device->bytesAvailable());
        qint64 bytesRead = device->read(buffer.data(), device->bytesAvailable());
        if (bytesRead == -1) {
            QTestEventLoop::instance().exitLoop();
            return;
        }
        buffer.truncate(bytesRead);
        totalBytes += bytesRead;

        if (accumulate)
            data += buffer;
    }
};

class ThreadedDataReader: public QThread
{
    Q_OBJECT
    // used to make the constructor only return after the tcp server started listening
    QSemaphore ready;
    QTcpSocket *client;
    int port;
public:
    qint64 transferRate;
    ThreadedDataReader()
        : port(-1), transferRate(-1)
    {
        start();
        ready.acquire();
    }

    inline int serverPort() const { return port; }

protected:
    void run() override
    {
        QTcpServer server;
        server.listen();
        port = server.serverPort();
        ready.release();

        server.waitForNewConnection(-1);
        client = server.nextPendingConnection();

        QEventLoop eventLoop;
        DataReader reader(client, false);
        QObject::connect(client, SIGNAL(disconnected()), &eventLoop, SLOT(quit()));

        QElapsedTimer timer;
        timer.start();
        eventLoop.exec();
        qint64 elapsed = timer.elapsed();

        transferRate = reader.totalBytes * 1000 / elapsed;
        qDebug() << "ThreadedDataReader::run" << "send rate:" << (transferRate / 1024) << "kB/s in" << elapsed << "msec";
    }
};

class DataGenerator: public QIODevice
{
    Q_OBJECT
    enum { Idle, Started, Stopped } state;
public:
    DataGenerator() : state(Idle)
    { open(ReadOnly); }

    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override { return state == Started ? 1024*1024 : 0; }

public slots:
    void start() { state = Started; emit readyRead(); }
    void stop() { state = Stopped; emit readyRead(); }

protected:
    qint64 readData(char *data, qint64 maxlen) override
    {
        if (state == Stopped)
            return -1;          // EOF

        // return as many bytes as are wanted
        memset(data, '@', maxlen);
        return maxlen;
    }
    qint64 writeData(const char *, qint64) override { return -1; }
};

class ThreadedDataReaderHttpServer: public QThread
{
    Q_OBJECT
    // used to make the constructor only return after the tcp server started listening
    QSemaphore ready;
    QTcpSocket *client;
    int port;
public:
    qint64 transferRate;
    ThreadedDataReaderHttpServer()
        : port(-1), transferRate(-1)
    {
        start();
        ready.acquire();
    }

    inline int serverPort() const { return port; }

protected:
    void run() override
    {
        QTcpServer server;
        server.listen();
        port = server.serverPort();
        ready.release();

        QVERIFY(server.waitForNewConnection(10*1000));
        client = server.nextPendingConnection();

        // read lines until we read the empty line seperating HTTP request from HTTP request body
        do {
            if (client->canReadLine()) {
                QString line = client->readLine();
                if (line == "\n" || line == "\r\n")
                    break; // empty line
            }
            if (!client->waitForReadyRead(10*1000)) {
                client->close();
                return;
            }
        } while (client->state() == QAbstractSocket::ConnectedState);

        client->write("HTTP/1.0 200 OK\r\n");
        client->write("Content-length: 0\r\n");
        client->write("\r\n");
        client->flush();

        QCoreApplication::processEvents();

        QEventLoop eventLoop;
        DataReader reader(client, false);
        QObject::connect(client, SIGNAL(disconnected()), &eventLoop, SLOT(quit()));

        QElapsedTimer timer;
        timer.start();
        eventLoop.exec();
        qint64 elapsed = timer.elapsed();

        transferRate = reader.totalBytes * 1000 / elapsed;
        qDebug() << "ThreadedDataReaderHttpServer::run" << "send rate:" << (transferRate / 1024) << "kB/s in" << elapsed << "msec";
    }
};


class FixedSizeDataGenerator : public QIODevice
{
    Q_OBJECT
    enum { Idle, Started, Stopped } state;
public:
    FixedSizeDataGenerator(qint64 size) : state(Idle)
    { open(ReadOnly | Unbuffered);
      toBeGeneratedTotalCount = toBeGeneratedCount = size;
    }

    qint64 bytesAvailable() const override
    {
        return state == Started ? toBeGeneratedCount + QIODevice::bytesAvailable() : 0;
    }

    bool isSequential() const override { return false; }

    bool reset() override { return false; }

    qint64 size() const override { return toBeGeneratedTotalCount; }

public slots:
    void start() { state = Started; emit readyRead(); }

protected:
    qint64 readData(char *data, qint64 maxlen) override
    {
        memset(data, '@', maxlen);

        if (toBeGeneratedCount <= 0) {
            return -1;
        }

        qint64 n = qMin(maxlen, toBeGeneratedCount);
        toBeGeneratedCount -= n;

        if (toBeGeneratedCount <= 0) {
            // make sure this is a queued connection!
            emit readChannelFinished();
        }

        return n;
    }
    qint64 writeData(const char *, qint64) override { return -1; }

    qint64 toBeGeneratedCount;
    qint64 toBeGeneratedTotalCount;
};

class HttpDownloadPerformanceServer : QObject {
    Q_OBJECT;
    qint64 dataSize;
    qint64 dataSent;
    QTcpServer server;
    QTcpSocket *client;
    bool serverSendsContentLength;
    bool chunkedEncoding;

public:
    HttpDownloadPerformanceServer (qint64 ds, bool sscl, bool ce) : dataSize(ds), dataSent(0),
    client(0), serverSendsContentLength(sscl), chunkedEncoding(ce) {
        server.listen();
        connect(&server, SIGNAL(newConnection()), this, SLOT(newConnectionSlot()));
    }

    int serverPort() {
        return server.serverPort();
    }

public slots:

    void newConnectionSlot() {
        client = server.nextPendingConnection();
        client->setParent(this);
        connect(client, SIGNAL(readyRead()), this, SLOT(readyReadSlot()));
        connect(client, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWrittenSlot(qint64)));
    }

    void readyReadSlot() {
        client->readAll();
        client->write("HTTP/1.0 200 OK\n");
        if (serverSendsContentLength)
            client->write(QString("Content-Length: " + QString::number(dataSize) + "\n").toLatin1());
        if (chunkedEncoding)
            client->write(QString("Transfer-Encoding: chunked\n").toLatin1());
        client->write("Connection: close\n\n");
    }

    void bytesWrittenSlot(qint64 amount) {
        Q_UNUSED(amount);
        if (dataSent == dataSize && client) {
            // close eventually

            // chunked encoding: we have to send a last "empty" chunk
            if (chunkedEncoding)
                client->write(QString("0\r\n\r\n").toLatin1());

            client->disconnectFromHost();
            server.close();
            client = 0;
            return;
        }

        // send data
        if (client && client->bytesToWrite() < 100*1024 && dataSent < dataSize) {
            qint64 amount = qMin(qint64(16*1024), dataSize - dataSent);
            QByteArray data(amount, '@');

            if (chunkedEncoding) {
                client->write(QString(QString("%1").arg(amount,0,16).toUpper() + "\r\n").toLatin1());
                client->write(data.constData(), amount);
                client->write(QString("\r\n").toLatin1());
            } else {
                client->write(data.constData(), amount);
            }

            dataSent += amount;
        }
    }
};

class HttpDownloadPerformanceClient : QObject {
    Q_OBJECT;
    QIODevice *device;
    public:
    HttpDownloadPerformanceClient (QIODevice *dev) : device(dev){
        connect(dev, SIGNAL(readyRead()), this, SLOT(readyReadSlot()));
    }

    public slots:
    void readyReadSlot() {
        device->readAll();
    }

};




class tst_qnetworkreply : public QObject
{
    Q_OBJECT

    QNetworkAccessManager manager;

public:
    using QObject::connect;
    bool connect(const QNetworkReplyPtr &sender, const char *signal, const QObject *receiver, const char *slot, Qt::ConnectionType ct = Qt::AutoConnection)
    { return connect(sender.data(), signal, receiver, slot, ct); }
private slots:
    void initTestCase();
    void httpLatency();

#ifndef QT_NO_SSL
    void echoPerformance_data();
    void echoPerformance();
    void preConnectEncrypted();
#endif // !QT_NO_SSL
    void preConnectEncrypted_data();

    void downloadPerformance();
    void uploadPerformance();
    void performanceControlRate();
    void httpUploadPerformance();
    void httpDownloadPerformance_data();
    void httpDownloadPerformance();
    void httpDownloadPerformanceDownloadBuffer_data();
    void httpDownloadPerformanceDownloadBuffer();
    void httpsRequestChain();
    void httpsUpload();
    void preConnect_data();
    void preConnect();

private:
    void runHttpsUploadRequest(const QByteArray &data, const QNetworkRequest &request);
    QPair<QNetworkReply *, qint64> runGetRequest(QNetworkAccessManager *manager,
                                                 const QNetworkRequest &request);
};

void tst_qnetworkreply::initTestCase()
{
    if (!QtNetworkSettings::verifyTestNetworkSettings())
        QSKIP("No network test server available");
}

void tst_qnetworkreply::httpLatency()
{
    QNetworkAccessManager manager;
    QBENCHMARK{
        QNetworkRequest request(QUrl("http://" + QtNetworkSettings::serverName() + "/qtest/"));
        QNetworkReply* reply = manager.get(request);
        connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()), Qt::QueuedConnection);
        QTestEventLoop::instance().enterLoop(5s);
        QVERIFY(!QTestEventLoop::instance().timeout());
        delete reply;
    }
}

QPair<QNetworkReply *, qint64> tst_qnetworkreply::runGetRequest(
        QNetworkAccessManager *manager, const QNetworkRequest &request)
{
    QElapsedTimer timer;
    timer.start();
    QNetworkReply *reply = manager->get(request);
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), reply, SLOT(ignoreSslErrors()));
    connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()), Qt::QueuedConnection);
    QTestEventLoop::instance().enterLoop(20s);
    qint64 elapsed = timer.elapsed();
    return qMakePair(reply, elapsed);
}

#ifndef QT_NO_SSL
void tst_qnetworkreply::echoPerformance_data()
{
     QTest::addColumn<bool>("ssl");
     QTest::newRow("no_ssl") << false;
     QTest::newRow("ssl") << true;
}

void tst_qnetworkreply::echoPerformance()
{
    QFETCH(bool, ssl);
    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl((ssl ? "https://" : "http://") + QtNetworkSettings::serverName() + "/qtest/cgi-bin/echo.cgi"));

    QByteArray data;
    data.resize(1024*1024*10); // 10 MB
    // init with garbage. needed so ssl cannot compress it in an efficient way.
    for (size_t i = 0; i < data.size() / sizeof(int); i++) {
        char r = char(QRandomGenerator::global()->generate());
        data.data()[i*sizeof(int)] = r;
    }

    QBENCHMARK{
        QNetworkReply* reply = manager.post(request, data);
        connect(reply, SIGNAL(sslErrors(QList<QSslError>)), reply, SLOT(ignoreSslErrors()));
        connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()), Qt::QueuedConnection);
        QTestEventLoop::instance().enterLoop(5s);
        QVERIFY(!QTestEventLoop::instance().timeout());
        QVERIFY(reply->error() == QNetworkReply::NoError);
        delete reply;
    }
}

void tst_qnetworkreply::preConnectEncrypted()
{
    QFETCH(std::chrono::milliseconds, sleepTime);
    QString hostName = QLatin1String("www.google.com");

    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl("https://" + hostName));

    // make sure we have a full request including
    // DNS lookup, TCP and SSL handshakes
#ifdef QT_BUILD_INTERNAL
    qt_qhostinfo_clear_cache();
#else
    qWarning("no internal build, could not clear DNS cache. Results may not be representative.");
#endif

    // first, benchmark a normal request
    QPair<QNetworkReply *, qint64> normalResult = runGetRequest(&manager, request);
    QNetworkReply *normalReply = normalResult.first;
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(normalReply->error() == QNetworkReply::NoError);
    qint64 normalElapsed = normalResult.second;

    // clear all caches again
#ifdef QT_BUILD_INTERNAL
    qt_qhostinfo_clear_cache();
#else
    qWarning("no internal build, could not clear DNS cache. Results may not be representative.");
#endif
    manager.clearAccessCache();

    // now try to make the connection beforehand
    manager.connectToHostEncrypted(hostName);
    QTestEventLoop::instance().enterLoop(sleepTime);

    // now make another request and hopefully use the existing connection
    QPair<QNetworkReply *, qint64> preConnectResult = runGetRequest(&manager, request);
    QNetworkReply *preConnectReply = normalResult.first;
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(preConnectReply->error() == QNetworkReply::NoError);
    qint64 preConnectElapsed = preConnectResult.second;
    qDebug() << request.url().toString() << "full request:" << normalElapsed
             << "ms, pre-connect request:" << preConnectElapsed << "ms, difference:"
             << (normalElapsed - preConnectElapsed) << "ms";
}

#endif // !QT_NO_SSL

void tst_qnetworkreply::preConnectEncrypted_data()
{
#ifndef QT_NO_OPENSSL
    QTest::addColumn<std::chrono::milliseconds>("sleepTime");
    // start a new normal request after preconnecting is done
    QTest::newRow("HTTPS-2secs") << 2000ms;

    // start a new normal request while preconnecting is in-flight
    QTest::newRow("HTTPS-100ms") << 100ms;
#endif // QT_NO_OPENSSL
}

void tst_qnetworkreply::downloadPerformance()
{
    // unlike the above function, this one tries to send as fast as possible
    // and measures how fast it was.
    TimedSender sender(5000);
    QNetworkRequest request(QUrl(QStringLiteral("debugpipe://127.0.0.1:") + QString::number(sender.serverPort()) + QStringLiteral("/?bare=1")));
    QNetworkReplyPtr reply(manager.get(request));
    DataReader reader(reply, false);

    QElapsedTimer loopTime;
    connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));
    loopTime.start();
    QTestEventLoop::instance().enterLoop(40);
    int elapsedTime = loopTime.elapsed();
    sender.wait();

    qint64 receivedBytes = reader.totalBytes;
    qDebug() << "tst_QNetworkReply::downloadPerformance" << "receive rate:" << (receivedBytes * 1000 / elapsedTime / 1024) << "kB/s and"
             << elapsedTime << "ms";
}

void tst_qnetworkreply::uploadPerformance()
{
      ThreadedDataReader reader;
      DataGenerator generator;


      QNetworkRequest request(QUrl(QStringLiteral("debugpipe://127.0.0.1:") + QString::number(reader.serverPort()) + QStringLiteral("/?bare=1")));
      QNetworkReplyPtr reply(manager.put(request, &generator));
      generator.start();
      connect(&reader, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));
      QTimer::singleShot(5000, &generator, SLOT(stop()));

      QTestEventLoop::instance().enterLoop(30);
      QCOMPARE(reply->error(), QNetworkReply::NoError);
      QVERIFY(!QTestEventLoop::instance().timeout());
}

constexpr qint64 MiB = 1024 * 1024;

void tst_qnetworkreply::httpUploadPerformance()
{
      constexpr qint64 UploadSize = 128 * MiB;

      ThreadedDataReaderHttpServer reader;
      FixedSizeDataGenerator generator(UploadSize);

      QNetworkRequest request(QUrl("http://127.0.0.1:" + QString::number(reader.serverPort()) + "/?bare=1"));
      request.setHeader(QNetworkRequest::ContentLengthHeader,UploadSize);

      QNetworkReplyPtr reply(manager.put(request, &generator));

      connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));

      QElapsedTimer time;
      generator.start();
      time.start();
      QTestEventLoop::instance().enterLoop(40);
      qint64 elapsed = time.elapsed();
      reader.exit();
      reader.wait();
      QVERIFY(reply->isFinished());
      QCOMPARE(reply->error(), QNetworkReply::NoError);
      QVERIFY(!QTestEventLoop::instance().timeout());

      qDebug() << "tst_QNetworkReply::httpUploadPerformance" << elapsed << "msec, "
              << ((UploadSize/1024.0)/(elapsed/1000.0)) << " kB/sec";
}


void tst_qnetworkreply::performanceControlRate()
{
    // this is a control comparison for the other two above
    // it does the same thing, but instead bypasses the QNetworkAccess system
    qDebug() << "The following are the maximum transfer rates that we can get in this system"
        " (bypassing QNetworkAccess)";

    TimedSender sender(5000);
    QTcpSocket sink;
    sink.connectToHost("127.0.0.1", sender.serverPort());
    DataReader reader(&sink, false);

    QElapsedTimer loopTime;
    connect(&sink, SIGNAL(disconnected()), &QTestEventLoop::instance(), SLOT(exitLoop()));
    loopTime.start();
    QTestEventLoop::instance().enterLoop(40);
    int elapsedTime = loopTime.elapsed();
    sender.wait();

    qint64 receivedBytes = reader.totalBytes;
    qDebug() << "tst_QNetworkReply::performanceControlRate" << "receive rate:" << (receivedBytes * 1000 / elapsedTime / 1024) << "kB/s and"
             << elapsedTime << "ms";
}

void tst_qnetworkreply::httpDownloadPerformance_data()
{
    QTest::addColumn<bool>("serverSendsContentLength");
    QTest::addColumn<bool>("chunkedEncoding");

    QTest::newRow("Server sends no Content-Length") << false << false;
    QTest::newRow("Server sends Content-Length")     << true << false;
    QTest::newRow("Server uses chunked encoding")     << false << true;

}

void tst_qnetworkreply::httpDownloadPerformance()
{
    QFETCH(bool, serverSendsContentLength);
    QFETCH(bool, chunkedEncoding);

    constexpr qint64 UploadSize = 128 * MiB;

    HttpDownloadPerformanceServer server(UploadSize, serverSendsContentLength, chunkedEncoding);

    QNetworkRequest request(QUrl("http://127.0.0.1:" + QString::number(server.serverPort()) + "/?bare=1"));
    QNetworkReplyPtr reply(manager.get(request));

    connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()), Qt::QueuedConnection);
    HttpDownloadPerformanceClient client(reply.data());

    QElapsedTimer time;
    time.start();
    QTestEventLoop::instance().enterLoop(40);
    QCOMPARE(reply->error(), QNetworkReply::NoError);
    QVERIFY(!QTestEventLoop::instance().timeout());

    qint64 elapsed = time.elapsed();
    qDebug() << "tst_QNetworkReply::httpDownloadPerformance" << elapsed << "msec, "
            << ((UploadSize/1024.0)/(elapsed/1000.0)) << " kB/sec";
};

enum HttpDownloadPerformanceDownloadBufferTestType {
    JustDownloadBuffer,
    DownloadBufferButUseRead,
    NoDownloadBuffer
};
Q_DECLARE_METATYPE(HttpDownloadPerformanceDownloadBufferTestType)

class HttpDownloadPerformanceClientDownloadBuffer : QObject {
    Q_OBJECT
private:
    HttpDownloadPerformanceDownloadBufferTestType testType;
    QNetworkReply *reply;
    qint64 uploadSize;
    QList<qint64> bytesAvailableList;
public:
    HttpDownloadPerformanceClientDownloadBuffer (QNetworkReply *reply, HttpDownloadPerformanceDownloadBufferTestType testType, qint64 uploadSize)
        : testType(testType), reply(reply), uploadSize(uploadSize)
    {
        connect(reply, SIGNAL(finished()), this, SLOT(finishedSlot()));
    }

    public slots:
    void finishedSlot() {
        if (testType == JustDownloadBuffer) {
            // We have a download buffer and use it. This should be the fastest benchmark result.
            QVariant downloadBufferAttribute = reply->attribute(QNetworkRequest::DownloadBufferAttribute);
            QSharedPointer<char> data = downloadBufferAttribute.value<QSharedPointer<char> >();
        } else if (testType == DownloadBufferButUseRead) {
            // We had a download buffer but we benchmark here the "legacy" read() way to access it
            char* replyData = (char*) malloc(uploadSize);
            QVERIFY(reply->read(replyData, uploadSize) == uploadSize);
            free(replyData);
        } else if (testType == NoDownloadBuffer) {
            // We did not have a download buffer but we still need to benchmark having the data, e.g. reading it all.
            // This should be the slowest benchmark result.
            char* replyData = (char*) malloc(uploadSize);
            QVERIFY(reply->read(replyData, uploadSize) == uploadSize);
            free(replyData);
        }

        QMetaObject::invokeMethod(&QTestEventLoop::instance(), "exitLoop", Qt::QueuedConnection);
    }
};

void tst_qnetworkreply::httpDownloadPerformanceDownloadBuffer_data()
{
    QTest::addColumn<HttpDownloadPerformanceDownloadBufferTestType>("testType");

    QTest::newRow("use-download-buffer") << JustDownloadBuffer;
    QTest::newRow("use-download-buffer-but-use-read") << DownloadBufferButUseRead;
    QTest::newRow("do-not-use-download-buffer") << NoDownloadBuffer;
}

// Please note that the whole "zero copy" download buffer API is private right now. Do not use it.
void tst_qnetworkreply::httpDownloadPerformanceDownloadBuffer()
{
    QFETCH(HttpDownloadPerformanceDownloadBufferTestType, testType);

    // On my Linux Desktop the results are already visible with 128 kB, however we use this to have good results.
    enum {UploadSize = 32*1024*1024}; // 32 MB

    HttpDownloadPerformanceServer server(UploadSize, true, false);

    QNetworkRequest request(QUrl("http://127.0.0.1:" + QString::number(server.serverPort()) + "/?bare=1"));
    if (testType == JustDownloadBuffer || testType == DownloadBufferButUseRead)
        request.setAttribute(QNetworkRequest::MaximumDownloadBufferSizeAttribute, 1024*1024*128); // 128 MB is max allowed

    QNetworkAccessManager manager;
    QNetworkReplyPtr reply(manager.get(request));

    HttpDownloadPerformanceClientDownloadBuffer client(reply.data(), testType, UploadSize);

    QBENCHMARK_ONCE {
        QTestEventLoop::instance().enterLoop(40);
        QCOMPARE(reply->error(), QNetworkReply::NoError);
        QVERIFY(reply->isFinished());
        QVERIFY(!QTestEventLoop::instance().timeout());
    }
}


class HttpsRequestChainHelper : public QObject {
    Q_OBJECT
public:
    QList<QNetworkRequest> requestList;

    QElapsedTimer timeOneRequest;
    QList<qint64> timeList;

    QElapsedTimer globalTime;

    QNetworkAccessManager manager;

    HttpsRequestChainHelper() {
    }
public slots:
    void doNextRequest() {
        // all requests done
        if (requestList.isEmpty()) {
            QTestEventLoop::instance().exitLoop();
            return;
        }

        if (qobject_cast<QNetworkReply*>(sender()) == 0) {
            // first start after DNS lookup, start timer
            globalTime.start();
        }
        QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
        if (reply) {
            QVERIFY(reply->error() == QNetworkReply::NoError);
            qDebug() << "time =" << timeOneRequest.elapsed() << "ms";
            timeList.append(timeOneRequest.elapsed());
        }

        QNetworkRequest request = requestList.takeFirst();
        timeOneRequest.restart();
        reply = manager.get(request);
        QObject::connect(reply, SIGNAL(sslErrors(QList<QSslError>)), reply, SLOT(ignoreSslErrors()));
        QObject::connect(reply, SIGNAL(finished()), this, SLOT(doNextRequest()));
    }

};

void tst_qnetworkreply::httpsRequestChain()
{
    int count = 10;

    QNetworkRequest request(QUrl("https://" + QtNetworkSettings::serverName() + "/fluke.gif"));
    // Disable keep-alive so we have the full re-connecting of TCP.
    request.setRawHeader("Connection", "close");

    HttpsRequestChainHelper helper;
    for (int i = 0; i < count; i++)
        helper.requestList.append(request);

    // Warm up DNS cache and then immediately start HTTP
    QHostInfo::lookupHost(QtNetworkSettings::serverName(), &helper, SLOT(doNextRequest()));

    // we can use QBENCHMARK_ONCE when we find out how to make it really run once.
    // there is still a warmup-run :(

    //QBENCHMARK_ONCE {
        QTestEventLoop::instance().enterLoop(40);
        QVERIFY(!QTestEventLoop::instance().timeout());
    //}

    qint64 elapsed = helper.globalTime.elapsed();

    qint64 average = (elapsed / count);

    std::sort(helper.timeList.begin(), helper.timeList.end());
    qint64 median = helper.timeList.at(5);

    qDebug() << "Total:" << elapsed << "   Average:" << average << "   Median:" << median;

}

void tst_qnetworkreply::runHttpsUploadRequest(const QByteArray &data, const QNetworkRequest &request)
{
    QNetworkReply* reply = manager.post(request, data);
    reply->ignoreSslErrors();
    connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(15);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QCOMPARE(reply->error(), QNetworkReply::NoError);
    reply->deleteLater();
}

void tst_qnetworkreply::httpsUpload()
{
    QByteArray data = QByteArray(2*1024*1024+1, '\177');
    QNetworkRequest request(QUrl("https://" + QtNetworkSettings::serverName() + "/qtest/cgi-bin/md5sum.cgi"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
//    for (int a = 0; a < 10; ++a)
//        runHttpsUploadRequest(data, request); // to warmup all TCP connections
    QBENCHMARK {
        runHttpsUploadRequest(data, request);
    }
}

void tst_qnetworkreply::preConnect_data()
{
    preConnectEncrypted_data();
}

void tst_qnetworkreply::preConnect()
{
    QString hostName = QLatin1String("www.google.com");

    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl("http://" + hostName));

    // make sure we have a full request including
    // DNS lookup and TCP handshake
#ifdef QT_BUILD_INTERNAL
    qt_qhostinfo_clear_cache();
#else
    qWarning("no internal build, could not clear DNS cache. Results may not be representative.");
#endif

    // first, benchmark a normal request
    QPair<QNetworkReply *, qint64> normalResult = runGetRequest(&manager, request);
    QNetworkReply *normalReply = normalResult.first;
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(normalReply->error() == QNetworkReply::NoError);
    qint64 normalElapsed = normalResult.second;

    // clear all caches again
#ifdef QT_BUILD_INTERNAL
    qt_qhostinfo_clear_cache();
#else
    qWarning("no internal build, could not clear DNS cache. Results may not be representative.");
#endif
    manager.clearAccessCache();

    // now try to make the connection beforehand
    QFETCH(std::chrono::milliseconds, sleepTime);
    manager.connectToHost(hostName);
    QTestEventLoop::instance().enterLoop(sleepTime);

    // now make another request and hopefully use the existing connection
    QPair<QNetworkReply *, qint64> preConnectResult = runGetRequest(&manager, request);
    QNetworkReply *preConnectReply = normalResult.first;
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(preConnectReply->error() == QNetworkReply::NoError);
    qint64 preConnectElapsed = preConnectResult.second;
    qDebug() << request.url().toString() << "full request:" << normalElapsed
             << "ms, pre-connect request:" << preConnectElapsed << "ms, difference:"
             << (normalElapsed - preConnectElapsed) << "ms";
}

QTEST_MAIN(tst_qnetworkreply)

#include "tst_qnetworkreply.moc"
