// Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QT3DRENDER_QPAINTEDTEXTURE_P_H
#define QT3DRENDER_QPAINTEDTEXTURE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <Qt3DRender/private/qabstracttextureimage_p.h>
#include <Qt3DRender/qtextureimagedatagenerator.h>
#include <Qt3DRender/qpaintedtextureimage.h>
#include <Qt3DRender/private/qt3drender_global_p.h>

QT_BEGIN_NAMESPACE

class QImage;
class QPainter;

namespace Qt3DRender {

class Q_3DRENDERSHARED_PRIVATE_EXPORT QPaintedTextureImagePrivate : public QAbstractTextureImagePrivate
{
public:
    QPaintedTextureImagePrivate();
    ~QPaintedTextureImagePrivate();

    Q_DECLARE_PUBLIC(QPaintedTextureImage)

    QSize m_imageSize;
    qreal m_devicePixelRatio;
    QScopedPointer<QImage> m_image;
    QTextureImageDataGeneratorPtr m_currentGenerator;

    // gets increased each time the image is re-painted.
    // used to distinguish between different generators
    quint64 m_generation;

    void repaint();
};

class QPaintedTextureImageDataGenerator : public QTextureImageDataGenerator
{
public:
    QPaintedTextureImageDataGenerator(const QImage &image, int gen, Qt3DCore::QNodeId texId);
    ~QPaintedTextureImageDataGenerator();

    // Will be executed from within a QAspectJob
    QTextureImageDataPtr operator ()() final;
    bool operator ==(const QTextureImageDataGenerator &other) const final;

    QT3D_FUNCTOR(QPaintedTextureImageDataGenerator)

private:
    QImage m_image;
    quint64 m_generation;
    Qt3DCore::QNodeId m_paintedTextureImageId;
};

} // namespace Qt3DRender

QT_END_NAMESPACE

#endif // QT3DRENDER_QPAINTEDTEXTURE_P_H
