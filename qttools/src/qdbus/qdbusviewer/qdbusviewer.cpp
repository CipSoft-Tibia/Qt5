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

#include "qdbusviewer.h"
#include "qdbusmodel.h"
#include "servicesproxymodel.h"
#include "propertydialog.h"
#include "logviewer.h"


#include <QtCore/QStringListModel>
#include <QtCore/QMetaProperty>
#include <QtCore/QSettings>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QAction>
#include <QtWidgets/QShortcut>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QMenu>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QHeaderView>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMetaType>

#include <private/qdbusutil_p.h>

class QDBusViewModel: public QDBusModel
{
public:
    inline QDBusViewModel(const QString &service, const QDBusConnection &connection)
        : QDBusModel(service, connection)
    {}

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
    {
        if (role == Qt::FontRole && itemType(index) == InterfaceItem) {
            QFont f;
            f.setItalic(true);
            return f;
        }
        return QDBusModel::data(index, role);
    }
};

class ServicesModel : public QStringListModel
{
public:
    explicit ServicesModel(QObject *parent = nullptr)
        : QStringListModel(parent)
    {}

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        return QStringListModel::flags(index) & ~Qt::ItemIsEditable;
    }
};

QDBusViewer::QDBusViewer(const QDBusConnection &connection, QWidget *parent)  :
    QWidget(parent),
    c(connection),
    objectPathRegExp(QLatin1String("\\[ObjectPath: (.*)\\]"))
{
    serviceFilterLine = new QLineEdit(this);
    serviceFilterLine->setPlaceholderText(tr("Search..."));

    // Create model for services list
    servicesModel = new ServicesModel(this);
    // Wrap service list model in proxy for easy filtering and interactive sorting
    servicesProxyModel = new ServicesProxyModel(this);
    servicesProxyModel->setSourceModel(servicesModel);
    servicesProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    servicesView = new QTableView(this);
    servicesView->installEventFilter(this);
    servicesView->setModel(servicesProxyModel);
    // Make services grid view behave like a list view with headers
    servicesView->verticalHeader()->hide();
    servicesView->horizontalHeader()->setStretchLastSection(true);
    servicesView->setShowGrid(false);
    // Sort service list by default
    servicesView->setSortingEnabled(true);
    servicesView->sortByColumn(0, Qt::AscendingOrder);

    connect(serviceFilterLine, &QLineEdit::textChanged, servicesProxyModel, &QSortFilterProxyModel::setFilterFixedString);
    connect(serviceFilterLine, &QLineEdit::returnPressed, this, &QDBusViewer::serviceFilterReturnPressed);

    tree = new QTreeView;
    tree->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(tree, &QAbstractItemView::activated, this, &QDBusViewer::activate);

    refreshAction = new QAction(tr("&Refresh"), tree);
    refreshAction->setData(42); // increase the amount of 42 used as magic number by one
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, this, &QDBusViewer::refreshChildren);

    QShortcut *refreshShortcut = new QShortcut(QKeySequence::Refresh, tree);
    connect(refreshShortcut, &QShortcut::activated, this, &QDBusViewer::refreshChildren);

    QVBoxLayout *layout = new QVBoxLayout(this);
    topSplitter = new QSplitter(Qt::Vertical, this);
    layout->addWidget(topSplitter);

    log = new LogViewer;
    connect(log, &QTextBrowser::anchorClicked, this, &QDBusViewer::anchorClicked);

    splitter = new QSplitter(topSplitter);
    splitter->addWidget(servicesView);

    QWidget *servicesWidget = new QWidget;
    QVBoxLayout *servicesLayout = new QVBoxLayout(servicesWidget);
    servicesLayout->addWidget(serviceFilterLine);
    servicesLayout->addWidget(servicesView);
    splitter->addWidget(servicesWidget);
    splitter->addWidget(tree);

    topSplitter->addWidget(splitter);
    topSplitter->addWidget(log);

    connect(servicesView->selectionModel(), &QItemSelectionModel::currentChanged, this, &QDBusViewer::serviceChanged);
    connect(tree, &QWidget::customContextMenuRequested, this, &QDBusViewer::showContextMenu);

    QMetaObject::invokeMethod(this, "refresh", Qt::QueuedConnection);

    if (c.isConnected()) {
        logMessage(QLatin1String("Connected to D-Bus."));
        QDBusConnectionInterface *iface = c.interface();
        connect(iface, &QDBusConnectionInterface::serviceRegistered, this, &QDBusViewer::serviceRegistered);
        connect(iface, &QDBusConnectionInterface::serviceUnregistered, this, &QDBusViewer::serviceUnregistered);
        connect(iface, &QDBusConnectionInterface::serviceOwnerChanged, this, &QDBusViewer::serviceOwnerChanged);
    } else {
        logError(QLatin1String("Cannot connect to D-Bus: ") + c.lastError().message());
    }

    objectPathRegExp.setMinimal(true);

}

