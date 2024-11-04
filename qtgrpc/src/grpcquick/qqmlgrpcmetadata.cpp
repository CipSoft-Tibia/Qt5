// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qqmlgrpcmetadata_p.h"
#include <QtCore/qstringtokenizer.h>

QT_BEGIN_NAMESPACE

QQmlGrpcMetadata::QQmlGrpcMetadata(QObject *parent)
    : QObject(parent)
{
}

QQmlGrpcMetadata::~QQmlGrpcMetadata() = default;

void QQmlGrpcMetadata::setData(const QVariantMap &data)
{
    if (m_variantdata == data)
        return;

    m_metadata.clear();
    m_variantdata = data;
    for (const auto&[key, val]: m_variantdata.asKeyValueRange()) {
        // Transform the variant map into a std::multimap
        for (const auto &it : QStringTokenizer(get<QString>(val), u','))
            m_metadata.insert(std::make_pair(key.toUtf8(), it.toUtf8()));
    }
    emit dataChanged();
}

QT_END_NAMESPACE
