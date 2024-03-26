// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "messageeditorwidgets.h"
#include "messagehighlighter.h"

#include <translator.h>

#include <QAbstractTextDocumentLayout>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QLayout>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QScrollArea>
#include <QTextBlock>
#include <QTextDocumentFragment>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtGui/private/qtextdocument_p.h>

QT_BEGIN_NAMESPACE

ExpandingTextEdit::ExpandingTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding));

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QAbstractTextDocumentLayout *docLayout = document()->documentLayout();
    connect(docLayout, &QAbstractTextDocumentLayout::documentSizeChanged,
            this, &ExpandingTextEdit::updateHeight);
    connect(this, &QTextEdit::cursorPositionChanged,
            this, &ExpandingTextEdit::reallyEnsureCursorVisible);

    m_minimumHeight = qRound(docLayout->documentSize().height()) + frameWidth() * 2;
}

void ExpandingTextEdit::updateHeight(const QSizeF &documentSize)
{
    m_minimumHeight = qRound(documentSize.height()) + frameWidth() * 2;
    updateGeometry();
}

QSize ExpandingTextEdit::sizeHint() const
{
    return QSize(100, m_minimumHeight);
}

QSize ExpandingTextEdit::minimumSizeHint() const
{
    return QSize(100, m_minimumHeight);
}

void ExpandingTextEdit::reallyEnsureCursorVisible()
{
    QObject *ancestor = parent();
    while (ancestor) {
        QScrollArea *scrollArea = qobject_cast<QScrollArea*>(ancestor);
        if (scrollArea &&
                (scrollArea->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff &&
                 scrollArea->horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOff)) {
            const QRect &r = cursorRect();
            const QPoint &c = mapTo(scrollArea->widget(), r.center());
            scrollArea->ensureVisible(c.x(), c.y());
            break;
        }
        ancestor = ancestor->parent();
    }
}

FormatTextEdit::FormatTextEdit(QWidget *parent)
    : ExpandingTextEdit(parent)
{
    setLineWrapMode(QTextEdit::WidgetWidth);
    setAcceptRichText(false);

    // Do not set different background if disabled
    QPalette p = palette();
    p.setColor(QPalette::Disabled, QPalette::Base, p.color(QPalette::Active, QPalette::Base));
    setPalette(p);

    setEditable(true);

    m_highlighter = new MessageHighlighter(this);
}

FormatTextEdit::~FormatTextEdit()
{
    emit editorDestroyed();
}

void FormatTextEdit::setEditable(bool editable)
{
    // save default frame style
    static int framed = frameStyle();
    static Qt::FocusPolicy defaultFocus = focusPolicy();

    if (editable) {
        setFrameStyle(framed);
        setFocusPolicy(defaultFocus);
    } else {
        setFrameStyle(QFrame::NoFrame | QFrame::Plain);
        setFocusPolicy(Qt::NoFocus);
    }

    setReadOnly(!editable);
}

void FormatTextEdit::setPlainText(const QString &text, bool userAction)
{
    if (!userAction) {
        // Prevent contentsChanged signal
        bool oldBlockState = blockSignals(true);
        document()->setUndoRedoEnabled(false);
        ExpandingTextEdit::setPlainText(text);
        // highlighter is out of sync because of blocked signals
        m_highlighter->rehighlight();
        document()->setUndoRedoEnabled(true);
        blockSignals(oldBlockState);
    } else {
        ExpandingTextEdit::setPlainText(text);
    }
}

void FormatTextEdit::setVisualizeWhitespace(bool value)
{
    QTextOption option = document()->defaultTextOption();
    if (value) {
        option.setFlags(option.flags()
                        | QTextOption::ShowLineAndParagraphSeparators
                        | QTextOption::ShowTabsAndSpaces);
    } else {
        option.setFlags(option.flags()
                        & ~QTextOption::ShowLineAndParagraphSeparators
                        & ~QTextOption::ShowTabsAndSpaces);
    }
    document()->setDefaultTextOption(option);
}

FormWidget::FormWidget(const QString &label, bool isEditable, QWidget *parent)
        : QWidget(parent),
          m_hideWhenEmpty(false)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(QMargins());

    m_label = new QLabel(this);
    QFont fnt;
    fnt.setBold(true);
    m_label->setFont(fnt);
    m_label->setText(label);
    layout->addWidget(m_label);

    m_editor = new FormatTextEdit(this);
    m_editor->setEditable(isEditable);
    //m_textEdit->setWhatsThis(tr("This area shows text from an auxillary translation."));
    layout->addWidget(m_editor);

    setLayout(layout);

    connect(m_editor, &QTextEdit::textChanged,
            this, &FormWidget::slotTextChanged);
    connect(m_editor, &QTextEdit::selectionChanged,
            this, &FormWidget::slotSelectionChanged);
    connect(m_editor, &QTextEdit::cursorPositionChanged,
            this, &FormWidget::cursorPositionChanged);
}

void FormWidget::slotTextChanged()
{
    emit textChanged(m_editor);
}