static inline QString topSplitterStateKey() { return QStringLiteral("topSplitterState"); }
static inline QString splitterStateKey() { return QStringLiteral("splitterState"); }

void QDBusViewer::saveState(QSettings *settings) const
{
    settings->setValue(topSplitterStateKey(), topSplitter->saveState());
    settings->setValue(splitterStateKey(), splitter->saveState());
}

void QDBusViewer::restoreState(const QSettings *settings)
{
    topSplitter->restoreState(settings->value(topSplitterStateKey()).toByteArray());
    splitter->restoreState(settings->value(splitterStateKey()).toByteArray());
}

void QDBusViewer::logMessage(const QString &msg)
{
    log->append(msg + QLatin1Char('\n'));
}

void QDBusViewer::showEvent(QShowEvent *)
{
    serviceFilterLine->setFocus();
}

bool QDBusViewer::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == servicesView) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->modifiers() == Qt::NoModifier) {
                if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return) {
                    tree->setFocus();
                }
            }
        }
    }
    return false;
}

void QDBusViewer::logError(const QString &msg)
{
    log->append(QLatin1String("<font color=\"red\">Error: </font>") + msg.toHtmlEscaped() + QLatin1String("<br>"));
}

void QDBusViewer::refresh()
{
    servicesModel->removeRows(0, servicesModel->rowCount());

    if (c.isConnected()) {
        const QStringList serviceNames = c.interface()->registeredServiceNames();
        servicesModel->setStringList(serviceNames);
    }
}

void QDBusViewer::activate(const QModelIndex &item)
{
    if (!item.isValid())
        return;

    const QDBusModel *model = static_cast<const QDBusModel *>(item.model());

    BusSignature sig;
    sig.mService = currentService;
    sig.mPath = model->dBusPath(item);
    sig.mInterface = model->dBusInterface(item);
    sig.mName = model->dBusMethodName(item);
    sig.mTypeSig = model->dBusTypeSignature(item);

    switch (model->itemType(item)) {
    case QDBusModel::SignalItem:
        connectionRequested(sig);
        break;
    case QDBusModel::MethodItem:
        callMethod(sig);
        break;
    case QDBusModel::PropertyItem:
        getProperty(sig);
        break;
    default:
        break;
    }
}

void QDBusViewer::getProperty(const BusSignature &sig)
{
    QDBusMessage message = QDBusMessage::createMethodCall(sig.mService, sig.mPath, QLatin1String("org.freedesktop.DBus.Properties"), QLatin1String("Get"));
    QList<QVariant> arguments;
    arguments << sig.mInterface << sig.mName;
    message.setArguments(arguments);
    c.callWithCallback(message, this, SLOT(dumpMessage(QDBusMessage)));
}

