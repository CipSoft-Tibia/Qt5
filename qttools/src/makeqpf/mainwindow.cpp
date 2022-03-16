/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "mainwindow.h"

#include <QFontComboBox>
#include <QFontDatabase>
#include <QFileDialog>
#include <QMessageBox>
#include <QListWidget>
#include <QDebug>
#include <QShortcut>
#include <QCompleter>
#include <QDirModel>
#include <QTextCodec>

QT_BEGIN_NAMESPACE

MainWindow::MainWindow(const QString &customFont)
{
    setupUi(this);
    pixelSize->setValue(QFontInfo(QFont()).pixelSize());
    populateCharacterRanges();

    {
        weightCombo->addItem(QLatin1String("Light"), QVariant(int(QFont::Light)));
        const int normalIdx = weightCombo->count();
        weightCombo->addItem(QLatin1String("Normal"), QVariant(int(QFont::Normal)));
        weightCombo->addItem(QLatin1String("DemiBold"), QVariant(int(QFont::DemiBold)));
        weightCombo->addItem(QLatin1String("Bold"), QVariant(int(QFont::Bold)));
        weightCombo->addItem(QLatin1String("Black"), QVariant(int(QFont::Black)));

        weightCombo->setCurrentIndex(normalIdx);
    }

    QShortcut *sc = new QShortcut(Qt::ControlModifier + Qt::Key_A, this);
    connect(sc, SIGNAL(activated()),
            this, SLOT(on_selectAll_clicked()));
    sc = new QShortcut(Qt::ControlModifier + Qt::Key_D, this);
    connect(sc, SIGNAL(activated()),
            this, SLOT(on_deselectAll_clicked()));
    sc = new QShortcut(Qt::ControlModifier + Qt::Key_I, this);
    connect(sc, SIGNAL(activated()),
            this, SLOT(on_invertSelection_clicked()));

    QCompleter *completer = new QCompleter(this);
    completer->setModel(new QDirModel(this));
    path->setCompleter(completer);
    path->setText(QDir::currentPath());

    completer = new QCompleter(this);
    completer->setModel(new QDirModel(this));
    sampleFile->setCompleter(completer);
    charCount->setText(QString());

    if (!customFont.isEmpty())
        addCustomFont(customFont);

    fontChanged();

    connect(fontComboBox, SIGNAL(currentFontChanged(QFont)),
            this, SLOT(fontChanged()));
    connect(pixelSize, SIGNAL(valueChanged(int)),
            this, SLOT(fontChanged()));
    connect(italic, SIGNAL(stateChanged(int)),
            this, SLOT(fontChanged()));
    connect(weightCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(fontChanged()));
}

void MainWindow::on_actionAdd_Custom_Font_triggered()
{
    QString fontFile = QFileDialog::getOpenFileName(this, tr("Add Custom Font"));
    if (fontFile.isEmpty())
        return;
    addCustomFont(fontFile);
}

void MainWindow::on_selectAll_clicked()
{
    for (int i = 0; i < characterRangeView->count(); ++i)
        characterRangeView->item(i)->setCheckState(Qt::Checked);
}

void MainWindow::on_deselectAll_clicked()
{
    for (int i = 0; i < characterRangeView->count(); ++i)
        characterRangeView->item(i)->setCheckState(Qt::Unchecked);
}

void MainWindow::on_invertSelection_clicked()
{
    for (int i = 0; i < characterRangeView->count(); ++i) {
        QListWidgetItem *item = characterRangeView->item(i);
        if (item->checkState() == Qt::Checked)
            item->setCheckState(Qt::Unchecked);
        else
            item->setCheckState(Qt::Checked);
    }
}

void MainWindow::fontChanged()
{
    QFont f = preview->font();
    f.setStyleStrategy(QFont::NoFontMerging);
    f.setPixelSize(pixelSize->value());
    f.setFamily(fontComboBox->currentFont().family());
    f.setItalic(italic->isChecked());
    f.setWeight(weightCombo->itemData(weightCombo->currentIndex()).toInt());

    if (!preview->isModified()) {
        QFontDatabase db;
        QFontDatabase::WritingSystem ws = db.writingSystems(f.family()).value(0, QFontDatabase::Any);
        QString sample = db.writingSystemSample(ws);
        preview->setText(sample);
        preview->setModified(false);
    }

    fileName->setText(QPF::fileNameForFont(f));

    preview->setFont(f);
}

void MainWindow::on_browsePath_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Directory"));
    if (!dir.isEmpty())
        path->setText(dir);
}

void MainWindow::on_browseSampleFile_clicked()
{
    QString dir = QFileDialog::getOpenFileName(this, tr("Select Sample File"));
    if (!dir.isEmpty()) {
        sampleFile->setText(dir);
        on_sampleFile_editingFinished();
    }
}

