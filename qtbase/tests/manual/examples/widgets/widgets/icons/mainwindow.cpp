// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "iconpreviewarea.h"
#include "iconsizespinbox.h"
#include "imagedelegate.h"

#include <QActionGroup>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QImageReader>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QRadioButton>
#include <QScreen>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QTableWidget>
#include <QWindow>

//! [40]
enum { OtherSize = QStyle::PM_CustomBase };
//! [40]

//! [0]
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    createActions();

    QGridLayout *mainLayout = new QGridLayout(centralWidget);

    QGroupBox *previewGroupBox = new QGroupBox(tr("Preview"));
    previewArea = new IconPreviewArea(previewGroupBox);
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroupBox);
    previewLayout->addWidget(previewArea);

    mainLayout->addWidget(previewGroupBox, 0, 0, 1, 2);
    mainLayout->addWidget(createImagesGroupBox(), 1, 0);
    QVBoxLayout *vBox = new QVBoxLayout;
    vBox->addWidget(createIconSizeGroupBox());
    vBox->addWidget(createHighDpiIconSizeGroupBox());
    vBox->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
    mainLayout->addLayout(vBox, 1, 1);
    createContextMenu();

    setWindowTitle(tr("Icons"));
    checkCurrentStyle();
    sizeButtonGroup->button(OtherSize)->click();
}
//! [0]

//! [44]
void MainWindow::show()
{
    QMainWindow::show();
    connect(windowHandle(), &QWindow::screenChanged, this, &MainWindow::screenChanged);
    screenChanged();
}
//! [44]

//! [1]
void MainWindow::about()
{
    QMessageBox::about(this, tr("About Icons"),
            tr("The <b>Icons</b> example illustrates how Qt renders an icon in "
               "different modes (active, normal, disabled, and selected) and "
               "states (on and off) based on a set of images."));
}
//! [1]

//! [2]
void MainWindow::changeStyle(bool checked)
{
    if (!checked)
        return;

    const QAction *action = qobject_cast<QAction *>(sender());
//! [2] //! [3]
    QStyle *style = QStyleFactory::create(action->data().toString());
//! [3] //! [4]
    Q_ASSERT(style);
    QApplication::setStyle(style);

    const QList<QAbstractButton*> buttons = sizeButtonGroup->buttons();
    for (QAbstractButton *button : buttons) {
        const QStyle::PixelMetric metric = static_cast<QStyle::PixelMetric>(sizeButtonGroup->id(button));
        const int value = style->pixelMetric(metric);
        switch (metric) {
        case QStyle::PM_SmallIconSize:
            button->setText(tr("Small (%1 x %1)").arg(value));
            break;
        case QStyle::PM_LargeIconSize:
            button->setText(tr("Large (%1 x %1)").arg(value));
            break;
        case QStyle::PM_ToolBarIconSize:
            button->setText(tr("Toolbars (%1 x %1)").arg(value));
            break;
        case QStyle::PM_ListViewIconSize:
            button->setText(tr("List views (%1 x %1)").arg(value));
            break;
        case QStyle::PM_IconViewIconSize:
            button->setText(tr("Icon views (%1 x %1)").arg(value));
            break;
        case QStyle::PM_TabBarIconSize:
            button->setText(tr("Tab bars (%1 x %1)").arg(value));
            break;
        default:
            break;
        }
    }

    triggerChangeSize();
}
//! [4]

//! [5]
void MainWindow::changeSize(QAbstractButton *button, bool checked)
{
    if (!checked)
        return;

    const int index = sizeButtonGroup->id(button);
    const bool other = index == int(OtherSize);
    const int extent = other
        ? otherSpinBox->value()
        : QApplication::style()->pixelMetric(static_cast<QStyle::PixelMetric>(index));

    previewArea->setSize(QSize(extent, extent));
    otherSpinBox->setEnabled(other);
}

void MainWindow::triggerChangeSize()
{
    changeSize(sizeButtonGroup->checkedButton(), true);
}
//! [5]

