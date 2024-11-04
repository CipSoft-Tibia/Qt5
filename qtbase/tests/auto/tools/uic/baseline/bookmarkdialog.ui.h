/********************************************************************************
** Form generated from reading UI file 'bookmarkdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef BOOKMARKDIALOG_H
#define BOOKMARKDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include "bookmarkwidget.h"

QT_BEGIN_NAMESPACE

class Ui_BookmarkDialog
{
public:
    QVBoxLayout *verticalLayout_3;
    QHBoxLayout *horizontalLayout;
    QVBoxLayout *verticalLayout_2;
    QLabel *label;
    QLabel *label_2;
    QVBoxLayout *verticalLayout;
    QLineEdit *bookmarkEdit;
    QComboBox *bookmarkFolders;
    QHBoxLayout *horizontalLayout_3;
    QToolButton *toolButton;
    QFrame *line;
    BookmarkWidget *bookmarkWidget;
    QHBoxLayout *horizontalLayout_4;
    QPushButton *newFolderButton;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *BookmarkDialog)
    {
        if (BookmarkDialog->objectName().isEmpty())
            BookmarkDialog->setObjectName("BookmarkDialog");
        BookmarkDialog->resize(450, 135);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(BookmarkDialog->sizePolicy().hasHeightForWidth());
        BookmarkDialog->setSizePolicy(sizePolicy);
        verticalLayout_3 = new QVBoxLayout(BookmarkDialog);
        verticalLayout_3->setObjectName("verticalLayout_3");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName("verticalLayout_2");
        label = new QLabel(BookmarkDialog);
        label->setObjectName("label");

        verticalLayout_2->addWidget(label);

        label_2 = new QLabel(BookmarkDialog);
        label_2->setObjectName("label_2");

        verticalLayout_2->addWidget(label_2);


        horizontalLayout->addLayout(verticalLayout_2);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        bookmarkEdit = new QLineEdit(BookmarkDialog);
        bookmarkEdit->setObjectName("bookmarkEdit");

        verticalLayout->addWidget(bookmarkEdit);

        bookmarkFolders = new QComboBox(BookmarkDialog);
        bookmarkFolders->setObjectName("bookmarkFolders");

        verticalLayout->addWidget(bookmarkFolders);


        horizontalLayout->addLayout(verticalLayout);


        verticalLayout_3->addLayout(horizontalLayout);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        toolButton = new QToolButton(BookmarkDialog);
        toolButton->setObjectName("toolButton");
        toolButton->setMinimumSize(QSize(25, 20));

        horizontalLayout_3->addWidget(toolButton);

        line = new QFrame(BookmarkDialog);
        line->setObjectName("line");
        line->setFrameShape(QFrame::Shape::HLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);

        horizontalLayout_3->addWidget(line);


        verticalLayout_3->addLayout(horizontalLayout_3);

        bookmarkWidget = new BookmarkWidget(BookmarkDialog);
        bookmarkWidget->setObjectName("bookmarkWidget");
        bookmarkWidget->setEnabled(true);
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Ignored);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(bookmarkWidget->sizePolicy().hasHeightForWidth());
        bookmarkWidget->setSizePolicy(sizePolicy1);

        verticalLayout_3->addWidget(bookmarkWidget);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        newFolderButton = new QPushButton(BookmarkDialog);
        newFolderButton->setObjectName("newFolderButton");

        horizontalLayout_4->addWidget(newFolderButton);

        buttonBox = new QDialogButtonBox(BookmarkDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        horizontalLayout_4->addWidget(buttonBox);


        verticalLayout_3->addLayout(horizontalLayout_4);


        retranslateUi(BookmarkDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, BookmarkDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, BookmarkDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(BookmarkDialog);
    } // setupUi

    void retranslateUi(QDialog *BookmarkDialog)
    {
        BookmarkDialog->setWindowTitle(QCoreApplication::translate("BookmarkDialog", "Add Bookmark", nullptr));
        label->setText(QCoreApplication::translate("BookmarkDialog", "Bookmark:", nullptr));
        label_2->setText(QCoreApplication::translate("BookmarkDialog", "Add in Folder:", nullptr));
        toolButton->setText(QCoreApplication::translate("BookmarkDialog", "+", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = bookmarkWidget->headerItem();
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("BookmarkDialog", "1", nullptr));
        newFolderButton->setText(QCoreApplication::translate("BookmarkDialog", "New Folder", nullptr));
    } // retranslateUi

};

namespace Ui {
    class BookmarkDialog: public Ui_BookmarkDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // BOOKMARKDIALOG_H
