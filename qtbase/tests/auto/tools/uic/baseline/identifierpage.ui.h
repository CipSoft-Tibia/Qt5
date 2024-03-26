/********************************************************************************
** Form generated from reading UI file 'identifierpage.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef IDENTIFIERPAGE_H
#define IDENTIFIERPAGE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_IdentifierPage
{
public:
    QGridLayout *gridLayout;
    QSpacerItem *spacerItem;
    QCheckBox *identifierCheckBox;
    QSpacerItem *spacerItem1;
    QSpacerItem *spacerItem2;
    QRadioButton *globalButton;
    QLineEdit *prefixLineEdit;
    QRadioButton *fileNameButton;
    QSpacerItem *spacerItem3;

    void setupUi(QWidget *IdentifierPage)
    {
        if (IdentifierPage->objectName().isEmpty())
            IdentifierPage->setObjectName("IdentifierPage");
        IdentifierPage->resize(417, 242);
        gridLayout = new QGridLayout(IdentifierPage);
        gridLayout->setObjectName("gridLayout");
        spacerItem = new QSpacerItem(20, 20, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        gridLayout->addItem(spacerItem, 0, 1, 1, 1);

        identifierCheckBox = new QCheckBox(IdentifierPage);
        identifierCheckBox->setObjectName("identifierCheckBox");

        gridLayout->addWidget(identifierCheckBox, 1, 0, 1, 3);

        spacerItem1 = new QSpacerItem(161, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(spacerItem1, 1, 3, 1, 1);

        spacerItem2 = new QSpacerItem(30, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(spacerItem2, 2, 0, 1, 1);

        globalButton = new QRadioButton(IdentifierPage);
        globalButton->setObjectName("globalButton");
        globalButton->setEnabled(false);
        globalButton->setChecked(true);

        gridLayout->addWidget(globalButton, 2, 1, 1, 1);

        prefixLineEdit = new QLineEdit(IdentifierPage);
        prefixLineEdit->setObjectName("prefixLineEdit");
        prefixLineEdit->setEnabled(false);

        gridLayout->addWidget(prefixLineEdit, 2, 2, 1, 1);

        fileNameButton = new QRadioButton(IdentifierPage);
        fileNameButton->setObjectName("fileNameButton");
        fileNameButton->setEnabled(false);

        gridLayout->addWidget(fileNameButton, 3, 1, 1, 2);

        spacerItem3 = new QSpacerItem(20, 31, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout->addItem(spacerItem3, 4, 1, 1, 1);


        retranslateUi(IdentifierPage);
        QObject::connect(globalButton, &QRadioButton::toggled, prefixLineEdit, &QLineEdit::setEnabled);

        QMetaObject::connectSlotsByName(IdentifierPage);
    } // setupUi

    void retranslateUi(QWidget *IdentifierPage)
    {
        IdentifierPage->setWindowTitle(QCoreApplication::translate("IdentifierPage", "Form", nullptr));
        identifierCheckBox->setText(QCoreApplication::translate("IdentifierPage", "Create identifiers", nullptr));
        globalButton->setText(QCoreApplication::translate("IdentifierPage", "Global prefix:", nullptr));
        fileNameButton->setText(QCoreApplication::translate("IdentifierPage", "Inherit prefix from file names", nullptr));
    } // retranslateUi

};

namespace Ui {
    class IdentifierPage: public Ui_IdentifierPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // IDENTIFIERPAGE_H
