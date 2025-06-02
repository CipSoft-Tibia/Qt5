// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef REDDITMODEL_H
#define REDDITMODEL_H

#include <QtNetworkAuth/qoauth2authorizationcodeflow.h>

#include <QtNetwork/qnetworkrequestfactory.h>

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qpointer.h>

QT_FORWARD_DECLARE_CLASS(QRestAccessManager)

class RedditModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    RedditModel(QObject *parent = nullptr);
    RedditModel(const QString &clientId, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

signals:
    void error(const QString &errorString);

private:
    void updateHotThreads();

    QNetworkRequestFactory redditApi;
    QRestAccessManager *network = nullptr;
    QOAuth2AuthorizationCodeFlow oauth2;
    QList<QJsonObject> threads;
};

#endif // REDDITMODEL_H
