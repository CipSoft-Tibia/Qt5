// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsvgwidget.h"

#include <qsvgrenderer.h>

#include "qstyleoption.h"
#include "qpainter.h"
#include <QtWidgets/private/qwidget_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QSvgWidget
    \inmodule QtSvgWidgets
    \ingroup painting

    \brief The QSvgWidget class provides a widget that is used to display the contents of
    Scalable Vector Graphics (SVG) files.
    \since 4.1

    This class enables developers to display SVG drawings alongside standard widgets, and
    is used in much the same way as QLabel is used for displaying text and bitmap images.

    Since QSvgWidget is a subclass of QWidget, SVG drawings are rendered using the properties
    of the display. More control can be exercised over the rendering process with the
    QSvgRenderer class, as this can be used to paint onto other paint devices, such as QImage
    and QGLWidget. The renderer used by the widget can be obtained with the renderer()
    function.

    Each QSvgWidget can be constructed with the file name of a SVG file, or they can be
    constructed without a specific file to render and one can be supplied later. The load()
    functions provide two different ways to load an SVG file: they accept either the file name
    of an SVG file or a QByteArray containing the serialized XML representation of an SVG file.

    By default, the widget provides a size hint to reflect the size of the drawing that it
    displays. If no data has been loaded, the widget provides the default QWidget size hint.
    Subclass this class and reimplement sizeHint() if you need to customize this behavior.

    \sa QSvgRenderer, {Qt SVG C++ Classes}, QPicture
*/

class QSvgWidgetPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QSvgWidget)
public:
    QSvgRenderer *renderer;
};

/*!
    Constructs a new SVG display widget with the given \a parent.
*/
QSvgWidget::QSvgWidget(QWidget *parent)
    : QWidget(*new QSvgWidgetPrivate, parent, {})
{
    d_func()->renderer = new QSvgRenderer(this);
    QObject::connect(d_func()->renderer, SIGNAL(repaintNeeded()),
                     this, SLOT(update()));
}

/*!
    Constructs a new SVG display widget with the given \a parent and loads the contents
    of the specified \a file.
*/
QSvgWidget::QSvgWidget(const QString &file, QWidget *parent)
    : QWidget(*new QSvgWidgetPrivate, parent, {})
{
    d_func()->renderer = new QSvgRenderer(file, this);
    QObject::connect(d_func()->renderer, SIGNAL(repaintNeeded()),
                     this, SLOT(update()));
}

/*!
    Destroys the widget.
*/
QSvgWidget::~QSvgWidget()
{

}

/*!
    Returns the renderer used to display the contents of the widget.
*/
QSvgRenderer * QSvgWidget::renderer() const
{
    Q_D(const QSvgWidget);
    return d->renderer;
}


/*!
    \reimp
*/
QSize QSvgWidget::sizeHint() const
{
    Q_D(const QSvgWidget);
    if (d->renderer->isValid())
        return d->renderer->defaultSize();
    else
        return QSize(128, 64);
}


/*!
    \reimp
*/
void QSvgWidget::paintEvent(QPaintEvent *)
{
    Q_D(QSvgWidget);
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    d->renderer->render(&p);
}

/*!
    Loads the contents of the specified SVG \a file and updates the widget.
*/
void QSvgWidget::load(const QString &file)
{
    Q_D(const QSvgWidget);
    d->renderer->load(file);
}

/*!
    Loads the specified SVG format \a contents and updates the widget.
*/
void QSvgWidget::load(const QByteArray &contents)
{
    Q_D(const QSvgWidget);
    d->renderer->load(contents);
}

QT_END_NAMESPACE
