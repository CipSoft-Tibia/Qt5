// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKREPEATER_P_P_H
#define QQUICKREPEATER_P_P_H

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

#include "qquickrepeater_p.h"
#include "qquickitem_p.h"

#include <QtCore/qpointer.h>

QT_REQUIRE_CONFIG(quick_repeater);

QT_BEGIN_NAMESPACE

class QQmlContext;
class QQmlInstanceModel;
class QQuickRepeaterPrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickRepeater)

public:
    QQuickRepeaterPrivate();
    ~QQuickRepeaterPrivate();

private:
    void requestItems();

    QPointer<QQmlInstanceModel> model;
    QVariant dataSource;
    QPointer<QObject> dataSourceAsObject;
    bool ownModel : 1;
    bool dataSourceIsObject : 1;
    bool delegateValidated : 1;
    int itemCount;

    QVector<QPointer<QQuickItem> > deletables;
};

QT_END_NAMESPACE

#endif // QQUICKREPEATER_P_P_H
