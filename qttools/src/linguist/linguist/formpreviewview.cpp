// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "formpreviewview.h"
#include "messagemodel.h"

#include <quiloader.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QFontComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMdiArea>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStackedLayout>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolBox>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QScrollArea>

#include <QtGui/QAction>

#include <QtCore/QDebug>
#include <QtCore/QTime>

QT_BEGIN_NAMESPACE

#if defined(Q_CC_SUN) || defined(Q_CC_HPACC) || defined(Q_CC_XLC)
size_t qHash(const QUiTranslatableStringValue &tsv)
#else
static size_t qHash(const QUiTranslatableStringValue &tsv)
#endif
{
    return qHash(tsv.value()) ^ qHash(tsv.qualifier());
}

static bool operator==(const QUiTranslatableStringValue &tsv1, const QUiTranslatableStringValue &tsv2)
{
    return tsv1.value() == tsv2.value() && tsv1.qualifier() == tsv2.qualifier();
}

#define INSERT_TARGET(_tsv, _type, _target, _prop) \
    do { \
        target.type = _type; \
        target.target._target; \
        target.prop._prop; \
        (*targets)[qvariant_cast<QUiTranslatableStringValue>(_tsv)].append(target); \
    } while (0)

static void registerTreeItem(QTreeWidgetItem *item, TargetsHash *targets)
{
    const QUiItemRolePair *irs = QFormInternal::qUiItemRoles;

    int cnt = item->columnCount();
    for (int i = 0; i < cnt; ++i) {
        for (unsigned j = 0; irs[j].shadowRole >= 0; j++) {
            QVariant v = item->data(i, irs[j].shadowRole);
            if (v.isValid()) {
                TranslatableEntry target;
                target.prop.treeIndex.column = i;
                INSERT_TARGET(v, TranslatableTreeWidgetItem, treeWidgetItem = item, treeIndex.index = j);
            }
        }
    }

    cnt = item->childCount();
    for (int j = 0; j < cnt; ++j)
        registerTreeItem(item->child(j), targets);
}

#define REGISTER_ITEM_CORE(item, propType, targetName) \
    const QUiItemRolePair *irs = QFormInternal::qUiItemRoles; \
    for (unsigned j = 0; irs[j].shadowRole >= 0; j++) { \
        QVariant v = item->data(irs[j].shadowRole); \
        if (v.isValid()) \
            INSERT_TARGET(v, propType, targetName = item, index = j); \
    }

static void registerListItem(QListWidgetItem *item, TargetsHash *targets)
{
    TranslatableEntry target;
    REGISTER_ITEM_CORE(item, TranslatableListWidgetItem, listWidgetItem);
}

static void registerTableItem(QTableWidgetItem *item, TargetsHash *targets)
{
    if (!item)
        return;

    TranslatableEntry target;
    REGISTER_ITEM_CORE(item, TranslatableTableWidgetItem, tableWidgetItem);
}

#define REGISTER_SUBWIDGET_PROP(mainWidget, propType, propName) \
    do { \
        QVariant v = mainWidget->widget(i)->property(propName); \
        if (v.isValid()) \
            INSERT_TARGET(v, propType, object = mainWidget, index = i); \
    } while (0)

