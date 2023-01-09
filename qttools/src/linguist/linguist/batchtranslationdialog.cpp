/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Linguist of the Qt Toolkit.
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

#include "batchtranslationdialog.h"
#include "phrase.h"
#include "messagemodel.h"

#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressDialog>

QT_BEGIN_NAMESPACE

CheckableListModel::CheckableListModel(QObject *parent)
  : QStandardItemModel(parent)
{
}

Qt::ItemFlags CheckableListModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

BatchTranslationDialog::BatchTranslationDialog(MultiDataModel *dataModel, QWidget *w)
 : QDialog(w), m_model(this), m_dataModel(dataModel)
{
    m_ui.setupUi(this);
    connect(m_ui.runButton, SIGNAL(clicked()), this, SLOT(startTranslation()));
    connect(m_ui.moveUpButton, SIGNAL(clicked()), this, SLOT(movePhraseBookUp()));
    connect(m_ui.moveDownButton, SIGNAL(clicked()), this, SLOT(movePhraseBookDown()));

    m_ui.phrasebookList->setModel(&m_model);
    m_ui.phrasebookList->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_ui.phrasebookList->setSelectionMode(QAbstractItemView::SingleSelection);
}


void BatchTranslationDialog::setPhraseBooks(const QList<PhraseBook *> &phrasebooks, int modelIndex)
{
    QString fn = QFileInfo(m_dataModel->srcFileName(modelIndex)).baseName();
    setWindowTitle(tr("Batch Translation of '%1' - Qt Linguist").arg(fn));
    m_model.clear();
    m_model.insertColumn(0);
    m_phrasebooks = phrasebooks;
    m_modelIndex = modelIndex;
    int count = phrasebooks.count();
    m_model.insertRows(0, count);
    for (int i = 0; i < count; ++i) {
        QModelIndex idx(m_model.index(i, 0));
        m_model.setData(idx, phrasebooks[i]->friendlyPhraseBookName());
        int sortOrder;
        if (phrasebooks[i]->language() != QLocale::C
            && m_dataModel->language(m_modelIndex) != QLocale::C) {
            if (phrasebooks[i]->language() != m_dataModel->language(m_modelIndex))
                sortOrder = 3;
            else
                sortOrder = (phrasebooks[i]->country()
                             == m_dataModel->model(m_modelIndex)->country()) ? 0 : 1;
        } else {
            sortOrder = 2;
        }
        m_model.setData(idx, sortOrder == 3 ? Qt::Unchecked : Qt::Checked, Qt::CheckStateRole);
        m_model.setData(idx, sortOrder, Qt::UserRole + 1);
        m_model.setData(idx, i, Qt::UserRole);
    }
    m_model.setSortRole(Qt::UserRole + 1);
    m_model.sort(0);
}

void BatchTranslationDialog::startTranslation()
{
    int translatedcount = 0;
    QCursor oldCursor = cursor();
    setCursor(Qt::BusyCursor);
    int messageCount = m_dataModel->messageCount();

    QProgressDialog *dlgProgress;
    dlgProgress = new QProgressDialog(tr("Searching, please wait..."), tr("&Cancel"), 0, messageCount, this);
    dlgProgress->show();

    int msgidx = 0;
    const bool translateTranslated = m_ui.ckTranslateTranslated->isChecked();
    const bool translateFinished = m_ui.ckTranslateFinished->isChecked();
    for (MultiDataModelIterator it(m_dataModel, m_modelIndex); it.isValid(); ++it) {
        if (MessageItem *m = it.current()) {
            if (!m->isObsolete()
                && (translateTranslated || m->translation().isEmpty())
                && (translateFinished || !m->isFinished())) {

                // Go through them in the order the user specified in the phrasebookList
                for (int b = 0; b < m_model.rowCount(); ++b) {
                    QModelIndex idx(m_model.index(b, 0));
                    QVariant checkState = m_model.data(idx, Qt::CheckStateRole);
                    if (checkState == Qt::Checked) {
                        PhraseBook *pb = m_phrasebooks[m_model.data(idx, Qt::UserRole).toInt()];
                        foreach (const Phrase *ph, pb->phrases()) {
                            if (ph->source() == m->text()) {
                                m_dataModel->setTranslation(it, ph->target());
                                m_dataModel->setFinished(it, m_ui.ckMarkFinished->isChecked());
                                ++translatedcount;
                                goto done; // break 2;
                            }
                        }
                    }
                }
            }
        }
      done:
        ++msgidx;
        if (!(msgidx & 15))
            dlgProgress->setValue(msgidx);
        qApp->processEvents();
        if (dlgProgress->wasCanceled())
            break;
    }
    dlgProgress->hide();

    setCursor(oldCursor);
    emit finished();
    QMessageBox::information(this, tr("Linguist batch translator"),
        tr("Batch translated %n entries", "", translatedcount), QMessageBox::Ok);
}

void BatchTranslationDialog::movePhraseBookUp()
{
    QModelIndexList indexes = m_ui.phrasebookList->selectionModel()->selectedIndexes();
    if (indexes.count() <= 0) return;

    QModelIndex sel = indexes[0];
    int row = sel.row();
    if (row > 0) {
        QModelIndex other = m_model.index(row - 1, 0);
        QMap<int, QVariant> seldata = m_model.itemData(sel);
        m_model.setItemData(sel, m_model.itemData(other));
        m_model.setItemData(other, seldata);
        m_ui.phrasebookList->selectionModel()->setCurrentIndex(other, QItemSelectionModel::ClearAndSelect);
    }
}

void BatchTranslationDialog::movePhraseBookDown()
{
    QModelIndexList indexes = m_ui.phrasebookList->selectionModel()->selectedIndexes();
    if (indexes.count() <= 0) return;

    QModelIndex sel = indexes[0];
    int row = sel.row();
    if (row < m_model.rowCount() - 1) {
        QModelIndex other = m_model.index(row + 1, 0);
        QMap<int, QVariant> seldata = m_model.itemData(sel);
        m_model.setItemData(sel, m_model.itemData(other));
        m_model.setItemData(other, seldata);
        m_ui.phrasebookList->selectionModel()->setCurrentIndex(other, QItemSelectionModel::ClearAndSelect);
    }
}

QT_END_NAMESPACE
