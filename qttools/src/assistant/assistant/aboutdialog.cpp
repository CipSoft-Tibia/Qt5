// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aboutdialog.h"

#include "helpviewer.h"
#include "tracer.h"

#include <QtCore/QBuffer>

#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLayout>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtGui/QDesktopServices>
#include <QtGui/QScreen>

QT_BEGIN_NAMESPACE

AboutLabel::AboutLabel(QWidget *parent)
    : QTextBrowser(parent)
{
    TRACE_OBJ
    setFrameStyle(QFrame::NoFrame);
    QPalette p;
    p.setColor(QPalette::Base, p.color(QPalette::Window));
    setPalette(p);
}

void AboutLabel::setText(const QString &text, const QByteArray &resources)
{
    TRACE_OBJ
    QDataStream in(resources);
    in >> m_resourceMap;

    QTextBrowser::setText(text);
}

QSize AboutLabel::minimumSizeHint() const
{
    TRACE_OBJ
    QTextDocument *doc = document();
    doc->adjustSize();
    return QSize(int(doc->size().width()), int(doc->size().height()));
}

QVariant AboutLabel::loadResource(int type, const QUrl &name)
{
    TRACE_OBJ
    if (type == 2 || type == 3) {
        if (m_resourceMap.contains(name.toString())) {
            return m_resourceMap.value(name.toString());
        }
    }
    return QVariant();
}

void AboutLabel::doSetSource(const QUrl &url, QTextDocument::ResourceType type)
{
    TRACE_OBJ
    Q_UNUSED(type);
    if (url.isValid() && (!HelpViewer::isLocalUrl(url)
    || !HelpViewer::canOpenPage(url.path()))) {
        if (!QDesktopServices::openUrl(url)) {
            QMessageBox::warning(this, tr("Warning"),
                tr("Unable to launch external application."), QMessageBox::Close);
        }
    }
}

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint |
        Qt::WindowTitleHint|Qt::WindowSystemMenuHint)
{
    TRACE_OBJ
    m_pixmapLabel = nullptr;
    m_aboutLabel = new AboutLabel();

    m_closeButton = new QPushButton();
    m_closeButton->setText(tr("&Close"));
    connect(m_closeButton, &QAbstractButton::clicked, this, &QWidget::close);

    m_layout = new QGridLayout(this);
    m_layout->addWidget(m_aboutLabel, 1, 0, 1, -1);
    m_layout->addItem(new QSpacerItem(20, 10, QSizePolicy::Minimum,
        QSizePolicy::Fixed), 2, 1, 1, 1);
    m_layout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding), 3, 0, 1, 1);
    m_layout->addWidget(m_closeButton, 3, 1, 1, 1);
    m_layout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding), 3, 2, 1, 1);
}

void AboutDialog::setText(const QString &text, const QByteArray &resources)
{
    TRACE_OBJ
    m_aboutLabel->setText(text, resources);
    updateSize();
}

void AboutDialog::setPixmap(const QPixmap &pixmap)
{
    TRACE_OBJ
    if (!m_pixmapLabel) {
        m_pixmapLabel = new QLabel();
        m_layout->addWidget(m_pixmapLabel, 0, 0, 1, -1, Qt::AlignCenter);
    }
    m_pixmapLabel->setPixmap(pixmap);
    updateSize();
}

QString AboutDialog::documentTitle() const
{
    TRACE_OBJ
    return m_aboutLabel->documentTitle();
}

void AboutDialog::updateSize()
{
    TRACE_OBJ
    auto screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    const QSize screenSize = screen->availableSize();
    int limit = qMin(screenSize.width()/2, 500);

#ifdef Q_OS_MAC
    limit = qMin(screenSize.width()/2, 420);
#endif

    layout()->activate();
    int width = layout()->totalMinimumSize().width();

    if (width > limit)
        width = limit;

    QFontMetrics fm(qApp->font("QWorkspaceTitleBar"));
    int windowTitleWidth = qMin(fm.horizontalAdvance(windowTitle()) + 50, limit);
    if (windowTitleWidth > width)
        width = windowTitleWidth;

    layout()->activate();
    int height = (layout()->hasHeightForWidth())
        ? layout()->totalHeightForWidth(width)
        : layout()->totalMinimumSize().height();
    setFixedSize(width, height);
    QCoreApplication::removePostedEvents(this, QEvent::LayoutRequest);
}

QT_END_NAMESPACE
