/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "fontpanel.h"

#include <QtWidgets/QLabel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QFontComboBox>
#include <QtCore/QTimer>
#include <QtWidgets/QLineEdit>

QT_BEGIN_NAMESPACE

FontPanel::FontPanel(QWidget *parentWidget) :
    QGroupBox(parentWidget),
    m_previewLineEdit(new QLineEdit),
    m_writingSystemComboBox(new QComboBox),
    m_familyComboBox(new QFontComboBox),
    m_styleComboBox(new QComboBox),
    m_pointSizeComboBox(new QComboBox),
    m_previewFontUpdateTimer(0)
{
    setTitle(tr("Font"));

    QFormLayout *formLayout = new QFormLayout(this);
    // writing systems
    m_writingSystemComboBox->setEditable(false);

    QList<QFontDatabase::WritingSystem> writingSystems = m_fontDatabase.writingSystems();
    writingSystems.push_front(QFontDatabase::Any);
    for (QFontDatabase::WritingSystem ws : qAsConst(writingSystems))
        m_writingSystemComboBox->addItem(QFontDatabase::writingSystemName(ws), QVariant(ws));
    connect(m_writingSystemComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FontPanel::slotWritingSystemChanged);
    formLayout->addRow(tr("&Writing system"), m_writingSystemComboBox);

    connect(m_familyComboBox, &QFontComboBox::currentFontChanged,
            this, &FontPanel::slotFamilyChanged);
    formLayout->addRow(tr("&Family"), m_familyComboBox);

    m_styleComboBox->setEditable(false);
    connect(m_styleComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FontPanel::slotStyleChanged);
    formLayout->addRow(tr("&Style"), m_styleComboBox);

    m_pointSizeComboBox->setEditable(false);
    connect(m_pointSizeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FontPanel::slotPointSizeChanged);
    formLayout->addRow(tr("&Point size"), m_pointSizeComboBox);

    m_previewLineEdit->setReadOnly(true);
    formLayout->addRow(m_previewLineEdit);

    setWritingSystem(QFontDatabase::Any);
}

QFont FontPanel::selectedFont() const
{
    QFont rc = m_familyComboBox->currentFont();
    const QString family = rc.family();
    rc.setPointSize(pointSize());
    const QString styleDescription = styleString();
    if (styleDescription.contains(QLatin1String("Italic")))
        rc.setStyle(QFont::StyleItalic);
    else if (styleDescription.contains(QLatin1String("Oblique")))
        rc.setStyle(QFont::StyleOblique);
    else
        rc.setStyle(QFont::StyleNormal);
    rc.setBold(m_fontDatabase.bold(family, styleDescription));

    // Weight < 0 asserts...
    const int weight = m_fontDatabase.weight(family, styleDescription);
    if (weight >= 0)
        rc.setWeight(weight);
    return rc;
}

void FontPanel::setSelectedFont(const QFont &f)
{
    m_familyComboBox->setCurrentFont(f);
    if (m_familyComboBox->currentIndex() < 0) {
        // family not in writing system - find the corresponding one?
        QList<QFontDatabase::WritingSystem> familyWritingSystems = m_fontDatabase.writingSystems(f.family());
        if (familyWritingSystems.empty())
            return;

        setWritingSystem(familyWritingSystems.front());
        m_familyComboBox->setCurrentFont(f);
    }

    updateFamily(family());

    const int pointSizeIndex = closestPointSizeIndex(f.pointSize());
    m_pointSizeComboBox->setCurrentIndex( pointSizeIndex);

    const QString styleString = m_fontDatabase.styleString(f);
    const int styleIndex = m_styleComboBox->findText(styleString);
    m_styleComboBox->setCurrentIndex(styleIndex);
    slotUpdatePreviewFont();
}


QFontDatabase::WritingSystem FontPanel::writingSystem() const
{
    const int currentIndex = m_writingSystemComboBox->currentIndex();
    if ( currentIndex == -1)
        return QFontDatabase::Latin;
    return static_cast<QFontDatabase::WritingSystem>(m_writingSystemComboBox->itemData(currentIndex).toInt());
}

QString FontPanel::family() const
{
    const int currentIndex = m_familyComboBox->currentIndex();
    return currentIndex != -1 ?  m_familyComboBox->currentFont().family() : QString();
}

int FontPanel::pointSize() const
{
    const int currentIndex = m_pointSizeComboBox->currentIndex();
    return currentIndex != -1 ? m_pointSizeComboBox->itemData(currentIndex).toInt() : 9;
}

QString FontPanel::styleString() const
{
    const int currentIndex = m_styleComboBox->currentIndex();
    return currentIndex != -1 ? m_styleComboBox->itemText(currentIndex) : QString();
}

