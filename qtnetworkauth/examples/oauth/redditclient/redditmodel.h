// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef REDDITMODEL_H
#define REDDITMODEL_H

#include "redditwrapper.h"

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qpointer.h>

QT_FORWARD_DECLARE_CLASS(QNetworkReply)

class RedditModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    RedditModel(QObject *parent = nullptr);
    RedditModel(const QString &clientId, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    void grant();

signals:
    void error(const QString &errorString);

private slots:
    void update();

private:
    RedditWrapper redditWrapper;
    QPointer<QNetworkReply> liveThreadReply;
    QList<QJsonObject> threads;
};

#endif // REDDITMODEL_H
