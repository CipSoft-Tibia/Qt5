// Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QT3DINPUT_INPUT_QUICK_QUICK3DINPUTSEQUENCE_H
#define QT3DINPUT_INPUT_QUICK_QUICK3DINPUTSEQUENCE_H

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

#include <Qt3DInput/qinputsequence.h>
#include <QtQml/QQmlListProperty>

#include <Qt3DQuickInput/private/qt3dquickinput_global_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DInput {
namespace Input {
namespace Quick {

class Q_3DQUICKINPUTSHARED_PRIVATE_EXPORT Quick3DInputSequence : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<Qt3DInput::QAbstractActionInput> sequences READ qmlActionInputs CONSTANT)
public:
    explicit Quick3DInputSequence(QObject *parent = nullptr);

    inline QInputSequence *parentSequence() const { return qobject_cast<QInputSequence *>(parent()); }
    QQmlListProperty<QAbstractActionInput> qmlActionInputs();
};

} // namespace Quick
} // namespace Input
} // namespace Qt3DInput

QT_END_NAMESPACE


#endif // QT3DINPUT_INPUT_QUICK_QUICK3DINPUTSEQUENCE_H
