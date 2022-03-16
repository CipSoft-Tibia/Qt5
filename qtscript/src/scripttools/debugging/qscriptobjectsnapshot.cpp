/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSCriptTools module of the Qt Toolkit.
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

#include "qscriptobjectsnapshot_p.h"

#include <QtCore/qmap.h>
#include <QtCore/qset.h>
#include <QtCore/qvector.h>
#include <QtCore/qnumeric.h>
#include <QtScript/qscriptvalueiterator.h>

QT_BEGIN_NAMESPACE

QScriptObjectSnapshot::QScriptObjectSnapshot()
{
}

QScriptObjectSnapshot::~QScriptObjectSnapshot()
{
}

static bool _q_equal(const QScriptValue &v1, const QScriptValue &v2)
{
    if (v1.strictlyEquals(v2))
        return true;
    if (v1.isNumber() && v2.isNumber() && qIsNaN(v1.toNumber()) && qIsNaN(v2.toNumber()))
        return true;
    return false;
}

QScriptObjectSnapshot::Delta QScriptObjectSnapshot::capture(const QScriptValue &object)
{
    Delta result;
    QMap<QString, QScriptValueProperty> currProps;
    QHash<QString, int> propertyNameToIndex;
    {
        int i = 0;
        QScriptValueIterator it(object);
        while (it.hasNext()) {
            it.next();
            QScriptValueProperty prop(it.name(), it.value(), it.flags());
            currProps.insert(it.name(), prop);
            propertyNameToIndex.insert(it.name(), i);
            ++i;
        }
        if (object.prototype().isValid()) {
            QString __proto__ = QString::fromLatin1("__proto__");
            QScriptValueProperty protoProp(
                __proto__, object.prototype(),
                QScriptValue::Undeletable | QScriptValue::ReadOnly);
            currProps.insert(__proto__, protoProp);
            propertyNameToIndex.insert(__proto__, i);
            ++i;
        }
    }

    QSet<QString> prevSet;
    for (int i = 0; i < m_properties.size(); ++i)
        prevSet.insert(m_properties.at(i).name());
    QSet<QString> currSet = currProps.keys().toSet();
    QSet<QString> removedProperties = prevSet - currSet;
    QSet<QString> addedProperties = currSet - prevSet;
    QSet<QString> maybeChangedProperties = currSet & prevSet;

    {
        QMap<int, QScriptValueProperty> am;
        QSet<QString>::const_iterator it;
        for (it = addedProperties.constBegin(); it != addedProperties.constEnd(); ++it) {
            int idx = propertyNameToIndex[*it];
            am[idx] = currProps[*it];
        }
        result.addedProperties = am.values();
    }

    {
        QSet<QString>::const_iterator it;
        for (it = maybeChangedProperties.constBegin(); it != maybeChangedProperties.constEnd(); ++it) {
            const QScriptValueProperty &p1 = currProps[*it];
            const QScriptValueProperty &p2 = findProperty(*it);
            if (!_q_equal(p1.value(), p2.value())
                || (p1.flags() != p2.flags())) {
                result.changedProperties.append(p1);
            }
        }
    }

    result.removedProperties = removedProperties.toList();

    m_properties = currProps.values();

    return result;
}

QScriptValueProperty QScriptObjectSnapshot::findProperty(const QString &name) const
{
    for (int i = 0; i < m_properties.size(); ++i) {
        if (m_properties.at(i).name() == name)
            return m_properties.at(i);
    }
    return QScriptValueProperty();
}

QScriptValuePropertyList QScriptObjectSnapshot::properties() const
{
    return m_properties;
}

QT_END_NAMESPACE
