/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLNATIVEDEBUGCONNECTOR_H
#define QQMLNATIVEDEBUGCONNECTOR_H

#include <private/qqmldebugconnector_p.h>
#include <QtCore/qvector.h>

QT_BEGIN_NAMESPACE

class QQmlNativeDebugConnector : public QQmlDebugConnector
{
    Q_OBJECT

public:
    QQmlNativeDebugConnector();
    ~QQmlNativeDebugConnector() override;

    bool blockingMode() const override;
    QQmlDebugService *service(const QString &name) const override;
    void addEngine(QJSEngine *engine) override;
    void removeEngine(QJSEngine *engine) override;
    bool hasEngine(QJSEngine *engine) const override;
    bool addService(const QString &name, QQmlDebugService *service) override;
    bool removeService(const QString &name) override;
    bool open(const QVariantHash &configuration) override;
    static void setDataStreamVersion(int version);

private:
    void sendMessage(const QString &name, const QByteArray &message);
    void sendMessages(const QString &name, const QList<QByteArray> &messages);
    void announceObjectAvailability(const QString &objectType, QObject *object, bool available);

    QVector<QQmlDebugService *> m_services;
    QVector<QJSEngine *> m_engines;
    bool m_blockingMode;
};

class QQmlNativeDebugConnectorFactory : public QQmlDebugConnectorFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlDebugConnectorFactory_iid FILE "qqmlnativedebugconnector.json")
public:
    QQmlDebugConnector *create(const QString &key) override;
};

QT_END_NAMESPACE

#endif // QQMLNATIVEDEBUGCONNECTOR_H

