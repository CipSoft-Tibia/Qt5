/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSensors module of the Qt Toolkit.
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

#ifndef QTESTSENSORGESTUREPLUGIN_P_H
#define QTESTSENSORGESTUREPLUGIN_P_H

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

#include <QObject>
#include <qsensorgestureplugininterface.h>

class QTestSensorGesturePlugin : public QObject, public QSensorGesturePluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.QSensorGesturePluginInterface")
    Q_INTERFACES(QSensorGesturePluginInterface)

public:
    explicit QTestSensorGesturePlugin();
    ~QTestSensorGesturePlugin();

    QList<QSensorGestureRecognizer *> createRecognizers() override;

//    QStringList gestureSignals() const;
    QStringList supportedIds() const override;
    QString name() const override;

    QList<QSensorGestureRecognizer*> recognizers() const;

    QList<QSensorGestureRecognizer*> recognizersList;

};



#endif // QTESTSENSORGESTUREPLUGIN_H