//! [6]
void MainWindow::changeIcon()
{
    QIcon icon;

    for (int row = 0; row < imagesTable->rowCount(); ++row) {
        const QTableWidgetItem *fileItem = imagesTable->item(row, 0);
        const QTableWidgetItem *modeItem = imagesTable->item(row, 1);
        const QTableWidgetItem *stateItem = imagesTable->item(row, 2);

        if (fileItem->checkState() == Qt::Checked) {
            const int modeIndex = IconPreviewArea::iconModeNames().indexOf(modeItem->text());
            Q_ASSERT(modeIndex >= 0);
            const int stateIndex = IconPreviewArea::iconStateNames().indexOf(stateItem->text());
            Q_ASSERT(stateIndex >= 0);
            const QIcon::Mode mode = IconPreviewArea::iconModes().at(modeIndex);
            const QIcon::State state = IconPreviewArea::iconStates().at(stateIndex);
//! [6]

//! [8]
            const QString fileName = fileItem->data(Qt::UserRole).toString();
            QImage image(fileName);
            if (!image.isNull())
                icon.addPixmap(QPixmap::fromImage(image), mode, state);
//! [8]
        }
    }
//! [11]
    previewArea->setIcon(icon);
//! [11]
}

void MainWindow::addSampleImages()
{
    addImages(QLatin1String(SRCDIR) + QLatin1String("/images"));
}

void MainWindow::addOtherImages()
{
    static bool firstInvocation = true;
    QString directory;
    if (firstInvocation) {
        firstInvocation = false;
        directory = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).value(0, QString());
    }
    addImages(directory);
}

//! [12]
void MainWindow::addImages(const QString &directory)
{
    QFileDialog fileDialog(this, tr("Open Images"), directory);
    QStringList mimeTypeFilters;
    const QList<QByteArray> mimeTypes = QImageReader::supportedMimeTypes();
    for (const QByteArray &mimeTypeName : mimeTypes)
        mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    fileDialog.setMimeTypeFilters(mimeTypeFilters);
    fileDialog.selectMimeTypeFilter(QLatin1String("image/png"));
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setFileMode(QFileDialog::ExistingFiles);
    if (!nativeFileDialogAct->isChecked())
        fileDialog.setOption(QFileDialog::DontUseNativeDialog);
    if (fileDialog.exec() == QDialog::Accepted)
        loadImages(fileDialog.selectedFiles());
//! [12]
}

