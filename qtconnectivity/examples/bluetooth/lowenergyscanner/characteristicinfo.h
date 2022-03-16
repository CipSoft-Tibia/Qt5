/***************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtBluetooth module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CHARACTERISTICINFO_H
#define CHARACTERISTICINFO_H
#include <QObject>
#include <QString>
#include <QtBluetooth/QLowEnergyCharacteristic>

class CharacteristicInfo: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString characteristicName READ getName NOTIFY characteristicChanged)
    Q_PROPERTY(QString characteristicUuid READ getUuid NOTIFY characteristicChanged)
    Q_PROPERTY(QString characteristicValue READ getValue NOTIFY characteristicChanged)
    Q_PROPERTY(QString characteristicHandle READ getHandle NOTIFY characteristicChanged)
    Q_PROPERTY(QString characteristicPermission READ getPermission NOTIFY characteristicChanged)

public:
    CharacteristicInfo() = default;
    CharacteristicInfo(const QLowEnergyCharacteristic &characteristic);
    void setCharacteristic(const QLowEnergyCharacteristic &characteristic);
    QString getName() const;
    QString getUuid() const;
    QString getValue() const;
    QString getHandle() const;
    QString getPermission() const;
    QLowEnergyCharacteristic getCharacteristic() const;

Q_SIGNALS:
    void characteristicChanged();

private:
    QLowEnergyCharacteristic m_characteristic;
};

#endif // CHARACTERISTICINFO_H
