/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include <Qt3DRender/private/qgeometryloaderfactory_p.h>

#include "objgeometryloader.h"
#include "plygeometryloader.h"
#include "stlgeometryloader.h"

QT_BEGIN_NAMESPACE

class DefaultGeometryLoaderPlugin : public Qt3DRender::QGeometryLoaderFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QGeometryLoaderFactory_iid FILE "default.json")
public:

    QStringList keys() const override
    {
        return QStringList() << OBJGEOMETRYLOADER_EXT
                             << PLYGEOMETRYLOADER_EXT
                             << STLGEOMETRYLOADER_EXT;
    }

    Qt3DRender::QGeometryLoaderInterface *create(const QString &ext) override
    {
        if (ext.compare(OBJGEOMETRYLOADER_EXT, Qt::CaseInsensitive) == 0)
            return new Qt3DRender::ObjGeometryLoader;
        else if (ext.compare(PLYGEOMETRYLOADER_EXT, Qt::CaseInsensitive) == 0)
            return new Qt3DRender::PlyGeometryLoader;
        else if (ext.compare(STLGEOMETRYLOADER_EXT, Qt::CaseInsensitive) == 0)
            return new Qt3DRender::StlGeometryLoader;
        return nullptr;
    }
};

QT_END_NAMESPACE

#include "main.moc"
