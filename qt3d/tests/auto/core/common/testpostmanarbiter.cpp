/****************************************************************************
**
** Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
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

// TODO Remove in Qt6
#include <QtCore/qcompilerdetection.h>
QT_WARNING_DISABLE_DEPRECATED

#include "testpostmanarbiter.h"
#include <Qt3DCore/private/qnode_p.h>

QT_BEGIN_NAMESPACE

TestPostman::TestPostman(TestArbiter *arbiter)
    : m_arbiter(arbiter)
{}

void TestPostman::setScene(Qt3DCore::QScene *)
{}

void TestPostman::sceneChangeEvent(const Qt3DCore::QSceneChangePtr &)
{}

void TestPostman::notifyBackend(const Qt3DCore::QSceneChangePtr &e)
{
    m_arbiter->sceneChangeEventWithLock(e);
}

bool TestPostman::shouldNotifyFrontend(const Qt3DCore::QSceneChangePtr &)
{
    return false;
}

TestArbiter::TestArbiter()
    : m_postman(new TestPostman(this))
{
}

TestArbiter::~TestArbiter()
{
}

void TestArbiter::sceneChangeEvent(const Qt3DCore::QSceneChangePtr &e)
{
    events.push_back(e);
}

void TestArbiter::sceneChangeEventWithLock(const Qt3DCore::QSceneChangePtr &e)
{
    events.push_back(e);
}

void TestArbiter::sceneChangeEventWithLock(const Qt3DCore::QSceneChangeList &e)
{
    events += QVector<Qt3DCore::QSceneChangePtr>(e.begin(), e.end());
}

Qt3DCore::QAbstractPostman *TestArbiter::postman() const
{
    return m_postman;
}

void TestArbiter::setArbiterOnNode(Qt3DCore::QNode *node)
{
    Qt3DCore::QNodePrivate::get(node)->setArbiter(this);
    const auto childNodes = node->childNodes();
    for (Qt3DCore::QNode *n : childNodes)
        setArbiterOnNode(n);
}

void TestArbiter::addDirtyFrontEndNode(Qt3DCore::QNode *node)
{
    if (!dirtyNodes.contains(node))
        dirtyNodes << node;
}

void TestArbiter::addDirtyFrontEndNode(Qt3DCore::QNode *node, Qt3DCore::QNode *subNode, const char *property, Qt3DCore::ChangeFlag change)
{
    if (!dirtyNodes.contains(node))
        dirtyNodes << node;
    dirtySubNodes.push_back({node, subNode, change, property});
}

void TestArbiter::removeDirtyFrontEndNode(Qt3DCore::QNode *node)
{
    dirtyNodes.removeOne(node);
}

QT_END_NAMESPACE
