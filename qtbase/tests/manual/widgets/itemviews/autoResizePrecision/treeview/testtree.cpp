// Copyright (C) 2012 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtWidgets/QtWidgets>

class TreeDialog : public QDialog
{
    Q_OBJECT
public:
    TreeDialog() { create(); }
protected:
    void create()
    {
        resize(1000, 233);
        gridLayout = new QGridLayout(this);
        treeWidget = new QTreeWidget(this);

        gridLayout->addWidget(treeWidget, 0, 0, 2, 1);
        spinPrecision = new QSpinBox(this);
        gridLayout->addWidget(spinPrecision, 0, 1, 1, 1);
        verticalSpacer = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
        gridLayout->addItem(verticalSpacer, 1, 1, 1, 1);

        QStringList itemInfo("Col1");
        itemInfo.append("Col2");
        itemInfo.append("Col3");
        itemInfo.append("Dummy");
        // Developer no. could also have been social security number og some other id.
        treeWidget->setHeaderLabels(itemInfo);

        QStringList sl1("This is Root Item");
        sl1.append("i");
        QTreeWidgetItem *rootitem = new QTreeWidgetItem(treeWidget, sl1);

        QStringList sl2("This is Child1 Item");
        sl2.append("WW");
        QTreeWidgetItem *child1 = new QTreeWidgetItem(rootitem, sl2);

        QString ii = QString::fromLatin1("ii");
        QStringList is;
        const int rowCount = 3000;
        spinPrecision->setMinimum(-1);
        spinPrecision->setMaximum(rowCount + 5);
        for (int u = 0; u < rowCount; ++u) {
            if (u % 25 == 0)
                ii += QString::fromLatin1("i");
            else
                ii[ii.length() - 1] = QChar::fromLatin1('a' + (u  % 25));
            ii[ii.length() - 2] = QChar::fromLatin1('i');
            is.append(ii);
        }

        for (int u = 0; u < rowCount - 2; ++u) { // -2 since we have rootitem and child1
            QString col1;
            col1 = QString::fromLatin1("This is child item %1").arg(u + 2);

            QStringList sl(col1);
            sl.append(is[u]);
            sl.append(is[rowCount - u - 1]);

            if (u > 500)
                new QTreeWidgetItem(rootitem, sl);
            else
                new QTreeWidgetItem(child1, sl);
        }
        treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        treeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        spinPrecision->setValue(treeWidget->header()->resizeContentsPrecision());
        connect(spinPrecision, SIGNAL(valueChanged(int)), this, SLOT(slotValueChanged(int)));
    } // setupUi
protected slots:
    void slotValueChanged(int newval);
protected:
    QGridLayout *gridLayout;
    QTreeWidget *treeWidget;
    QSpinBox *spinPrecision;
    QSpacerItem *verticalSpacer;
};

void TreeDialog::slotValueChanged(int newval)
{
    treeWidget->header()->setResizeContentsPrecision(newval);
    for (int u = 0; u < treeWidget->header()->count(); ++u)
        treeWidget->resizeColumnToContents(u);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TreeDialog d1;
    d1.show();
    app.exec();
}

#include "testtree.moc"
