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

#include <QtCore/QDebug>

#include <Qt3DQuickRender/private/quick3dtexture_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {
namespace Render {
namespace Quick {

Quick3DTextureExtension::Quick3DTextureExtension(QObject *parent)
    : QObject(parent)
{
}

QQmlListProperty<QAbstractTextureImage> Quick3DTextureExtension::textureImages()
{
    return QQmlListProperty<QAbstractTextureImage>(this, 0,
                                                   &Quick3DTextureExtension::appendTextureImage,
                                                   &Quick3DTextureExtension::textureImageCount,
                                                   &Quick3DTextureExtension::textureImageAt,
                                                   &Quick3DTextureExtension::clearTextureImageList);
}

void Quick3DTextureExtension::appendTextureImage(QQmlListProperty<QAbstractTextureImage> *list, QAbstractTextureImage *textureImage)
{
    Quick3DTextureExtension *self = qobject_cast<Quick3DTextureExtension *>(list->object);
    if (self)
        self->parentTexture()->addTextureImage(textureImage);
}

QAbstractTextureImage *Quick3DTextureExtension::textureImageAt(QQmlListProperty<QAbstractTextureImage> *list, int index)
{
    Quick3DTextureExtension *self = qobject_cast<Quick3DTextureExtension *>(list->object);
    if (self)
        return self->parentTexture()->textureImages().at(index);
    return nullptr;
}

int Quick3DTextureExtension::textureImageCount(QQmlListProperty<QAbstractTextureImage> *list)
{
    Quick3DTextureExtension *self = qobject_cast<Quick3DTextureExtension *>(list->object);
    if (self)
        return self->parentTexture()->textureImages().count();
    return 0;
}

void Quick3DTextureExtension::clearTextureImageList(QQmlListProperty<QAbstractTextureImage> *list)
{
    if (Quick3DTextureExtension *self = qobject_cast<Quick3DTextureExtension *>(list->object)) {
        const auto images = self->parentTexture()->textureImages();
        for (QAbstractTextureImage *img : images)
            self->parentTexture()->removeTextureImage(img);
    }
}

} // namespace Quick
} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE
