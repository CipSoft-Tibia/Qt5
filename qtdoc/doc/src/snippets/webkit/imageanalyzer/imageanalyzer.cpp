// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause


#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QtConcurrentMap>
#include "imageanalyzer.h"

/*!
 * This class operates as follows:
 * Parent calls the slot startAnalysis which shoves a list of QStrings into URLQueue and then calls fetchURLs.
 * FetchURLs sends out HTTP GETs for each image it can't get out of the cache.
 * As the responses come in, handleReply tries to create an image out of each and pushes those images into imageQueue.
 * On the last (detected by no outstandingFetches and URLQueue.isEmpty()) call to queueImage (from handleReply)
 * a thread is forked to process all the images. When it finishes, it emits a finished signal that is received
 * by our JavaScript code.
 */

//! [ ImageAnalyzer - Constructor ]
ImageAnalyzer::ImageAnalyzer(QNetworkDiskCache* netcache, QObject* parent)
    : QObject(parent), m_cache(netcache), m_outstandingFetches(0)
{
    /*  ImageAnalyzer only wants to receive http responses
        for requests that it makes, so that's why it has its own
        QNetworkAccessManager. */
    m_network = new QNetworkAccessManager(this);
    m_watcher = new QFutureWatcher<QRgb>(this);
    /*  We want to share a cache with the web browser,
        in case it has some images we want: */
    m_network->setCache(m_cache);

    QObject::connect(m_network, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(handleReply(QNetworkReply*)));
    QObject::connect(m_watcher, SIGNAL(finished()),
                     this, SLOT(doneProcessing()));
    QObject::connect(m_watcher, SIGNAL(progressValueChanged(int)),
                     this, SLOT(progressStatus(int)));
}
//! [ ImageAnalyzer - Constructor ]
ImageAnalyzer::~ImageAnalyzer()
{
    delete(m_watcher);
}


QRgb ImageAnalyzer::lastResults()
{
    int rTot = 0;
    int bTot = 0;
    int gTot = 0;
    int count = m_watcher->future().results().size();
    foreach (const QRgb & triplet, m_watcher->future().results())
    {
        rTot += qRed(triplet);
        bTot += qBlue(triplet);
        gTot += qGreen(triplet);
    }
    return qRgb(rTot/count, bTot/count, gTot/count);
}

float ImageAnalyzer::lastRed() { return qRed(lastResults())/2.55; }
float ImageAnalyzer::lastGreen() { return qGreen(lastResults())/2.55; }
float ImageAnalyzer::lastBlue() { return qBlue(lastResults())/2.55; }

void ImageAnalyzer::progressStatus(int newstat)
{
    emit updateProgress(newstat, m_watcher->progressMaximum());
}


bool ImageAnalyzer::isBusy()
{
    return m_watcher->isRunning();
}


//! [ ImageAnalyzer - startAnalysis ]
void ImageAnalyzer::startAnalysis(const QStringList & urls)
{
    m_URLQueue = urls;
    fetchURLs();
}
//! [ ImageAnalyzer - startAnalysis ]

/*!
 * Analyzes the entire queue - just starts all our http GETs.
 */
//! [ ImageAnalyzer - fetchURLs ]
void ImageAnalyzer::fetchURLs()
{
    while (!m_URLQueue.isEmpty())
    {
        QString url = m_URLQueue.takeFirst();
        QUrl URL = QUrl(url);
        QIODevice * pData = m_cache->data(URL);
        // Is image already loaded in cache?
        if (pData == 0) {
            // HTTP Get image over network.
            m_outstandingFetches++;
            QNetworkRequest request = QNetworkRequest(URL);
            request.setRawHeader("User-Agent", "Digia - Custom Qt app");
            m_network->get(request);
        } else {
            // Get image from cache
            QImage image;
            image.load(pData, 0);
            if (!image.isNull())
                queueImage(image);
            delete(pData);
        }
    }
}
//! [ ImageAnalyzer - fetchURLs ]
/*
 * Slot to handle the incoming responses from our http GETs
 */
//! [ ImageAnalyzer - handleReply ]
void ImageAnalyzer::handleReply(QNetworkReply * pReply)
{
    m_outstandingFetches--;
    if (pReply->error()) {
        qDebug() << "Error code" << pReply->error();
        qDebug() << "Http code" << pReply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        return;
    }
    QImage image;
    image.load(pReply, 0);
    pReply->deleteLater();
    if (image.isNull()) {
        qDebug() << "bad image";
        qDebug() << pReply->rawHeaderList();
        foreach (QByteArray element, pReply->rawHeaderList()) {
            qDebug() << element << " = " << pReply->rawHeader(element);
        }
        return;
    }
    queueImage(image);
}
//! [ ImageAnalyzer - handleReply ]

void ImageAnalyzer::doneProcessing()
{
    m_imageQueue = QList<QImage>();
    emit finishedAnalysis();
}
//! [ ImageAnalyzer - queueImage ]
void ImageAnalyzer::queueImage(QImage img)
{
    if (!img.isNull())
        m_imageQueue << img;

    if (m_outstandingFetches == 0 && m_URLQueue.isEmpty()) {
        m_watcher->setFuture(QtConcurrent::mapped(m_imageQueue, averageRGB));
    }
}
//! [ ImageAnalyzer - queueImage ]

//! [ ImageAnalyzer - averageRGB ]
QRgb averageRGB(const QImage &img)
{
    int pixelCount = img.width() * img.height();
    int rAvg, gAvg, bAvg;

    // We waste some time here:
    for (int timeWaster=0; timeWaster < 100; timeWaster++) {
        quint64 rTot = 0;
        quint64 gTot = 0;
        quint64 bTot = 0;
        for (int i=0; i < img.width(); i++) {
            for (int j=0; j < img.height(); j++) {
                QRgb pixel = img.pixel(i,j);
                rTot += qRed(pixel);
                gTot += qGreen(pixel);
                bTot += qBlue(pixel);
            }
        }
        rAvg = (rTot)/(pixelCount);
        gAvg = (gTot)/(pixelCount);
        bAvg = (bTot)/(pixelCount);
    }
    return qRgb(rAvg, gAvg, bAvg);
}
//! [ ImageAnalyzer - averageRGB ]
