// Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QT3DCORE_QUICK_QUICK3DJOINT_P_H
#define QT3DCORE_QUICK_QUICK3DJOINT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QObject>
#include <Qt3DCore/qjoint.h>
#include <QtQml/QQmlListProperty>

#include <Qt3DQuick/private/qt3dquick_global_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DCore {
namespace Quick {

class Q_3DQUICKSHARED_PRIVATE_EXPORT Quick3DJoint : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<Qt3DCore::QJoint> childJoints READ childJoints)
public:
    explicit Quick3DJoint(QObject *parent = nullptr);

    QQmlListProperty<Qt3DCore::QJoint> childJoints();

    inline QJoint *parentJoint() const { return qobject_cast<QJoint*>(parent()); }
};

} // namespace Quick
} // namespace Qt3DCore

QT_END_NAMESPACE

#endif // QT3DCORE_QUICK_QUICK3DJOINT_P_H