static void buildTargets(QObject *o, TargetsHash *targets)
{
    TranslatableEntry target;

    const auto propNames = o->dynamicPropertyNames();
    for (const QByteArray &prop : propNames) {
        if (prop.startsWith(PROP_GENERIC_PREFIX)) {
            const QByteArray propName = prop.mid(sizeof(PROP_GENERIC_PREFIX) - 1);
            INSERT_TARGET(o->property(prop),
                TranslatableProperty, object = o, name = qstrdup(propName.data()));
        }
    }
    if (0) {
#ifndef QT_NO_TABWIDGET
    } else if (QTabWidget *tabw = qobject_cast<QTabWidget*>(o)) {
        const int cnt = tabw->count();
        for (int i = 0; i < cnt; ++i) {
            REGISTER_SUBWIDGET_PROP(tabw, TranslatableTabPageText, PROP_TABPAGETEXT);
# ifndef QT_NO_TOOLTIP
            REGISTER_SUBWIDGET_PROP(tabw, TranslatableTabPageToolTip, PROP_TABPAGETOOLTIP);
# endif
# ifndef QT_NO_WHATSTHIS
            REGISTER_SUBWIDGET_PROP(tabw, TranslatableTabPageWhatsThis, PROP_TABPAGEWHATSTHIS);
# endif
        }
#endif
#ifndef QT_NO_TOOLBOX
    } else if (QToolBox *toolw = qobject_cast<QToolBox*>(o)) {
        const int cnt = toolw->count();
        for (int i = 0; i < cnt; ++i) {
            REGISTER_SUBWIDGET_PROP(toolw, TranslatableToolItemText, PROP_TOOLITEMTEXT);
# ifndef QT_NO_TOOLTIP
            REGISTER_SUBWIDGET_PROP(toolw, TranslatableToolItemToolTip, PROP_TOOLITEMTOOLTIP);
# endif
        }
#endif
#ifndef QT_NO_COMBOBOX
    } else if (QComboBox *combow = qobject_cast<QComboBox*>(o)) {
        if (!qobject_cast<QFontComboBox*>(o)) {
            const int cnt = combow->count();
            for (int i = 0; i < cnt; ++i) {
                const QVariant v = combow->itemData(i, Qt::DisplayPropertyRole);
                if (v.isValid())
                    INSERT_TARGET(v, TranslatableComboBoxItem, comboBox = combow, index = i);
            }
        }
#endif
#ifndef QT_NO_LISTWIDGET
    } else if (QListWidget *listw = qobject_cast<QListWidget*>(o)) {
        const int cnt = listw->count();
        for (int i = 0; i < cnt; ++i)
            registerListItem(listw->item(i), targets);
#endif
#ifndef QT_NO_TABLEWIDGET
    } else if (QTableWidget *tablew = qobject_cast<QTableWidget*>(o)) {
        const int row_cnt = tablew->rowCount();
        const int col_cnt = tablew->columnCount();
        for (int j = 0; j < col_cnt; ++j)
            registerTableItem(tablew->horizontalHeaderItem(j), targets);
        for (int i = 0; i < row_cnt; ++i) {
            registerTableItem(tablew->verticalHeaderItem(i), targets);
            for (int j = 0; j < col_cnt; ++j)
                registerTableItem(tablew->item(i, j), targets);
        }
#endif
#ifndef QT_NO_TREEWIDGET
    } else if (QTreeWidget *treew = qobject_cast<QTreeWidget*>(o)) {
        if (QTreeWidgetItem *item = treew->headerItem())
            registerTreeItem(item, targets);
        const int cnt = treew->topLevelItemCount();
        for (int i = 0; i < cnt; ++i)
            registerTreeItem(treew->topLevelItem(i), targets);
#endif
    }
    for (QObject *co : o->children())
        buildTargets(co, targets);
}

static void destroyTargets(TargetsHash *targets)
{
    for (const auto &targetList : std::as_const(*targets))
        for (const TranslatableEntry &target : targetList)
            if (target.type == TranslatableProperty)
                delete target.prop.name;
    targets->clear();
}

static void retranslateTarget(const TranslatableEntry &target, const QString &text)
{
    switch (target.type) {
    case TranslatableProperty:
        target.target.object->setProperty(target.prop.name, text);
        break;
#ifndef QT_NO_TABWIDGET
    case TranslatableTabPageText:
        target.target.tabWidget->setTabText(target.prop.index, text);
        break;
# ifndef QT_NO_TOOLTIP
    case TranslatableTabPageToolTip:
        target.target.tabWidget->setTabToolTip(target.prop.index, text);
        break;
# endif
# ifndef QT_NO_WHATSTHIS
    case TranslatableTabPageWhatsThis:
        target.target.tabWidget->setTabWhatsThis(target.prop.index, text);
        break;
# endif
#endif // QT_NO_TABWIDGET
#ifndef QT_NO_TOOLBOX
    case TranslatableToolItemText:
        target.target.toolBox->setItemText(target.prop.index, text);
        break;
# ifndef QT_NO_TOOLTIP
    case TranslatableToolItemToolTip:
        target.target.toolBox->setItemToolTip(target.prop.index, text);
        break;
# endif
#endif // QT_NO_TOOLBOX
#ifndef QT_NO_COMBOBOX
    case TranslatableComboBoxItem:
        target.target.comboBox->setItemText(target.prop.index, text);
        break;
#endif
#ifndef QT_NO_LISTWIDGET
    case TranslatableListWidgetItem:
        target.target.listWidgetItem->setData(target.prop.index, text);
        break;
#endif
#ifndef QT_NO_TABLEWIDGET
    case TranslatableTableWidgetItem:
        target.target.tableWidgetItem->setData(target.prop.index, text);
        break;
#endif
#ifndef QT_NO_TREEWIDGET
    case TranslatableTreeWidgetItem:
        target.target.treeWidgetItem->setData(target.prop.treeIndex.column, target.prop.treeIndex.index, text);
        break;
#endif
    }
}

