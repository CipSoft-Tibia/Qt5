// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TREEITEM_H
#define TREEITEM_H

#include <QObject>
#include <QOpcUaEUInformation>
#include <QOpcUaNode>
#include <QOpcUaRange>

#include <memory>

class OpcUaModel;

class TreeItem : public QObject
{
    Q_OBJECT
public:
    explicit TreeItem(OpcUaModel *model);
    TreeItem(QOpcUaNode *node, OpcUaModel *model, TreeItem *parent);
    TreeItem(QOpcUaNode *node, OpcUaModel *model, const QOpcUaReferenceDescription &browsingData, TreeItem *parent);
    ~TreeItem();
    TreeItem *child(int row);
    int childIndex(const TreeItem *child) const;
    int childCount();
    int columnCount() const;
    QVariant data(int column);
    int row() const;
    TreeItem *parentItem();
    void appendChild(TreeItem *child);
    QPixmap icon(int column) const;
    bool hasChildNodeItem(const QString &nodeId) const;
    void setMonitoringEnabled(bool active);
    bool monitoringEnabled() const;
    bool supportsMonitoring() const;

private slots:
    void startBrowsing();
    void handleAttributes(QOpcUa::NodeAttributes attr);
    void browseFinished(const QList<QOpcUaReferenceDescription> &children, QOpcUa::UaStatusCode statusCode);

private:
    QString variantToString(const QVariant &value, const QString &typeNodeId = QString()) const;
    QString localizedTextToString(const QOpcUaLocalizedText &text) const;
    QString rangeToString(const QOpcUaRange &range) const;
    QString euInformationToString(const QOpcUaEUInformation &info) const;
    template <typename T>
    QString numberArrayToString(const QList<T> &vec) const;

    std::unique_ptr<QOpcUaNode> mOpcNode;
    OpcUaModel *mModel = nullptr;
    bool mAttributesReady = false;
    bool mBrowseStarted = false;
    QList<TreeItem *> mChildItems;
    QSet<QString> mChildNodeIds;
    TreeItem *mParentItem = nullptr;

private:
    QString mNodeBrowseName;
    QString mNodeId;
    QString mNodeDisplayName;
    bool mHistorizing;
    QOpcUa::NodeClass mNodeClass = QOpcUa::NodeClass::Undefined;
};

template <typename T>
QString TreeItem::numberArrayToString(const QList<T> &vec) const
{
    QString list(QLatin1Char('['));
    for (int i = 0, size = vec.size(); i < size; ++i) {
        if (i)
            list.append(QLatin1Char(';'));
        list.append(QString::number(vec.at(i)));
    }
    list.append(QLatin1Char(']'));
    return list;
}

#endif // TREEITEM_H
