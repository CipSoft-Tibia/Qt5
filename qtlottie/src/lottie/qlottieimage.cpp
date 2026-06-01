// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qlottieimage_p.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonObject>

#include <QtLottie/private/qlottieconstants_p.h>

QT_BEGIN_NAMESPACE

QLottieImage::QLottieImage(const QLottieImage &other)
    : QLottieBase(other)
{
    m_url = other.m_url;
    m_size = other.m_size;
    m_image = other.m_image;
}

QLottieImage::QLottieImage(const QJsonObject &definition, QLottieBase *parent)
{
    setParent(parent);
    construct(definition);
}

QLottieBase *QLottieImage::clone() const
{
    return new QLottieImage(*this);
}

void QLottieImage::construct(const QJsonObject &definition)
{
    QLottieBase::parse(definition);
    if (m_hidden)
        return;

    qCDebug(lcLottieQtLottieParser) << "QLottieImage::construct():" << m_name;

    QJsonObject asset = definition.value(QLatin1String("asset")).toObject();
    QString assetString = asset.value(QLatin1String("p")).toString();

    if (assetString.startsWith(QLatin1String("data:image"))) {
        QStringList assetsDataStringList = assetString.split(QLatin1String(","));
        if (assetsDataStringList.size() > 1) {
            QByteArray assetData = QByteArray::fromBase64(assetsDataStringList[1].toLatin1());
            m_image.loadFromData(assetData);
        }
        if (m_image.isNull()) {
            qCWarning(lcLottieQtLottieParser) << "Unable to load embedded image asset"
                                              << asset.value(QLatin1String("id")).toString();
        }
    }
    else {
        QFileInfo info(asset.value(QLatin1String("fileSource")).toString());
        QString urlPath = info.path() + QLatin1Char('/')
                + asset.value(QLatin1String("u")).toString() + QLatin1Char('/') + assetString;
        m_url = QUrl(urlPath);
        m_url.setScheme(QLatin1String("file"));
        QString path = m_url.toLocalFile();
        m_image.load(path);
        if (m_image.isNull())
            qCWarning(lcLottieQtLottieParser) << "Unable to load file" << path;
    }

    const qreal width = asset.value(QLatin1String("w")).toDouble();
    const qreal height = asset.value(QLatin1String("h")).toDouble();
    m_size = QSizeF(width, height);
}

void QLottieImage::render(QLottieRenderer &renderer) const
{
    renderer.render(*this);
}

QT_END_NAMESPACE
