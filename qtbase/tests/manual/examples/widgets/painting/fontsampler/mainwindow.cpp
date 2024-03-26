// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>
#if defined(QT_PRINTSUPPORT_LIB)
#include <QtPrintSupport/qtprintsupportglobal.h>
#if QT_CONFIG(printdialog)
#include <QPrinter>
#include <QPrintDialog>
#if QT_CONFIG(printpreviewdialog)
#include <QPrintPreviewDialog>
#endif
#endif
#endif

#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
   : QMainWindow(parent)
{
    setupUi(this);

    sampleSizes << 32 << 24 << 16 << 14 << 12 << 8 << 4 << 2 << 1;
    markedCount = 0;
    setupFontTree();

    connect(quitAction, &QAction::triggered,
            qApp, &QApplication::quit);
    connect(fontTree, &QTreeWidget::currentItemChanged,
            this, &MainWindow::showFont);
    connect(fontTree, &QTreeWidget::itemChanged,
            this, &MainWindow::updateStyles);

    fontTree->topLevelItem(0)->setSelected(true);
    showFont(fontTree->topLevelItem(0));
}

void MainWindow::setupFontTree()
{
    fontTree->setColumnCount(1);
    fontTree->setHeaderLabels({ tr("Font") });

    const QStringList fontFamilies = QFontDatabase::families();
    for (const QString &family : fontFamilies) {
        const QStringList styles = QFontDatabase::styles(family);
        if (styles.isEmpty())
            continue;

        QTreeWidgetItem *familyItem = new QTreeWidgetItem(fontTree);
        familyItem->setText(0, family);
        familyItem->setCheckState(0, Qt::Unchecked);
        familyItem->setFlags(familyItem->flags() | Qt::ItemIsAutoTristate);

        for (const QString &style : styles) {
            QTreeWidgetItem *styleItem = new QTreeWidgetItem(familyItem);
            styleItem->setText(0, style);
            styleItem->setCheckState(0, Qt::Unchecked);
            styleItem->setData(0, Qt::UserRole, QVariant(QFontDatabase::weight(family, style)));
            styleItem->setData(0, Qt::UserRole + 1, QVariant(QFontDatabase::italic(family, style)));
        }
    }
}

void MainWindow::on_clearAction_triggered()
{
    const QList<QTreeWidgetItem *> items = fontTree->selectedItems();
    for (QTreeWidgetItem *item : items)
        item->setSelected(false);
    fontTree->currentItem()->setSelected(true);
}

void MainWindow::on_markAction_triggered()
{
    markUnmarkFonts(Qt::Checked);
}

void MainWindow::on_unmarkAction_triggered()
{
    markUnmarkFonts(Qt::Unchecked);
}

void MainWindow::markUnmarkFonts(Qt::CheckState state)
{
    const QList<QTreeWidgetItem *> items = fontTree->selectedItems();
    for (QTreeWidgetItem *item : items) {
        if (item->checkState(0) != state)
            item->setCheckState(0, state);
    }
}

void MainWindow::showFont(QTreeWidgetItem *item)
{
    if (!item)
        return;

    QString family;
    QString style;
    int weight;
    bool italic;

    if (item->parent()) {
        family = item->parent()->text(0);
        style = item->text(0);
        weight = item->data(0, Qt::UserRole).toInt();
        italic = item->data(0, Qt::UserRole + 1).toBool();
    } else {
        family = item->text(0);
        style = item->child(0)->text(0);
        weight = item->child(0)->data(0, Qt::UserRole).toInt();
        italic = item->child(0)->data(0, Qt::UserRole + 1).toBool();
    }

    QString oldText = textEdit->toPlainText().trimmed();
    bool modified = textEdit->document()->isModified();
    textEdit->clear();
    QFont font(family, 32, weight, italic);
    font.setStyleName(style);
    textEdit->document()->setDefaultFont(font);

    QTextCursor cursor = textEdit->textCursor();
    QTextBlockFormat blockFormat;
    blockFormat.setAlignment(Qt::AlignCenter);
    cursor.insertBlock(blockFormat);

    if (modified)
        cursor.insertText(QString(oldText));
    else
        cursor.insertText(QString("%1 %2").arg(family).arg(style));

    textEdit->document()->setModified(modified);
}

void MainWindow::updateStyles(QTreeWidgetItem *item, int column)
{
    if (!item || column != 0)
        return;

    Qt::CheckState state = item->checkState(0);
    QTreeWidgetItem *parent = item->parent();

    if (parent) {
        // Only count style items.
        if (state == Qt::Checked)
            ++markedCount;
        else
            --markedCount;
    }

    printAction->setEnabled(markedCount > 0);
    printPreviewAction->setEnabled(markedCount > 0);
}

