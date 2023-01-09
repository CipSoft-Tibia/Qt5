/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtPositioning module of the Qt Toolkit.
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
#ifndef WIDGET_H
#define WIDGET_H

#include "logwidget.h"

#include <QWidget>
#include <QGeoPositionInfoSource>

namespace Ui {
    class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(LogWidget *log, QWidget *parent = nullptr);
    ~Widget();

public slots:
    void positionUpdated(QGeoPositionInfo gpsPos);
    void setInterval(int msec);
    void positionTimedOut();
    void errorChanged(QGeoPositionInfoSource::Error err);
private slots:
    void on_buttonRetrieve_clicked();
    void on_buttonStart_clicked();
    void on_radioButton_2_clicked();
    void on_radioButton_clicked();
    void on_radioButton_3_clicked();
    void on_radioButton_4_clicked();

    void on_buttonUpdateSupported_clicked();
    void on_buttonResetError_clicked();

private:
    LogWidget *log = nullptr;
    Ui::Widget *ui;
    QGeoPositionInfoSource *m_posSource;
};

#endif // WIDGET_H