void QDBusViewer::setProperty(const BusSignature &sig)
{
    QDBusInterface iface(sig.mService, sig.mPath, sig.mInterface, c);
    QMetaProperty prop = iface.metaObject()->property(iface.metaObject()->indexOfProperty(sig.mName.toLatin1()));

    bool ok;
    QString input = QInputDialog::getText(this, tr("Arguments"),
                    tr("Please enter the value of the property %1 (type %2)").arg(
                        sig.mName, QString::fromLatin1(prop.typeName())),
                    QLineEdit::Normal, QString(), &ok);
    if (!ok)
        return;

    QVariant value = input;
    if (!value.convert(prop.type())) {
        QMessageBox::warning(this, tr("Unable to marshall"),
                tr("Value conversion failed, unable to set property"));
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(sig.mService, sig.mPath, QLatin1String("org.freedesktop.DBus.Properties"), QLatin1String("Set"));
    QList<QVariant> arguments;
    arguments << sig.mInterface << sig.mName << QVariant::fromValue(QDBusVariant(value));
    message.setArguments(arguments);
    c.callWithCallback(message, this, SLOT(dumpMessage(QDBusMessage)));

}

static QString getDbusSignature(const QMetaMethod& method)
{
    // create a D-Bus type signature from QMetaMethod's parameters
    QString sig;
    for (int i = 0; i < method.parameterTypes().count(); ++i) {
        int type = QMetaType::type(method.parameterTypes().at(i));
        sig.append(QString::fromLatin1(QDBusMetaType::typeToSignature(type)));
    }
    return sig;
}

void QDBusViewer::callMethod(const BusSignature &sig)
{
    QDBusInterface iface(sig.mService, sig.mPath, sig.mInterface, c);
    const QMetaObject *mo = iface.metaObject();

    // find the method
    QMetaMethod method;
    for (int i = 0; i < mo->methodCount(); ++i) {
        const QString signature = QString::fromLatin1(mo->method(i).methodSignature());
        if (signature.startsWith(sig.mName) && signature.at(sig.mName.length()) == QLatin1Char('('))
            if (getDbusSignature(mo->method(i)) == sig.mTypeSig)
                method = mo->method(i);
    }
    if (!method.isValid()) {
        QMessageBox::warning(this, tr("Unable to find method"),
                tr("Unable to find method %1 on path %2 in interface %3").arg(
                    sig.mName).arg(sig.mPath).arg(sig.mInterface));
        return;
    }

    PropertyDialog dialog;
    QList<QVariant> args;

    const QList<QByteArray> paramTypes = method.parameterTypes();
    const QList<QByteArray> paramNames = method.parameterNames();
    QList<int> types; // remember the low-level D-Bus type
    for (int i = 0; i < paramTypes.count(); ++i) {
        const QByteArray paramType = paramTypes.at(i);
        if (paramType.endsWith('&'))
            continue; // ignore OUT parameters

        int type = QMetaType::type(paramType);
        dialog.addProperty(QString::fromLatin1(paramNames.value(i)), type);
        types.append(type);
    }

    if (!types.isEmpty()) {
        dialog.setInfo(tr("Please enter parameters for the method \"%1\"").arg(sig.mName));

        if (dialog.exec() != QDialog::Accepted)
            return;

        args = dialog.values();
    }

    // Try to convert the values we got as closely as possible to the
    // dbus signature. This is especially important for those input as strings
    for (int i = 0; i < args.count(); ++i) {
        QVariant a = args.at(i);
        int desttype = types.at(i);
        if (desttype < int(QMetaType::User) && desttype != int(QVariant::Map)
            && a.canConvert(desttype)) {
            args[i].convert(desttype);
        }
        // Special case - convert a value to a QDBusVariant if the
        // interface wants a variant
        if (types.at(i) == qMetaTypeId<QDBusVariant>())
            args[i] = QVariant::fromValue(QDBusVariant(args.at(i)));
    }

    QDBusMessage message = QDBusMessage::createMethodCall(sig.mService, sig.mPath, sig.mInterface,
            sig.mName);
    message.setArguments(args);
    c.callWithCallback(message, this, SLOT(dumpMessage(QDBusMessage)));
}

void QDBusViewer::showContextMenu(const QPoint &point)
{
    QModelIndex item = tree->indexAt(point);
    if (!item.isValid())
        return;

    const QDBusModel *model = static_cast<const QDBusModel *>(item.model());

    BusSignature sig;
    sig.mService = currentService;
    sig.mPath = model->dBusPath(item);
    sig.mInterface = model->dBusInterface(item);
    sig.mName = model->dBusMethodName(item);
    sig.mTypeSig = model->dBusTypeSignature(item);

    QMenu menu;
    menu.addAction(refreshAction);

    switch (model->itemType(item)) {
    case QDBusModel::SignalItem: {
        QAction *action = new QAction(tr("&Connect"), &menu);
        action->setData(1);
        menu.addAction(action);
        break; }
    case QDBusModel::MethodItem: {
        QAction *action = new QAction(tr("&Call"), &menu);
        action->setData(2);
        menu.addAction(action);
        break; }
    case QDBusModel::PropertyItem: {
        QAction *actionSet = new QAction(tr("&Set value"), &menu);
        actionSet->setData(3);
        QAction *actionGet = new QAction(tr("&Get value"), &menu);
        actionGet->setData(4);
        menu.addAction(actionSet);
        menu.addAction(actionGet);
        break; }
    default:
        break;
    }

    QAction *selectedAction = menu.exec(tree->viewport()->mapToGlobal(point));
    if (!selectedAction)
        return;

    switch (selectedAction->data().toInt()) {
    case 1:
        connectionRequested(sig);
        break;
    case 2:
        callMethod(sig);
        break;
    case 3:
        setProperty(sig);
        break;
    case 4:
        getProperty(sig);
        break;
    }
}

void QDBusViewer::connectionRequested(const BusSignature &sig)
{
    if (!c.connect(sig.mService, QString(), sig.mInterface, sig.mName, this,
              SLOT(dumpMessage(QDBusMessage)))) {
        logError(tr("Unable to connect to service %1, path %2, interface %3, signal %4").arg(
                    sig.mService).arg(sig.mPath).arg(sig.mInterface).arg(sig.mName));
    }
}

void QDBusViewer::dumpMessage(const QDBusMessage &message)
{
    QList<QVariant> args = message.arguments();
    QString out = QLatin1String("Received ");

    switch (message.type()) {
    case QDBusMessage::SignalMessage:
        out += QLatin1String("signal ");
        break;
    case QDBusMessage::ErrorMessage:
        out += QLatin1String("error message ");
        break;
    case QDBusMessage::ReplyMessage:
        out += QLatin1String("reply ");
        break;
    default:
        out += QLatin1String("message ");
        break;
    }

    out += QLatin1String("from ");
    out += message.service();
    if (!message.path().isEmpty())
        out += QLatin1String(", path ") + message.path();
    if (!message.interface().isEmpty())
        out += QLatin1String(", interface <i>") + message.interface() + QLatin1String("</i>");
    if (!message.member().isEmpty())
        out += QLatin1String(", member ") + message.member();
    out += QLatin1String("<br>");
    if (args.isEmpty()) {
        out += QLatin1String("&nbsp;&nbsp;(no arguments)");
    } else {
        out += QLatin1String("&nbsp;&nbsp;Arguments: ");
        for (const QVariant &arg : qAsConst(args)) {
            QString str = QDBusUtil::argumentToString(arg).toHtmlEscaped();
            // turn object paths into clickable links
            str.replace(objectPathRegExp, QLatin1String("[ObjectPath: <a href=\"qdbus://bus\\1\">\\1</a>]"));
            // convert new lines from command to proper HTML line breaks
            str.replace(QStringLiteral("\n"), QStringLiteral("<br/>"));
            out += str;
            out += QLatin1String(", ");
        }
        out.chop(2);
    }

    log->append(out);
}

void QDBusViewer::serviceChanged(const QModelIndex &index)
{
    delete tree->model();

    currentService.clear();
    if (!index.isValid())
        return;
    currentService = index.data().toString();

    QDBusViewModel *model = new QDBusViewModel(currentService, c);
    tree->setModel(model);
    connect(model, &QDBusModel::busError, this, &QDBusViewer::logError);
}

void QDBusViewer::serviceRegistered(const QString &service)
{
    if (service == c.baseService())
        return;

    servicesModel->insertRows(0, 1);
    servicesModel->setData(servicesModel->index(0, 0), service);
}

static QModelIndex findItem(QStringListModel *servicesModel, const QString &name)
{
    QModelIndexList hits = servicesModel->match(servicesModel->index(0, 0), Qt::DisplayRole, name);
    if (hits.isEmpty())
        return QModelIndex();

    return hits.first();
}

void QDBusViewer::serviceUnregistered(const QString &name)
{
    QModelIndex hit = findItem(servicesModel, name);
    if (!hit.isValid())
        return;
    servicesModel->removeRows(hit.row(), 1);
}

void QDBusViewer::serviceOwnerChanged(const QString &name, const QString &oldOwner,
                                      const QString &newOwner)
{
    QModelIndex hit = findItem(servicesModel, name);

    if (!hit.isValid() && oldOwner.isEmpty() && !newOwner.isEmpty())
        serviceRegistered(name);
    else if (hit.isValid() && !oldOwner.isEmpty() && newOwner.isEmpty())
        servicesModel->removeRows(hit.row(), 1);
    else if (hit.isValid() && !oldOwner.isEmpty() && !newOwner.isEmpty()) {
        servicesModel->removeRows(hit.row(), 1);
        serviceRegistered(name);
    }
}

void QDBusViewer::serviceFilterReturnPressed()
{
    if (servicesProxyModel->rowCount() <= 0)
        return;

    servicesView->selectRow(0);
    servicesView->setFocus();
}

void QDBusViewer::refreshChildren()
{
    QDBusModel *model = qobject_cast<QDBusModel *>(tree->model());
    if (!model)
        return;
    model->refresh(tree->currentIndex());
}

void QDBusViewer::anchorClicked(const QUrl &url)
{
    if (url.scheme() != QLatin1String("qdbus"))
        // not ours
        return;

    // swallow the click without setting a new document
    log->setSource(QUrl());

    QDBusModel *model = qobject_cast<QDBusModel *>(tree->model());
    if (!model)
        return;

    QModelIndex idx = model->findObject(QDBusObjectPath(url.path()));
    if (!idx.isValid())
        return;

    tree->scrollTo(idx);
    tree->setCurrentIndex(idx);
}