QMap<QString, StyleItems> MainWindow::currentPageMap()
{
    QMap<QString, StyleItems> pageMap;

    for (int row = 0; row < fontTree->topLevelItemCount(); ++row) {
        QTreeWidgetItem *familyItem = fontTree->topLevelItem(row);
        QString family;

        if (familyItem->checkState(0) == Qt::Checked) {
            family = familyItem->text(0);
            pageMap[family] = StyleItems();
        }

        for (int childRow = 0; childRow < familyItem->childCount(); ++childRow) {
            QTreeWidgetItem *styleItem = familyItem->child(childRow);
            if (styleItem->checkState(0) == Qt::Checked)
                pageMap[family].append(styleItem);
        }
    }

    return pageMap;
}

void MainWindow::on_printAction_triggered()
{
#if defined(QT_PRINTSUPPORT_LIB) && QT_CONFIG(printdialog)
    pageMap = currentPageMap();

    if (pageMap.count() == 0)
        return;

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    int from = printer.fromPage();
    int to = printer.toPage();
    if (from <= 0 && to <= 0)
        printer.setFromTo(1, pageMap.keys().count());

    printDocument(&printer);
#endif
}

void MainWindow::printDocument(QPrinter *printer)
{
#if defined(QT_PRINTSUPPORT_LIB) && QT_CONFIG(printdialog)
    printer->setFromTo(1, pageMap.count());

    QProgressDialog progress(tr("Preparing font samples..."), tr("&Cancel"),
                             0, pageMap.count(), this);
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setWindowTitle(tr("Font Sampler"));
    progress.setMinimum(printer->fromPage() - 1);
    progress.setMaximum(printer->toPage());

    QPainter painter;
    painter.begin(printer);
    bool firstPage = true;

    for (int page = printer->fromPage(); page <= printer->toPage(); ++page) {

        if (!firstPage)
            printer->newPage();

        qApp->processEvents();
        if (progress.wasCanceled())
            break;

        printPage(page - 1, &painter, printer);
        progress.setValue(page);
        firstPage = false;
    }

    painter.end();
#endif
}

void MainWindow::on_printPreviewAction_triggered()
{
#if defined(QT_PRINTSUPPORT_LIB) && QT_CONFIG(printpreviewdialog)
    pageMap = currentPageMap();

    if (pageMap.count() == 0)
        return;

    QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, &QPrintPreviewDialog::paintRequested,
            this, &MainWindow::printDocument);
    preview.exec();
#endif
}

void MainWindow::printPage(int index, QPainter *painter, QPrinter *printer)
{
#if defined(QT_PRINTSUPPORT_LIB) && QT_CONFIG(printdialog)
    const QString family = std::next(pageMap.begin(), index).key();
    const StyleItems items = pageMap.value(family);

    // Find the dimensions of the text on each page.
    qreal width = 0.0;
    qreal height = 0.0;
    for (const QTreeWidgetItem *item : items) {
        QString style = item->text(0);
        int weight = item->data(0, Qt::UserRole).toInt();
        bool italic = item->data(0, Qt::UserRole + 1).toBool();

        // Calculate the maximum width and total height of the text.
        for (int size : std::as_const(sampleSizes)) {
            QFont font(family, size, weight, italic);
            font.setStyleName(style);
            font = QFont(font, painter->device());
            QFontMetricsF fontMetrics(font);
            QRectF rect = fontMetrics.boundingRect(
            QString("%1 %2").arg(family).arg(style));
            width = qMax(rect.width(), width);
            height += rect.height();
        }
    }

    qreal xScale = printer->pageRect(QPrinter::DevicePixel).width() / width;
    qreal yScale = printer->pageRect(QPrinter::DevicePixel).height() / height;
    qreal scale = qMin(xScale, yScale);

    qreal remainingHeight = printer->pageRect(QPrinter::DevicePixel).height()/scale - height;
    qreal spaceHeight = (remainingHeight / 4.0) / (items.count() + 1);
    qreal interLineHeight = (remainingHeight / 4.0) / (sampleSizes.count() * items.count());

    painter->save();
    painter->translate(printer->pageRect(QPrinter::DevicePixel).width() / 2.0, printer->pageRect(QPrinter::DevicePixel).height() / 2.0);
    painter->scale(scale, scale);
    painter->setBrush(QBrush(Qt::black));

    qreal x = -width / 2.0;
    qreal y = -height / 2.0 - remainingHeight / 4.0 + spaceHeight;

    for (const QTreeWidgetItem *item : items) {
        QString style = item->text(0);
        int weight = item->data(0, Qt::UserRole).toInt();
        bool italic = item->data(0, Qt::UserRole + 1).toBool();

        // Draw each line of text.
        for (int size : std::as_const(sampleSizes)) {
            QFont font(family, size, weight, italic);
            font.setStyleName(style);
            font = QFont(font, painter->device());
            QFontMetricsF fontMetrics(font);
            QRectF rect = fontMetrics.boundingRect(QString("%1 %2").arg(
                          font.family()).arg(style));
            y += rect.height();
            painter->setFont(font);
            painter->drawText(QPointF(x, y), QString("%1 %2").arg(family).arg(style));
            y += interLineHeight;
        }
        y += spaceHeight;
    }

    painter->restore();
#endif
}
