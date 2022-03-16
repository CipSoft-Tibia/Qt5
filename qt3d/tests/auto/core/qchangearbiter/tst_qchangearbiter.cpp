/****************************************************************************
**
** Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
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

#include <QtTest/QTest>
#include <Qt3DCore/private/qobserverinterface_p.h>
#include <Qt3DCore/private/qobservableinterface_p.h>
#include <Qt3DCore/private/qchangearbiter_p.h>
#include <Qt3DCore/private/qpostman_p.h>
#include <Qt3DCore/qscenechange.h>
#include <Qt3DCore/qcomponentaddedchange.h>
#include <Qt3DCore/qcomponentremovedchange.h>
#include <Qt3DCore/qpropertyupdatedchange.h>
#include <Qt3DCore/qpropertynodeaddedchange.h>
#include <Qt3DCore/qpropertynoderemovedchange.h>
#include <Qt3DCore/qscenechange.h>
#include <Qt3DCore/private/qscene_p.h>
#include <Qt3DCore/qnode.h>
#include <Qt3DCore/qentity.h>
#include <Qt3DCore/qcomponent.h>
#include <Qt3DCore/qbackendnode.h>
#include <Qt3DCore/private/qsceneobserverinterface_p.h>
#include <Qt3DCore/private/qnode_p.h>
#include <Qt3DCore/private/qbackendnode_p.h>
#include <QThread>
#include <QWaitCondition>

class tst_QChangeArbiter : public QObject
{
    Q_OBJECT

private slots:
    void registerObservers();
    void registerSceneObserver();
    void unregisterObservers();
    void unregisterSceneObservers();
    void distributeFrontendChanges();
    void distributePropertyChanges();
    void distributeBackendChanges();
};

class AllChangesChange : public Qt3DCore::QSceneChange
{
public:
    AllChangesChange(Qt3DCore::QNodeId subjectId)
        : Qt3DCore::QSceneChange(Qt3DCore::AllChanges, subjectId)
    {
    }
};

class tst_Node : public Qt3DCore::QEntity
{
public:
    explicit tst_Node(Qt3DCore::QNode *parent = 0) : Qt3DCore::QEntity(parent)
    {}

    void sendNodeAddedNotification(QNode *node)
    {
        Qt3DCore::QPropertyNodeAddedChangePtr e(new Qt3DCore::QPropertyNodeAddedChange(id(), node));
        e->setPropertyName("PropertyValueAdded");
        Qt3DCore::QNodePrivate::get(this)->notifyObservers(e);
    }

    void sendNodeRemovedNotification(QNode *node)
    {
        Qt3DCore::QPropertyNodeRemovedChangePtr e(new Qt3DCore::QPropertyNodeRemovedChange(id(), node));
        e->setPropertyName("PropertyValueRemoved");
        Qt3DCore::QNodePrivate::get(this)->notifyObservers(e);
    }

    void sendNodeUpdatedNotification()
    {
        Qt3DCore::QPropertyUpdatedChangePtr e(new Qt3DCore::QPropertyUpdatedChange(id()));
        e->setPropertyName("PropertyUpdated");
        Qt3DCore::QNodePrivate::get(this)->notifyObservers(e);
    }

    void sendComponentAddedNotification(Qt3DCore::QComponent *component)
    {
        Qt3DCore::QComponentAddedChangePtr e(new Qt3DCore::QComponentAddedChange(this, component));
        Qt3DCore::QNodePrivate::get(this)->notifyObservers(e);
    }

    void sendComponentRemovedNotification(Qt3DCore::QComponent *component)
    {
        Qt3DCore::QComponentRemovedChangePtr e(new Qt3DCore::QComponentRemovedChange(this, component));
        Qt3DCore::QNodePrivate::get(this)->notifyObservers(e);
    }

    void sendAllChangesNotification()
    {
        Qt3DCore::QSceneChangePtr e(new AllChangesChange(id()));
        Qt3DCore::QNodePrivate::get(this)->notifyObservers(e);
    }

    void sceneChangeEvent(const Qt3DCore::QSceneChangePtr &change) override
    {
        QVERIFY(!change.isNull());
        m_lastChanges << change;
    }

    Qt3DCore::QSceneChangePtr lastChange() const
    {
        if (m_lastChanges.isEmpty())
            return Qt3DCore::QSceneChangePtr();
        return m_lastChanges.last();
    }

    QList<Qt3DCore::QSceneChangePtr> lastChanges() const
    {
        return m_lastChanges;
    }

private:
    QList<Qt3DCore::QSceneChangePtr> m_lastChanges;
};

// used to test property change notifications
class PropertyTestNode : public Qt3DCore::QNode
{
    Q_OBJECT
    Q_PROPERTY(int prop1 READ prop1 WRITE setProp1 NOTIFY prop1Changed)
    Q_PROPERTY(float prop2 READ prop2 WRITE setProp2 NOTIFY prop2Changed)

public:
    explicit PropertyTestNode(Qt3DCore::QNode *parent = 0) : Qt3DCore::QNode(parent)
    {}

    int prop1() const { return m_prop1; }
    void setProp1(int v)
    {
        if (m_prop1 != v) {
            m_prop1 = v;
            Q_EMIT prop1Changed(v);
        }
    }

    float prop2() const { return m_prop2; }
    void setProp2(float v)
    {
        if (m_prop2 != v) {
            m_prop2 = v;
            Q_EMIT prop2Changed(v);
        }
    }

Q_SIGNALS:
    void prop1Changed(int v);
    void prop2Changed(float v);

private:
    int m_prop1 = 0;
    float m_prop2 = 0.0f;
};

class tst_SimpleObserver : public Qt3DCore::QObserverInterface
{
public:

    // QObserverInterface interface
    void sceneChangeEvent(const Qt3DCore::QSceneChangePtr &e) override
    {
        QVERIFY(!e.isNull());
        m_lastChanges.append(e);
    }

    Qt3DCore::QSceneChangePtr lastChange() const
    {
        if (m_lastChanges.isEmpty())
            return Qt3DCore::QSceneChangePtr();
        return m_lastChanges.last();
    }

    QList<Qt3DCore::QSceneChangePtr> lastChanges() const
    {
        return m_lastChanges;
    }

    void clear()
    {
        m_lastChanges.clear();
    }

private:
    QList<Qt3DCore::QSceneChangePtr> m_lastChanges;
};

class tst_BackendObserverObservable : public Qt3DCore::QBackendNode
{
public:

    tst_BackendObserverObservable()
        : Qt3DCore::QBackendNode(ReadWrite)
    {}

    // QObserverInterface interface
    void sceneChangeEvent(const Qt3DCore::QSceneChangePtr &e) override
    {
        QVERIFY(!e.isNull());
        m_lastChanges << e;
        m_targetId = e->subjectId();
    }

    // should be called in thread
    void sendReply()
    {
        Qt3DCore::QPropertyUpdatedChangePtr reply;
        reply = QSharedPointer<Qt3DCore::QPropertyUpdatedChange>::create(m_targetId);
        reply->setDeliveryFlags(Qt3DCore::QSceneChange::DeliverToAll);
        reply->setPropertyName("Reply");
        notifyObservers(reply);
        qDebug() << Q_FUNC_INFO;
    }

    Qt3DCore::QSceneChangePtr lastChange() const
    {
        if (m_lastChanges.isEmpty())
            return Qt3DCore::QSceneChangePtr();
        return m_lastChanges.last();
    }

    QList<Qt3DCore::QSceneChangePtr> lastChanges() const
    {
        return m_lastChanges;
    }

    void clear()
    {
        m_lastChanges.clear();
    }

private:
    QList<Qt3DCore::QSceneChangePtr> m_lastChanges;
    Qt3DCore::QNodeId m_targetId;

};

class ThreadedAnswer : public QThread
{
    Q_OBJECT
public:
    ThreadedAnswer(Qt3DCore::QChangeArbiter *arbiter, tst_BackendObserverObservable *backend)
        : QThread()
        , m_arbiter(arbiter)
        , m_backendObs(backend)
    {}

    ~ThreadedAnswer() { qDebug() << this; }

    void run() override
    {
        // create backend change queue on QChangeArbiter
        Qt3DCore::QChangeArbiter::createThreadLocalChangeQueue(m_arbiter);
        m_backendObs->sendReply();
        // gives time for other threads to start waiting
        QThread::currentThread()->sleep(1);
        // wake waiting condition
        m_waitingForReplyToBeSent.wakeOne();
        exec();
        Qt3DCore::QChangeArbiter::destroyThreadLocalChangeQueue(m_arbiter);
    }

    QWaitCondition *waitingCondition() { return &m_waitingForReplyToBeSent; }

private:
    Qt3DCore::QChangeArbiter *m_arbiter;
    tst_BackendObserverObservable *m_backendObs;
    QWaitCondition m_waitingForReplyToBeSent;
};

class tst_PostManObserver : public Qt3DCore::QAbstractPostman
{
public:

    tst_PostManObserver()
        : m_sceneInterface(nullptr)
        , m_allowFrontendNotifications(false)
    {}

    void setScene(Qt3DCore::QScene *scene) final
    {
        m_sceneInterface = scene;
    }

    void setAllowFrontendNotifications(bool allow)
    {
        m_allowFrontendNotifications = allow;
    }

    // QObserverInterface interface
    void sceneChangeEvent(const Qt3DCore::QSceneChangePtr &e)
    {
        QVERIFY(!e.isNull());
        Qt3DCore::QPropertyUpdatedChangePtr change = qSharedPointerDynamicCast<Qt3DCore::QPropertyUpdatedChange>(e);
        QVERIFY(!change.isNull());
        Qt3DCore::QNode *targetNode = m_sceneInterface->lookupNode(change->subjectId());
        QVERIFY(targetNode != nullptr);
        m_lastChanges << e;
    }

    Qt3DCore::QSceneChangePtr lastChange() const
    {
        if (m_lastChanges.isEmpty())
            return Qt3DCore::QSceneChangePtr();
        return m_lastChanges.last();
    }

    QList<Qt3DCore::QSceneChangePtr> lastChanges() const
    {
        return m_lastChanges;
    }

    void notifyBackend(const Qt3DCore::QSceneChangePtr &e) final
    {
        m_sceneInterface->arbiter()->sceneChangeEventWithLock(e);
    }

    bool shouldNotifyFrontend(const Qt3DCore::QSceneChangePtr &)
    {
        return m_allowFrontendNotifications;
    }

private:
    Qt3DCore::QScene *m_sceneInterface;
    QList<Qt3DCore::QSceneChangePtr> m_lastChanges;
    bool m_allowFrontendNotifications;
};

class tst_SceneObserver : public Qt3DCore::QSceneObserverInterface
{
    // QSceneObserverInterface interface
public:
    void sceneNodeAdded(Qt3DCore::QSceneChangePtr &e)
    {
        QVERIFY(!e.isNull());
        QVERIFY(e->type() == Qt3DCore::NodeCreated);
        m_lastChange = e;
    }

    void sceneNodeRemoved(Qt3DCore::QSceneChangePtr &e)
    {
        QVERIFY(!e.isNull());
        QVERIFY((e->type() == Qt3DCore::NodeDeleted));
        m_lastChange = e;
    }

    void sceneNodeUpdated(Qt3DCore::QSceneChangePtr &e)
    {
        m_lastChange = e;
    }

    Qt3DCore::QSceneChangePtr lastChange() const
    {
        return m_lastChange;
    }

private:
    Qt3DCore::QSceneChangePtr m_lastChange;
};

void tst_QChangeArbiter::registerObservers()
{
    // GIVEN
    QScopedPointer<Qt3DCore::QChangeArbiter> arbiter(new Qt3DCore::QChangeArbiter());
    QScopedPointer<Qt3DCore::QScene> scene(new Qt3DCore::QScene());
    QScopedPointer<Qt3DCore::QAbstractPostman> postman(new tst_PostManObserver);
    arbiter->setPostman(postman.data());
    arbiter->setScene(scene.data());
    postman->setScene(scene.data());
    scene->setArbiter(arbiter.data());
    // Replaces initialize as we have no JobManager in this case
    Qt3DCore::QChangeArbiter::createThreadLocalChangeQueue(arbiter.data());

    // WHEN
    Qt3DCore::QNode *root = new tst_Node();
    Qt3DCore::QNode *child = new tst_Node();
    Qt3DCore::QNodePrivate::get(root)->setScene(scene.data());
    scene->addObservable(root);

    QList<tst_SimpleObserver *> observers;
    for (int i = 0; i < 5; i++) {
        tst_SimpleObserver *s = new tst_SimpleObserver();
        arbiter->registerObserver(s, root->id());
        observers << s;
    }

    arbiter->syncChanges();

    // THEN
    Q_FOREACH (tst_SimpleObserver *o, observers)
        QVERIFY(o->lastChange().isNull());

    child->setParent(root);
    arbiter->syncChanges();
    Q_FOREACH (tst_SimpleObserver *o, observers) {
        QCOMPARE(o->lastChanges().size(), 1);
        QVERIFY(o->lastChanges().last()->type() == Qt3DCore::PropertyValueAdded);
    }

    Qt3DCore::QChangeArbiter::destroyThreadLocalChangeQueue(arbiter.data());
}

void tst_QChangeArbiter::registerSceneObserver()
{
    // GIVEN
    Qt3DCore::QComponent dummyComponent;
    QScopedPointer<Qt3DCore::QChangeArbiter> arbiter(new Qt3DCore::QChangeArbiter());
    QScopedPointer<Qt3DCore::QScene> scene(new Qt3DCore::QScene());
    QScopedPointer<Qt3DCore::QAbstractPostman> postman(new tst_PostManObserver);
    arbiter->setPostman(postman.data());
    arbiter->setScene(scene.data());
    postman->setScene(scene.data());
    scene->setArbiter(arbiter.data());
    // Replaces initialize as we have no JobManager in this case
    Qt3DCore::QChangeArbiter::createThreadLocalChangeQueue(arbiter.data());

    // WHEN
    tst_Node *root = new tst_Node();
    Qt3DCore::QNode *child = new tst_Node();
    Qt3DCore::QNodePrivate::get(root)->setScene(scene.data());
    Qt3DCore::QNodePrivate::get(root)->m_hasBackendNode = true;
    scene->addObservable(root);

    QList<tst_SimpleObserver *> observers;
    for (int i = 0; i < 5; i++) {
        tst_SimpleObserver *s = new tst_SimpleObserver();
        arbiter->registerObserver(s, root->id());
        observers << s;
    }

    QList<tst_SceneObserver *> sceneObservers;
    for (int i = 0; i < 5; i++) {
        tst_SceneObserver *s = new tst_SceneObserver();
        arbiter->registerSceneObserver(s);
        sceneObservers << s;
    }

    arbiter->syncChanges();

    // THEN
    Q_FOREACH (tst_SimpleObserver *o, observers)
        QVERIFY(o->lastChange().isNull());
    Q_FOREACH (tst_SceneObserver *s, sceneObservers)
        QVERIFY(s->lastChange().isNull());

    // WHEN
    child->setParent(root);
    arbiter->syncChanges();

    // THEN
    Q_FOREACH (tst_SimpleObserver *o, observers) {
        QVERIFY(!o->lastChange().isNull());
        QVERIFY(o->lastChange()->type() == Qt3DCore::PropertyValueAdded);
    }
    Q_FOREACH (tst_SceneObserver *s, sceneObservers) {
        QVERIFY(!s->lastChange().isNull());
        QVERIFY(s->lastChange()->type() == Qt3DCore::NodeCreated);
    }

    // WHEN
    root->sendComponentAddedNotification(&dummyComponent);
    arbiter->syncChanges();

    // THEN
    Q_FOREACH (tst_SimpleObserver *o, observers) {
        QVERIFY(!o->lastChange().isNull());
        QVERIFY(o->lastChange()->type() == Qt3DCore::ComponentAdded);
    }
    Q_FOREACH (tst_SceneObserver *s, sceneObservers) {
        QVERIFY(!s->lastChange().isNull());
        QVERIFY(s->lastChange()->type() == Qt3DCore::NodeCreated);
    }

    Qt3DCore::QChangeArbiter::destroyThreadLocalChangeQueue(arbiter.data());
}

void tst_QChangeArbiter::unregisterObservers()
{
    // GIVEN
    QScopedPointer<Qt3DCore::QChangeArbiter> arbiter(new Qt3DCore::QChangeArbiter());
    QScopedPointer<Qt3DCore::QScene> scene(new Qt3DCore::QScene());
    QScopedPointer<Qt3DCore::QAbstractPostman> postman(new tst_PostManObserver);
    arbiter->setPostman(postman.data());
    arbiter->setScene(scene.data());
    postman->setScene(scene.data());
    scene->setArbiter(arbiter.data());
    // Replaces initialize as we have no JobManager in this case
    Qt3DCore::QChangeArbiter::createThreadLocalChangeQueue(arbiter.data());

    // WHEN
    tst_Node *root = new tst_Node();
    Qt3DCore::QNode *child = new tst_Node();
    Qt3DCore::QNodePrivate::get(root)->setScene(scene.data());
    scene->addObservable(root);

    QList<tst_SimpleObserver *> observers;
    for (int i = 0; i < 5; i++) {
        tst_SimpleObserver *s = new tst_SimpleObserver();
        arbiter->registerObserver(s, root->id());
        observers << s;
    }

    arbiter->syncChanges();

    // THEN
    Q_FOREACH (tst_SimpleObserver *o, observers)
        QVERIFY(o->lastChange().isNull());

    // WHEN
    child->setParent(root);
    arbiter->syncChanges();

    // THEN
    Q_FOREACH (tst_SimpleObserver *o, observers) {
        QVERIFY(!o->lastChange().isNull());
        QVERIFY(o->lastChange()->type() == Qt3DCore::PropertyValueAdded);
    }

    // WHEN
    Q_FOREACH (tst_SimpleObserver *o, observers)
        arbiter->unregisterObserver(o, root->id());

    root->sendAllChangesNotification();
    arbiter->syncChanges();

    // THEN
    Q_FOREACH (tst_SimpleObserver *o, observers) {
        QVERIFY(!o->lastChange().isNull());
        QVERIFY(o->lastChange()->type() == Qt3DCore::PropertyValueAdded);
    }

    Qt3DCore::QChangeArbiter::destroyThreadLocalChangeQueue(arbiter.data());
}

void tst_QChangeArbiter::unregisterSceneObservers()
{
    // GIVEN
    Qt3DCore::QComponent dummyComponent;
    QScopedPointer<Qt3DCore::QChangeArbiter> arbiter(new Qt3DCore::QChangeArbiter());
    QScopedPointer<Qt3DCore::QScene> scene(new Qt3DCore::QScene());
    QScopedPointer<Qt3DCore::QAbstractPostman> postman(new tst_PostManObserver);
    arbiter->setPostman(postman.data());
    arbiter->setScene(scene.data());
    postman->setScene(scene.data());
    scene->setArbiter(arbiter.data());
    // Replaces initialize as we have no JobManager in this case
    Qt3DCore::QChangeArbiter::createThreadLocalChangeQueue(arbiter.data());

    // WHEN
    tst_Node *root = new tst_Node();
    Qt3DCore::QNode *child = new tst_Node();
    Qt3DCore::QNodePrivate::get(root)->setScene(scene.data());
    Qt3DCore::QNodePrivate::get(root)->m_hasBackendNode = true;
    scene->addObservable(root);

    QList<tst_SimpleObserver *> observers;
    for (int i = 0; i < 5; i++) {
        tst_SimpleObserver *s = new tst_SimpleObserver();
        arbiter->registerObserver(s, root->id());
        observers << s;
    }

    QList<tst_SceneObserver *> sceneObservers;
    for (int i = 0; i < 5; i++) {
        tst_SceneObserver *s = new tst_SceneObserver();
        arbiter->registerSceneObserver(s);
        sceneObservers << s;
    }

    arbiter->syncChanges();

    // THEN
    Q_FOREACH (tst_SimpleObserver *o, observers)
        QVERIFY(o->lastChange().isNull());
    Q_FOREACH (tst_SceneObserver *s, sceneObservers)
        QVERIFY(s->lastChange().isNull());

    // WHEN
    child->setParent(root);
    arbiter->syncChanges();

    // THEN
    Q_FOREACH (tst_SimpleObserver *o, observers) {
        QVERIFY(!o->lastChange().isNull());
        QVERIFY(o->lastChange()->type() == Qt3DCore::PropertyValueAdded);
    }
    Q_FOREACH (tst_SceneObserver *s, sceneObservers) {
        QVERIFY(!s->lastChange().isNull());
        QVERIFY(s->lastChange()->type() == Qt3DCore::NodeCreated);
    }

    // WHEN
    root->sendComponentAddedNotification(&dummyComponent);
    arbiter->syncChanges();

    // THEN
    Q_FOREACH (tst_SimpleObserver *o, observers) {
        QVERIFY(!o->lastChange().isNull());
        QVERIFY(o->lastChange()->type() == Qt3DCore::ComponentAdded);
    }
    Q_FOREACH (tst_SceneObserver *s, sceneObservers) {
        QVERIFY(!s->lastChange().isNull());
        QVERIFY(s->lastChange()->type() == Qt3DCore::NodeCreated);
    }

    // WHEN
    child->setParent(Q_NODE_NULLPTR);
    arbiter->syncChanges();

    // THEN
    Q_FOREACH (tst_SimpleObserver *o, observers) {
        QVERIFY(!o->lastChange().isNull());
        QVERIFY(o->lastChange()->type() == Qt3DCore::PropertyValueRemoved);
    }
    Q_FOREACH (tst_SceneObserver *s, sceneObservers) {
        QVERIFY(!s->lastChange().isNull());
        QVERIFY(s->lastChange()->type() == Qt3DCore::NodeDeleted);
    }

    Q_FOREACH (tst_SceneObserver *s, sceneObservers)
        arbiter->unregisterSceneObserver(s);

    // WHEN
    child->setParent(root);
    arbiter->syncChanges();

    // THEN
    Q_FOREACH (tst_SimpleObserver *o, observers) {
        QVERIFY(!o->lastChange().isNull());
        QVERIFY(o->lastChange()->type() == Qt3DCore::PropertyValueAdded);
    }
    Q_FOREACH (tst_SceneObserver *s, sceneObservers) {
        QVERIFY(!s->lastChange().isNull());
        QVERIFY(s->lastChange()->type() == Qt3DCore::NodeDeleted);
    }

    Qt3DCore::QChangeArbiter::destroyThreadLocalChangeQueue(arbiter.data());
}

void tst_QChangeArbiter::distributeFrontendChanges()
{
    // GIVEN
    Qt3DCore::QComponent dummyComponent;
    Qt3DCore::QNode dummyNode;
    QScopedPointer<Qt3DCore::QChangeArbiter> arbiter(new Qt3DCore::QChangeArbiter());
    QScopedPointer<Qt3DCore::QScene> scene(new Qt3DCore::QScene());
    QScopedPointer<Qt3DCore::QAbstractPostman> postman(new tst_PostManObserver);
    arbiter->setPostman(postman.data());
    arbiter->setScene(scene.data());
    postman->setScene(scene.data());
    scene->setArbiter(arbiter.data());
    // Replaces initialize as we have no JobManager in this case
    Qt3DCore::QChangeArbiter::createThreadLocalChangeQueue(arbiter.data());

    // WHEN
    tst_Node *root = new tst_Node();
    Qt3DCore::QNodePrivate::get(root)->setScene(scene.data());
    scene->addObservable(root);

    tst_SimpleObserver *backendAllChangedObserver = new tst_SimpleObserver();
    tst_SimpleObserver *backendNodeAddedObserver = new tst_SimpleObserver();
    tst_SimpleObserver *backendNodeRemovedObserver = new tst_SimpleObserver();
    tst_SimpleObserver *backendNodeUpdatedObserver = new tst_SimpleObserver();
    tst_SimpleObserver *backendComponentAddedObserver = new tst_SimpleObserver();
    tst_SimpleObserver *backendComponentRemovedObserver = new tst_SimpleObserver();

    arbiter->registerObserver(backendAllChangedObserver, root->id());
    arbiter->registerObserver(backendNodeAddedObserver, root->id(), Qt3DCore::PropertyValueAdded);
    arbiter->registerObserver(backendNodeUpdatedObserver, root->id(), Qt3DCore::PropertyUpdated);
    arbiter->registerObserver(backendNodeRemovedObserver, root->id(), Qt3DCore::PropertyValueRemoved);
    arbiter->registerObserver(backendComponentAddedObserver, root->id(), Qt3DCore::ComponentAdded);
    arbiter->registerObserver(backendComponentRemovedObserver, root->id(), Qt3DCore::ComponentRemoved);

    arbiter->syncChanges();

    // THEN
    QVERIFY(backendAllChangedObserver->lastChange().isNull());
    QVERIFY(backendNodeAddedObserver->lastChange().isNull());
    QVERIFY(backendNodeUpdatedObserver->lastChange().isNull());
    QVERIFY(backendNodeRemovedObserver->lastChange().isNull());
    QVERIFY(backendComponentAddedObserver->lastChange().isNull());
    QVERIFY(backendComponentRemovedObserver->lastChange().isNull());

    // WHEN
    root->sendNodeAddedNotification(&dummyNode);
    arbiter->syncChanges();

    // THEN
    QCOMPARE(backendAllChangedObserver->lastChanges().count(), 1);
    QCOMPARE(backendNodeAddedObserver->lastChanges().count(), 1);
    QCOMPARE(backendNodeUpdatedObserver->lastChanges().count(), 0);
    QCOMPARE(backendNodeRemovedObserver->lastChanges().count(), 0);
    QCOMPARE(backendComponentAddedObserver->lastChanges().count(), 0);
    QCOMPARE(backendComponentRemovedObserver->lastChanges().count(), 0);

    // WHEN
    root->sendNodeUpdatedNotification();
    arbiter->syncChanges();

    // THEN
    QCOMPARE(backendAllChangedObserver->lastChanges().count(), 2);
    QCOMPARE(backendNodeAddedObserver->lastChanges().count(), 1);
    QCOMPARE(backendNodeUpdatedObserver->lastChanges().count(), 1);
    QCOMPARE(backendNodeRemovedObserver->lastChanges().count(), 0);
    QCOMPARE(backendComponentAddedObserver->lastChanges().count(), 0);
    QCOMPARE(backendComponentRemovedObserver->lastChanges().count(), 0);

    // WHEN
    root->sendNodeRemovedNotification(&dummyNode);
    arbiter->syncChanges();

    // THEN
    QCOMPARE(backendAllChangedObserver->lastChanges().count(), 3);
    QCOMPARE(backendNodeAddedObserver->lastChanges().count(), 1);
    QCOMPARE(backendNodeUpdatedObserver->lastChanges().count(), 1);
    QCOMPARE(backendNodeRemovedObserver->lastChanges().count(), 1);
    QCOMPARE(backendComponentAddedObserver->lastChanges().count(), 0);
    QCOMPARE(backendComponentRemovedObserver->lastChanges().count(), 0);

    // WHEN
    root->sendComponentAddedNotification(&dummyComponent);
    arbiter->syncChanges();

    // THEN
    QCOMPARE(backendAllChangedObserver->lastChanges().count(), 4);
    QCOMPARE(backendNodeAddedObserver->lastChanges().count(), 1);
    QCOMPARE(backendNodeUpdatedObserver->lastChanges().count(), 1);
    QCOMPARE(backendNodeRemovedObserver->lastChanges().count(), 1);
    QCOMPARE(backendComponentAddedObserver->lastChanges().count(), 1);
    QCOMPARE(backendComponentRemovedObserver->lastChanges().count(), 0);

    // WHEN
    root->sendComponentRemovedNotification(&dummyComponent);
    arbiter->syncChanges();

    // THEN
    QCOMPARE(backendAllChangedObserver->lastChanges().count(), 5);
    QCOMPARE(backendNodeAddedObserver->lastChanges().count(), 1);
    QCOMPARE(backendNodeUpdatedObserver->lastChanges().count(), 1);
    QCOMPARE(backendNodeRemovedObserver->lastChanges().count(), 1);
    QCOMPARE(backendComponentAddedObserver->lastChanges().count(), 1);
    QCOMPARE(backendComponentRemovedObserver->lastChanges().count(), 1);

    // WHEN
    root->sendAllChangesNotification();
    arbiter->syncChanges();

    // THEN
    QCOMPARE(backendAllChangedObserver->lastChanges().count(), 6);
    QCOMPARE(backendNodeAddedObserver->lastChanges().count(), 2);
    QCOMPARE(backendNodeUpdatedObserver->lastChanges().count(), 2);
    QCOMPARE(backendNodeRemovedObserver->lastChanges().count(), 2);
    QCOMPARE(backendComponentAddedObserver->lastChanges().count(), 2);
    QCOMPARE(backendComponentRemovedObserver->lastChanges().count(), 2);

    Qt3DCore::QChangeArbiter::destroyThreadLocalChangeQueue(arbiter.data());
}

void tst_QChangeArbiter::distributePropertyChanges()
{
    // GIVEN
    QScopedPointer<Qt3DCore::QChangeArbiter> arbiter(new Qt3DCore::QChangeArbiter());
    QScopedPointer<Qt3DCore::QScene> scene(new Qt3DCore::QScene());
    QScopedPointer<Qt3DCore::QAbstractPostman> postman(new tst_PostManObserver);
    arbiter->setPostman(postman.data());
    arbiter->setScene(scene.data());
    postman->setScene(scene.data());
    scene->setArbiter(arbiter.data());
    // Replaces initialize as we have no JobManager in this case
    Qt3DCore::QChangeArbiter::createThreadLocalChangeQueue(arbiter.data());

    // Test change notifications made to the root node:

    // WHEN
    PropertyTestNode *root = new PropertyTestNode();
    Qt3DCore::QNodePrivate::get(root)->setScene(scene.data());
    Qt3DCore::QNodePrivate::get(root)->m_hasBackendNode = true;
    scene->addObservable(root);

    tst_SimpleObserver *rootObserver = new tst_SimpleObserver();
    arbiter->registerObserver(rootObserver, root->id());
    arbiter->syncChanges();

    // THEN
    QVERIFY(rootObserver->lastChange().isNull());

    // WHEN
    root->setProp1(root->prop1() + 1);
    arbiter->syncChanges();

    // THEN
    QVERIFY(!rootObserver->lastChange().isNull());
    QCOMPARE(rootObserver->lastChange()->type(), Qt3DCore::PropertyUpdated);
    Qt3DCore::QPropertyUpdatedChangePtr propChange = qSharedPointerDynamicCast<Qt3DCore::QPropertyUpdatedChange>(rootObserver->lastChange());
    QCOMPARE(root->id(), propChange->subjectId());
    QCOMPARE(QString(propChange->propertyName()), QString("prop1"));

    // WHEN
    root->setProp2(root->prop2() + 1.f);
    arbiter->syncChanges();

    // THEN
    QVERIFY(!rootObserver->lastChange().isNull());
    QCOMPARE(rootObserver->lastChange()->type(), Qt3DCore::PropertyUpdated);
    propChange = qSharedPointerDynamicCast<Qt3DCore::QPropertyUpdatedChange>(rootObserver->lastChange());
    QCOMPARE(root->id(), propChange->subjectId());
    QCOMPARE(QString(propChange->propertyName()), QString("prop2"));

    // Test change notifications made to an entity that was added to the scene
    // via QNode::setParent()

    // WHEN
    PropertyTestNode *setParentChild = new PropertyTestNode();
    setParentChild->setParent(root);
    tst_SimpleObserver *setParentChildObserver = new tst_SimpleObserver();
    arbiter->registerObserver(setParentChildObserver, setParentChild->id());
    setParentChild->setProp2(setParentChild->prop2() + 1.f);
    arbiter->syncChanges();

    // THEN
    QVERIFY(!setParentChildObserver->lastChange().isNull());
    QCOMPARE(setParentChildObserver->lastChange()->type(), Qt3DCore::PropertyUpdated);
    propChange = qSharedPointerDynamicCast<Qt3DCore::QPropertyUpdatedChange>(setParentChildObserver->lastChange());
    QCOMPARE(setParentChild->id(), propChange->subjectId());
    QCOMPARE(QString(propChange->propertyName()), QString("prop2"));

    // Test change notifications made to an entity that was added to the scene
    // via the QNode() constructor parent parameter

    // WHEN
    PropertyTestNode *directChild = new PropertyTestNode(root);
    QCoreApplication::processEvents();  // make sure the post-ctor initialization is executed for the node
    tst_SimpleObserver *directChildObserver = new tst_SimpleObserver();
    arbiter->registerObserver(directChildObserver, directChild->id());
    directChild->setProp1(directChild->prop1() + 1);
    arbiter->syncChanges();

    // THEN
    QVERIFY(!directChildObserver->lastChange().isNull());
    QCOMPARE(directChildObserver->lastChange()->type(), Qt3DCore::PropertyUpdated);
    propChange = qSharedPointerDynamicCast<Qt3DCore::QPropertyUpdatedChange>(directChildObserver->lastChange());
    QCOMPARE(directChild->id(), propChange->subjectId());
    QCOMPARE(QString(propChange->propertyName()), QString("prop1"));

    Qt3DCore::QChangeArbiter::destroyThreadLocalChangeQueue(arbiter.data());
}

void tst_QChangeArbiter::distributeBackendChanges()
{

    // GIVEN
    QScopedPointer<Qt3DCore::QChangeArbiter> arbiter(new Qt3DCore::QChangeArbiter());
    QScopedPointer<Qt3DCore::QScene> scene(new Qt3DCore::QScene());
    QScopedPointer<tst_PostManObserver> postman(new tst_PostManObserver);
    arbiter->setPostman(postman.data());
    arbiter->setScene(scene.data());
    postman->setScene(scene.data());
    scene->setArbiter(arbiter.data());
    // Replaces initialize as we have no JobManager in this case
    Qt3DCore::QChangeArbiter::createThreadLocalChangeQueue(arbiter.data());

    // In order for backend -> frontend changes to work properly,
    // the backend notification should only be sent
    // from a worker thread which has a dedicated ChangeQueue in the
    // ChangeArbiter different than the frontend ChangeQueue. In this
    // test we will only check that the backend has received the frontend notification


    // WHEN
    tst_Node *root = new tst_Node();
    Qt3DCore::QNodePrivate::get(root)->setScene(scene.data());
    Qt3DCore::QNodePrivate::get(root)->m_hasBackendNode = true;
    scene->addObservable(root);

    tst_BackendObserverObservable *backenObserverObservable = new tst_BackendObserverObservable();
    arbiter->registerObserver(Qt3DCore::QBackendNodePrivate::get(backenObserverObservable), root->id());
    arbiter->scene()->addObservable(Qt3DCore::QBackendNodePrivate::get(backenObserverObservable), root->id());
    Qt3DCore::QBackendNodePrivate::get(backenObserverObservable)->setArbiter(arbiter.data());

    arbiter->syncChanges();

    // THEN
    QVERIFY(root->lastChange().isNull());
    QVERIFY(backenObserverObservable->lastChange().isNull());
    QCOMPARE(backenObserverObservable->lastChanges().count(), 0);

    // WHEN
    root->sendAllChangesNotification();
    arbiter->syncChanges();

    // THEN
    // backend observer receives event from frontend node "root"
    QCOMPARE(root->lastChanges().count(), 0);
    QCOMPARE(postman->lastChanges().count(), 0);
    QCOMPARE(backenObserverObservable->lastChanges().count(), 1);

    backenObserverObservable->clear();

    {
        // WHEN
        // simulate a worker thread
        QScopedPointer<ThreadedAnswer> answer(new ThreadedAnswer(arbiter.data(), backenObserverObservable));
        postman->setAllowFrontendNotifications(false);
        QWaitCondition *waitingForBackendReplyCondition = answer->waitingCondition();

        QMutex mutex;
        // sends reply from another thread (simulates job thread)
        answer->start();
        mutex.lock();
        waitingForBackendReplyCondition->wait(&mutex);
        mutex.unlock();

        // To verify that backendObserver sent a reply
        arbiter->syncChanges();

        // THEN
        // the repliers should receive it's reply
        QCOMPARE(backenObserverObservable->lastChanges().count(), 1);
        // verify that postMan has received the change
        QCOMPARE(postman->lastChanges().count(), 0);
        answer->exit();
        answer->wait();
        backenObserverObservable->clear();
    }

    {
        // WHEN
        // simulate a worker thread
        QScopedPointer<ThreadedAnswer> answer(new ThreadedAnswer(arbiter.data(), backenObserverObservable));
        postman->setAllowFrontendNotifications(true);
        QWaitCondition *waitingForBackendReplyCondition = answer->waitingCondition();
        QMutex mutex;
        // sends reply from another thread (simulates job thread)
        answer->start();
        mutex.lock();
        waitingForBackendReplyCondition->wait(&mutex);
        mutex.unlock();

        // To verify that backendObserver sent a reply
        arbiter->syncChanges();

        // THEN
        // the repliers should receive it's reply
        QCOMPARE(backenObserverObservable->lastChanges().count(), 1);
        // verify that postMan has received the change
        QCOMPARE(postman->lastChanges().count(), 1);

        // verify correctness of the reply
        Qt3DCore::QPropertyUpdatedChangePtr c = qSharedPointerDynamicCast<Qt3DCore::QPropertyUpdatedChange>(postman->lastChange());
        QVERIFY(!c.isNull());
        QVERIFY(c->subjectId() == root->id());
        qDebug() << c->propertyName();
        QVERIFY(strcmp(c->propertyName(), "Reply") == 0);
        QVERIFY(c->type() == Qt3DCore::PropertyUpdated);
        answer->exit();
        answer->wait();
    }

    Qt3DCore::QChangeArbiter::destroyThreadLocalChangeQueue(arbiter.data());
}

QTEST_MAIN(tst_QChangeArbiter)

#include "tst_qchangearbiter.moc"