static void retranslateTargets(
    const QList<TranslatableEntry> &targets, const QUiTranslatableStringValue &tsv,
    const DataModel *dataModel, const QString &className)
{
    QString sourceText = QString::fromUtf8(tsv.value());
    QString text;
    if (MessageItem *msg = dataModel->findMessage(
            className, sourceText, QString::fromUtf8(tsv.qualifier())))
        text = msg->translation();
    if (text.isEmpty() && !tsv.value().isEmpty())
        text = QLatin1Char('#') + sourceText;

    for (const TranslatableEntry &target : targets)
        retranslateTarget(target, text);
}

static void bringToFront(QWidget *w)
{
    for (; QWidget *pw = w->parentWidget(); w = pw) {
#ifndef QT_NO_STACKEDWIDGET
        if (QStackedWidget *stack = qobject_cast<QStackedWidget *>(pw)) {
#ifndef QT_NO_TABWIDGET
            // Updating QTabWidget's embedded QStackedWidget does not update its
            // QTabBar, so handle tab widgets explicitly.
            if (QTabWidget *tab = qobject_cast<QTabWidget *>(stack->parent()))
                tab->setCurrentWidget(w);
            else
#endif
                stack->setCurrentWidget(w);
            continue;
        }
#endif
#ifndef QT_NO_TOOLBOX
        if (QScrollArea *sv = qobject_cast<QScrollArea *>(pw)) {
            if (QToolBox *tb = qobject_cast<QToolBox *>(sv->parent()))
                tb->setCurrentWidget(w);
        }
#endif
    }
}

static void highlightTreeWidgetItem(QTreeWidgetItem *item, int col, bool on)
{
    QVariant br = item->data(col, Qt::BackgroundRole + 500);
    QVariant fr = item->data(col, Qt::ForegroundRole + 500);
    if (on) {
        if (!br.isValid() && !fr.isValid()) {
            item->setData(col, Qt::BackgroundRole + 500, item->data(col, Qt::BackgroundRole));
            item->setData(col, Qt::ForegroundRole + 500, item->data(col, Qt::ForegroundRole));
            QPalette pal = qApp->palette();
            item->setData(col, Qt::BackgroundRole, pal.color(QPalette::Dark));
            item->setData(col, Qt::ForegroundRole, pal.color(QPalette::Light));
        }
    } else {
        if (br.isValid() || fr.isValid()) {
            item->setData(col, Qt::BackgroundRole, br);
            item->setData(col, Qt::ForegroundRole, fr);
            item->setData(col, Qt::BackgroundRole + 500, QVariant());
            item->setData(col, Qt::ForegroundRole + 500, QVariant());
        }
    }
}

template <class T>
static void highlightWidgetItem(T *item, bool on)
{
    QVariant br = item->data(Qt::BackgroundRole + 500);
    QVariant fr = item->data(Qt::ForegroundRole + 500);
    if (on) {
        if (!br.isValid() && !fr.isValid()) {
            item->setData(Qt::BackgroundRole + 500, item->data(Qt::BackgroundRole));
            item->setData(Qt::ForegroundRole + 500, item->data(Qt::ForegroundRole));
            QPalette pal = qApp->palette();
            item->setData(Qt::BackgroundRole, pal.color(QPalette::Dark));
            item->setData(Qt::ForegroundRole, pal.color(QPalette::Light));
        }
    } else {
        if (br.isValid() || fr.isValid()) {
            item->setData(Qt::BackgroundRole, br);
            item->setData(Qt::ForegroundRole, fr);
            item->setData(Qt::BackgroundRole + 500, QVariant());
            item->setData(Qt::ForegroundRole + 500, QVariant());
        }
    }
}

