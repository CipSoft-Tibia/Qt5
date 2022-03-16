/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtLocation module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qgeomapparameter_p.h"
#include <QtCore/QVariant>
#include <QDebug>
#include <QMetaProperty>

QT_BEGIN_NAMESPACE

QGeoMapParameter::QGeoMapParameter(QObject *parent) : QObject(parent)
{

}

QGeoMapParameter::QGeoMapParameter(const QList<QPair<QLatin1String, QVariant> > &properties, QObject *parent) : QObject(parent)
{
    for (const auto &p: properties) {
        if (p.first == QLatin1String("type"))
            setType(p.second.toString());
        else
            updateProperty(p.first.data(), p.second);
    }
}

QGeoMapParameter::~QGeoMapParameter()
{
}

bool QGeoMapParameter::operator==(const QGeoMapParameter &other) const
{
    return (other.toVariantMap() == toVariantMap());
}

QString QGeoMapParameter::type() const
{
    return m_type;
}

void QGeoMapParameter::setType(const QString &type)
{
    if (m_type.isEmpty())
        m_type = type;
}

// DO NOT USE to set "type"
void QGeoMapParameter::updateProperty(const char *propertyName, QVariant value)
{
    setProperty(propertyName, value);
    // This should technically be emitted only for dynamically added properties.
    // Since this object has only type defined as Q_PROPERTY() which is a set-once
    // no check is really needed here.
    emit propertyUpdated(this, propertyName);
}

QVariantMap QGeoMapParameter::toVariantMap() const
{
    QVariantMap res;
    const QMetaObject *metaObj = metaObject();
    for (int i = 2; i < metaObj->propertyCount(); ++i) { // 0 is objectName, 1 is type, we want to skip both of them here.
        const char *propName = metaObj->property(i).name();
        res[QLatin1String(propName)] = property(propName);
    }
    return res;
}

bool QGeoMapParameter::hasProperty(const char *propertyName)
{
    return metaObject()->indexOfProperty(propertyName) != -1
            || dynamicPropertyNames().indexOf(QByteArray(propertyName)) != -1;
}

QT_END_NAMESPACE

