/********************************************************************************
** Form generated from reading UI file 'filespage.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef FILESPAGE_H
#define FILESPAGE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_FilesPage
{
public:
    QGridLayout *gridLayout;
    QLabel *fileLabel;
    QListWidget *fileListWidget;
    QPushButton *removeButton;
    QPushButton *removeAllButton;
    QSpacerItem *spacerItem;
    QSpacerItem *spacerItem1;

    void setupUi(QWidget *FilesPage)
    {
        if (FilesPage->objectName().isEmpty())
            FilesPage->setObjectName("FilesPage");
        FilesPage->resize(417, 242);
        gridLayout = new QGridLayout(FilesPage);
        gridLayout->setObjectName("gridLayout");
        fileLabel = new QLabel(FilesPage);
        fileLabel->setObjectName("fileLabel");
        fileLabel->setWordWrap(true);

        gridLayout->addWidget(fileLabel, 0, 0, 1, 2);

        fileListWidget = new QListWidget(FilesPage);
        fileListWidget->setObjectName("fileListWidget");

        gridLayout->addWidget(fileListWidget, 1, 0, 3, 1);

        removeButton = new QPushButton(FilesPage);
        removeButton->setObjectName("removeButton");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(removeButton->sizePolicy().hasHeightForWidth());
        removeButton->setSizePolicy(sizePolicy);

        gridLayout->addWidget(removeButton, 1, 1, 1, 1);

        removeAllButton = new QPushButton(FilesPage);
        removeAllButton->setObjectName("removeAllButton");

        gridLayout->addWidget(removeAllButton, 2, 1, 1, 1);

        spacerItem = new QSpacerItem(75, 16, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout->addItem(spacerItem, 3, 1, 1, 1);

        spacerItem1 = new QSpacerItem(20, 31, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout->addItem(spacerItem1, 4, 0, 1, 1);


        retranslateUi(FilesPage);

        QMetaObject::connectSlotsByName(FilesPage);
    } // setupUi

    void retranslateUi(QWidget *FilesPage)
    {
        FilesPage->setWindowTitle(QCoreApplication::translate("FilesPage", "Form", nullptr));
        fileLabel->setText(QCoreApplication::translate("FilesPage", "Files:", nullptr));
        removeButton->setText(QCoreApplication::translate("FilesPage", "Remove", nullptr));
        removeAllButton->setText(QCoreApplication::translate("FilesPage", "Remove All", nullptr));
    } // retranslateUi

};

namespace Ui {
    class FilesPage: public Ui_FilesPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // FILESPAGE_H
