/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include "qgeoserviceproviderplugin_test.h"
#include "qgeocodingmanagerengine_test.h"
#include "qgeoroutingmanagerengine_test.h"
#include "qgeotiledmappingmanagerengine_test.h"
#include "qplacemanagerengine_test.h"

#include <QtPlugin>

namespace
{
    template<class EngineType>
    EngineType * createEngine(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString)
    {
        const QString failError = parameters.value(QStringLiteral("error")).toString();
        const QString failErrorString = parameters.value(QStringLiteral("errorString")).toString();

        if (!failError.isEmpty()) {
            *error = QGeoServiceProvider::Error(failError.toInt());
            *errorString = failErrorString;
            return 0;
        } else {
            return new EngineType(parameters, error, errorString);
        }
    }
}

QGeoServiceProviderFactoryTest::QGeoServiceProviderFactoryTest()
{
}

QGeoServiceProviderFactoryTest::~QGeoServiceProviderFactoryTest()
{
}

QGeoRoutingManagerEngine* QGeoServiceProviderFactoryTest::createRoutingManagerEngine(
            const QVariantMap &parameters,
            QGeoServiceProvider::Error *error, QString *errorString) const
{
    return createEngine<QGeoRoutingManagerEngineTest>(parameters, error, errorString);
}


QGeoCodingManagerEngine* QGeoServiceProviderFactoryTest::createGeocodingManagerEngine(
                const QVariantMap &parameters, QGeoServiceProvider::Error *error,
                QString *errorString) const
{
    return createEngine<QGeoCodingManagerEngineTest>(parameters, error, errorString);
}


QGeoMappingManagerEngine* QGeoServiceProviderFactoryTest::createMappingManagerEngine(
            const QVariantMap &parameters,
            QGeoServiceProvider::Error *error, QString *errorString) const
{
    return createEngine<QGeoTiledMappingManagerEngineTest>(parameters, error, errorString);
}

QPlaceManagerEngine* QGeoServiceProviderFactoryTest::createPlaceManagerEngine(
        const QVariantMap &parameters,
        QGeoServiceProvider::Error *error, QString *errorString) const
{
    Q_UNUSED(error);
    Q_UNUSED(errorString);
    return new QPlaceManagerEngineTest(parameters);
}
