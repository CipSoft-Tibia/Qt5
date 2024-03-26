// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickview.h"
#include "qquickview_p.h"

#include "qquickwindow_p.h"
#include "qquickitem_p.h"
#include "qquickitemchangelistener_p.h"

#include <QtQml/qqmlengine.h>
#include <private/qqmlengine_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <QtCore/qbasictimer.h>

#include <memory>

QT_BEGIN_NAMESPACE

void QQuickViewPrivate::init(QQmlEngine* e)
{
    Q_Q(QQuickView);

    engine = e;

    if (engine.isNull())
        engine = new QQmlEngine(q);

    QQmlEngine::setContextForObject(contentItem, engine.data()->rootContext());

    if (!engine.data()->incubationController())
        engine.data()->setIncubationController(q->incubationController());

    {
        // The content item has CppOwnership policy (set in QQuickWindow). Ensure the presence of a JS
        // wrapper so that the garbage collector can see the policy.
        QV4::ExecutionEngine *v4 = engine.data()->handle();
        QV4::QObjectWrapper::wrap(v4, contentItem);
    }
}

QQuickViewPrivate::QQuickViewPrivate()
    : component(nullptr), resizeMode(QQuickView::SizeViewToRootObject), initialSize(0,0)
{
}

QQuickViewPrivate::~QQuickViewPrivate()
{
}

void QQuickViewPrivate::execute()
{
    Q_Q(QQuickView);
    if (!engine) {
        qWarning() << "QQuickView: invalid qml engine.";
        return;
    }

    if (root)
        delete root;
    if (component) {
        delete component;
        component = nullptr;
    }
    if (!source.isEmpty()) {
        component = new QQmlComponent(engine.data(), source, q);
        if (!component->isLoading()) {
            q->continueExecute();
        } else {
            QObject::connect(component, SIGNAL(statusChanged(QQmlComponent::Status)),
                             q, SLOT(continueExecute()));
        }
    }
}

void QQuickViewPrivate::itemGeometryChanged(QQuickItem *resizeItem, QQuickGeometryChange change,
                                            const QRectF &oldGeometry)
{
    Q_Q(QQuickView);
    if (resizeItem == root && resizeMode == QQuickView::SizeViewToRootObject) {
        // wait for both width and height to be changed
        resizetimer.start(0,q);
    }
    QQuickItemChangeListener::itemGeometryChanged(resizeItem, change, oldGeometry);
}

/*!
    \class QQuickView
    \since 5.0
    \brief The QQuickView class provides a window for displaying a Qt Quick user interface.

    \inmodule QtQuick

    This is a convenience subclass of QQuickWindow which
    will automatically load and display a QML scene when given the URL of the main source file. Alternatively,
    you can instantiate your own objects using QQmlComponent and place them in a manually setup QQuickWindow.

    Typical usage:

    \snippet qquickview-ex.cpp 0

    To receive errors related to loading and executing QML with QQuickView,
    you can connect to the statusChanged() signal and monitor for QQuickView::Error.
    The errors are available via QQuickView::errors().

    QQuickView also manages sizing of the view and root object.  By default, the \l resizeMode
    is SizeViewToRootObject, which will load the component and resize it to the
    size of the view.  Alternatively the resizeMode may be set to SizeRootObjectToView which
    will resize the view to the size of the root object.

    \sa {Exposing Attributes of C++ Types to QML}, QQuickWidget
*/


/*! \fn void QQuickView::statusChanged(QQuickView::Status status)
    This signal is emitted when the component's current \a status changes.
*/

/*!
  Constructs a QQuickView with the given \a parent.
  The default value of \a parent is 0.

*/
QQuickView::QQuickView(QWindow *parent)
: QQuickWindow(*(new QQuickViewPrivate), parent)
{
    d_func()->init();
}

/*!
  Constructs a QQuickView with the given QML \a source and \a parent.
  The default value of \a parent is 0.

*/
QQuickView::QQuickView(const QUrl &source, QWindow *parent)
    : QQuickView(parent)
{
    setSource(source);
}

/*!
  Constructs a QQuickView with the given QML \a engine and \a parent.

  Note: In this case, the QQuickView does not own the given \a engine object;
  it is the caller's responsibility to destroy the engine. If the \a engine is deleted
  before the view, status() will return QQuickView::Error.

  \sa Status, status(), errors()
*/
QQuickView::QQuickView(QQmlEngine* engine, QWindow *parent)
    : QQuickWindow(*(new QQuickViewPrivate), parent)
{
    Q_ASSERT(engine);
    d_func()->init(engine);
}

