// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QABSTRACT3DINPUTHANDLER_H
#define QABSTRACT3DINPUTHANDLER_H

#include <QtGraphs/qgraphsglobal.h>
#include <QtGraphs/q3dscene.h>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtGui/QWheelEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QTouchEvent>

QT_BEGIN_NAMESPACE

class QAbstract3DInputHandlerPrivate;

class Q_GRAPHS_EXPORT QAbstract3DInputHandler : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QAbstract3DInputHandler)
    Q_PROPERTY(QAbstract3DInputHandler::InputView inputView READ inputView WRITE setInputView NOTIFY inputViewChanged)
    Q_PROPERTY(QPoint inputPosition READ inputPosition WRITE setInputPosition NOTIFY positionChanged)
    Q_PROPERTY(Q3DScene *scene READ scene WRITE setScene NOTIFY sceneChanged)

public:
    enum InputView {
        InputViewNone = 0,
        InputViewOnPrimary,
        InputViewOnSecondary
    };
    Q_ENUM(InputView)

protected:
    explicit QAbstract3DInputHandler(QAbstract3DInputHandlerPrivate *d, QObject *parent = nullptr);
    explicit QAbstract3DInputHandler(QObject *parent = nullptr);

public:
    virtual ~QAbstract3DInputHandler();

    // Input event listeners
    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    virtual void touchEvent(QTouchEvent *event);
    virtual void mousePressEvent(QMouseEvent *event, const QPoint &mousePos);
    virtual void mouseReleaseEvent(QMouseEvent *event, const QPoint &mousePos);
    virtual void mouseMoveEvent(QMouseEvent *event, const QPoint &mousePos);
#if QT_CONFIG(wheelevent)
    virtual void wheelEvent(QWheelEvent *event);
#endif

    QAbstract3DInputHandler::InputView inputView() const;
    void setInputView(QAbstract3DInputHandler::InputView inputView);

    QPoint inputPosition() const;
    void setInputPosition(const QPoint &position, bool forceSelection = false);

    Q3DScene *scene() const;
    void setScene(Q3DScene *scene);

Q_SIGNALS:
    void positionChanged(const QPoint &position);
    void inputViewChanged(QAbstract3DInputHandler::InputView view);
    void sceneChanged(Q3DScene *scene);

public Q_SLOTS:
    void handleSelection(const QPoint &position);

protected:
    QScopedPointer<QAbstract3DInputHandlerPrivate> d_ptr;

    void setPrevDistance(int distance);
    int prevDistance() const;
    void setPreviousInputPos(const QPoint &position);
    QPoint previousInputPos() const;

private:
    Q_DISABLE_COPY(QAbstract3DInputHandler)

    friend class QQuickGraphsItem;
    friend class Q3DInputHandler;
    friend class Q3DTouchInputHandler;
};

QT_END_NAMESPACE

#endif