void FormWidget::slotSelectionChanged()
{
    emit selectionChanged(m_editor);
}

void FormWidget::setTranslation(const QString &text, bool userAction)
{
    m_editor->setPlainText(text, userAction);
    if (m_hideWhenEmpty)
        setHidden(text.isEmpty());
}

void FormWidget::setEditingEnabled(bool enable)
{
    // Use read-only state so that the text can still be copied
    m_editor->setReadOnly(!enable);
    m_label->setEnabled(enable);
}


class ButtonWrapper : public QWidget
{
    // no Q_OBJECT: no need to, and don't want the useless moc file

public:
    ButtonWrapper(QWidget *wrapee, QWidget *relator)
    {
        QBoxLayout *box = new QVBoxLayout;
        box->setContentsMargins(QMargins());
        setLayout(box);
        box->addWidget(wrapee, 0, Qt::AlignBottom);
        if (relator)
            relator->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (event->type() == QEvent::Resize) {
            QWidget *relator = static_cast<QWidget *>(object);
            setFixedHeight(relator->height());
        }
        return false;
    }
};

FormMultiWidget::FormMultiWidget(const QString &label, QWidget *parent)
        : QWidget(parent),
          m_hideWhenEmpty(false),
          m_multiEnabled(false),
          m_plusIcon(QIcon(QLatin1String(":/images/plus.png"))),  // make static
          m_minusIcon(QIcon(QLatin1String(":/images/minus.png")))
{
    m_label = new QLabel(this);
    QFont fnt;
    fnt.setBold(true);
    m_label->setFont(fnt);
    m_label->setText(label);

    m_plusButtons.append(
            new ButtonWrapper(makeButton(m_plusIcon, &FormMultiWidget::plusButtonClicked), 0));
}

QAbstractButton *FormMultiWidget::makeButton(const QIcon &icon)
{
    QAbstractButton *btn = new QToolButton(this);
    btn->setIcon(icon);
    btn->setFixedSize(icon.availableSizes().first() /* + something */);
    btn->setFocusPolicy(Qt::NoFocus);
    return btn;
}

void FormMultiWidget::addEditor(int idx)
{
    FormatTextEdit *editor = new FormatTextEdit(this);
    m_editors.insert(idx, editor);

    m_minusButtons.insert(idx, makeButton(m_minusIcon, &FormMultiWidget::minusButtonClicked));
    m_plusButtons.insert(idx + 1,
            new ButtonWrapper(makeButton(m_plusIcon, &FormMultiWidget::plusButtonClicked), editor));

    connect(editor, &QTextEdit::textChanged,
            this, &FormMultiWidget::slotTextChanged);
    connect(editor, &QTextEdit::selectionChanged,
            this, &FormMultiWidget::slotSelectionChanged);
    connect(editor, &QTextEdit::cursorPositionChanged,
            this, &FormMultiWidget::cursorPositionChanged);
    editor->installEventFilter(this);

    emit editorCreated(editor);
}

bool FormMultiWidget::eventFilter(QObject *watched, QEvent *event)
{
    int i = 0;
    while (m_editors.at(i) != watched)
        if (++i >= m_editors.size()) // Happens when deleting an editor
            return false;
    if (event->type() == QEvent::FocusOut) {
        m_minusButtons.at(i)->setToolTip(QString());
        m_plusButtons.at(i)->setToolTip(QString());
        m_plusButtons.at(i + 1)->setToolTip(QString());
    } else if (event->type() == QEvent::FocusIn) {
        m_minusButtons.at(i)->setToolTip(/*: translate, but don't change */ tr("Alt+Delete"));
        m_plusButtons.at(i)->setToolTip(/*: translate, but don't change */ tr("Shift+Alt+Insert"));
        m_plusButtons.at(i + 1)->setToolTip(/*: translate, but don't change */ tr("Alt+Insert"));
    } else if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->modifiers() & Qt::AltModifier) {
            if (ke->key() == Qt::Key_Delete) {
                deleteEditor(i);
                return true;
            } else if (ke->key() == Qt::Key_Insert) {
                if (!(ke->modifiers() & Qt::ShiftModifier))
                    ++i;
                insertEditor(i);
                return true;
            }
        }
    }
    return false;
}

