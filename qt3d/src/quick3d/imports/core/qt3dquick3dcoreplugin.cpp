/****************************************************************************
**
** Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
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

#include "qt3dquick3dcoreplugin.h"

#include <Qt3DCore/qarmature.h>
#include <Qt3DCore/qabstractskeleton.h>
#include <Qt3DCore/qskeletonloader.h>
#include <Qt3DCore/qtransform.h>
#include <Qt3DCore/qjoint.h>
#include <QtCore/qvariantanimation.h>

#include <Qt3DQuick/private/quick3dnodev9_p.h>
#include <Qt3DQuick/private/quick3dentity_p.h>
#include <Qt3DQuick/private/quick3dentityloader_p.h>
#include <Qt3DQuick/private/quick3dnodeinstantiator_p.h>
#include <Qt3DQuick/private/quick3djoint_p.h>
#include <Qt3DQuick/private/qquaternionanimation_p.h>
#include <Qt3DQuick/private/qt3dquick_global_p.h>

#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

void Qt3DQuick3DCorePlugin::registerTypes(const char *uri)
{
    Qt3DCore::Quick::Quick3D_initialize();

    qmlRegisterUncreatableType<Qt3DCore::QComponent>(uri, 2, 0, "Component3D", QLatin1String(""));

    Qt3DCore::Quick::registerExtendedType<Qt3DCore::QEntity, Qt3DCore::Quick::Quick3DEntity>("QEntity", "Qt3D.Core/Entity", uri, 2, 0, "Entity");
    qmlRegisterType<Qt3DCore::Quick::Quick3DEntityLoader>(uri, 2, 0, "EntityLoader");
    qmlRegisterType<Qt3DCore::Quick::Quick3DEntityLoader, 12>(uri, 2, 12, "EntityLoader");
    qmlRegisterType<Qt3DCore::Quick::Quick3DNodeInstantiator>(uri, 2, 0, "NodeInstantiator");
    qmlRegisterType<Qt3DCore::QTransform>(uri, 2, 0, "Transform");
    qmlRegisterType<Qt3DCore::QArmature>(uri, 2, 10, "Armature");
    qmlRegisterUncreatableType<Qt3DCore::QAbstractSkeleton>(uri, 2, 10, "AbstractSkeleton", QStringLiteral("AbstractSkeleton is an abstract base class"));
    qmlRegisterType<Qt3DCore::QSkeletonLoader>(uri, 2, 10, "SkeletonLoader");

    qmlRegisterType<Qt3DCore::Quick::QQuaternionAnimation>(uri, 2, 0, "QuaternionAnimation");
    qRegisterAnimationInterpolator<QQuaternion>(Qt3DCore::Quick::q_quaternionInterpolator);

    // Ideally we want to make Node an uncreatable type
    // We would need qmlRegisterUncreatableExtendedType for that
    qmlRegisterExtendedUncreatableType<Qt3DCore::QNode, Qt3DCore::Quick::Quick3DNode>(uri, 2, 0, "Node", QStringLiteral("Node is a base class"));
    qmlRegisterExtendedUncreatableType<Qt3DCore::QNode, Qt3DCore::Quick::Quick3DNodeV9, 9>(uri, 2, 9, "Node", QStringLiteral("Node is a base class"));

    Qt3DCore::Quick::registerExtendedType<Qt3DCore::QJoint, Qt3DCore::Quick::Quick3DJoint>("QJoint", "Qt3D.Core/Joint", uri, 2, 10, "Joint");

    // Auto-increment the import to stay in sync with ALL future Qt minor versions
    qmlRegisterModule(uri, 2, QT_VERSION_MINOR);
}

Qt3DQuick3DCorePlugin::~Qt3DQuick3DCorePlugin()
{
    Qt3DCore::Quick::Quick3D_uninitialize();
}

QT_END_NAMESPACE
