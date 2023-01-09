/****************************************************************************
**
** Copyright (C) 2013-2018 Esri <contracts@esri.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtLocation module of the Qt Toolkit.
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

#ifndef GEOTILEDMAPESRI_H
#define GEOTILEDMAPESRI_H

#include <QtLocation/private/qgeotiledmap_p.h>
#ifdef LOCATIONLABS
#include <QtLocation/private/qgeotiledmaplabs_p.h>
typedef QGeoTiledMapLabs Map;
#else
typedef QGeoTiledMap Map;
#endif

QT_BEGIN_NAMESPACE

class GeoTiledMappingManagerEngineEsri;

class GeoTiledMapEsri: public Map
{
    Q_OBJECT

public:
    explicit GeoTiledMapEsri(GeoTiledMappingManagerEngineEsri *engine, QObject *parent = nullptr);
    virtual ~GeoTiledMapEsri();

protected:
    void evaluateCopyrights(const QSet<QGeoTileSpec> &visibleTiles) override;

    inline GeoTiledMappingManagerEngineEsri *engine() const;

private:
    GeoTiledMappingManagerEngineEsri *m_engine;
    int m_mapId;
};

inline GeoTiledMappingManagerEngineEsri *GeoTiledMapEsri::engine() const
{
    return m_engine;
}

QT_END_NAMESPACE

#endif // GEOTILEDMAPESRI_H
