// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKREPEATER_P_H
#define QQUICKREPEATER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qquickitem.h"

#include <private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_repeater);

QT_BEGIN_NAMESPACE

class QQmlChangeSet;

class QQuickRepeaterPrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickRepeater : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged FINAL)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_CLASSINFO("DefaultProperty", "delegate")
    QML_NAMED_ELEMENT(Repeater)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickRepeater(QQuickItem *parent=nullptr);
    virtual ~QQuickRepeater();

    QVariant model() const;
    void setModel(const QVariant &);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *);

    int count() const;

    Q_INVOKABLE QQuickItem *itemAt(int index) const;

Q_SIGNALS:
    void modelChanged();
    void delegateChanged();
    void countChanged();

    void itemAdded(int index, QQuickItem *item);
    void itemRemoved(int index, QQuickItem *item);

private:
    void clear();
    void regenerate();

protected:
    void componentComplete() override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;

private Q_SLOTS:
    void createdItem(int index, QObject *item);
    void initItem(int, QObject *item);
    void modelUpdated(const QQmlChangeSet &changeSet, bool reset);

private:
    Q_DISABLE_COPY(QQuickRepeater)
    Q_DECLARE_PRIVATE(QQuickRepeater)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickRepeater)

#endif // QQUICKREPEATER_P_H
