/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qbinaryjsonobject_p.h"
#include "qbinaryjson_p.h"

#include <qjsonobject.h>

QT_BEGIN_NAMESPACE

QBinaryJsonObject::~QBinaryJsonObject()
{
    if (d && !d->ref.deref())
        delete d;
}

QBinaryJsonObject QBinaryJsonObject::fromJsonObject(const QJsonObject &object)
{
    QBinaryJsonObject binary;
    for (auto it = object.begin(), end = object.end(); it != end; ++it)
        binary.insert(it.key(), QBinaryJsonValue::fromJsonValue(it.value()));
    if (binary.d) // We want to compact it as it is a root item now
        binary.d->compactionCounter++;
    binary.compact();
    return binary;
}

void QBinaryJsonObject::insert(const QString &key, const QBinaryJsonValue &value)
{
    bool latinOrIntValue;
    uint valueSize = QBinaryJsonPrivate::Value::requiredStorage(value, &latinOrIntValue);

    bool latinKey = QBinaryJsonPrivate::useCompressed(key);
    uint valueOffset = sizeof(QBinaryJsonPrivate::Entry)
            + QBinaryJsonPrivate::qStringSize(key, latinKey);
    uint requiredSize = valueOffset + valueSize;

    if (!detach(requiredSize + sizeof(QBinaryJsonPrivate::offset))) // offset for the new index entry
        return;

    if (!o->length)
        o->tableOffset = sizeof(QBinaryJsonPrivate::Object);

    bool keyExists = false;
    uint pos = o->indexOf(key, &keyExists);
    if (keyExists)
        ++d->compactionCounter;

    uint off = o->reserveSpace(requiredSize, pos, 1, keyExists);
    if (!off)
        return;

    QBinaryJsonPrivate::Entry *e = o->entryAt(pos);
    e->value.type = value.t;
    e->value.latinKey = latinKey;
    e->value.latinOrIntValue = latinOrIntValue;
    e->value.value = QBinaryJsonPrivate::Value::valueToStore(
                value, reinterpret_cast<char *>(e) - reinterpret_cast<char *>(o) + valueOffset);
    QBinaryJsonPrivate::copyString(reinterpret_cast<char *>(e + 1), key, latinKey);
    if (valueSize) {
        QBinaryJsonPrivate::Value::copyData(value, reinterpret_cast<char *>(e) + valueOffset,
                                            latinOrIntValue);
    }

    if (d->compactionCounter > 32U && d->compactionCounter >= unsigned(o->length) / 2U)
        compact();
}

char *QBinaryJsonObject::takeRawData(uint *size) const
{
    if (d)
        return d->takeRawData(size);
    *size = 0;
    return nullptr;
}

bool QBinaryJsonObject::detach(uint reserve)
{
    if (!d) {
        if (reserve >= QBinaryJsonPrivate::Value::MaxSize) {
            qWarning("QBinaryJson: Document too large to store in data structure");
            return false;
        }
        d = new QBinaryJsonPrivate::MutableData(reserve, QJsonValue::Object);
        o = static_cast<QBinaryJsonPrivate::Object *>(d->header->root());
        d->ref.ref();
        return true;
    }
    if (reserve == 0 && d->ref.loadRelaxed() == 1)
        return true;

    QBinaryJsonPrivate::MutableData *x = d->clone(o, reserve);
    if (!x)
        return false;
    x->ref.ref();
    if (!d->ref.deref())
        delete d;
    d = x;
    o = static_cast<QBinaryJsonPrivate::Object *>(d->header->root());
    return true;
}

void QBinaryJsonObject::compact()
{
    if (!d || !d->compactionCounter)
        return;

    detach();
    d->compact();
    o = static_cast<QBinaryJsonPrivate::Object *>(d->header->root());
}

QT_END_NAMESPACE
