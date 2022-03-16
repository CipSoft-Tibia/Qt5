/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Designer of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDESIGNER_SERVER_H
#define QDESIGNER_SERVER_H

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QTcpServer;
class QTcpSocket;

class QDesignerServer: public QObject
{
    Q_OBJECT
public:
    explicit QDesignerServer(QObject *parent = 0);
    ~QDesignerServer() override;

    quint16 serverPort() const;

    static void sendOpenRequest(int port, const QStringList &files);

private slots:
    void handleNewConnection();
    void readFromClient();
    void socketClosed();

private:
    QTcpServer *m_server;
    QTcpSocket *m_socket;
};

class QDesignerClient: public QObject
{
    Q_OBJECT
public:
    explicit QDesignerClient(quint16 port, QObject *parent = 0);
    ~QDesignerClient() override;

private slots:
    void readFromSocket();

private:
    QTcpSocket *m_socket;
};

QT_END_NAMESPACE

#endif // QDESIGNER_SERVER_H