#define AUTOFILL_BACKUP_PROP "_q_linguist_autoFillBackup"
#define PALETTE_BACKUP_PROP "_q_linguist_paletteBackup"
#define FONT_BACKUP_PROP "_q_linguist_fontBackup"

static void highlightWidget(QWidget *w, bool on);

static void highlightAction(QAction *a, bool on)
{
    QVariant bak = a->property(FONT_BACKUP_PROP);
    if (on) {
        if (!bak.isValid()) {
            QFont fnt = qApp->font();
            a->setProperty(FONT_BACKUP_PROP, QVariant::fromValue(a->font().resolve(fnt)));
            fnt.setBold(true);
            fnt.setItalic(true);
            a->setFont(fnt);
        }
    } else {
        if (bak.isValid()) {
            a->setFont(qvariant_cast<QFont>(bak));
            a->setProperty(FONT_BACKUP_PROP, QVariant());
        }
    }
    for (QObject *o : a->associatedObjects()) {
        if (QWidget *w = qobject_cast<QWidget *>(o))
            highlightWidget(w, on);
    }
}

static void highlightWidget(QWidget *w, bool on)
{
    QVariant bak = w->property(PALETTE_BACKUP_PROP);
    if (on) {
        if (!bak.isValid()) {
            QPalette pal = qApp->palette();
            for (QObject *co : w->children())
                if (QWidget *cw = qobject_cast<QWidget *>(co))
                    cw->setPalette(cw->palette().resolve(pal));
            w->setProperty(PALETTE_BACKUP_PROP, QVariant::fromValue(w->palette().resolve(pal)));
            w->setProperty(AUTOFILL_BACKUP_PROP, QVariant::fromValue(w->autoFillBackground()));
            QColor col1 = pal.color(QPalette::Dark);
            QColor col2 = pal.color(QPalette::Light);
            pal.setColor(QPalette::Base, col1);
            pal.setColor(QPalette::Window, col1);
            pal.setColor(QPalette::Button, col1);
            pal.setColor(QPalette::Text, col2);
            pal.setColor(QPalette::WindowText, col2);
            pal.setColor(QPalette::ButtonText, col2);
            pal.setColor(QPalette::BrightText, col2);
            w->setPalette(pal);
            w->setAutoFillBackground(true);
        }
    } else {
        if (bak.isValid()) {
            w->setPalette(qvariant_cast<QPalette>(bak));
            w->setAutoFillBackground(qvariant_cast<bool>(w->property(AUTOFILL_BACKUP_PROP)));
            w->setProperty(PALETTE_BACKUP_PROP, QVariant());
            w->setProperty(AUTOFILL_BACKUP_PROP, QVariant());
        }
    }
    if (QMenu *m = qobject_cast<QMenu *>(w))
        if (m->menuAction())
            highlightAction(m->menuAction(), on);
}

static void highlightTarget(const TranslatableEntry &target, bool on)
{
    switch (target.type) {
    case TranslatableProperty:
        if (QAction *a = qobject_cast<QAction *>(target.target.object)) {
            highlightAction(a, on);
        } else if (QWidget *w = qobject_cast<QWidget *>(target.target.object)) {
            bringToFront(w);
            highlightWidget(w, on);
        }
        break;
#ifndef QT_NO_COMBOBOX
    case TranslatableComboBoxItem:
        static_cast<QComboBox *>(target.target.object)->setCurrentIndex(target.prop.index);
        goto frontAndHighlight;
#endif
#ifndef QT_NO_TABWIDGET
    case TranslatableTabPageText:
        static_cast<QTabWidget *>(target.target.object)->setCurrentIndex(target.prop.index);
        goto frontAndHighlight;
# ifndef QT_NO_TOOLTIP
    case TranslatableTabPageToolTip:
# endif
# ifndef QT_NO_WHATSTHIS
    case TranslatableTabPageWhatsThis:
# endif
#endif // QT_NO_TABWIDGET
#ifndef QT_NO_TOOLBOX
    case TranslatableToolItemText:
# ifndef QT_NO_TOOLTIP
    case TranslatableToolItemToolTip:
# endif
#endif // QT_NO_TOOLBOX
#if !defined(QT_NO_COMBOBOX) || !defined(QT_NO_TABWIDGET)
      frontAndHighlight:
#endif
        bringToFront(static_cast<QWidget *>(target.target.object));
        highlightWidget(static_cast<QWidget *>(target.target.object), on);
        break;
#ifndef QT_NO_LISTWIDGET
    case TranslatableListWidgetItem:
        bringToFront(target.target.listWidgetItem->listWidget());
        highlightWidgetItem(target.target.listWidgetItem, on);
        break;
#endif
#ifndef QT_NO_TABLEWIDGET
    case TranslatableTableWidgetItem:
        bringToFront(target.target.tableWidgetItem->tableWidget());
        highlightWidgetItem(target.target.tableWidgetItem, on);
        break;
#endif
#ifndef QT_NO_TREEWIDGET
    case TranslatableTreeWidgetItem:
        bringToFront(target.target.treeWidgetItem->treeWidget());
        highlightTreeWidgetItem(target.target.treeWidgetItem, target.prop.treeIndex.column, on);
        break;
#endif
    }
}

