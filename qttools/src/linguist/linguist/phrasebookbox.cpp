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

/*  TRANSLATOR PhraseBookBox

  Go to Phrase > Edit Phrase Book...  The dialog that pops up is a
  PhraseBookBox.
*/

#include "phrasebookbox.h"
#include "translationsettingsdialog.h"

#include <QtEvents>
#include <QLineEdit>
#include <QMessageBox>
#include <QHeaderView>
#include <QSortFilterProxyModel>

QT_BEGIN_NAMESPACE

PhraseBookBox::PhraseBookBox(PhraseBook *phraseBook, QWidget *parent)
    : QDialog(parent),
      m_phraseBook(phraseBook),
      m_translationSettingsDialog(0)
{

// This definition needs to be within class context for lupdate to find it
#define NewPhrase tr("(New Entry)")

    setupUi(this);
    setWindowTitle(tr("%1[*] - Qt Linguist").arg(m_phraseBook->friendlyPhraseBookName()));
    setWindowModified(m_phraseBook->isModified());

    phrMdl = new PhraseModel(this);

    m_sortedPhraseModel = new QSortFilterProxyModel(this);
    m_sortedPhraseModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_sortedPhraseModel->setSortLocaleAware(true);
    m_sortedPhraseModel->setDynamicSortFilter(true);
    m_sortedPhraseModel->setSourceModel(phrMdl);

    phraseList->setModel(m_sortedPhraseModel);
    phraseList->header()->setDefaultSectionSize(150);
    phraseList->header()->setSectionResizeMode(QHeaderView::Interactive);

    connect(sourceLed, SIGNAL(textChanged(QString)),
            this, SLOT(sourceChanged(QString)));
    connect(targetLed, SIGNAL(textChanged(QString)),
            this, SLOT(targetChanged(QString)));
    connect(definitionLed, SIGNAL(textChanged(QString)),
            this, SLOT(definitionChanged(QString)));
    connect(phraseList->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(selectionChanged()));
    connect(newBut, SIGNAL(clicked()), this, SLOT(newPhrase()));
    connect(removeBut, SIGNAL(clicked()), this, SLOT(removePhrase()));
    connect(settingsBut, SIGNAL(clicked()), this, SLOT(settings()));
    connect(saveBut, SIGNAL(clicked()), this, SLOT(save()));
    connect(m_phraseBook, SIGNAL(modifiedChanged(bool)), this, SLOT(setWindowModified(bool)));

    sourceLed->installEventFilter(this);
    targetLed->installEventFilter(this);
    definitionLed->installEventFilter(this);

    foreach (Phrase *p, phraseBook->phrases())
        phrMdl->addPhrase(p);

    phraseList->sortByColumn(0, Qt::AscendingOrder);

    enableDisable();
}

bool PhraseBookBox::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress &&
            (obj == sourceLed || obj == targetLed || obj == definitionLed))
    {
        const QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        const int key = keyEvent->key();

        switch (key) {
        case Qt::Key_Down:
        case Qt::Key_Up:
        case Qt::Key_PageDown:
        case Qt::Key_PageUp:
            return QApplication::sendEvent(phraseList, event);
        }
    }
    return QDialog::eventFilter(obj, event);
}

void PhraseBookBox::newPhrase()
{
    Phrase *p = new Phrase();
    p->setSource(NewPhrase);
    m_phraseBook->append(p);
    selectItem(phrMdl->addPhrase(p));
}

void PhraseBookBox::removePhrase()
{
    QModelIndex index = currentPhraseIndex();
    Phrase *phrase = phrMdl->phrase(index);
    m_phraseBook->remove(phrase);
    phrMdl->removePhrase(index);
    delete phrase;
}

void PhraseBookBox::settings()
{
    if (!m_translationSettingsDialog)
        m_translationSettingsDialog = new TranslationSettingsDialog(this);
    m_translationSettingsDialog->setPhraseBook(m_phraseBook);
    m_translationSettingsDialog->exec();
}

void PhraseBookBox::save()
{
    const QString &fileName = m_phraseBook->fileName();
    if (!m_phraseBook->save(fileName))
        QMessageBox::warning(this,
                             tr("Qt Linguist"),
                             tr("Cannot save phrase book '%1'.").arg(fileName));
}

void PhraseBookBox::sourceChanged(const QString& source)
{
    QModelIndex index = currentPhraseIndex();
    if (index.isValid())
        phrMdl->setData(phrMdl->index(index.row(), 0), source);
}

void PhraseBookBox::targetChanged(const QString& target)
{
    QModelIndex index = currentPhraseIndex();
    if (index.isValid())
        phrMdl->setData(phrMdl->index(index.row(), 1), target);
}

void PhraseBookBox::definitionChanged(const QString& definition)
{
    QModelIndex index = currentPhraseIndex();
    if (index.isValid())
        phrMdl->setData(phrMdl->index(index.row(), 2), definition);
}

void PhraseBookBox::selectionChanged()
{
    enableDisable();
}

void PhraseBookBox::selectItem(const QModelIndex &index)
{
    const QModelIndex &sortedIndex = m_sortedPhraseModel->mapFromSource(index);
    phraseList->scrollTo(sortedIndex);
    phraseList->setCurrentIndex(sortedIndex);
}

void PhraseBookBox::enableDisable()
{
    QModelIndex index = currentPhraseIndex();

    sourceLed->blockSignals(true);
    targetLed->blockSignals(true);
    definitionLed->blockSignals(true);

    bool indexValid = index.isValid();

    if (indexValid) {
        Phrase *p = phrMdl->phrase(index);
        sourceLed->setText(p->source().simplified());
        targetLed->setText(p->target().simplified());
        definitionLed->setText(p->definition());
    }
    else {
        sourceLed->setText(QString());
        targetLed->setText(QString());
        definitionLed->setText(QString());
    }

    sourceLed->setEnabled(indexValid);
    targetLed->setEnabled(indexValid);
    definitionLed->setEnabled(indexValid);
    removeBut->setEnabled(indexValid);

    sourceLed->blockSignals(false);
    targetLed->blockSignals(false);
    definitionLed->blockSignals(false);

    QWidget *f = QApplication::focusWidget();
    if (f != sourceLed && f != targetLed && f != definitionLed) {
        QLineEdit *led = (sourceLed->text() == NewPhrase ? sourceLed : targetLed);
        led->setFocus();
        led->selectAll();
    } else {
        static_cast<QLineEdit*>(f)->selectAll();
    }
}

QModelIndex PhraseBookBox::currentPhraseIndex() const
{
    return m_sortedPhraseModel->mapToSource(phraseList->currentIndex());
}

QT_END_NAMESPACE