/*!
    \internal
*/
QQuickView::QQuickView(const QUrl &source, QQuickRenderControl *control)
    : QQuickWindow(*(new QQuickViewPrivate), control)
{
    d_func()->init();
    setSource(source);
}

/*!
  Destroys the QQuickView.
*/
QQuickView::~QQuickView()
{
    // Ensure that the component is destroyed before the engine; the engine may
    // be a child of the QQuickViewPrivate, and will be destroyed by its dtor
    Q_D(QQuickView);
    delete d->root;
}

/*!
  \property QQuickView::source
  \brief The URL of the source of the QML component.

  Ensure that the URL provided is full and correct, in particular, use
  \l QUrl::fromLocalFile() when loading a file from the local filesystem.

  Note that setting a source URL will result in the QML component being
  instantiated, even if the URL is unchanged from the current value.
*/

/*!
    Sets the source to the \a url, loads the QML component and instantiates it.

    Ensure that the URL provided is full and correct, in particular, use
    \l QUrl::fromLocalFile() when loading a file from the local filesystem.

    Calling this method multiple times with the same url will result
    in the QML component being reinstantiated.
 */
void QQuickView::setSource(const QUrl& url)
{
    Q_D(QQuickView);
    d->source = url;
    d->execute();
}

/*!
   Sets the initial properties \a initialProperties with which the QML
   component gets initialized after calling \l QQuickView::setSource().

   \snippet qquickview-ex.cpp 1

   \note You can only use this function to initialize top-level properties.
   \note This function should always be called before setSource, as it has
   no effect once the component has become \c Ready.

   \sa QQmlComponent::createWithInitialProperties()
   \since 5.14
*/
void QQuickView::setInitialProperties(const QVariantMap &initialProperties)
{
    Q_D(QQuickView);
    d->initialProperties = initialProperties;
}

/*!
    \internal

    Set the source \a url, \a component and content \a item (root of the QML object hierarchy) directly.
 */
void QQuickView::setContent(const QUrl& url, QQmlComponent *component, QObject* item)
{
    Q_D(QQuickView);
    d->source = url;
    d->component = component;

    if (d->component && d->component->isError()) {
        const QList<QQmlError> errorList = d->component->errors();
        for (const QQmlError &error : errorList) {
            QMessageLogger(error.url().toString().toLatin1().constData(), error.line(), nullptr).warning()
                    << error;
        }
        emit statusChanged(status());
        return;
    }

    if (!d->setRootObject(item))
        delete item;
    emit statusChanged(status());
}

/*!
  Returns the source URL, if set.

  \sa setSource()
 */
QUrl QQuickView::source() const
{
    Q_D(const QQuickView);
    return d->source;
}

/*!
  Returns a pointer to the QQmlEngine used for instantiating
  QML Components.
 */
QQmlEngine* QQuickView::engine() const
{
    Q_D(const QQuickView);
    return d->engine ? const_cast<QQmlEngine *>(d->engine.data()) : nullptr;
}

/*!
  This function returns the root of the context hierarchy.  Each QML
  component is instantiated in a QQmlContext.  QQmlContext's are
  essential for passing data to QML components.  In QML, contexts are
  arranged hierarchically and this hierarchy is managed by the
  QQmlEngine.
 */
QQmlContext* QQuickView::rootContext() const
{
    Q_D(const QQuickView);
    return d->engine ? d->engine.data()->rootContext() : nullptr;
}

/*!
    \enum QQuickView::Status
    Specifies the loading status of the QQuickView.

    \value Null This QQuickView has no source set.
    \value Ready This QQuickView has loaded and created the QML component.
    \value Loading This QQuickView is loading network data.
    \value Error One or more errors has occurred. Call errors() to retrieve a list
           of errors.
*/

/*! \enum QQuickView::ResizeMode

  This enum specifies how to resize the view.

  \value SizeViewToRootObject The view resizes with the root item in the QML.
  \value SizeRootObjectToView The view will automatically resize the root item to the size of the view.
*/

/*!
    \property QQuickView::status
    The component's current \l{QQuickView::Status} {status}.
*/

