// Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QT3DANIMATION_QCHANNELMAPPER_P_H
#define QT3DANIMATION_QCHANNELMAPPER_P_H

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

#include <Qt3DCore/private/qnode_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DAnimation {

class QAbstractChannelMapping;

class QChannelMapperPrivate : public Qt3DCore::QNodePrivate
{
public:
    QChannelMapperPrivate();

    Q_DECLARE_PUBLIC(QChannelMapper)

    QList<QAbstractChannelMapping *> m_mappings;
};

struct QChannelMapperData
{
    QList<Qt3DCore::QNodeId> mappingIds;
};

} // namespace Qt3DAnimation


QT_END_NAMESPACE

#endif // QT3DANIMATION_QCHANNELMAPPER_P_H
