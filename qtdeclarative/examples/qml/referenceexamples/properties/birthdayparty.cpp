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
#include "birthdayparty.h"

BirthdayParty::BirthdayParty(QObject *parent)
: QObject(parent), m_host(nullptr)
{
}

// ![0]
Person *BirthdayParty::host() const
{
    return m_host;
}

void BirthdayParty::setHost(Person *c)
{
    m_host = c;
}

QQmlListProperty<Person> BirthdayParty::guests()
{
    return {this, this,
             &BirthdayParty::appendGuest,
             &BirthdayParty::guestCount,
             &BirthdayParty::guest,
             &BirthdayParty::clearGuests,
             &BirthdayParty::replaceGuest,
             &BirthdayParty::removeLastGuest};
}

void BirthdayParty::appendGuest(Person* p) {
    m_guests.append(p);
}


int BirthdayParty::guestCount() const
{
    return m_guests.count();
}

Person *BirthdayParty::guest(int index) const
{
    return m_guests.at(index);
}

void BirthdayParty::clearGuests() {
    m_guests.clear();
}

void BirthdayParty::replaceGuest(int index, Person *p)
{
    m_guests[index] = p;
}

void BirthdayParty::removeLastGuest()
{
    m_guests.removeLast();
}

// ![0]

void BirthdayParty::appendGuest(QQmlListProperty<Person>* list, Person* p) {
    reinterpret_cast< BirthdayParty* >(list->data)->appendGuest(p);
}

void BirthdayParty::clearGuests(QQmlListProperty<Person>* list) {
    reinterpret_cast< BirthdayParty* >(list->data)->clearGuests();
}

void BirthdayParty::replaceGuest(QQmlListProperty<Person> *list, int i, Person *p)
{
    reinterpret_cast< BirthdayParty* >(list->data)->replaceGuest(i, p);
}

void BirthdayParty::removeLastGuest(QQmlListProperty<Person> *list)
{
    reinterpret_cast< BirthdayParty* >(list->data)->removeLastGuest();
}

Person* BirthdayParty::guest(QQmlListProperty<Person>* list, int i) {
    return reinterpret_cast< BirthdayParty* >(list->data)->guest(i);
}

int BirthdayParty::guestCount(QQmlListProperty<Person>* list) {
    return reinterpret_cast< BirthdayParty* >(list->data)->guestCount();
}
