// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "widget.h"
#include "ui_widget.h"
#include <QGeoPositionInfoSource>
#include <QDebug>

Widget::Widget(LogWidget *logWidget, QWidget *parent) :
    QWidget(parent),
    log(logWidget),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    qDebug() << "Available:" << QGeoPositionInfoSource::availableSources();
    m_posSource = QGeoPositionInfoSource::createDefaultSource(this);
    if (!m_posSource)
        qFatal("No Position Source created!");
    connect(m_posSource, SIGNAL(positionUpdated(QGeoPositionInfo)),
            this, SLOT(positionUpdated(QGeoPositionInfo)));

    connect(ui->horizontalSlider, SIGNAL(valueChanged(int)),
            this, SLOT(setInterval(int)));

    ui->groupBox->setLayout(ui->gridLayout);
    ui->horizontalSlider->setMinimum(m_posSource->minimumUpdateInterval());
    ui->labelTimeOut->setVisible(false);

    connect(m_posSource, SIGNAL(errorOccurred(QGeoPositionInfoSource::Error)),
            this, SLOT(errorChanged(QGeoPositionInfoSource::Error)));
    connect(m_posSource, &QGeoPositionInfoSource::supportedPositioningMethodsChanged,
            this, [this]() {
        auto methods = m_posSource->supportedPositioningMethods();
        const QString status = QStringLiteral("Satellite: %1 ").arg(bool(methods & QGeoPositionInfoSource::SatellitePositioningMethods))
                + QStringLiteral("Non-Satellite: %1").arg(bool(methods & QGeoPositionInfoSource::NonSatellitePositioningMethods));

        qDebug() << "Available Positioning Methods Changed" << status;
        log->appendLog(status);
    });
}

void Widget::positionUpdated(QGeoPositionInfo gpsPos)
{
    QGeoCoordinate coord = gpsPos.coordinate();
    ui->labelLatitude->setText(QString::number(coord.latitude()));
    ui->labelLongitude->setText(QString::number(coord.longitude()));
    ui->labelAltitude->setText(QString::number(coord.altitude()));
    ui->labelTimeStamp->setText(gpsPos.timestamp().toString());
    if (gpsPos.hasAttribute(QGeoPositionInfo::HorizontalAccuracy))
        ui->labelHAccuracy->setText(QString::number(gpsPos.attribute(QGeoPositionInfo::HorizontalAccuracy)));
    else
        ui->labelHAccuracy->setText(QStringLiteral("N/A"));

    if (gpsPos.hasAttribute(QGeoPositionInfo::VerticalAccuracy))
        ui->labelVAccuracy->setText(QString::number(gpsPos.attribute(QGeoPositionInfo::VerticalAccuracy)));
    else
        ui->labelVAccuracy->setText(QStringLiteral("N/A"));

    if (gpsPos.hasAttribute(QGeoPositionInfo::Direction))
        ui->labelDirection->setText(QString::number(gpsPos.attribute(QGeoPositionInfo::Direction)));
    else
        ui->labelDirection->setText(QStringLiteral("N/A"));

    if (gpsPos.hasAttribute(QGeoPositionInfo::GroundSpeed))
        ui->labelSpeed->setText(QString::number(gpsPos.attribute(QGeoPositionInfo::GroundSpeed)));
    else
        ui->labelSpeed->setText(QStringLiteral("N/A"));

    log->appendLog(coord.toString());
}

void Widget::positionTimedOut()
{
    ui->labelTimeOut->setVisible(true);
}

void Widget::errorChanged(QGeoPositionInfoSource::Error err)
{
    if (err == QGeoPositionInfoSource::UpdateTimeoutError) {
        // handle timeout
        positionTimedOut();
    } else {
        // handle other errors
        ui->labelErrorState->setText(QString::number(err));
        m_posSource->stopUpdates();
        ui->checkBox->setChecked(false);
    }
}

Widget::~Widget()
{
    delete ui;
}

void Widget::setInterval(int msec)
{
    m_posSource->setUpdateInterval(msec);
}

void Widget::on_buttonRetrieve_clicked()
{
    // Requesting current position for _one_ time
    m_posSource->requestUpdate(10000);
}

void Widget::on_buttonStart_clicked()
{
    // Either start or stop the current position info source
    bool running = ui->checkBox->isChecked();
    if (running) {
        ui->checkBox->setChecked(false);
        m_posSource->stopUpdates();
    } else {
        ui->checkBox->setChecked(true);
        m_posSource->startUpdates();
    }
}

void Widget::on_radioButton_clicked()
{
    m_posSource->setPreferredPositioningMethods(QGeoPositionInfoSource::NoPositioningMethods);
}

void Widget::on_radioButton_2_clicked()
{
    m_posSource->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
}

void Widget::on_radioButton_3_clicked()
{
    m_posSource->setPreferredPositioningMethods(QGeoPositionInfoSource::NonSatellitePositioningMethods);
}

void Widget::on_radioButton_4_clicked()
{
    m_posSource->setPreferredPositioningMethods(QGeoPositionInfoSource::AllPositioningMethods);
}

void Widget::on_buttonUpdateSupported_clicked()
{
    QGeoPositionInfoSource::PositioningMethods m = m_posSource->supportedPositioningMethods();
    QString text;
    switch (m) {
    case QGeoPositionInfoSource::NoPositioningMethods:
        text = QStringLiteral("None");
        break;
    case QGeoPositionInfoSource::SatellitePositioningMethods:
        text = QStringLiteral("Satellite");
        break;
    case QGeoPositionInfoSource::NonSatellitePositioningMethods:
        text = QStringLiteral("Non Satellite");
        break;
    case QGeoPositionInfoSource::AllPositioningMethods:
        text = QStringLiteral("All");
        break;
    }

    ui->labelSupported->setText(text);
}

void Widget::on_buttonResetError_clicked()
{
    ui->labelErrorState->setText(QStringLiteral("N/A"));
}
