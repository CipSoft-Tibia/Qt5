// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "bmimage_p.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonObject>

#include "bmtrimpath_p.h"

QT_BEGIN_NAMESPACE

BMImage::BMImage(const BMImage &other)
    : BMBase(other)
{
    m_position = other.m_position;
    m_radius = other.m_radius;
    m_image = other.m_image;
}

BMImage::BMImage(const QJsonObject &definition, const QVersionNumber &version, BMBase *parent)
{
    setParent(parent);
    construct(definition, version);
}

BMBase *BMImage::clone() const
{
    return new BMImage(*this);
}

void BMImage::construct(const QJsonObject &definition, const QVersionNumber &version)
{
    BMBase::parse(definition);
    if (m_hidden)
        return;

    qCDebug(lcLottieQtBodymovinParser) << "BMImage::construct():" << m_name;

    QJsonObject asset = definition.value(QLatin1String("asset")).toObject();
    QString assetString = asset.value(QLatin1String("p")).toString();

    if (assetString.startsWith(QLatin1String("data:image"))) {
        QStringList assetsDataStringList = assetString.split(QLatin1String(","));
        if (assetsDataStringList.size() > 1) {
            QByteArray assetData = QByteArray::fromBase64(assetsDataStringList[1].toLatin1());
            m_image.loadFromData(assetData);
        }
    }
    else {
        QFileInfo info(asset.value(QLatin1String("fileSource")).toString());
        QString url = info.path() + QDir::separator() + asset.value(QLatin1String("u")).toString() + assetString;
        QString path = QUrl(url).toLocalFile();
        m_image.load(path);
        if (m_image.isNull()) {
            qWarning() << "Unable to load file " << path;
        }
    }

    QJsonObject position = definition.value(QLatin1String("p")).toObject();
    position = resolveExpression(position);
    m_position.construct(position, version);

    QJsonObject radius = definition.value(QLatin1String("r")).toObject();
    radius = resolveExpression(radius);
    m_radius.construct(radius, version);
}

void BMImage::updateProperties(int frame)
{
    m_position.update(frame);
    m_radius.update(frame);

    m_center = QPointF(m_position.value().x() - m_radius.value() / 2,
                             m_position.value().y() - m_radius.value() / 2);
}

void BMImage::render(LottieRenderer &renderer) const
{
    renderer.render(*this);
}

QPointF BMImage::position() const
{
    return m_position.value();
}

qreal BMImage::radius() const
{
    return m_radius.value();
}

QT_END_NAMESPACE
