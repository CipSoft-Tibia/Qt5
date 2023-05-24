// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef CHATMESSAGEMODEL_H
#define CHATMESSAGEMODEL_H

#include <QAbstractListModel>
#include <memory>

#include "simplechat.qpb.h"

class ChatMessageModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit ChatMessageModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;

    void append(const qtgrpc::examples::chat::ChatMessageRepeated &messages);

private:
    qtgrpc::examples::chat::ChatMessageRepeated m_container;
};

#endif // CHATMESSAGEMODEL_H