static void highlightTargets(const QList<TranslatableEntry> &targets, bool on)
{
    for (const TranslatableEntry &target : targets)
        highlightTarget(target, on);
}

FormPreviewView::FormPreviewView(QWidget *parent, MultiDataModel *dataModel)
  : QMainWindow(parent), m_form(0), m_dataModel(dataModel)
{
    m_mdiSubWindow = new QMdiSubWindow;
    m_mdiSubWindow->setWindowFlags(m_mdiSubWindow->windowFlags() & ~Qt::WindowSystemMenuHint);
    m_mdiArea = new QMdiArea(this);
    m_mdiArea->addSubWindow(m_mdiSubWindow);
    setCentralWidget(m_mdiArea);
    m_mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void FormPreviewView::setSourceContext(int model, MessageItem *messageItem)
{
    if (model < 0 || !messageItem) {
        m_lastModel = -1;
        return;
    }

    QDir dir = QFileInfo(m_dataModel->srcFileName(model)).dir();
    QString fileName = QDir::cleanPath(dir.absoluteFilePath(messageItem->fileName()));
    if (m_lastFormName != fileName) {
        delete m_form;
        m_form = 0;
        m_lastFormName.clear();
        m_highlights.clear();
        destroyTargets(&m_targets);

        static QUiLoader *uiLoader;
        if (!uiLoader) {
            uiLoader = new QUiLoader(this);
            uiLoader->setLanguageChangeEnabled(true);
            uiLoader->setTranslationEnabled(false);
        }

        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "CANNOT OPEN FORM" << fileName;
            m_mdiSubWindow->hide();
            return;
        }
        m_form = uiLoader->load(&file, m_mdiSubWindow);
        if (!m_form) {
            qDebug() << "CANNOT LOAD FORM" << fileName;
            m_mdiSubWindow->hide();
            return;
        }
        file.close();
        buildTargets(m_form, &m_targets);

        setToolTip(fileName);

        m_form->setWindowFlags(Qt::Widget);
        m_form->setWindowModality(Qt::NonModal);
        m_form->setFocusPolicy(Qt::NoFocus);
        m_form->show(); // needed, otherwide the Qt::NoFocus is not propagated.
        m_mdiSubWindow->setWidget(m_form);
        m_mdiSubWindow->setWindowTitle(m_form->windowTitle());
        m_mdiSubWindow->show();
        m_mdiArea->cascadeSubWindows();
        m_lastFormName = fileName;
        m_lastClassName = messageItem->context();
        m_lastModel = -1;
    } else {
        highlightTargets(m_highlights, false);
    }
    QUiTranslatableStringValue tsv;
    tsv.setValue(messageItem->text().toUtf8());
    tsv.setQualifier(messageItem->comment().toUtf8());
    m_highlights = m_targets.value(tsv);
    if (m_lastModel != model) {
        for (auto it = m_targets.cbegin(), end = m_targets.cend(); it != end; ++it)
            retranslateTargets(*it, it.key(), m_dataModel->model(model), m_lastClassName);
        m_lastModel = model;
    } else {
        retranslateTargets(m_highlights, tsv, m_dataModel->model(model), m_lastClassName);
    }
    highlightTargets(m_highlights, true);
}

QT_END_NAMESPACE
