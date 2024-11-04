// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef Q3DSCENE_H
#define Q3DSCENE_H

#if 0
#  pragma qt_class(Q3DScene)
#endif

#include <QtCore/QObject>
#include <QtCore/QRect>
#include <QtGraphs/qgraphsglobal.h>
#include <QtQmlIntegration/qqmlintegration.h>

QT_BEGIN_NAMESPACE

class Q3DScenePrivate;

class QT_TECH_PREVIEW_API Q_GRAPHS_EXPORT Q3DScene : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Q3DScene)
    Q_PROPERTY(QRect viewport READ viewport NOTIFY viewportChanged)
    Q_PROPERTY(QRect primarySubViewport READ primarySubViewport WRITE setPrimarySubViewport NOTIFY
                   primarySubViewportChanged)
    Q_PROPERTY(QRect secondarySubViewport READ secondarySubViewport WRITE setSecondarySubViewport
                   NOTIFY secondarySubViewportChanged)
    Q_PROPERTY(QPoint selectionQueryPosition READ selectionQueryPosition WRITE
                   setSelectionQueryPosition NOTIFY selectionQueryPositionChanged)
    Q_PROPERTY(bool secondarySubviewOnTop READ isSecondarySubviewOnTop WRITE
                   setSecondarySubviewOnTop NOTIFY secondarySubviewOnTopChanged)
    Q_PROPERTY(
        bool slicingActive READ isSlicingActive WRITE setSlicingActive NOTIFY slicingActiveChanged)
    Q_PROPERTY(float devicePixelRatio READ devicePixelRatio WRITE setDevicePixelRatio NOTIFY
                   devicePixelRatioChanged)
    Q_PROPERTY(QPoint graphPositionQuery READ graphPositionQuery WRITE setGraphPositionQuery NOTIFY
                   graphPositionQueryChanged)
    Q_PROPERTY(QPoint invalidSelectionPoint READ invalidSelectionPoint CONSTANT)

    QML_NAMED_ELEMENT(Scene3D)
    QML_UNCREATABLE("Trying to create uncreatable: Scene3D.")

public:
    explicit Q3DScene(QObject *parent = nullptr);
    ~Q3DScene() override;

    QRect viewport() const;

    QRect primarySubViewport() const;
    void setPrimarySubViewport(const QRect &primarySubViewport);
    bool isPointInPrimarySubView(const QPoint &point);

    QRect secondarySubViewport() const;
    void setSecondarySubViewport(const QRect &secondarySubViewport);
    bool isPointInSecondarySubView(const QPoint &point);

    void setSelectionQueryPosition(const QPoint &point);
    const QPoint selectionQueryPosition() const;

    void setGraphPositionQuery(const QPoint &point);
    QPoint graphPositionQuery() const;

    void setSlicingActive(bool isSlicing);
    bool isSlicingActive() const;

    void setSecondarySubviewOnTop(bool isSecondaryOnTop);
    bool isSecondarySubviewOnTop() const;

    float devicePixelRatio() const;
    void setDevicePixelRatio(float pixelRatio);

    QPoint invalidSelectionPoint() const;

Q_SIGNALS:
    void viewportChanged(const QRect &viewport);
    void primarySubViewportChanged(const QRect &subViewport);
    void secondarySubViewportChanged(const QRect &subViewport);
    void secondarySubviewOnTopChanged(bool isSecondaryOnTop);
    void slicingActiveChanged(bool isSlicingActive);
    void devicePixelRatioChanged(float pixelRatio);
    void selectionQueryPositionChanged(const QPoint &position);
    void graphPositionQueryChanged(const QPoint &position);
    void needRender();

private:
    Q_DISABLE_COPY(Q3DScene)

    friend class QAbstract3DGraph;
    friend class QQuickGraphsItem;
};

QT_END_NAMESPACE

#endif