void FontPanel::setWritingSystem(QFontDatabase::WritingSystem ws)
{
    m_writingSystemComboBox->setCurrentIndex(m_writingSystemComboBox->findData(QVariant(ws)));
    updateWritingSystem(ws);
}


void FontPanel::slotWritingSystemChanged(int)
{
    updateWritingSystem(writingSystem());
    delayedPreviewFontUpdate();
}

void FontPanel::slotFamilyChanged(const QFont &)
{
    updateFamily(family());
    delayedPreviewFontUpdate();
}

void FontPanel::slotStyleChanged(int)
{
    updatePointSizes(family(), styleString());
    delayedPreviewFontUpdate();
}

void FontPanel::slotPointSizeChanged(int)
{
    delayedPreviewFontUpdate();
}

void FontPanel::updateWritingSystem(QFontDatabase::WritingSystem ws)
{

    m_previewLineEdit->setText(QFontDatabase::writingSystemSample(ws));
    m_familyComboBox->setWritingSystem (ws);
    // Current font not in WS ... set index 0.
    if (m_familyComboBox->currentIndex() < 0) {
        m_familyComboBox->setCurrentIndex(0);
        updateFamily(family());
    }
}

void FontPanel::updateFamily(const QString &family)
{
    // Update styles and trigger update of point sizes.
    // Try to maintain selection or select normal
    const QString &oldStyleString = styleString();

    const QStringList &styles = m_fontDatabase.styles(family);
    const bool hasStyles = !styles.empty();

    m_styleComboBox->setCurrentIndex(-1);
    m_styleComboBox->clear();
    m_styleComboBox->setEnabled(hasStyles);

    int normalIndex = -1;
    const QString normalStyle = QLatin1String("Normal");

    if (hasStyles) {
        for (const QString &style : styles) {
            // try to maintain selection or select 'normal' preferably
            const int newIndex = m_styleComboBox->count();
            m_styleComboBox->addItem(style);
            if (oldStyleString == style) {
                m_styleComboBox->setCurrentIndex(newIndex);
            } else {
                if (oldStyleString ==  normalStyle)
                    normalIndex = newIndex;
            }
        }
        if (m_styleComboBox->currentIndex() == -1 && normalIndex != -1)
            m_styleComboBox->setCurrentIndex(normalIndex);
    }
    updatePointSizes(family, styleString());
}

int FontPanel::closestPointSizeIndex(int desiredPointSize) const
{
    //  try to maintain selection or select closest.
    int closestIndex = -1;
    int closestAbsError = 0xFFFF;

    const int pointSizeCount = m_pointSizeComboBox->count();
    for (int i = 0; i < pointSizeCount; i++) {
        const int itemPointSize = m_pointSizeComboBox->itemData(i).toInt();
        const int absError = qAbs(desiredPointSize - itemPointSize);
        if (absError < closestAbsError) {
            closestIndex  = i;
            closestAbsError = absError;
            if (closestAbsError == 0)
                break;
        } else {    // past optimum
            if (absError > closestAbsError) {
                break;
            }
        }
    }
    return closestIndex;
}


void FontPanel::updatePointSizes(const QString &family, const QString &styleString)
{
    const int oldPointSize = pointSize();

    QList<int> pointSizes =  m_fontDatabase.pointSizes(family, styleString);
    if (pointSizes.empty())
        pointSizes = QFontDatabase::standardSizes();

    const bool hasSizes = !pointSizes.empty();
    m_pointSizeComboBox->clear();
    m_pointSizeComboBox->setEnabled(hasSizes);
    m_pointSizeComboBox->setCurrentIndex(-1);

    //  try to maintain selection or select closest.
    if (hasSizes) {
        QString n;
        for (int pointSize : qAsConst(pointSizes))
            m_pointSizeComboBox->addItem(n.setNum(pointSize), QVariant(pointSize));
        const int closestIndex = closestPointSizeIndex(oldPointSize);
        if (closestIndex != -1)
            m_pointSizeComboBox->setCurrentIndex(closestIndex);
    }
}

void FontPanel::slotUpdatePreviewFont()
{
    m_previewLineEdit->setFont(selectedFont());
}

void FontPanel::delayedPreviewFontUpdate()
{
    if (!m_previewFontUpdateTimer) {
        m_previewFontUpdateTimer = new QTimer(this);
        connect(m_previewFontUpdateTimer, &QTimer::timeout,
                this, &FontPanel::slotUpdatePreviewFont);
        m_previewFontUpdateTimer->setInterval(0);
        m_previewFontUpdateTimer->setSingleShot(true);
    }
    if (m_previewFontUpdateTimer->isActive())
        return;
    m_previewFontUpdateTimer->start();
}

QT_END_NAMESPACE
