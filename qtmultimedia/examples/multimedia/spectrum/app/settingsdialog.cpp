/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "settingsdialog.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(
            const QList<QAudioDeviceInfo> &availableInputDevices,
            const QList<QAudioDeviceInfo> &availableOutputDevices,
            QWidget *parent)
    :   QDialog(parent)
    ,   m_windowFunction(DefaultWindowFunction)
    ,   m_inputDeviceComboBox(new QComboBox(this))
    ,   m_outputDeviceComboBox(new QComboBox(this))
    ,   m_windowFunctionComboBox(new QComboBox(this))
{
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);

    // Populate combo boxes

    QAudioDeviceInfo device;
    foreach (device, availableInputDevices)
        m_inputDeviceComboBox->addItem(device.deviceName(),
                                       QVariant::fromValue(device));
    foreach (device, availableOutputDevices)
        m_outputDeviceComboBox->addItem(device.deviceName(),
                                       QVariant::fromValue(device));

    m_windowFunctionComboBox->addItem(tr("None"), QVariant::fromValue(int(NoWindow)));
    m_windowFunctionComboBox->addItem("Hann", QVariant::fromValue(int(HannWindow)));
    m_windowFunctionComboBox->setCurrentIndex(m_windowFunction);

    // Initialize default devices
    if (!availableInputDevices.empty())
        m_inputDevice = availableInputDevices.front();
    if (!availableOutputDevices.empty())
        m_outputDevice = availableOutputDevices.front();

    // Add widgets to layout

    QScopedPointer<QHBoxLayout> inputDeviceLayout(new QHBoxLayout);
    QLabel *inputDeviceLabel = new QLabel(tr("Input device"), this);
    inputDeviceLayout->addWidget(inputDeviceLabel);
    inputDeviceLayout->addWidget(m_inputDeviceComboBox);
    dialogLayout->addLayout(inputDeviceLayout.data());
    inputDeviceLayout.take(); // ownership transferred to dialogLayout

    QScopedPointer<QHBoxLayout> outputDeviceLayout(new QHBoxLayout);
    QLabel *outputDeviceLabel = new QLabel(tr("Output device"), this);
    outputDeviceLayout->addWidget(outputDeviceLabel);
    outputDeviceLayout->addWidget(m_outputDeviceComboBox);
    dialogLayout->addLayout(outputDeviceLayout.data());
    outputDeviceLayout.take(); // ownership transferred to dialogLayout

    QScopedPointer<QHBoxLayout> windowFunctionLayout(new QHBoxLayout);
    QLabel *windowFunctionLabel = new QLabel(tr("Window function"), this);
    windowFunctionLayout->addWidget(windowFunctionLabel);
    windowFunctionLayout->addWidget(m_windowFunctionComboBox);
    dialogLayout->addLayout(windowFunctionLayout.data());
    windowFunctionLayout.take(); // ownership transferred to dialogLayout

    // Connect
    connect(m_inputDeviceComboBox, QOverload<int>::of(&QComboBox::activated),
            this, &SettingsDialog::inputDeviceChanged);
    connect(m_outputDeviceComboBox, QOverload<int>::of(&QComboBox::activated),
            this, &SettingsDialog::outputDeviceChanged);
    connect(m_windowFunctionComboBox, QOverload<int>::of(&QComboBox::activated),
            this, &SettingsDialog::windowFunctionChanged);

    // Add standard buttons to layout
    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    dialogLayout->addWidget(buttonBox);

    // Connect standard buttons
    connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked,
            this, &SettingsDialog::accept);
    connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
            this, &SettingsDialog::reject);

    setLayout(dialogLayout);
}

SettingsDialog::~SettingsDialog()
{

}

void SettingsDialog::windowFunctionChanged(int index)
{
    m_windowFunction = static_cast<WindowFunction>(
            m_windowFunctionComboBox->itemData(index).value<int>());
}

void SettingsDialog::inputDeviceChanged(int index)
{
    m_inputDevice = m_inputDeviceComboBox->itemData(index).value<QAudioDeviceInfo>();
}

void SettingsDialog::outputDeviceChanged(int index)
{
    m_outputDevice = m_outputDeviceComboBox->itemData(index).value<QAudioDeviceInfo>();
}

