// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "exportdialog.h"

#include <QtWidgets>
#include <QSvgRenderer>

#include "svgview.h"

static inline QString picturesLocation()
{
    return QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).value(0, QDir::currentPath());
}

MainWindow::MainWindow()
    : QMainWindow()
    , m_view(new SvgView)
    , m_zoomLabel(new QLabel)
{
    QToolBar *toolBar = new QToolBar(this);
    addToolBar(Qt::TopToolBarArea, toolBar);

    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    const QIcon openIcon = QIcon::fromTheme("document-open", QIcon(":/qt-project.org/styles/commonstyle/images/standardbutton-open-32.png"));
    QAction *openAction = fileMenu->addAction(openIcon, tr("&Open..."), this, &MainWindow::openFile);
    openAction->setShortcut(QKeySequence::Open);
    toolBar->addAction(openAction);
    const QIcon exportIcon = QIcon::fromTheme("document-save", QIcon(":/qt-project.org/styles/commonstyle/images/standardbutton-save-32.png"));
    QAction *exportAction = fileMenu->addAction(exportIcon, tr("&Export..."), this, &MainWindow::exportImage);
    exportAction->setToolTip(tr("Export Image"));
    exportAction->setShortcut(Qt::CTRL | Qt::Key_E);
    toolBar->addAction(exportAction);
    QAction *quitAction = fileMenu->addAction(tr("E&xit"), qApp, QCoreApplication::quit);
    quitAction->setShortcuts(QKeySequence::Quit);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    m_backgroundAction = viewMenu->addAction(tr("&Background"));
    m_backgroundAction->setEnabled(false);
    m_backgroundAction->setCheckable(true);
    m_backgroundAction->setChecked(false);
    connect(m_backgroundAction, &QAction::toggled, m_view, &SvgView::setViewBackground);

    m_outlineAction = viewMenu->addAction(tr("&Outline"));
    m_outlineAction->setEnabled(false);
    m_outlineAction->setCheckable(true);
    m_outlineAction->setChecked(true);
    connect(m_outlineAction, &QAction::toggled, m_view, &SvgView::setViewOutline);

    viewMenu->addSeparator();
    QAction *zoomAction = viewMenu->addAction(tr("Zoom &In"), m_view, &SvgView::zoomIn);
    zoomAction->setShortcut(QKeySequence::ZoomIn);
    zoomAction = viewMenu->addAction(tr("Zoom &Out"), m_view, &SvgView::zoomOut);
    zoomAction->setShortcut(QKeySequence::ZoomOut);
    zoomAction = viewMenu->addAction(tr("Reset Zoom"), m_view, &SvgView::resetZoom);
    zoomAction->setShortcut(Qt::CTRL | Qt::Key_0);

    QMenu *rendererMenu = menuBar()->addMenu(tr("&Renderer"));
    m_nativeAction = rendererMenu->addAction(tr("&Native"));
    m_nativeAction->setCheckable(true);
    m_nativeAction->setChecked(true);
    m_nativeAction->setData(int(SvgView::Native));
#ifdef USE_OPENGLWIDGETS
    m_glAction = rendererMenu->addAction(tr("&OpenGL"));
    m_glAction->setCheckable(true);
    m_glAction->setData(int(SvgView::OpenGL));
#endif
    m_imageAction = rendererMenu->addAction(tr("&Image"));
    m_imageAction->setCheckable(true);
    m_imageAction->setData(int(SvgView::Image));

    rendererMenu->addSeparator();
    m_antialiasingAction = rendererMenu->addAction(tr("&Antialiasing"));
    m_antialiasingAction->setCheckable(true);
    m_antialiasingAction->setChecked(false);
    connect(m_antialiasingAction, &QAction::toggled, m_view, &SvgView::setAntialiasing);

    QActionGroup *rendererGroup = new QActionGroup(this);
    rendererGroup->addAction(m_nativeAction);
#ifdef USE_OPENGLWIDGETS
    rendererGroup->addAction(m_glAction);
#endif
    rendererGroup->addAction(m_imageAction);

    menuBar()->addMenu(rendererMenu);

    connect(rendererGroup, &QActionGroup::triggered,
            [this] (QAction *a) { setRenderer(a->data().toInt()); });

    QMenu *help = menuBar()->addMenu(tr("&Help"));
    help->addAction(tr("About Qt"), qApp, &QApplication::aboutQt);

    setCentralWidget(m_view);

    m_zoomLabel->setToolTip(tr("Use the mouse wheel to zoom"));
    statusBar()->addPermanentWidget(m_zoomLabel);
    updateZoomLabel();
    connect(m_view, &SvgView::zoomChanged, this, &MainWindow::updateZoomLabel);
}