void MainWindow::loadImages(const QStringList &fileNames)
{
    for (const QString &fileName : fileNames) {
        const int row = imagesTable->rowCount();
        imagesTable->setRowCount(row + 1);
//! [13]
        const QFileInfo fileInfo(fileName);
        const QString imageName = fileInfo.baseName();
        const QString fileName2x = fileInfo.absolutePath()
            + QLatin1Char('/') + imageName + QLatin1String("@2x.") + fileInfo.suffix();
        const QFileInfo fileInfo2x(fileName2x);
        const QImage image(fileName);
        const QString toolTip =
            tr("Directory: %1\nFile: %2\nFile@2x: %3\nSize: %4x%5")
               .arg(QDir::toNativeSeparators(fileInfo.absolutePath()), fileInfo.fileName())
               .arg(fileInfo2x.exists() ? fileInfo2x.fileName() : tr("<None>"))
               .arg(image.width()).arg(image.height());
        QTableWidgetItem *fileItem = new QTableWidgetItem(imageName);
        fileItem->setData(Qt::UserRole, fileName);
        fileItem->setIcon(QPixmap::fromImage(image));
        fileItem->setFlags((fileItem->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
        fileItem->setToolTip(toolTip);
//! [13]

//! [15]
        QIcon::Mode mode = QIcon::Normal;
        QIcon::State state = QIcon::Off;
        if (guessModeStateAct->isChecked()) {
            if (imageName.contains(QLatin1String("_act"), Qt::CaseInsensitive))
                mode = QIcon::Active;
            else if (imageName.contains(QLatin1String("_dis"), Qt::CaseInsensitive))
                mode = QIcon::Disabled;
            else if (imageName.contains(QLatin1String("_sel"), Qt::CaseInsensitive))
                mode = QIcon::Selected;

            if (imageName.contains(QLatin1String("_on"), Qt::CaseInsensitive))
                state = QIcon::On;
//! [15]
        }

//! [18]
        imagesTable->setItem(row, 0, fileItem);
        QTableWidgetItem *modeItem =
            new QTableWidgetItem(IconPreviewArea::iconModeNames().at(IconPreviewArea::iconModes().indexOf(mode)));
        modeItem->setToolTip(toolTip);
        imagesTable->setItem(row, 1, modeItem);
        QTableWidgetItem *stateItem =
            new QTableWidgetItem(IconPreviewArea::iconStateNames().at(IconPreviewArea::iconStates().indexOf(state)));
        stateItem->setToolTip(toolTip);
        imagesTable->setItem(row, 2, stateItem);
        imagesTable->openPersistentEditor(modeItem);
        imagesTable->openPersistentEditor(stateItem);

        fileItem->setCheckState(Qt::Checked);
//! [18]
    }
}

//! [20]
void MainWindow::removeAllImages()
{
    imagesTable->setRowCount(0);
    changeIcon();
}
//! [20]

//! [21]
QWidget *MainWindow::createImagesGroupBox()
{
    QGroupBox *imagesGroupBox = new QGroupBox(tr("Images"));

    imagesTable = new QTableWidget;
    imagesTable->setSelectionMode(QAbstractItemView::NoSelection);
    imagesTable->setItemDelegate(new ImageDelegate(this));
//! [21]

//! [22]
    const QStringList labels({tr("Image"), tr("Mode"), tr("State")});

    imagesTable->horizontalHeader()->setDefaultSectionSize(90);
    imagesTable->setColumnCount(3);
    imagesTable->setHorizontalHeaderLabels(labels);
    imagesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    imagesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    imagesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    imagesTable->verticalHeader()->hide();
//! [22]

//! [24]
    connect(imagesTable, &QTableWidget::itemChanged,
            this, &MainWindow::changeIcon);

    QVBoxLayout *layout = new QVBoxLayout(imagesGroupBox);
    layout->addWidget(imagesTable);
    return imagesGroupBox;
//! [24]
}

//! [26]
QWidget *MainWindow::createIconSizeGroupBox()
{
    QGroupBox *iconSizeGroupBox = new QGroupBox(tr("Icon Size"));

    sizeButtonGroup = new QButtonGroup(this);
    sizeButtonGroup->setExclusive(true);

    connect(sizeButtonGroup, &QButtonGroup::buttonToggled,
            this, &MainWindow::changeSize);

    QRadioButton *smallRadioButton = new QRadioButton;
    sizeButtonGroup->addButton(smallRadioButton, QStyle::PM_SmallIconSize);
    QRadioButton *largeRadioButton = new QRadioButton;
    sizeButtonGroup->addButton(largeRadioButton, QStyle::PM_LargeIconSize);
    QRadioButton *toolBarRadioButton = new QRadioButton;
    sizeButtonGroup->addButton(toolBarRadioButton, QStyle::PM_ToolBarIconSize);
    QRadioButton *listViewRadioButton = new QRadioButton;
    sizeButtonGroup->addButton(listViewRadioButton, QStyle::PM_ListViewIconSize);
    QRadioButton *iconViewRadioButton = new QRadioButton;
    sizeButtonGroup->addButton(iconViewRadioButton, QStyle::PM_IconViewIconSize);
    QRadioButton *tabBarRadioButton = new QRadioButton;
    sizeButtonGroup->addButton(tabBarRadioButton, QStyle::PM_TabBarIconSize);
    QRadioButton *otherRadioButton = new QRadioButton(tr("Other:"));
    sizeButtonGroup->addButton(otherRadioButton, OtherSize);
    otherSpinBox = new IconSizeSpinBox;
    otherSpinBox->setRange(8, 256);
    const QString spinBoxToolTip =
        tr("Enter a custom size within %1..%2")
           .arg(otherSpinBox->minimum()).arg(otherSpinBox->maximum());
    otherSpinBox->setValue(64);
    otherSpinBox->setToolTip(spinBoxToolTip);
    otherRadioButton->setToolTip(spinBoxToolTip);
//! [26]

//! [27]
    connect(otherSpinBox, &QSpinBox::valueChanged,
            this, &MainWindow::triggerChangeSize);

    QHBoxLayout *otherSizeLayout = new QHBoxLayout;
    otherSizeLayout->addWidget(otherRadioButton);
    otherSizeLayout->addWidget(otherSpinBox);
    otherSizeLayout->addStretch();

    QGridLayout *layout = new QGridLayout(iconSizeGroupBox);
    layout->addWidget(smallRadioButton, 0, 0);
    layout->addWidget(largeRadioButton, 1, 0);
    layout->addWidget(toolBarRadioButton, 2, 0);
    layout->addWidget(listViewRadioButton, 0, 1);
    layout->addWidget(iconViewRadioButton, 1, 1);
    layout->addWidget(tabBarRadioButton, 2, 1);
    layout->addLayout(otherSizeLayout, 3, 0, 1, 2);
    layout->setRowStretch(4, 1);
    return iconSizeGroupBox;
//! [27]
}

void MainWindow::screenChanged()
{
    devicePixelRatioLabel->setText(QString::number(devicePixelRatio()));
    if (const QWindow *window = windowHandle()) {
        const QScreen *screen = window->screen();
        const QString screenDescription =
            tr("\"%1\" (%2x%3)").arg(screen->name())
               .arg(screen->geometry().width()).arg(screen->geometry().height());
        screenNameLabel->setText(screenDescription);
    }
    changeIcon();
}

QWidget *MainWindow::createHighDpiIconSizeGroupBox()
{
    QGroupBox *highDpiGroupBox = new QGroupBox(tr("High DPI Scaling"));
    QFormLayout *layout = new QFormLayout(highDpiGroupBox);
    devicePixelRatioLabel = new QLabel(highDpiGroupBox);
    screenNameLabel = new QLabel(highDpiGroupBox);
    layout->addRow(tr("Screen:"), screenNameLabel);
    layout->addRow(tr("Device pixel ratio:"), devicePixelRatioLabel);
    return highDpiGroupBox;
}

//! [28]
void MainWindow::createActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    addSampleImagesAct = new QAction(tr("Add &Sample Images..."), this);
    addSampleImagesAct->setShortcut(tr("Ctrl+A"));
    connect(addSampleImagesAct, &QAction::triggered, this, &MainWindow::addSampleImages);
    fileMenu->addAction(addSampleImagesAct);

    addOtherImagesAct = new QAction(tr("&Add Images..."), this);
    addOtherImagesAct->setShortcut(QKeySequence::Open);
    connect(addOtherImagesAct, &QAction::triggered, this, &MainWindow::addOtherImages);
    fileMenu->addAction(addOtherImagesAct);

    removeAllImagesAct = new QAction(tr("&Remove All Images"), this);
    removeAllImagesAct->setShortcut(tr("Ctrl+R"));
    connect(removeAllImagesAct, &QAction::triggered,
            this, &MainWindow::removeAllImages);
    fileMenu->addAction(removeAllImagesAct);

    fileMenu->addSeparator();

    QAction *exitAct = fileMenu->addAction(tr("&Quit"), qApp, &QCoreApplication::quit);
    exitAct->setShortcuts(QKeySequence::Quit);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    styleActionGroup = new QActionGroup(this);
    const QStringList styleKeys = QStyleFactory::keys();
    for (const QString &styleName : styleKeys) {
        QAction *action = new QAction(tr("%1 Style").arg(styleName), styleActionGroup);
        action->setData(styleName);
        action->setCheckable(true);
        connect(action, &QAction::triggered, this, &MainWindow::changeStyle);
        viewMenu->addAction(action);
    }

    QMenu *settingsMenu = menuBar()->addMenu(tr("&Settings"));

    guessModeStateAct = new QAction(tr("&Guess Image Mode/State"), this);
    guessModeStateAct->setCheckable(true);
    guessModeStateAct->setChecked(true);
    settingsMenu->addAction(guessModeStateAct);

    nativeFileDialogAct = new QAction(tr("&Use Native File Dialog"), this);
    nativeFileDialogAct->setCheckable(true);
    nativeFileDialogAct->setChecked(true);
    settingsMenu->addAction(nativeFileDialogAct);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About"), this, &MainWindow::about);
    helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
}
//! [28]

//! [30]
void MainWindow::createContextMenu()
{
    imagesTable->setContextMenuPolicy(Qt::ActionsContextMenu);
    imagesTable->addAction(addSampleImagesAct);
    imagesTable->addAction(addOtherImagesAct);
    imagesTable->addAction(removeAllImagesAct);
}
//! [30]

//! [31]
void MainWindow::checkCurrentStyle()
{
    const QList<QAction *> actions = styleActionGroup->actions();
    for (QAction *action : actions) {
        const QString styleName = action->data().toString();
        const std::unique_ptr<QStyle> candidate{QStyleFactory::create(styleName)};
        Q_ASSERT(candidate);
        if (candidate->metaObject()->className()
                == QApplication::style()->metaObject()->className()) {
            action->trigger();
            return;
        }
    }
}
//! [31]