QQuickView::Status QQuickView::status() const
{
    Q_D(const QQuickView);
    if (!d->engine)
        return QQuickView::Error;

    if (!d->component)
        return QQuickView::Null;

    if (d->component->status() == QQmlComponent::Ready && !d->root)
        return QQuickView::Error;

    return QQuickView::Status(d->component->status());
}

/*!
    Return the list of errors that occurred during the last compile or create
    operation.  When the status is not Error, an empty list is returned.
*/
QList<QQmlError> QQuickView::errors() const
{
    Q_D(const QQuickView);
    QList<QQmlError> errs;

    if (d->component)
        errs = d->component->errors();

    if (!d->engine) {
        QQmlError error;
        error.setDescription(QLatin1String("QQuickView: invalid qml engine."));
        errs << error;
    } else if (d->component && d->component->status() == QQmlComponent::Ready && !d->root) {
        QQmlError error;
        error.setDescription(QLatin1String("QQuickView: invalid root object."));
        errs << error;
    }

    return errs;
}

/*!
    \property QQuickView::resizeMode
    \brief whether the view should resize the window contents

    If this property is set to SizeViewToRootObject (the default), the view
    resizes to the size of the root item in the QML.

    If this property is set to SizeRootObjectToView, the view will
    automatically resize the root item to the size of the view.

    \sa initialSize()
*/

void QQuickView::setResizeMode(ResizeMode mode)
{
    Q_D(QQuickView);
    if (d->resizeMode == mode)
        return;

    if (d->root) {
        if (d->resizeMode == SizeViewToRootObject) {
            QQuickItemPrivate *p = QQuickItemPrivate::get(d->root);
            p->removeItemChangeListener(d, QQuickItemPrivate::Geometry);
        }
    }

    d->resizeMode = mode;
    if (d->root) {
        d->initResize();
    }
}

void QQuickViewPrivate::initResize()
{
    if (root) {
        if (resizeMode == QQuickView::SizeViewToRootObject) {
            QQuickItemPrivate *p = QQuickItemPrivate::get(root);
            p->addItemChangeListener(this, QQuickItemPrivate::Geometry);
        }
    }
    updateSize();
}

void QQuickViewPrivate::updateSize()
{
    Q_Q(QQuickView);
    if (!root)
        return;

    if (resizeMode == QQuickView::SizeViewToRootObject) {
        QSize newSize = QSize(root->width(), root->height());
        if (newSize.isValid() && newSize != q->size()) {
            q->resize(newSize);
        }
    } else if (resizeMode == QQuickView::SizeRootObjectToView) {
        bool needToUpdateWidth = !qFuzzyCompare(q->width(), root->width());
        bool needToUpdateHeight = !qFuzzyCompare(q->height(), root->height());

        if (needToUpdateWidth && needToUpdateHeight)
            root->setSize(QSizeF(q->width(), q->height()));
        else if (needToUpdateWidth)
            root->setWidth(q->width());
        else if (needToUpdateHeight)
            root->setHeight(q->height());
    }
}

QSize QQuickViewPrivate::rootObjectSize() const
{
    QSize rootObjectSize(0,0);
    int widthCandidate = -1;
    int heightCandidate = -1;
    if (root) {
        widthCandidate = root->width();
        heightCandidate = root->height();
    }
    if (widthCandidate > 0) {
        rootObjectSize.setWidth(widthCandidate);
    }
    if (heightCandidate > 0) {
        rootObjectSize.setHeight(heightCandidate);
    }
    return rootObjectSize;
}

QQuickView::ResizeMode QQuickView::resizeMode() const
{
    Q_D(const QQuickView);
    return d->resizeMode;
}

/*!
  \internal
 */
void QQuickView::continueExecute()
{
    Q_D(QQuickView);
    disconnect(d->component, SIGNAL(statusChanged(QQmlComponent::Status)), this, SLOT(continueExecute()));

    if (d->component->isError()) {
        const QList<QQmlError> errorList = d->component->errors();
        for (const QQmlError &error : errorList) {
            QMessageLogger(error.url().toString().toLatin1().constData(), error.line(), nullptr).warning()
                    << error;
        }
        emit statusChanged(status());
        return;
    }

    std::unique_ptr<QObject> obj(d->initialProperties.empty()
                                 ? d->component->create()
                                 : d->component->createWithInitialProperties(d->initialProperties));

    if (d->component->isError()) {
        const QList<QQmlError> errorList = d->component->errors();
        for (const QQmlError &error : errorList) {
            QMessageLogger(error.url().toString().toLatin1().constData(), error.line(), nullptr).warning()
                    << error;
        }
        emit statusChanged(status());
        return;
    }

    if (d->setRootObject(obj.get()))
        Q_UNUSED(obj.release());
    emit statusChanged(status());
}


