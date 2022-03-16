/****************************************************************************
**
** Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
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

#include "quick3dchannelmapper_p.h"

QT_BEGIN_NAMESPACE

namespace Qt3DAnimation {
namespace Animation {
namespace Quick {

Quick3DChannelMapper::Quick3DChannelMapper(QObject *parent)
    : QObject(parent)
{
}

QQmlListProperty<QAbstractChannelMapping> Quick3DChannelMapper::qmlMappings()
{
    return QQmlListProperty<QAbstractChannelMapping>(this, 0,
                                                     &Quick3DChannelMapper::appendMapping,
                                                     &Quick3DChannelMapper::mappingCount,
                                                     &Quick3DChannelMapper::mappingAt,
                                                     &Quick3DChannelMapper::clearMappings);
}

void Quick3DChannelMapper::appendMapping(QQmlListProperty<QAbstractChannelMapping> *list, QAbstractChannelMapping *mapping)
{
    Quick3DChannelMapper *extension = qobject_cast<Quick3DChannelMapper *>(list->object);
    extension->parentMapper()->addMapping(mapping);
}

QAbstractChannelMapping *Quick3DChannelMapper::mappingAt(QQmlListProperty<QAbstractChannelMapping> *list, int index)
{
    Quick3DChannelMapper *extension = qobject_cast<Quick3DChannelMapper *>(list->object);
    return extension->parentMapper()->mappings().at(index);
}

int Quick3DChannelMapper::mappingCount(QQmlListProperty<QAbstractChannelMapping> *list)
{
    Quick3DChannelMapper *extension = qobject_cast<Quick3DChannelMapper *>(list->object);
    return extension->parentMapper()->mappings().count();
}

void Quick3DChannelMapper::clearMappings(QQmlListProperty<QAbstractChannelMapping> *list)
{
    Quick3DChannelMapper *extension = qobject_cast<Quick3DChannelMapper *>(list->object);
    const auto mappings = extension->parentMapper()->mappings();
    for (QAbstractChannelMapping *mapping : mappings)
        extension->parentMapper()->removeMapping(mapping);
}

} // namespace Quick
} // namespace Animation
} // namespace Qt3DAnimation

QT_END_NAMESPACE