void MainWindow::on_generate_clicked()
{
    QFile f(path->text() + QDir::separator() + fileName->text());
    if (f.exists()) {
        if (QMessageBox::warning(this, QString(),
                                 tr("%1 already exists.\nDo you want to replace it?").arg(f.fileName()),
                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
            != QMessageBox::Yes) {
            statusBar()->showMessage(tr("Pre-rendering aborted..."));
            return;
        }
    }

    QList<QPF::CharacterRange> ranges;

    if (chooseFromSampleFile->isChecked()) {
        ranges = sampleFileRanges;
    } else if (chooseFromCodePoints->isChecked()) {
        ranges.clear();
        for (int i = 0; i < characterRangeView->count(); ++i) {
            QListWidgetItem *item = characterRangeView->item(i);
            if (item->checkState() != Qt::Checked)
                continue;

            QPF::CharacterRange range = qvariant_cast<QPF::CharacterRange>(item->data(Qt::UserRole));
            ranges.append(range);
        }
    }

    const int generationOptions = QPF::IncludeCMap | QPF::RenderGlyphs;
    QByteArray qpf = QPF::generate(preview->font(), generationOptions, ranges);
    f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    f.write(qpf);
    f.close();

    statusBar()->showMessage(tr("Font successfully pre-rendered to %1").arg(fileName->text()));
}

void MainWindow::on_sampleFile_editingFinished()
{
    sampleFileRanges.clear();
    QFile f(sampleFile->text());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        sampleFileRanges.append(QPF::CharacterRange()); // default = all
        return;
    }
    QTextStream stream(&f);
    stream.setCodec(QTextCodec::codecForName("utf-8"));
    stream.setAutoDetectUnicode(true);
    QString text = stream.readAll();

    QSet<QChar> coverage;
    for (int i = 0; i < text.length(); ++i)
        coverage.insert(text.at(i));

    QList<QChar> sortedCoverage = QList<QChar>::fromSet(coverage);
    qSort(sortedCoverage);
    // play simple :)
    for (QChar ch : qAsConst(sortedCoverage)) {
        QPF::CharacterRange r;
        r.start = ch.unicode();
        r.end = r.start + 1;
        sampleFileRanges.append(r);
    }

    charCount->setText(tr("(%1 unique characters found)").arg(sortedCoverage.count()));
}

void MainWindow::populateCharacterRanges()
{
    QFile f(":/Blocks.txt");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QRegExp rangeExpr("([0-9a-f]+)\\.\\.([0-9a-f]+); (.+)");
    rangeExpr.setCaseSensitivity(Qt::CaseInsensitive);

    QString ellipsis(QChar(0x2026));
    if (!characterRangeView->fontMetrics().inFont(ellipsis.at(0)))
        ellipsis = QLatin1String("...");

    while (!f.atEnd()) {
        QString line = QString::fromLatin1(f.readLine());

        if (line.endsWith(QLatin1Char('\n')))
            line.chop(1);
        if (line.endsWith(QLatin1Char('\r')))
            line.chop(1);

        line = line.trimmed();

        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;

        if (!rangeExpr.exactMatch(line) || rangeExpr.captureCount() != 3)
            continue;

        QPF::CharacterRange range;

        bool ok = false;
        range.start = rangeExpr.cap(1).toUInt(&ok, /*base*/16);
        if (!ok)
            continue;
        range.end = rangeExpr.cap(2).toUInt(&ok, /*base*/16);
        if (!ok)
            continue;

        if (range.start >= 0xffff || range.end >= 0xffff)
            continue;

        QString description = rangeExpr.cap(3);

        QListWidgetItem *item = new QListWidgetItem(characterRangeView);
        QString text = description;
        text.append(QLatin1String(" ("));
        text.append(rangeExpr.cap(1));
        text.append(ellipsis);
        text.append(rangeExpr.cap(2));
        text.append(QLatin1String(")"));
        item->setText(text);
        item->setCheckState(Qt::Checked);

        item->setData(Qt::UserRole, QVariant::fromValue(range));
    }
}

void MainWindow::addCustomFont(const QString &fontFile)
{
    int id = QFontDatabase::addApplicationFont(fontFile);
    if (id < 0) {
        QMessageBox::warning(this, tr("Error Adding Custom Font"),
                             tr("The custom font %s could not be loaded.").arg(fontFile));
        return;
    }
    const QStringList families = QFontDatabase::applicationFontFamilies(id);
    if (families.isEmpty()) {
        QMessageBox::warning(this, tr("Error Adding Custom Font"),
                             tr("The custom font %s provides no font families.").arg(fontFile));
        return;
    }
    QFont f(families.first());
    fontComboBox->setCurrentFont(f);
}

QT_END_NAMESPACE