void MainWindow::openFile()
{
    QFileDialog fileDialog(this);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setMimeTypeFilters(QStringList() << "image/svg+xml" << "image/svg+xml-compressed");
    fileDialog.setWindowTitle(tr("Open SVG File"));
    if (m_currentPath.isEmpty())
        fileDialog.setDirectory(picturesLocation());

    while (fileDialog.exec() == QDialog::Accepted && !loadFile(fileDialog.selectedFiles().constFirst()))
        ;
}

bool MainWindow::loadFile(const QString &fileName)
{
    if (!QFileInfo::exists(fileName) || !m_view->openFile(fileName)) {
        QMessageBox::critical(this, tr("Open SVG File"),
                              tr("Could not open file '%1'.").arg(QDir::toNativeSeparators(fileName)));
        return false;
    }

    if (!fileName.startsWith(":/")) {
        m_currentPath = fileName;
        setWindowFilePath(fileName);
        const QSize size = m_view->svgSize();
        const QString message =
            tr("Opened %1, %2x%3").arg(QFileInfo(fileName).fileName()).arg(size.width()).arg(size.width());
        statusBar()->showMessage(message);
    }

    m_outlineAction->setEnabled(true);
    m_backgroundAction->setEnabled(true);

    const QSize availableSize = this->screen()->availableGeometry().size();
    resize(m_view->sizeHint().expandedTo(availableSize / 4) + QSize(80, 80 + menuBar()->height()));

    return true;
}

void MainWindow::setRenderer(int renderMode)
{
    m_view->setRenderer(static_cast<SvgView::RendererType>(renderMode));
}

void MainWindow::exportImage()
{
    ExportDialog exportDialog(this);
    exportDialog.setExportSize(m_view->svgSize());
    QString fileName;
    if (m_currentPath.isEmpty()) {
        fileName = picturesLocation() + QLatin1String("/export.png");
    } else {
        const QFileInfo fi(m_currentPath);
        fileName = fi.absolutePath() + QLatin1Char('/') + fi.baseName() + QLatin1String(".png");
    }
    exportDialog.setExportFileName(fileName);

    while (true) {
        if (exportDialog.exec() != QDialog::Accepted)
            break;

        const QSize imageSize = exportDialog.exportSize();
        QImage image(imageSize, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter painter;
        painter.begin(&image);
        m_view->renderer()->render(&painter, QRectF(QPointF(), QSizeF(imageSize)));
        painter.end();

        const QString fileName = exportDialog.exportFileName();
        if (image.save(fileName)) {

            const QString message = tr("Exported %1, %2x%3, %4 bytes")
                .arg(QDir::toNativeSeparators(fileName)).arg(imageSize.width()).arg(imageSize.height())
                .arg(QFileInfo(fileName).size());
            statusBar()->showMessage(message);
            break;
        } else {
            QMessageBox::critical(this, tr("Export Image"),
                                  tr("Could not write file '%1'.").arg(QDir::toNativeSeparators(fileName)));
        }
    }
}

void MainWindow::updateZoomLabel()
{
    const int percent = qRound(m_view->zoomFactor() * qreal(100));
    m_zoomLabel->setText(QString::number(percent) + QLatin1Char('%'));
}
