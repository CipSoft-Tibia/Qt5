// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QTGRAPHS_QSURFACE3DSERIES_H
#define QTGRAPHS_QSURFACE3DSERIES_H

#include <QtGraphs/qabstract3dseries.h>
#include <QtGraphs/qsurfacedataproxy.h>

QT_BEGIN_NAMESPACE

class QSurface3DSeriesPrivate;

class Q_GRAPHS_EXPORT QSurface3DSeries : public QAbstract3DSeries
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSurface3DSeries)
    Q_PROPERTY(QSurfaceDataProxy *dataProxy READ dataProxy WRITE setDataProxy NOTIFY
                   dataProxyChanged FINAL)
    Q_PROPERTY(
        QPoint selectedPoint READ selectedPoint WRITE setSelectedPoint NOTIFY selectedPointChanged)
    Q_PROPERTY(bool flatShadingSupported READ isFlatShadingSupported NOTIFY
                   flatShadingSupportedChanged FINAL)
    Q_PROPERTY(QSurface3DSeries::DrawFlags drawMode READ drawMode WRITE setDrawMode NOTIFY
                   drawModeChanged FINAL)
    Q_PROPERTY(QSurface3DSeries::Shading shading READ shading WRITE setShading NOTIFY shadingChanged)
    Q_PROPERTY(QImage texture READ texture WRITE setTexture NOTIFY textureChanged FINAL)
    Q_PROPERTY(
        QString textureFile READ textureFile WRITE setTextureFile NOTIFY textureFileChanged FINAL)
    Q_PROPERTY(QColor wireframeColor READ wireframeColor WRITE setWireframeColor NOTIFY
                   wireframeColorChanged FINAL)
    Q_PROPERTY(
        QSurfaceDataArray dataArray READ dataArray WRITE setDataArray NOTIFY dataArrayChanged FINAL)
    QML_ELEMENT
    QML_UNCREATABLE("Trying to create uncreatable: QSurface3DSeries, use Surface3DSeries instead.")

public:
    enum DrawFlag {
        DrawWireframe = 0x1,
        DrawSurface = 0x2,
        DrawSurfaceAndWireframe = DrawWireframe | DrawSurface,
        DrawFilledSurface = 0x4,
    };
    Q_FLAG(DrawFlag)
    Q_DECLARE_FLAGS(DrawFlags, DrawFlag)

    enum class Shading { Smooth, Flat };
    Q_ENUM(Shading)

    explicit QSurface3DSeries(QObject *parent = nullptr);
    explicit QSurface3DSeries(QSurfaceDataProxy *dataProxy, QObject *parent = nullptr);
    ~QSurface3DSeries() override;

    void setDataProxy(QSurfaceDataProxy *proxy);
    QSurfaceDataProxy *dataProxy() const;

    void setSelectedPoint(QPoint position);
    QPoint selectedPoint() const;
    static QPoint invalidSelectionPosition();

    void setShading(const QSurface3DSeries::Shading shading);
    QSurface3DSeries::Shading shading() const;

    void setDrawMode(QSurface3DSeries::DrawFlags mode);
    QSurface3DSeries::DrawFlags drawMode() const;

    bool isFlatShadingSupported() const;

    void setTexture(const QImage &texture);
    QImage texture() const;
    void setTextureFile(const QString &filename);
    QString textureFile() const;

    void setWireframeColor(QColor color);
    QColor wireframeColor() const;

    void setDataArray(const QSurfaceDataArray &newDataArray);
    void clearRow(qsizetype rowIndex);
    void clearArray();
    const QSurfaceDataArray &dataArray() const &;
    QSurfaceDataArray dataArray() &&;

Q_SIGNALS:
    void dataProxyChanged(QSurfaceDataProxy *proxy);
    void selectedPointChanged(QPoint position);
    void flatShadingSupportedChanged(bool enabled);
    void drawModeChanged(QSurface3DSeries::DrawFlags mode);
    void textureChanged(const QImage &image);
    void textureFileChanged(const QString &filename);
    void wireframeColorChanged(QColor color);
    void dataArrayChanged(const QSurfaceDataArray &array);
    void shadingChanged(const Shading shading);

protected:
    explicit QSurface3DSeries(QSurface3DSeriesPrivate &d, QObject *parent = nullptr);

private:
    Q_DISABLE_COPY(QSurface3DSeries)

    friend class QQuickGraphsSurface;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QSurface3DSeries::DrawFlags)

QT_END_NAMESPACE

#endif