/*!
  \internal

  Sets \a obj as root object and returns true if that operation succeeds.
  Otherwise returns \c false. If \c false is returned, the root object is
  \c nullptr afterwards. You can explicitly set the root object to nullptr,
  and the return value will be \c true.
*/
bool QQuickViewPrivate::setRootObject(QObject *obj)
{
    Q_Q(QQuickView);
    if (root == obj)
        return true;

    delete root;
    if (obj == nullptr)
        return true;

    if (QQuickItem *sgItem = qobject_cast<QQuickItem *>(obj)) {
        root = sgItem;
        root->setFlag(QQuickItem::ItemIsViewport);
        sgItem->setParentItem(q->QQuickWindow::contentItem());
        QQml_setParent_noEvent(sgItem, q->QQuickWindow::contentItem());
        initialSize = rootObjectSize();
        if ((resizeMode == QQuickView::SizeViewToRootObject || q->width() <= 1 || q->height() <= 1) &&
            initialSize != q->size()) {
            q->resize(initialSize);
        }
        initResize();
        return true;
    }

    if (qobject_cast<QWindow *>(obj)) {
        qWarning() << "QQuickView does not support using a window as a root item." << Qt::endl
                   << Qt::endl
                   << "If you wish to create your root window from QML, consider using QQmlApplicationEngine instead." << Qt::endl;
        return false;
    }

    qWarning() << "QQuickView only supports loading of root objects that derive from QQuickItem." << Qt::endl
               << Qt::endl
               << "Ensure your QML code is written for QtQuick 2, and uses a root that is or" << Qt::endl
               << "inherits from QtQuick's Item (not a Timer, QtObject, etc)." << Qt::endl;
    return false;
}

/*!
  \internal
  If the \l {QTimerEvent} {timer event} \a e is this
  view's resize timer, sceneResized() is emitted.
 */
void QQuickView::timerEvent(QTimerEvent* e)
{
    Q_D(QQuickView);
    if (!e || e->timerId() == d->resizetimer.timerId()) {
        d->updateSize();
        d->resizetimer.stop();
    }
}

/*!
    \internal
    Preferred size follows the root object geometry.
*/
QSize QQuickView::sizeHint() const
{
    Q_D(const QQuickView);
    QSize rootObjectSize = d->rootObjectSize();
    if (rootObjectSize.isEmpty()) {
        return size();
    } else {
        return rootObjectSize;
    }
}

/*!
  Returns the initial size of the root object.

  If \l resizeMode is QQuickItem::SizeRootObjectToView the root object will be
  resized to the size of the view.  initialSize contains the size of the
  root object before it was resized.
*/
QSize QQuickView::initialSize() const
{
    Q_D(const QQuickView);
    return d->initialSize;
}

/*!
  Returns the view's root \l {QQuickItem} {item}.
 */
QQuickItem *QQuickView::rootObject() const
{
    Q_D(const QQuickView);
    return d->root;
}

/*!
  \internal
  This function handles the \l {QResizeEvent} {resize event}
  \a e.
 */
void QQuickView::resizeEvent(QResizeEvent *e)
{
    Q_D(QQuickView);
    if (d->resizeMode == SizeRootObjectToView)
        d->updateSize();

    QQuickWindow::resizeEvent(e);
}

/*! \reimp */
void QQuickView::keyPressEvent(QKeyEvent *e)
{
    QQuickWindow::keyPressEvent(e);
}

/*! \reimp */
void QQuickView::keyReleaseEvent(QKeyEvent *e)
{
    QQuickWindow::keyReleaseEvent(e);
}

/*! \reimp */
void QQuickView::mouseMoveEvent(QMouseEvent *e)
{
    QQuickWindow::mouseMoveEvent(e);
}

/*! \reimp */
void QQuickView::mousePressEvent(QMouseEvent *e)
{
    QQuickWindow::mousePressEvent(e);
}

/*! \reimp */
void QQuickView::mouseReleaseEvent(QMouseEvent *e)
{
    QQuickWindow::mouseReleaseEvent(e);
}


QT_END_NAMESPACE

#include "moc_qquickview.cpp"
