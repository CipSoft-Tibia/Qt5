/********************************************************************************
** Form generated from reading UI file 'qpagesetupwidget.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef QPAGESETUPWIDGET_H
#define QPAGESETUPWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QPageSetupWidget
{
public:
    QGridLayout *gridLayout_3;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_2;
    QLabel *pageSizeLabel;
    QComboBox *pageSizeCombo;
    QLabel *widthLabel;
    QHBoxLayout *horizontalLayout_3;
    QDoubleSpinBox *pageWidth;
    QLabel *heightLabel;
    QDoubleSpinBox *pageHeight;
    QLabel *paperSourceLabel;
    QComboBox *paperSource;
    QSpacerItem *horizontalSpacer_4;
    QHBoxLayout *horizontalLayout_4;
    QComboBox *unitCombo;
    QSpacerItem *horizontalSpacer_3;
    QWidget *preview;
    QGroupBox *groupBox_3;
    QVBoxLayout *verticalLayout;
    QRadioButton *portrait;
    QRadioButton *landscape;
    QRadioButton *reverseLandscape;
    QRadioButton *reversePortrait;
    QSpacerItem *verticalSpacer_5;
    QGroupBox *groupBox;
    QHBoxLayout *horizontalLayout_2;
    QGridLayout *gridLayout;
    QDoubleSpinBox *topMargin;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_7;
    QDoubleSpinBox *leftMargin;
    QSpacerItem *horizontalSpacer;
    QDoubleSpinBox *rightMargin;
    QSpacerItem *horizontalSpacer_8;
    QSpacerItem *horizontalSpacer_2;
    QDoubleSpinBox *bottomMargin;
    QSpacerItem *horizontalSpacer_5;
    QGroupBox *pagesPerSheetButtonGroup;
    QGridLayout *gridLayout_4;
    QComboBox *pagesPerSheetCombo;
    QSpacerItem *horizontalSpacer_6;
    QLabel *label;
    QComboBox *pagesPerSheetLayoutCombo;
    QLabel *label_2;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *QPageSetupWidget)
    {
        if (QPageSetupWidget->objectName().isEmpty())
            QPageSetupWidget->setObjectName("QPageSetupWidget");
        QPageSetupWidget->resize(416, 515);
        gridLayout_3 = new QGridLayout(QPageSetupWidget);
        gridLayout_3->setContentsMargins(0, 0, 0, 0);
        gridLayout_3->setObjectName("gridLayout_3");
        groupBox_2 = new QGroupBox(QPageSetupWidget);
        groupBox_2->setObjectName("groupBox_2");
        gridLayout_2 = new QGridLayout(groupBox_2);
        gridLayout_2->setObjectName("gridLayout_2");
        pageSizeLabel = new QLabel(groupBox_2);
        pageSizeLabel->setObjectName("pageSizeLabel");

        gridLayout_2->addWidget(pageSizeLabel, 0, 0, 1, 1);

        pageSizeCombo = new QComboBox(groupBox_2);
        pageSizeCombo->setObjectName("pageSizeCombo");

        gridLayout_2->addWidget(pageSizeCombo, 0, 1, 1, 1);

        widthLabel = new QLabel(groupBox_2);
        widthLabel->setObjectName("widthLabel");

        gridLayout_2->addWidget(widthLabel, 1, 0, 1, 1);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        pageWidth = new QDoubleSpinBox(groupBox_2);
        pageWidth->setObjectName("pageWidth");
        pageWidth->setMaximum(9999.989999999999782);

        horizontalLayout_3->addWidget(pageWidth);

        heightLabel = new QLabel(groupBox_2);
        heightLabel->setObjectName("heightLabel");

        horizontalLayout_3->addWidget(heightLabel);

        pageHeight = new QDoubleSpinBox(groupBox_2);
        pageHeight->setObjectName("pageHeight");
        pageHeight->setMaximum(9999.989999999999782);

        horizontalLayout_3->addWidget(pageHeight);


        gridLayout_2->addLayout(horizontalLayout_3, 1, 1, 1, 1);

        paperSourceLabel = new QLabel(groupBox_2);
        paperSourceLabel->setObjectName("paperSourceLabel");

        gridLayout_2->addWidget(paperSourceLabel, 2, 0, 1, 1);

        paperSource = new QComboBox(groupBox_2);
        paperSource->setObjectName("paperSource");

        gridLayout_2->addWidget(paperSource, 2, 1, 1, 1);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_4, 1, 2, 1, 1);


        gridLayout_3->addWidget(groupBox_2, 1, 0, 1, 2);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        unitCombo = new QComboBox(QPageSetupWidget);
        unitCombo->setObjectName("unitCombo");

        horizontalLayout_4->addWidget(unitCombo);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_3);


        gridLayout_3->addLayout(horizontalLayout_4, 0, 0, 1, 2);

        preview = new QWidget(QPageSetupWidget);
        preview->setObjectName("preview");

        gridLayout_3->addWidget(preview, 2, 1, 2, 1);

        groupBox_3 = new QGroupBox(QPageSetupWidget);
        groupBox_3->setObjectName("groupBox_3");
        verticalLayout = new QVBoxLayout(groupBox_3);
        verticalLayout->setObjectName("verticalLayout");
        portrait = new QRadioButton(groupBox_3);
        portrait->setObjectName("portrait");
        portrait->setChecked(true);

        verticalLayout->addWidget(portrait);

        landscape = new QRadioButton(groupBox_3);
        landscape->setObjectName("landscape");

        verticalLayout->addWidget(landscape);

        reverseLandscape = new QRadioButton(groupBox_3);
        reverseLandscape->setObjectName("reverseLandscape");

        verticalLayout->addWidget(reverseLandscape);

        reversePortrait = new QRadioButton(groupBox_3);
        reversePortrait->setObjectName("reversePortrait");

        verticalLayout->addWidget(reversePortrait);

        verticalSpacer_5 = new QSpacerItem(0, 0, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer_5);


        gridLayout_3->addWidget(groupBox_3, 2, 0, 1, 1);

        groupBox = new QGroupBox(QPageSetupWidget);
        groupBox->setObjectName("groupBox");
        horizontalLayout_2 = new QHBoxLayout(groupBox);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        topMargin = new QDoubleSpinBox(groupBox);
        topMargin->setObjectName("topMargin");
        topMargin->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        topMargin->setMaximum(999.990000000000009);

        gridLayout->addWidget(topMargin, 0, 1, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalSpacer_7 = new QSpacerItem(0, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_7);

        leftMargin = new QDoubleSpinBox(groupBox);
        leftMargin->setObjectName("leftMargin");
        leftMargin->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        leftMargin->setMaximum(999.990000000000009);

        horizontalLayout->addWidget(leftMargin);

        horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        rightMargin = new QDoubleSpinBox(groupBox);
        rightMargin->setObjectName("rightMargin");
        rightMargin->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        rightMargin->setMaximum(999.990000000000009);

        horizontalLayout->addWidget(rightMargin);

        horizontalSpacer_8 = new QSpacerItem(0, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_8);


        gridLayout->addLayout(horizontalLayout, 1, 0, 1, 3);

        horizontalSpacer_2 = new QSpacerItem(0, 20, QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(horizontalSpacer_2, 0, 2, 1, 1);

        bottomMargin = new QDoubleSpinBox(groupBox);
        bottomMargin->setObjectName("bottomMargin");
        bottomMargin->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        bottomMargin->setMaximum(999.990000000000009);

        gridLayout->addWidget(bottomMargin, 2, 1, 1, 1);

        horizontalSpacer_5 = new QSpacerItem(0, 20, QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(horizontalSpacer_5, 0, 0, 1, 1);


        horizontalLayout_2->addLayout(gridLayout);


        gridLayout_3->addWidget(groupBox, 3, 0, 1, 1);

        pagesPerSheetButtonGroup = new QGroupBox(QPageSetupWidget);
        pagesPerSheetButtonGroup->setObjectName("pagesPerSheetButtonGroup");
        gridLayout_4 = new QGridLayout(pagesPerSheetButtonGroup);
        gridLayout_4->setObjectName("gridLayout_4");
        pagesPerSheetCombo = new QComboBox(pagesPerSheetButtonGroup);
        pagesPerSheetCombo->setObjectName("pagesPerSheetCombo");

        gridLayout_4->addWidget(pagesPerSheetCombo, 0, 1, 1, 1);

        horizontalSpacer_6 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_4->addItem(horizontalSpacer_6, 0, 2, 1, 1);

        label = new QLabel(pagesPerSheetButtonGroup);
        label->setObjectName("label");

        gridLayout_4->addWidget(label, 1, 0, 1, 1);

        pagesPerSheetLayoutCombo = new QComboBox(pagesPerSheetButtonGroup);
        pagesPerSheetLayoutCombo->setObjectName("pagesPerSheetLayoutCombo");

        gridLayout_4->addWidget(pagesPerSheetLayoutCombo, 1, 1, 1, 1);

        label_2 = new QLabel(pagesPerSheetButtonGroup);
        label_2->setObjectName("label_2");

        gridLayout_4->addWidget(label_2, 0, 0, 1, 1);


        gridLayout_3->addWidget(pagesPerSheetButtonGroup, 5, 0, 1, 2);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_3->addItem(verticalSpacer, 6, 0, 1, 1);

#if QT_CONFIG(shortcut)
        pageSizeLabel->setBuddy(pageSizeCombo);
        widthLabel->setBuddy(pageWidth);
        heightLabel->setBuddy(pageHeight);
        paperSourceLabel->setBuddy(paperSource);
#endif // QT_CONFIG(shortcut)

        retranslateUi(QPageSetupWidget);

        QMetaObject::connectSlotsByName(QPageSetupWidget);
    } // setupUi

    void retranslateUi(QWidget *QPageSetupWidget)
    {
        QPageSetupWidget->setWindowTitle(QCoreApplication::translate("QPageSetupWidget", "Form", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("QPageSetupWidget", "Paper", nullptr));
        pageSizeLabel->setText(QCoreApplication::translate("QPageSetupWidget", "Page size:", nullptr));
        widthLabel->setText(QCoreApplication::translate("QPageSetupWidget", "Width:", nullptr));
        heightLabel->setText(QCoreApplication::translate("QPageSetupWidget", "Height:", nullptr));
        paperSourceLabel->setText(QCoreApplication::translate("QPageSetupWidget", "Paper source:", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("QPageSetupWidget", "Orientation", nullptr));
        portrait->setText(QCoreApplication::translate("QPageSetupWidget", "Portrait", nullptr));
        landscape->setText(QCoreApplication::translate("QPageSetupWidget", "Landscape", nullptr));
        reverseLandscape->setText(QCoreApplication::translate("QPageSetupWidget", "Reverse landscape", nullptr));
        reversePortrait->setText(QCoreApplication::translate("QPageSetupWidget", "Reverse portrait", nullptr));
        groupBox->setTitle(QCoreApplication::translate("QPageSetupWidget", "Margins", nullptr));
#if QT_CONFIG(tooltip)
        topMargin->setToolTip(QCoreApplication::translate("QPageSetupWidget", "top margin", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        topMargin->setAccessibleName(QCoreApplication::translate("QPageSetupWidget", "top margin", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        leftMargin->setToolTip(QCoreApplication::translate("QPageSetupWidget", "left margin", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        leftMargin->setAccessibleName(QCoreApplication::translate("QPageSetupWidget", "left margin", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        rightMargin->setToolTip(QCoreApplication::translate("QPageSetupWidget", "right margin", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        rightMargin->setAccessibleName(QCoreApplication::translate("QPageSetupWidget", "right margin", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        bottomMargin->setToolTip(QCoreApplication::translate("QPageSetupWidget", "bottom margin", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        bottomMargin->setAccessibleName(QCoreApplication::translate("QPageSetupWidget", "bottom margin", nullptr));
#endif // QT_CONFIG(accessibility)
        pagesPerSheetButtonGroup->setTitle(QCoreApplication::translate("QPageSetupWidget", "Page Layout", nullptr));
        label->setText(QCoreApplication::translate("QPageSetupWidget", "Page order:", nullptr));
        label_2->setText(QCoreApplication::translate("QPageSetupWidget", "Pages per sheet:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class QPageSetupWidget: public Ui_QPageSetupWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // QPAGESETUPWIDGET_H
