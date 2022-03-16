/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Designer of the Qt Toolkit.
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

#include "dpi_chooser.h"

#include <deviceprofile_p.h>

#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qspinbox.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qcheckbox.h>

QT_BEGIN_NAMESPACE

enum { minDPI = 50, maxDPI = 400 };

namespace qdesigner_internal {

// Entry struct for predefined values
struct DPI_Entry {
    int dpiX;
    int dpiY;
    const char *description;
};

const struct DPI_Entry dpiEntries[] = {
    //: Embedded device standard screen resolution
    {  96,   96, QT_TRANSLATE_NOOP("DPI_Chooser", "Standard (96 x 96)") },
    //: Embedded device screen resolution
    { 179,  185, QT_TRANSLATE_NOOP("DPI_Chooser", "Greenphone (179 x 185)") },
    //: Embedded device high definition screen resolution
    { 192,  192, QT_TRANSLATE_NOOP("DPI_Chooser", "High (192 x 192)") }
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

Q_DECLARE_METATYPE(const struct qdesigner_internal::DPI_Entry*);

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

// -------------  DPI_Chooser

DPI_Chooser::DPI_Chooser(QWidget *parent) :
    QWidget(parent),
    m_systemEntry(new DPI_Entry),
    m_predefinedCombo(new QComboBox),
    m_dpiXSpinBox(new QSpinBox),
    m_dpiYSpinBox(new QSpinBox)
{
    typedef void (QComboBox::*QComboIntSignal)(int);

    // Predefined settings: System
    DeviceProfile::systemResolution(&(m_systemEntry->dpiX), &(m_systemEntry->dpiY));
    m_systemEntry->description = 0;
    const struct DPI_Entry *systemEntry = m_systemEntry;
    //: System resolution
    m_predefinedCombo->addItem(tr("System (%1 x %2)").arg(m_systemEntry->dpiX).arg(m_systemEntry->dpiY), QVariant::fromValue(systemEntry));
    // Devices. Exclude the system values as not to duplicate the entries
    for (const DPI_Entry &e : dpiEntries) {
        if (e.dpiX != m_systemEntry->dpiX || e.dpiY != m_systemEntry->dpiY)
            m_predefinedCombo->addItem(tr(e.description), QVariant::fromValue(&e));
    }
    m_predefinedCombo->addItem(tr("User defined"));

    setFocusProxy(m_predefinedCombo);
    m_predefinedCombo->setEditable(false);
    m_predefinedCombo->setCurrentIndex(0);
    connect(m_predefinedCombo, static_cast<QComboIntSignal>(&QComboBox::currentIndexChanged),
            this, &DPI_Chooser::syncSpinBoxes);
    // top row with predefined settings
    QVBoxLayout *vBoxLayout = new QVBoxLayout;
    vBoxLayout->setMargin(0);
    vBoxLayout->addWidget(m_predefinedCombo);
    // Spin box row
    QHBoxLayout *hBoxLayout = new QHBoxLayout;
    hBoxLayout->setMargin(0);

    m_dpiXSpinBox->setMinimum(minDPI);
    m_dpiXSpinBox->setMaximum(maxDPI);
    hBoxLayout->addWidget(m_dpiXSpinBox);
    //: DPI X/Y separator
    hBoxLayout->addWidget(new QLabel(tr(" x ")));

    m_dpiYSpinBox->setMinimum(minDPI);
    m_dpiYSpinBox->setMaximum(maxDPI);
    hBoxLayout->addWidget(m_dpiYSpinBox);

    hBoxLayout->addStretch();
    vBoxLayout->addLayout(hBoxLayout);
    setLayout(vBoxLayout);

    syncSpinBoxes();
}

DPI_Chooser::~DPI_Chooser()
{
    delete m_systemEntry;
}

void DPI_Chooser::getDPI(int *dpiX, int *dpiY) const
{
    *dpiX = m_dpiXSpinBox->value();
    *dpiY = m_dpiYSpinBox->value();
}

void DPI_Chooser::setDPI(int dpiX, int dpiY)
{
    // Default to system if it is something weird
    const bool valid = dpiX >= minDPI && dpiX <= maxDPI &&  dpiY >= minDPI && dpiY <= maxDPI;
    if (!valid) {
        m_predefinedCombo->setCurrentIndex(0);
        return;
    }
    // Try to find the values among the predefined settings
    const int count = m_predefinedCombo->count();
    int predefinedIndex = -1;
    for (int i = 0; i < count; i++) {
        const QVariant data = m_predefinedCombo->itemData(i);
        if (data.type() != QVariant::Invalid) {
            const struct DPI_Entry *entry = qvariant_cast<const struct DPI_Entry *>(data);
            if (entry->dpiX == dpiX && entry->dpiY == dpiY) {
                predefinedIndex = i;
                break;
            }
        }
    }
    if (predefinedIndex != -1) {
        m_predefinedCombo->setCurrentIndex(predefinedIndex); // triggers syncSpinBoxes()
    } else {
        setUserDefinedValues(dpiX, dpiY);
    }
}

void DPI_Chooser::setUserDefinedValues(int dpiX, int dpiY)
{
    const bool blocked = m_predefinedCombo->blockSignals(true);
    m_predefinedCombo->setCurrentIndex(m_predefinedCombo->count() - 1);
    m_predefinedCombo->blockSignals(blocked);

    m_dpiXSpinBox->setEnabled(true);
    m_dpiYSpinBox->setEnabled(true);
    m_dpiXSpinBox->setValue(dpiX);
    m_dpiYSpinBox->setValue(dpiY);
}

void DPI_Chooser::syncSpinBoxes()
{
    const int predefIdx = m_predefinedCombo->currentIndex();
    const QVariant data = m_predefinedCombo->itemData(predefIdx);

    // Predefined mode in which spin boxes are disabled or user defined?
    const bool userSetting = data.type() == QVariant::Invalid;
    m_dpiXSpinBox->setEnabled(userSetting);
    m_dpiYSpinBox->setEnabled(userSetting);

    if (!userSetting) {
        const struct DPI_Entry *entry = qvariant_cast<const struct DPI_Entry *>(data);
        m_dpiXSpinBox->setValue(entry->dpiX);
        m_dpiYSpinBox->setValue(entry->dpiY);
    }
}
}

QT_END_NAMESPACE