void FormMultiWidget::updateLayout()
{
    delete layout();

    QGridLayout *layout = new QGridLayout;
    layout->setContentsMargins(QMargins());
    setLayout(layout);

    bool variants = m_multiEnabled && m_label->isEnabled();

    layout->addWidget(m_label, 0, 0, 1, variants ? 2 : 1);

    if (variants) {
        QVBoxLayout *layoutForPlusButtons = new QVBoxLayout;
        layoutForPlusButtons->setContentsMargins(QMargins());
        for (int i = 0; i < m_plusButtons.size(); ++i)
            layoutForPlusButtons->addWidget(m_plusButtons.at(i), Qt::AlignTop);
        layout->addLayout(layoutForPlusButtons, 1, 0, Qt::AlignTop);

        const int minimumRowHeight = m_plusButtons.at(0)->sizeHint().height() / 2.0;
        QGridLayout *layoutForLabels = new QGridLayout;
        layoutForLabels->setContentsMargins(QMargins());
        layoutForLabels->setRowMinimumHeight(0, minimumRowHeight);
        for (int j = 0; j < m_editors.size(); ++j) {
            layoutForLabels->addWidget(m_editors.at(j), 1 + j, 0, Qt::AlignVCenter);
            layoutForLabels->addWidget(m_minusButtons.at(j), 1 + j, 1, Qt::AlignVCenter);
        }
        layoutForLabels->setRowMinimumHeight(m_editors.size() + 1, minimumRowHeight);
        layout->addLayout(layoutForLabels, 1, 1, Qt::AlignTop);
    } else {
        for (int k = 0; k < m_editors.size(); ++k)
            layout->addWidget(m_editors.at(k), 1 + k, 0, Qt::AlignVCenter);
    }

    for (int i = 0; i < m_plusButtons.size(); ++i)
        m_plusButtons.at(i)->setVisible(variants);
    for (int j = 0; j < m_minusButtons.size(); ++j)
        m_minusButtons.at(j)->setVisible(variants);

    updateGeometry();
}

void FormMultiWidget::slotTextChanged()
{
    emit textChanged(static_cast<QTextEdit *>(sender()));
}

void FormMultiWidget::slotSelectionChanged()
{
    emit selectionChanged(static_cast<QTextEdit *>(sender()));
}

void FormMultiWidget::setTranslation(const QString &text, bool userAction)
{
    QStringList texts = text.split(QChar(Translator::BinaryVariantSeparator), Qt::KeepEmptyParts);

    while (m_editors.size() > texts.size()) {
        delete m_minusButtons.takeLast();
        delete m_plusButtons.takeLast();
        delete m_editors.takeLast();
    }
    while (m_editors.size() < texts.size())
        addEditor(m_editors.size());
    updateLayout();

    for (int i = 0; i < texts.size(); ++i)
        // XXX this will emit n textChanged signals
        m_editors.at(i)->setPlainText(texts.at(i), userAction);

    if (m_hideWhenEmpty)
        setHidden(text.isEmpty());
}

// Copied from QTextDocument::toPlainText() and modified to
// not replace QChar::Nbsp with QLatin1Char(' ')
QString toPlainText(const QString &text)
{
    QString txt = text;
    QChar *uc = txt.data();
    QChar *e = uc + txt.size();

    for (; uc != e; ++uc) {
        switch (uc->unicode()) {
        case 0xfdd0: // QTextBeginningOfFrame
        case 0xfdd1: // QTextEndOfFrame
        case QChar::ParagraphSeparator:
        case QChar::LineSeparator:
            *uc = QLatin1Char('\n');
            break;
        }
    }
    return txt;
}

QString FormMultiWidget::getTranslation() const
{
    QString ret;
    for (int i = 0; i < m_editors.size(); ++i) {
        if (i)
            ret += QChar(Translator::BinaryVariantSeparator);
        ret += toPlainText(m_editors.at(i)->document()->toRawText());
    }
    return ret;
}

void FormMultiWidget::setEditingEnabled(bool enable)
{
    // Use read-only state so that the text can still be copied
    for (int i = 0; i < m_editors.size(); ++i)
        m_editors.at(i)->setReadOnly(!enable);
    m_label->setEnabled(enable);
    if (m_multiEnabled)
        updateLayout();
}

void FormMultiWidget::setMultiEnabled(bool enable)
{
    m_multiEnabled = enable;
    if (m_label->isEnabled())
        updateLayout();
}

void FormMultiWidget::minusButtonClicked()
{
    int i = 0;
    while (m_minusButtons.at(i) != sender())
        ++i;
    deleteEditor(i);
}

void FormMultiWidget::plusButtonClicked()
{
    QWidget *btn = static_cast<QAbstractButton *>(sender())->parentWidget();
    int i = 0;
    while (m_plusButtons.at(i) != btn)
        ++i;
    insertEditor(i);
}

void FormMultiWidget::deleteEditor(int idx)
{
    if (m_editors.size() == 1) {
        // Don't just clear(), so the undo history is not lost
        QTextCursor c = m_editors.first()->textCursor();
        c.select(QTextCursor::Document);
        c.removeSelectedText();
    } else {
        if (!m_editors.at(idx)->toPlainText().isEmpty()) {
            if (QMessageBox::question(topLevelWidget(), tr("Confirmation - Qt Linguist"),
                                      tr("Delete non-empty length variant?"),
                                      QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)
                != QMessageBox::Yes)
                return;
        }
        delete m_editors.takeAt(idx);
        delete m_minusButtons.takeAt(idx);
        delete m_plusButtons.takeAt(idx + 1);
        updateLayout();
        emit textChanged(m_editors.at((m_editors.size() == idx) ? idx - 1 : idx));
    }
}

void FormMultiWidget::insertEditor(int idx)
{
    addEditor(idx);
    updateLayout();
    emit textChanged(m_editors.at(idx));
}

QT_END_NAMESPACE
