/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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
#include "person.h"

ShoeDescription::ShoeDescription(QObject *parent)
: QObject(parent), m_size(0), m_price(0)
{
}

int ShoeDescription::size() const
{
    return m_size;
}

void ShoeDescription::setSize(int s)
{
    if (m_size == s)
        return;

    m_size = s;
    emit shoeChanged();
}

QColor ShoeDescription::color() const
{
    return m_color;
}

void ShoeDescription::setColor(const QColor &c)
{
    if (m_color == c)
        return;

    m_color = c;
    emit shoeChanged();
}

QString ShoeDescription::brand() const
{
    return m_brand;
}

void ShoeDescription::setBrand(const QString &b)
{
    if (m_brand == b)
        return;

    m_brand = b;
    emit shoeChanged();
}

qreal ShoeDescription::price() const
{
    return m_price;
}

void ShoeDescription::setPrice(qreal p)
{
    if (m_price == p)
        return;

    m_price = p;
    emit shoeChanged();
}

Person::Person(QObject *parent)
: QObject(parent)
{
}

QString Person::name() const
{
    return m_name;
}

void Person::setName(const QString &n)
{
    if (m_name == n)
        return;

    m_name = n;
    emit nameChanged();
}

ShoeDescription *Person::shoe()
{
    return &m_shoe;
}


Boy::Boy(QObject * parent)
: Person(parent)
{
}


Girl::Girl(QObject * parent)
: Person(parent)
{
}

